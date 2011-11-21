//============================================================================
// Name        : MIRFeatureExtraction.cpp
// Author      : Pourya Shirazian
// Version     :
// Copyright   : Your copyright notice
// Description : Hello World in C++, Ansi-style
//============================================================================

#include <iostream>
using namespace std;

#include "GL/glew.h"
#include "GL/freeglut.h"
#include <vector>

#include "AA_Models/PS_FileDirectory.h"
#include "AA_Models/PS_SketchConfig.h"
#include "MarSystemManager.h"

#include "PS_CPU_INFO.h"
#include "PS_MDCT.h"
#include "PS_DrawChart.h"


#ifdef WIN32
	#include <io.h>
	#include "Windows.h"
#elif defined(__linux__)
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <dirent.h>
	#include <unistd.h>
#endif

#include "tbb/task_scheduler_init.h"
#include "tbb/tick_count.h"

#define WINDOW_SIZE_WIDTH 640
#define WINDOW_SIZE_HEIGHT 480
#define PERS_FOV 60.0f
#define PERS_ZNEAR 0.1f
#define PERS_ZFAR 1000.0f

using namespace Marsyas;

//GLUT CallBacks
void Close();
void Display();
void Resize(int w, int h);
void Keyboard(int key, int x, int y);
void MousePress(int button, int state, int x, int y);
void MouseMove(int x, int y);
void InitGL();



struct AppSettings{
	int ctThreadCountStart;
	int ctThreadCountEnd;
	int ctThreads;
};


tbb::task_scheduler_init* g_lpTaskSchedular = NULL;
CDrawChart* g_lpDrawChart = NULL;
AppSettings g_appSettings;
//////////////////////////////////////////////////////////////////////////////
void Close()
{
	SAFE_DELETE(g_lpTaskSchedular);
	SAFE_DELETE(g_lpDrawChart);
}

void Display()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glLoadIdentity();

	if(g_lpDrawChart->countSeries() > 0)
	{
		//glPushMatrix()
		U32 ctSeries = g_lpDrawChart->countSeries();
		float dd = 1.0f / (float)ctSeries;
		for(int i=0; i<ctSeries; i++)
		{
			glPushMatrix();
			glTranslatef(0.0f, i * dd, i * dd);
				g_lpDrawChart->draw(i, PLOT_LINE_STRIP);
			glPopMatrix();
		}
	}

	glutSwapBuffers();
}

void Resize(int w, int h)
{
    int side = MATHMIN(w, h);
    glViewport((w - side) / 2, (h - side) / 2, side, side);

	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, 1.0f, 0.0f, 4.0f);
	//gluPerspective(PERS_FOV, (float)w / (float)h, PERS_ZNEAR, PERS_ZFAR);

	glMatrixMode(GL_MODELVIEW);
}

void Keyboard(int key, int x, int y)
{

}

void MousePress(int button, int state, int x, int y)
{

}

void MouseMove(int x, int y)
{
	//g_arcBallCam.mouseMove(x, y);
	glutPostRedisplay();
}

void sfPlay(string strSoundFP)
{
	MarSystemManager mng;

	MarSystem* series = mng.create("Series", "series");


	series->addMarSystem(mng.create("SoundFileSource", "src"));
	series->addMarSystem(mng.create("Gain", "gain"));
	series->addMarSystem(mng.create("SoundFileSink", "dest"));

	series->updctrl("SoundFileSource/src/mrs_string/filename", strSoundFP);


	series->updctrl("Gain/gain/mrs_real/gain", 1.0);
	series->updctrl("SoundFileSink/dest/mrs_natural/nChannels",
					series->getctrl("SoundFileSource/src/mrs_natural/nChannels"));
	//series->updctrl("SoundFileSink/dest/mrs_real/israte",
	//        series->getctrl("SoundFileSource/src/mrs_real/osrate"));

	series->updctrl("mrs_natural/inSamples", 2048);
	series->updctrl("SoundFileSink/dest/mrs_string/filename", "ajay.wav");


	cout << (*series) << endl;

	while (series->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		series->tick();
	}
}

template<typename T>
U32 ReadSoundFile(string strSoundFP, std::vector<T>& outputData, U32 windowSize = 1)
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

	cout << "channels=" << channels << endl;

	realvec processedData;

	//Get Data Chunks and perform MDCT
	U32 ctTicks = 0;

	outputData.reserve(512 * 1024);

	//Temp Array
	std::vector<T> arrTemp;
	arrTemp.resize(512);
	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		ctTicks++;
		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

		U32 szData = processedData.getSize();
		memcpy(&arrTemp[0], processedData.getData(), szData * sizeof(T));

		//Copy to output
		outputData.insert(outputData.end(), arrTemp.begin(), arrTemp.end());
	}

	arrTemp.resize(0);
	delete net;

	return ctTicks;
}

int ListFilesDir(std::vector<string>& vPaths, const char* chrPath, const char* chrExt, bool bDirOnly)
{
	DIR *dp;
	struct dirent *pEntry;
	struct stat st;

	if((dp  = opendir(chrPath)) == NULL)
	{
			return 0;
	}

	while ((pEntry = readdir(dp)) != NULL)
	{
		const string strFileName = pEntry->d_name;
		const string strFullFileName = string(chrPath) + string("/") + strFileName;

		if(strFileName[0] == '.')
			continue;

		if(stat(strFullFileName.c_str(), &st) == -1)
			continue;

		const bool isDir = (st.st_mode & S_IFDIR) != 0;

		if(bDirOnly)
		{
			if(isDir)
				vPaths.push_back(strFullFileName);
		}
		else
		{
			if(chrExt)
			{
				DAnsiStr strExt = PS::FILESTRINGUTILS::ExtractFileExt(DAnsiStr(strFullFileName.c_str()));
				if(strExt == DAnsiStr(chrExt))
					vPaths.push_back(strFullFileName);
			}
			else
				vPaths.push_back(strFullFileName);
		}
	}
	closedir(dp);

	return vPaths.size();
}

