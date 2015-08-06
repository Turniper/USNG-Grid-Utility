//Matt Hegarty
//6/17/2015
//A program to superimpose the USNG upon properly EXIF tagged UAV images

//--------------------------------------------------------------------------------------------
//Relevant Tech Details for the Sigma DP1 (Camera we used)
//Focal length: 16.6 mm (28mm in 35mm equivalent, these formulae use the first number though)
//Sensor Size: 20.7 x 13.8 mm
//--------------------------------------------------------------------------------------------

//Requires the two other executables in the repository to function, neither were written by me.

//-------------------------------------------------------------------------------------------
//Some info about USNG
//The first 5 characters identify a 100,000 m square
//The remaining characters identify a location in terms of Easting and Northing
//E and N always use the same number of characters, and E always come first.
//-------------------------------------------------------------------------------------------

#include "tinydir.h"
#include <direct.h>
#include "CalcSize.h"
#include "CalcCoords.h"
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <fstream>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>

using namespace cv;
using namespace std;

Mat src;

string WINDOW_NAME = "USNG Utility Preview Window";
string IMG_EXT[] = { "jpg", "jpeg", "png", "bmp" };
string IMAGE_DIR = "";	//Stores Argv[1]

//stops all execution permanently, keeps window open.  For debug purposes.
void stop() {
	while (true)
		waitKey(0);
}

double rationalStringtoDouble(string rational) {
	//I think this one is self explanatory.  Necessary because exif stores some tag fields as rationals.
	//If the value isn't rational (No '/'), it treats it as a double.
	double dub;
	int position = 0;
	position = rational.find_first_of('/');
	//strtod automatically extracts the numberator when run on a rational string.
	double num = strtod(rational.c_str(), nullptr);
	rational.erase(0, position+1);
	double denom = strtod(rational.c_str(), nullptr);
	dub = num/denom;
	if (position == -1)	//(No '/' in string)
		return num;
	return dub;
}


vector<string> readDir(string dir_name)
{
	tinydir_dir dir;
	vector<string> files;
	if (tinydir_open(&dir, dir_name.c_str()) == -1) {
		cerr << "Couldn't find directory\n";
		strerror(errno);
	}
	//cerr << "Entering While next loop\n";
	while (dir.has_next)
	{
		tinydir_file file;
		//cerr << "Reading file\n";
		tinydir_readfile(&dir, &file);
		//cerr << "Read file\n";
		string ext = string(file.extension);
		transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (!file.is_dir && find(begin(IMG_EXT), end(IMG_EXT), ext) != end(IMG_EXT))
		{
			files.push_back(file.name);
			//cout << file.name << endl;
		}

		tinydir_next(&dir);
	}

	tinydir_close(&dir);
	return files;
}


