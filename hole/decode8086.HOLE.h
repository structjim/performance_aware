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
#include"../hole/jims_general_tools.HOLE.h"
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
const char *REG_NAMES[16]=
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
const char *MEM_STRINGS[8]=
{	//Selected by r/m bits.
	//See table on page 4-20.
	"bx + si",
	"bx + di",
	"bp + si",
	"bp + di",
	"si",
	"di",
	"bp",
	"bx"
};
typedef struct
{	//An intermediate state for an instruction, to be
	//used both for disassembly and for simulation.
	char string[64];
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
	u8 disp_size, rm_value;
	//s16 disp_value;
	bool S, D, is_reg_not_mem, is_direct_access; //D means reg is dest
	//Instructions appear here in same order as 8086 manual (page 4-20).
	if(bytes[0]>>2 == 0b100010)
	{	//MOV R/M <D> REG TODO:MEMORY!
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
	else if(bytes[0]>>2 == 0b110001 &&! (bytes[1] & 0b00111000))
	{	//MOV R/M, IMM
		targetP->mnemonic_type = MNEM_MOV;
		S = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		targetP->mod_value = bytes[1]>>6;
		rm_value = SubByte_ic(bytes[1], 5, 3);
		is_reg_not_mem = (targetP->mod_value == 0b11);
		is_direct_access = (targetP->mod_value==0b00 && rm_value==0b110);
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
			bytes[1] & 0b00000111;
		targetP->operand_values[1] = targetP->W?
			*((s16*)(bytes+2+disp_size))
			:
			*((s8*)(bytes+2+disp_size));
		if(disp_size)
			targetP->disp_value = (is_direct_access || targetP->mod_value==0b10)?
				*(s16*)(bytes+2)
				:
				*(s8*)(bytes+2);
		else targetP->disp_value = 0;
	}
	else if(bytes[0]>>4 == 0b1011)
	{	//MOV REG, IMM
		targetP->W = bytes[0] & MASK_BIT_5;
		targetP->mnemonic_type = MNEM_MOV;
		targetP->size = 2 + targetP->W;
		targetP->operand_types[0] = OPERAND_TYPE_REGISTER;
		targetP->operand_types[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operand_values[0] = SubByte_ic(bytes[0], 5, 3);//RM
		targetP->operand_values[1] = *((s16*)(bytes+1));//IMM
	}
	else if(bytes[0]>>2 == 0b000000)
	{	//ADD R/M <D> REG TODO:MEMORY!
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
	else if(bytes[0]>>2 == 0b100000)
	{	//(ADD/SUB/CMP) R/M, IMM
		//HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE
		//HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE
		//HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE
		//HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE
		//HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE
		//HERE HERE HERE HERE HERE HERE HERE HERE HERE HERE
		switch((bytes[1] & 0b00111000) >> 3)
		{
		case 0b000:
			targetP->mnemonic_type = MNEM_ADD;
			break;
		case 0b101:
			targetP->mnemonic_type = MNEM_SUB;
			break;
		case 0b111:
			targetP->mnemonic_type = MNEM_CMP;
			break;
		default:
		}
		targetP->W = (bytes[0] & MASK_BIT_8);
		targetP->mod_value = bytes[1]>>6;
		is_reg_not_mem = (targetP->mod_value == 0b11);
		disp_size = targetP->mod_value % 3;
		targetP->mnemonic_type = MNEM_ADD;
		S = (bytes[0] & MASK_BIT_7);
		bool immediate_is_word = targetP->W && !S;
		targetP->size = 3 + immediate_is_word + disp_size;
		targetP->operand_types[0] = is_reg_not_mem?
			OPERAND_TYPE_REGISTER
			:
			OPERAND_TYPE_MEMORY;
		targetP->operand_types[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operand_values[0] = SubByte_ic(bytes[1], 5, 3);//RM
		if(immediate_is_word)
		{
			targetP->operand_values[1] = *((s16*)(bytes+2+disp_size));//IMM
		}
		else
		{
			targetP->operand_values[1] = *((s8*)(bytes+2+disp_size));//IMM
		}
	}
	else if(bytes[0]>>2 == 0b001010)
	{	//SUB R/M <D> REG TODO:MEMORY!
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
	{	//SUB R/M, IMM TODO:MEMORY!
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
	{	//CMP R/M <D> REG TODO:MEMORY!
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
	//Populate string:
	char elements[2][32] = {"", ""};;
	char mnemonic_string[8];
	switch(targetP->mnemonic_type)
	{
	case MNEM_ADD:
		sprintf(mnemonic_string,  "add");
		break;
	case MNEM_CMP:
		sprintf(mnemonic_string,  "cmp");
		break;
	case MNEM_JNZ:
		sprintf(mnemonic_string,  "jnz");
		break;
	case MNEM_MOV:
		sprintf(mnemonic_string,  "mov");
		break;
	case MNEM_SUB:
		sprintf(mnemonic_string,  "sub");
		break;
	default:
		SPAM(69)("ERROR SELECTING MNEMONIC IN PopulateInstructionString!!!");
		break;
	}
	//TODO: Handle stuff like "word" and "byte"
	//TODO: Handle stuff like "word" and "byte"
	//TODO: Handle stuff like "word" and "byte"
	u8 operand_count =
		(targetP->operand_types[0] != OPERAND_TYPE_NONE)
		+
		(targetP->operand_types[1] != OPERAND_TYPE_NONE);
	for(int i=0 ; i<operand_count ; i++)
	{
		switch(targetP->operand_types[i])
		{
		case OPERAND_TYPE_IMMEDIATE:
			sprintf(elements[i], " %i", targetP->operand_values[i]);
			break;
		case OPERAND_TYPE_REGISTER:
			u8 reg_index = (targetP->operand_values[i]<<1) | targetP->W;
			sprintf(elements[i], " %s", REG_NAMES[reg_index]);
			break;
		case OPERAND_TYPE_MEMORY:
			bool is_direct_access = (!targetP->mod_value && targetP->operand_values[i]==0b110);
			if(is_direct_access)
				sprintf(elements[i], " [%i]",	targetP->disp_value);
			else
				sprintf(elements[i], " [%s + %i]", MEM_STRINGS[targetP->operand_values[i]],
						targetP->disp_value);
			break;\
		}
	}
	sprintf(targetP->string, operand_count==2 ? "%s%s,%s":"%s%s%s",
			mnemonic_string, elements[0], elements[1]);
}
#endif
