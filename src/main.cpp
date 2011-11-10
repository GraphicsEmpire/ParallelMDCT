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

#include "PS_CPU_INFO.h"
#include "PS_SIMDVecN.h"

#include "AA_Models/PS_FileDirectory.h"
#include "AA_Models/PS_SketchConfig.h"
#include "MarSystemManager.h"

#ifdef WIN32
	#include <io.h>
	#include "Windows.h"
#elif defined(__linux__)
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <dirent.h>
	#include <unistd.h>
#endif

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

void Close()
{

}

void Display()
{
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

	glLoadIdentity();
	//glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
	//glTranslatef(0.0f, 0.0f, 10.0f);

	/*
	vec3f p = g_arcBallCam.getCoordinates();
	vec3f c = g_arcBallCam.getCenter();
	gluLookAt(p.x, p.y, p.z, c.x, c.y, c.z, 0.0f, 1.0f, 0.0f);
	*/

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

void processMDCT(string strSoundFP, U32 windowSize = 1)
{
	MarSystemManager mng;

	// A series to contain everything
	MarSystem* net = mng.create("Series", "net");

	// The sound file
	net->addMarSystem(mng.create("SoundFileSource", "src"));
	net->updControl("SoundFileSource/src/mrs_string/filename", strSoundFP);
	net->updControl("SoundFileSource/src/mrs_natural/pos", 0);
	//net->updControl("SoundFileSource/src/mrs_natural/pos", position_);
	net->setctrl("mrs_natural/inSamples", windowSize);
	net->addMarSystem(mng.create("MaxMin","maxmin"));

	mrs_natural channels = net->getctrl("mrs_natural/onObservations")->to<mrs_natural>();

	cout << "channels=" << channels << endl;

	realvec processedData;

	double y_max_left = 0;
	double y_min_left = 0;

	double y_max_right = 0;
	double y_min_right = 0;

	//Get Data Chunks and perform MDCT
	U32 ctTicks = 0;
	while (net->getctrl("SoundFileSource/src/mrs_bool/hasData")->to<mrs_bool>())
	{
		ctTicks++;

		net->tick();
		processedData = net->getctrl("mrs_realvec/processedData")->to<mrs_realvec>();

		U32 szData = processedData.getSize();
		y_max_left = processedData(0,0);
		y_min_left = processedData(0,1);
		//processedData.getCols();

		if (channels == 2)
		{
			y_max_right = processedData(1,0);
			y_min_right = processedData(1,1);
		}
	}

	delete net;
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
	static const GLfloat lightColor[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
	static const GLfloat lightPos[4] = { 5.0f, 5.0f, 10.0f, 0.0f };

	//glFrontFace(GL_CCW);
	//Set Colors of Light
	glLightfv(GL_LIGHT0, GL_AMBIENT, lightColor);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT0, GL_SPECULAR, lightColor);

	//Set Light Position
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

	//Turn on Light 0
	glEnable(GL_LIGHT0);

	//Enable Lighting
	glEnable(GL_LIGHTING);
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

	printf("Start Feature Extraction on %d files.\n", vFiles.size());

	if(vFiles.size() > 0)
	{
		//sfPlay(vFiles[0]);
		processMDCT(vFiles[0], 1);
	}



	glutMainLoop();



	return 0;
}
