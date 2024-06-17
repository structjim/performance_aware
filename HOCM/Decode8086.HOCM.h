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
#ifndef hocm_decode_8086
#define hocm_decode_8086
#include<assert.h>
#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<string.h>
#include"../HOCM/JimsTypedefs.HOCM.h"
const u8 MASK_BIT_1 = 1<<7;
const u8 MASK_BIT_2 = 1<<6;
const u8 MASK_BIT_3 = 1<<5;
const u8 MASK_BIT_4 = 1<<4;
const u8 MASK_BIT_5 = 1<<3;
const u8 MASK_BIT_6 = 1<<2;
const u8 MASK_BIT_7 = 1<<1;
const u8 MASK_BIT_8 = 1;
enum
{
	MNEM_ADD, MNEM_CMP, MNEM_JNZ, MNEM_MOV, MNEM_SUB
};
enum
{
	OPERAND_TYPE_NONE=0, OPERAND_TYPE_IMMEDIATE, OPERAND_TYPE_REG, OPERAND_TYPE_RM
};

typedef struct
{	//An intermediate state for an instruction, to be used
	//both for disassembly and for simulation.
	u8 mnemonicType, modValue, dispValue, size;
	u8 operandTypes[2];
	s16 operandValues[2];//REG bits, R/M bits, Immediate values...
	bool W;
}Instr8086;

void populateInstr_nmdzttvvw(Instr8086 *instrIN, u8 mnemonicTypeIN,
							 u8 modValueIN, u8 dispValueIN, u8 sizeIN,
							 u8 operandType0IN, u8 operandType1IN,
							 s16 operandValue0IN, s16 operandValue1IN,
							 bool WIN)
{
	instrIN->mnemonicType = mnemonicTypeIN; //n
	instrIN->modValue = modValueIN; //m
	instrIN->dispValue = dispValueIN; //s
	instrIN->size = sizeIN; //z
	instrIN->operandTypes[0] = operandType0IN; //t
	instrIN->operandTypes[1] = operandType1IN; //t
	instrIN->operandValues[0] = operandValue0IN; //v
	instrIN->operandValues[1] = operandValue1IN; //v
	instrIN->W = WIN; //w
}

