#include<stdio.h>
#include<stdbool.h>
#include<string.h>
#define DEBUG_PRINT if(DEBUG)printf
bool DEBUG=1;

enum //Instruction types, in the order they appear in the 8086 manual
{
	MOV_RM_TOFROM_REG,
	MOV_IMMED_TO_RM,
	MOV_IMMED_TO_REG,
	MOV_MEM_TO_ACC,
	MOV_ACC_TO_MEM,
	MOV_RM_TO_SEGREG,
	MOV_SEGREG_TO_RM
};

char modMsgs[4][64]=
{
	"  MOD is: 00 (Mem Mode, no disp)\n",
	"  MOD is: 01 (Mem Mode, 8-bit disp)\n",
	"  MOD is: 10 (Mem Mode, 16-bit disp)\n",
	"  MOD is: 11 (Reg Mode, no disp)\n"
};

void printBits(unsigned char *startP, int size, int columns);
void fillRegName(char regName[], char regBits, bool W);
void fillRmName(char rmName[], char rmBits, char modBits, bool W, short disp);
short shortValFromUCharP(unsigned char *charP);

int main(int argc, char *argv[])
{
	//Get file(s)
	FILE *fInP = fopen(argv[1], "rb");
	FILE *fOutP = fopen(argv[2], "w");

	//Get file in size
	fseek(fInP, 0L, SEEK_END); //Set file position to end
	int fInSz = ftell(fInP); //Store position (size)
	fseek(fInP, 0L, SEEK_SET);//Return to start of file

	//Get file contents
	unsigned char inBytes[fInSz];
	for(int i=0 ; i<fInSz ; i++)
	{
		inBytes[i] = fgetc(fInP);
	}
	
	//instrP points to the first byte of the instruction being disassembled
	unsigned char *instrP = inBytes;

	//In debug mode, print entire bin contents as 0s and 1s, in 4 columns.
	DEBUG_PRINT("\n\"%s\" bin size is %i. Contents:\n\n", argv[1], fInSz);
	if(DEBUG){printBits(instrP, fInSz, 4);}
	DEBUG_PRINT("\n\n");
	
	//Prep vars
	int instrsProcessed=0;
	int bytesProcessed=0;
	short instrSz;
	short instrType;
	bool D;
	bool W;
	short dispDirAVal; //For disp and DIRECT ACCESS
	short dispSz;
	short dataVal;
	//short dataSz;
	unsigned char modBits;
	unsigned char regBits;
	unsigned char rmBits;
	char regName[3]; //Human-readable REG operand
	char rmName[32]; //Human-readable R/M operand
	char instrString[32];

	//WRITE bit width directive to output file
	fprintf(fOutP, "%s", "bits 16\n");
	
	while(bytesProcessed < fInSz)
	{///Each iteration disassembles one instruction
		
		//WRITE newline to file for every instruction after the first
		if(bytesProcessed)fprintf(fOutP, "%c", '\n');

		DEBUG_PRINT("Beginning work on instruction %i. ", instrsProcessed+1);
		DEBUG_PRINT("First byte: ");
		if(DEBUG){printBits(instrP, 1, 0);}
		DEBUG_PRINT("\n");
		
		//Determine instruction type (See table 4.12 on page 4.22 in the 8086 manual.)
		if(      (instrP[0]&0b11111100) == 0b10001000){instrType = MOV_RM_TOFROM_REG;} ///100010DW
		else if( (instrP[0]&0b11111110) == 0b11000110){instrType = MOV_IMMED_TO_RM;} ///1100011W
		else if( (instrP[0]&0b11110000) == 0b10110000){instrType = MOV_IMMED_TO_REG;} ///1011WReg
		else if( (instrP[0]&0b11111110) == 0b10100000){instrType = MOV_MEM_TO_ACC;} ///1010000w
		else if( (instrP[0]&0b11111110) == 0b10100010){instrType = MOV_ACC_TO_MEM;} ///1010001w
		//else if( (instrP[0]&0b11111111) == 0b10001110){instrType = MOV_RM_TO_SEGREG;}
		//else if( (instrP[0]&0b11111111) == 0b10001100){instrType = MOV_SEGREG_TO_RM;}
		else
		{
			printf("ERROR: Unrecognized instruction byte: ");
			printBits(instrP, 1, 0);
			printf("\nBytes processed: %i\n[Terminating...]\n", bytesProcessed); 
			return 1;
		}

		//Process this instruction (Implementations are in same order as 8086 manual from p4.22)
		if(instrType == MOV_RM_TOFROM_REG)
		{///MOV_RM_TOFROM_REG
			DEBUG_PRINT("  Instruction identified: (MOV_RM_TOFROM_REG)\n");
			DEBUG_PRINT("  Instruction format: 100010DW MdRegR/m (DISP-LO) (DISP-HI)\n");
			
			//Prep vars for operand parsing
			D = (instrP[0] & 0b00000010);
			W = (instrP[0] & 0b00000001);
			modBits = instrP[1]>>6;
			dispSz = modBits%3;
			instrSz = 2 + dispSz;
			regBits = (instrP[1]&0b00111000)>>3;
			rmBits = (instrP[1]&0b00000111);
			dispDirAVal = 0;
			DEBUG_PRINT(modMsgs[modBits]);
			DEBUG_PRINT("  %s\n", D?"(D) MOV REG, R/M":"(!D) MOV R/M, REG");
			DEBUG_PRINT("  %s\n", W?"(W) MOV 2 bytes":"(!W) MOV 1 byte");
			
			//DISP
			if(modBits == 0b01)
			{
				dispDirAVal = (char)instrP[2];
			}
			else if(modBits == 0b10)
			{
				dispDirAVal = shortValFromUCharP(instrP+2);
			}
			else if(modBits==0b00 && rmBits==0b110)
			{
				//Special DIRECT ACCESS case
				instrSz+=2;
				dispDirAVal = shortValFromUCharP(instrP+2);
			}
			
			//REG name generation
			fillRegName(regName, regBits, W);
			
			//RM name generation
			fillRmName(rmName, rmBits, modBits, W, dispDirAVal);
			
			//Build string
			if(D)
			{	//MOV REG, R/M
				sprintf(instrString, "mov %s, %s", regName, rmName);
			}
			else
			{	//MOV R/M, REG
				sprintf(instrString, "mov %s, %s", rmName, regName);
			}
		}
		else if(instrType == MOV_IMMED_TO_RM)
		{///MOV_IMMED_TO_RM
			DEBUG_PRINT("  Instruction identified: (MOV_IMMED_TO_RM)\n");
			DEBUG_PRINT("  Instruction format: 1100011W Md000R/m (DISP-LO) (DISP-HI) Datadata Dataifw1\n");
			
			//Prep variables
			W = (instrP[0] & 0b00000001);
			modBits = instrP[1]>>6;
			dispSz = modBits%3;
			instrSz = 2 + dispSz + (1+(short)W);
			rmBits = (instrP[1]&0b00000111);
			DEBUG_PRINT(modMsgs[modBits]);
			DEBUG_PRINT("  %s\n", W?"(W) MOV 2 bytes":"(!W) MOV 1 byte");
			
			//DISP
			if(modBits == 0b01)
			{
				dispDirAVal = (char)instrP[2];
			}
			else if(modBits == 0b10)
			{
				dispDirAVal = shortValFromUCharP(instrP+2);
			}
			
			//RM name generation
			fillRmName(rmName, rmBits, modBits, W, dispDirAVal);
			
			//DATA / build string
			if(!W)
			{
				dataVal = (char)instrP[2+dispSz];
				sprintf(instrString, "mov %s, byte %i", rmName, dataVal);
			}
			else
			{
				dataVal = shortValFromUCharP(instrP+2+dispSz);
				sprintf(instrString, "mov %s, word %i", rmName, dataVal);
			}
		}
		else if(instrType == MOV_IMMED_TO_REG)
		{///MOV_IMMED_TO_REG
			DEBUG_PRINT("  Instruction identified: (MOV_IMMED_TO_REG)\n");
			DEBUG_PRINT("  Instruction format: 1011WReg Datadata Dataifw1\n");
			
			//Prep vars
			regBits = (instrP[0]&0b00000111);
			W = (instrP[0]&0b00001000);
			instrSz = 2 + ((short)W);
			DEBUG_PRINT("  %s\n", W?"(W) MOV 2 bytes":"(!W) MOV 1 byte");
			
			//REG
			fillRegName(regName, regBits, W);
			
			//DATA
			if(!W)
			{
				dataVal = (char)instrP[1];
			}
			else
			{
				dataVal = shortValFromUCharP(instrP+1);
			}
			
			//Build string
			sprintf(instrString, "mov %s, %i", regName, dataVal);
		}
		else if(instrType == MOV_MEM_TO_ACC)
		{///MOV_MEM_TO_ACC
			DEBUG_PRINT("  Instruction identified: (MOV_MEM_TO_ACC)\n");
			DEBUG_PRINT("  Instruction format: 1010000W addr--lo addr--hi\n");
			W = (instrP[0] & 0b00000001);
			DEBUG_PRINT("  %s\n", W?"(W) MOV 2 bytes":"(!W) MOV 1 byte");
			
			//addr (stored in dispDirAVal)
			if(W)
			{
				dispDirAVal = shortValFromUCharP(instrP+1);
				instrSz = 3;
			}
			else
			{
				dispDirAVal = (char)instrP[1];
				instrSz = 2;
			}
			
			//Build string
			sprintf(instrString, "mov ax, [%i]", dispDirAVal);
		}
		else if(instrType == MOV_ACC_TO_MEM)
		{///MOV_ACC_TO_MEM
			DEBUG_PRINT("  Instruction identified: (MOV_ACC_TO_MEM)\n");
			DEBUG_PRINT("  Instruction format: 1010001W addr--lo addr--hi\n");
			W = (instrP[0] & 0b00000001);
			DEBUG_PRINT("  %s\n", W?"(W) MOV 2 bytes":"(!W) MOV 1 byte");
			
			//addr (stored in dispDirAVal)
			if(W)
			{
				dispDirAVal = shortValFromUCharP(instrP+1);
				instrSz = 3;
			}
			else
			{
				dispDirAVal = (char)instrP[1];
				instrSz = 2;
			}
			
			//Build string
			sprintf(instrString, "mov [%i], ax", dispDirAVal);
		}
		else ///ERROR
		{
			printf("ERROR: Unrecognized mnemonic. Terminating.\n");
			return 1;
		}
		
		//WRITE instruction to output file
		fprintf(fOutP, instrString);
		DEBUG_PRINT("  Successfully disassembled -> \"%s\" [Writing...]\n", instrString);
		
		//Loop upkeep
		DEBUG_PRINT("  Instruction size: %i\n  ", instrSz);
		if(DEBUG){printBits(instrP, instrSz, 0);}
		instrP += instrSz;
		instrsProcessed++;
		bytesProcessed = instrP-inBytes;
		DEBUG_PRINT("\n  Bytes processed: %i/%i\n", instrP-inBytes, fInSz);
	}
	//End of disassembling

	//Get output file size
	fseek(fOutP, 0L, SEEK_END); //Set file position to end
	int fOutSz = ftell(fOutP); //Store position (size)
	fseek(fOutP, 0L, SEEK_SET); //Return to start of file
	
	DEBUG_PRINT("Disassembly DONE.\n");
	DEBUG_PRINT("Total instructions: %i\n", instrsProcessed);
	DEBUG_PRINT("File IN size: %i\n", fInSz);
	DEBUG_PRINT("File OUT size: %i\n", fOutSz);

	//Print output file contents to stdout
	printf("\n\n::: CONTENTS OF OUTPUT FILE :::\n");
	printf("%s", *fOutP);
	
	///TODO: Compare bins
	//Done.
	fclose(fInP);
	fclose(fOutP);
	//system("pause");
	return 0;
}
//END MAIN

