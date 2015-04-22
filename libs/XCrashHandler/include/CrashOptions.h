// CrashOptions.h  Version 2.0.1
//
// Author:  Hans Dietrich
//          hdietrich@gmail.com
//
// License:
//     This file is Copyright ?2010 Hans Dietrich. All Rights Reserved.
//
//     This source file is the property of Hans Dietrich and is not to be
//     re-distributed by any means whatsoever without the expressed written
//     consent of Hans Dietrich.
//
//     This source code can only be used under the Terms of Use set forth
//     on the Hans Dietrich Software web site. Hans Dietrich Software grants 
//     to you (one software developer) the limited right to use this software.
//
//     This software is provided "as is" with no expressed or implied warranty.
//     Hans Dietrich accepts no liability for any damage or loss of business 
//     that this software may cause.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef CRASHOPTIONS_H
#define CRASHOPTIONS_H

///////////////////////////////////////////////////////////////////////////////
// SCREENSHOT
///////////////////////////////////////////////////////////////////////////////
// comment out this line if you don't want screenshots
#define XCRASHREPORT_WRITE_SCREENSHOT

// Up to 16 screenshots will be generated, one for each monitor on the user's 
// system.  You can reduce this to any number below 16.
#define XCRASHREPORT_SCREENSHOT_MAX_MONITORS		16

#define XCRASHREPORT_SCREENSHOT_RESOLUTION_LOW		1
#define XCRASHREPORT_SCREENSHOT_RESOLUTION_MEDIUM	2
#define XCRASHREPORT_SCREENSHOT_RESOLUTION_HIGH		3
// This controls how big the screenshot files are.  The files are BMP files,
// and so will be compressed substantially when zipped.
#define XCRASHREPORT_SCREENSHOT_RESOLUTION			XCRASHREPORT_SCREENSHOT_RESOLUTION_HIGH

///////////////////////////////////////////////////////////////////////////////
// MINIDUMP
///////////////////////////////////////////////////////////////////////////////
// comment out this line if you don't want minidumps
#define XCRASHREPORT_WRITE_MINIDUMP

///////////////////////////////////////////////////////////////////////////////
// FILE NAMES
///////////////////////////////////////////////////////////////////////////////
#define XCRASHREPORT_MINI_DUMP_FILE			_T("CRASH.DMP")
#define XCRASHREPORT_ERROR_LOG_FILE			_T("ERRORLOG.TXT")
#define XCRASHREPORT_REGISTRY_DUMP_FILE		_T("REGISTRY.TXT")
#define XCRASHREPORT_BMP_FILE				_T("XCRASHREPORT_Mon")
#ifdef _UNICODE
	#define XCRASHREPORT_CRASH_REPORT_APP	_T("XCrashReportRU.exe")
	#define XCRASHREPORT_BMPVIEWER_APP		_T("XBmpViewerRU.exe")
#else
	#define XCRASHREPORT_CRASH_REPORT_APP	_T("XCrashReportRA.exe")
	#define XCRASHREPORT_BMPVIEWER_APP		_T("XBmpViewerRA.exe")
#endif

///////////////////////////////////////////////////////////////////////////////
// INI FILE
///////////////////////////////////////////////////////////////////////////////
#define INI_FILE_NAME						_T("XCrashReport.ini")
#define INI_EMAIL_SECTION					_T("Email")
#define INI_EMAIL_SEND_METHOD				_T("SendMethod")
#define INI_EMAIL_SEND_TO_NAME				_T("SendToName")
#define INI_EMAIL_SEND_TO_ADDRESS			_T("SendToAddress")
#define INI_FILE_SECTION					_T("FilesToAdd")
#define INI_FILE_TEMPLATE					_T("File%03d")
#define INI_REG_SECTION						_T("RegistryToAdd")
#define INI_REG_TEMPLATE					_T("Registry%03d")
#define MAX_INI_ITEMS						999

#define INI_SMTP_NAME						_T("SmtpName")
#define INI_SMTP_EMAIL_ADDRESS				_T("SmtpEmailAddress")
#define INI_SMTP_USER_NAME					_T("SmtpUserName")
#define INI_SMTP_PASSWORD					_T("SmtpPassword")
#define INI_SMTP_SERVER						_T("SmtpServer")
#define INI_SMTP_PORT						_T("SmtpPort")
#define INI_SMTP_CONN_TYPE					_T("SmtpConnType")
#define INI_SMTP_AUTH_TYPE					_T("SmtpAuthType")
#define INI_SMTP_ENCODING					_T("SmtpEncoding")


///////////////////////////////////////////////////////////////////////////////
// SMTP - password decryption key
//
// The following GUID is used to decrypt the email address and password in 
// SMTP authentication.  It is strongly suggested that you replace this GUID 
// with one that you create yourself, to avoid any possible security breach.
// This key is used by XSmtpConfig.exe and CrashReport.exe.
///////////////////////////////////////////////////////////////////////////////

#define SMTP_PASSWORD_KEY	_T("35DCE3D2-23A1-41b9-A429-BFA03C3C3765")

///////////////////////////////////////////////////////////////////////////////
// EMAIL - name and address are specified in INI file
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
// REGISTRY - keys are specified in INI file
///////////////////////////////////////////////////////////////////////////////
// comment out this line if you don't want to export registry keys
#define XCRASHREPORT_DUMP_REGISTRY



#endif // CRASHOPTIONS_H
