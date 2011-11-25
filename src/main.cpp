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
#include "AA_Sqlite/sqlite3.h"
#include "MarSystemManager.h"

#include "PS_CPU_INFO.h"
#include "PS_MDCT.h"
#include "PS_DrawChart.h"
#include "PS_GNUPLOT_Driver.h"
#include "PS_Classifier.h"

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
using namespace PS::FILESTRINGUTILS;

//GLUT CallBacks
void Close();
void Display();
void Resize(int w, int h);
void Keyboard(int key, int x, int y);
void MousePress(int button, int state, int x, int y);
void MouseMove(int x, int y);

//App Functions
int ListFilesDir(std::vector<string>& vPaths, const char* chrPath, const char* chrExt, bool bDirOnly);

template<typename T>
U32 ReadSoundFile(string strSoundFP, std::vector<T>& outputData, U32 windowSize = 1);

int SaveCSV(string strFP, std::vector<double>& data);
int SaveARFF(string strFP, std::vector<double>& zeros, std::vector<double>& centroid, std::vector<char>& truth);

//Application Settings
struct APPSETTINGS{
	int ctThreadCountStart;
	int ctThreadCountEnd;
	int ctThreads;
	int ctFilesToUse;

	bool bDisplaySpectrums;
	bool bRoot;
	bool bPrintPlot;
	int idxPrintPlotStart;
	int idxPrintPlotEnd;
};

//Global Vars
std::string g_strLogFP;
tbb::task_scheduler_init* g_lpTaskSchedular = NULL;
CDrawChart* g_lpDrawChart = NULL;
APPSETTINGS g_appSettings;
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

	//cout << "channels=" << channels << endl;

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

int sqlite_Callback(void *NotUsed, int argc, char **argv, char **azColName)
{
	int i;
	for(i=0; i<argc; i++){
      printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	printf("\n");
	return 0;
}

bool sqlite_IsLogTableExist()
{
	sqlite3* db;
	char* lpSqlError = 0;
	int rc = sqlite3_open(g_strLogFP.c_str(), &db);
	if(rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return false;
	}

	const char *strSQL = "SELECT CASE WHEN tbl_name = 'tblPerfLog' THEN 1 ELSE 0 END FROM sqlite_master WHERE tbl_name = 'tblPerfLog' AND type = 'table'";
	rc = sqlite3_exec(db, strSQL, sqlite_Callback, 0, &lpSqlError);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", lpSqlError);
		sqlite3_free(lpSqlError);
		return false;
	}
	return true;
}



bool sqlite_CreateTableIfNotExist()
{
	sqlite3* db;
	char* lpSqlError = 0;
	int rc = sqlite3_open(g_strLogFP.c_str(), &db);
	if(rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return false;
	}

	const char *strSQL = "CREATE TABLE IF NOT EXISTS tblPerfLog(xpID INTEGER PRIMARY KEY AUTOINCREMENT, xpSoundName VARCHAR(30), xpTime DATETIME, "
						 "ctSignalSamples int, ctFrames int, ctFramesPerChunk int, "
						 "processTime double, ctThreads smallint, ctSIMDLength smallint, szWorkItemMem int, szTotalMemUsage int, szLastLevelCache int);";

	rc = sqlite3_exec(db, strSQL, sqlite_Callback, 0, &lpSqlError);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", lpSqlError);
		sqlite3_free(lpSqlError);
		return false;
	}

	return true;
}

