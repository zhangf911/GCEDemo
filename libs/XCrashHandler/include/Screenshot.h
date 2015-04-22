// Screenshot.h Version 1.4
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

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

class CScreenShot
{
public:
	CScreenShot(int nResolution);
	// we can't use dtor because of SEH
	void Destroy()
	{
		for (DWORD i = 0; i < m_dwNumMonitors; i++)
		{
			CBitmapInfo *pBitmapInfo = &m_arrBitmaps[i];
			if (pBitmapInfo)
				pBitmapInfo->Destroy();
		}

		delete [] m_arrBitmaps;
		m_arrBitmaps = 0;
		m_dwNumMonitors = 0;
	}
	BOOL WriteScreenShot(LPCTSTR lpszFolder, LPCTSTR lpszFilePrefix);

private:
	// Protects the class from being accidentally copied.
	CScreenShot(const CScreenShot& rScreenShot);
	// Protects the class from being accidentally copied.
	CScreenShot& operator=(const CScreenShot& rScreenShot);

	// Per-monitor bitmap information
	struct CBitmapInfo
	{
	public:
		// Initialize an object.
		CBitmapInfo()
		{
			m_pBmpInfo = 0;
			m_pBitsArray = 0;
			m_dwBmpHdrSize = 0;
			m_dwBitsArraySize = 0;
		}
		void Destroy()
		{
			delete [] m_pBmpInfo;
			delete [] m_pBitsArray;
			m_pBmpInfo = 0;
			m_pBitsArray = 0;
			m_dwBmpHdrSize = 0;
			m_dwBitsArraySize = 0;
		}

		PBITMAPINFO m_pBmpInfo;		// Bitmap information
		PBYTE m_pBitsArray;			// Bitmap bits
		DWORD m_dwBmpHdrSize;		// Bitmap header size
		DWORD m_dwBitsArraySize;	// Image size
	};

	CBitmapInfo *	m_arrBitmaps;
	DWORD			m_dwNumMonitors;
};

#endif // SCREENSHOT_H
