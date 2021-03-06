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
#include <cmath>
#include <random>
#include <sstream>
#include "DecodingTable.h"
#define PI 3.141592654

using namespace std;

int main()
{
	int nobWithoutZeropadding; 
	int nob; //number of bits
	float Es_N0_min, Es_N0_max, step;
	fstream fileInputBits, fileEncodedBits, fileAfterCanalBits, fileDecodedBits, fileResults, fileWithoutCodingAfterCanal;
	const int frameLength = 3;
	const int encodedBlockLength = 6;
	ifstream temporaryForTesting; // <- delete

	// files where outputs are stored 
	fileEncodedBits.open("EncodedBits.txt", ios::out);
	fileAfterCanalBits.open("AfterCanalBits.txt", ios::out);
	fileDecodedBits.open("DecodedBits.txt", ios::out);
	fileResults.open("Results.txt", ios::out);
	fileWithoutCodingAfterCanal.open("ClearInputAfterCanal.txt", ios::out);

	// generator matrix
	int GM[3][6] = { {1, 1, 0, 1, 0, 0},{0, 1, 1, 0, 1, 0},{1, 0, 1, 0, 0, 1} };

	// test matrix - not necessary in our way of decoding
	int TM[3][6] = { {1, 0, 0, 1, 1, 0},{0, 1, 0, 0, 1, 1},{0, 0, 1, 1, 0, 1} };

	// transposed test matrix
	int TM_t[6][3] = { {1, 0, 0},{0, 1, 0},{0, 0, 1},{1, 0, 1},{1, 1, 0},{0, 1, 1} };

	// interaction with user
	try {
		cout << "Number of bits in one data frame delivered to encoder: ";
		cin >> nobWithoutZeropadding;
		if (cin.fail())
			throw "Type integer value";
		if (nobWithoutZeropadding < 0)
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
		if (step <= 0)
			throw "Step should be greater than 0";
	}
	catch (const char* msg) {
		cerr << "Error! " << msg << endl;
		system("pause");
		exit(0);
	}
	// generate input using normal distribution generator and dividing input into 3-bits blocks
	int *inputDataWithoutZeropadding = new int[nobWithoutZeropadding];
	random_device rd{};
	mt19937 gen{ rd() };
	normal_distribution<> d{ 0.5, 0.125 };
	//////******************MAIN VER******************/
	// do this only once and then test on one, same input
	//fileInputBits.open("InputBits.txt", ios::out);
	//for (int i = 0; i < 100000; i++) {
	//	int generatedBit = round(d(gen));
	//	if (i < nobWithoutZeropadding)
	//		inputDataWithoutZeropadding[i] = generatedBit;
	//	fileInputBits << generatedBit << endl;
	//}
	//fileInputBits.close();
	////*******************************************/
	///*************TEST VER*************/
	//reading input data from const file (not generating new input)
	string a;
	temporaryForTesting.open("InputBits.txt", ios::in);
	for(int i = 0; i < nobWithoutZeropadding; i++)
	{
		getline(temporaryForTesting, a);
		inputDataWithoutZeropadding[i] = atoi(a.c_str());
	}
	temporaryForTesting.close();
	/**********************************/

	// zeropadding
	nob = nobWithoutZeropadding;
	while (nob % frameLength != 0)
		nob++;
	int *inputData = new int[nob];
	for (int i = 0; i < nob; i++) {
		if (i < nobWithoutZeropadding)
			inputData[i] = inputDataWithoutZeropadding[i];
		else
			inputData[i] = 0;
	}
	delete[] inputDataWithoutZeropadding;

	// grouping into 3-bits blocks
	int var = 0;
	int **inputDataBlocks = new int*[nob / frameLength];
	for (int i = 0; i < nob / frameLength; i++) {
		inputDataBlocks[i] = new int[frameLength];
		for (int j = 0; j < frameLength; j++) {
			inputDataBlocks[i][j] = inputData[var];
			var++;
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
			//// saving encoded bits into a file
			fileEncodedBits << encodedBlocks[i][j];
		}
		fileEncodedBits << endl;
	}

	float **encodedBlocksAfterCanal = new float*[nob / frameLength];
	int **blocksAfterDecision = new int*[nob / frameLength];
	int **syndroms = new int*[nob / frameLength];
	int **bitsRestored = new int*[nob / frameLength];
	int **blocksAfterCorrection = new int*[nob / frameLength];
	int *outputData = new int[nob];
	float **withoutCodingBlocksAfterCanal = new float*[nob / frameLength];

	// main loop
	// transmission in canal with increasing Es_N0 and then saving blocks after canal into a file
	while (Es_N0_min <= Es_N0_max) {
		fileAfterCanalBits << "Es_N0: " << Es_N0_min << endl;
		for (int i = 0; i < nob / frameLength; i++)
		{
			encodedBlocksAfterCanal[i] = new float[encodedBlockLength]();
			withoutCodingBlocksAfterCanal[i] = new float[frameLength]();
			for (int j = 0; j < encodedBlockLength; j++)
			{
				kanal(Es_N0_min, encodedBlockLength, encodedBlocks[i], encodedBlocksAfterCanal[i]);
				fileAfterCanalBits << encodedBlocksAfterCanal[i][j] << ' ';
				if(j < frameLength)
					kanal(Es_N0_min, frameLength, inputDataBlocks[i], withoutCodingBlocksAfterCanal[i]);
			}
			fileAfterCanalBits << endl;
		}

		// decision-making algorythm in a receiver on a data from canal for both coding case and without coding
		for (int i = 0; i < nob / frameLength; i++)
		{
			blocksAfterDecision[i] = new int[encodedBlockLength]();
			for (int j = 0; j < encodedBlockLength; j++)
			{
				if (encodedBlocksAfterCanal[i][j] > 0)
					blocksAfterDecision[i][j] = 1;
				else
					blocksAfterDecision[i][j] = 0;
				if (j < frameLength)
				{
					if (withoutCodingBlocksAfterCanal[i][j] > 0)
						withoutCodingBlocksAfterCanal[i][j] = 1;
					else
						withoutCodingBlocksAfterCanal[i][j] = 0;
					fileWithoutCodingAfterCanal << withoutCodingBlocksAfterCanal[i][j] << endl;
				}
			}
		}

		// check BER for transmitted without codding
		int errorsWithoutCoding = 0;
		for (int i = 0; i < nob / frameLength; i++)
		{
			for (int j = 0; j < frameLength; j++)
			{
				if (withoutCodingBlocksAfterCanal[i][j] != inputDataBlocks[i][j])
					errorsWithoutCoding++;
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
		float BERwithoutCoding = (float)errorsWithoutCoding / (float)nob;
		fileResults << "Es/N0=" << Es_N0_min << " BER=" << BER << "  BER without coding=" << BERwithoutCoding << endl;
		cout << "Es/N0=" << Es_N0_min << " -> BER=" << BER << "  BER without coding=" << BERwithoutCoding << endl;
		
		// when somebody puts Es_N0_min-Es_N0_max range with too big step for this range, the border value
		// of Es_N0_max will be also contained in the output - e.g. Esmin = 0, Esmax = 10, step = 3 -> instead of simulating with 0,3,6,9,
		// there is also a limit (max) value simulation - 0,3,6,9,10
		if (((Es_N0_max - Es_N0_min) < step) && (Es_N0_max - Es_N0_min) !=0)
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
	fileEncodedBits.close();
	fileAfterCanalBits.close();
	fileDecodedBits.close();
	fileResults.close();
	fileWithoutCodingAfterCanal.close();
	system("pause");
}

