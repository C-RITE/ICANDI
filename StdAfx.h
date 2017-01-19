// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__550351AA_77C2_4DC8_826E_3DDD4BAEB805__INCLUDED_)
#define AFX_STDAFX_H__550351AA_77C2_4DC8_826E_3DDD4BAEB805__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define VC_EXTRALEAN		// Exclude rarely-used stuff from Windows headers

#ifndef WINVER					
#define WINVER 0x0501			
#endif

#include <afxwin.h>         // MFC core and standard components
#include <afxext.h>         // MFC extensions
#include <afxdisp.h>        // MFC Automation classes
#include <afxdtctl.h>		// MFC support for Internet Explorer 4 Common Controls
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>			// MFC support for Windows Common Controls
#include <afxmt.h>
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <Winsock2.h>
#include "utils/netcomm/WinSock2Async.h"
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__550351AA_77C2_4DC8_826E_3DDD4BAEB805__INCLUDED_)
