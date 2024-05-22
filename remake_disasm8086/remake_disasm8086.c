/*	========================================================================
	(C) Copyright 2024 by structJim, All Rights Reserved.
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the author(s) be held liable for any
	damages arising from the use of this software.
	========================================================================
	Disasm 8086 REVENGEANCE
	An (albeit incomplete) Intel 8086 .asm disassembler. It gets kind of
	crazy in here. Pardon our dust! This is just for learning, so it's not
	gonna be strictly good. This was made while following along with Casey
	Muratori's "Performance Aware Programming" course, which is hosted at
	https://www.ComputerEnhance.com.
	========================================================================
	Instructions:
	1. Build in GCC with c99 standard.
	2. Pass an 8086 binary file and output destination to the resulting bin.
	3. Receive .asm file.
	4. Profit?
	========================================================================*/

#define DEBUG_PRINT if(DEBUG)printf
#define USE_BYTE false
#define USE_WORD true //word=2bytes
#include<stdio.h>
#include<stdbool.h>
#include<string.h>
#include<time.h>
#include"LinkedList_String32.HOCM.h"
bool DEBUG=1;

short valFromUCharP(unsigned char *charP, bool W);
void fillRegName(char regName[], char regBitsVal, bool W);
void fillRmName(char rmName[], char rmBitsVal, char modBitsVal, bool W, short disp);
void DEBUG_printBytesIn01s(unsigned char *startP, int size, int columns);

construct_LL_S32(instrStrings);
char instrStringAuxBuffer[32];
FILE *fInP, *fOutP;
unsigned int fInSz, bytesDone=0;
unsigned char *instrP;
//TODO: Might need to think about max int value eventually.

//Operands
enum{NO_OPERAND, OPERAND_REG_BIT11, OPERAND_RM_BIT14, OPERAND_IMM};
unsigned char operandTypes[2];
char operandStrings[2][8];
//Operation types:
char *mnemName;
unsigned char opType, modBitsValue, regBitsValue, rmBitsValue, srBitsVal;
enum{MOV_REG_RM};
bool D, W, V, S, hasWBit8, hasDBit7;
//Remember: D means reg is destination!

const char modMsgs[4][40]=
{
	//Since 00 and 11 are 0 disp, we can easily get disp size with modValue%3.
	"  MOD: 00 (Mem Mode, no disp)\n", "  MOD: 01 (Mem Mode, 8-bit disp)\n",
	"  MOD: 10 (Mem Mode, 16-bit disp)\n", "  MOD: 11 (Reg Mode, no disp)\n"
};

int main(int argc, char *argv[])
{
	//Get binary file and output destination.
	fInP = fopen(argv[1], "rb");
	fOutP = fopen(argv[2], "w");
	//Get binary file size
	fseek(fInP, 0L, SEEK_END); //Set file position to end
	fInSz = ftell(fInP); //Store position (file size)
	fseek(fInP, 0L, SEEK_SET);//Return to start of file
	//Get binary file contents
	//TODO: vvv Double-check if this copy is necessary. (Doubt it.)
	unsigned char inBytes[fInSz];
	for(int i=0 ; i<fInSz ; i++)
	{
		inBytes[i] = fgetc(fInP);
	}
	//For iterating through input
	instrP = inBytes;
	
	//Print binary, in 4 columns.
	DEBUG_PRINT("\n\"%s\" bin size is %i. Contents:\n\n", argv[1], fInSz);
	DEBUG_printBytesIn01s(inBytes, fInSz, 4);
	DEBUG_PRINT("\n\n");
	
	//Master loop. (One iteration decodes one instruction.)
	while(bytesDone < fInSz){
		printf("\nThe while started ok.\n");
		//6 leading opcode bits.
		switch(instrP[0]>>2)
		{
			case 0b100010:
				opType = MOV_REG_RM; //2~4
				mnemName = "mov";
				hasDBit7 = hasWBit8 = true;
				modBitsValue = (instrP[1] & 0b11000000)>>6;
				//D means reg is destination (AKA first operand).
				operandTypes[!D] = OPERAND_REG_BIT11;
				operandTypes[D] = OPERAND_RM_BIT14;
				break;
			default:
				opType = 69;
		}

		//Pre-string-building upkeep
		if(hasDBit7){
		}
		
		//Build instruction string.
		if(operandTypes[0] && operandTypes[1])
		{	//Two operands
			sprintf(instrStringAuxBuffer, "%s %s, %s", mnemName, operandStrings[0], operandStrings[1]);
		}
		else if(operandTypes[0] && !operandTypes[1])
		{	//One operand
		}
		else if(!operandTypes[0] && !operandTypes[1])
		{	//No operands
		}
		else{/*ERROR*/}
		
		appendLL_S32(&instrStrings, instrStringAuxBuffer);
		printf("%s\n", getIndexLL_S32(&instrStrings, 0));
		bytesDone+=2;
	}
	
/*
	int instrSizes[fInSz];

	//Write to output file
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
*/
	fclose(fInP);
	fclose(fOutP);
	construct_LL_S32(instrStrings);
	return 0;
}






