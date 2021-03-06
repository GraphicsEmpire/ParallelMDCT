/*
 * PS_CPU_INFO.cpp
 *
 *  Created on: 2011-11-27
 *      Author: pourya
 */
#include "PS_CPU_INFO.h"
#include "tbb/task_scheduler_init.h"
#include "PS_SIMDVecN.h"
#include <string.h>
//#include "PS_FileDirectory.h"

//Implementation
const char* ProcessorInfo::GetCacheTypeString(CACHE_TYPE t)
{
	if(t == ctData)
		return "DATA";
	else if(t == ctInstruction)
		return "INSTRUCTION";
	else if(t == ctUnified)
		return "UNIFIED";
	else
		return "NONE";
}

void ProcessorInfo::getAllInfo()
{
	this->ctCores = tbb::task_scheduler_init::default_num_threads();
	this->cache_line_size  = GetCacheLineSize();
	this->simd_float_lines = PS_SIMD_FLEN;
	this->bSupportAVX 	   = SimdDetectFeature(AVXFlag);
	this->bOSSupportAVX    = SimdDetectFeature(OSXSAVEFlag);

	//U8  ctCacheInfo;
	//U8  cache_line_size;
	//U8  cache_levels[MAX_CACHE_INFO_LENGTH];
	//U32 cache_sizes[MAX_CACHE_INFO_LENGTH];
	//CACHE_TYPE cache_types[MAX_CACHE_INFO_LENGTH];


	this->ctCacheInfo = 0;
	for(int iLevel = 1; iLevel <= MAX_CACHE_LEVEL; iLevel++)
		getLevelInfo(iLevel);
}

void ProcessorInfo::getLevelInfo(int level)
{
	if(level == 1)
	{
		if(HasCacheLevel(1, ctData))
		{
			this->cache_levels[ctCacheInfo] = 1;
			this->cache_types[ctCacheInfo] = ctData;
			this->cache_sizes[ctCacheInfo] = GetCacheSize(1, ctData);
			ctCacheInfo++;
		}

		if(HasCacheLevel(1, ctInstruction))
		{
			this->cache_levels[ctCacheInfo] = 1;
			this->cache_types[ctCacheInfo] = ctInstruction;
			this->cache_sizes[ctCacheInfo] = GetCacheSize(1, ctInstruction);
			ctCacheInfo++;
		}
	}
	else
	{
		if(HasCacheLevel(level, ctUnified))
		{
			this->cache_levels[ctCacheInfo] = level;
			this->cache_types[ctCacheInfo] = ctUnified;
			this->cache_sizes[ctCacheInfo] = GetCacheSize(level, ctUnified);
			ctCacheInfo++;
		}
	}
}

#if defined(__APPLE__)

#include <sys/sysctl.h>
U32 ProcessorInfo::GetCacheLineSize()
{
    U32 line_size = 0;
    U32 sizeof_line_size = sizeof(line_size);
    sysctlbyname("hw.cachelinesize", &line_size, &sizeof_line_size, 0, 0);
    return line_size;
}

#elif defined(PS_OS_WINDOWS)

#include <stdlib.h>
#include <windows.h>
U32 ProcessorInfo::GetCacheLineSize()
{
    U32 line_size = 0;
    DWORD buffer_size = 0;
    DWORD i = 0;
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;

    GetLogicalProcessorInformation(0, &buffer_size);
    buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
    GetLogicalProcessorInformation(&buffer[0], &buffer_size);

    for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i) {
        if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
            line_size = buffer[i].Cache.LineSize;
            break;
        }
    }

    free(buffer);
    return line_size;
}

U32 ProcessorInfo::GetCacheSize(int level, CACHE_TYPE cacheType)
{
	U32 szCache = 0;
	DWORD buffer_size = 0;
	DWORD i = 0;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;

	GetLogicalProcessorInformation(0, &buffer_size);
	buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
	GetLogicalProcessorInformation(&buffer[0], &buffer_size);

	for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i)
	{
		if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == level) {
			szCache = buffer[i].Cache.Size;
			break;
		}
	}

	free(buffer);
	return szCache;
}

