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
#include "PS_JobDispatcher.h"

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
U32 CountSamples(string strSoundFP);

template<typename T>
U32 ReadSoundFile(string strSoundFP, std::vector<T>& outputData, U32 windowSize = 1);

template<typename T>
int SaveCSV(string strFP, std::vector<T>& data);


//Application Settings
struct APPSETTINGS{
	int ctThreadCountStart;
	int ctThreadCountEnd;
	int ctThreads;
	int ctFilesToUse;

	bool bProcessBatchInParallel;
	bool bUseSIMD;
	bool bCopyParallel;
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
		for(U32 i=0; i<ctSeries; i++)
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
int SaveBinary(string strFP, const std::vector<T>& data)
{
	char buffer[1024];

	ofstream myfile;
	myfile.open (strFP.c_str());

	//Write number of samples
	myfile << data.size();
	for(U32 i=0; i<data.size(); i++)
	{
		myfile << data[i];
	}

	myfile.close();
	return 1;
}

template<typename T>
int LoadBinary(string strFP, std::vector<T>& data)
{
	ifstream myfile;
	myfile.open (strFP.c_str());

	U32 ctSamples = 0;
	myfile >> ctSamples;

	data.resize(ctSamples);
	myfile.read((char *)data.data(), sizeof(T) * ctSamples);
	myfile.close();
	return 1;
}


/*!
 * Save data array in CSV format
 */
template<typename T>
int SaveCSV(string strFP, std::vector<T>& data)
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

bool sqlite_InsertLogRecord(const char* lpStrSoundName, double processTimeMS, U8 ctSIMDLength,
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
	sqlite3_bind_int(statement, idxParam, ctSIMDLength);

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


//Main
int main(int argc, char* argv[])
{
	//Detect CPU Info
	ProcessorInfo info;

	memset(&g_appSettings, 0, sizeof(APPSETTINGS));
	g_appSettings.ctFilesToUse = 1;
	g_appSettings.bDisplaySpectrums = false;
	g_appSettings.bRoot     = true;
	g_appSettings.bPrintPlot = false;
	g_appSettings.bProcessBatchInParallel = false;
	g_appSettings.bUseSIMD   = true;
	g_appSettings.bCopyParallel = true;
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

		if(std::strcmp(strArg.c_str(), "-s") == 0)
		{
			if(atoi(argv[i+1]) != 0)
				g_appSettings.bUseSIMD = true;
			else
				g_appSettings.bUseSIMD = false;
		}

		if(std::strcmp(strArg.c_str(), "-c") == 0)
		{
			if(atoi(argv[i+1]) != 0)
				g_appSettings.bCopyParallel = true;
			else
				g_appSettings.bCopyParallel = false;
		}

		if(std::strcmp(strArg.c_str(), "-b") == 0)
		{
			if(atoi(argv[i+1]) != 0)
				g_appSettings.bProcessBatchInParallel = true;
			else
				g_appSettings.bProcessBatchInParallel = false;
		}


		if(std::strcmp(strArg.c_str(), "-h") == 0)
		{
			printf("Usage: ParsipCmd -t [threads_count] -i -a [attempts] -c [cell_size] -f [model_file_path]\n");
			//printf("-a \t [runs] Number of runs for stats.\n");
			printf("-f \t [FileCount] Number of audio files to use for this experiment.\n");
			printf("-r \t Path is root.\n");
			printf("-i \t Display spectrums.\n");
			printf("-b \t [0/1] Turn on and off Batch in Parallel.\n");
			printf("-c \t [0/1] Turn on and off Copy in Parallel.\n");
			printf("-s \t [0/1] Turn on and off SIMD.\n");
			printf("-p \t [Start - End] Plot performance graph using db values in range.\n");
			printf("-t \t [threads count] Threads count.\n");
			printf("-x \t [Start - End] Threads count interval.\n");

			return 0;
		}
	}

	printf("Parallel MDCT Feature Extraction for MIR - Pourya Shirazian [Use -h for usage.]\n");
	printf("**********************************************************************************\n");
	if(info.bOSSupportAVX)
		printf("OS supports AVX\n");
	else
		printf("OS DOESNOT support AVX\n");

	if(info.bSupportAVX)
		printf("CPU supports AVX.\n");
	else
		printf("CPU DOESNOT support AVX\n");


	printf("Running Machine: CoresCount:%u, SIMD FLOATS:%u\n", info.ctCores, info.simd_float_lines);
	printf("CacheLine:%u [B]\n", info.cache_line_size);
	for(int i=0; i<info.ctCacheInfo; i++)
	{
		if(info.cache_types[i] != ProcessorInfo::ctNone)
		{
			printf("\tLevel:%d, Type:%s, Size:%u [KB]\n",
					info.cache_levels[i],
					ProcessorInfo::GetCacheTypeString(info.cache_types[i]),
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
	for(int i=info.ctCacheInfo; i >= 0; i--)
	{
		if(info.cache_types[i] != ProcessorInfo::ctNone)
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
	//g_appSettings.ctThreadCountStart = g_appSettings.ctThreadCountEnd = 1;
	for(int iThread = g_appSettings.ctThreadCountStart; iThread  <= g_appSettings.ctThreadCountEnd; iThread++)
	{
		SAFE_DELETE(g_lpTaskSchedular);
		g_appSettings.ctThreads = iThread;
		g_lpTaskSchedular = new tbb::task_scheduler_init(g_appSettings.ctThreads);
		printf("**********************************************************************\n");
		printf("Parallel MDCT - Initialized with %d threads. Simd is %d. Parallel Copy is %d. Parallel Batch is %d\n",
				g_appSettings.ctThreads, g_appSettings.bUseSIMD, g_appSettings.bCopyParallel, g_appSettings.bProcessBatchInParallel);


		//Report Total Time Spent
		tbb::tick_count t0 = tbb::tick_count::now();

		U32 ctTotalFrames = 0;
		if(g_appSettings.bProcessBatchInParallel)
		{
			U32 ctFiles = MATHMIN(g_appSettings.ctFilesToUse, vFiles.size());
			ProcessBatched body(vFiles, g_appSettings.bUseSIMD, g_appSettings.bCopyParallel);
			tbb::parallel_for(blocked_range<size_t>(0, ctFiles), body, tbb::auto_partitioner());
		}
		else
		{
			double ms;
			U32 ctFrames = 0;
			//For all files
			for(U32 iFile =0; iFile<(U32)g_appSettings.ctFilesToUse; iFile++)
			{
				//Arrays to hold values
				std::vector<float> arrInputSignal;
				std::vector<float> arrOutputSpectrum;

				tbb::tick_count pt0 = tbb::tick_count::now();

				//1. Read Sound Files
				arrInputSignal.reserve(1024 * FRAME_DATA_COUNT);
				ReadSoundFile(vFiles[iFile], arrInputSignal, 1);

				//2. Apply MDCT
				ctFrames = ApplyMDCT<float>(arrInputSignal, arrOutputSpectrum, g_appSettings.bUseSIMD, g_appSettings.bCopyParallel);
				ctTotalFrames += ctFrames;
				/*
				if(bSimd)
					SaveCSV<float>("SpectrumSimd.csv", arrOutputSpectrum);
				else
					SaveCSV<float>("SpectrumNormal.csv", arrOutputSpectrum);
					*/

				tbb::tick_count pt1 = tbb::tick_count::now();
				ms = (pt1 - pt0).seconds() * 1000.0;
				printf("[%u of %u] File %s processed in %.2f [ms] \n", iFile+1, g_appSettings.ctFilesToUse, vFiles[iFile].c_str(), ms);

				if(g_appSettings.bDisplaySpectrums)
				{
					//3. Normalized Output
					NormalizeData<float>(arrOutputSpectrum);

					vec3f c;
					c.x = RandRangeT<float>(0.0f, 1.0f);
					c.y = RandRangeT<float>(0.0f, 1.0f);
					c.z = RandRangeT<float>(0.0f, 1.0f);
					g_lpDrawChart->addSeries(arrOutputSpectrum, "Spectrum", 1.0f, vec2f(0.0, 1.5f), c);
				}

				//Log
				{
					U8 simdLen = PS_SIMD_FLEN;
					if(!g_appSettings.bUseSIMD)
						simdLen = 1;

					DAnsiStr strFN = PS::FILESTRINGUTILS::ExtractFileName(DAnsiStr(vFiles[iFile].c_str()));
					sqlite_InsertLogRecord(strFN.ptr(), ms, simdLen, arrInputSignal.size(),
										   ctFrames, 1, iThread, szWorkMem, ctFrames*szWorkMem, szLLC);
				}

				//Cleanup
				arrInputSignal.resize(0);
				arrOutputSpectrum.resize(0);
			}
		}


		//Stop Process
		tbb::tick_count t1 = tbb::tick_count::now();
		double msTotal = (t1 - t0).seconds() * 1000.0;

		//Log
		U32 ctFiles = MATHMIN(vFiles.size(), g_appSettings.ctFilesToUse);
		if(ctFiles > 0)
		{
			U8 simdLen = PS_SIMD_FLEN;
			if(!g_appSettings.bUseSIMD)
				simdLen = 1;

			string strTitle;
			if(g_appSettings.bProcessBatchInParallel)
			{
				ctTotalFrames = PrintResults();
				strTitle = "Total Process Batch";
			}
			else
				strTitle = "Total Process Sequential";

			//Log
			sqlite_InsertLogRecord(strTitle.c_str(), msTotal, simdLen, ctTotalFrames * FRAME_DATA_COUNT,
							   	   ctTotalFrames, 1, iThread, szWorkMem, (ctTotalFrames/ctFiles) * szWorkMem, szLLC);
			printf("#FilesProcessed = %u, #FramesTotal = %u, TotalTime= %0.2f\n", ctFiles, ctTotalFrames, msTotal);
		}

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