bool sqlite_InsertLogRecord(const char* lpStrSoundName, double processTimeMS,
							U32 ctSamples, U32 ctFrames, U32 ctFramesPerChunk,
							U32 ctThreads, U32 szWorkMem, U32 szTotalMem, U32 szLLC)
{
	sqlite3* db;
	char* lpSqlError = 0;

	int rc = sqlite3_open(g_strLogFP.c_str(), &db);
	if(rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
	}

	//////////////////////////////////////////////////////////
	//Model Name
	const int MODEL_STRING_LENGTH = 30;
	char chrModelName[MODEL_STRING_LENGTH];
	{
		strncpy(chrModelName, lpStrSoundName, MODEL_STRING_LENGTH);
	}

	//Get Time
	const int TIME_STRING_LENGTH = 20;
	char chrTimeStamp[TIME_STRING_LENGTH];
	{
		time_t rawtime;
		struct tm* currentTime;
		time(&rawtime);
		currentTime = localtime(&rawtime);
		strftime(chrTimeStamp, TIME_STRING_LENGTH, "%Y-%m-%d %H:%M:%S", currentTime);
	}

	//const char *strSQL = "CREATE TABLE IF NOT EXISTS tblPerfLog(xpID INTEGER PRIMARY KEY AUTOINCREMENT, xpSoundName VARCHAR(30), xpTime DATETIME, "
	//					 "ctSignalSamples int, ctFrames int, ctFramesPerChunk int, "
	//					 "processTime double, ctThreads smallint, ctSIMDLength smallint, szWorkItemMem int, szTotalMemUsage int, szLastLevelCache int);";
	//////////////////////////////////////////////////////////
	const char *strSQL = "INSERT INTO tblPerfLog (xpSoundName, xpTime, ctSignalSamples, ctFrames, ctFramesPerChunk, "
						"processTime, ctThreads, ctSIMDLength, szWorkItemMem, szTotalMemUsage, szLastLevelCache) values"
						"(@xp_sound_name, @xp_time, @ct_signal_samples, @ct_frames, @ct_frames_per_chunk, "
						"@process_time, @ct_threads, @ct_simd_length, @sz_work_item_mem, @sz_total_mem_usage, @sz_last_level_cache);";

	sqlite3_stmt* statement;
	sqlite3_prepare_v2(db, strSQL, -1, &statement, NULL);

	//Bind All Parameters for Insert Statement
	//int ctParams = sqlite3_bind_parameter_count(statement);

	int idxParam = sqlite3_bind_parameter_index(statement, "@xp_sound_name");
	sqlite3_bind_text(statement, idxParam, chrModelName, -1, SQLITE_TRANSIENT);

	idxParam = sqlite3_bind_parameter_index(statement, "@xp_time");
	sqlite3_bind_text(statement, idxParam, chrTimeStamp, -1, SQLITE_TRANSIENT);

	idxParam = sqlite3_bind_parameter_index(statement, "@ct_signal_samples");
	sqlite3_bind_int(statement, idxParam, ctSamples);

	//Hardware Log
	idxParam = sqlite3_bind_parameter_index(statement, "@ct_frames");
	sqlite3_bind_int(statement, idxParam, ctFrames);

	idxParam = sqlite3_bind_parameter_index(statement, "@ct_frames_per_chunk");
	sqlite3_bind_int(statement, idxParam, ctFramesPerChunk);

	idxParam = sqlite3_bind_parameter_index(statement, "@process_time");
	sqlite3_bind_double(statement, idxParam, processTimeMS);

	idxParam = sqlite3_bind_parameter_index(statement, "@ct_threads");
	sqlite3_bind_int(statement, idxParam, ctThreads);

	idxParam = sqlite3_bind_parameter_index(statement, "@ct_simd_length");
	sqlite3_bind_int(statement, idxParam, PS_SIMD_FLEN);

	idxParam = sqlite3_bind_parameter_index(statement, "@sz_work_item_mem");
	sqlite3_bind_int(statement, idxParam, szWorkMem);

	idxParam = sqlite3_bind_parameter_index(statement, "@sz_total_mem_usage");
	sqlite3_bind_int(statement, idxParam, szTotalMem);

	idxParam = sqlite3_bind_parameter_index(statement, "@sz_last_level_cache");
	sqlite3_bind_int(statement, idxParam, szLLC);


	rc = sqlite3_step(statement);
	if(rc != SQLITE_DONE)
	{
		fprintf(stderr, "SQL error: %s\n", sqlite3_errmsg(db));
		sqlite3_free(lpSqlError);
	}

	//Free Statement
	sqlite3_finalize(statement);

	//Close DB
	sqlite3_close(db);

	return true;
}

