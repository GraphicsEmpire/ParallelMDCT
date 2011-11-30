/*
 * PS_JobDispatcher.h
 *
 *  Created on: 2011-11-29
 *      Author: pourya
 */

#ifndef PS_JOBDISPATCHER_H_
#define PS_JOBDISPATCHER_H_

#include <vector>
#include <string>
#include <algorithm>

#include "PS_MathBase.h"
#include "tbb/blocked_range.h"
#include "tbb/enumerable_thread_specific.h"

using namespace std;
using namespace tbb;

typedef tbb::enumerable_thread_specific< std::pair<double, int> > CounterProcessTimeTotalFrames;


/*!
 * Performs Feature Extraction on a batch of files in Parallel
 */
class ProcessBatched{
private:
	std::vector<std::string> m_vFiles;
	bool m_bUseSimd;
	bool m_bCopyParallel;

public:
	ProcessBatched(const std::vector<string>& vFiles, bool bUseSimd, bool bCopyParallel)
	{
		m_bUseSimd = bUseSimd;
		m_bCopyParallel = bCopyParallel;
		m_vFiles.assign(vFiles.begin(), vFiles.end());
	}

	void operator()(const blocked_range<size_t>& range) const;

	/*!
	 * Processed one file and return number of miliseconds spent on file
	 */
	double process(const string& str, U32& ctOutFrames) const;

};

U32 PrintResults();

#endif /* PS_JOBDISPATCHER_H_ */
