/*
 * PS_GNUPLOT_Driver.cpp
 *
 *  Created on: 2011-10-18
 *      Author: pourya
 */

#include <string.h>
#include <limits.h>
#include "PS_GNUPLOT_Driver.h"
#include "AA_Models/PS_FileDirectory.h"
using namespace PS;
using namespace PS::FILESTRINGUTILS;


GNUPlotDriver::GNUPlotDriver(int iStartXP, int iEndXP)
{
	m_minX       = INT_MAX;
	m_maxX       = INT_MIN;
	m_minY 		 = FLT_MAX;
	m_maxY       = FLT_MIN;

	m_idxStart   = iStartXP;
	m_idxEnd     = iEndXP;
	m_strLabelX  = "Number of cores";
	m_strLabelY  = "Process time [ms]";
	m_strTitle   = "Process time vs. Number of cores";
	m_strBGColor = "white";
	m_strDataColor = "blue";
	m_lpGNUPlot = NULL;

	m_bRaise = true;
	m_bPersist = true;
}

GNUPlotDriver::~GNUPlotDriver()
{
	if(m_lpGNUPlot)
	{
		fclose(m_lpGNUPlot);
	}

}

int GNUPlotDriver::sqlite_Callback(void* lpDriver, int argc, char **argv, char **azColName)
{
	int x = 0;
	float y = 0;
	for(int i=0; i<argc; i++)
	{
		printf("%s = %s\t", azColName[i], argv[i] ? argv[i] : "NULL");
		if(strcmp(azColName[i], "ctThreads") == 0)
			x = atoi(argv[i]);
		else if(strcmp(azColName[i], "processTime") == 0)
			y = atof(argv[i]);
	}
	printf("\n");

	if(lpDriver)
		reinterpret_cast<GNUPlotDriver*>(lpDriver)->addDataPoint(x, y);
	return 0;
}

void GNUPlotDriver::addDataPoint(int x, float y)
{
	m_minX = (x < m_minX)? x : m_minX;
	m_maxX = (x > m_maxX)? x : m_maxX;

	m_minY = (y < m_minY)? y : m_minY;
	m_maxY = (y > m_maxX)? y : m_maxY;

	m_strData << x << " " << y << endl;
}

bool GNUPlotDriver::createGraph()
{
	string strDBPath;
	{
		char buffer[1024];
		GetExePath(buffer, 1024);
		strcat(buffer, "Log.db");
		strDBPath = std::string(buffer);
	}

	sqlite3* db;
	char* lpSqlError = 0;

	int rc = sqlite3_open(strDBPath.c_str(), &db);
	if(rc)
	{
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return false;
	}
/*
	const char *strSQL = "CREATE TABLE IF NOT EXISTS tblPerfLog(xpID INTEGER PRIMARY KEY AUTOINCREMENT, xpSoundName VARCHAR(30), xpTime DATETIME, "
						 "ctSignalSamples int, ctFrames int, ctFramesPerChunk int, "
						 "processTime double, ctThreads smallint, ctSIMDLength smallint, szWorkItemMem int, szTotalMemUsage int, szLastLevelCache int);";
*/
	char buffer[1024];

	sprintf(buffer, "SELECT xpID, processTotal, ctThreads, ctSIMDLength FROM tblPerfLog WHERE xpID >= %d and xpID <= %d", m_idxStart, m_idxEnd);

	rc = sqlite3_exec(db, buffer, sqlite_Callback, this, &lpSqlError);
	if(rc != SQLITE_OK)
	{
		fprintf(stderr, "SQL error: %s\n", lpSqlError);
		sqlite3_free(lpSqlError);
		return false;
	}

	string strDatFN;
	{
		ofstream myfile;
		GetExePath(buffer, 1024);
		strcat(buffer, "Log.dat");

		strDatFN = string(buffer);
		myfile.open (buffer);
		myfile << m_strData.str() << endl;
		myfile.close();
	}

#ifdef PS_OS_LINUX
    try{
		if(!m_lpGNUPlot)
		{
		  string com = (string) "gnuplot ";
		  m_lpGNUPlot = popen(com.c_str(),"w");
		}


		sprintf(buffer, "plot \"ParsipCmdLinuxLog.dat\" using 1:2", strDatFN.c_str());
		fprintf(m_lpGNUPlot, "%s", buffer);
    }

	catch (ios::failure const &problem)
	{
		printf("gnuplot_driver: %s\n", problem.what());
	}
#endif

	return true;

}
