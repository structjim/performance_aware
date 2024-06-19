/*	========================================================================
	(C) Copyright 2024 by structJim, All Rights Reserved.
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the author(s) be held liable for any
	damages arising from the use of this software.
	========================================================================
	Just general stuff that I'll use in basically every project.
	========================================================================*/

#ifndef hole_jims_general_tools
#define hole_jims_general_tools
#include<stdbool.h>
#include<stdint.h>

#define DEBUG_PRINT if(DEBUG)printf
#define SPAM(spamn) for(int spami=0; spami++<spamn;)printf

//If you want debug mode, flip the bool main(). DON'T CHANGE IT HERE!
bool DEBUG=0;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#endif
