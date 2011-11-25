/*
 * PS_Classifier.cpp
 *
 *  Created on: 2011-11-22
 *      Author: pourya
 */

#include "PS_Classifier.h"


int Classifier::computeSpectralCentroid(string strSoundFP, std::vector<double>& outputData, int windowSize)
{
	MarSystemManager mng;

	// A series to contain everything
	MarSystem* net = mng.create("Series", "ExtractSpectralCentroid");

	// The sound file
	net->addMarSystem(mng.create("SoundFileSource", "src"));
	net->addMarSystem(mng.create("Windowing", "ham"));
	net->addMarSystem(mng.create("Spectrum", "spk"));
	net->addMarSystem(mng.create("PowerSpectrum", "pspk"));
	MarSystem* centroid = mng.create("Centroid", "centroid");
	net->addMarSystem(centroid);

	//Set input filename
	net->updControl("SoundFileSource/src/mrs_string/filename", strSoundFP);

	//Set offset from the beginning of the song
	net->updControl("SoundFileSource/src/mrs_natural/pos", 0);

	//Window size
	net->setctrl("mrs_natural/inSamples", windowSize);

	mrs_natural channels = net->getctrl("mrs_natural/onObservations")->to<mrs_natural>();

	cout << "channels=" << channels << endl;

	realvec processedData;

	//Get Data Chunks and perform MDCT
	U32 ctTicks = 0;

	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		ctTicks++;
		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

		U32 szData = processedData.getSize();
		outputData.push_back(processedData(0, 0));
	}
	delete net;

	return ctTicks;

}

int Classifier::computeZeroCrossings(string strSoundFP,
									 std::vector<double>& zeros)
{
	MarSystemManager mng;

	// A series to contain everything
	MarSystem* net = mng.create("Series", "ExtractSpectralCentroid");
	net->addMarSystem(mng.create("SoundFileSource", "src"));
	net->addMarSystem(mng.create("ZeroCrossings", "zcrs"));

	//Set input filename
	net->updControl("SoundFileSource/src/mrs_string/filename", strSoundFP);

	//Set offset from the beginning of the song
	net->updControl("SoundFileSource/src/mrs_natural/pos", 0);

	//Window size
	//net->setctrl("mrs_natural/inSamples", 1);

	mrs_natural channels = net->getctrl("mrs_natural/onObservations")->to<mrs_natural>();

	cout << "channels=" << channels << endl;

	realvec processedData;

	//Get Data Chunks and perform MDCT
	U32 ctTicks = 0;

	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		ctTicks++;
		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

		int ctCols = processedData.getCols();
		int ctRows = processedData.getRows();
		int szData = processedData.getSize();
		zeros.push_back(processedData(0, 0));
	}

	delete net;

	return ctTicks;


}

int Classifier::computeSpectralCentroidWithZeroCrossings(string strSoundFP,
														 std::vector<double>& centroids,
														 std::vector<double>& zeros,
														 int windowSize)
{
	MarSystemManager mng;

	// A series to contain everything
	MarSystem* net = mng.create("Series", "ExtractSpectralCentroid");
	net->addMarSystem(mng.create("SoundFileSource", "src"));

	//Fanout 1: Zero Crossing 2: Centroid
	MarSystem* fanout = mng.create("Fanout", "fanout");
	fanout->addMarSystem(mng.create("ZeroCrossings", "zcrs"));

	// The Spectral Net
	MarSystem* spectralNet = mng.create("Series", "spectralNet");
	spectralNet->addMarSystem(mng.create("Windowing", "ham"));
	spectralNet->addMarSystem(mng.create("Spectrum", "spk"));
	spectralNet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
	spectralNet->addMarSystem(mng.create("Centroid", "centroid"));

	//
	fanout->addMarSystem(spectralNet);
	net->addMarSystem(fanout);


	//Set input filename
	net->updControl("SoundFileSource/src/mrs_string/filename", strSoundFP);

	//Set offset from the beginning of the song
	net->updControl("SoundFileSource/src/mrs_natural/pos", 0);

	//Window size
	net->setctrl("mrs_natural/inSamples", windowSize);

	mrs_natural channels = net->getctrl("mrs_natural/onObservations")->to<mrs_natural>();

	cout << "channels=" << channels << endl;

	realvec processedData;

	//Get Data Chunks and perform MDCT
	U32 ctTicks = 0;

	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		ctTicks++;
		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

		int ctCols = processedData.getCols();
		int ctRows = processedData.getRows();
		int szData = processedData.getSize();
		zeros.push_back(processedData(0, 0));
		centroids.push_back(processedData(1, 0));
	}
	delete net;

	return ctTicks;

}

