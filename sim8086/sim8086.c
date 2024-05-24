/*	========================================================================
	(C) Copyright 2024 by structJim, All Rights Reserved.
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the author(s) be held liable for any
	damages arising from the use of this software.
	========================================================================
	Disasm 8086
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
#include"../HOCM/LinkedList_String32.HOCM.h"
bool DEBUG=1;

/// opForm: This large enum is for the opForms (operation formats), which we'll
/// use to denote the max size of an instruction, whether it has certain fields
/// (like D, W, S, V, reg) and how those fields are arranged in the bits. One
/// opForm can apear in many mnemonics (EG: mov), and one mnemonic can support
/// many opForms.

void fillRegName(char regName[], char regBitsVal, bool W);
void fillRmName(char rmName[], char rmBitsVal, char modBitsVal, bool W, short disp);
void DEBUG_printBytesIn01s(unsigned char *startP, int size, int columns);
void ERROR_TERMINATE(unsigned char *inBytesP,int *instrSizes,unsigned int instrsDone,FILE*fOutP,char msg[]);
void printAllRegContents(unsigned char *reg_raw_bitsIN);
	
//Simulation registers
unsigned char reg_raw_bits[16] = {0};
//0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
//AL AH BL BH CL CH DL DH SP SP BP BP SI SI DI DI

unsigned char *regPs[16] =
{//Based on REG table on page 4-20.
	/*AL*/reg_raw_bits,      /*AX*/reg_raw_bits,
	/*CL*/reg_raw_bits+4,    /*CX*/reg_raw_bits+4,
	/*DL*/reg_raw_bits+6,    /*DX*/reg_raw_bits+6,
	/*BL*/reg_raw_bits+2,    /*BX*/reg_raw_bits+2,
	/*AH*/reg_raw_bits+1,    /*SP*/reg_raw_bits+8,
	/*CH*/reg_raw_bits+5,    /*BP*/reg_raw_bits+10,
	/*DH*/reg_raw_bits+6,    /*SI*/reg_raw_bits+12,
	/*BH*/reg_raw_bits+3,    /*DI*/reg_raw_bits+14
};
enum
{
	REGP_INDEX_AL, REGP_INDEX_AX,
	REGP_INDEX_CL, REGP_INDEX_CX,
	REGP_INDEX_DL, REGP_INDEX_DX,
	REGP_INDEX_BL, REGP_INDEX_BX,
	REGP_INDEX_AH, REGP_INDEX_SP,
	REGP_INDEX_CH, REGP_INDEX_BP,
	REGP_INDEX_DH, REGP_INDEX_SI,
	REGP_INDEX_BH, REGP_INDEX_DI
};
char regNames[16][3]=
{
	"al", "ax",
	"cl", "cx",
	"dl", "dx",
	"bl", "bx",
	"ah", "sp",
	"ch", "bp",
	"dh", "si",
	"bh", "di"
};
bool W, D;
unsigned short immediateValue;
unsigned char regCode4a; //Appends W. See table on p. 4-20
unsigned char regCode4b; //Appends W. See table on p. 4-20
unsigned char const MASK_BIT_5 = 0b00001000;
unsigned char const MASK_BIT_7 = 0b00000010;
unsigned char const MASK_BIT_8 = 0b00000001;

unsigned char instrSz;
unsigned int instrsDone=0;
unsigned int bytesDone=0;
clock_t starttime;

