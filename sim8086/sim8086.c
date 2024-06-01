/*	========================================================================
	(C) Copyright 2024 by structJim, All Rights Reserved.
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the author(s) be held liable for any
	damages arising from the use of this software.
	========================================================================
	Sim8086
	A (pretty minimal, let's be real) 8086 simulator. This was made while
	following along with Casey Muratori's "Performance Aware Programming"
	course, which is hosted at https://www.ComputerEnhance.com.
	========================================================================
	Instructions: Just use the BAR script and give it a .asm file. Or...
	1. Build in GCC with c99 standard.
	2. Pass an 8086 binary.
	3. (Hopefully) see some stuff happen.
	4. TODO: Get a .asm file back, maybe? I mean, this would be a great time
	to rewrite the disassembler. But we'll just have to wait and see if that
	materalizes...
	========================================================================*/

#define DEBUG_PRINT if(DEBUG)printf
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include"../HOCM/Decode8086.HOCM.h"
#include"../HOCM/JimsTypedefs.HOCM.h"
bool DEBUG=1;

const u8 FLAG_MASK_ZERO = 1<<7;
const u8 FLAG_MASK_PARITY = 1<<6;
const u8 FLAG_MASK_SIGN = 1<<5;

void setZeroFlag_vw(u8 *flagsPIN, void *valueIN, bool wIN);
void DEBUG_printBytesIn01s(u8 *startP, int size, int columns);
void printAllRegContents(u8 *reg_raw_bitsIN);
void populateOperandString(char *stringPIN, Instr8086 *instructionPIN, u8 operandIndex);
u8 *getOperandP(Instr8086 *instructionPIN, u8 operandIndex);
u8 reg_raw_bits[20] = {0};//Simulation registers
// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
// AL AH BL BH CL CH DL DH SP SP BP BP SI SI DI DI FLAGS IP IP

u8 *flagsP = reg_raw_bits+16, *ipP = reg_raw_bits+18;
//FLAGS bits: Zero, Parity, Sign, Overflow, AF, ...

u8 *registerPointers[16] =
{
	//Pointers to raw bits where the registers live.
	//Arranged to have incices like table on page 4-20.
	/*AL*/reg_raw_bits,      /*AX*/reg_raw_bits,
	/*CL*/reg_raw_bits+4,    /*CX*/reg_raw_bits+4,
	/*DL*/reg_raw_bits+6,    /*DX*/reg_raw_bits+6,
	/*BL*/reg_raw_bits+2,    /*BX*/reg_raw_bits+2,
	/*AH*/reg_raw_bits+1,    /*SP*/reg_raw_bits+8,
	/*CH*/reg_raw_bits+5,    /*BP*/reg_raw_bits+10,
	/*DH*/reg_raw_bits+6,    /*SI*/reg_raw_bits+12,
	/*BH*/reg_raw_bits+3,    /*DI*/reg_raw_bits+14
};
char registerNames[16][3]=
{//Use a regW4, aka (reg<<1)|W,  as first argument.
	"al", "ax",
	"cl", "cx",
	"dl", "dx",
	"bl", "bx",
	"ah", "sp",
	"ch", "bp",
	"dh", "si",
	"bh", "di"
};
Instr8086 *instructionDataP;
u8 regW4a; //Uses (reg<<1)|W, See table on p. 4-20
u8 regW4b; //Uses (reg<<1)|W. See table on p. 4-20
bool W, D;
u16 immediateValue;

u8 instrSz;
unsigned int instrsDone=0;
unsigned int bytesDone=0;
clock_t starttime;

