#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdio.h>
#include <malloc.h>
#include <limits>
#include <cstdlib>
#include "Kanal.hpp"
#include <fstream>
#include <algorithm>
#include <iterator>
#include <iomanip>
#include "DecodingTable.h"
#define PI 3.141592654

using namespace std;

int main()
{
	int nob; //number of bits
	float Es_N0_min, Es_N0_max, step;
	fstream fileInputBits, fileEncodedBits, fileAfterCanalBits, fileDecodedBits, fileResults;
	const int frameLength = 3;
	const int encodedBlockLength = 6;

	// files where outputs are stored 
	fileInputBits.open("InputBits.txt", ios::out);
	fileEncodedBits.open("EncodedBits.txt", ios::out);
	fileAfterCanalBits.open("AfterCanalBits.txt", ios::out);
	fileDecodedBits.open("DecodedBits.txt", ios::out);
	fileResults.open("Results.txt", ios::out);

	// generator matrix
	int GM[3][6] = { {1, 1, 0, 1, 0, 0},{0, 1, 1, 0, 1, 0},{1, 0, 1, 0, 0, 1} };

	// test matrix
	int TM[3][6] = { {1, 0, 0, 1, 1, 0},{0, 1, 0, 0, 1, 1},{0, 0, 1, 1, 0, 1} };

	// transposed test matrix
	int TM_t[6][3] = { {1, 0, 0},{0, 1, 0},{0, 0, 1},{1, 0, 1},{1, 1, 0},{0, 1, 1} };

	// interaction with user
	try {
		cout << "Number of bits in one data frame delivered to encoder: ";
		cin >> nob;
		if (cin.fail())
			throw "Type integer value";
		if (nob < 0)
			throw "Number of bits should be greater than 0";
		cout << "Minimum value of Es/N0: ";
		cin >> Es_N0_min;
		if (cin.fail())
			throw "Type float value";
		cout << "Maximum value of Es/N0: ";
		cin >> Es_N0_max;
		if (cin.fail())
			throw "Type float value";
		if (Es_N0_min >= Es_N0_max)
			throw "Minimum value cannot be greater than maximum one!";
		cout << "Step change of parameter Es/N0: ";
		cin >> step;
		if (cin.fail())
			throw "Type float value";
		if (step > (Es_N0_max - Es_N0_min))
			throw "Type proper step value";
		if (step < 0)
			throw "Step should be greater than 0";
	}
	catch (const char* msg) {
		cerr << "Error! " << msg << endl;
		system("pause");
		exit(0);
	}

	// dividing input into 3-bits blocks
	int *inputData = new int[nob];
	srand(time(NULL));
	for (int i = 0; i < nob; i++) {
		inputData[i] = rand() % 2;
	}
	int var = 0;
	int **inputDataBlocks = new int*[nob / frameLength];
	for (int i = 0; i < nob / frameLength; i++) {
		inputDataBlocks[i] = new int[frameLength];
		for (int j = 0; j < frameLength; j++) {
			inputDataBlocks[i][j] = inputData[var];
			var++;
		}
	}

	// saving input blocks of bits into a file
	for (int i = 0; i < nob / frameLength; i++) {
		for (int j = 0; j < frameLength; j++) {
			fileInputBits << inputDataBlocks[i][j];
		} 
	}

	// multiplying matrices to get code words
	int **encodedBlocks = new int*[nob / frameLength];
	for (int i = 0; i < nob / frameLength; i++)
	{
		encodedBlocks[i] = new int[encodedBlockLength]();
		for (int j = 0; j < encodedBlockLength; j++)
		{
			for (int k = 0; k < frameLength; k++)
			{
				encodedBlocks[i][j] = encodedBlocks[i][j] ^ (inputDataBlocks[i][k] & GM[k][j]);
			}
		}
	}

	// saving encoded bits into a file
	for (int i = 0; i < nob / frameLength; i++)
	{
		for (int j = 0; j < encodedBlockLength; j++)
		{
			fileEncodedBits << encodedBlocks[i][j];
		}
		fileEncodedBits << endl;
	}

	// moze by je wyjebac wszystkie na gore gdzie tam deklarujemy wszystko, tak samo te encodedBlocks wyzej itp, w jedno miejsce wszystko?
	float **encodedBlocksAfterCanal = new float*[nob / frameLength];
	int **blocksAfterDecision = new int*[nob / frameLength];
	int **syndroms = new int*[nob / frameLength];
	int **bitsRestored = new int*[nob / frameLength];
	int **blocksAfterCorrection = new int*[nob / frameLength];
	int *outputData = new int[nob];

	// main loop
	// transmission in canal with increasing Es_N0 and then saving blocks after canal into a file
	while (Es_N0_min <= Es_N0_max) {
		fileAfterCanalBits << Es_N0_min << endl;
		for (int i = 0; i < nob / frameLength; i++)
		{
			encodedBlocksAfterCanal[i] = new float[encodedBlockLength]();
			for (int j = 0; j < encodedBlockLength; j++)
			{
				kanal(Es_N0_min, encodedBlockLength, encodedBlocks[i], encodedBlocksAfterCanal[i]);
				fileAfterCanalBits << encodedBlocksAfterCanal[i][j];
			}
			fileAfterCanalBits << endl;
		}

		// decision-making algorythm in a receiver on a data from canal
		for (int i = 0; i < nob / frameLength; i++)
		{
			blocksAfterDecision[i] = new int[encodedBlockLength]();
			for (int j = 0; j < encodedBlockLength; j++)
			{
				if (encodedBlocksAfterCanal[i][j] > 0)
					blocksAfterDecision[i][j] = 1;
				else
					blocksAfterDecision[i][j] = 0;
			}
		}
		
		// decoding part
		// correction of some errors using decoding table from DecodingTable.cpp 
		for (int i = 0; i < nob / frameLength; i++)
		{
			blocksAfterCorrection[i] = new int[encodedBlockLength]();
			for (int j = 0; j < encodedBlockLength; j++)
			{
				blocksAfterCorrection[i][j] = blocksAfterDecision[i][j];
			}
			DecodingTable(blocksAfterCorrection[i]);
		}

		// taking only information bits
		int var = 0;
		for (int i = 0; i < nob / frameLength; i++)
		{
			for (int j = 3; j < encodedBlockLength; j++)
			{
				outputData[var] = blocksAfterCorrection[i][j];
				var++;
			}
		}

		// saving decoded bits into a file and counting failedBits for BER calculations
		int failedBits = 0;
		fileDecodedBits << "Es/N0: " << Es_N0_min << endl;
		for (int i = 0; i < nob; i++)
		{
			fileDecodedBits << outputData[i];
			if (outputData[i] != inputData[i])
				failedBits++;
		}
		fileDecodedBits << endl << endl;

		// calculating BER and saving it to file with proper Es/N0 value
		float BER = (float)failedBits / (float)nob;
		fileResults << "Es/N0=" << Es_N0_min << " BER=" << BER << endl;
		cout << "Es/N0=" << Es_N0_min << " -> BER=" << BER << endl;
		
		// when somebody put Es_N0_min-Es_N0_max range with too big step for this range, the border value
		// of Es_N0_max will be also contained in the output - e.g. Esmin = 0, Esmax = 10, step = 3 -> instead of simulating with 0,3,6,9,
		// there is also a limit (max) value simulation - 0,3,6,9,10
		if ((Es_N0_max - Es_N0_min) < step && (Es_N0_max - Es_N0_min) !=0)
			Es_N0_min = Es_N0_max;
		else 
			Es_N0_min += step;
	}

	delete[] inputDataBlocks;
	delete[] inputData;
	delete[] encodedBlocks;
	delete[] syndroms;
	delete[] blocksAfterDecision;
	delete[] blocksAfterCorrection;
	delete[] outputData;
	fileInputBits.close();
	fileEncodedBits.close();
	fileAfterCanalBits.close();
	fileDecodedBits.close();
	fileResults.close();
	system("pause");
}


