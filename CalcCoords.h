//Matt Hegarty
//6/23/2015
//Wrapper to call usng_conv, a fortran program that converts other coordinates into USNG format.
//usng_conv is a United States government program, and can be found at http://www.ngs.noaa.gov/TOOLS/usng.shtml


#include <string>
#include <iostream>
#include <stdio.h>
#include "exec.h"
#include <cstring>

using namespace std;

//Lat/Long should be in CDDMMSS.nn format, where the first C is N/S/E/W code.
void convertLatLongToUSNG(string latitude, string longitude, string loc) {
	string command = "";
	string progOutput;
	command += "START /B usng_conv GP2US ";

	command += latitude;
	command += " ";
	command += longitude;

	command += " 83 > ";	//Add datum code
	command += loc;

	//cout << command << "\n";  //Debug purposes


	progOutput = exec(command.c_str());
	//Runs the program via command line.
	//cout << progOutput;	//Debug
	return;
}