void fillRegName(char regName[], char regBits, bool W)
{
	//                W? 01 R/M
	char regNameMatrix[2][2][8]=
	{// 000  001  010  011  100  101  110  111   <- R/M bit vals
		'a', 'c', 'd', 'b', 'a', 'c', 'd', 'b',  // !w
		'l', 'l', 'l', 'l', 'h', 'h', 'h', 'h',
		
		'a', 'c', 'd', 'b', 's', 'b', 's', 'd',   // w
		'x', 'x', 'x', 'x', 'p', 'p', 'i', 'i'
	};
	
	regName[0] = regNameMatrix[(char)W][0][regBits];
	regName[1] = regNameMatrix[(char)W][1][regBits];
	regName[2] = '\0';
	
	DEBUG_PRINT("  REG is: %s\n", regName);
}
void fillRmName(char rmName[], char rmBits, char modBits, bool W, short dispDA)
{
	if(modBits == 0b11)
	{
		DEBUG_PRINT("  This R/M tastes a lot like a REG.\n");
		fillRegName(rmName, rmBits, W);
	}
	else if(modBits==0b00 && rmBits==0b110)
	{
		//The special case
		sprintf(rmName, "[%i]", dispDA);
	}
	else
	{
		//We know we aren't MOD=11, and we aren't the special DIRECT ADDRESS case.
		//Besides that, this block is MOD agnostic.
		//See table 4.10 on page 4.20.
		if     (rmBits==0b000){strcpy(rmName, "[bx + si");}
		else if(rmBits==0b001){strcpy(rmName, "[bx + di");}
		else if(rmBits==0b010){strcpy(rmName, "[bp + si");}
		else if(rmBits==0b011){strcpy(rmName, "[bp + di");}
		else if(rmBits==0b100){strcpy(rmName, "[si");}
		else if(rmBits==0b101){strcpy(rmName, "[di");}
		else if(rmBits==0b110){strcpy(rmName, "[bp");}
		else if(rmBits==0b111){strcpy(rmName, "[bx");}
		else{printf("ERROR DISASSEMBLING R/M");}
		
		//But we need to CONCATENATE a little more!
		//Just the disp for MOD=01 and MOD=10, then final bracket
		if(modBits==0b01 || modBits==0b10)
		{
			if(dispDA >= 0)
			{
				strncat(rmName, " + ", 3);
			}
			else
			{
				dispDA = dispDA * (-1);
				strncat(rmName, " - ", 3);
			}
			//conc displacement as string
			char dispString[8];
			sprintf(dispString, "%i", dispDA);
			strncat(rmName, dispString, 8);
		}
		//Everyone here gets a bracket!
		strncat(rmName, "]", 2);
	}
	DEBUG_PRINT("  R/M is: %s\n", rmName);
}
short shortValFromUCharP(unsigned char *charP)
{
	return (short)charP[1]<<8 | (short)charP[0];
}
void printBits(unsigned char *startP, int size, int columns)
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