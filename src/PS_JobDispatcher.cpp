/*
 * PS_JobDispatcher.cpp
 *
 *  Created on: 2011-11-29
 *      Author: pourya
 */
#include "PS_JobDispatcher.h"
#include "PS_MDCT.h"
#include "tbb/tick_count.h"

CounterProcessTimeTotalFrames g_ctProcessTimeFrames(std::make_pair(0.0, 0));


void ProcessBatched::operator()(const blocked_range<size_t>& range) const
{
	U32 ctFrames = 0;
	CounterProcessTimeTotalFrames::reference m_localCounter = g_ctProcessTimeFrames.local();
	for(size_t i=range.begin(); i != range.end(); i++)
	{
		m_localCounter.first += process(m_vFiles[i], ctFrames);
		m_localCounter.second += ctFrames;
	}
}

double ProcessBatched::process(const string& strFP, U32 &ctOutFrames) const
{
	//Arrays to hold values
	std::vector<float> arrInputSignal;
	std::vector<float> arrOutputSpectrum;

	tbb::tick_count t0 = tbb::tick_count::now();

	//1. Read Sound Files
	//U32 ctSamples = CountSamples(vFiles[iFile]);
	arrInputSignal.reserve(1024 * FRAME_DATA_COUNT);
	ReadSoundFile(strFP, arrInputSignal, 1);


	//2. Apply MDCT
	ctOutFrames = ApplyMDCT<float>(arrInputSignal, arrOutputSpectrum, m_bUseSimd, m_bCopyParallel);

	arrInputSignal.resize(0);
	arrOutputSpectrum.resize(0);
	/*
	if(bSimd)
		SaveCSV<float>("SpectrumSimd.csv", arrOutputSpectrum);
	else
		SaveCSV<float>("SpectrumNormal.csv", arrOutputSpectrum);
		*/

	tbb::tick_count t1 = tbb::tick_count::now();
	return (t1 - t0).seconds() * 1000.0;
}


U32 PrintResults()
{
	U32 ctFrames = 0;
	int idxThread = 0;
	//Print Results
	for(CounterProcessTimeTotalFrames::const_iterator i = g_ctProcessTimeFrames.begin(); i != g_ctProcessTimeFrames.end(); i++)
	{
		idxThread++;
		printf("Thread#  %d, Processed Time %.2f, Processed Frames %u \n", idxThread, i->first, i->second);

		ctFrames += i->second;
	}
	g_ctProcessTimeFrames.clear();

	return ctFrames;
}