u8 subByte_ic(u8 byteIN, u8 firstBitIndex, u8 bitCount)
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
void decodeBinaryAndPopulateInstruction(Instr8086 *targetP, u8 *bytes)
{
	//u8 instructionSizeOUT;
	u8 dispSize;
	u8 modValue;
	bool D; //D means reg is dest
	//Instructions appear here in same order as 8086 manual (page 4-20).
	if(bytes[0]>>2 ==  0b100010)
	{	//MOV R/M to/from register
		printf("Decoded MOV RM <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		modValue = bytes[1]>>6;
		dispSize =  modValue % 3;
		targetP->mnemonicType = MNEM_MOV;
		targetP->size = 2 + dispSize;
		targetP->operandTypes[!D] = OPERAND_TYPE_REG;
		targetP->operandTypes[D] = OPERAND_TYPE_RM;
		targetP->operandValues[!D] = subByte_ic(bytes[1], 2, 3);//REG
		targetP->operandValues[D] = subByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0]>>2 == 0b1100011 &&
			( (bytes[1] & 0b00111000)>>3 == 0 ))
	{	//MOV immediate to R/M
		printf("Decoded MOV IMM > R/M\n");
		targetP->W = bytes[0] & MASK_BIT_8;
		targetP->mnemonicType = MNEM_MOV;
		targetP->size = 3 + targetP->W;
		targetP->operandTypes[0] = OPERAND_TYPE_RM;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operandValues[0] = subByte_ic(bytes[1], 5, 3);//REG
		targetP->operandValues[1] = *((s16*)(bytes+2));//IMM
	}
	else if(bytes[0]>>4 == 0b1011)
	{	//MOV immediate to register
		printf("Decoded MOV IMM > REG. Yup.\n");
		targetP->W = bytes[0] & MASK_BIT_5;
		targetP->mnemonicType = MNEM_MOV;
		targetP->size = 2 + targetP->W;
		targetP->operandTypes[0] = OPERAND_TYPE_REG;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operandValues[0] = subByte_ic(bytes[0], 5, 3);//RM
		targetP->operandValues[1] = *((s16*)(bytes+1));//IMM
	}
	else if(bytes[0]>>2 == 0b000000)
	{	//ADD R/M to/from register
		printf("Decoded ADD RM <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		modValue = bytes[1]>>6;
		dispSize =  modValue % 3;
		targetP->mnemonicType = MNEM_ADD;
		targetP->size = 2 + dispSize;
		targetP->operandTypes[!D] = OPERAND_TYPE_REG;
		targetP->operandTypes[D] = OPERAND_TYPE_RM;
		targetP->operandValues[!D] = subByte_ic(bytes[1], 2, 3);//REG
		targetP->operandValues[D] = subByte_ic(bytes[1], 5, 3);//R/M
	}
	else if((bytes[0]>>2 == 0b100000) && ((bytes[1]&0b00111000) == 0b0))
	{	//ADD immediate to R/M
		printf("Decoded ADD IMM > RM\n");
		targetP->W = (bytes[0] & MASK_BIT_8);
		modValue = bytes[1]>>6;
		dispSize = modValue % 3;
		targetP->mnemonicType = MNEM_ADD;
		bool immediateIsWord = targetP->W && !(bytes[0] & MASK_BIT_7);
		targetP->size = 3 + immediateIsWord + dispSize;
		targetP->operandTypes[0] = OPERAND_TYPE_RM;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operandValues[0] = subByte_ic(bytes[1], 5, 3);//RM
		if(immediateIsWord)
		{	//Immediate is only word if W && !S.
			targetP->operandValues[1] = *((s16*)(bytes+2+dispSize));//IMM
		}
		else
		{
			targetP->operandValues[1] = *((s8*)(bytes+2+dispSize));//IMM
		}
	}
	else if(bytes[0]>>2 == 0b001010)
	{	//SUB R/M to/from register
		printf("Decoded SUB RM <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		modValue = bytes[1]>>6;
		dispSize =  modValue % 3;
		targetP->mnemonicType = MNEM_SUB;
		targetP->size = 2 + dispSize;
		targetP->operandTypes[!D] = OPERAND_TYPE_REG;
		targetP->operandTypes[D] = OPERAND_TYPE_RM;
		targetP->operandValues[!D] = subByte_ic(bytes[1], 2, 3);//REG
		targetP->operandValues[D] = subByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0]>>2 == 0b100000 && ((bytes[1]&0b00111000) == 0b00101000))
	{	//SUB immediate from RM
		printf("Decoded SUB IMM > RM\n");
		targetP->W = bytes[0] & MASK_BIT_8;
		//S???
		modValue = bytes[1]>>6;
		dispSize =  modValue % 3;
		targetP->mnemonicType = MNEM_SUB;
		bool immediateIsWord = targetP->W && !(bytes[0] & MASK_BIT_7);
		targetP->size = 3 + immediateIsWord + dispSize;
		targetP->operandTypes[0] = OPERAND_TYPE_RM;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDIATE;
		targetP->operandValues[0] = subByte_ic(bytes[1], 5, 3);//REG
		if(immediateIsWord)
		{	//Immediate is only word if W && !S.
			targetP->operandValues[1] = *((s16*)(bytes+2+dispSize));//IMM
		}
		else
		{
			targetP->operandValues[1] = *((s8*)(bytes+2+dispSize));//IMM
		}
	}
	else if(bytes[0]>>2 == 0b001110)
	{	//CMP R/M to/from register
		printf("Decoded CMP RM <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		modValue = bytes[1]>>6;
		dispSize =  modValue % 3;
		targetP->mnemonicType = MNEM_CMP;
		targetP->size = 2 + dispSize;
		targetP->operandTypes[!D] = OPERAND_TYPE_REG;
		targetP->operandTypes[D] = OPERAND_TYPE_RM;
		targetP->operandValues[!D] = subByte_ic(bytes[1], 2, 3);//REG
		targetP->operandValues[D] = subByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0] == 0b01110101)
	{	//JNZ
		printf("Decoded JNZ\n");
		targetP->mnemonicType = MNEM_JNZ;
		targetP->size = 2;
		targetP->operandTypes[0] = OPERAND_TYPE_IMMEDIATE;
		targetP->operandTypes[0] = OPERAND_TYPE_NONE;
		targetP->operandValues[0] = *(s8*)(bytes+1);
	}
	else
	{
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n");
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n");
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n\n");
	}
}
#endif
