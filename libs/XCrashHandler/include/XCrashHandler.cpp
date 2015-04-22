// XCrashHandler.cpp  Version 2.0.1.281
//
// Copyright ?1998 Bruce Dawson
//
// This source file contains the exception handler for recording error
// information after crashes. See ExceptionHandler.h for information
// on how to hook it in.
//
// Author:       Bruce Dawson
//               brucedawson@cygnus-software.com
//
// Modified by:  Hans Dietrich
//               hdietrich@gmail.com
//
// Version 1.5:  - Replaced asm with intrinsics
//               - Added support for x64
//               - Replaced ExceptionAttacher with SetUnhandledExceptionFilter()
//               - Dumped environment variables
//
// Version 1.4:  - Added invocation of XCrashReport.exe
//
// Version 1.3:  - Added minidump output
//
// Version 1.1:  - reformatted output for XP-like error report
//               - added ascii output to stack dump
//
// A paper by the original author can be found at:
//     http://www.cygnus-software.com/papers/release_debugging.html
//
///////////////////////////////////////////////////////////////////////////////

// Disable warnings generated by the Windows header files.
#pragma warning(disable : 4514)
#pragma warning(disable : 4201)

#define _WIN32_WINDOWS 0x0500	// for IsDebuggerPresent

// does not require MFC;  use 'Not using precompiled headers'

//#include <shlwapi.h>
#include <windows.h>
#include <tchar.h>
#include <intrin.h>
#include <lm.h>				// for NetWkstaGetInfo
#include "XGetWinVer.h"
#include "miniversion.h"
#include "dbghelp.h"
#include "CrashOptions.h"
#include "Shlobj.h"
#include "Screenshot.h"
#include "XCrashHandler.h"
#include "FileSystemUtil.h"
#include "Log.h"
#include <string>
#include <time.h>
using std::string;

#ifdef XCRASHREPORT_WRITE_MINIDUMP
#pragma message("automatic link to DBGHELP.LIB")
#pragma comment(lib, "dbghelp.lib")
#endif

#pragma message("automatic link to NETAPI32.LIB")
#pragma comment(lib, "netapi32.lib")

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

const int NumCodeBytes = 16;	// Number of code bytes to record.
const int MaxStackDump = 4096;	// Maximum number of DWORDS in stack dumps.
#ifdef _WIN64
const int StackColumns = 2;		// Number of columns in stack dump.
#else
const int StackColumns = 4;		// Number of columns in stack dump.
#endif

#define	ONEK			1024
#define	SIXTYFOURK		(64*ONEK)
#define	ONEM			(ONEK*ONEK)
#define	ONEG			(ONEK*ONEK*ONEK)

void GetTimeString(char* INbuffer, const char* INtimeFormat)
{
	time_t now;
	struct tm * timeinfo = NULL;

	time(&now);
	timeinfo = localtime(&now);

	if (timeinfo != NULL)
	{
		strftime(INbuffer, TIME_FORMAT_LENGTH, INtimeFormat, timeinfo);
	}
	else
	{
		INbuffer[0] = '\0';
	}
}

