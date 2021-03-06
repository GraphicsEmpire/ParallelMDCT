/*
 * PS_GNUPLOT_Driver.h
 *
 *  Created on: 2011-10-18
 *      Author: pourya
 */

#ifndef PS_GNUPLOT_DRIVER_H_
#define PS_GNUPLOT_DRIVER_H_

#include <iostream>
#include <cmath>
#include <fstream>
#include <sstream>

#include "AA_Sqlite/sqlite3.h"

using namespace std;

class GNUPlotDriver{
public:
	GNUPlotDriver(int iStartXP, int iEndXP);
	~GNUPlotDriver();

	bool createGraph();
	void addDataPoint(int x, float y);

	static int sqlite_Callback(void* lpDriver, int argc, char **argv, char **azColName);
private:
	int m_idxStart, m_idxEnd;
	int m_minX;
	int m_maxX;
	float m_minY;
	float m_maxY;

	stringstream m_strData;
	string m_strDBXLabel;
	string m_strDBYLabel;



	string m_strTitle;
	string m_strLabelX;
	string m_strLabelY;
	string m_strBGColor;
	string m_strDataColor;
	bool m_bRaise;
	bool m_bPersist;
	FILE* m_lpGNUPlot;
};


#endif /* PS_GNUPLOT_DRIVER_H_ */
