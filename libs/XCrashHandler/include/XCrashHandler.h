// XCrashHandler.h  Version 2.0.1
//
// Copyright ?1998 Bruce Dawson
//
// Author:       Bruce Dawson
//               brucedawson@cygnus-software.com
//
// Modified by:  Hans Dietrich
//               hdietrich@gmail.com
//
// A paper by the original author can be found at:
//     http://www.cygnus-software.com/papers/release_debugging.html
//
///////////////////////////////////////////////////////////////////////////////

#ifndef XCRASHHANDLER_H
#define XCRASHHANDLER_H

// We forward declare PEXCEPTION_POINTERS so that the function
// prototype doesn't needlessly require windows.h.

typedef struct _EXCEPTION_POINTERS EXCEPTION_POINTERS, *PEXCEPTION_POINTERS;

 //LONG __stdcall RecordExceptionInfo(PEXCEPTION_POINTERS data);

class RecordDump
{
public:
	static void Record(PEXCEPTION_POINTERS data);
};

#endif // XCRASHHANDLER_H