int main(int argc, char *argv[])
{
	//starttime=clock();
	instructionDataP = (Instr8086*)malloc(sizeof(Instr8086));
	//Get file contents
	FILE *fInP = fopen(argv[1], "rb");
	fseek(fInP, 0L, SEEK_END); //Set file position to end
	int fInSz = ftell(fInP); //Store position (file size)
	fseek(fInP, 0L, SEEK_SET);//Return to start of file
	u8 inBytes[fInSz];
	for(int i=0 ; i<fInSz ; i++)
	{
		inBytes[i] = fgetc(fInP);
	}
	fclose(fInP);
	u8 *placeInBinary = inBytes;

	//In debug mode, print all bits of bin, in 8 columns.
	DEBUG_PRINT("\n\"%s\" binary size is %i.\nRaw binary contents:\n\n", argv[1], fInSz);
	DEBUG_printBytesIn01s((u8*)fInP, fInSz, 8);
	DEBUG_PRINT("\n\n");

	//Initial register print
	printf("Starting memory:\n\n");
	printAllRegContents(reg_raw_bits);
		
	//MASTER LOOP. Each iteration disassembles one instruction.
	while(bytesDone < fInSz)
	{
		u8 *destP, *sourceP;
		char opStringsPrintFun[2][32];

		//Reset zero flag.
		*flagsP = (*flagsP & ~FLAG_MASK_ZERO);
		
		//Decode instruction into struct
		decodeBinaryAndPopulateInstruction(instructionDataP, placeInBinary);
		
		//Select and execute simulation behavior
		switch(instructionDataP->mnemonicType)
		{
		case MNEM_MOV:
			//Get pointers to operand values.
			destP = getOperandP(instructionDataP, 0);
			sourceP = getOperandP(instructionDataP, 1);;
			//Operate!
			if(instructionDataP->W)
			{	//Word
				*(s16*)destP = *(s16*)sourceP;
			}
			else
			{	//Byte
				*destP = *sourceP;
			}
			//Print
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf("Below is register contents after operation: mov %s, %s ",
				   opStringsPrintFun[0], opStringsPrintFun[1]);
			printf("(Size: %i)\n", instructionDataP->instrSize);
			printf("Binary: ");
			DEBUG_printBytesIn01s(placeInBinary, instructionDataP->instrSize, 8);
			printf("\n\n");
			break;
		case MNEM_ADD:
			//Get pointers to operand values.
			destP = getOperandP(instructionDataP, 0);
			sourceP = getOperandP(instructionDataP, 1);
			//Operate!
			if(instructionDataP->W)
			{	//Word
				*(s16*)destP += *(s16*)sourceP;
				setZeroFlag_vw(flagsP, destP, instructionDataP->W);
			}
			else
			{	//Byte
				*destP += *sourceP;
				setZeroFlag_vw(flagsP, destP, instructionDataP->W);
			}
			//Print
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf("Below is register contents after operation: add %s, %s ",
				   opStringsPrintFun[0], opStringsPrintFun[1]);
			printf("(Size: %i)\n", instructionDataP->instrSize);
			printf("Binary: ");
			DEBUG_printBytesIn01s(placeInBinary, instructionDataP->instrSize, 8);
			printf("\n\n");
			break;
		case MNEM_SUB:
			//Get pointers to operand values.
			destP = getOperandP(instructionDataP, 0);
			sourceP = getOperandP(instructionDataP, 1);;
			//Operate!
			if(instructionDataP->W)
			{	//Word
				*(s16*)destP -= *(s16*)sourceP;
				setZeroFlag_vw(flagsP, destP, instructionDataP->W);
			}
			else
			{	//Byte
				*destP -= *sourceP;
				setZeroFlag_vw(flagsP, destP, instructionDataP->W);
			}
			//Print
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf("Below is register contents after operation: sub %s, %s ",
				   opStringsPrintFun[0], opStringsPrintFun[1]);
			printf("(Size: %i)\n", instructionDataP->instrSize);
			printf("Binary: ");
			DEBUG_printBytesIn01s(placeInBinary, instructionDataP->instrSize, 8);
			printf("\n\n");
			break;
		default:
			printf("[[[SIMULATION ERROR! Next 6 bytes on instruction stream:]]]\n   ");
			DEBUG_printBytesIn01s(placeInBinary, 6, 0);
			printf("\n\n");
		}
		printAllRegContents(reg_raw_bits);
		bytesDone += instructionDataP->instrSize;
		placeInBinary += instructionDataP->instrSize;
	}//End of decoding and simulation
	printf("\n");
/*	
	//Write to output file
	//FILE *fOutP = fopen(argv[2], "w");
	unsigned int bytesWritten = 0;
	unsigned int labelsWritten = 0;
	fprintf(fOutP, "%s", "bits 16"); //Bit width directive counts as 0 bytes
	for(int i=0 ; i<instrsDone ; i++)
	{//Each iteration writes 1 instr
		fprintf(fOutP, "\n%s", getIndexLL_S32(&instrStrings, i));
		bytesWritten += instrSizes[i];
		
		if(labelsWritten < labelCount)
		{
			//labelIndices[] are byte numbers that need labels after
			//Labels are named after the index of the preceding byte
			if(labelIndices[labelsWritten] == bytesWritten)
			{
				fprintf(fOutP, "\nlabel%i:", bytesWritten);
				labelsWritten++;
			}
		}
	}
	//Get output file size
	fseek(fOutP, 0, SEEK_END); //Set file position to end
	int fOutSz = ftell(fOutP); //Store offset (file out size)
	DEBUG_PRINT("Disassembly DONE.\n");
	DEBUG_PRINT("Total instructions: %i\n", instrsDone);
	DEBUG_PRINT("Total labels: %i\n", labelCount);
	DEBUG_PRINT("File IN size: %i\n", fInSz);
	DEBUG_PRINT("File OUT size: %i\n", fOutSz);
	//fclose(fOutP);
*/
	free(instructionDataP);
	return 0;
}//END MAIN

