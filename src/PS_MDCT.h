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
template<typename T>
class CMDCTComputer
{
public:
	typedef SOAFRAMECHUNKS<T, FRAME_DATA_COUNT,1> CHUNKS1;

private:
	CHUNKS1 *m_lpChunksOdd;
	CHUNKS1 *m_lpChunksEven;
	CHUNKS1 *m_lpSpectrum;
	U32 m_ctChunks;

public:
	CMDCTComputer(CHUNKS1* lpChunksOdd, CHUNKS1* lpChunksEven, CHUNKS1* lpSpectrum, U32 ctChunks)
	{
		m_lpChunksOdd  = lpChunksOdd;
		m_lpChunksEven = lpChunksEven;
		m_lpSpectrum   = lpSpectrum;
		m_ctChunks 	   = ctChunks;
	}

	void operator()(const blocked_range<size_t>& range) const
	{
		for(size_t i=range.begin(); i != range.end(); i++)
			process(m_lpChunksOdd[i], m_lpChunksEven[i], m_lpSpectrum[i], i != m_ctChunks - 1);
	}

	void process(const CHUNKS1& inputOdd, const CHUNKS1& inputEven, CHUNKS1& output, bool bComputeEven) const;
	void commonKernel(const T* lpInput, T* lpOutput) const;
};

template<typename T>
void CMDCTComputer<T>::process(const CHUNKS1& inputOdd, const CHUNKS1& inputEven, CHUNKS1& output, bool bComputeEven) const
{
	U32 hn = FRAME_DATA_COUNT / 2;
	T d;

	T piOver2N = Pi / (T)(2 * FRAME_DATA_COUNT);
	T *pd = &output.data[0];

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

/*
	commonKernel(inputOdd.data, &output.data[0]);
	if(bComputeEven)
		commonKernel(inputEven.data, &output.data[FRAME_DATA_COUNT / 2]);
		*/

}

template<typename T>
void CMDCTComputer<T>::commonKernel(const T* lpInput, T* lpOutput) const
{
	U32 hn = FRAME_DATA_COUNT / 2;
	U32 mMax = PS_SIMD_BLOCKS(FRAME_DATA_COUNT);
	T* pd = lpOutput;

	Float_ dd(0.0f);
	Float_ PIOVER2N(Pi / (T)(2 * FRAME_DATA_COUNT));

	Float_ ii(PS_SIMD_FLEN);
	Float_ two(2.0f);
	Float_ Ndiv2PlusOne(hn + 1.0f);

	float PS_SIMD_ALIGN(arrM[PS_SIMD_FLEN]);
	for(int iSimd=0; iSimd<PS_SIMD_FLEN; iSimd++)
		arrM[iSimd] = iSimd;
	Float_ mm(arrM);

	//ODD
	for(U32 k=0; k<hn; k++)
	{
		dd.setZero();

		Float_ ss = PIOVER2N * Float_(2.0f * k + 1);
		for(U32 m=0; m<mMax; m++)
		{
			Float_ xm(&lpInput[m * PS_SIMD_FLEN]);
			Float_ angle = ss * (two * mm + Ndiv2PlusOne);

#ifdef SIMD_USE_M128
			dd = dd + xm * SIMDCosine(angle.v);
			mm = mm + ii;
#else

#endif
		}

		float d = 0.0f;
		for(int i=0; i<PS_SIMD_FLEN; i++)
			d += dd[i];
		pd[k] = d;
	}
}

template<typename T>
bool ApplyMDCT(const std::vector<T>& arrSignalData, std::vector<T>& arrOutput)
{
	U32 szData = arrSignalData.size();
	U32 ctFrames = szData / FRAME_DATA_COUNT;
	U32 ctChunks = ctFrames;

	//typedef CMDCTComputer<T>::CHUNKS1 CHUNKS1;
	typedef SOAFRAMECHUNKS<T, FRAME_DATA_COUNT, 1> CHUNKS1;
	CHUNKS1 *lpChunksOdd  = reinterpret_cast<CHUNKS1*>(AllocAligned(sizeof(CHUNKS1) * ctChunks));
	CHUNKS1 *lpChunksEven = reinterpret_cast<CHUNKS1*>(AllocAligned(sizeof(CHUNKS1) * ctChunks));
	CHUNKS1 *lpSpectrum   = reinterpret_cast<CHUNKS1*>(AllocAligned(sizeof(CHUNKS1) * ctChunks));

	//Odd
	const T* lpSourceBegin = &arrSignalData[0];
	U32 szCopy;
	for(U32 i=0; i<ctChunks; i++)
	{
		if(szData > FRAME_DATA_COUNT)
			szCopy = FRAME_DATA_COUNT;
		else
			szCopy = szData;
		memcpy(lpChunksOdd[i].data, &lpSourceBegin[i*FRAME_DATA_COUNT], szCopy * sizeof(T));
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
		memcpy(lpChunksEven[i].data, &lpSourceBegin[i*FRAME_DATA_COUNT], szCopy * sizeof(T));
		szData -= szCopy;
	}

	CMDCTComputer<T> body(lpChunksOdd, lpChunksEven, lpSpectrum, ctChunks);
	tbb::parallel_for(blocked_range<size_t>(0, ctChunks), body, tbb::auto_partitioner());

	FreeAligned(lpChunksOdd);
	FreeAligned(lpChunksEven);
	/////////////////////////////////////////////////////////////
	arrOutput.resize(0);
	std::vector<T> arrTemp;
	arrTemp.resize(FRAME_DATA_COUNT);
	for(U32 i=0; i<ctChunks; i++)
	{
		memcpy(&arrTemp[0], lpSpectrum[i].data, FRAME_DATA_COUNT * sizeof(T));
		arrOutput.insert(arrOutput.begin(), arrTemp.begin(), arrTemp.end());
	}

	FreeAligned(lpSpectrum);

	return true;
}


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
