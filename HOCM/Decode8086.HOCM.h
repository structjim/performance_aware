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
const u8 MASK_BIT_1 = 0b10000000;
const u8 MASK_BIT_2 = 0b01000000;
const u8 MASK_BIT_3 = 0b00100000;
const u8 MASK_BIT_4 = 0b00010000;
const u8 MASK_BIT_5 = 0b00001000;
const u8 MASK_BIT_6 = 0b00000100;
const u8 MASK_BIT_7 = 0b00000010;
const u8 MASK_BIT_8 = 0b00000001;
enum{MNEM_ADD, MNEM_CMP, MNEM_MOV, MNEM_SUB};
enum{OPERAND_TYPE_NONE, OPERAND_TYPE_IMMEDITATE, OPERAND_TYPE_REG, OPERAND_TYPE_RM};
struct Instruction_8086
{
	//Note that we don't need any strings for mnemonic or operand names.
	//Strings will be handled by a disassembler. Logical simulation can
	//simply go without and to the operations.
	s16 operandValues[2];//REG bits, R/M bits, Immediate values...
	u8 operandTypes[2];
	u8 dispValue, instrSize, mnemonicType, modValue;
	bool W;
};
typedef struct Instruction_8086 Instr8086;
u8 subByte_ic(u8 byteIN, u8 firstBitIndex, u8 bitCount)
{
	//"ic" because argument 2 and 3 are (index, count).
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
		targetP->instrSize = 2 + dispSize;
		targetP->operandTypes[!D] = OPERAND_TYPE_REG;
		targetP->operandTypes[D] = OPERAND_TYPE_RM;
		targetP->operandValues[!D] = subByte_ic(bytes[1], 2, 3);//REG
		targetP->operandValues[D] = subByte_ic(bytes[1], 5, 3);//R/M
	}
	else if(bytes[0]>>4 == 0b1011)
	{	//MOV immediate to register
		printf("Decoded MOV IMM > RM\n");
		targetP->W = bytes[0] & MASK_BIT_5;
		targetP->mnemonicType = MNEM_MOV;
		targetP->instrSize = 2+targetP->W;
		targetP->operandTypes[0] = OPERAND_TYPE_RM;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDITATE;
		targetP->operandValues[0] = subByte_ic(bytes[0], 5, 3);//REG
		targetP->operandValues[1] = *((u16*)(bytes+1));//IMM
	}
	else if(bytes[0]>>2 == 0b000000)
	{	//ADD R/M to/from register
		printf("Decoded ADD RM <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		modValue = bytes[1]>>6;
		dispSize =  modValue % 3;
		targetP->mnemonicType = MNEM_ADD;
		targetP->instrSize = 2 + dispSize;
		targetP->operandTypes[!D] = OPERAND_TYPE_REG;
		targetP->operandTypes[D] = OPERAND_TYPE_RM;
		targetP->operandValues[!D] = subByte_ic(bytes[1], 2, 3);//REG
		targetP->operandValues[D] = subByte_ic(bytes[1], 5, 3);//R/M
	}
	else if((bytes[0]>>2 == 0b100000) && ((bytes[1]&0b00111000) == 0b0))
	{	//ADD immediate to R/M
		printf("Decoded ADD IMM > RM\n");
		targetP->W = bytes[0] & MASK_BIT_8;
		//S???
		modValue = bytes[1]>>6;
		dispSize = modValue % 3;
		targetP->mnemonicType = MNEM_ADD;
		targetP->instrSize = 3 + targetP->W + dispSize;
		targetP->operandTypes[0] = OPERAND_TYPE_RM;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDITATE;
		targetP->operandValues[0] = subByte_ic(bytes[1], 5, 3);//RM
		targetP->operandValues[1] = *((u16*)(bytes+2+dispSize));//IMM
	}
	else if(bytes[0]>>2 == 0b001010)
	{	//SUB R/M to/from register
		printf("Decoded SUB RM <> REG\n");
		D = bytes[0] & MASK_BIT_7;
		targetP->W = bytes[0] & MASK_BIT_8;
		modValue = bytes[1]>>6;
		dispSize =  modValue % 3;
		targetP->mnemonicType = MNEM_SUB;
		targetP->instrSize = 2 + dispSize;
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
		targetP->instrSize = 3 + targetP->W + dispSize;
		targetP->operandTypes[0] = OPERAND_TYPE_RM;
		targetP->operandTypes[1] = OPERAND_TYPE_IMMEDITATE;
		targetP->operandValues[0] = subByte_ic(bytes[1], 5, 3);//REG
		targetP->operandValues[1] = *((u16*)(bytes+2+dispSize));//IMM
	}
	else
	{
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n");
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n");
		printf("[[[DECODING ERROR! COULDN'T SELECT OPERATION TYPE!]]]\n\n");
	}
}
#endif
