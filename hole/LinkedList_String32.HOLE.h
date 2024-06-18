/*	========================================================================
	//////////////////////////////////////
	// HEADER ONLY LIBRARY OR EXTENSION //
	//////////////////////////////////////
	========================================================================
	(C) Copyright 2024 by structJim, All Rights Reserved.
	This software is provided 'as-is', without any express or implied
	warranty. In no event will the author(s) be held liable for any
	damages arising from the use of this software.
	========================================================================
	A linked list implementation for strings with a max size of 32,
	including null terminators. (Strings exceeding 32 bytes will be
	truncated, with null terminators forced.)
	========================================================================
	How to use:
	Declare new lists with the construct_ macro, which zeroes out members.
	ALWAYS use the derstruct_ macro on an LL before it leaves scope.
	========================================================================*/

#ifndef hole_list_s32
#define hole_list_s32
#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<stdbool.h>
#define construct_LL_S32(listName) struct LinkedList_String32 listName = {0}
#define destruct_LL_S32(listName) emptyLL_S32(&listName)

struct Node_String32{
	char data[32];
	struct Node_String32 *prevP, *nextP;
};
struct LinkedList_String32{
	int length;
	struct Node_String32 *headP, *tailP;
};

//Function reference:
void appendLL_S32(struct LinkedList_String32 *listINP, char *stringIN);
void prependLL_S32(struct LinkedList_String32 *listINP, char *stringIN);
char*getIndexLL_S32(struct LinkedList_String32 *listINP, int indexIN);
char*getTailLL_S32(struct LinkedList_String32 *listINP);
char*getHeadLL_S32(struct LinkedList_String32 *listINP);
bool deleteIndexLL_S32(struct LinkedList_String32 *listINP, int indexIN);
bool insertAfterIndexLL_S32(struct LinkedList_String32 *listINP, int indexIN, char *stringIN);
void emptyLL_S32(struct LinkedList_String32 *listINP);

/*========================================================================*/
/*========================================================================*/
/*========================================================================*/

void appendLL_S32(struct LinkedList_String32 *listINP, char *stringIN)
{
	struct Node_String32 *newNodeP = malloc (sizeof(struct Node_String32));

	//Copy string contents.
	strncpy(newNodeP->data, stringIN, 31);
	//Force null terminator.
	newNodeP->data[31] = '\0';

	if(listINP->length++)
	{	//Point old tail forward at the new node.
		listINP->tailP->nextP = newNodeP;
		//Point new node back at the old tail.
		newNodeP->prevP = listINP->tailP;
		//New node has no next.
		newNodeP->nextP = NULL;
		//New node is new tail.
		listINP->tailP = newNodeP;
	}
	else
	{	//First node. It's head and tail!
		listINP->headP = newNodeP;
		listINP->tailP = newNodeP;
		//New/first node has no prev or next.
		newNodeP->prevP = NULL;
		newNodeP->nextP = NULL;
	}
}
void prependLL_S32(struct LinkedList_String32 *listINP, char *stringIN)
{
	struct Node_String32 *newNodeP = malloc (sizeof(struct Node_String32));

	//Copy string contents.
	strncpy(newNodeP->data, stringIN, 31);
	//Force null terminator.
	newNodeP->data[31] = '\0';

	if(listINP->length++)
	{	//Point old head back at the new node.
		listINP->headP->prevP = newNodeP;
		//Point new node forward at the old head.
		newNodeP->nextP = listINP->headP;
		//New node has no prev.
		newNodeP->prevP = NULL;
		//New node is new head.
		listINP->headP = newNodeP;
	}
	else
	{	//First node. It's head and tail!
		listINP->headP = newNodeP;
		listINP->tailP = newNodeP;
		//New/first node has no prev or next.
		newNodeP->prevP = NULL;
		newNodeP->nextP = NULL;
	}
}
bool insertAfterIndexLL_S32(struct LinkedList_String32 *listINP, int indexIN, char *stringIN)
{
	if(indexIN<0 || indexIN >= listINP->length){return false;} //OOB guard.
	else if((indexIN+1) == listINP->length)
	{	//Insert after last index. We can just append instead.
		appendLL_S32(listINP, stringIN);
	}
	else
	{	//This is a proper insert.
		struct Node_String32 *newNodeP = malloc (sizeof(struct Node_String32));
		struct Node_String32 *beforeNewP, *afterNewP;

		//Copy string contents.
		strncpy(newNodeP->data, stringIN, 31);
		//Force null terminator.
		newNodeP->data[31] = '\0';

		//Start beforeNewP at head, tetatively.
		beforeNewP = listINP->headP;
		//Move beforeNewP up, accordingly.
		for (int i=0 ; i<indexIN ; i++)
		{
			beforeNewP = beforeNewP->nextP;
		}
		afterNewP = beforeNewP->nextP;

		//Link it all up.
		beforeNewP->nextP = newNodeP;
		afterNewP->prevP = newNodeP;
		newNodeP->prevP = beforeNewP;
		newNodeP->nextP = afterNewP;
		
		listINP->length++;
	}
	return 1;
}
bool deleteIndexLL_S32(struct LinkedList_String32 *listINP, int indexIN)
{
	if(indexIN<0 || indexIN >= listINP->length){return false;} //OOB guard.
	else if(indexIN == 0)
	{	//Delete first.
		listINP->headP = listINP->headP->nextP;
		free(listINP->headP->prevP);
		listINP->headP->prevP = NULL;
	}
	else if((indexIN+1) == listINP->length)
	{	//Delete last.
		listINP->tailP = listINP->tailP->prevP;
		free(listINP->tailP->nextP);
		listINP->tailP->nextP = NULL;
		//TODO: Handle ONLY node deletion
	}
	else
	{	//Middle node delete.
		struct Node_String32 *targetNodeP = listINP->headP;
		//Move targetNodeP up, accordingly.
		for (int i=0 ; i<indexIN ; i++)
		{
			targetNodeP = targetNodeP->nextP;
		}
		//Link it all up.
		targetNodeP->prevP->nextP = targetNodeP->nextP;
		targetNodeP->nextP->prevP = targetNodeP->prevP;
		free(targetNodeP);
	}
	listINP->length--;
	return 1;
}
char *getHeadLL_S32(struct LinkedList_String32 *listINP)
{
	return listINP->headP->data;
}
char *getTailLL_S32(struct LinkedList_String32 *listINP)
{
	return listINP->tailP->data;
}
char *getIndexLL_S32(struct LinkedList_String32 *listINP, int indexIN)
{	//Point targetNodeP at head.
	struct Node_String32 *targetNodeP = listINP->headP;
	//Move targetNodeP up to desired target node.
	for(int i=0 ; i<indexIN ; i++)
	{
		targetNodeP = targetNodeP->nextP;
	}
	return targetNodeP->data;
}
void emptyLL_S32(struct LinkedList_String32 *listINP)
{
	struct Node_String32 *nextTargetNodeP, *nowTargetNodeP;
	//Start destruction at tail.
	nextTargetNodeP = listINP->tailP;
	for(; listINP->length ; listINP->length--)
	{
		nowTargetNodeP = nextTargetNodeP;
		nextTargetNodeP = nowTargetNodeP->prevP; //At head, ==NULL.
		free(nowTargetNodeP);
	}
	listINP->headP = NULL;
	listINP->tailP = NULL;
	//All nodes freed. listINP->length is now 0.
}
#endif