void setZeroFlag_vw(u8 *flagsPIN, void *valueIN, bool wIN)
{
	if(wIN)
	{	//Word
		if(*(s16*)valueIN)
		{	//Not zero
			*flagsPIN = *flagsPIN & 0b01111111;
		}
		else
		{	//Zero
			*flagsPIN = *flagsPIN | 0b10000000;
		}
	}
	else
	{	//Byte
		if((*(u8*)valueIN))
		{	//Not zero
			*flagsPIN = *flagsPIN & 0b01111111;
		}
		else
		{	//Zero
			*flagsPIN = *flagsPIN | 0b10000000;
		}
	}
}
void printAllRegContents(u8 *reg_raw_bitsIN)
{
	DEBUG_PRINT("  ");
	
	//First, print 16-bit reg. (AX, BX, CX, DX)
	for(u8 i=0 ; i<4 ; i++)
	{
		DEBUG_PRINT("%cX         0x%04x ", 'A'+i, *(u16*)(reg_raw_bitsIN+i+i));
	}
	DEBUG_PRINT("\n");

	DEBUG_PRINT("  ");
	
	//Print 8-bit registers (AL, AH, BL, BH, CL, CH, DL, DH)
	for(u8 i=0 ; i<8 ; i++)
	{
		DEBUG_PRINT("%c%c  0x%02x ", 'A'+(i/2), i%2?'H':'L', *(u8*)(reg_raw_bitsIN+i));
	}	
	DEBUG_PRINT("\n  ");
	DEBUG_printBytesIn01s(reg_raw_bitsIN, 8, 8);
	DEBUG_PRINT("\n");

	DEBUG_PRINT("  ");
	
	//Print the other 16-bit reg. (SP, BP, SI, DI, FLAGS, IP)
	char *labels[4] = {"SP", "BP", "SI", "DI"};
	for(u8 i=0 ; i<4 ; i++)
	{
		DEBUG_PRINT("%s         0x%04x ", labels[i], *(u16*)(reg_raw_bitsIN+i+i));
	}
	DEBUG_PRINT("\n  ");
	DEBUG_printBytesIn01s(reg_raw_bitsIN+8, 8, 8);
	DEBUG_PRINT("\n");

	//The other two things (FLAGS and IP)
	DEBUG_PRINT("  FLAGS      0x%04x IP         0x%04x\n  ", *(u16*)flagsP, *(u16*)ipP);
	DEBUG_printBytesIn01s(flagsP, 2, 0);
	DEBUG_printBytesIn01s(ipP, 2, 0);
	DEBUG_PRINT("\n\n");
}
void DEBUG_printBytesIn01s(u8 *startP, int size, int columns)
{
	for(int i=0 ; i<size ; i++)
	{
		///Each iteration prints 1 byte and a space
		for(int j=7 ; j>=0 ; j--)
		{
			///Each iteration prints 1 bit
			DEBUG_PRINT("%i", (startP[i] >> j) & 1);
		}
		
		DEBUG_PRINT(" ");
		if( columns!=0 && i!=0 && !((i+1)%columns) )
		{
			DEBUG_PRINT("\n");
		}
	}
}
u8 *getOperandP(Instr8086 *instructionPIN, u8 operandIndex)
{
	//Note: Depends on global registerPointers array.
	bool W = instructionPIN->W;
	u8 registerIndex;
	switch(instructionPIN->operandTypes[operandIndex])
	{
	case OPERAND_TYPE_IMMEDITATE:
		return (u8*)&instructionPIN->operandValues[operandIndex];
	case OPERAND_TYPE_REG:
		registerIndex = (instructionPIN->operandValues[operandIndex] << 1) | W;
		return registerPointers[registerIndex];
	case OPERAND_TYPE_RM:
		registerIndex = (instructionPIN->operandValues[operandIndex] << 1) | W;
		return registerPointers[registerIndex];
		//TODO: We can't do memory yet!
	default:
		printf("[[[ERROR SELECTING OPERAND TYPE IN getOperandP function!]]]\n");
		printf("[[[ERROR SELECTING OPERAND TYPE IN getOperandP function!]]]\n");
		printf("[[[ERROR SELECTING OPERAND TYPE IN getOperandP function!]]]\n\n");
		return NULL;
	}
}
void populateOperandString(char *stringPIN, Instr8086 *instructionPIN, u8 operandIndex)
{
	//Note: Depends on global registerNames array.
	bool W = instructionPIN->W;
	u8 registerIndex;
	s16 *immediateValueP; //Points at instr's immediate value
	switch(instructionPIN->operandTypes[operandIndex])
	{
	case OPERAND_TYPE_IMMEDITATE:
		s16 valueOUT;
		immediateValueP = &(instructionPIN->operandValues[operandIndex]);
		valueOUT = W ? *(s16*)immediateValueP : *(u8*)immediateValueP;
		sprintf(stringPIN, "%i", valueOUT);
		break;
	case OPERAND_TYPE_REG:
		registerIndex = (instructionPIN->operandValues[operandIndex] << 1) | W;
		sprintf(stringPIN, "%s", registerNames[registerIndex]);
		break;
	case OPERAND_TYPE_RM:
		registerIndex = (instructionPIN->operandValues[operandIndex] << 1) | W;
		sprintf(stringPIN, "%s", registerNames[registerIndex]);
		break;
		//TODO: We can't do memory yet!
	}
}
