/*
 * PS_MDCT.cpp
 *
 *  Created on: 2011-11-16
 *      Author: pourya
 */

#include "PS_MDCT.h"
#include "simd_trigonometry.h"

#include "MarSystemManager.h"

using namespace Marsyas;

/*!
 * Computes MDCT for one frame
 */
void CommonKernel(const void* lpInput, void* lpOutput)
{
	U32 hn = FRAME_DATA_COUNT / 2;
	U32 mMax = PS_SIMD_BLOCKS(FRAME_DATA_COUNT);
	float* pOut = reinterpret_cast<float*>(lpOutput);
	const float* pIn = reinterpret_cast<const float*>(lpInput);


	Float_ PIOVER2N(Pi / (float)(2 * FRAME_DATA_COUNT));

	Float_ ii(PS_SIMD_FLEN);
	Float_ two(2.0f);
	Float_ Ndiv2PlusOne(hn + 1.0f);

	float PS_SIMD_ALIGN(arrDataCos[PS_SIMD_FLEN]);
	float PS_SIMD_ALIGN(arrM[PS_SIMD_FLEN]);
	for(int iSimd=0; iSimd<PS_SIMD_FLEN; iSimd++)
		arrM[iSimd] = iSimd;

	//Common Compute
	for(U32 k=0; k<hn; k++)
	{
		Float_ dd(0.0f);
		Float_ mm(arrM);

		Float_ ss = PIOVER2N * Float_(2.0f * k + 1);
		for(U32 m=0; m<mMax; m++)
		{
			Float_ xm(&pIn[m * PS_SIMD_FLEN]);
			Float_ angle = ss * (two * mm + Ndiv2PlusOne);

#ifdef SIMD_USE_M128
			//for(int iSimd=0; iSimd<PS_SIMD_FLEN; iSimd++)
				//dd[iSimd] = dd[iSimd] + xm[iSimd] * cos(angle[iSimd]);
			dd = dd + xm * cos_ps(angle.v);
			mm = mm + ii;
#elif defined(SIMD_USE_M256)
			angle.store(&arrDataCos[0]);

			__m128 xLo = _mm_load_ps(&arrDataCos[0]);
			__m128 xHi = _mm_load_ps(&arrDataCos[4]);
			xLo = cos_ps(xLo);
			xHi = cos_ps(xHi);
			_mm_store_ps(&arrDataCos[0], xLo);
			_mm_store_ps(&arrDataCos[4], xHi);

			angle = Float_(arrDataCos);
#endif
		}

		dd = SimdhAdd(dd, dd);
		dd = SimdhAdd(dd, dd);

		//float d = 0.0f;
		//for(int i=0; i<PS_SIMD_FLEN; i++)
			//d += dd[i];
		pOut[k] = dd[0];
	}
}

void CosineTest()
{
	float PS_SIMD_ALIGN(arrData[2048]);
	float PS_SIMD_ALIGN(arrOutput1[2048]);
	float PS_SIMD_ALIGN(arrOutput2[2048]);
	float PS_SIMD_ALIGN(arrDataCos[16]);
	for(U32 i=0; i<2048; i++)
	{
		arrData[i] = RandRangeT<float>(0.0f, 10.0f);
		arrOutput1[i] = cos(arrData[i]);
	}

	U32 szBlocks = PS_SIMD_BLOCKS(2048);
	for(U32 i=0; i<szBlocks; i++)
	{
		Float_ x = Float_(&arrData[i * PS_SIMD_FLEN]);
		//x = SIMDCosine(x.v);
#ifdef SIMD_USE_M128
		x = cos_ps(x.v);
#elif defined(SIMD_USE_M256)
		x.store(&arrDataCos[0]);

		__m128 xLo = _mm_load_ps(&arrDataCos[0]);
		__m128 xHi = _mm_load_ps(&arrDataCos[4]);
		xLo = cos_ps(xLo);
		xHi = cos_ps(xHi);
		_mm_store_ps(&arrDataCos[0], xLo);
		_mm_store_ps(&arrDataCos[4], xHi);
		x = Float_(arrDataCos);
#endif
		x.store(&arrOutput2[i * PS_SIMD_FLEN]);
	}

	{
		std::vector<float> arrOutput;
		arrOutput.insert(arrOutput.end(), &arrOutput1[0], &arrOutput1[2048]);
		//SaveCSV("OutputCos.csv", arrOutput);

		arrOutput.resize(0);
		arrOutput.insert(arrOutput.end(), &arrOutput2[0], &arrOutput2[2048]);
		//SaveCSV("OutputSimdCos.csv", arrOutput);
	}
}


U32 CountSamples(string strSoundFP)
{
	MarSystemManager mng;

	// A series to contain everything
	MarSystem* net = mng.create("Series", "net");

	// The sound file
	net->addMarSystem(mng.create("SoundFileSource", "src"));

	//Set input filename
	net->updControl("SoundFileSource/src/mrs_string/filename", strSoundFP);

	//Set offset from the beginning of the song
	net->updControl("SoundFileSource/src/mrs_natural/pos", 0);

	//Window size
	net->setctrl("mrs_natural/inSamples", 1);

	mrs_natural channels = net->getctrl("mrs_natural/onObservations")->to<mrs_natural>();


	//Get Data Chunks and perform MDCT
	realvec processedData;
	U32 ctSamples = 0;
	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();
		ctSamples += processedData.getSize();
	}


	delete net;
	return ctSamples;
}


U32 ReadSoundFile(string strSoundFP, std::vector<float>& outputData, U32 windowSize)
{
	MarSystemManager mng;

	// A series to contain everything
	MarSystem* net = mng.create("Series", "net");

	// The sound file
	net->addMarSystem(mng.create("SoundFileSource", "src"));

	//Set input filename
	net->updControl("SoundFileSource/src/mrs_string/filename", strSoundFP);

	//Set offset from the beginning of the song
	net->updControl("SoundFileSource/src/mrs_natural/pos", 0);

	//Window size
	net->setctrl("mrs_natural/inSamples", windowSize);

	mrs_natural channels = net->getctrl("mrs_natural/onObservations")->to<mrs_natural>();



	//Get Data Chunks and perform MDCT
	U32 ctTicks = 0;
	realvec processedData;

	//Temp Array
	//std::vector<T> arrTemp;
	//arrTemp.resize(512);
	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		ctTicks++;
		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

		U32 szData = processedData.getSize();
		//U32 ctCols = processedData.getCols();
		//U32 ctRows = processedData.getRows();
		//memcpy(&arrTemp[0], processedData.getData(), szData * sizeof(T));
		for(U32 i=0; i<szData; i++)
			outputData.push_back(static_cast<float>(processedData(0, i)));

		//Copy to output
		//outputData.insert(outputData.end(), arrTemp.begin(), arrTemp.end());
	}

	delete net;
	return ctTicks;
}