/////////// TO NIECH NA RAZIE ZOSTANIE, ZOBACZYMY CO RAJCH MI ODPISZE
		////
		//// syndroms calculating
		//for (int i = 0; i < nob / frameLength; i++)
		//{
		//	syndroms[i] = new int[frameLength]();
		//	for (int j = 0; j < frameLength; j++)
		//	{
		//		for (int k = 0; k < encodedBlockLength; k++)
		//		{
		//			syndroms[i][j] = syndroms[i][j] ^ (encodedBlocksAfterDecision[i][k] & TM_t[k][j]);
		//		}
		//	}
		//}

		//// TU JEST CHUJNIA
		//int checkVector[frameLength];
		//for (int i = 0; i < frameLength; i++)
		//	checkVector[i] = 0;

		//for (int i = 0; i < nob / frameLength; i++)
		//{
		//	bitsRestored[i] = new int[encodedBlockLength]();
		//	for (int j = 0; j < encodedBlockLength; j++)
		//	{
		//		if (equal(checkVector, checkVector + sizeof(checkVector) / sizeof(*checkVector), syndroms[i]))
		//		{
		//			for (int k = 0; k < frameLength; k++)
		//			{
		//				bitsRestored[i][j] = bitsRestored[i][j] ^ (syndroms[i][k] & TM[k][j]);
		//			}
		//		}
		//		else bitsRestored[i][j] = encodedBlocksAfterDecision[i][j];
		//	}
		//}

		//for (int i = 0; i < nob / frameLength; i++)
		//{
		//	for (int j = 0; j < frameLength; j++)
		//	{
		//		cout << syndroms[i][j] << " ";
		//	}
		//	cout << endl;
		//}
		//cout << endl;

		//for (int i = 0; i < nob / frameLength; i++)
		//{
		//	for (int j = 0; j < encodedBlockLength; j++)
		//	{
		//		cout << bitsRestored[i][j] << " ";
		//	}
		//	cout << endl;
		//}
		//cout << endl;