void fillRegName(char regName[], char regBitsVal, bool W)
{
	//                W? 01 R/M
	char regNameMatrix[2][2][8]=
	{// 000  001  010  011  100  101  110  111   <- R/M bit vals
		'a', 'c', 'd', 'b', 'a', 'c', 'd', 'b',  // !w
		'l', 'l', 'l', 'l', 'h', 'h', 'h', 'h',
		
		'a', 'c', 'd', 'b', 's', 'b', 's', 'd',   // w
		'x', 'x', 'x', 'x', 'p', 'p', 'i', 'i'
	};
	
	regName[0] = regNameMatrix[(char)W][0][regBitsVal];
	regName[1] = regNameMatrix[(char)W][1][regBitsVal];
	regName[2] = '\0';
	
	DEBUG_PRINT("  REG name: %s\n", regName);
}
void fillRmName(char rmName[], char rmBitsVal, char modBitsVal, bool W, short dispDirectAccess)
{
	if(modBitsVal == 0b11)
	{
		fillRegName(rmName, rmBitsVal, W);
		DEBUG_PRINT("  ^Actually an R/M; it's merely REG-flavored!\n");
	}
	else if(modBitsVal==0b00 && rmBitsVal==0b110)
	{
		//The special case
		sprintf(rmName, "[%i]", dispDirectAccess);
	}
	else
	{
		//We know we aren't MOD=11, and we aren't the special DIRECT ADDRESS case.
		//Besides that, this block is MOD agnostic.
		//See table 4.10 on page 4.20.
		/**/ if (rmBitsVal==0b000){strcpy(rmName, "[bx + si");}
		else if(rmBitsVal==0b001){strcpy(rmName, "[bx + di");}
		else if(rmBitsVal==0b010){strcpy(rmName, "[bp + si");}
		else if(rmBitsVal==0b011){strcpy(rmName, "[bp + di");}
		else if(rmBitsVal==0b100){strcpy(rmName, "[si");}
		else if(rmBitsVal==0b101){strcpy(rmName, "[di");}
		else if(rmBitsVal==0b110){strcpy(rmName, "[bp");}
		else if(rmBitsVal==0b111){strcpy(rmName, "[bx");}
		else{printf("ERROR DISASSEMBLING R/M");}
		
		//But we need to CONCATENATE a little more!
		//Just the disp for MOD=01 and MOD=10, then final bracket
		if(modBitsVal==0b01 || modBitsVal==0b10)
		{
			if(dispDirectAccess >= 0)
			{
				strcat(rmName, " + ");
			}
			else
			{
				dispDirectAccess = dispDirectAccess * (-1);
				strcat(rmName, " - ");
			}
			//conc displacement as string
			char dispString[8];
			sprintf(dispString, "%i", dispDirectAccess);
			strncat(rmName, dispString, 8);
		}
		//Everyone here gets a bracket!
		strncat(rmName, "]", 2);
	}
	if(modBitsVal != 0b11)
	{
		//fillRegName will have already printed the value.
		DEBUG_PRINT("  RM name: %s\n", rmName);
	}
}
short valFromUCharP(unsigned char *charP, bool word)
{
	//Always returns signed value.
	if(word)
	{
		//Also use charP+1 to make a short
		return * ((short*)charP);
	}
	else
	{
		//Just use charP
		return (char)*charP;
	}
}
void DEBUG_printBytesIn01s(unsigned char *startP, int size, int columns)
{
	for(int i=0 ; i<size ; i++)
	{
		///Each iteration prints 1 byte and a space
		for(int j=7 ; j>=0 ; j--)
		{
			///Each iteration prints 1 bit
			printf("%i", (startP[i] >> j) & 1);
		}
		
		printf(" ");
		if( columns!=0 && i!=0 && !((i+1)%columns) )
		{
			printf("\n");
		}
	}
}