int main(int argc, char *argv[])
{	
	starttime=clock();
	//Get file(s)
	FILE *fInP = fopen(argv[1], "rb");
	//FILE *fOutP = fopen(argv[2], "w");

	//Get file in size
	fseek(fInP, 0L, SEEK_END); //Set file position to end
	int fInSz = ftell(fInP); //Store position (file size)
	fseek(fInP, 0L, SEEK_SET);//Return to start of file
	
	int instrSizes[fInSz];
	
	//These are the bytes after which we will write labels in the final asm.
	unsigned int labelCount=0;
	unsigned int labelIndex;
	bool labelExists;
	unsigned int labelIndices[fInSz];
	
	//Get file contents
	unsigned char inBytes[fInSz];
	for(int i=0 ; i<fInSz ; i++)
	{
		inBytes[i] = fgetc(fInP);
	}
	//For iterating through input
	unsigned char *inputP = inBytes;

	//In debug mode, print all bits of bin, in 8 columns.
	DEBUG_PRINT("\n\"%s\" bin size is %i. Contents:\n\n", argv[1], fInSz);
	DEBUG_printBytesIn01s(inBytes, fInSz, 8);
	DEBUG_PRINT("\n");

	//Initial register print
	printf("------------------------------------------------------------------------------\n");
	printf("\nStarting memory:\n\n");
	printAllRegContents(reg_raw_bits);
		
	//MASTER LOOP. Each iteration disassembles one instruction.
	while(bytesDone < fInSz)
	{
		bool opIdentified = false;
		bool DEBUG_useImmediate;
		//4 leading opcode bits
		switch(inputP[0]>>4)
		{
		case 0b1011:
			W = inputP[0] & MASK_BIT_5;
			instrSz = 2+W;
			immediateValue = (unsigned short)inputP[1];
			DEBUG_useImmediate = true;
			break;
		}
		//6 leading opcode bits
		if(!opIdentified)switch(inputP[0]>>2)
		{
			case 0b100010:
			W = inputP[0] & MASK_BIT_8;
			D = inputP[0] & MASK_BIT_7;
			instrSz = 2 + ((inputP[1]&0b11000000)>>6) % 3; //2 + modbits%3
			DEBUG_useImmediate = false;
			break;
		}

		//Decoded. Now, execute!
		if(DEBUG_useImmediate)
		{//MOV imm to reg
			regCode4a = (inputP[0] & 0b00000111)<<1 | W;
			DEBUG_PRINT("------------------------------------------------------------------------------\n");
			printf("\nmov %s, %i  |  ", regNames[regCode4a], immediateValue);
			if(W)
			{
				printf("%i->", *(unsigned short*)regPs[regCode4a]);
				*(unsigned short*)regPs[regCode4a] = immediateValue;
				printf("%i (instruction size: %i)\n\n", *(unsigned short*)regPs[regCode4a], instrSz);
			}
			else
			{
				printf("%i->%i\n", 420, 69);
			}
			printAllRegContents(reg_raw_bits);
		}
		else//REG RM
		{
			regCode4a = (inputP[1] & 0b00111000)>>2 | W;
			regCode4b = (inputP[1] & 0b00000111)<<1 | W;
			if(!D)
			{
				//D means reg is dest (first operand).
				//If not, R/M is dest, so swap.
				unsigned char aux = regCode4a;
				regCode4a = regCode4b;
				regCode4b = aux;
			}
			DEBUG_PRINT("------------------------------------------------------------------------------\n");
			printf("\nmov %s, %s  |  ", regNames[regCode4a], regNames[regCode4b]);
			if(W)
			{
				printf("%i->", *(unsigned short*)regPs[regCode4a]);
				*(unsigned short*)regPs[regCode4a] = *(unsigned short*)regPs[regCode4b];
				printf("%i (instruction size: %i)\n\n", *(unsigned short*)regPs[regCode4a], instrSz);
			}
			else
			{
				printf("%i->%i\n", 420, 69);
			}
			printAllRegContents(reg_raw_bits);
		}
		bytesDone+=instrSz;
		inputP+=instrSz;
	}//End of disassembling
	printf("\n");
/*	
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
	//fclose(fOutP);
	return 0;
}
//END MAIN











void printAllRegContents(unsigned char *reg_raw_bitsIN)
{
	DEBUG_PRINT("AL       AH       BL       BH       CL       CH       DL       DH\n");
	DEBUG_printBytesIn01s(reg_raw_bitsIN, 8, 8);
	DEBUG_PRINT("SP                BP                SI                DI\n");
	DEBUG_printBytesIn01s(reg_raw_bitsIN+8, 8, 8);
	DEBUG_PRINT("\n");
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
void fillRmName(char rmName[], char rmBitsVal, char modBitsVal, bool W, short dispDA)
{
	if(modBitsVal == 0b11)
	{
		fillRegName(rmName, rmBitsVal, W);
		DEBUG_PRINT("  ^Actually an R/M; it's merely REG-flavored!\n");
	}
	else if(modBitsVal==0b00 && rmBitsVal==0b110)
	{
		//The special case
		sprintf(rmName, "[%i]", dispDA);
	}
	else
	{
		//We know we aren't MOD=11, and we aren't the special DIRECT ADDRESS case.
		//Besides that, this block is MOD agnostic.
		//See table 4.10 on page 4.20.
		if     (rmBitsVal==0b000){strcpy(rmName, "[bx + si");}
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
			if(dispDA >= 0)
			{
				strcat(rmName, " + ");
			}
			else
			{
				dispDA = dispDA * (-1);
				strcat(rmName, " - ");
			}
			//conc displacement as string
			char dispString[8];
			sprintf(dispString, "%i", dispDA);
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
void DEBUG_printBytesIn01s(unsigned char *startP, int size, int columns)
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
