#include<stdbool.h>
#include<stdint.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<math.h>
//#include<time.h>
#include"../hole/jims_general_tools.HOLE.h"
//#include"listing_0065_haversine_formula.cpp"

const u32 samples_needed = 6;
const u16 X_ABS_MAX = 180;
const u16 Y_ABS_MAX = 90;

int main()
{
/*	JSON format...
	{"pairs":[
	{"x0":1.0, "y0":1.0, "x1":1.0, "y1":1.0},
	...
	{"x0":1.0, "y0":1.0, "x1":1.0, "y1":1.0}
	]}
*/		
	FILE *fOutP = fopen("output/hav_values.json", "w");
	fprintf(fOutP, "{\"pairs\":[\n");
	for(int i=0 ; i<samples_needed ; i++)
	{
		fprintf(
			//(Ratio of 2*MAX)-MAX puts 0 at the center.
			fOutP, "    {\"x0\":%f, \"y0\":%f, \"x1\":%f, \"y1\":%f}%s",
			rand() / (f64)RAND_MAX * 2*X_ABS_MAX - X_ABS_MAX,//-180~180
			rand() / (f64)RAND_MAX * 2*Y_ABS_MAX - Y_ABS_MAX,//-90~90
			rand() / (f64)RAND_MAX * 2*X_ABS_MAX - X_ABS_MAX,//-180~180
			rand() / (f64)RAND_MAX * 2*Y_ABS_MAX - Y_ABS_MAX,//-90~90
			i==(samples_needed-1) ? "\n" : ",\n"
		);
		//fprintf(fOutP, (samples_needed-i)==1 ? "\n" : ",\n");
	}
	fprintf(fOutP, "]}\n");
	fclose(fOutP);
	return 0;
}
