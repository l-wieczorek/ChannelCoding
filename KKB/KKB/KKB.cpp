#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <limits>
#include "Kanal.cpp"
#define PI 3.141592654

using namespace std;

int main()
{
	int nob; //number of bits
	float Eb_N0_min, Eb_N0_max, step;
	try{
	cout << "Number of bits in one data frame delivered to encoder: ";
	cin >> nob;
	if (cin.fail())
		throw "Type integer value";
	cout << "Minimum value of Eb/N0: ";
	cin >> Eb_N0_min;
	if (cin.fail())
		throw "Type float value";
	cout << "Maximum value of Eb/N0: ";
	cin >> Eb_N0_max;
	if (cin.fail())
		throw "Type float value";
	cout << "Step change of parameter Eb/N0: ";
	cin >> step;
	if (cin.fail())
		throw "Type float value";
	}
	catch (const char* msg) {
		cerr << "Error! "<<  msg << endl;
		system("pause");
		exit(0);	
	}
	if (Eb_N0_max > Eb_N0_min) {
		//commit test

	}
	else
		cout << "You put wrong values, run program one more time!" << endl;
	system("pause");
}

