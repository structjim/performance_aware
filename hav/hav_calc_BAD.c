#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
#include<assert.h>
//#include<time.h>
#include"../hole/jims_general_tools.HOLE.h"

//We need the first version to be kind of
//naive and BAD, so we'll just use a LL.

//Structs
typedef struct
{
	f64 x0, y0, x1, y1;
}HavSet; //For Haverstein calc
typedef struct _LLNode_HavSet
{
	struct _LLNode_HavSet *prevP, *nextP;
	HavSet data;
}LLNode_HavSet;
typedef struct
{
	LLNode_HavSet *headP, *tailP; u32 n;
}LL_HavSet;

//Functions
HavSet *get_ll_hav_set(LL_HavSet *ll_INP, u32 index);
void append_ll_hav_set(LL_HavSet *ll_INP, HavSet *hav_IN);
f64 parse_f64(FILE *fileP);
bool move_to_next_colon(FILE *fileP);

//Variables
LL_HavSet hav_sets = {0};

//////////////////////
///      MAIN      ///
//////////////////////
int main(int argc, char *argv[])
{
	DEBUG=1;
	DEBUG_PRINT("Values parsed:\n");
	bool end_of_file_reached = false;
	FILE *fInP = fopen(argv[1], "rb");
	//int fInSz = ftell(fInP); //Store position (file size)
	//fseek(fInP, 0L, SEEK_SET);//Return to start of file
	move_to_next_colon(fInP); //To first colon, in "pairs:"
	end_of_file_reached = move_to_next_colon(fInP); //To colon before first value
	while( ! end_of_file_reached)
	{	//Each iteration loads one hav set
		f64 hav_values[4];
		for(int i=0 ; i<4 &&! end_of_file_reached ; i++)
		{	//Each iteration loads one of 4 values for this set
			hav_values[i] = parse_f64(fInP);
			end_of_file_reached = move_to_next_colon(fInP);
		}
		DEBUG_PRINT("\n");
		HavSet hav_set = {hav_values[0], hav_values[1], hav_values[2], hav_values[3]};
		append_ll_hav_set(&hav_sets, &hav_set);
	}
	printf("DONE.\n");
}

HavSet *get_ll_hav_set(LL_HavSet *ll_INP, u32 index_to_get)
{
	LLNode_HavSet *target_nodeP = ll_INP->headP;
	for(int i=0 ; i<index_to_get ; i++)
	{
		target_nodeP = target_nodeP->nextP;
	}
	assert(target_nodeP != 0);
	return &(target_nodeP->data);
}

void append_ll_hav_set(LL_HavSet *ll_INP, HavSet *hav_IN)
{
	if(ll_INP->n == 0)
	{
		ll_INP->headP = malloc(sizeof(LLNode_HavSet));
		ll_INP->tailP = ll_INP->headP;
		ll_INP->headP->data = *hav_IN; //same as tail
	}
	else
	{
		LLNode_HavSet *old_tailP = ll_INP->tailP;
		LLNode_HavSet *new_tailP = malloc(sizeof(LLNode_HavSet));
		new_tailP->data = *hav_IN;
		old_tailP->nextP = new_tailP;
		new_tailP->prevP = old_tailP;
		ll_INP->tailP = new_tailP;
	}
	ll_INP->n++;
}

f64 parse_f64(FILE *fileP)
{
	char now_char = ':';
	f64 this_value_total = 0.0f;
	u32 digit_processed_count = 0;
	u16 digits_before_decimal_count = 0;
	bool is_negative = false; 
	now_char = fgetc(fileP);//First digit this value
	while
	(
		now_char=='.'
		|| now_char=='-'
		|| ((now_char >= '0') && (now_char <= '9')) //is a digit
	)
	{	//Keeps going as long as we get '.' or '-' or a digit
		if(now_char=='-')
		{
			is_negative = true;
		}
		else if(now_char=='.')
		{
			digits_before_decimal_count = digit_processed_count;
		}
		else
		{	//It' a digit. Append it to the value.
			this_value_total *= 10.0f;
			this_value_total += now_char - '0';
			digit_processed_count++;
		}
		now_char = fgetc(fileP);
	}
	//decimal_offset is digit count RIGHT of the decimal
	u16 decimal_offset = digit_processed_count - digits_before_decimal_count;
	for(int j=0 ; j<decimal_offset ; j++)
	{
		this_value_total /= 10.0f;
	}
	if(is_negative)
	{
		this_value_total *= -1.0f;
	}
	DEBUG_PRINT("%f, ", this_value_total);
	return this_value_total;
}

bool move_to_next_colon(FILE *fileP)
{	//Returns false if it reaches end of file.
	char now_char = '?';
	while(now_char != ':' && now_char != EOF)
	{	//Next ':', or end of file
		now_char = fgetc(fileP);
	}
	return now_char==EOF ? true : false;
}