void InitGL()
{

}

//Main
int main(int argc, char* argv[]) {
	//Detect CPU Info
	ProcessorInfo info;
	{
		info.bSupportAVX 	  = SimdDetectFeature(AVXFlag);
		//info.ctCores 		  = tbb::task_scheduler_init::default_num_threads();
		info.ctCores = 1;
		info.simd_float_lines = PS_SIMD_FLEN;

		info.cache_line_size = GetCacheLineSize();

		for(int i=0; i<4; i++)
		{
			info.cache_sizes[i]  = GetCacheSize(i, false);
			info.cache_levels[i] = GetCacheLevel(i);
			info.cache_types[i]  = GetCacheType(i);
		}
	}

	printf("Parallel MDCT Feature Extraction for MIR - Pourya Shirazian [Use -h for usage.]\n");
	printf("**********************************************************************************\n");
	if(SimdDetectFeature(OSXSAVEFlag))
		printf("OS supports AVX\n");
	else
		printf("OS DOESNOT support AVX\n");

	if(info.bSupportAVX)
		printf("CPU supports AVX.\n");
	else
		printf("CPU DOESNOT support AVX\n");


	printf("Running Machine: CoresCount:%u, SIMD FLOATS:%u\n", info.ctCores, info.simd_float_lines);
	printf("CacheLine:%u [B]\n", info.cache_line_size);
	for(int i=0; i<4; i++)
	{
		if(info.cache_types[i] != ctNone)
		{
			printf("\tLevel:%d, Type:%s, Size:%u [KB]\n",
					info.cache_levels[i],
					GetCacheTypeString(info.cache_types[i]),
					info.cache_sizes[i] / 1024);
		}
	}
	printf("**********************************************************************************\n");
	g_lpDrawChart = new CDrawChart();

	//Init GL
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
	glutInitWindowSize(WINDOW_SIZE_WIDTH, WINDOW_SIZE_HEIGHT);
	glutCreateWindow("Parallel MDCT Feature Extraction for MIR");
	glutDisplayFunc(Display);
	glutReshapeFunc(Resize);
	glutMouseFunc(MousePress);
	glutMotionFunc(MouseMove);
	glutSpecialFunc(Keyboard);
	glutCloseFunc(Close);
	//////////////////////////////////////////////////////////////
	//Initialization
	//Enable features we want to use from OpenGL
	glShadeModel(GL_SMOOTH);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
	//glEnable(GL_NORMALIZE);
	glClearColor(0.45f, 0.45f, 0.45f, 1.0f);


	//Compiling shaders
	GLenum err = glewInit();
	if (err != GLEW_OK)
	{
		//Problem: glewInit failed, something is seriously wrong.
		fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
		exit(1);
	}

	///////////////////////////////////////////////////////////////////////////////////
	std::vector<string> vFiles;
	{
		//Create a list of all files
		using namespace PS;
		using namespace PS::FILESTRINGUTILS;

		DAnsiStr strInfPath = ChangeFileExt(GetExePath(), ".inf");
		CSketchConfig* lpConfig = new PS::CSketchConfig(strInfPath, CSketchConfig::fmRead);

		DAnsiStr strRoot = lpConfig->readString("GENERAL", "ROOTPATH");
		DAnsiStr strExt  = lpConfig->readString("GENERAL", "FILEEXT");
		//int ctGroups = lpConfig->readInt("GENERAL", "GROUPCOUNT");

		std::vector<string> vDirs;
		if(ListFilesDir(vDirs, strRoot.ptr(), NULL, true) > 0)
		{
			for(int i=0; i<(int)vDirs.size(); i++)
			{
				ListFilesDir(vFiles, vDirs[i].c_str(), strExt.ptr(), false);
			}
		}

		SAFE_DELETE(lpConfig);
	}


	printf("Start Feature Extraction on %u files.\n", vFiles.size());

	SAFE_DELETE(g_lpTaskSchedular);
	g_appSettings.ctThreads = tbb::task_scheduler_init::default_num_threads();
	g_lpTaskSchedular = new tbb::task_scheduler_init(g_appSettings.ctThreads);
	printf("Parallel MDCT - Initialized with %d threads. \n", g_appSettings.ctThreads);


	double ms;

	int ctFiles = 1;
	if(vFiles.size() > 5)
	{
		for(U32 i=0; i<ctFiles; i++)
		{
			std::vector<double> arrInputSignal;
			std::vector<double> arrOutputSpectrum;

			tbb::tick_count t0 = tbb::tick_count::now();

			//1. Read Sound Files
			ReadSoundFile<double>(vFiles[i], arrInputSignal, 1);

			//2. Apply MDCT
			ApplyMDCT<double>(arrInputSignal, arrOutputSpectrum);
			arrInputSignal.resize(0);

			tbb::tick_count t1 = tbb::tick_count::now();
			ms = (t1 - t0).seconds();
			printf("File %s processed in %.2f [ms] \n", vFiles[i].c_str(), ms * 1000.0);

			//3. Normalized Output
			NormalizeData<double>(arrOutputSpectrum);
			//arrOutputSpectrum.resize(100);

			vec3f c;
			c.x = RandRangeT<float>(0.0f, 1.0f);
			c.y = RandRangeT<float>(0.0f, 1.0f);
			c.z = RandRangeT<float>(0.0f, 1.0f);
			g_lpDrawChart->addSeries(arrOutputSpectrum, "Spectrum", 1.0f, vec2f(0.0, 1.5f), c);
		}
	}


	glutMainLoop();



	return 0;
}
