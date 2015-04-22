#ifndef LIBRARY_UTIL_FILESYSTEMUTIL_H
#define LIBRARY_UTIL_FILESYSTEMUTIL_H

#include <string>
#include <Windows.h>

namespace ZGF
{
	using std::string;


	class FileSystemUtil
	{
	public:
		static void GetCurrWorkingDir(char** OUTpath)
		{
			string strPath;
			char szFull[_MAX_PATH];
			::GetModuleFileName(NULL, szFull, sizeof(szFull)/sizeof(char));
			string tempPath = szFull;
			size_t pathSize = tempPath.find_last_of('\\');
			strPath = tempPath.substr(0, pathSize + 1);
			*OUTpath = new char[strPath.size() + 1];
			memcpy(*OUTpath, strPath.c_str(), strPath.size());
			(*OUTpath)[strPath.size()] = '\0';
		}

		static string GetCurrWorkingDir()
		{
			string strPath;
			char szFull[_MAX_PATH];
			::GetModuleFileName(NULL, szFull, sizeof(szFull)/sizeof(char));
			string tempPath = szFull;
			size_t pathSize = tempPath.find_last_of('\\');
			strPath = tempPath.substr(0, pathSize + 1);
			return strPath;
		}

		static string GetCurrProcessName()
		{
			string strPath;
			char szFull[_MAX_PATH];
			::GetModuleFileName(NULL, szFull, sizeof(szFull)/sizeof(char));
			string tempPath = szFull;
			size_t pathSize = tempPath.find_last_of('\\');
			strPath = tempPath.substr(pathSize+1);
			pathSize = strPath.find_last_of('.');
			strPath = strPath.substr(0, pathSize);
			return strPath;
		}

		static void GetCurrProcessName(char** OUTname)
		{
			string strPath;
			char szFull[_MAX_PATH];
			::GetModuleFileName(NULL, szFull, sizeof(szFull)/sizeof(char));
			string tempPath = szFull;
			size_t pathSize = tempPath.find_last_of('\\');
			strPath = tempPath.substr(pathSize+1);
			pathSize = strPath.find_last_of('.');
			strPath = strPath.substr(0, pathSize).c_str();
			*OUTname = new char[strPath.size() + 1];
			memcpy(*OUTname, strPath.c_str(), strPath.size());
			(*OUTname)[strPath.size()] = '\0';
		}
	};
}



#endif