#pragma once
#include <string>
#include <vector>
#include "PS_String.h"
//#include "DSystem/include/DContainers.h"

using namespace std;

namespace PS{
	namespace FILESTRINGUTILS{

		long GetFileSize(const char* chrFilePath);

		bool FileExists(const DAnsiStr& strFilePath);

		DAnsiStr GetExePath();

		void GetExePath(char *exePath, int szBuffer);

		DAnsiStr ExtractFileTitleOnly(const DAnsiStr& strFilePath);

		DAnsiStr ExtractFilePath(const DAnsiStr& fileName);

		DAnsiStr ExtractFileName(const DAnsiStr& strPathFileName);

		/*!
		 * Returns file extension without dot
		 */
		DAnsiStr ExtractFileExt(const DAnsiStr& strPathFileName);

		DAnsiStr CreateNewFileAtRoot(const char* pExtWithDot);

		DAnsiStr ChangeFileExt(const DAnsiStr& strFilePath, const DAnsiStr& strExtWithDot);


		//int listFilesInDir(std::vector<string>& lstFiles, const char* pDir = NULL, const char* pExtensions = NULL);	
		int ListFilesInDir(std::vector<DAnsiStr>& lstFiles, const char* pDir, const char* pExtensions, bool storeWithPath);
		//int ListFilesInDir(DVec<DAnsiStr>& lstFiles, const char* pDir, const char* pExtensions, bool storeWithPath);


		bool WriteTextFile(DAnsiStr strFN, const std::vector<DAnsiStr>& content);
		bool WriteTextFile(DAnsiStr strFN, const DAnsiStr& strContent );

		bool ReadTextFile(DAnsiStr strFN, std::vector<DAnsiStr>& content);
		bool ReadTextFile(DAnsiStr strFN, DAnsiStr& content);
	}
}