int main(int argc, char** argv) {
	double altitude;	//Should be positive.  If it's negative, we have problems.  Serious problems.  Or bad exif data.
	string latitude;	//+ for N, - for South, format is DDMMSS.nn
	string longitude;	//+ for W, - for E, format is DDDMMSS.nn
	pair<double, double> dimensions;	//Stores physical dimensions of the picture after they are returned.
	char GPSLatitudeRef = ' ';
	char GPSLongitudeRef = ' ';
	string usngCoords;
	string cmd = "";
	string temp;
	const int MetersPerLine = 20;

	/*  For testing things.
	string test = "45345/2";
	double test2;
	test2 = rationalStringtoDouble(test);
	cout << test2;
	while (true)
	waitKey(0);
	*/

	if (argc != 5) {
		cout << "You provided an incorrect number of arguments.  Each argument should be separated by spaces, not commas.";
		cout << "USAGE: Argument 1:Directory_Path (Should be full of exif tagged jpg images) \n";
		cout << "USAGE: Argument 2-4: focal_length sensor_size_x sensor_size_y \n";
		cout << "USAGE: No numerical arguments should contain units, camera measurements are assumed to be millimeters, but as long as they are the same unit for all arguments, it will work. \n";
		cout << "USAGE: Altitude must be provided in meters, in the GPSAltitude exif tag on each image.  GPS location should also be provided in the appropriate exif tags. \n";
		return 1;
	}

	//Open directory
	IMAGE_DIR = argv[1];
	vector<string> files = readDir(IMAGE_DIR);

	string strGriddedPath = IMAGE_DIR + "/gridded";
	#if defined(_WIN32)
		_mkdir(strGriddedPath.c_str());
	#else 
		mkdir(strGriddedPath.c_str(), 0777); //Just in case for unix
	#endif

	//for each image
	for(int i = 0; i < files.size(); i++) {
		//cout << files.at(i) << "\n";

		/*  Debug purposes, for testing different CalcSize Algorithms
		pair<double, double> pear = CalcSize2(230, 20.7, 13.8, 16.6);
		cout << pear.first << "\n";
		cout << pear.second << "\n";
		*/


		//Extract exif data
		//set cmd to the appropriate command
		cmd = "START exiftool.exe ";
		cmd += (IMAGE_DIR + '/' + files[i]);
		cmd += " -GPSAltitude -GPSLatitude -GPSLongitude -c %d%d%.8f -w TEMPgpsData.txt";
		temp = exec(cmd.c_str());
		cout << "Ran exiftool for: " << files.at(i) << "\n";

		//Read exif file
		ifstream GPS_STREAM;
		string loc = (IMAGE_DIR + '/' + files[i]);
		loc.erase(loc.end() - 4, loc.end());
		loc += "TEMPgpsData.txt";
		//cout << loc << "\n";
		GPS_STREAM.open(loc);
		if (!GPS_STREAM.is_open())
			return 2;
		GPS_STREAM >> temp >> temp;
		if (temp != "Altitude") {
			cerr << "Bad GPS data, can't find altitude (Looking at the GPSAltitude exif tag). "  
			"Close the program and try again with properly tagged data.";
			stop();
		}
		GPS_STREAM >> temp >> temp;
		altitude = rationalStringtoDouble(temp);
		cout << "Got Altitude: " << altitude << "\n";

		GPS_STREAM >> temp >> temp >> temp;	//Skips unit, newline, and then GPS.
		if (temp != "Latitude") {
			cerr << "Bad GPS data, can't find latitude (Looking at the GPSLatitude exif tag). "  
			"Close the program and try again with properly tagged data.";
			stop();
		}
		GPS_STREAM >> temp >> latitude;		//Format code is in the command running exiftool (-c).
		

		//Get lat/long
		GPS_STREAM >> temp >> temp >> temp;	//Skips unit, newline, and then GPS.
		if (temp != "Longitude") {
			cerr << "Bad GPS data, can't find longitude (Looking at the GPSLongitude exif tag). "  
			"Close the program and try again with properly tagged data.";
			stop();
		}
		GPS_STREAM >> temp >> longitude;		//Format code is in the command running exiftool (-c).

		GPS_STREAM.close();


		//latitude = rationalStringtoDouble(temp);
		//longitude = rationalStringtoDouble(temp);

		//This really should always be N, but covering the other case just in case.
		if (GPSLatitudeRef == 'S')
			latitude.insert(0, "S");
		else
			latitude.insert(0, "N");
		//Same deal as above, in the U.S. all coordinates should be W.
		if (GPSLongitudeRef == 'E')
			longitude.insert(0, "E");
		else
			longitude.insert(0, "W");


		//Truncate length of the strings to work with the toUSNG command.
		/////////////////////////////////////////////////////
		longitude.erase(longitude.end() - 6, longitude.end());
		latitude.erase(latitude.end() - 6, latitude.end());

		//Add padding zero to the start of longitude
		longitude.insert(1, "0");

		cout << "Got Longitude: " << longitude << "\n";
		cout << "Got Latitude: " << latitude << "\n";

		string loc2 = (IMAGE_DIR + '/' + files[i]);
		loc2.erase(loc2.end() - 4, loc2.end());
		loc2 += "TEMPcoordData.txt";

		//convert exif coords to USNG (Creates temp file)
		convertLatLongToUSNG(latitude, longitude, loc2);

		//Read USNG temp file
		ifstream COORD_STREAM;
		COORD_STREAM.open(loc2);
		if (!COORD_STREAM.is_open())
			return 2;

		char temp2[256];

		while (temp2[0] != '-')
			COORD_STREAM.getline(temp2, 256);
		//Get's the final line, with the coordinate output.
		//cout << temp2 << "\n";
		COORD_STREAM >> temp >> temp >> temp >> temp >> temp >> temp;
		//Drop extra characters
		temp.erase(temp.end() - 4, temp.end());
		usngCoords = temp;
		cout << "USNG Coordinate for this picture is: " << usngCoords << "\n";
		COORD_STREAM.close();


		//Calculate size of image
		double hx = strtod(argv[3], nullptr);
		double hy = strtod(argv[4], nullptr);
		double f = strtod(argv[2], nullptr);	//The focal length of the camera as a double.  See usage note for unit information.
		dimensions = CalcSize2(altitude, hx, hy, f);
		cout << "Calculated size of image: x = " << dimensions.first << " m; y = " << dimensions.second << "\n";
		//dimensions.first is x, second is y.

		//Image manipulation operations begin below here.
		//--------------------------------------------------------
		//Calculate how many pixels lines are to be separated by.
		int numOfLinesX = dimensions.first / MetersPerLine;
		int numOfLinesY = dimensions.second / MetersPerLine;
		cout << numOfLinesX << " lines in the horizontal direction, " << numOfLinesY << " lines in the vertical direction. \n";

		//Get size of image in px
		//Determine how many pixels per line.
		src = imread((IMAGE_DIR + '/' + files[i]), 1);
		if (!src.data)
		{
			cout << "Error: " + (IMAGE_DIR + files[i]) + " not found." << endl;
			return 3;
		}
		cout << "The image has " << src.cols << " pixels in the x direction.\n";
		cout << "The image has " << src.rows << " pixels in the y direction.\n";
		//Impose and label grid
		int pixPerLineX = src.cols/numOfLinesX;
		int pixPerLineY = src.rows/numOfLinesY;
		cout << "Pixels per line (x, then y): \n" << pixPerLineX << "\n" << pixPerLineY << "\n";
		//line(src, Point(-10, -10), Point(10000, 10000), Scalar(110, 220, 0), 2, 8);
		
		//Draw X lines
		for (int i = 0; i < numOfLinesX; i++) {
			line(src, Point((i * pixPerLineX), -10), Point((i * pixPerLineX), src.rows), Scalar(0, 0, 255), 2, 8);
		}
		//Draw Y lines
		for (int i = 0; i < numOfLinesY; i++) {
			line(src, Point(-10, (i * pixPerLineY)), Point(src.cols, (i * pixPerLineY)), Scalar(0, 0, 255), 2, 8);
		}
		//Print center coordinate

		putText(src, usngCoords, Point(src.cols/2, src.rows/2), FONT_HERSHEY_SIMPLEX, 2, Scalar(0, 0, 255), 4);
		circle(src, Point(src.cols / 2, src.rows / 2), 8, Scalar(0, 0, 255), 8, 8);



		//Save the file
		//----------------------------------------------------------------
		//string strGriddedPath = IMAGE_DIR + "/gridded";
		

		imwrite((strGriddedPath + '/' + files[i]), src);
		//imshow(WINDOW_NAME, src);


		//Delete temp files for the current image.
		if (std::remove(loc.c_str()) != 0)
			cerr << "Error Deleting TempExifData file named: " << loc << "\n";
		if (std::remove(loc2.c_str()) != 0)
			cerr << "Error Deleting TempGpsCoordinate file named: " << loc << "\n";
	}

	cout << "All the operations were successful, all pictures are now tagged with their location and gridded. \n";
	cout << "They are in a subfolder of the target directory named 'Gridded'.  You can close this window.";
	//stop();
	return 0;
}

//Return error codes -
//0 = successful run
//1 = input format error
//2 = bad/malformed data
//3 = internal/graphics
