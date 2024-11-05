/*	========================================================================
	(C) Copyright 2024 by structJim, All Rights Reserved.
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the author(s) be held liable for any
	damages arising from the use of this software.
	========================================================================
	Just general stuff that I'll use in basically every project.
	========================================================================*/

#ifndef hole_profiler_bad
#define hole_profiler_bad
#include"../hole/jims_general_tools.HOLE.h"
#include <x86intrin.h>
#include <sys/time.h>

//This number can be anything. If it's exceeded, splits
//will overflow into heap-allocated blocks of the same size
//and print a warning when the final timings are printed.
const u16 ARENA_SPLIT_COUNT = 16;
//(Not an arena YET.)

//Structs
typedef struct
{
	char label[32];
	u64 timestamp;
}ProfileSplit;
typedef struct _LLNode_ProfileSplit
{
	struct _LLNode_ProfileSplit *prevP, *nextP;
	ProfileSplit data;
}LLNode_ProfileSplit;
typedef struct
{
	LLNode_ProfileSplit *headP, *tailP;
	u32 n;
	u64 first;
}LL_ProfileSplit;

//Variables
LL_ProfileSplit splits = {0};

//Functions
static u64 GetOSTimerFreq(void)
{
	return 1000000;
}
static u64 ReadOSTimer(void)
{
	struct timeval Value;
	gettimeofday(&Value, 0);
	
	u64 Result = GetOSTimerFreq()*(u64)Value.tv_sec + (u64)Value.tv_usec;
	return Result;
}
ProfileSplit *get_ll_profile_split(LL_ProfileSplit *ll_INP, u32 index_to_get)
{
	if(PROFILE)
	{
		LLNode_ProfileSplit *target_nodeP = ll_INP->headP;
		for(int i=0 ; i<index_to_get ; i++)
		{
			target_nodeP = target_nodeP->nextP;
		}
		assert(target_nodeP != 0);
		return &(target_nodeP->data);
	}
	else return 0;
}

void append_ll_profile_split(LL_ProfileSplit *ll_INP, ProfileSplit *split_IN)
{
	if(PROFILE)
	{
		if(ll_INP->n == 0)
		{
			ll_INP->headP = malloc(sizeof(LLNode_ProfileSplit));
			ll_INP->tailP = ll_INP->headP;
			ll_INP->headP->data = *split_IN; //same as tail
		}
		else
		{
			LLNode_ProfileSplit *old_tailP = ll_INP->tailP;
			LLNode_ProfileSplit *new_tailP = malloc(sizeof(LLNode_ProfileSplit));
			new_tailP->data = *split_IN;
			old_tailP->nextP = new_tailP;
			new_tailP->prevP = old_tailP;
			ll_INP->tailP = new_tailP;
		}
		ll_INP->n++;
	}
}

void _SPLIT(char *labelIN)
{
	if(PROFILE)
	{
		//Add a new split into the profile split LL.
		ProfileSplit new_split;
		strncpy(new_split.label, labelIN, 31);
		new_split.label[31] = '\n';
		new_split.timestamp = ReadOSTimer();
		if( !splits.headP )
		{
			splits.first = new_split.timestamp;
		}
		new_split.timestamp -= splits.first;
		append_ll_profile_split(&splits, &new_split);
	}
}

void PRINT_PROFILE_SPLITS(void)
{
	if(PROFILE)
	{
		printf("\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\n");
		ProfileSplit *splitP;
		for(int i=0 ; i<splits.n ; i++)
		{
			splitP = get_ll_profile_split(&splits, i);
			printf("%31s:%9li us\n", (*splitP).label, (*splitP).timestamp);
		}
		printf("/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\/\\\n");
	}
}

void destruct_ll_profile_split(LL_ProfileSplit *ll_INP)
{
	return;
}

#endif
