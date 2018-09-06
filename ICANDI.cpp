// ICANDI.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "ICANDI.h"

#include "MainFrm.h"
#include "ICANDIDoc.h"
#include "ICANDIView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CVirtex5BMD       g_objVirtex5BMD;
extern WDC_DEVICE_HANDLE g_hDevVirtex5;
extern DIAG_DMA          g_dma;

// this routine is to handle the case with abnormal shutdown of the software
// where we have to turn off FPGA video sampling
LONG AppExceptionHandler(LPEXCEPTION_POINTERS p)
{
	g_objVirtex5BMD.AppStopADC(g_hDevVirtex5);
//	g_objVirtex5BMD.DIAG_DMAClose(g_hDevVirtex5, &g_dma);

    AfxMessageBox("Program is closed abnormally", MB_ICONSTOP);

    return EXCEPTION_EXECUTE_HANDLER;
}



/////////////////////////////////////////////////////////////////////////////
// CICANDIApp

BEGIN_MESSAGE_MAP(CICANDIApp, CWinApp)
	//{{AFX_MSG_MAP(CICANDIApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CICANDIApp construction

CICANDIApp::CICANDIApp()
{
	// TODO: add construction code here,


	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CICANDIApp object

CICANDIApp theApp;

/////////////////////////////////////////////////////////////////////////////
// CICANDIApp initialization

BOOL CICANDIApp::InitInstance()
{
    m_imageSizeX = 512;
	m_imageSizeY = 512;
	// Initialize the state of the grab
	m_isGrabStarted = FALSE;

	// open Virtex-5 device.
	DWORD   status, nCode;
	char   *msg;
	CString errMsg;

	msg = new char [256];
	g_hDevVirtex5 = NULL;
	m_bInvalidDevice = FALSE;

	status = g_objVirtex5BMD.VIRTEX5_LibInit();
	msg = g_objVirtex5BMD.GetErrMsg();
	if (status != WD_STATUS_SUCCESS) {
		errMsg.Format("Can't initialize device Xilinx ML-506. %s", msg);
		AfxMessageBox(errMsg, MB_OK);
		m_bInvalidDevice = TRUE;
		delete [] msg;
		return FALSE;
	}

	g_hDevVirtex5 = g_objVirtex5BMD.DeviceFindAndOpen(VIRTEX5_DEFAULT_VENDOR_ID, VIRTEX5_DEFAULT_DEVICE_ID);
	msg   = g_objVirtex5BMD.GetErrMsg();
	nCode = g_objVirtex5BMD.GetErrCode();
	if (nCode != WD_STATUS_SUCCESS) {
		errMsg.Format("Can't open device Xilinx ML-506. Error: %s", msg);
		AfxMessageBox(errMsg, MB_OK);
		m_bInvalidDevice = TRUE;

		if (g_hDevVirtex5)
			g_objVirtex5BMD.DeviceClose(g_hDevVirtex5, NULL);
 
		status = g_objVirtex5BMD.VIRTEX5_LibUninit();
		if (WD_STATUS_SUCCESS != status)
		{
			msg   = g_objVirtex5BMD.GetErrMsg();
			errMsg.Format("Failed to uninit the VIRTEX5 library: %s", msg);
			AfxMessageBox(errMsg, MB_OK);
		}
	}

	delete [] msg;



	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

//	ULONG_PTR m_gdiplusToken;
//	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
//	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	SetRegistryKey(_T("Image Capture & Delivery Interface (ICANDI) - BRP Project"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.

	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(CICANDIDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(CICANDIView));
	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;

	// The one and only window has been initialized, so show and update it.
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)&AppExceptionHandler); 

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CICANDIApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CICANDIApp message handlers


int CICANDIApp::ExitInstance() 
{
	DWORD   dwStatus;
	char   *msg = NULL;
	CString errMsg;

	msg = new char [256];

	if (g_hDevVirtex5)
		g_objVirtex5BMD.DeviceClose(g_hDevVirtex5, NULL);

	dwStatus = g_objVirtex5BMD.VIRTEX5_LibUninit();
	if (WD_STATUS_SUCCESS != dwStatus)
	{
		msg   = g_objVirtex5BMD.GetErrMsg();
		errMsg.Format("virtex5_diag: Failed to uninit the VIRTEX5 library: %s", msg);
		AfxMessageBox(errMsg);
	}

	delete [] msg;
	msg = NULL;
	
  	
	return CWinApp::ExitInstance();
}


