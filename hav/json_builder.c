#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
//#include<time.h>
#include"../hole/jims_general_tools.HOLE.h"
//#include"listing_0065_haversine_formula.h"

const u32 samples_needed = 1500000;

int main()
{
	printf("\nI hav it!\n");

	//f64 f = ReferenceHaversine(1.0, 1.0, 1.0, 1.0, 1.0);
	//f=f;

/*	JSON format...
	{"pairs":[
	{"x0":1.0, "y0":1.0, "x1":1.0, "y1":1.0},
	{"x0":1.0, "y0":1.0, "x1":1.0, "y1":1.0},
	{"x0":1.0, "y0":1.0, "x1":1.0, "y1":1.0}
	]}
*/
	
	//Write to output file
	FILE *fOutP = fopen("output/hav_values.json", "w");
	fprintf(fOutP, "{\"pairs\":[\n");
	for(int i=0 ; i<samples_needed ; i++)
	{
		fprintf(
			fOutP, "    {\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}",
			(rand()/(f64)RAND_MAX) * 360 - 180,
			(rand()/(f64)RAND_MAX) * 360 - 180,
			(rand()/(f64)RAND_MAX) * 360 - 180,
			(rand()/(f64)RAND_MAX) * 360 - 180
		);
		fprintf(fOutP, (samples_needed-i)==1 ? "\n" : ",\n");
	}
	fprintf(fOutP, "]}\n");
	fclose(fOutP);
/*
	unsigned int labelsWritten = 0;
	fprintf(fOutP, "%s", "bits 16"); //Bit width directive counts as 0 bytes
	for(int i=0 ; i<instrs_done ; i++)
	{//Each iteration writes 1 instr
		fprintf(fOutP, "\n%s", getIndexLL_S32(&instrStrings, i));
		bytesWritten += sizes[i];
		
		if(labelsWritten < labelCount)
		{
			//labelIndices[] are byte numbers that need labels after
			//Labels are named after the index of the preceding byte
			if(labelIndices[labelsWritten] == bytesWritten)
			{
				fprintf(fOutP, "\nlabel%i:", bytesWritten);
				labelsWritten++;
			}
		}
	}
	//Get output file size
	fseek(fOutP, 0, SEEK_END); //Set file position to end
*/
	return 0;
}

/*
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
*/