///////////////////////////////////////////////////////////////////////////////
// lstrrchr (avoid the C Runtime )
static TCHAR * lstrrchr(LPCTSTR string, int ch)
{
	TCHAR *start = (TCHAR *)string;

	while (*string++)                       /* find end of string */
		;
											/* search towards front */
	while (--string != start && *string != (TCHAR) ch)
		;

	if (*string == (TCHAR) ch)                /* char found ? */
		return (TCHAR *)string;

	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// hprintf behaves similarly to printf, with a few vital differences.
// It uses wvsprintf to do the formatting, which is a system routine,
// thus avoiding C run time interactions. For similar reasons it
// uses WriteFile rather than fwrite.
// The one limitation that this imposes is that wvsprintf, and
// therefore hprintf, cannot handle floating point numbers.

// Too many calls to WriteFile can take a long time, causing
// confusing delays when programs crash. Therefore I implemented
// a simple buffering scheme for hprintf

#define HPRINTF_BUFFER_SIZE (16*1024)				// must be at least 2048
static TCHAR hprintf_buffer[HPRINTF_BUFFER_SIZE];	// wvsprintf never prints more than one K.
static int  hprintf_index = 0;

///////////////////////////////////////////////////////////////////////////////
// hflush
static void hflush(HANDLE hLogFile)
{
	if (hprintf_index > 0)
	{
		DWORD NumBytes;
		WriteFile(hLogFile, hprintf_buffer, lstrlen(hprintf_buffer)*sizeof(TCHAR), 
			&NumBytes, 0);
		hprintf_index = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////
// hprintf
static void hprintf(HANDLE hLogFile, LPCTSTR Format, ...)
{
	if (hprintf_index > (HPRINTF_BUFFER_SIZE-1024))
	{
		DWORD NumBytes;
		WriteFile(hLogFile, hprintf_buffer, lstrlen(hprintf_buffer)*sizeof(TCHAR), 
			&NumBytes, 0);
		hprintf_index = 0;
	}

	va_list arglist;
	va_start(arglist, Format);
	hprintf_index += wvsprintf(&hprintf_buffer[hprintf_index], Format, arglist);
	va_end(arglist);
}

#ifdef XCRASHREPORT_WRITE_MINIDUMP

///////////////////////////////////////////////////////////////////////////////
// DumpMiniDump
static void DumpMiniDump(HANDLE hFile, PEXCEPTION_POINTERS excpInfo)
{
	if (excpInfo == NULL) 
	{
		// Generate exception to get proper context in dump
		__try 
		{
			OutputDebugString(_T("raising exception\r\n"));
			RaiseException(EXCEPTION_BREAKPOINT, 0, 0, NULL);
		} 
		__except(DumpMiniDump(hFile, GetExceptionInformation()),
				 EXCEPTION_CONTINUE_EXECUTION) 
		{
		}
	} 
	else
	{
		OutputDebugString(_T("writing minidump\r\n"));
		MINIDUMP_EXCEPTION_INFORMATION eInfo;
		eInfo.ThreadId = GetCurrentThreadId();
		eInfo.ExceptionPointers = excpInfo;
		eInfo.ClientPointers = FALSE;

		// note:  MiniDumpWithIndirectlyReferencedMemory does not work on Win98
		MiniDumpWriteDump(
			GetCurrentProcess(),
			GetCurrentProcessId(),
			hFile,
			MiniDumpNormal,
			excpInfo ? &eInfo : NULL,
			NULL,
			NULL);
		
	}
}

#endif	// XCRASHREPORT_WRITE_MINIDUMP

///////////////////////////////////////////////////////////////////////////////
// FormatTime
//
// Format the specified FILETIME to output in a human readable format,
// without using the C run time.
static void FormatTime(LPTSTR output, FILETIME TimeToPrint)
{
	output[0] = 0;
	WORD Date, Time;
	if (FileTimeToLocalFileTime(&TimeToPrint, &TimeToPrint) &&
				FileTimeToDosDateTime(&TimeToPrint, &Date, &Time))
	{
		wsprintf(output, _T("%d/%d/%d %02d:%02d:%02d"),
					(Date / 32) & 15, Date & 31, (Date / 512) + 1980,
					(Time >> 11), (Time >> 5) & 0x3F, (Time & 0x1F) * 2);
	}
}

///////////////////////////////////////////////////////////////////////////////
// DumpModuleInfo
//
// Print information about a code module (DLL or EXE) such as its size,
// location, time stamp, etc.
static bool DumpModuleInfo(HANDLE LogFile, HINSTANCE ModuleHandle, int nModuleNo)
{
	bool rc = false;
	TCHAR szModName[MAX_PATH*2];
	ZeroMemory(szModName, sizeof(szModName));

	__try
	{
		if (GetModuleFileName(ModuleHandle, szModName, _countof(szModName)-2) > 0)
		{
			// If GetModuleFileName returns greater than zero then this must
			// be a valid code module address. Therefore we can try to walk
			// our way through its structures to find the link time stamp.
			IMAGE_DOS_HEADER *DosHeader = (IMAGE_DOS_HEADER*)ModuleHandle;
		    if (IMAGE_DOS_SIGNATURE != DosHeader->e_magic)
	    	    return false;

			//IMAGE_NT_HEADERS *NTHeader = (IMAGE_NT_HEADERS*)((TCHAR *)DosHeader
			IMAGE_NT_HEADERS *NTHeader = (IMAGE_NT_HEADERS*)((BYTE *)DosHeader
						+ DosHeader->e_lfanew);
		    if (IMAGE_NT_SIGNATURE != NTHeader->Signature)
	    	    return false;

			// open the code module file so that we can get its file date and size
			HANDLE ModuleFile = CreateFile(szModName, GENERIC_READ,
						FILE_SHARE_READ, 0, OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL, 0);

			TCHAR TimeBuffer[100];
			TimeBuffer[0] = 0;
			
			DWORD FileSize = 0;
			if (ModuleFile != INVALID_HANDLE_VALUE)
			{
				FileSize = GetFileSize(ModuleFile, 0);
				FILETIME LastWriteTime;
				if (GetFileTime(ModuleFile, 0, 0, &LastWriteTime))
				{
					FormatTime(TimeBuffer, LastWriteTime);
				}
				CloseHandle(ModuleFile);
			}
			hprintf(LogFile, _T("Module %d\r\n"), nModuleNo);
			hprintf(LogFile, _T("%s\r\n"), szModName);
			hprintf(LogFile, _T("Image Base: 0x%08x  Image Size: 0x%08x\r\n"), 
				NTHeader->OptionalHeader.ImageBase, 
				NTHeader->OptionalHeader.SizeOfImage), 

			hprintf(LogFile, _T("Checksum:   0x%08x  Time Stamp: 0x%08x\r\n"), 
				NTHeader->OptionalHeader.CheckSum,
				NTHeader->FileHeader.TimeDateStamp);

			hprintf(LogFile, _T("File Size:  %-10d  File Time:  %s\r\n"),
						FileSize, TimeBuffer);

			hprintf(LogFile, _T("Version Information:\r\n"));

			CMiniVersion ver(szModName);
			TCHAR szBuf[200];
			WORD dwBuf[4];

			ver.GetCompanyName(szBuf, sizeof(szBuf)-1);
			hprintf(LogFile, _T("   Company:    %s\r\n"), szBuf);

			ver.GetProductName(szBuf, sizeof(szBuf)-1);
			hprintf(LogFile, _T("   Product:    %s\r\n"), szBuf);

			ver.GetFileDescription(szBuf, sizeof(szBuf)-1);
			hprintf(LogFile, _T("   FileDesc:   %s\r\n"), szBuf);

			ver.GetFileVersion(dwBuf);
			hprintf(LogFile, _T("   FileVer:    %d.%d.%d.%d\r\n"), 
				dwBuf[0], dwBuf[1], dwBuf[2], dwBuf[3]);

			ver.GetProductVersion(dwBuf);
			hprintf(LogFile, _T("   ProdVer:    %d.%d.%d.%d\r\n"), 
				dwBuf[0], dwBuf[1], dwBuf[2], dwBuf[3]);

			ver.Release();

			hprintf(LogFile, _T("\r\n"));

			rc = true;
		}
	}
	// Handle any exceptions by continuing from this point.
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
	}
	return rc;
}

///////////////////////////////////////////////////////////////////////////////
// DumpModuleList
//
// Scan memory looking for code modules (DLLs or EXEs). VirtualQuery is used
// to find all the blocks of address space that were reserved or committed,
// and ShowModuleInfo will display module information if they are code
// modules.
static void DumpModuleList(HANDLE LogFile)
{
	SYSTEM_INFO	SystemInfo;
	GetSystemInfo(&SystemInfo);

	const size_t PageSize = SystemInfo.dwPageSize;

	// Set NumPages to the number of pages in the 4GByte address space,
	// while being careful to avoid overflowing ints
	const size_t NumPages = 4 * size_t(ONEG / PageSize);
	size_t pageNum = 0;
	void *LastAllocationBase = 0;

	int nModuleNo = 1;

	while (pageNum < NumPages)
	{
		MEMORY_BASIC_INFORMATION MemInfo;
		if (VirtualQuery((void *)(pageNum * PageSize), &MemInfo, sizeof(MemInfo)))
		{
			if (MemInfo.RegionSize > 0)
			{
				// Adjust the page number to skip over this block of memory
				pageNum += MemInfo.RegionSize / PageSize;
				if (MemInfo.State == MEM_COMMIT && MemInfo.AllocationBase >
							LastAllocationBase)
				{
					// Look for new blocks of committed memory, and try
					// recording their module names - this will fail
					// gracefully if they aren't code modules
					LastAllocationBase = MemInfo.AllocationBase;
					if (DumpModuleInfo(LogFile, 
									   (HINSTANCE)LastAllocationBase, 
									   nModuleNo))
					{
						nModuleNo++;
					}
				}
			}
			else
			{
				pageNum += SIXTYFOURK / PageSize;
			}
		}
		else
		{
			pageNum += SIXTYFOURK / PageSize;
		}

		// If VirtualQuery fails we advance by 64K because that is the
		// granularity of address space doled out by VirtualAlloc()
	}
}

///////////////////////////////////////////////////////////////////////////////
// DumpSystemInformation
//
// Record information about the user's system, such as processor type, amount
// of memory, etc.
static void DumpSystemInformation(HANDLE LogFile)
{
	FILETIME CurrentTime;
	GetSystemTimeAsFileTime(&CurrentTime);
	TCHAR szTimeBuffer[100];
	FormatTime(szTimeBuffer, CurrentTime);

	hprintf(LogFile, _T("Error occurred at %s.\r\n"), szTimeBuffer);

	TCHAR szModuleName[MAX_PATH*2];
	ZeroMemory(szModuleName, sizeof(szModuleName));
	if (GetModuleFileName(0, szModuleName, _countof(szModuleName)-2) <= 0)
		lstrcpy(szModuleName, _T("Unknown"));

	TCHAR szUserName[200];
	ZeroMemory(szUserName, sizeof(szUserName));
	DWORD UserNameSize = _countof(szUserName)-2;
	if (!GetUserName(szUserName, &UserNameSize))
		lstrcpy(szUserName, _T("Unknown"));

	hprintf(LogFile, _T("%s, run by %s.\r\n"), szModuleName, szUserName);

	__try
	{
		LPWKSTA_INFO_100 pLMBuf = 0;
		NET_API_STATUS nStatus = NetWkstaGetInfo(NULL, 100, (LPBYTE *)&pLMBuf);
		if ((nStatus == NERR_Success) && pLMBuf)
		{
			// These strings will always be Unicode since _WIN32_WINNT is always defined.
			hprintf(LogFile, _T("Computer Name: %ls   Domain: %ls.\r\n"), 
				pLMBuf->wki100_computername, pLMBuf->wki100_langroup);
			NetApiBufferFree(pLMBuf);
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		hprintf(LogFile, _T("Exception encountered calling NetWkstaGetInfo.\r\n"));
	}

	// print out operating system
	TCHAR szWinVer[50], szMajorMinorBuild[50];
	int nWinVer;
	//GetWinVer(szWinVer, &nWinVer, szMajorMinorBuild);
	hprintf(LogFile, _T("Operating system:  %s (%s).\r\n"), 
		"Windows 7", "0");

	SYSTEM_INFO	SystemInfo;
	GetSystemInfo(&SystemInfo);
	hprintf(LogFile, _T("%d logical processor(s), type %d.\r\n"),
				SystemInfo.dwNumberOfProcessors, SystemInfo.dwProcessorType);

	////////////////////////////////////////////////////////////////////////////

	// __cpuid with an InfoType argument of 0 returns the number of
	// valid Ids in CPUInfo[0] and the CPU identification string in
	// the other three array elements. The CPU identification string is
	// not in linear order. 
	//
	// For more info:  http://msdn.microsoft.com/en-us/library/hskdteyh.aspx

	int CPUInfo[4];

	ZeroMemory(CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo, 0);

	char CPUVendor[40];			// always ANSI
	ZeroMemory(CPUVendor, sizeof(CPUVendor));
	*((int*)CPUVendor)     = CPUInfo[1];
	*((int*)(CPUVendor+4)) = CPUInfo[3];
	*((int*)(CPUVendor+8)) = CPUInfo[2];

	ZeroMemory(CPUInfo, sizeof(CPUInfo));
	__cpuid(CPUInfo, 1);

	UINT nSteppingId = CPUInfo[0] & 0xF;
	UINT nModel      = (CPUInfo[0] >> 4) & 0xF;
	UINT nFamily     = (CPUInfo[0] >> 8) & 0xF;

	hprintf(LogFile, _T("CPU Vendor: %hs  Stepping: %X  Model: %X  Family: %X.\r\n"), 
		CPUVendor, nSteppingId, nModel, nFamily);

	////////////////////////////////////////////////////////////////////////////

	MEMORYSTATUSEX MemInfo;
	MemInfo.dwLength = sizeof(MemInfo);
	GlobalMemoryStatusEx(&MemInfo);

	// Print out info on memory, rounded up.
	hprintf(LogFile, _T("%u%% memory in use:\r\n"), MemInfo.dwMemoryLoad);
	hprintf(LogFile, _T("%10u MB physical memory\r\n"), 
		(MemInfo.ullTotalPhys + ONEM - 1) / ONEM);
	hprintf(LogFile, _T("%10u MB physical memory free\r\n"), 
		(MemInfo.ullAvailPhys + ONEM - 1) / ONEM);
	hprintf(LogFile, _T("%10u MB paging file\r\n"), 
		(MemInfo.ullTotalPageFile + ONEM - 1) / ONEM);
	hprintf(LogFile, _T("%10u MB paging file free\r\n"), 
		(MemInfo.ullAvailPageFile + ONEM - 1) / ONEM);
	hprintf(LogFile, _T("%10u MB user address space\r\n"), 
		(MemInfo.ullTotalVirtual + ONEM - 1) / ONEM);
	hprintf(LogFile, _T("%10u MB user address space free\r\n"), 
		(MemInfo.ullAvailVirtual + ONEM - 1) / ONEM);
}

///////////////////////////////////////////////////////////////////////////////
// GetExceptionDescription
//
// Translate the exception code into something human readable
static const TCHAR *GetExceptionDescription(DWORD ExceptionCode)
{
	struct ExceptionNames
	{
		DWORD	ExceptionCode;
		TCHAR *	ExceptionName;
	};

	// from ntstatus.h
	static ExceptionNames ExceptionMap[] =
	{
		{0x40010005, _T("a Control-C")},
		{0x40010008, _T("a Control-Break")},
		{0x80000002, _T("a Datatype Misalignment")},
		{0x80000003, _T("a Breakpoint")},
		{0xC0000005, _T("an Access Violation")},
		{0xC0000006, _T("an In Page Error")},
		{0xC0000017, _T("a No Memory")},
		{0xC000001D, _T("an Illegal Instruction")},
		{0xC0000025, _T("a Noncontinuable Exception")},
		{0xC0000026, _T("an Invalid Disposition")},
		{0xC000008C, _T("a Array Bounds Exceeded")},
		{0xC000008D, _T("a Float Denormal Operand")},
		{0xC000008E, _T("a Float Divide by Zero")},
		{0xC000008F, _T("a Float Inexact Result")},
		{0xC0000090, _T("a Float Invalid Operation")},
		{0xC0000091, _T("a Float Overflow")},
		{0xC0000092, _T("a Float Stack Check")},
		{0xC0000093, _T("a Float Underflow")},
		{0xC0000094, _T("an Integer Divide by Zero")},
		{0xC0000095, _T("an Integer Overflow")},
		{0xC0000096, _T("a Privileged Instruction")},
		{0xC00000FD, _T("a Stack Overflow")},
		{0xC000013A, _T("a CTRL+C Exit")},
		{0xC0000142, _T("a DLL Initialization Failure")},
		{0xC000014C, _T("a Corrupt Registry")},
		{0xC00002B4, _T("Multiple Floating Point Faults")},
		{0xC00002B5, _T("Multiple Floating Point Traps")},
		{0xC00002C0, _T("an Illegal VLM Reference")},
		{0xC0000374, _T("a Corrupt Heap")},
		{0xC0000409, _T("a Stack Buffer Overrun")},
		{0xC0000417, _T("a CRT Invalid Parameter Exception")},
		{0xC0009898, _T("a WOW Assertion Error")},
		{0xE06D7363, _T("a Microsoft C++ Exception")},
	};

	for (int i = 0; i < _countof(ExceptionMap); i++)
		if (ExceptionCode == ExceptionMap[i].ExceptionCode)
			return ExceptionMap[i].ExceptionName;

	return _T("an Unknown exception type");
}

///////////////////////////////////////////////////////////////////////////////
// GetFilePart
static TCHAR * GetFilePart(LPCTSTR source)
{
	TCHAR *result = lstrrchr(source, _T('\\'));
	if (result)
		result++;
	else
		result = (TCHAR *)source;
	return result;
}

///////////////////////////////////////////////////////////////////////////////
// DumpEnvironmentVariables
static void DumpEnvironmentVariables(HANDLE LogFile)
{
	__try
	{
		LPTCH buf = ::GetEnvironmentStrings();
		if (buf)
		{
			hprintf(LogFile, _T("Environment Variables:\r\n"));
			TCHAR *envvar = (TCHAR *)buf;
			int n = 0;
			while (*envvar)
			{
				if (envvar[0] != _T('='))
				{
					hprintf(LogFile, _T("%5d:  <%s>\n"), n, envvar);
				}
				envvar += lstrlen(envvar) + 1;
				n++;
			}
			::FreeEnvironmentStrings(buf);
			hprintf(LogFile, _T("\r\n\r\n"));
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		hprintf(LogFile, _T("Exception encountered during environment variable dump.\r\n"));
	}
}

///////////////////////////////////////////////////////////////////////////////
// DumpStack86
#ifndef _WIN64
static void DumpStack86(HANDLE LogFile, DWORD *pStack)
{
	hprintf(LogFile, _T("\r\n\r\nStack:\r\n"));

	__try
	{
		// http://en.wikipedia.org/wiki/Win32_Thread_Information_Block
		// get top of stack
		void *pStackTop = (void *) __readfsdword(0x04);

		if (pStackTop > pStack + MaxStackDump)
			pStackTop = pStack + MaxStackDump;

		int nDwordsPrinted = 0;
		DWORD* pStackStart = pStack;

		while ((pStack + 1) <= pStackTop)
		{
			if (nDwordsPrinted == 0)
			{
				pStackStart = pStack;
				hprintf(LogFile, _T("0x%08x: "), pStack);
			}

			hprintf(LogFile, _T("%08x "), *pStack++);
			nDwordsPrinted++;

			if ((nDwordsPrinted >= StackColumns) || ((pStack + 1) > pStackTop))
			{
				if (nDwordsPrinted < StackColumns)
				{
					// not enough columns, need to pad
					int n = StackColumns - nDwordsPrinted;
					for (int i = 0; i < n; i++)
						hprintf(LogFile, _T("         "));
				}

				// output ASCII
				for (int i = 0; i < nDwordsPrinted; i++)
				{
					DWORD dwStack = *pStackStart++;
					for (int j = 0; j < sizeof(dwStack); j++)	// 4 bytes per DWORD
					{
						char c = (char)(dwStack & 0xFF);
						if (c < 0x20 || c > 0x7E)
							c = '.';
						hprintf(LogFile, _T("%hc"), c);
						dwStack = dwStack >> 8;
					}
				}

				hprintf(LogFile, _T("\r\n"));
				nDwordsPrinted = 0;
			}
		}
		hprintf(LogFile, _T("\r\n"));
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		hprintf(LogFile, _T("Exception encountered during stack dump.\r\n"));
	}
	hprintf(LogFile, _T("\r\n"));
}
#endif

///////////////////////////////////////////////////////////////////////////////
// DumpStack64
#ifdef _WIN64
static void DumpStack64(HANDLE LogFile, DWORD64 *pStack)
{
	hprintf(LogFile, _T("\r\n\r\nStack:\r\n"));

	__try
	{
		// get top of stack
		DWORD64 *pStackTop = (DWORD64 *) __readgsqword(8);

		//hprintf(LogFile, _T("pStackTop=%016I64x  +MaxStackDump=%016I64x   pStack=%016I64x\r\n\r\n"), pStackTop, pStack+MaxStackDump, pStack);

		if (pStackTop > pStack + MaxStackDump)
			pStackTop = pStack + MaxStackDump;

		//hprintf(LogFile, _T("pStackTop=%016I64x \r\n\r\n"), pStackTop);

		int nDwordsPrinted = 0;
		DWORD64 *pStackStart = pStack;

		while ((pStack + 1) <= pStackTop)
		{
			if (nDwordsPrinted == 0)
			{
				// output address column
				pStackStart = pStack;
				hprintf(LogFile, _T("0x%016I64x: "), pStack);
			}

			hprintf(LogFile, _T("%016I64x "), *pStack++);
			nDwordsPrinted++;

			if ((nDwordsPrinted >= StackColumns) || ((pStack + 1) > pStackTop))
			{
				if (nDwordsPrinted < StackColumns)
				{
					// not enough columns, need to pad
					int n = StackColumns - nDwordsPrinted;
					for (int i = 0; i < n; i++)
						hprintf(LogFile, _T("                 "));
				}

				// output ASCII
				for (int i = 0; i < nDwordsPrinted; i++)
				{
					DWORD64 dwStack = *pStackStart++;
					for (int j = 0; j < sizeof(dwStack); j++)		// 8 bytes per DWORD64
					{
						char c = (char)(dwStack & 0xFF);
						if (c < 0x20 || c > 0x7E)
							c = '.';
						hprintf(LogFile, _T("%hc"), c);
						dwStack = dwStack >> 8;
					}
				}

				hprintf(LogFile, _T("\r\n"));
				nDwordsPrinted = 0;
			}
		} // while
		hprintf(LogFile, _T("\r\n"));
	}
	__except(EXCEPTION_EXECUTE_HANDLER)
	{
		hprintf(LogFile, _T("Exception encountered during stack dump.\r\n"));
	}
	hprintf(LogFile, _T("\r\n"));
}
#endif

///////////////////////////////////////////////////////////////////////////////
// DumpCompilerOptions
static void DumpCompilerOptions(HANDLE LogFile)
{
	hprintf(LogFile, _T("\r\nCompiler Options:\r\n"));

#ifdef _MSC_VER
	hprintf(LogFile, _T("_MSC_VER=%d  "), _MSC_VER);
#endif	

#ifdef _MFC_VER
	hprintf(LogFile, _T("_MFC_VER=0x%X  "), _MFC_VER);
#endif	
	
#ifdef WINVER
	hprintf(LogFile, _T("WINVER=0x%X  "), WINVER);
#endif	
	
#ifdef _WIN32_WINNT
	hprintf(LogFile, _T("_WIN32_WINNT=0x%X  "), _WIN32_WINNT);
#endif	

#ifdef _WIN32_WINDOWS
	hprintf(LogFile, _T("_WIN32_WINDOWS=0x%X  "), _WIN32_WINDOWS);
#endif	
	hprintf(LogFile, _T("\r\n"));

#ifdef NTDDI_VERSION
	hprintf(LogFile, _T("NTDDI_VERSION=0x%X  "), NTDDI_VERSION);
#endif	

#ifdef _WIN32_IE
	hprintf(LogFile, _T("_WIN32_IE=0x%X  "), _WIN32_IE);
#endif	

#if defined(_UNICODE) || defined(UNICODE)
#if defined(UNICODE)
	hprintf(LogFile, _T("UNICODE  "));
#endif
#if defined (_UNICODE)
	hprintf(LogFile, _T("_UNICODE  "));
#endif
#else
	hprintf(LogFile, _T("ANSI  "));
#endif

#ifdef _M_AMD64
	hprintf(LogFile, _T("_M_AMD64  "));
#endif

#ifdef _AMD64_
	hprintf(LogFile, _T("_AMD64_  "));
#endif

#ifdef _WIN64
	hprintf(LogFile, _T("_WIN64  "));
#endif

#ifdef _DEBUG
	hprintf(LogFile, _T("Debug  "));
#else
	hprintf(LogFile, _T("Release  "));
#endif
	hprintf(LogFile, _T("\r\n"));
}

///////////////////////////////////////////////////////////////////////////////
// DumpRegisters
static void DumpRegisters(HANDLE LogFile, PCONTEXT Context)
{
	// Print out the register values in an XP error window compatible format.
#ifdef _WIN64
	hprintf(LogFile, _T("\r\n"));
	hprintf(LogFile, _T("Context:\r\n"));
	hprintf(LogFile, _T("RDI:   0x%016I64x    RSI:    0x%016I64x\r\n"),
		Context->Rdi, Context->Rsi);
	hprintf(LogFile, _T("RAX:   0x%016I64x    RBX:    0x%016I64x\r\n"),
		Context->Rax, Context->Rbx);
	hprintf(LogFile, _T("RCX:   0x%016I64x    RDX:    0x%016I64x\r\n"),
		Context->Rcx, Context->Rdx);
	hprintf(LogFile, _T("RIP:   0x%016I64x    RBP:    0x%016I64x  \r\n"),
		Context->Rip, Context->Rbp);
	hprintf(LogFile, _T("R8:    0x%016I64x    R9:     0x%016I64x  \r\n"),
		Context->R8, Context->R9);
	hprintf(LogFile, _T("R10:   0x%016I64x    R11:    0x%016I64x  \r\n"),
		Context->R10, Context->R11);
	hprintf(LogFile, _T("R12:   0x%016I64x    R13:    0x%016I64x  \r\n"),
		Context->R12, Context->R13);
	hprintf(LogFile, _T("R14:   0x%016I64x    R15:    0x%016I64x  \r\n"),
		Context->R14, Context->R15);
	hprintf(LogFile, _T("RSP:   0x%016I64x    EFlags: 0x%08x\r\n"),
		Context->Rsp, Context->EFlags);
	hprintf(LogFile, _T("SegCs: 0x%04x    SegDs: 0x%04x    SegEs: 0x%04x\r\n"),
		Context->SegCs, Context->SegDs, Context->SegEs);
	hprintf(LogFile, _T("SegFs: 0x%04x    SegGs: 0x%04x    SegSs: 0x%04x\r\n"),
		Context->SegFs, Context->SegGs, Context->SegSs);

#else

	hprintf(LogFile, _T("\r\n"));
	hprintf(LogFile, _T("Context:\r\n"));
	hprintf(LogFile, _T("EDI:    0x%08x   ESI: 0x%08x   EAX:   0x%08x\r\n"),
		Context->Edi, Context->Esi, Context->Eax);
	hprintf(LogFile, _T("EBX:    0x%08x   ECX: 0x%08x   EDX:   0x%08x\r\n"),
		Context->Ebx, Context->Ecx, Context->Edx);
	hprintf(LogFile, _T("EIP:    0x%08x   EBP: 0x%08x   SegCs: 0x%08x\r\n"),
		Context->Eip, Context->Ebp, Context->SegCs);
	hprintf(LogFile, _T("EFlags: 0x%08x   ESP: 0x%08x   SegSs: 0x%08x\r\n"),
		Context->EFlags, Context->Esp, Context->SegSs);
#endif
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//
// RecordExceptionInfo
//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

LONG __stdcall RecordExceptionInfo(PEXCEPTION_POINTERS pExceptPtrs) 
{
	OutputDebugString(_T("in RecordExceptionInfo\r\n"));

	static bool bFirstTime = true;
	if (!bFirstTime)	// Going recursive! That must mean this routine crashed!
		return EXCEPTION_CONTINUE_SEARCH;
	//bFirstTime = false;

	// Create a filename to record the error information to.
	// We use common documents folder to avoid UAC problems.

	char strTime[TIME_FORMAT_LENGTH] = "\0";
	GetTimeString(strTime, TIME_FORMAT2);

	TCHAR szPath[MAX_PATH*2];
	ZeroMemory(szPath, sizeof(szPath));
	//if (SHGetSpecialFolderPath(0, szPath, CSIDL_PERSONAL, TRUE))
	//{
	//	OutputDebugString(szPath);
	//	OutputDebugString(_T("\r\n"));
	//	int len = lstrlen(szPath);
	//	if (szPath[len-1] != _T('\\'))
	//		szPath[len] = _T('\\');
	//}
	//string curPath = GetCurrWorkingDir();
	char* curPath = 0;
	ZGF::FileSystemUtil::GetCurrWorkingDir(&curPath);
	memcpy(szPath, curPath, strlen(curPath));
	delete[] curPath;
	lstrcat(szPath, "Dump\\");
	{
		CreateDirectory(szPath, NULL);
	}
	//NOTE_LOG("DUMP PATH : %s",
	//	szPath);
	char* pProcessName = NULL;
	ZGF::FileSystemUtil::GetCurrProcessName(&pProcessName);
	lstrcat(szPath, pProcessName);
	delete[] pProcessName;
	// take screenshots of all monitors
#ifdef XCRASHREPORT_WRITE_SCREENSHOT
	//CScreenShot ss(XCRASHREPORT_SCREENSHOT_RESOLUTION);
	//ss.WriteScreenShot(szPath, XCRASHREPORT_BMP_FILE);
	//ss.Destroy();
#endif

	TCHAR szModuleName[MAX_PATH*2];
	ZeroMemory(szModuleName, sizeof(szModuleName));
	if (GetModuleFileName(0, szModuleName, _countof(szModuleName)-2) <= 0)
		lstrcpy(szModuleName, _T("Unknown"));

	TCHAR *pszFilePart = GetFilePart(szModuleName);

	// Extract the file name portion and remove its file extension
	TCHAR szFileName[MAX_PATH*2];
	lstrcpy(szFileName, pszFilePart);
	TCHAR *lastperiod = lstrrchr(szFileName, _T('.'));
	if (lastperiod)
		*lastperiod = 0;

	// Replace the executable filename with our error log file name
	//lstrcpy(pszFilePart, XCRASHREPORT_ERROR_LOG_FILE);
	TCHAR szErrorLog[MAX_PATH*2];
	lstrcpy(szErrorLog, szPath);
	lstrcat(szErrorLog, strTime);
	lstrcat(szErrorLog, XCRASHREPORT_ERROR_LOG_FILE);
	HANDLE hLogFile = CreateFile(szErrorLog, GENERIC_WRITE, 0, 0,
				CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, 0);

	if (hLogFile == INVALID_HANDLE_VALUE)
	{
		OutputDebugString(_T("Error creating exception report\r\n"));
		return EXCEPTION_CONTINUE_SEARCH;
	}

#ifdef _UNICODE
	// write BOM
	DWORD dwBytesWritten = 0;
	TCHAR BOM = 0xFEFF;
	WriteFile(hLogFile, &BOM, 2, &dwBytesWritten, 0);
#endif

	PEXCEPTION_RECORD Exception = pExceptPtrs->ExceptionRecord;
	PCONTEXT          Context   = pExceptPtrs->ContextRecord;

	TCHAR szCrashModulePathName[MAX_PATH*3];
	ZeroMemory(szCrashModulePathName, sizeof(szCrashModulePathName));

	TCHAR *pszCrashModuleFileName = _T("Unknown");

	MEMORY_BASIC_INFORMATION MemInfo;

	// VirtualQuery can be used to get the allocation base associated with a
	// code address, which is the same as the ModuleHandle. This can be used
	// to get the filename of the module that the crash happened in.
#ifdef _WIN64
	if (VirtualQuery((void*)Context->Rip, &MemInfo, sizeof(MemInfo)) &&
		(GetModuleFileName((HINSTANCE)MemInfo.AllocationBase,
		szCrashModulePathName,
		_countof(szCrashModulePathName)-2) > 0))
#else
		if (VirtualQuery((void*)Context->Eip, &MemInfo, sizeof(MemInfo)) &&
			(GetModuleFileName((HINSTANCE)MemInfo.AllocationBase,
			szCrashModulePathName,
			_countof(szCrashModulePathName)-2) > 0))
#endif
	{
		pszCrashModuleFileName = GetFilePart(szCrashModulePathName);
	}

	// Print out the beginning of the error log in a Win95 error window
	// compatible format.
	hprintf(hLogFile, _T("%s caused %s (0x%08x) \r\nin module %s at %04x:%08x.\r\n\r\n"),
				szFileName, GetExceptionDescription(Exception->ExceptionCode),
				Exception->ExceptionCode,
				pszCrashModuleFileName, Context->SegCs, 
#ifdef _WIN64				
				Context->Rip);
#else
				Context->Eip);
#endif

	DumpSystemInformation(hLogFile);

	// If the exception was an access violation, print out some additional
	// information, to the error log and the debugger.
	if ((Exception->ExceptionCode == STATUS_ACCESS_VIOLATION) &&
		(Exception->NumberParameters >= 2))
	{
		TCHAR szDebugMessage[1000];
		const TCHAR* readwrite = _T("Read from");
		if (Exception->ExceptionInformation[0])
			readwrite = _T("Write to");
		wsprintf(szDebugMessage, _T("\r\n%s location %08x caused an access violation.\r\n"),
					readwrite, Exception->ExceptionInformation[1]);

#ifdef	_DEBUG
		// The Visual C++ debugger doesn't actually tell you whether a read
		// or a write caused the access violation, nor does it tell what
		// address was being read or written. So I fixed that.
		OutputDebugString(_T("Exception handler: "));
		OutputDebugString(szDebugMessage);
#endif

		hprintf(hLogFile, _T("%s"), szDebugMessage);
	}

	DumpCompilerOptions(hLogFile);

	DumpRegisters(hLogFile, Context);

	// Print out the bytes of code at the instruction pointer. Since the
	// crash may have been caused by an instruction pointer that was bad,
	// this code needs to be wrapped in an exception handler, in case there
	// is no memory to read. If the dereferencing of code[] fails, the
	// exception handler will print '??'.

#ifdef _WIN64
	hprintf(hLogFile, _T("\r\nBytes at CS:RIP:\r\n"));
	BYTE * code = (BYTE *)Context->Rip;
#else
	hprintf(hLogFile, _T("\r\nBytes at CS:EIP:\r\n"));
	BYTE * code = (BYTE *)Context->Eip;
#endif

	for (int codebyte = 0; codebyte < NumCodeBytes; codebyte++)
	{
		__try
		{
			hprintf(hLogFile, _T("%02x "), code[codebyte]);

		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			hprintf(hLogFile, _T("?? "));
		}
	}

	// Time to print part or all of the stack to the error log. This allows
	// us to figure out the call stack, parameters, local variables, etc.

	// Esp contains the bottom of the stack, or at least the bottom of
	// the currently used area
#ifdef _WIN64
	DWORD64 *pStack = (DWORD64 *)Context->Rsp;
#else
	DWORD* pStack = (DWORD *)Context->Esp;
#endif

#ifdef _WIN64
	DumpStack64(hLogFile, pStack);
#else
	DumpStack86(hLogFile, pStack);
#endif

	DumpEnvironmentVariables(hLogFile);

	DumpModuleList(hLogFile);

	hprintf(hLogFile, _T("\r\n===== [end of %s] =====\r\n"), 
		XCRASHREPORT_ERROR_LOG_FILE);
	hflush(hLogFile);
	CloseHandle(hLogFile);

	///////////////////////////////////////////////////////////////////////////
	//
	// write minidump
	//
	///////////////////////////////////////////////////////////////////////////

#ifdef XCRASHREPORT_WRITE_MINIDUMP

	// Replace the filename with our minidump file name
	lstrcat(szPath, strTime);
	lstrcat(szPath, XCRASHREPORT_MINI_DUMP_FILE);

	// Create the file
	HANDLE hMiniDumpFile = CreateFile(
								szPath,
								GENERIC_WRITE,
								0,
								NULL,
								CREATE_ALWAYS,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
								NULL);

	// Write the minidump to the file
	if (hMiniDumpFile != INVALID_HANDLE_VALUE)
	{
		DumpMiniDump(hMiniDumpFile, pExceptPtrs);

		// Close file
		CloseHandle(hMiniDumpFile);
	}

#endif	// XCRASHREPORT_WRITE_MINIDUMP

	if (IsDebuggerPresent())
	{
		// let the debugger catch this -
		// return the magic value which tells Win32 that this handler didn't
		// actually handle the exception - so that things will proceed as per
		// normal.
		return EXCEPTION_CONTINUE_SEARCH;
	}
	else
	{
		///////////////////////////////////////////////////////////////////////
		//
		//  pop up our crash report app
		//
		///////////////////////////////////////////////////////////////////////

		// This turns off the standard crash message box - may be necessary if
		// you are using 3rd-party libraries
		//SetErrorMode(SEM_NOGPFAULTERRORBOX);

		// Replace the filename with our crash report exe file name
		lstrcpy(pszFilePart, XCRASHREPORT_CRASH_REPORT_APP);

		TCHAR szCommandLine[MAX_PATH*2];
		lstrcpy(szCommandLine, szModuleName);

		lstrcat(szCommandLine, _T(" \""));	// surround app name with quotes
		ZeroMemory(szModuleName, sizeof(szModuleName));
		GetModuleFileName(0, szModuleName, _countof(szModuleName)-2);
		lstrcat(szCommandLine, 	GetFilePart(szModuleName));
		lstrcat(szCommandLine, _T("\""));

		OutputDebugString(szCommandLine);
		OutputDebugString(_T("\r\n"));

		STARTUPINFO si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESHOWWINDOW;
		si.wShowWindow = SW_SHOW;

		PROCESS_INFORMATION pi;
		ZeroMemory(&pi, sizeof(pi));

		if (CreateProcess(
					NULL,					// name of executable module
					szCommandLine,			// command line string
					NULL,					// process attributes
					NULL,					// thread attributes
					FALSE,					// handle inheritance option
					0,						// creation flags
					NULL,					// new environment block
					NULL,					// current directory name
					&si,					// startup information
					&pi))					// process information
		{
			// XCrashReport.exe was successfully started, so
			// suppress the standard crash dialog
			return EXCEPTION_EXECUTE_HANDLER;
		}
		else
		{
			ExitProcess(0);
			// XCrashReport.exe was not started - let
			// the standard crash dialog appear
			return EXCEPTION_CONTINUE_SEARCH;
		}
	}
}

//
// Dummy exception filter to catch CRT-forced WER exceptions
LPTOP_LEVEL_EXCEPTION_FILTER WINAPI 
	MyDummySetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER)
{
	OutputDebugString(_T("in MyDummySetUnhandledExceptionFilter\r\n"));
	return NULL;
}

// 
// The following code hooks SetUnhandledExceptionFilter() to prevent the CRT 
// from forcing a call to WER (Windows Error Reporting), which will bypass
// calling our RecordExceptionInfo().  For example, if no invalid parameter 
// handler is defined (which is the default) and an invalid parameter is 
// detected.
//
static BOOL PreventSetUnhandledExceptionFilter()
{
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	if (hKernel32 == NULL) 
		return FALSE;
	void *pOrgEntry = GetProcAddress(hKernel32, "SetUnhandledExceptionFilter");
	if (pOrgEntry == NULL) 
		return FALSE;

	DWORD dwOldProtect = 0;
	SIZE_T jmpSize = 5;
#ifdef _WIN64
	jmpSize = 13;
#endif
	BOOL bProt = VirtualProtect(pOrgEntry, jmpSize, 
		PAGE_EXECUTE_READWRITE, &dwOldProtect);
	BYTE newJump[20];
	void *pNewFunc = &MyDummySetUnhandledExceptionFilter;
#ifdef _WIN64
	// We must use R10 or R11, because these are "scratch" registers 
	// which need not to be preserved across function calls
	// For more info see: Register Usage for x64 64-Bit
	// http://msdn.microsoft.com/en-us/library/ms794547.aspx
	newJump[0] = 0x49;  // MOV R11, ...
	newJump[1] = 0xBB;  // ...
	memcpy(&newJump[2], &pNewFunc, sizeof(pNewFunc));
	newJump[10] = 0x41;  // JMP R11, ...
	newJump[11] = 0xFF;  // ...
	newJump[12] = 0xE3;  // ...
#else
	DWORD dwOrgEntryAddr = (DWORD) pOrgEntry;
	dwOrgEntryAddr += jmpSize; // add 5 for 5 op-codes for jmp rel32
	DWORD dwNewEntryAddr = (DWORD) pNewFunc;
	DWORD dwRelativeAddr = dwNewEntryAddr - dwOrgEntryAddr;
	// JMP rel32: Jump near, relative, displacement relative to next instruction.
	newJump[0] = 0xE9;  // JMP rel32
	memcpy(&newJump[1], &dwRelativeAddr, sizeof(pNewFunc));
#endif
	SIZE_T bytesWritten;
	BOOL bRet = WriteProcessMemory(GetCurrentProcess(),
		pOrgEntry, newJump, jmpSize, &bytesWritten);

	if (bProt)
	{
		DWORD dwBuf;
		VirtualProtect(pOrgEntry, jmpSize, dwOldProtect, &dwBuf);
	}
	return bRet;
}

void RecordDump::Record(PEXCEPTION_POINTERS data)
{
	TCHAR szPath[MAX_PATH*2];
	ZeroMemory(szPath, sizeof(szPath));
	char* curPath = 0;
	ZGF::FileSystemUtil::GetCurrWorkingDir(&curPath);
	memcpy(szPath, curPath, strlen(curPath));
	delete[] curPath;
	lstrcat(szPath, "Dump\\");

	//if(PathIsDirectory(szPath) == FALSE)
	{
		CreateDirectory(szPath, NULL);
	}
	
	char* pProcessName = NULL;
	ZGF::FileSystemUtil::GetCurrProcessName(&pProcessName);
	lstrcat(szPath, pProcessName);
	delete[] pProcessName;

	char strTime[TIME_FORMAT_LENGTH] = "\0";
	GetTimeString(strTime, TIME_FORMAT2);
	lstrcat(szPath, strTime);
	// Replace the filename with our minidump file name
	lstrcat(szPath, "Crash.DMP");

	//NOTE_LOG("%s", szPath);

	// Create the file
	HANDLE hMiniDumpFile = CreateFile(
		szPath,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH,
		NULL);

	// Write the minidump to the file
	if (hMiniDumpFile != INVALID_HANDLE_VALUE)
	{
		DumpMiniDump(hMiniDumpFile, data);

		// Close file
		CloseHandle(hMiniDumpFile);
	}
	//RecordExceptionInfo(data);
}

///////////////////////////////////////////////////////////////////////////////
// This class initializes the unhandled exception handler
///////////////////////////////////////////////////////////////////////////////

class CXCrashHandler
{
public:
	CXCrashHandler()
	{
		OutputDebugString(_T("in CXCrashHandler\r\n"));
		m_pOldExceptionFilter = ::SetUnhandledExceptionFilter(RecordExceptionInfo);
		BOOL bRet = PreventSetUnhandledExceptionFilter();
		if (bRet)
		{
			OutputDebugString(_T("PreventSetUnhandledExceptionFilter succeeded\r\n"));
		}
	}

	~CXCrashHandler()
	{
		OutputDebugString(_T("in ~CXCrashHandler\r\n"));
		::SetUnhandledExceptionFilter(m_pOldExceptionFilter);
	}

private:
	LPTOP_LEVEL_EXCEPTION_FILTER m_pOldExceptionFilter;
};

// set exception handler before any other objects constructed
#pragma warning(disable : 4073)	// initializers put in library initialization area
#pragma init_seg(lib)
static CXCrashHandler g_XCrashHandler;