CACHE_TYPE ProcessorInfo::GetCacheType(int level, CACHE_TYPE cacheType)
{
	CACHE_TYPE cache_type = ctNone;
	DWORD buffer_size = 0;
	DWORD i = 0;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;

	GetLogicalProcessorInformation(0, &buffer_size);
	buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
	GetLogicalProcessorInformation(&buffer[0], &buffer_size);

	for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i)
	{
		if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == level) {
			if(buffer[i].Cache.Type == CacheUnified)
				cache_type = ctUnified;
			else if(buffer[i].Cache.Type == CacheData)
				cache_type = ctData;
			else if(buffer[i].Cache.Type == CacheInstruction)
				cache_type = ctInstruction;
			break;
		}
	}

	free(buffer);
	return cache_type;
}

bool ProcessorInfo::HasCacheLevel(int level, CACHE_TYPE cacheType)
{
	DWORD buffer_size = 0;
	DWORD i = 0;
	SYSTEM_LOGICAL_PROCESSOR_INFORMATION * buffer = 0;

	GetLogicalProcessorInformation(0, &buffer_size);
	buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
	GetLogicalProcessorInformation(&buffer[0], &buffer_size);

	for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i)
	{
		if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == level)
		{
			if(buffer[i].Cache.Size > 0)
				return true;
		}
	}

	free(buffer);
	return false;
}

#elif defined(PS_OS_LINUX)

#include <stdio.h>

int ProcessorInfo::GetCacheNodeIndex(int level, CACHE_TYPE cacheType)
{
    FILE * p = 0;
    char chrBuffer[1024];

    CACHE_TYPE tempCacheType = ctNone;
    int tempLevel = 0;

    //Iterate over cache nodes
    for(int i=0; i<MAX_CACHE_INFO_LENGTH; i++)
    {
		sprintf(chrBuffer, "/sys/devices/system/cpu/cpu0/cache/index%d/level", i);
		p = fopen(chrBuffer, "r");
		if (p)
		{
			fscanf(p, "%d", &tempLevel);
			fclose(p);

			if(tempLevel == level)
			{
				sprintf(chrBuffer, "/sys/devices/system/cpu/cpu0/cache/index%d/type", i);
				p = fopen(chrBuffer, "r");
				if (p)
				{
					fscanf(p,"%s", chrBuffer);

					if(strcmp(chrBuffer, "Data") == 0)
						tempCacheType = ctData;
					else if(strcmp(chrBuffer, "Instruction") == 0)
						tempCacheType = ctInstruction;
					else if(strcmp(chrBuffer, "Unified") == 0)
						tempCacheType = ctUnified;

					fclose(p);

					if(tempCacheType == cacheType)
						return i;
				}
			}
		}
    }

    return -1;
}

U32 ProcessorInfo::GetCacheLineSize()
{
    FILE * p = 0;
    p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
    U32 i = 0;
    if (p) {
        fscanf(p, "%d", &i);
        fclose(p);
    }
    return i;
}

bool ProcessorInfo::HasCacheLevel(int level, CACHE_TYPE cacheType)
{
	return (GetCacheNodeIndex(level, cacheType) >= 0);
}

U32 ProcessorInfo::GetCacheSize(int level, CACHE_TYPE cacheType)
{
	int index = GetCacheNodeIndex(level, cacheType);
	if(index < 0)
		return false;

    FILE * p = 0;
    char chrBuffer[1024];
    sprintf(chrBuffer, "/sys/devices/system/cpu/cpu0/cache/index%d/size", index);
    p = fopen(chrBuffer, "r");
    U32 i = 0;
    if (p) {
        fscanf(p, "%dK", &i);
        fclose(p);
    }

    //Report in Bytes
    return i*1024;
}

ProcessorInfo::CACHE_TYPE ProcessorInfo::GetCacheType(int level, ProcessorInfo::CACHE_TYPE cacheType)
{
	int index = GetCacheNodeIndex(level, cacheType);
	if(index >= 0)
		return cacheType;
	else
		return ctNone;
}

#else
#error Unrecognized platform
#endif

