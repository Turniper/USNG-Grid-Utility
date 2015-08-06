//Matt Hegarty
//6/19/2015
//Function to calculate the size of an image's real dimensions given camera informtion and altitude.

//-----------------------------------------------------------------------------------------
//Formula 1
//theta = 2*arctan(size of sensor in chosen dimension (mm) / (2 * focal distance (mm))
//distance = 2 * altitude * (tangent(theta))  //Assuming my trig is correct.
//
//-----------------------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
//Formula 2
//S = (s-f)*I / f
//Size of side = (altitude - focal length) * sensor size / focal length
//
//-----------------------------------------------------------------------------------------

#include <math.h>
#include <iostream>
#define PI 3.14159265

using namespace std;

//altitude = altitude image was taken at; hx = horizontal sensor size; hy = vertical sensor size; f = focal length;
pair<double, double> CalcSize(double altitude, double hx, double hy, double f) {
	double szVals[2];
	double angX = (2 * atan(hx /(2 * f)));	//atan returns radians
	double angY = (2 * atan(hy / (2 * f)));

	double distX = (2 * altitude * tan(angX));
	double distY = (2 * altitude * tan(angY));

	cout << distX << "\n";
	cout << distY << "\n";

	pair<double, double> pear = make_pair(distX, distY); 
	return pear;
}


pair <double, double> CalcSize2(double altitude, double hx, double hy, double f) {
	pair<double, double> pear;
	double distX = (altitude - (f/1000)) * hx / f;
	double distY = (altitude - (f/1000)) * hy / f;

	pear = make_pair(distX, distY);

	return pear;
}
