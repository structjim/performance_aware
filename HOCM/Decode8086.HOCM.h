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
const u1 MASK_BIT_1 = 0b10000000;
const u1 MASK_BIT_2 = 0b01000000;
const u1 MASK_BIT_3 = 0b00100000;
const u1 MASK_BIT_4 = 0b00010000;
const u1 MASK_BIT_5 = 0b00001000;
const u1 MASK_BIT_6 = 0b00000100;
const u1 MASK_BIT_7 = 0b00000010;
const u1 MASK_BIT_8 = 0b00000001;
enum{MNEM_ADD, MNEM_CMP, MNEM_MOV, MNEM_SUB};
enum{OPERAND_TYPE_NONE, OPERAND_TYPE_IMMEDITATE, OPERAND_TYPE_REG, OPERAND_TYPE_RM};
struct Instruction_8086
{
	//Note that we don't need any strings for mnemonic or operand names.
	//Strings will be handled by a disassembler. Logical simulation can
	//simply go without and to the operations.
	u2 operandValues[2];//REG bits, R/M bits, Immediate values...
	u1 operandTypes[2];
	u1 dispValue, instrSize, mnemonicType, modValue;
	bool W;
};
typedef struct Instruction_8086 Instr8086;
u1 subByte_ic(u1 byteIN, u1 firstBitIndex, u1 bitCount)
{
	//"ic" because argument 2 and 3 are (index, count).
	//Example: Pass this (0b00111000, 2, 3) and you'll get 7.
	assert(firstBitIndex < 8);
	assert(bitCount > 0);
	assert(firstBitIndex+bitCount <= 8);
	
	u1 byteOUT = byteIN;
	u1 leftMask = 0b11111111 >> firstBitIndex;
	u1 rightShiftsNeeded = 8 - (firstBitIndex + bitCount);
	byteOUT = byteOUT & leftMask;
	return byteOUT >> rightShiftsNeeded;
}
u1 decodeBinaryAndPopulateInstruction(Instr8086 *targetP, u1 *bytes)
{
	u1 instructionSizeOUT = 0;
	u1 dispSize = 0;
	bool D; //D means reg is dest
	//Instructions appear here in same order as 8086 manual (page 4-20).
	if(bytes[0]>>2 ==  0b100010)
	{	//MOV R/M to/from register
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		dispSize = subByte_ic(bytes[1], 0, 2) % 3;
		targetP->mnemonicType = MNEM_MOV;
		targetP->instrSize = 2 + dispSize;
		targetP->operandTypes[!D] = OPERAND_TYPE_REG;
		targetP->operandTypes[D] = OPERAND_TYPE_RM;
		targetP->operandValues[!D] = subByte_ic(bytes[1], 2, 3);//REG
		targetP->operandValues[D] = subByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0]>>4 == 0b1011)
	{	//MOV immediate to register
		targetP->W = bytes[0] & MASK_BIT_5;
		targetP->mnemonicType = MNEM_MOV;
		targetP->instrSize = 2+targetP->W;
		targetP->operandTypes[0] = OPERAND_TYPE_REG;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDITATE;
		targetP->operandValues[0] = subByte_ic(bytes[0], 5, 3);//REG
		targetP->operandValues[1] = *((u2*)(bytes+1));//IMM
	}
}
#endif
