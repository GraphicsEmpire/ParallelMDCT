/*
 * PS_CPU_INFO.h
 *
 *  Created on: 2011-10-18
 *      Author: pourya
 */

#ifndef PS_CPU_INFO_H_
#define PS_CPU_INFO_H_

#include <stddef.h>
#include <string.h>
#include "PS_MathBase.h"


typedef enum CACHE_TYPE {ctNone, ctData, ctInstruction, ctUnified};

struct ProcessorInfo{
	U8 ctCores;
	U8 cache_line_size;
	U8 simd_float_lines;
	U32 cache_sizes[4];
	U8  cache_levels[4];
	CACHE_TYPE cache_types[4];

	bool bSupportAVX;
	bool bSupportSSE;
};

U32 GetCacheLineSize();

#if defined(__APPLE__)

#include <sys/sysctl.h>
U32 GetCacheLineSize() {
    U32 line_size = 0;
    U32 sizeof_line_size = sizeof(line_size);
    sysctlbyname("hw.cachelinesize", &line_size, &sizeof_line_size, 0, 0);
    return line_size;
}

#elif defined(_WIN32)

#include <stdlib.h>
#include <windows.h>
U32 GetCacheLineSize() {
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

#elif defined(linux)

#include <stdio.h>
U32 GetCacheLineSize()
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

U8 GetCacheLevel(int index)
{
    FILE * p = 0;
    char chrBuffer[1024];

    sprintf(chrBuffer, "/sys/devices/system/cpu/cpu0/cache/index%d/level", index);
    p = fopen(chrBuffer, "r");
    U8 level = 0;
    if (p) {
        fscanf(p, "%d", &level);
        fclose(p);
    }
    return level;
}

const char* GetCacheTypeString(CACHE_TYPE t)
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

U32 GetCacheSize(int index, bool bInKiloBytes = true)
{
    FILE * p = 0;
    char chrBuffer[1024];

    sprintf(chrBuffer, "/sys/devices/system/cpu/cpu0/cache/index%d/size", index);
    p = fopen(chrBuffer, "r");
    U32 i = 0;
    if (p) {
        fscanf(p, "%dK", &i);
        fclose(p);
    }

    if(!bInKiloBytes)
    	i *= 1024;

    return i;
}

CACHE_TYPE GetCacheType(int index)
{
    FILE * p = 0;
    char chrBuffer[1024];

    sprintf(chrBuffer, "/sys/devices/system/cpu/cpu0/cache/index%d/type", index);
    p = fopen(chrBuffer, "r");

    CACHE_TYPE cacheType = ctNone;
    if (p) {
    	fscanf(p,"%s", chrBuffer);

    	if(strcmp(chrBuffer, "Data") == 0)
    		cacheType = ctData;
    	else if(strcmp(chrBuffer, "Instruction") == 0)
    		cacheType = ctInstruction;
    	else if(strcmp(chrBuffer, "Unified") == 0)
    		cacheType = ctUnified;

    	fclose(p);
    }

    return cacheType;
}

#else
#error Unrecognized platform
#endif


#endif /* PS_CPU_INFO_H_ */
