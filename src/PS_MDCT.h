/*
 * PS_MDCT.h
 *
 *  Created on: 2011-11-16
 *      Author: pourya
 */

#ifndef PS_MDCT_H_
#define PS_MDCT_H_

#include "PS_SIMDVecN.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include <vector>

using namespace tbb;
using namespace std;

#define FRAME_DATA_COUNT 512
#define FRAMES_IN_CHUNK 8

//Compute Modified Discrete Cosine Transform
template<typename T, U32 ctDataLength, U32 ctFramesInChunk>
struct PS_BEGIN_ALIGNED(PS_SIMD_FLEN) SOAFRAMECHUNKS
{
	T data[PS_SIMD_PADSIZE(ctDataLength*ctFramesInChunk)];
};

typedef SOAFRAMECHUNKS<double, FRAME_DATA_COUNT, 1> CHUNKS1D;
typedef SOAFRAMECHUNKS<double, FRAME_DATA_COUNT, 8> CHUNKS8D;
typedef SOAFRAMECHUNKS<float, FRAME_DATA_COUNT, 1> CHUNKS1F;
typedef SOAFRAMECHUNKS<float, FRAME_DATA_COUNT, 8> CHUNKS8F;


//Computes Modified Discrete Cosine Transform using multi-core and
//SIMD technique
class CMDCTComputer
{
private:
	CHUNKS1D *m_lpChunksOdd;
	CHUNKS1D *m_lpChunksEven;
	CHUNKS1D *m_lpSpectrum;
	U32 m_ctChunks;

public:
	CMDCTComputer(CHUNKS1D *lpChunksOdd, CHUNKS1D *lpChunksEven, CHUNKS1D* lpSpectrum, U32 ctChunks);
	void operator()(const blocked_range<size_t>& range) const;
	void process(const CHUNKS1D& inputOdd, const CHUNKS1D& inputEven, CHUNKS1D& output, bool bComputeEven) const;
};


bool ApplyMDCT(const std::vector<double>& arrSignalData, std::vector<double>& arrOutput);

template<typename T>
T GetMaxData(const std::vector<T> &arrData)
{
	assert(arrData.size() > 0);
	T mx = arrData[0];
	for(U32 i=1; i<arrData.size(); i++)
	{
		if(arrData[i] > mx)
			mx = arrData[i];
	}
	return mx;
}

template<typename T>
T GetMinData(const std::vector<T> &arrData)
{
	assert(arrData.size() > 0);
	T mx = arrData[0];
	for(U32 i=1; i<arrData.size(); i++)
	{
		if(arrData[i] < mx)
			mx = arrData[i];
	}
	return mx;
}

template<typename T>
void NormalizeData(std::vector<T>& arrData)
{
	T mx = GetMaxData(arrData);

	T invs = (mx > 0.0)?1.0/mx:1.0;
	for(U32 i=0; i<arrData.size();i++)
		arrData[i] *= invs;
}





















#endif /* PS_MDCT_H_ */