int Classifier::computeSpectralCentroidWithZeroCrossingsSmoothed(string strSoundFP,
																 std::vector<double>& centroids,
																 std::vector<double>& zeros,
																 int windowSize, int k)
{
	MarSystemManager mng;

	// A series to contain everything
	MarSystem* net = mng.create("Series", "ExtractSpectralCentroid");
	net->addMarSystem(mng.create("SoundFileSource", "src"));

	//Fanout 1: Zero Crossing 2: Centroid
	MarSystem* fanout = mng.create("Fanout", "fanout");
	fanout->addMarSystem(mng.create("ZeroCrossings", "zcrs"));

	//The Spectral Net
	MarSystem* spectralNet = mng.create("Series", "spectralNet");
	spectralNet->addMarSystem(mng.create("Windowing", "ham"));
	spectralNet->addMarSystem(mng.create("Spectrum", "spk"));
	spectralNet->addMarSystem(mng.create("PowerSpectrum", "pspk"));
	spectralNet->addMarSystem(mng.create("Centroid", "centroid"));

	//Final net
	fanout->addMarSystem(spectralNet);
	net->addMarSystem(fanout);

	//Smoothing
	net->addMarSystem(mng.create("Memory", "mem"));
	net->addMarSystem(mng.create("Mean", "mean"));
	net->updControl("Memory/mem/mrs_natural/memSize", k);


	//Set input filename
	net->updControl("SoundFileSource/src/mrs_string/filename", strSoundFP);

	//Set offset from the beginning of the song
	net->updControl("SoundFileSource/src/mrs_natural/pos", 0);

	//Window size
	net->setctrl("mrs_natural/inSamples", windowSize);

	mrs_natural channels = net->getctrl("mrs_natural/onObservations")->to<mrs_natural>();

	cout << "channels=" << channels << endl;

	realvec processedData;

	//Get Data Chunks and perform MDCT
	U32 ctTicks = 0;

	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		ctTicks++;
		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

		int ctCols = processedData.getCols();
		int ctRows = processedData.getRows();
		int szData = processedData.getSize();
		zeros.push_back(processedData(0, 0));
		centroids.push_back(processedData(1, 0));
	}
	delete net;

	return ctTicks;

}

int Classifier::sonifySpectralCentroid(std::vector<double>& centroids)
{
	//we usualy start by creating a MarSystem manager
	//to help us on MarSystem creation
	MarSystemManager mng;

	//create the network, which is a simple Series network with a sine wave
	//oscilator and a audio sink object to send the ausio data for playing
	//in the sound card
	MarSystem *network = mng.create("Series", "network");
	network->addMarSystem(mng.create("SineSource", "src"));
	network->addMarSystem(mng.create("AudioSink", "dest"));

	//set the window (i.e. audio frame) size (in samples). Let's say, 256 samples.
	//This is done in the outmost MarSystem (i.e. the Series/network) because flow
	//controls (as is the case of inSamples) are propagated through the network.
	//Check the Marsyas documentation for mode details.
	network->updControl("mrs_natural/inSamples", 256);

	//set oscilator frequency to 440Hz
	//network->updControl("SineSource/src/mrs_real/frequency", 440.0);

	//configure audio sink module
	//512 is just some reasonable buffer size
	//you can try different ones and see what you get...
	network->updControl("AudioSink/dest/mrs_natural/bufferSize", 512);
	// set the sampling to 44100  - a safe choice in most configurations
	network->updControl("mrs_real/israte", 44100.0);
	network->updControl("AudioSink/dest/mrs_bool/initAudio", true);

	//now it's time for ticking the network,
	//ad aeternum (i.e. until the user quits by CTRL+C)
	U32 ticks = 0;
	while (ticks < centroids.size())
	{
		network->updControl("SineSource/src/mrs_real/frequency", centroids[ticks]);
		network->tick();
		ticks++;
	}

	//ok, this is not really necessary because we are quiting by CTRL+C,
	//but it's a good habit anyway ;-)
	delete network;

	return(0);
}

