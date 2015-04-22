// Screenshot.cpp Version 1.4
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

#include <windows.h>
#include <tchar.h>
#include "Screenshot.h"

#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof(array[0]))
#endif

//=============================================================================
CScreenShot::CScreenShot(int nResolution)
//=============================================================================
{
	// get number of monitors
	m_dwNumMonitors = 0;
	DWORD dwNumMonitors = GetSystemMetrics(SM_CMONITORS);
	m_arrBitmaps = new CBitmapInfo [dwNumMonitors];
	if (m_arrBitmaps == NULL)
		return;

	DWORD dwMonitorNumber, dwDeviceNumber;
	for (dwMonitorNumber = dwDeviceNumber = 0; 
		 dwMonitorNumber < dwNumMonitors; 
		 dwDeviceNumber++)
	{
		DISPLAY_DEVICE DisplayDevice;
		DisplayDevice.cb = sizeof(DisplayDevice);
		if (!EnumDisplayDevices(NULL, dwDeviceNumber, &DisplayDevice, 0))
			break;
		if ((DisplayDevice.StateFlags & DISPLAY_DEVICE_ATTACHED_TO_DESKTOP) == 0)
			continue;
		BOOL bResult = FALSE;
		HDC hDisplayDC = CreateDC(DisplayDevice.DeviceName, NULL, NULL, NULL);
		if (hDisplayDC)
		{
			int nWidth = GetDeviceCaps(hDisplayDC, HORZRES);
			int nHeight = GetDeviceCaps(hDisplayDC, VERTRES);
			HBITMAP hBitmap = CreateCompatibleBitmap(hDisplayDC, nWidth, nHeight);
			if (hBitmap)
			{
				HDC hMemDC = CreateCompatibleDC(hDisplayDC);
				if (hMemDC)
				{
					CBitmapInfo *pBitmapInfo = &m_arrBitmaps[dwMonitorNumber];

					HBITMAP hOldBitmap = (HBITMAP) SelectObject(hMemDC, hBitmap);
					BitBlt(hMemDC, 0, 0, nWidth, nHeight, hDisplayDC, 0, 0, SRCCOPY);

					BITMAP bmpInfo;
					GetObject(hBitmap, sizeof(bmpInfo), &bmpInfo);
					WORD wPalSize, wBmpBits = bmpInfo.bmPlanes * bmpInfo.bmBitsPixel;
					if (wBmpBits <= 1)
					{
						wBmpBits = 1;  // monochrome image
						wPalSize = 2;
					}
					else if ((wBmpBits <= 4) || (nResolution == 1))	// low
					{
						wBmpBits = 4;  // palette-based 4 bpp image
						wPalSize = 16;
					}
					else if ((wBmpBits <= 8) || (nResolution == 2))	// medium
					{
						wBmpBits = 8;  // palette-based 8 bpp image
						wPalSize = 256;
					}
					else											// high
					{
						wBmpBits = 16; // force to 16 bpp image (don't allow 24 bpp)
						wPalSize = 0;  // don't use palette
					}

					pBitmapInfo->m_dwBmpHdrSize = 
						sizeof(BITMAPINFOHEADER) + wPalSize * sizeof(RGBQUAD);
					pBitmapInfo->m_pBmpInfo = 
						(PBITMAPINFO) new BYTE [pBitmapInfo->m_dwBmpHdrSize];

					if (pBitmapInfo->m_pBmpInfo)
					{
						ZeroMemory(pBitmapInfo->m_pBmpInfo, pBitmapInfo->m_dwBmpHdrSize);
						BITMAPINFOHEADER& bmpHdr = pBitmapInfo->m_pBmpInfo->bmiHeader;
						bmpHdr.biSize        = sizeof(bmpHdr);
						bmpHdr.biWidth       = nWidth;
						bmpHdr.biHeight      = nHeight;
						bmpHdr.biPlanes      = 1;
						bmpHdr.biBitCount    = wBmpBits;
						bmpHdr.biCompression = BI_RGB;

						// call GetDIBits with a NULL bits array, so it will 
						// calculate the biSizeImage field
						GetDIBits(hMemDC, hBitmap, 0, nHeight, NULL, 
							pBitmapInfo->m_pBmpInfo, DIB_RGB_COLORS);
						if (bmpHdr.biSizeImage == 0)
						{
							bmpHdr.biSizeImage = 
								(wBmpBits * nWidth + 31) / 32 * 4 * nHeight;
						}
						pBitmapInfo->m_dwBitsArraySize = bmpHdr.biSizeImage;
						pBitmapInfo->m_pBitsArray = 
							new BYTE [pBitmapInfo->m_dwBitsArraySize];

						if (pBitmapInfo->m_pBitsArray)
						{
							if (GetDIBits(hMemDC, hBitmap, 0, nHeight, 
									pBitmapInfo->m_pBitsArray, 
									pBitmapInfo->m_pBmpInfo, DIB_RGB_COLORS))
									bResult = TRUE;
						}
					}

					if (bResult)
						dwMonitorNumber++;

					SelectObject(hMemDC, hOldBitmap);
					DeleteDC(hMemDC);
				}
				DeleteObject(hBitmap);
			}
			DeleteDC(hDisplayDC);
		}
	}

	m_dwNumMonitors = dwMonitorNumber;
}

//=============================================================================
BOOL CScreenShot::WriteScreenShot(LPCTSTR lpszFolder, LPCTSTR lpszFilePrefix)
//=============================================================================
{
	for (DWORD dwMonitorNumber = 0; dwMonitorNumber < m_dwNumMonitors; dwMonitorNumber++)
	{
		CBitmapInfo* pBitmapInfo = m_arrBitmaps + dwMonitorNumber;

		if ((pBitmapInfo == NULL) || (pBitmapInfo->m_pBmpInfo == NULL))
			return FALSE;

		TCHAR szFileName[MAX_PATH*2];
		ZeroMemory(szFileName, sizeof(szFileName));
		wsprintf(szFileName, _T("%s%s%d.bmp"), lpszFolder, lpszFilePrefix, dwMonitorNumber + 1);

		HANDLE hFile = CreateFile(szFileName, GENERIC_WRITE, 0, NULL, 
							CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return FALSE;

		BITMAPFILEHEADER bmpfh;
		ZeroMemory(&bmpfh, sizeof(bmpfh));
		bmpfh.bfType = 'MB';
		bmpfh.bfOffBits = sizeof(bmpfh) + pBitmapInfo->m_dwBmpHdrSize;
		bmpfh.bfSize = bmpfh.bfOffBits + pBitmapInfo->m_dwBitsArraySize;

		DWORD dwBytesWritten = 0;
		WriteFile(hFile, &bmpfh, sizeof(bmpfh), &dwBytesWritten, NULL);
		WriteFile(hFile, pBitmapInfo->m_pBmpInfo, pBitmapInfo->m_dwBmpHdrSize, 
			&dwBytesWritten, NULL);
		WriteFile(hFile, pBitmapInfo->m_pBitsArray, pBitmapInfo->m_dwBitsArraySize, 
			&dwBytesWritten, NULL);
		CloseHandle(hFile);
	}

	return TRUE;
}
