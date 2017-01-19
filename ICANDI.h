// ICANDI.h : main header file for the ICANDI application
//

#if !defined(AFX_ICANDI_H__0C8B3FD1_B41F_45A6_93C1_6A61BB89620B__INCLUDED_)
#define AFX_ICANDI_H__0C8B3FD1_B41F_45A6_93C1_6A61BB89620B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols
//typedef unsigned __int64 ULONG_PTR;//gdi+ :-(
//#include "gdiplus.h"
//using namespace Gdiplus;

/////////////////////////////////////////////////////////////////////////////
// CICANDIApp:
// See ICANDI.cpp for the implementation of this class
//
int  g_CloseIPC();

class CICANDIApp : public CWinApp
{
public:
	CICANDIApp();

	long	m_imageSizeX;				// Buffer Size X
	long	m_imageSizeY;				// Buffer Size Y
	BOOL 	m_isGrabStarted;			// State of the grab
	BOOL    m_bInvalidDevice;

private:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CICANDIApp)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	//{{AFX_MSG(CICANDIApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICANDI_H__0C8B3FD1_B41F_45A6_93C1_6A61BB89620B__INCLUDED_)
