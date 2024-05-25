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

void DEBUG_printBytesIn01s(u1 *startP, int size, int columns);
void printAllRegContents(u1 *reg_raw_bitsIN);
u1 *populateOperandString(char *stringPIN, Instr8086 *instructionPIN, u1 operandIndex);
u1 *getOperandP(Instr8086 *instructionPIN, u1 operandIndex);
u1 reg_raw_bits[20] = {0};//Simulation registers
//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16 17 18 19
//AL AH BL BH CL CH DL DH SP SP BP BP SI SI DI DI FLAGS IP IP

u1 *flagsP = reg_raw_bits+16, *ipP = reg_raw_bits+18;
u1 *registerPointers[16] =
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
{//Use a regCode4 as first argument.
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
u1 regCode4a; //Uses (reg<<1)|W. See table on p. 4-20
u1 regCode4b; //Uses (reg<<1)|W. See table on p. 4-20
bool W, D;
u2 immediateValue;

u1 instrSz;
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
	u1 inBytes[fInSz];
	for(int i=0 ; i<fInSz ; i++)
	{
		inBytes[i] = fgetc(fInP);
	}
	fclose(fInP);
	u1 *placeInBinary = inBytes;
	//Labels for .asm
	//unsigned int labelCount=0;
	//unsigned int labelIndex;
	//bool labelExists;
	//unsigned int labelIndices[fInSz];

	//In debug mode, print all bits of bin, in 8 columns.
	DEBUG_PRINT("\n\"%s\" binary size is %i.\nRaw binary contents:\n\n", argv[1], fInSz);
	DEBUG_printBytesIn01s((u1*)fInP, fInSz, 8);
	DEBUG_PRINT("\n\n");

	//Initial register print
	printf("\nStarting memory:\n\n");
	printAllRegContents(reg_raw_bits);
		
	//MASTER LOOP. Each iteration disassembles one instruction.
	while(bytesDone < fInSz)
	{
		decodeBinaryAndPopulateInstruction(instructionDataP, placeInBinary);
		
		//Choose simulation behavior
		switch(instructionDataP->mnemonicType)
		{
		case MNEM_MOV:
			//Get pointers to the values we'll be operating on.
			u1 *destP = getOperandP(instructionDataP, 0);
			u1 *sourceP = getOperandP(instructionDataP, 1);;
			//Operate!
			if(instructionDataP->W)
			{
				//Word mov.
				*(u2*)destP = *(u2*)sourceP;
			}
			else
			{
				//Byte mov.
				*destP = *sourceP;
			}
			char opStringsPrintFun[2][32];
			populateOperandString(opStringsPrintFun[0], instructionDataP, 0);
			populateOperandString(opStringsPrintFun[1], instructionDataP, 1);
			printf
			(
				"Below is register contents after operation: mov %s, %s\n\n",
				opStringsPrintFun[0],
				opStringsPrintFun[1]
			);
			break;
		}
		printAllRegContents(reg_raw_bits);
		bytesDone += instructionDataP->instrSize;
		placeInBinary += instructionDataP->instrSize;
	}//End of disassembling
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

void printAllRegContents(u1 *reg_raw_bitsIN)
{
	DEBUG_PRINT("AL       AH       BL       BH       CL       CH       DL       DH\n");
	DEBUG_printBytesIn01s(reg_raw_bitsIN, 8, 8);
	DEBUG_PRINT("SP                BP                SI                DI\n");
	DEBUG_printBytesIn01s(reg_raw_bitsIN+8, 8, 8);
	printf("FLAGS             IP\n");
	DEBUG_printBytesIn01s(flagsP, 2, 0);
	DEBUG_printBytesIn01s(ipP, 2, 0);
	DEBUG_PRINT("\n\n");
}
void DEBUG_printBytesIn01s(u1 *startP, int size, int columns)
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
u1 *getOperandP(Instr8086 *instructionPIN, u1 operandIndex)
{
	//Note: Depends on global registerPointers array.
	bool W = instructionPIN->W;
	u1 registerIndex;
	switch(instructionPIN->operandTypes[operandIndex])
	{
	case OPERAND_TYPE_IMMEDITATE:
		return (u1*)&instructionPIN->operandValues[operandIndex];
	case OPERAND_TYPE_REG:
		registerIndex = (instructionPIN->operandValues[operandIndex] << 1) | W;
		return registerPointers[registerIndex];
	case OPERAND_TYPE_RM:
		registerIndex = (instructionPIN->operandValues[operandIndex] << 1) | W;
		return registerPointers[registerIndex];
		//TODO: We can't do memory yet!
	}
}
u1 *populateOperandString(char *stringPIN, Instr8086 *instructionPIN, u1 operandIndex)
{
	//Note: Depends on global registerNames array.
	char stringOUT[32];
	bool W = instructionPIN->W;
	u1 registerIndex;
	u2 *immP; //Points at instr's immediate value
	switch(instructionPIN->operandTypes[operandIndex])
	{
	case OPERAND_TYPE_IMMEDITATE:
		u2 valueOUT;
		immP = &instructionPIN->operandValues[operandIndex];
		valueOUT = W ? *(u2*)immP : *(u1*)immP;
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
