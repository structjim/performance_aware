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
s8 *getOperandP(Instr8086 *instructionPIN, u8 operandIndex);
u8 reg_raw_bits[20] = {0};//Simulation registers
// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
// AL AH BL BH CL CH DL DH SP SP BP BP SI SI DI DI FLAGS IP IP

u8 *flagsP = reg_raw_bits+16;
u16 *ipP = (u16*)(reg_raw_bits+18);
//FLAGS bits: Zero, Parity, Sign, Overflow, AF, ...

s8 *registerPointers[16] =
{
	//Pointers to raw bits where the registers live.
	//Arranged to have incices like table on page 4-20.
	/*AL*/(s8*)reg_raw_bits,      /*AX*/(s8*)reg_raw_bits,
	/*CL*/(s8*)reg_raw_bits+4,    /*CX*/(s8*)reg_raw_bits+4,
	/*DL*/(s8*)reg_raw_bits+6,    /*DX*/(s8*)reg_raw_bits+6,
	/*BL*/(s8*)reg_raw_bits+2,    /*BX*/(s8*)reg_raw_bits+2,
	/*AH*/(s8*)reg_raw_bits+1,    /*SP*/(s8*)reg_raw_bits+8,
	/*CH*/(s8*)reg_raw_bits+5,    /*BP*/(s8*)reg_raw_bits+10,
	/*DH*/(s8*)reg_raw_bits+6,    /*SI*/(s8*)reg_raw_bits+12,
	/*BH*/(s8*)reg_raw_bits+3,    /*DI*/(s8*)reg_raw_bits+14
};
char registerNames[16][3]=
{//Use the 4 bits (reg<<1)|W as first argument.
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
s16 immediateValue;
s16 compareAUX;

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
	DEBUG_printBytesIn01s(inBytes, fInSz, 8);
	DEBUG_PRINT("\n\n");

	//Initial register print
	printf("Starting memory:\n\n");
	printAllRegContents(reg_raw_bits);

	int STOPPER = 20;
		
	//MASTER LOOP. Each iteration disassembles one instruction.
	while(*ipP < fInSz && STOPPER--)
	{
		s8 *destP, *sourceP;
		char opStringsPrintFun[2][32];

		//Reset zero flag. TODO: This is premature!
		//*flagsP = (*flagsP & ~FLAG_MASK_ZERO);
		//Decode instruction into struct
		decodeBinaryAndPopulateInstruction(instructionDataP, placeInBinary + *ipP);
		//Move instruction pointer
		*ipP += instructionDataP->size;
		
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
				*(s8*)destP = *(s8*)sourceP;
			}
			*flagsP = *flagsP & 0b00011111; //Mask out math flags
			//Print
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf("Below is register contents after operation: mov %s, %s ",
				   opStringsPrintFun[0], opStringsPrintFun[1]);
			printf("(Size: %i)\n", instructionDataP->size);
			printf("Binary: ");
			DEBUG_printBytesIn01s(placeInBinary + *ipP-instructionDataP->size, instructionDataP->size, 8);
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
			}
			else
			{	//Byte
				*(s8*)destP += *(s8*)sourceP;
			}
			setZeroFlag_vw(flagsP, destP, instructionDataP->W);
			//Set sign flag...
			if( (*(destP+instructionDataP->W))>>7 )
			{	//Yes sign flag
				*flagsP = *flagsP | FLAG_MASK_SIGN;
			}
			else
			{	//No sign flag
				*flagsP = *flagsP & (~FLAG_MASK_SIGN);
			}
			//Print
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf("Below is register contents after operation: add %s, %s ",
				   opStringsPrintFun[0], opStringsPrintFun[1]);
			printf("(Size: %i)\n", instructionDataP->size);
			printf("Binary: ");
			DEBUG_printBytesIn01s(placeInBinary + *ipP-instructionDataP->size, instructionDataP->size, 8);
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
			}
			else
			{	//Byte
				*(s8*)destP -= *(s8*)sourceP;
			}
			setZeroFlag_vw(flagsP, destP, instructionDataP->W);
			//Set sign flag...
			if( (*(destP+instructionDataP->W))>>7 )
			{	//Yes sign flag
				*flagsP = *flagsP | FLAG_MASK_SIGN;
			}
			else
			{	//No sign flag
				*flagsP = *flagsP & (~FLAG_MASK_SIGN);
			}
			//Print
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf("Below is register contents after operation: sub %s, %s ",
				   opStringsPrintFun[0], opStringsPrintFun[1]);
			printf("(Size: %i)\n", instructionDataP->size);
			printf("Binary: ");
			DEBUG_printBytesIn01s(placeInBinary + *ipP-instructionDataP->size, instructionDataP->size, 8);
			printf("\n\n");
			break;
		case MNEM_CMP:
			//Get pointers to operand values.
			destP = getOperandP(instructionDataP, 0);
			sourceP = getOperandP(instructionDataP, 1);;
			//Operate!
			if(instructionDataP->W)
			{	//Word
				compareAUX = *(s16*)destP - *(s16*)sourceP;
			}
			else
			{	//Byte
				compareAUX = *(s8*)destP - *(s8*)sourceP;
			}
			setZeroFlag_vw(flagsP, &compareAUX, instructionDataP->W);
			//Set sign flag...
			if( (*(destP+instructionDataP->W))>>7 )
			{	//Yes sign flag
				*flagsP = *flagsP | FLAG_MASK_SIGN;
			}
			else
			{	//No sign flag
				*flagsP = *flagsP & (~FLAG_MASK_SIGN);
			}
			//Print
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf("Below is register contents after operation: cmp %s, %s ",
				   opStringsPrintFun[0], opStringsPrintFun[1]);
			printf("(Size: %i)\n", instructionDataP->size);
			printf("Binary: ");
			DEBUG_printBytesIn01s(placeInBinary + *ipP-instructionDataP->size, instructionDataP->size, 8);
			printf("\n\n");
			break;
		case MNEM_JNZ:
			if( !((*flagsP)>>7) )
			{
				*ipP += instructionDataP->operandValues[0];
				printf("Jumped %i.\n\n", instructionDataP->operandValues[0]);
			}
			else
			{
				printf("Didn't jump.\n\n");
			}
			break;
		default:
			printf("[[[SIMULATION ERROR! Next 6 bytes on instruction stream:]]]\n   ");
			DEBUG_printBytesIn01s(placeInBinary + *ipP-instructionDataP->size, 6, 0);
			printf("\n\n");
		}
		//bytesDone += instructionDataP->size;
		printAllRegContents(reg_raw_bits);
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
		bytesWritten += sizes[i];
		
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
	{	//Labels and hex
		DEBUG_PRINT("%cX         0x%04x ", 'A'+i, *(u16*)(reg_raw_bitsIN+i+i));
	}
	DEBUG_PRINT("\n  ");	
	//Print 8-bit registers (AL, AH, BL, BH, CL, CH, DL, DH)
	for(u8 i=0 ; i<8 ; i++)
	{	//Labels and hex
		DEBUG_PRINT("%c%c  0x%02x ", 'A'+(i/2), i%2?'H':'L', *(u8*)(reg_raw_bitsIN+i));
	}	
	DEBUG_PRINT("\n  ");
	DEBUG_printBytesIn01s(reg_raw_bitsIN, 8, 8);
	DEBUG_PRINT("\n");

	DEBUG_PRINT("  ");
	
	//Print the other 16-bit reg. (SP, BP, SI, DI, FLAGS, IP)
	char *labels[4] = {"SP", "BP", "SI", "DI"};
	for(u8 i=0 ; i<4 ; i++)
	{	//Labels and hex
		DEBUG_PRINT("%s         0x%04x ", labels[i], *(u16*)(reg_raw_bitsIN+8+i+i));
	}
	DEBUG_PRINT("\n  ");
	DEBUG_printBytesIn01s(reg_raw_bitsIN+8, 8, 8);
	DEBUG_PRINT("\n");

	//The other two things (FLAGS and IP)
	DEBUG_PRINT("  FLAGS      0x%04x IP         0x%04x\n  ", *(u16*)flagsP, *(u16*)ipP);
	DEBUG_printBytesIn01s(flagsP, 2, 0);
	DEBUG_printBytesIn01s((u8*)ipP, 2, 0);
	DEBUG_PRINT("\n (zps     )\n\n");
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
s8 *getOperandP(Instr8086 *instructionPIN, u8 operandIndex)
{
	//Note: Depends on global registerPointers array.
	bool W = instructionPIN->W;
	u8 registerIndex;
	switch(instructionPIN->operandTypes[operandIndex])
	{
	case OPERAND_TYPE_IMMEDIATE:
		return (s8*)&instructionPIN->operandValues[operandIndex];
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
	case OPERAND_TYPE_IMMEDIATE:
		s16 valueOUT;
		immediateValueP = &(instructionPIN->operandValues[operandIndex]);
		valueOUT = W ? *(s16*)immediateValueP : *(s8*)immediateValueP;
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
