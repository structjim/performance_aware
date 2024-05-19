/*
	Disassembles 16 bit MOV operations from argument binary file and
	outputs assembly code to hw01_output.asm in the working directory
	Only works for MOV, and only when the MOD bits are 11.
*/

#include<stdio.h>
#include<stdbool.h>
bool DEBUG=1;

void fillRegChars(char regChars[2], char regBits, bool W);

int main(int argc, char **argv)
{
	//Get file(s)
	FILE *fInP = fopen(argv[1], "rb");
	FILE *fOutP = fopen("hw01_output.asm", "w");

	//Get file size
	fseek(fInP, 0L, SEEK_END); //Set file position to end
	int fInSz = ftell(fInP); //Store position (size)
	fseek(fInP, 0L, SEEK_SET);//Return to start of file
	if(DEBUG)printf("File IN size is %i.\n", fInSz);

	//Get file contents
	unsigned char inBytes[fInSz];
	for(int i=0; i<fInSz; i++)
	{
		inBytes[i] = fgetc(fInP);
	}

	if(DEBUG)printf("Bytes get: %s\n", inBytes);

	//Ensure even byte count
	if(fInSz %2)
	{
		printf("Uneven byte count. Terminating.");
		return 1;
	}

	//Write bit width directive to output file
	//(We just do this literally with no logic needed, right?)
	fprintf(fOutP, "%s", "bits 16\n");

	//Disassemble all instructions and write to file
	for(int i=0; i<fInSz; i+=2)
	{
		//Each iteration parses 2 bytes, aka 1 instruction
		/// Expected binary from listing 37: 10001001 11011001
		///                                  MnemonDW MdRegR/m
		///                                  mov cx, bx
		
		//WRITE newline and every instruction after the first
		if(i!=0)fprintf(fOutP, "%s", "\n");

		if(DEBUG)printf("Beginning work on mnemonic %i.\n", i/2); //Bytes processed/2 == mnemonic index
		//Mnemonic
		switch( inBytes[i] >> 2 )
		{
			case 0b100010: //MOV
			{
				if(DEBUG)printf("  Mnemonic was recognized as 100010 (mov)! [Writing...]\n");
				fprintf(fOutP, "%s", "mov "); //write mnemonic name to output file
				
				//Prep for operand parsing
				bool D = (inBytes[i] & 0b00000010);
				bool W = (inBytes[i] & 0b00000001);
				if(DEBUG)printf("  D is %s, W is %s.\n", D?"true":"false", W?"true":"false");
				char regChars[2]; //Human-readable REG operand chars
				char rmChars[2]; //Human-readable R/M operand chars
				
				//REG name generation
				fillRegChars(regChars, (inBytes[i+1]&0b00111000)>>3, W);
				if(DEBUG)printf("  REG is: %c%c\n", regChars[0], regChars[1]);
				
				//MOD name generation
				switch( inBytes[i+1] >> 6 )
				{
					case 0b11: //Register Mode
					{
						if(DEBUG)printf("  MOD was recognized as 11 (Register Mode).\n");
					}break;
					default:
					{
						printf("ERROR: Instruction %i unrecognized mod. Terminating.\n", i/2);
						return 1;
					}
				}
				
				//R/M
				//NOTE: We can only blindly reuse fillRegChars() while MOD is 11
				fillRegChars(rmChars, (inBytes[i+1]&0b00000111), W);
				if(DEBUG)printf("  R/M is: %c%c\n", rmChars[0], rmChars[1]);
				
				//WRITE OPERANDS
				if(DEBUG)printf("  REG and R/M disassembled successfully. [Writing...]\n");
				if(D)
				{
					fprintf(fOutP, "%c", regChars[0]);
					fprintf(fOutP, "%c", regChars[1]);
					fprintf(fOutP, "%s", ", ");
					fprintf(fOutP, "%c", rmChars[0]);
					fprintf(fOutP, "%c", rmChars[1]);
				}
				else
				{
					fprintf(fOutP, "%c", rmChars[0]);
					fprintf(fOutP, "%c", rmChars[1]);
					fprintf(fOutP, "%s", ", ");
					fprintf(fOutP, "%c", regChars[0]);
					fprintf(fOutP, "%c", regChars[1]);
				}
			}break;

			default:
			{
				printf("ERROR: Instruction unrecognized mnemonic. Terminating.\n");
				return 1;
			}
		}//end mnemonic
	}

	//PRINT FILE
	//Get output file size
	fseek(fOutP, 0L, SEEK_END); //Set file position to end
	int fOutSz = ftell(fOutP); //Store position (size)
	fseek(fOutP, 0L, SEEK_SET); //Return to start of file
	if(DEBUG)printf("File OUT size is %i.\n", fOutSz);

	//Print output file to stdout
	printf("::: CONTENTS OF OUTPUT FILE :::\n");
	printf ("%s", *fOutP);
	
	
	
	//Done.
	fclose(fInP);
	fclose(fOutP);
	//system("pause");
	return 0;
}

void fillRegChars(char regChars[2], char regBits, bool W)
{
	//Fills in chars for MOV operand name. Works for REG
	//Works for R/M as well, as long as MOD bits in the instruction are 11
	//Char values come from tables on page 4-20 of 8086 manual (page 162 of the pdf)
	switch(regBits)
	{
		case 0b000:
		{
			regChars[0] = 'a';			//first char of operand
			regChars[1] = !W?'l':'x';	//secnd char of operand
		}break;
		
		case 0b001:
		{
			regChars[0] = 'c';
			regChars[1] = !W?'l':'x';
		}break;
		
		case 0b010:
		{
			regChars[0] = 'd';
			regChars[1] = !W?'l':'x';
		}break;
		
		case 0b011:
		{
			regChars[0] = 'b';
			regChars[1] = !W?'l':'x';
		}break;
		
		case 0b100:
		{
			regChars[0] = !W?'a':'s';
			regChars[1] = !W?'h':'p';
		}break;
		
		case 0b101:
		{
			regChars[0] = !W?'c':'b';
			regChars[1] = !W?'h':'p';
		}break;
		
		case 0b110:
		{
			regChars[0] = !W?'d':'s';
			regChars[1] = !W?'h':'i';
		}break;
		
		case 0b111:
		{
			regChars[0] = !W?'b':'d';
			regChars[1] = !W?'h':'i';
		}break;
		
		default:
		{
			printf("~~ERROR parsing REG (OR R/M). Terminating.\n");
		}
	}
}