int Classifier::sonifySpectralCentroid(string strSoundFP, std::vector<double>& centroids)
{
	MarSystemManager mng;

	MarSystem* audioSeries = mng.create("Series", "audioseries");
	audioSeries->addMarSystem(mng.create("SoundFileSource", "src"));
	audioSeries->addMarSystem(mng.create("Gain", "gain"));


	//Parallel for File and Sine Source
	MarSystem* parallel = mng.create("Parallel", "parallel");
	parallel->addMarSystem(audioSeries);
	parallel->addMarSystem(mng.create("SineSource", "sinesrc"));


	//Final network
	MarSystem* net = mng.create("Series", "sonificationseries");
	net->addMarSystem(parallel);
	net->addMarSystem(mng.create("AudioSink", "audiosink"));

	//Set params
	audioSeries->updctrl("SoundFileSource/src/mrs_string/filename", strSoundFP);
	audioSeries->updctrl("Gain/gain/mrs_real/gain", 1.0);

	int ctChannels = audioSeries->getctrl("SoundFileSource/src/mrs_natural/nChannels")->to<mrs_natural>();
	audioSeries->updctrl("SoundFileSink/dest/mrs_natural/nChannels", ctChannels);

	net->updctrl("mrs_natural/inSamples", 512);

	net->updControl("AudioSink/audiosink/mrs_natural/bufferSize", 512);
	// set the sampling to 44100  - a safe choice in most configurations
	net->updControl("mrs_real/israte", 44100.0);
	net->updControl("AudioSink/audiosink/mrs_bool/initAudio", true);


	cout << (*audioSeries) << endl;

	U32 ctTicks = 0;
	while (net->getctrl("Parallel/parallel/Series/audioseries/SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		if(ctTicks < centroids.size())
			net->updControl("Parallel/parallel/SineSource/sinesrc/mrs_real/frequency", centroids[ctTicks]);
		net->tick();

		ctTicks++;
	}

	delete audioSeries;
	return 1;
}


void Classifier::classifyEmperically(std::vector<double>& zeros,
									 std::vector<double>& centroids,
									 std::vector<char>& groundTruth)
{
	//My rule
	U32 ctFrames = centroids.size();
	U32 ctErrorDisco = 0;
	U32 ctErrorClassic = 0;
	U32 ctGroupTotal = ctFrames / 2;

	for(U32 i=0; i<ctFrames; i++)
	{
		//Disco it is
		if(centroids[i] > 0.1 && zeros[i] > 0.11)
		{
			//I guess it is Disco
			if(groundTruth[i] != 'd')
				ctErrorDisco++;
		}
		else
		{
			//I guess it is Classic
			if(groundTruth[i] != 'c')
				ctErrorClassic++;
		}
	}

	//
	float fErrorRate = (float)(ctErrorClassic + ctErrorDisco) / (float)ctFrames;
	float fErrorRateDisco = (float)ctErrorDisco/(float)(ctGroupTotal);
	float fErrorRateClassic = (float)ctErrorClassic/(float)(ctGroupTotal);

	printf("Total Error: %d, Rate:%.2f\n", ctErrorClassic + ctErrorDisco, fErrorRate);
	printf("Disco Correct: %d, Error:%d, ErrorRate:%.2f \n", ctGroupTotal - ctErrorDisco, ctErrorDisco, fErrorRateDisco);
	printf("Classic Correct: %d, Error:%d, ErrorRate:%.2f \n", ctGroupTotal - ctErrorClassic, ctErrorClassic, fErrorRateClassic);
}
