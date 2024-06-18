/*	========================================================================
	(C) Copyright 2024 by structJim, All Rights Reserved.
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the author(s) be held liable for any
	damages arising from the use of this software.
	========================================================================
	8086 Decoding
	Provides an abstraction struct (InstructionFor8086) that describes a
	single 8086 instruction, and populates it for decoding binary streams.
	The populated struct can be used for disassembly, or for simulation.
	========================================================================
	Instructions:
	========================================================================*/
#ifndef hole_decode_8086
#define hole_decode_8086
#include<assert.h>
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<string.h>
#include"../hole/JimsTypedefs.HOLE.h"
enum
{
	MNEM_ADD, MNEM_CMP, MNEM_JNZ, MNEM_MOV, MNEM_SUB
};
enum
{
	OPERAND_TYPE_NONE=0, OPERAND_TYPE_IMMEDIATE, OPERAND_TYPE_REGISTER,
	OPERAND_TYPE_MEMORY
};
const u8 MASK_BIT_1 = 1<<7;
const u8 MASK_BIT_2 = 1<<6;
const u8 MASK_BIT_3 = 1<<5;
const u8 MASK_BIT_4 = 1<<4;
const u8 MASK_BIT_5 = 1<<3;
const u8 MASK_BIT_6 = 1<<2;
const u8 MASK_BIT_7 = 1<<1;
const u8 MASK_BIT_8 = 1;
const char *register_names[16]=
{//Use 4 bits (reg<<1)|W as the array index.
	"al", "ax",
	"cl", "cx",
	"dl", "dx",
	"bl", "bx",
	"ah", "sp",
	"ch", "bp",
	"dh", "si",
	"bh", "di"
};
const char *rm_memory_strings[8]=
{//See table on page 4-20.
	"(BX) + (SI)",
	"(BX) + (DI)",
	"(BP) + (SI)",
	"(BP) + (DI)",
	"(SI)",
	"(DI)",
	"(BP)",
	"(BX)"
};
typedef struct
{	//An intermediate state for an instruction, to be
	//used both for disassembly and for simulation.
	char string[32];
	u8 mnemonic_type, mod_value, size;
	u8 operand_types[2];
	s16 operand_values[2];//REG bits, R/M bits, Immediate values...
	s16 disp_value;
	bool W;
}Instr8086;
u8 SubByte_ic(u8 byteIN, u8 firstBitIndex, u8 bitCount)
{
	//"ic" because arguments 2,3 are index,count.
	//Example: Pass this (0b00111000, 2, 3) and you'll get 7.
	assert(firstBitIndex < 8);
	assert(bitCount > 0);
	assert(firstBitIndex+bitCount <= 8);
	
	u8 byteOUT = byteIN;
	u8 leftMask = 0b11111111 >> firstBitIndex;
	u8 rightShiftsNeeded = 8 - (firstBitIndex + bitCount);
	byteOUT = byteOUT & leftMask;
	return byteOUT >> rightShiftsNeeded;
}
void DecodeBinaryAndPopulateInstruction(Instr8086 *targetP, u8 *bytes)
{
	//u8 mod_value;
	u8 disp_size;
	//s16 disp_value;
	bool D; //D means reg is dest
	//Instructions appear here in same order as 8086 manual (page 4-20).
	if(bytes[0]>>2 ==  0b100010)
	{	//MOV R/M <> REG
		printf("Decoded MOV R/M <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		targetP->mod_value = bytes[1]>>6;
		disp_size =  targetP->mod_value % 3;
		targetP->mnemonic_type = MNEM_MOV;
		targetP->size = 2 + disp_size;
		targetP->operand_types[!D] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[D] = OPERAND_TYPE_REGISTER;
		targetP->operand_values[!D] = SubByte_ic(bytes[1], 2, 3);//REG
		targetP->operand_values[D] = SubByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0]>>2 == 0b1100011 &&! (bytes[1] & 0b00111000)>>3)
	{	//MOV R/M, IMM. HERE!!! HERE!!! HERE!!! HERE!!! HERE!!! HERE!!!
		printf("Decoded MOV R/M, IMM\n");
		targetP->mnemonic_type = MNEM_MOV;
		targetP->W = bytes[0] & MASK_BIT_8;
		targetP->mod_value = bytes[1]>>6;
		u8 rm_value = SubByte_ic(bytes[1], 5, 3);
		bool is_reg_not_mem = (targetP->mod_value == 0b11);
		bool is_direct_access = (targetP->mod_value==0b00 && rm_value==0b110);
		disp_size = is_direct_access? 2 : (targetP->mod_value % 3);
		targetP->size = 3 + disp_size + targetP->W;
		targetP->operand_types[0] = is_reg_not_mem ?
			OPERAND_TYPE_REGISTER
			:
			OPERAND_TYPE_MEMORY;
		targetP->operand_types[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operand_values[0] = is_reg_not_mem ?
			SubByte_ic(bytes[1], 5, 3)
			:
			targetP->W? *(s16*)(bytes+2) : *(s8*)(bytes+2);
		targetP->operand_values[1] = targetP->W?
			*((s16*)(bytes+2+disp_size))
			:
			*((s8*)(bytes+2+disp_size));
		if(disp_size)
			targetP->disp_value = (is_direct_access || targetP->mod_value==0b10)?
				*(s16*)(bytes+2)
				:
				*(s8*)(bytes+2);
	}
	else if(bytes[0]>>4 == 0b1011)
	{	//MOV REG, IMM
		printf("Decoded MOV REG, IMM\n");
		targetP->W = bytes[0] & MASK_BIT_5;
		targetP->mnemonic_type = MNEM_MOV;
		targetP->size = 2 + targetP->W;
		targetP->operand_types[0] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operand_values[0] = SubByte_ic(bytes[0], 5, 3);//RM
		targetP->operand_values[1] = *((s16*)(bytes+1));//IMM
	}
	else if(bytes[0]>>2 == 0b000000)
	{	//ADD R/M <> REG
		printf("Decoded ADD R/M <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		targetP->mod_value = bytes[1]>>6;
		disp_size =  targetP->mod_value % 3;
		targetP->mnemonic_type = MNEM_ADD;
		targetP->size = 2 + disp_size;
		targetP->operand_types[!D] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[D] = OPERAND_TYPE_REGISTER;
		targetP->operand_values[!D] = SubByte_ic(bytes[1], 2, 3);//REG
		targetP->operand_values[D] = SubByte_ic(bytes[1], 5, 3);//R/M
	}
	else if((bytes[0]>>2 == 0b100000) && ((bytes[1]&0b00111000) == 0b0))
	{	//ADD R/M, IMM
		printf("Decoded ADD R/M, IMM\n");
		targetP->W = (bytes[0] & MASK_BIT_8);
		targetP->mod_value = bytes[1]>>6;
		disp_size = targetP->mod_value % 3;
		targetP->mnemonic_type = MNEM_ADD;
		bool immediate_is_word = targetP->W && !(bytes[0] & MASK_BIT_7);
		targetP->size = 3 + immediate_is_word + disp_size;
		targetP->operand_types[0] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operand_values[0] = SubByte_ic(bytes[1], 5, 3);//RM
		if(immediate_is_word)
		{	//Immediate is only word if W && !S.
			targetP->operand_values[1] = *((s16*)(bytes+2+disp_size));//IMM
		}
		else
		{
			targetP->operand_values[1] = *((s8*)(bytes+2+disp_size));//IMM
		}
	}
	else if(bytes[0]>>2 == 0b001010)
	{	//SUB R/M <> REG
		printf("Decoded SUB R/M <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		targetP->mod_value = bytes[1]>>6;
		disp_size =  targetP->mod_value % 3;
		targetP->mnemonic_type = MNEM_SUB;
		targetP->size = 2 + disp_size;
		targetP->operand_types[!D] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[D] = OPERAND_TYPE_REGISTER;
		targetP->operand_values[!D] = SubByte_ic(bytes[1], 2, 3);//REG
		targetP->operand_values[D] = SubByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0]>>2 == 0b100000 && ((bytes[1]&0b00111000) == 0b00101000))
	{	//SUB R/M, IMM
		printf("Decoded SUB R/M, IMM\n");
		targetP->W = bytes[0] & MASK_BIT_8;
		//S???
		targetP->mod_value = bytes[1]>>6;
		disp_size =  targetP->mod_value % 3;
		targetP->mnemonic_type = MNEM_SUB;
		bool immediate_is_word = targetP->W && !(bytes[0] & MASK_BIT_7);
		targetP->size = 3 + immediate_is_word + disp_size;
		targetP->operand_types[0] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operand_values[0] = SubByte_ic(bytes[1], 5, 3);//REG
		if(immediate_is_word)
		{	//Immediate is only word if W && !S.
			targetP->operand_values[1] = *((s16*)(bytes+2+disp_size));//IMM
		}
		else
		{
			targetP->operand_values[1] = *((s8*)(bytes+2+disp_size));//IMM
		}
	}
	else if(bytes[0]>>2 == 0b001110)
	{	//CMP R/M <> REG
		printf("Decoded CMP R/M <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		targetP->mod_value = bytes[1]>>6;
		disp_size =  targetP->mod_value % 3;
		targetP->mnemonic_type = MNEM_CMP;
		targetP->size = 2 + disp_size;
		targetP->operand_types[!D] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[D] = OPERAND_TYPE_REGISTER;
		targetP->operand_values[!D] = SubByte_ic(bytes[1], 2, 3);//REG
		targetP->operand_values[D] = SubByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0] == 0b01110101)
	{	//JNZ
		printf("Decoded JNZ\n");
		targetP->mnemonic_type = MNEM_JNZ;
		targetP->size = 2;
		targetP->operand_types[0] = OPERAND_TYPE_IMMEDIATE;
		targetP->operand_types[0] = OPERAND_TYPE_NONE;
		targetP->operand_values[0] = *(s8*)(bytes+1);
	}
	else
	{
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n");
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n");
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n\n");
	}
}
#endif
