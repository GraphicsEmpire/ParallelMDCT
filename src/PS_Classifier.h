/*
 * PS_Classifier.h
 *
 *  Created on: 2011-11-22
 *      Author: pourya
 */

#ifndef PS_CLASSIFIER_H_
#define PS_CLASSIFIER_H_

#include <string.h>
#include "MarSystemManager.h"
#include "PS_MathBase.h"

using namespace std;
using namespace Marsyas;

class Classifier{
public:

	//Q1
	static int computeSpectralCentroid(string strSoundFP, std::vector<double>& outputData, int windowSize);

	//Q2
	static int sonifySpectralCentroid(std::vector<double>& centroids);
	static int sonifySpectralCentroid(string strSoundFP, std::vector<double>& centroids);

	//Q3
	static int computeSpectralCentroidWithZeroCrossings(string strSoundFP,
														std::vector<double>& centroids,
														std::vector<double>& zeros, int windowSize);

	//Q5
	static int computeSpectralCentroidWithZeroCrossingsSmoothed(string strSoundFP,
																std::vector<double>& centroids,
																std::vector<double>& zeros, int windowSize, int k);

	//Q3
	static int computeZeroCrossings(string strSoundFP,
									std::vector<double>& zeros);

	//Q4
	void classifyEmperically(std::vector<double>& zeros,
							 std::vector<double>& centroids,
							 std::vector<char>& groundTruth);



};

#endif /* PS_CLASSIFIER_H_ */