void doQ1()
{
	std::vector<DAnsiStr> vFiles;
	{
		//Create a list of all files
		using namespace PS;
		using namespace PS::FILESTRINGUTILS;

		DAnsiStr strInfPath = ChangeFileExt(GetExePath(), ".inf");
		CSketchConfig* lpConfig = new PS::CSketchConfig(strInfPath, CSketchConfig::fmRead);

		DAnsiStr strRoot = lpConfig->readString("GENERAL", "ROOTPATH");
		DAnsiStr strExt  = lpConfig->readString("GENERAL", "FILEEXT");
		//int ctGroups = lpConfig->readInt("GENERAL", "GROUPCOUNT");
		DAnsiStr strFile;

		strFile = strRoot + DAnsiStr("/classical/classical.00000.au");
		vFiles.push_back(strFile);

		strFile = strRoot + DAnsiStr("/classical/classical.00009.au");
		vFiles.push_back(strFile);

		strFile = strRoot + DAnsiStr("/disco/disco.00000.au");
		vFiles.push_back(strFile);

		strFile = strRoot + DAnsiStr("/disco/disco.00009.au");
		vFiles.push_back(strFile);

		SAFE_DELETE(lpConfig);
	}

	std::vector<double> centroids;
	std::vector<double> zeros;
	std::vector<char> groundTruth;

	DAnsiStr strExeDir = ExtractFilePath(GetExePath());
	for(int i=0; i<vFiles.size(); i++)
	{
		//
		printf("Processing %s\n", vFiles[i].ptr());
		getchar();

		//Classifier::computeSpectralCentroid(vFiles[i].ptr(), centroids, 512);
		//int ctTicks = Classifier::computeSpectralCentroidWithZeroCrossings(vFiles[i].ptr(), centroids, zeros, 512);
		int ctTicks = Classifier::computeSpectralCentroidWithZeroCrossingsSmoothed(vFiles[i].ptr(), centroids, zeros, 512, 40);
		//Classifier::sonifySpectralCentroid(centroids);


		for(int j=0; j<ctTicks; j++)
		{
			if(i < 2)
				groundTruth.push_back('c');
			else
				groundTruth.push_back('d');
		}

		/*
		DAnsiStr strFP1 = strExeDir + ChangeFileExt(ExtractFileName(vFiles[i]), DAnsiStr(".csv") + DAnsiStr("SmoothCent"));
		SaveCSV(strFP1.c_str(), centroids);


		DAnsiStr strFP2 = strExeDir + ChangeFileExt(ExtractFileName(vFiles[i]), DAnsiStr(".csv")) + DAnsiStr("SmoothZeros");
		SaveCSV(strFP2.c_str(), zeros);
		centroids.resize(0);
		zeros.resize(0);
		*/

	}

	DAnsiStr strArff = ExtractFilePath(GetExePath()) + "genre_smooth.arff";
	SaveARFF(strArff.ptr(), zeros, centroids, groundTruth);
}

int SaveARFF(string strFP, std::vector<double>& zeros, std::vector<double>& centroid, std::vector<char>& truth)
{
	char buffer[1024];

	ofstream myfile;
	myfile.open (strFP.c_str());

	string strTemp;
	myfile << "% ARFF exported by Pourya Shirazian."<< endl;
	myfile << "@relation genre_classic_disco" << endl;
	myfile << "@attribute \'cent\' real"<< endl;
	myfile << "@attribute \'zero\' real"<< endl;
	myfile << "@attribute \'class\' {classic, disco}"<< endl;
	myfile << "@data"<< endl;

	U32 ctFrames = centroid.size();
	for(U32 i=0; i<ctFrames; i++)
	{
		if(truth[i] == 'c')
			sprintf(buffer, "%.4f, %.4f, classic", centroid[i], zeros[i]);
		else
			sprintf(buffer, "%.4f, %.4f, disco", centroid[i], zeros[i]);

		strTemp = string(buffer);
		myfile << strTemp << endl;
	}

	myfile.close();
	return 1;
}

