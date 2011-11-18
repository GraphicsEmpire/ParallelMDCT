/*
 * PS_MDCT.cpp
 *
 *  Created on: 2011-11-16
 *      Author: pourya
 */

#include "PS_MDCT.h"

CMDCTComputer::CMDCTComputer(CHUNKS1D *lpChunksOdd, CHUNKS1D *lpChunksEven,
							 CHUNKS1D *lpSpectrum, U32 ctChunks)
{
	m_lpChunksOdd  = lpChunksOdd;
	m_lpChunksEven = lpChunksEven;
	m_lpSpectrum   = lpSpectrum;
	m_ctChunks 	   = ctChunks;
}

void CMDCTComputer::operator()(const blocked_range<size_t>& range) const
{
	for(size_t i=range.begin(); i != range.end(); i++)
	{
		process(m_lpChunksOdd[i], m_lpChunksEven[i], m_lpSpectrum[i], i != m_ctChunks - 1);
	}
}

void CMDCTComputer::process(const CHUNKS1D& inputOdd, const CHUNKS1D& inputEven, CHUNKS1D& output, bool bComputeEven) const
{
	U32 hn = FRAME_DATA_COUNT / 2;
	double d;

	double piOver2N = Pi / (double)(2 * FRAME_DATA_COUNT);
	double *pd = &output.data[0];
	//ODD
	for(U32 k=0; k<hn; k++)
	{
		d = 0.0;
		for(U32 m=0; m<FRAME_DATA_COUNT; m++)
		{
			d += inputOdd.data[m] * cos(piOver2N * (2.0 * k + 1) * (2.0 * m + 1 + hn));
		}
		pd[k] = d;
	}

	//EVEN
	if(bComputeEven)
	{
		pd = &output.data[hn];
		for(U32 k=0; k<hn; k++)
		{
			d = 0.0;
			for(U32 m=0; m<FRAME_DATA_COUNT; m++)
			{
				d += inputEven.data[m] * cos(piOver2N * (2.0 * k + 1) * (2.0 * m + 1 + hn));
			}
			pd[k] = d;
		}
	}
}


bool ApplyMDCT(const std::vector<double>& arrSignalData, std::vector<double>& arrOutput)
{
	U32 szData = arrSignalData.size();
	U32 ctFrames = szData / FRAME_DATA_COUNT;
	U32 ctChunks = ctFrames;
	CHUNKS1D *lpChunksOdd  = reinterpret_cast<CHUNKS1D*>(AllocAligned(sizeof(CHUNKS1D) * ctChunks));
	CHUNKS1D *lpChunksEven = reinterpret_cast<CHUNKS1D*>(AllocAligned(sizeof(CHUNKS1D) * ctChunks));
	CHUNKS1D *lpSpectrum   = reinterpret_cast<CHUNKS1D*>(AllocAligned(sizeof(CHUNKS1D) * ctChunks));

	//Odd
	const double* lpSourceBegin = &arrSignalData[0];
	U32 szCopy;
	for(U32 i=0; i<ctChunks; i++)
	{
		if(szData > FRAME_DATA_COUNT)
			szCopy = FRAME_DATA_COUNT;
		else
			szCopy = szData;
		memcpy(lpChunksOdd[i].data, &lpSourceBegin[i*FRAME_DATA_COUNT], szCopy * sizeof(double));
		szData -= szCopy;
	}

	//Even
	szData = arrSignalData.size() - FRAME_DATA_COUNT / 2;
	lpSourceBegin = &arrSignalData[FRAME_DATA_COUNT / 2];
	for(U32 i=0; i<ctChunks; i++)
	{
		if(szData > FRAME_DATA_COUNT)
			szCopy = FRAME_DATA_COUNT;
		else
			szCopy = szData;
		memcpy(lpChunksEven[i].data, &lpSourceBegin[i*FRAME_DATA_COUNT], szCopy * sizeof(double));
		szData -= szCopy;
	}

	CMDCTComputer body(lpChunksOdd, lpChunksEven, lpSpectrum, ctChunks);
	tbb::parallel_for(blocked_range<size_t>(0, ctChunks), body, tbb::auto_partitioner());

	FreeAligned(lpChunksOdd);
	FreeAligned(lpChunksEven);
	/////////////////////////////////////////////////////////////
	arrOutput.resize(0);
	std::vector<double> arrTemp;
	arrTemp.resize(FRAME_DATA_COUNT);
	for(U32 i=0; i<ctChunks; i++)
	{
		memcpy(&arrTemp[0], lpSpectrum[i].data, FRAME_DATA_COUNT * sizeof(double));

		arrOutput.insert(arrOutput.begin(), arrTemp.begin(), arrTemp.end());
	}

	FreeAligned(lpSpectrum);

	return true;
}

















