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

#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//#include<time.h>
#include"../hole/decode8086.HOLE.h"
#include"../hole/jims_general_tools.HOLE.h"

const u8 FLAG_MASK_ZERO = 1<<7;
const u8 FLAG_MASK_PARITY = 1<<6;
const u8 FLAG_MASK_SIGN = 1<<5;

void SetZeroFlag(s16 valueIN);
void SetSignFlag(bool sign_bit);
void PrintBytesIn01s(void *startP, int size, int column_count);
void PrintAllRegContents();
int LoadFileTo8086Memory(char *argPIN, u8 memoryIN[]);
u8 *GetOperandP(Instr8086 *instructionPIN, u8 operand_index);

u16 flags = 0;
u16 ip = 0; //Instruction pointer register
bool D;
u8 memory[1024 * 1024] = {0};
u8 reg_raw_bits[16] = {0};//Simulation registers
// 0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15
// AL AH BL BH CL CH DL DH SP SP BP BP SI SI DI DI

u8 *reg_pntrs[16] =
{	//Pointers to raw bits where the registers live.
	//Arranged to have indices like the table on p4-20.
	//Use 4 bits (reg<<1)|W as the array index.
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
{	//Arranged to have indices like the table on p4-20.
	REG_AL, REG_AX,
	REG_CL, REG_CX,
	REG_DL, REG_DX,
	REG_BL, REG_BX,
	REG_AH, REG_SP,
	REG_CH, REG_BP,
	REG_DH, REG_SI,
	REG_BH, REG_DI
};
int main(int argc, char *argv[])
{
	DEBUG=1;
	
	int file_in_size = LoadFileTo8086Memory(argv[1], memory);

	//Print all bits of bin, in 8 columns.
	DEBUG_PRINT("\n\"%s\" binary size is %i.\nRaw binary contents:\n\n", argv[1], file_in_size);
	if(DEBUG)PrintBytesIn01s(memory, file_in_size, 8);
	DEBUG_PRINT("\n\n");

	//Initial register print
	DEBUG_PRINT("Starting registers:\n\n");
	if(DEBUG)PrintAllRegContents();

	Instr8086 *instruction_dataP = (Instr8086*)malloc(sizeof(Instr8086));

	while(ip < file_in_size)
	{	// Each iteration decodes + sims one instruction.
		u8 *destP, *sourceP;
		s16 math_result;
		
		DecodeBinaryAndPopulateInstruction(instruction_dataP, memory+ip);
		ip += instruction_dataP->size;
		bool W = instruction_dataP->W;
		
		//Select and execute simulation behavior
		switch(instruction_dataP->mnemonic_type)
		{
		case MNEM_MOV:
			destP = GetOperandP(instruction_dataP, 0);
			sourceP = GetOperandP(instruction_dataP, 1);;
			if(W)
				*(s16*)destP = *(s16*)sourceP;
			else
				*(s8*)destP = *(s8*)sourceP;
			break;
		case MNEM_ADD:
			destP = GetOperandP(instruction_dataP, 0);
			sourceP = GetOperandP(instruction_dataP, 1);
			if(W) //NOTE: R = D += S
				math_result = *(s16*)destP += *(s16*)sourceP;
			else
				math_result = *(s8*)destP += *(s8*)sourceP;
			SetZeroFlag(math_result);
			SetSignFlag( (*(destP+W)) >> 7 );
			break;
		case MNEM_SUB:
			destP = GetOperandP(instruction_dataP, 0);
			sourceP = GetOperandP(instruction_dataP, 1);;
			if(W) //NOTE: R = D -= S
				math_result = *(s16*)destP -= *(s16*)sourceP;
			else
				math_result = *(s8*)destP -= *(s8*)sourceP;
			SetZeroFlag(math_result);
			SetSignFlag( (*(destP+W)) >> 7 );
			break;
		case MNEM_CMP:
			destP = GetOperandP(instruction_dataP, 0);
			sourceP = GetOperandP(instruction_dataP, 1);;
			if(W)
				math_result = *(s16*)destP - *(s16*)sourceP;
			else
				math_result = *(s8*)destP - *(s8*)sourceP;
			SetZeroFlag(math_result);
			SetSignFlag( (*(destP+W)) >> 7 );
			break;
		case MNEM_JNZ:
			if( !(flags>>7) )
			{
				ip += instruction_dataP->operand_values[0];
				printf("Jumped %i.\n\n", instruction_dataP->operand_values[0]);
			}
			else
			{
				printf("Didn't jump.\n\n");
			}
			break;
		default:
			printf("[[[SIMULATION ERROR! Next 6 bytes on instruction stream:]]]\n   ");
			PrintBytesIn01s(memory + ip - instruction_dataP->size, 6, 0);
			printf("\n\n");
		}
		//Print
		printf("%s (Size: %i)\n",instruction_dataP->string, instruction_dataP->size);
		PrintBytesIn01s(memory + ip - instruction_dataP->size, instruction_dataP->size, 8);
		printf("\n\n");
		PrintAllRegContents(reg_raw_bits);
		instruction_dataP->mnemonic_type = MNEM_NONE;
	}//End of decoding and simulation loop
	free(instruction_dataP);
	return 0;
}//END MAIN

int LoadFileTo8086Memory(char *argPIN, u8 memoryIN[])
{
	FILE *openP = fopen(argPIN, "rb");
	fseek(openP, 0L, SEEK_END); //Set file position to end
	int file_in_size = ftell(openP); //Store position (file size)
	fseek(openP, 0L, SEEK_SET);//Return to start of file
	for(int i=0 ; i<file_in_size ; i++)
	{	//Copy bin from file to memory
		memoryIN[i] = fgetc(openP);
	}
	fclose(openP);
	return file_in_size;
}
void SetZeroFlag(s16 valueIN)
{
	if(valueIN)
	{	//Not zero
		flags = flags & 0b01111111;
	}
	else
	{	//Zero
		flags = flags | 0b10000000;
	}
}
void SetSignFlag(bool sign_bit)
{
	if(sign_bit)
		flags = flags | FLAG_MASK_SIGN;//yes
	else
		flags = flags & (~FLAG_MASK_SIGN);//no
}

void PrintAllRegContents()
{
	printf("  ");
	
	//First, print 16-bit reg. (AX, BX, CX, DX)
	for(u8 i=0 ; i<4 ; i++)
	{	//Labels and hex
		printf("%cX         0x%04x ", 'A'+i, *(u16*)(reg_raw_bits+i+i));
	}
	printf("\n  ");	
	//Print 8-bit registers (AL, AH, BL, BH, CL, CH, DL, DH)
	for(u8 i=0 ; i<8 ; i++)
	{	//Labels and hex
		printf("%c%c  0x%02x ", 'A'+(i/2), i%2?'H':'L', *(u8*)(reg_raw_bits+i));
	}	
	printf("\n  ");
	PrintBytesIn01s(reg_raw_bits, 8, 8);
	printf("\n");

	printf("  ");
	
	//Print the other 16-bit reg. (SP, BP, SI, DI, FLAGS, IP)
	char *labels[4] = {"SP", "BP", "SI", "DI"};
	for(u8 i=0 ; i<4 ; i++)
	{	//Labels and hex
		printf("%s         0x%04x ", labels[i], *(u16*)(reg_raw_bits+8+i+i));
	}
	printf("\n  ");
	PrintBytesIn01s(reg_raw_bits+8, 8, 8);
	printf("\n");

	//The other two things (FLAGS and IP)
	printf("  FLAGS      0x%04x IP         0x%04x\n  ", flags, ip);
	PrintBytesIn01s(&flags, 2, 0);
	PrintBytesIn01s(&ip, 2, 0);
	printf("\n (zps     )\n\n");
}
void PrintBytesIn01s(void *startP, int size, int column_count)
{
	for(int i=0 ; i<size ; i++)
	{
		///Each iteration prints 1 byte and a space
		for(int j=7 ; j>=0 ; j--)
		{
			///Each iteration prints 1 bit
			printf("%i", ( ((u8*)startP)[i] >> j ) & 1);
		}
		
		printf(" ");
		if( column_count && i && !((i+1)%column_count) )
		{
			printf("\n");
		}
	}
}
u8 *GetOperandP(Instr8086 *instruction_dataPIN, u8 operand_index)
{
	//Note: Depends on global reg_pntrs array.
	bool W = instruction_dataPIN->W;
	u8 registerIndex;
	switch(instruction_dataPIN->operand_types[operand_index])
	{
	case OPERAND_TYPE_IMMEDIATE:
		return (u8*)&instruction_dataPIN->operand_values[operand_index];
	case OPERAND_TYPE_REGISTER:
		registerIndex = (instruction_dataPIN->operand_values[operand_index] << 1) | W;
		return reg_pntrs[registerIndex];
	case OPERAND_TYPE_MEMORY:
		s16 disp_value = instruction_dataPIN->disp_value;
		switch( instruction_dataPIN->operand_values[operand_index] )
		{
		case 0b000:
			return memory + *(s16*)reg_pntrs[REG_BX] + *(s16*)reg_pntrs[REG_SI] + disp_value;
		case 0b001:
			return memory + *(s16*)reg_pntrs[REG_BX] + *(s16*)reg_pntrs[REG_DI] + disp_value;
		case 0b010:
			return memory + *(s16*)reg_pntrs[REG_BP] + *(s16*)reg_pntrs[REG_SI] + disp_value;
		case 0b011:
			return memory + *(s16*)reg_pntrs[REG_BP] + *(s16*)reg_pntrs[REG_DI] + disp_value;
		case 0b100:
			return memory + *(s16*)reg_pntrs[REG_SI] + disp_value;
		case 0b101:
			return memory + *(s16*)reg_pntrs[REG_DX] + disp_value;
		case 0b110:
			if(instruction_dataPIN->mod_value==0b00)
				return memory + disp_value;//Direct access!
			else
				return memory + *(s16*)reg_pntrs[REG_BP] + disp_value;
		case 0b111:
			return memory + *(s16*)reg_pntrs[REG_BX] + disp_value;
		default:
			SPAM("[[[MEM DISP ERROR IN GetOperandP! DISP:%i RM:%i]]]\n", disp_value, instruction_dataPIN->operand_values[operand_index]);
			break;
		}
	default:
		SPAM("[[[ERROR SELECTING OPERAND TYPE IN GetOperandP function!]]]\n");
		return NULL;
	}
}