int SaveCSV(string strFP, std::vector<double>& data)
{
	char buffer[1024];

	ofstream myfile;
	myfile.open (strFP.c_str());

	string strTemp;
	for(U32 i=0; i<data.size(); i++)
	{
		if(i < data.size() - 1)
			sprintf(buffer, "%.4f, ", data[i]);
		else
			sprintf(buffer, "%.4f", data[i]);

		strTemp = string(buffer);
		myfile << strTemp;
	}

	myfile.close();
	return 1;
}

//Main

int main(int argc, char* argv[]) {

	doQ1();
	//Detect CPU Info
	ProcessorInfo info;
	{
		info.bSupportAVX 	  = SimdDetectFeature(AVXFlag);
		info.ctCores 		  = tbb::task_scheduler_init::default_num_threads();
		info.simd_float_lines = PS_SIMD_FLEN;
		info.cache_line_size  = GetCacheLineSize();

		for(int i=0; i<4; i++)
		{
			info.cache_sizes[i]  = GetCacheSize(i, false);
			info.cache_levels[i] = GetCacheLevel(i);
			info.cache_types[i]  = GetCacheType(i);
		}
	}

	memset(&g_appSettings, 0, sizeof(APPSETTINGS));
	g_appSettings.ctFilesToUse = 1;
	g_appSettings.bDisplaySpectrums = false;
	g_appSettings.bRoot     = true;
	g_appSettings.bPrintPlot = false;
	g_appSettings.ctThreads = info.ctCores;
	g_appSettings.ctThreadCountStart = info.ctCores;
	g_appSettings.ctThreadCountEnd   = info.ctCores;


	for(int i=0; i<argc; i++)
	{
		std::string strArg = argv[i];

		/*
		if(std::strcmp(strArg.c_str(), "-a") == 0)
		{
			g_appSettings.attempts = atoi(argv[i+1]);
		}
		*/

		if(std::strcmp(strArg.c_str(), "-f") == 0)
		{
			g_appSettings.ctFilesToUse = atoi(argv[i+1]);
		}

		if(std::strcmp(strArg.c_str(), "-r") == 0)
		{
			g_appSettings.bRoot = true;
		}

		if(std::strcmp(strArg.c_str(), "-i") == 0)
		{
			g_appSettings.bDisplaySpectrums = true;
		}

		if(std::strcmp(strArg.c_str(), "-p") == 0)
		{
			g_appSettings.bPrintPlot = true;
			g_appSettings.idxPrintPlotStart = atoi(argv[i+1]);
			g_appSettings.idxPrintPlotEnd = atoi(argv[i+2]);
		}

		if(std::strcmp(strArg.c_str(), "-t") == 0)
		{
			g_appSettings.ctThreads = atoi(argv[i+1]);
			g_appSettings.ctThreadCountStart = g_appSettings.ctThreads;
			g_appSettings.ctThreadCountEnd = g_appSettings.ctThreads;
		}

		if(std::strcmp(strArg.c_str(), "-x") == 0)
		{
			g_appSettings.ctThreadCountStart = atoi(argv[i+1]);
			g_appSettings.ctThreadCountEnd = atoi(argv[i+2]);
			g_appSettings.ctThreads = g_appSettings.ctThreadCountStart;
		}

		if(std::strcmp(strArg.c_str(), "-h") == 0)
		{
			printf("Usage: ParsipCmd -t [threads_count] -i -a [attempts] -c [cell_size] -f [model_file_path]\n");
			//printf("-a \t [runs] Number of runs for stats.\n");
			printf("-f \t [FileCount] Number of audio files to use for this experiment.\n");
			printf("-r \t Path is root.\n");
			printf("-i \t Display spectrums.\n");
			printf("-p \t [Start - End] Plot performance graph using db values in range.\n");
			printf("-t \t [threads count] Threads count.\n");
			printf("-x \t [Start - End] Threads count interval.\n");

			return 0;
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

	//Cache Memory

	U32 szWorkMem = 3 * sizeof(CMDCTComputer<double>::CHUNKS1);
	U32 szLLC = 0;
	for(int i=3; i >= 0; i--)
	{
		if(info.cache_types[i] != ctNone)
		{
			szLLC = info.cache_sizes[i];
			if(szLLC > 0) break;
		}
	}
	printf("Last Level Cache: %u [B], Work Memory Per Thread: %u [B].\n", szLLC, szWorkMem);

	//Correct if not found enough files
	g_appSettings.ctFilesToUse = MATHMIN(g_appSettings.ctFilesToUse, (int)vFiles.size());
	printf("Sound Files Found: %d, Files to be Processed: %d.\n", vFiles.size(), g_appSettings.ctFilesToUse);

	//Create log table if it doesnot exist
	//Get Log File Path
	{
		char buffer[1024];
		GetExePath(buffer, 1024);
		std::strcat(buffer, "Log.db");
		g_strLogFP = std::string(buffer);
	}
	sqlite_CreateTableIfNotExist();


	//iterate over threads
	for(int iThread = g_appSettings.ctThreadCountStart; iThread  <= g_appSettings.ctThreadCountEnd; iThread++)
	{
		SAFE_DELETE(g_lpTaskSchedular);
		g_appSettings.ctThreads = iThread;
		g_lpTaskSchedular = new tbb::task_scheduler_init(g_appSettings.ctThreads);
		printf("**********************************************************************\n");
		printf("Parallel MDCT - Initialized with %d threads. \n", g_appSettings.ctThreads);

		double ms;
		int ctFrames = 0;

		//For all files
		for(U32 iFile =0; iFile<(U32)g_appSettings.ctFilesToUse; iFile++)
		{
			//Arrays to hold values
			std::vector<double> arrInputSignal;
			std::vector<double> arrOutputSpectrum;

			tbb::tick_count t0 = tbb::tick_count::now();

			//1. Read Sound Files
			ReadSoundFile<double>(vFiles[iFile], arrInputSignal, 1);

			//2. Apply MDCT
			ApplyMDCT<double>(arrInputSignal, arrOutputSpectrum, ctFrames);

			tbb::tick_count t1 = tbb::tick_count::now();
			ms = (t1 - t0).seconds() * 1000.0;
			printf("[%u of %u] File %s processed in %.2f [ms] \n", iFile+1, g_appSettings.ctFilesToUse, vFiles[iFile].c_str(), ms);

			if(g_appSettings.bDisplaySpectrums)
			{
				//3. Normalized Output
				NormalizeData<double>(arrOutputSpectrum);
				//arrOutputSpectrum.resize(100);

				vec3f c;
				c.x = RandRangeT<float>(0.0f, 1.0f);
				c.y = RandRangeT<float>(0.0f, 1.0f);
				c.z = RandRangeT<float>(0.0f, 1.0f);
				g_lpDrawChart->addSeries(arrOutputSpectrum, "Spectrum", 1.0f, vec2f(0.0, 1.5f), c);
			}

			//Log
			{
				DAnsiStr strFN = PS::FILESTRINGUTILS::ExtractFileName(DAnsiStr(vFiles[iFile].c_str()));
				sqlite_InsertLogRecord(strFN.ptr(), ms, arrInputSignal.size(),
									   ctFrames, 1, iThread, szWorkMem, ctFrames*szWorkMem, szLLC);
			}

			//Cleanup
			arrInputSignal.resize(0);
			arrOutputSpectrum.resize(0);
		}

		//Log Frames Distribution
		//PrintThreadWorkHistory(g_appSettings.ctFilesToUse);
	}


	if(g_appSettings.bPrintPlot)
	{
		printf("**********************************************************************************\n");
		printf("Preparing GNUPLOT Data in range [%d, %d] \n",
				g_appSettings.idxPrintPlotStart,
				g_appSettings.idxPrintPlotEnd);
		GNUPlotDriver* lpPlot = new GNUPlotDriver(g_appSettings.idxPrintPlotStart, g_appSettings.idxPrintPlotEnd);
		bool bres = lpPlot->createGraph();
		SAFE_DELETE(lpPlot);

		if(bres)
		{
			printf("Log.dat file created with format [ctThreads ProcessTime] just run gnuplot on it.\n");
			printf("Example: plot \"./ParsipCmdLinuxLog.dat\" using 1:2 title \"Performance\" with lines. \n");
		}
	}

	glutMainLoop();


	return 0;
}
