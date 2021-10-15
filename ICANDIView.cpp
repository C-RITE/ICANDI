// ICANDIView.cpp : implementation of the CICANDIView class
//

#include "stdafx.h"
#include "ICANDI.h"

#include "ICANDIView.h"
#include "ICANDIParams.h"
#include "FolderDlg.h"
#include <fstream>
#include <vector>
#include <algorithm>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern AOSLO_MOVIE  aoslo_movie;
extern ICANDIParams g_ICANDIParams;			// struct for storing ICANDI parameters
extern CView		*g_viewRawVideo;
extern CView        *g_viewMsgVideo;
extern UINT         g_frameIndex;
extern BOOL         g_bFFTIsRunning;
extern BOOL         g_bStimulusOn;
extern POINT        g_StimulusPos;
extern POINT        g_StimulusPosBak;		// stimulus position on the stabilized video
extern long         g_BlockIndex;
extern BOOL         g_bMultiStimuli;
extern BOOL			g_bGain0Tracking;
extern BOOL			g_bRecord;
extern BOOL         g_bNoGPUdevice;
extern BOOL			g_bTCAOverride;
extern BOOL			g_bAutoMeasureTCA;
extern BOOL			g_bConstPwr;

extern FILE         *g_fpTimeLog;
extern BOOL          g_bTimingTest;
extern LARGE_INTEGER g_ticksPerSecond;

extern CVirtex5BMD       g_objVirtex5BMD;
extern WDC_DEVICE_HANDLE g_hDevVirtex5;

extern unsigned short	*g_usRed_LUT;
extern unsigned short	*g_usGreen_LUT;
extern unsigned short	*g_usIR_LUT;
extern double		g_dRedMax;
extern double		g_dGreenMax;
extern double		g_dIRMax;
extern int			g_ncurRedPos;
extern int			g_ncurGreenPos;
extern int          g_ncurIRPos;
extern float	    g_Motion_ScalerX;	//volts per 128 pixels in X direction
extern float	    g_Motion_ScalerY;	//volts per 128 pixels in Y direction

extern short		g_nCurFlashCount;
extern short		g_nFlashCount;

extern BOOL			g_bMatlabCtrl;
extern BOOL			g_bMatlabVideo;
extern BOOL			g_bMatlab_Loop;
extern BOOL			g_bMatlab_Trigger;
extern BOOL			g_bMatlab_Update;
extern BOOL			g_bMatlabAVIsavevideo;
extern BOOL			g_bRunSLR;
extern HANDLE		g_eSyncOCT;
extern BOOL			g_bSyncOCT;
extern BOOL			g_bSyncOCTReady;

extern POINT	g_RGBClkShiftsAuto[3];
extern POINT	g_RGBClkShiftsUser[3];

using std::vector;
using std::random_shuffle;

typedef vector<int> IntVector ;
typedef IntVector::iterator IntVectorIt ;
/////////////////////////////////////////////////////////////////////////////
// CICANDIView

IMPLEMENT_DYNCREATE(CICANDIView, CView)

BEGIN_MESSAGE_MAP(CICANDIView, CView)
	//{{AFX_MSG_MAP(CICANDIView)	
	ON_NOTIFY(EN_MSGFILTER, IDC_EDITBOX_PREFIX, UpdatePrefix)
	ON_NOTIFY(EN_MSGFILTER, IDC_EDITBOX_LASERPOWER, UpdateMaxPower)
	ON_NOTIFY(EN_MSGFILTER, IDC_EDITBOX_FIELDSIZE, UpdateFieldSize)
	ON_NOTIFY(EN_MSGFILTER, IDE_TCA_RED_X, UpdateTCAValues)
	ON_NOTIFY(EN_MSGFILTER, IDE_TCA_RED_Y, UpdateTCAValues)
	ON_NOTIFY(EN_MSGFILTER, IDE_TCA_GR_X, UpdateTCAValues)
	ON_NOTIFY(EN_MSGFILTER, IDE_TCA_GR_Y, UpdateTCAValues)
	ON_BN_CLICKED(ID_RED_RADIO, UpdateRedLaser)
	ON_BN_CLICKED(ID_GREEN_RADIO, UpdateGreenLaser)
	ON_BN_CLICKED(ID_IR_RADIO, UpdateIRLaser)
	ON_BN_CLICKED(ID_DELIVERY_MODE1, UpdateDeliveryMode1)
	ON_BN_CLICKED(ID_DELIVERY_MODE2, UpdateDeliveryMode2)
	ON_BN_CLICKED(ID_DELIVERY_MODE3, UpdateDeliveryMode3)
	ON_BN_CLICKED(ID_DELIVERY_CONSTPWR, UpdateConstPwr)
	ON_BN_CLICKED(ID_AOM_840, SwitchTo840)
	ON_BN_CLICKED(ID_AOM_788, SwitchTo788)
	ON_CBN_SELCHANGE(ID_WAVELENGTH, UpdateWaveLength)
	ON_BN_CLICKED(ID_LOAD_STIMULUS, LoadStimulus)
	ON_BN_CLICKED(ID_LOAD_MULTI_STIM, LoadMultiStim)
	ON_BN_CLICKED(IDC_VOLTS_PER_DEG_Y, MotionScalerChkY)
	ON_BN_CLICKED(IDC_MOTION_ANGLE_Y, MotionAngleChkY)
	ON_BN_CLICKED(ID_STIMULUS_VIDEO, StimulusVideo)
	ON_BN_CLICKED(ID_BUTTON_GAIN0_TRACK, UpdateGain0TrackingStatus)
	ON_BN_CLICKED(ID_STIMULUS_REWIND, RewindVideo)
	ON_BN_CLICKED(ID_STABLIZE_GO, StablizationGo)	
	ON_BN_CLICKED(IDC_CHECK_SLR, EnableSLR)
	ON_BN_CLICKED(ID_BUTTON_OLD_REF, UpdateOldRef)
	ON_BN_CLICKED(ID_PUPIL_MASK, OnEnablePupilMask)
	ON_BN_CLICKED(ID_ONE_FRAME_DELAY, ApplyOneFrameDelay)
	ON_BN_CLICKED(ID_AOM_RED_SWITCH, SwitchREDAOM)
	ON_BN_CLICKED(ID_AOM_GREEN_SWITCH, SwitchGreenAOM)
	ON_BN_CLICKED(ID_AOM_IR_SWITCH, SwitchIRAOM)
	ON_BN_CLICKED(ID_RED_CALIBRATION, CalibrateRed)
	ON_BN_CLICKED(ID_SAVE_VIDEO, SaveVideoCommand)
//	ON_BN_CLICKED(IDC_BUTTON_RAW_NAME, LoadRawVideoName)
	ON_BN_CLICKED(IDC_SYNC_OCT, EnableOCTSync)
	ON_BN_CLICKED(IDB_TCA_MEASURE, OnMeasureTCA)
	ON_BN_CLICKED(IDC_TCA_APPLY, OnApplyTCA)
	ON_BN_CLICKED(IDC_TCA_OVERRIDE, OnTCAOverride)
	ON_BN_CLICKED(IDC_BUTTON_DEWARP_NAME, LoadDewarpedVideo)
	ON_EN_KILLFOCUS(IDL_VIDEO_FILENAME, RawNameKillFocus)
	ON_WM_CREATE()
	ON_MESSAGE(WM_MOVIE_SEND, OnSendMovie)
	ON_WM_PAINT()
	ON_WM_HSCROLL()
	ON_WM_SIZE()
	ON_NOTIFY(TCN_SELCHANGE, ID_TAB_CONTROLLER, OnSelchangeTabMain)
	ON_WM_VSCROLL()
	//}}AFX_MSG_MAP
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, CView::OnFilePrintPreview)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CICANDIView construction/destruction

CICANDIView::CICANDIView()
{
	g_viewMsgVideo     = this;
	m_bSaveDewarpImage = FALSE;		// show/hide dewarped video
	m_bStablizationOn  = FALSE;
	m_bVoltsPerDegY   = FALSE;
	m_bMotionAngleY	   = FALSE;
	m_bStabCurves      = TRUE;

	m_ButtonDist       = 30;
	m_ButtonHeight     = 20;
	m_ButtonWidth      = 110;

	for (int i = 0; i < 15; i ++) m_StimFreq[i] = GRABBER_FRAME_RATE;//30;
	m_FreqCount        = 8;
	m_StimFreq[0]      = 1;
	m_StimFreq[1]      = 2;
	m_StimFreq[2]      = 3;
	m_StimFreq[3]      = 5;
	m_StimFreq[4]      = 6;
	m_StimFreq[5]      = 10;
	m_StimFreq[6]      = 15;

	m_nRedPowerOld     = 0;
	m_nGreenPowerOld   = 0;
	m_nIRPowerOld      = 0;
	m_bCalib		   = false;

	//reset parallel port
	Out32(0x378,0);

}

CICANDIView::~CICANDIView()
{
}

BOOL CICANDIView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

/////////////////////////////////////////////////////////////////////////////
// CICANDIView drawing

void CICANDIView::OnDraw(CDC* pDC)
{
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	// TODO: add draw code for native data here
}

/////////////////////////////////////////////////////////////////////////////
// CICANDIView printing

BOOL CICANDIView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void CICANDIView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void CICANDIView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

/////////////////////////////////////////////////////////////////////////////
// CICANDIView diagnostics

#ifdef _DEBUG
void CICANDIView::AssertValid() const
{
	CView::AssertValid();
}

void CICANDIView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CICANDIDoc* CICANDIView::GetDocument() // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CICANDIDoc)));
	return (CICANDIDoc*)m_pDocument;
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CICANDIView message handlers

void CICANDIView::FontFormat(CHARFORMAT &cf, int sel)
{
	switch (sel)
	{
	case 0:
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE |
			CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE;
		cf.dwEffects = CFE_BOLD;
		cf.yHeight = 10 * 20;
		cf.crTextColor = RGB(0, 0, 255);
		strcpy(cf.szFaceName, "Arial");
		cf.bCharSet = 0;
		cf.bPitchAndFamily = 0;
		break;
	case 1:		
		cf.cbSize = sizeof(CHARFORMAT);
		cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE |
			CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE;
		cf.dwEffects = CFE_BOLD;
		cf.yHeight = 300;
		cf.crTextColor = RGB(255, 0, 0);
		strcpy(cf.szFaceName, "Arial");
		cf.bCharSet = 0;
		cf.bPitchAndFamily = 0;
		break;
	default:
		;
	}    
}


int CICANDIView::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT       rect;
	int        i, bmpNum = 8;
	CString    text, temp;
	CHARFORMAT cf, cf1;
	CBitmap    bitmap; 
	//char       temp[80];
	char       szAppPath[80] = "";

	
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	FontFormat(cf, 0);
	FontFormat(cf1, 1);

	rect.left   = 0;
	rect.top    = 0;
	rect.bottom = 0;
	rect.right  = 0;
	m_tabControl.Create(TCS_MULTILINE | WS_CHILD | WS_VISIBLE, rect, this, ID_TAB_CONTROLLER);
	m_tabControl.InsertItem(0, "Main Control");
	m_tabControl.InsertItem(1, "Stimulus Setup");
//	m_tabControl.InsertItem(2, "TCA Setup");

//	streamA=fopen("C:\\timing.txt", "w+t");

	// draw a frame to group the inputs for video file names
	
	rect.left = 10;	rect.right  = rect.left + m_ButtonWidth*3+35;
	rect.top  = 40; rect.bottom = rect.top + m_ButtonHeight*3+15;
	m_frameOne.Create("", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER, rect, this, ID_FRAME_ONE);

	rect.left = 20;	rect.right  = rect.left + m_ButtonWidth+40;
	rect.top  = 30; rect.bottom = rect.top + m_ButtonHeight;
	m_lasPowerControl.Create("  Laser Power Control", WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDC_LASER_CONTROL);

	// draw a radio button to select red laser
	rect.left = 20;	 rect.right  = rect.left + 40;
	rect.top  = 55; rect.bottom = rect.top + m_ButtonHeight;
	m_redLaser.Create("Rd", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					   rect, this, ID_RED_RADIO);
	m_redLaser.SetCheck(1);

	// draw a radio button to select green laser
	rect.left = 60;	 rect.right  = rect.left + 40;
	rect.top  = 55; rect.bottom = rect.top + m_ButtonHeight;
	m_grLaser.Create("Gr", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					   rect, this, ID_GREEN_RADIO);

	// draw a radio button to select green laser
	rect.left = 100;	 rect.right  = rect.left + 40;
	rect.top  = 55; rect.bottom = rect.top + m_ButtonHeight;
	m_irLaser.Create("IR", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					   rect, this, ID_IR_RADIO);

	rect.left = 20;	 rect.right  = rect.left + m_ButtonWidth*2+15;
	rect.top  = 85; rect.bottom = rect.top + m_ButtonHeight;
	m_sldLaserPow14.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | SBS_HORZ, rect, this, IDC_LASER_POWER_14BITS);
	m_sldLaserPow14.EnableWindow(true);
	m_bCalib?m_sldLaserPow14.SetScrollRange(0,16383)/*calibration*/:m_sldLaserPow14.SetScrollRange(0,1000)/*real system*/;
	m_sldLaserPow14.SetScrollPos(0);
	m_sldLaserPow14.SetFocus();

	// draw a label for laser power percentage
	rect.left = 233;	rect.right  = rect.left + 53;
	rect.top  = 60; rect.bottom = rect.top + m_ButtonHeight;
	m_lblLaserPowerPer.Create("0.0%",  WS_CHILD | WS_VISIBLE | SS_CENTER, 
					 rect, this, IDC_LABEL_LASERSLIDERPOS);

	// draw an edit box for entering laser power
	rect.left = 285;	rect.right  = rect.left + m_ButtonWidth/2 - 5;
	rect.top  = 55; rect.bottom = rect.top + m_ButtonHeight + 5;
	m_edtLaserPower.Create(WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, IDC_EDITBOX_LASERPOWER);
	m_edtLaserPower.SetDefaultCharFormat(cf);
	m_edtLaserPower.SetWindowText("0.000");
	m_edtLaserPower.SendMessage(EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

	// draw a label for laser power units
	rect.left = rect.right+3;	rect.right  = rect.left + 25;
	rect.top  = 60; rect.bottom = rect.top + m_ButtonHeight;
	m_lblLaserPower.Create("uW",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDC_LABEL_LASERPOWER);

	// draw a label for laser power in other units (other units Trolands)
	rect.left = 260;	rect.right  = rect.left + 100;
	rect.top  = 90; rect.bottom = rect.top + m_ButtonHeight;
	m_lblLaserPowerTl.Create("0 Td",  WS_CHILD | WS_VISIBLE | SS_RIGHT, 
					 rect, this, IDC_LABEL_LASERPOWERTL);

	// draw an edit box for entering feild size
	rect.left = 138;	rect.right  = rect.left + m_ButtonWidth/2 - 10;
	rect.top  = 55; rect.bottom = rect.top + m_ButtonHeight + 5;
	m_edtFieldSize.Create(WS_CHILD | WS_VISIBLE | ES_RIGHT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, IDC_EDITBOX_FIELDSIZE);
	m_edtFieldSize.SetDefaultCharFormat(cf);
	m_edtFieldSize.SetWindowText("1.8");
	m_edtFieldSize.SendMessage(EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

	// draw a label for degrees
	rect.left = rect.right+2;	rect.right  = rect.left + m_ButtonWidth/2;
	rect.top  = 60; rect.bottom = rect.top + m_ButtonHeight;
	m_lblFeildSize.Create("Deg.Sq",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDC_LABEL_FEILDSIZE);

	// draw a frame to group the inputs for video file names
	rect.left = 10;	rect.right  = rect.left + m_ButtonWidth*3+35;
	rect.top  = 115+m_ButtonHeight; rect.bottom = rect.top + m_ButtonHeight*3+25;
	m_frameTwo.Create("", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER, rect, this, ID_FRAME_TWO);

	rect.left = 20;	rect.right  = rect.left + m_ButtonWidth+15;
	rect.top  = 105+m_ButtonHeight; rect.bottom = rect.top + m_ButtonHeight;
	m_fraVideoName.Create("  Video Parameters", WS_CHILD | WS_VISIBLE | SS_LEFT,
					 rect, this, IDF_VIDEO_PARAMS);

	//draw a label for current video folder
	rect.left = 20;	rect.right  = rect.left + (int)(m_ButtonWidth)-65;
	rect.top  = rect.top + 30; rect.bottom = rect.top + m_ButtonHeight+5;
	m_lblVideoFolder.Create("Folder: ",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDL_VIDEO_FOLDER);

	// draw a box for current Video Folder
	rect.left = rect.right+5;	rect.right  = rect.left + (int)(m_ButtonWidth + 30);
	rect.top  = rect.top - 5; rect.bottom = rect.top + m_ButtonHeight+5;
	m_edtFolder.Create(WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, 
					 rect, this, IDE_VIDEO_FOLDER);
	m_edtFolder.SetDefaultCharFormat(cf);
	m_edtFolder.EnableWindow(FALSE);

	// draw a label for entering prefix of saved file name
	rect.left = rect.right + 22;	rect.right  = rect.left + m_ButtonWidth/2 + 32;
	rect.top  = rect.top-5; rect.bottom = rect.top + m_ButtonHeight+5;
	m_lblPrefix.Create("Prefix:",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDC_LABEL_PREFIX);

	// draw an edit box for entering prefix of saved file name
	rect.left = rect.right - 32;	rect.right  = rect.left + m_ButtonWidth/2 + 20;
	rect.top  = rect.top-5; rect.bottom = rect.top + m_ButtonHeight+5;
	m_edtPrefix.Create(WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, IDC_EDITBOX_PREFIX);
	m_edtPrefix.SetDefaultCharFormat(cf);
	m_edtPrefix.SetWindowText("Prefix");
	m_edtPrefix.SendMessage(EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

	// draw a label for entering video length
	rect.left = (int)(m_ButtonWidth*1.7) + 65;	rect.right  = rect.left + m_ButtonWidth/2 + 5;	
	rect.top  = rect.top+m_ButtonHeight+15; rect.bottom = rect.top + m_ButtonHeight+5;
	m_lblVideoLen.Create("Length:",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDC_LABEL_VIDEOLEN);
	// draw an edit box for entering video length
	rect.left = rect.right - 7;	rect.right  = rect.left + m_ButtonWidth/2;
	rect.top  = rect.top-5; rect.bottom = rect.top + m_ButtonHeight+5;
	m_edtVideoLen.Create(WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, IDC_EDITBOX_VIDEOLEN);
	m_edtVideoLen.SetDefaultCharFormat(cf);
	m_edtVideoLen.SetWindowText("10");

	//draw a label for current video filename
	rect.left = 20;	rect.right  = rect.left + (int)(m_ButtonWidth)-80;
	rect.top  = rect.top+20; rect.bottom = rect.top + m_ButtonHeight-5;
	m_lblVideoFileName.Create("File: ",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDL_VIDEO_FILENAME);

	// draw a box for current Video Filename
	rect.left = rect.right+5;	rect.right  = rect.left + (int)(m_ButtonWidth + 80);
	rect.top  = rect.top - 10; rect.bottom = rect.top + m_ButtonHeight+10;
	m_edtFilename.Create(WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL, 
					 rect, this, IDE_VIDEO_FILENAME);
	m_edtFilename.SetDefaultCharFormat(cf1);
	m_edtFilename.EnableWindow(FALSE);

	// draw a check box for syncing video grab with OCT
	rect.left = rect.right+15;	rect.right  = rect.left + m_ButtonWidth ;	
	rect.top  = rect.top+15; rect.bottom = rect.top + m_ButtonHeight;
	m_chkSyncOCT.Create("Sync w/OCT",  WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP | WS_DISABLED, 
					  rect, this, IDC_SYNC_OCT);

	// draw a push button to start FFT stablization
	rect.left = 390;	rect.right  = rect.left + m_ButtonWidth - 45;
	rect.top  = 34;		rect.bottom = rect.top + m_ButtonHeight+5;
	m_btnGo.Create("Go", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_DISABLED, 
					 rect, this, ID_STABLIZE_GO);

	rect.left = rect.right+5;	 rect.right  = rect.left + m_ButtonWidth-40 ;	
	m_chkSLR.Create("SLR On", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP | WS_DISABLED, 
					  rect, this, IDC_CHECK_SLR);
	rect.left = 420;	 rect.right  = rect.left + m_ButtonWidth ;
	rect.top  = rect.bottom +3;		rect.bottom = rect.top + m_ButtonHeight;
	m_btnUpdateOldRef.Create("Update Old Ref.", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP | WS_DISABLED, 
					 rect, this, ID_BUTTON_OLD_REF);

	// draw an check box for deselecting/selecting file name of dewarpped video
	rect.left = 390;	 rect.right  = rect.left + 10;
	rect.top = rect.bottom+3; rect.bottom = rect.top + m_ButtonHeight+3;
	m_btnDewarpName.Create("", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					  rect, this, IDC_BUTTON_DEWARP_NAME);
	m_btnDewarpName.SetCheck(0); // 10/21/2011
	m_bSaveDewarpImage = FALSE;

	// draw a push button to suspend FFT stablization
	rect.left  = rect.right + 5; rect.right  = rect.left + m_ButtonWidth;	
	m_btnSaveVideo.Create("Save Video", WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_DISABLED, 
						rect, this, ID_SAVE_VIDEO);

	// draw a checkbox to turn on/off AOM control
	rect.left = 395;	 rect.right  = rect.left + m_ButtonWidth + 5;
	rect.top  = rect.bottom + 3; rect.bottom = rect.top + m_ButtonHeight;
	m_chkPupilMask.Create("Pupil Mask", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, rect, this, ID_PUPIL_MASK);


	rect.top  = rect.bottom; rect.bottom = rect.top + m_ButtonHeight;
	m_chkOneFrameDelay.Create("1 Frame Delay", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, rect, this, ID_ONE_FRAME_DELAY);

	rect.top  = rect.bottom; rect.bottom = rect.top + m_ButtonHeight;
	rect.right  = rect.left + m_ButtonWidth/2-15;
	m_chkIRAOM.Create("IR", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_AOM_IR_SWITCH);
	m_chkIRAOM.SetCheck(1);
	rect.left   = rect.right + 1;
	rect.right  = rect.left + m_ButtonWidth/2-15;
	m_chkRedAOM.Create("Rd", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_AOM_RED_SWITCH);
	m_chkRedAOM.SetCheck(1);

	rect.left = rect.right + 1;
	rect.right = rect.left + m_ButtonWidth/2-15;
	m_chkGrAOM.Create("Gr", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_AOM_GREEN_SWITCH);
	m_chkGrAOM.SetCheck(1);

	rect.left = 395;
	rect.right = rect.left + m_ButtonWidth;
	rect.top  = rect.bottom; rect.bottom = rect.top + m_ButtonHeight;
	m_chkRedCal.Create("Laser calib.", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_RED_CALIBRATION);
	m_bCalib = false;
	
	rect.right  = rect.left + m_ButtonWidth / 2;
	rect.top    = rect.bottom; rect.bottom = rect.top + m_ButtonHeight;
	rect.bottom += 100;

	m_waveLength.Create(WS_CHILD | WS_VISIBLE | CBS_HASSTRINGS | CBS_DROPDOWNLIST,
					rect, this, ID_WAVELENGTH);
	
	for (int i = 0; i < 12; i++){
		if (g_ICANDIParams.waveLength[i] == "") break;
		m_waveLength.AddString(g_ICANDIParams.waveLength[i]);
	}
	m_waveLength.SetCurSel(0);

	// draw a frame to group setup parameters for stimulus delivery
	rect.left = 5;	rect.right  = rect.left + m_ButtonWidth*3+5;//-10;
	rect.top  = 40; rect.bottom = rect.top + m_ButtonHeight*7+40;
	m_frameThree.Create("", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER, rect, this, ID_FRAME_THREE);

	rect.left = 15;	rect.right  = rect.left + m_ButtonWidth+10;
	rect.top  = 30; rect.bottom = rect.top + m_ButtonHeight;
	m_fraStimulusSetup.Create("Stimulus Delivery", WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDC_STIMULUS_SETUP);

	// adjustment of Motion traces scaling constant	
	if (g_ICANDIParams.VRangePerDegX <= 0.6 && g_ICANDIParams.VRangePerDegX >= 0.2) {
		i = g_ICANDIParams.VRangePerDegX*1000-200;
	} else {
		i = 0;
		g_ICANDIParams.VRangePerDegX = 0.4;
	}
	g_Motion_ScalerX = (float)(g_ICANDIParams.VRangePerDegX*128./g_ICANDIParams.PixPerDegX);
	
	rect.left = 15;	 rect.right  = rect.left + m_ButtonWidth;
	rect.top  = 48; rect.bottom = rect.top  + m_ButtonHeight-5;
	m_lblVoltsPerDeg.Create("Volts Per Deg", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_VOLTS_PER_DEG_LABEL);
	rect.left = rect.right;	 rect.right  = rect.left + m_ButtonWidth + 15;
	rect.bottom = rect.top + m_ButtonHeight-5;
	m_sldVoltsPerDeg.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rect, this, ID_VOLTS_PER_DEG_SLIDER);
	m_sldVoltsPerDeg.SetScrollRange(1,401,TRUE);		// set motion scaler 0.1,0.2,0.3,0.4,0.5,0.6, ..., 5.0 
	m_sldVoltsPerDeg.SetScrollPos(i,TRUE);
	rect.left = rect.right+5;	 rect.right  = rect.left + m_ButtonWidth/2-5;
	text.Format("%1.3f V/d", g_ICANDIParams.VRangePerDegX);
	m_lblVoltsPerDegValue.Create(text, WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_MOTION_SCALER_VALUE_LABEL);	
	g_Motion_ScalerY = (float)(g_ICANDIParams.VRangePerDegY*128./g_ICANDIParams.PixPerDegY);
	
	rect.left = rect.right+2;	 rect.right  = rect.left + m_ButtonWidth/2-25;
//	rect.top  = rect.top+m_ButtonHeight-10;rect.bottom = rect.top  + m_ButtonHeight-5;
	m_chkVoltsPerDegY.Create("Y", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, IDC_VOLTS_PER_DEG_Y);

	// adjustment of Motion traces angle	
	rect.left = 15;	 rect.right  = rect.left + m_ButtonWidth;
	rect.top  = rect.top+m_ButtonHeight-1; rect.bottom = rect.top  + m_ButtonHeight-5;
	m_lblMotionAngle.Create("Motion Angle", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_MOTION_ANGLE_LABEL);
	rect.left = rect.right;	 rect.right  = rect.left + m_ButtonWidth + 15;
	rect.bottom = rect.top + m_ButtonHeight-5;
	m_sldMotionAngle.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rect, this, ID_MOTION_ANGLE_SLIDER);
	m_sldMotionAngle.SetScrollRange(0,300,TRUE);		// angle range is from -10->10deg in 0.2deg steps
	i = g_ICANDIParams.MotionAngleX*5+150; 
	m_sldMotionAngle.SetScrollPos(i,TRUE);	
	rect.left = rect.right+5;	 rect.right  = rect.left + m_ButtonWidth/2-5;
	text.Format("%2.2f Deg", g_ICANDIParams.MotionAngleX);
	m_lblMotionAngleValue.Create(text, WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_MOTION_ANGLE_VALUE_LABEL);		
	
	rect.left = rect.right+2;	 rect.right  = rect.left + m_ButtonWidth/2-25;
//	rect.top  = rect.top+m_ButtonHeight-10;rect.bottom = rect.top  + m_ButtonHeight-5;
	m_chkMotionAngleY.Create("Y", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, IDC_MOTION_ANGLE_Y);

	// flash frequency
	rect.left = 15;   rect.right  = rect.left + m_ButtonWidth;
	rect.top  = rect.top+m_ButtonHeight-1; rect.bottom = rect.top  + m_ButtonHeight-5;
	m_lblFlashFreq.Create("Flash frequency", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_FLASH_FREQ_LABEL);

	rect.left = rect.right; rect.right = rect.left + m_ButtonWidth + 15;
	rect.bottom = rect.top + m_ButtonHeight-5;
	m_sldFlashFreq.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rect, this, ID_FLASH_FREQ_SLIDER);
	m_sldFlashFreq.SetScrollRange(0,7,TRUE);		// set flash available frequency 1,2,3,5,6,10,15,30 Hz
	m_sldFlashFreq.SetScrollPos(2,TRUE);			// default 3 Hz
	rect.left = rect.right+5;	 rect.right  = rect.left + m_ButtonWidth/2;
	m_lblFlashFrequency.Create("3 Hz", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_FLASH_FREQUENCY_LABEL);


	// flash duty cycle
	//rect.left = m_ButtonWidth*3 + 47;   rect.right  = rect.left + m_ButtonWidth/2;
	rect.left = 15;   rect.right  = rect.left + m_ButtonWidth;
	rect.top  = rect.top+m_ButtonHeight-1; rect.bottom = rect.top  + m_ButtonHeight-5;
	m_lblFlashDuty.Create("Flash duty cycle", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_FLASH_DUTY_LABEL);
	rect.left = rect.right;
	rect.right = rect.left + m_ButtonWidth + 15;
	rect.bottom = rect.top + m_ButtonHeight-5;
	m_sldDutyCycle.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rect, this, ID_FLASH_DUTY_SLIDER);
	int ulimit = GRABBER_FRAME_RATE	/ aoslo_movie.FlashFrequency - 2;
	m_sldDutyCycle.SetScrollRange(0,ulimit,TRUE);		// set flash duty cycle
	m_sldDutyCycle.SetScrollPos(ulimit/2,TRUE);		// default 5:5 (5 on 5 off)
	rect.left = rect.right+5;	 rect.right  = rect.left + m_ButtonWidth/2;
	m_lblFlashDutyCycle.Create("5:5", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_FLASH_DUTYCYCLE_LABEL);

	// stabilization mode for stimulus video
	rect.left = 15;   rect.right  = rect.left + m_ButtonWidth;
	rect.top  = rect.bottom+3; rect.bottom = rect.top + m_ButtonHeight-5;
	m_lblStimGain.Create("Stimulus Gain", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_STIM_GAIN_LBL);

	rect.left = rect.right; rect.right = rect.left + m_ButtonWidth + 15;
	m_sldStimGainCtrl.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rect, this, ID_STIM_GAIN_CONTROL);
	m_sldStimGainCtrl.SetScrollRange(0, 600, TRUE);
	m_sldStimGainCtrl.SetScrollPos(400, TRUE);
	rect.left = rect.right+5;   rect.right  = rect.left + m_ButtonWidth/2;
	m_lblStimGainVal.Create("1.00", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_STIM_GAIN_VALUE_LBL);

	rect.left = 15;   rect.right  = rect.left + m_ButtonWidth;
	rect.top  = rect.bottom+3; rect.bottom = rect.top + m_ButtonHeight-5;
	m_lblMaskGain.Create("Mask Gain", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_MASK_GAIN_LBL);

	rect.left = rect.right; rect.right = rect.left + m_ButtonWidth + 15;
	m_sldMaskGainCtrl.Create(WS_CHILD | WS_VISIBLE | SBS_HORZ, rect, this, ID_MASK_GAIN_CONTROL);
	m_sldMaskGainCtrl.SetScrollRange(0, 600, TRUE);	 
	m_sldMaskGainCtrl.SetScrollPos(400, TRUE);
	rect.left = rect.right+5;   rect.right  = rect.left + m_ButtonWidth/2;
	m_lblMaskGainVal.Create("1.00", WS_CHILD | WS_VISIBLE | SS_LEFT, rect, this, ID_MASK_GAIN_VALUE_LBL);
	m_lblMaskGain.EnableWindow(FALSE);
	m_lblMaskGainVal.EnableWindow(FALSE);
	m_sldMaskGainCtrl.EnableWindow(FALSE);


	// delivery mode
	rect.left = 25;   rect.right  = rect.left + (int)(m_ButtonWidth*1.3);
	rect.top  = 120 + 2*m_ButtonHeight; rect.bottom = rect.top + m_ButtonHeight-2;
	m_chkConstPwr.Create("Constant Power", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP,
				rect, this, ID_DELIVERY_CONSTPWR);

	RECT nextOne(rect);
	nextOne.left = rect.right;
	nextOne.right = rect.right +=90;
	m_Imaging840.Create("840nm", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					 nextOne, this, ID_AOM_840);
	m_Imaging840.SetCheck(g_ICANDIParams.Imaging840);

	RECT nextTwo(nextOne);
	nextTwo.left += 70;
	nextTwo.right += 70;
	m_Imaging788.Create("788nm", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					 nextTwo, this, ID_AOM_788);
	m_Imaging788.SetCheck(!g_ICANDIParams.Imaging840);

	//rect.left = m_ButtonWidth*3 + 47; rect.right  = rect.left + m_ButtonWidth + 40;
	rect.left = 25;   rect.right  = rect.left + (int)(m_ButtonWidth*1.3);
	rect.top  = 140 + 2*m_ButtonHeight; rect.bottom = rect.top + m_ButtonHeight-2;
	m_btnDeliveryMode1.Create("Single Stimulation", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					   rect, this, ID_DELIVERY_MODE1);
	m_btnDeliveryMode1.SetCheck(1);
	rect.top  = rect.bottom + 3; rect.bottom = rect.bottom + m_ButtonHeight;
	m_btnDeliveryMode2.Create("User-defined Stim", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					   rect, this, ID_DELIVERY_MODE2);
	m_btnDeliveryMode2.SetCheck(0);
	
	rect.left = rect.right + 5; rect.right  = rect.left + (int)(m_ButtonWidth*1.3);
	rect.top  = 140 + 2*m_ButtonHeight; rect.bottom = rect.top + m_ButtonHeight-2;
	m_btnDeliveryMode3.Create("Random Stimulation", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, 
					   rect, this, ID_DELIVERY_MODE3);
	m_btnDeliveryMode3.SetCheck(0);

	// draw a push button to load multiple stimuli from several files
	rect.top  = rect.bottom + 3; rect.bottom = rect.bottom + m_ButtonHeight;
	m_btnMultiStim.Create("Multiple Stimuli", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_LOAD_MULTI_STIM);
//	m_btnMultiStim.EnableWindow(FALSE);

	// draw a push button to load stimulus from file
	rect.left = 360; rect.right  = rect.left + m_ButtonWidth-50;
	rect.top  = 35; rect.bottom = rect.top + m_ButtonHeight+5;
	m_btnStimulus.Create("Load", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 
					 rect, this, ID_LOAD_STIMULUS);

	rect.left = rect.right+20; rect.right  = rect.left + m_ButtonWidth-20;
	rect.top  = 30; rect.bottom = rect.top + m_ButtonHeight;
	m_chkStimVideo.Create("Stim Video", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_STIMULUS_VIDEO);
//	rect.left = rect.right + 4; rect.right  = rect.left + m_ButtonWidth-15;
	rect.top  = rect.bottom; rect.bottom = rect.top + m_ButtonHeight;
	m_chkRewindVideo.Create("Loop video", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_STIMULUS_REWIND);
	m_chkRewindVideo.EnableWindow(FALSE);

	rect.top  = rect.bottom ; rect.bottom = rect.top + m_ButtonHeight;
	rect.left = 375; rect.right  = rect.left + m_ButtonWidth + 30;
	m_chkGain0Tracking.Create("Gain0 Tracking On", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, ID_BUTTON_GAIN0_TRACK);

	// draw a frame to group setup parameters for TCA setup
	rect.left = 345;	rect.right  = rect.left + m_ButtonWidth*2-30;
	rect.top  = rect.bottom+10; rect.bottom = rect.top + m_ButtonHeight*6;
	m_frameFour.Create("", WS_CHILD | WS_VISIBLE | SS_LEFT | WS_BORDER, rect, this, ID_FRAME_FOUR);

	rect.left = 355;	rect.right  = rect.left + m_ButtonWidth-40;
	rect.top  -= 10; rect.bottom = rect.top + m_ButtonHeight;
	m_fraTCASetup.Create("TCA Setup", WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDC_TCA_SETUP);

	rect.left = 355;	rect.right  = rect.left + m_ButtonWidth-30;
	rect.top  = rect.bottom; rect.bottom = rect.top + m_ButtonHeight+5;
	m_TCAMeasure.Create("Measure", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP, 
					 rect, this, IDB_TCA_MEASURE);

	rect.left = rect.right+10;	rect.right  = rect.left + m_ButtonWidth-25;	
	m_TCAApply.Create("Apply TCA", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP | WS_DISABLED, 
					 rect, this, IDC_TCA_APPLY);
	rect.left = 395;	rect.right  = rect.left + m_ButtonWidth-30;
	rect.top = rect.bottom+5;	rect.bottom  = rect.top + m_ButtonHeight;	
	m_TCAOverride.Create("Override", WS_CHILD | WS_VISIBLE | BS_CHECKBOX | WS_TABSTOP, 
					 rect, this, IDC_TCA_OVERRIDE);
	m_TCAOverride.SetCheck(g_bTCAOverride);

	rect.left = 396;	rect.right  = rect.left + m_ButtonWidth-75;
	rect.top = rect.bottom;	rect.bottom  = rect.top + m_ButtonHeight;	
	m_lblTCARed.Create("Red",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDL_TCA_RED_LBL);

	rect.left = rect.right+1;	rect.right  = rect.left + m_ButtonWidth-50;	
	m_lblTCAGr.Create("Green",  WS_CHILD | WS_VISIBLE | SS_LEFT, 
					 rect, this, IDL_TCA_GR_LBL);

	rect.left = 365;	rect.right  = rect.left + m_ButtonWidth-90;
	rect.top = rect.bottom-1;	rect.bottom  = rect.top + m_ButtonHeight;	
	m_lblTCAX.Create("dX",  WS_CHILD | WS_VISIBLE | SS_RIGHT, 
					 rect, this, IDL_TCA_X_LBL);

	rect.left = rect.right+5; rect.right  = rect.left + (int)(m_ButtonWidth*0.35);
	rect.top -= 1;	rect.bottom  = rect.top + m_ButtonHeight;
	m_edtTCARedX.Create(WS_CHILD | WS_VISIBLE | ES_CENTER | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER | WS_DISABLED,  
					 rect, this, IDE_TCA_RED_X);
	m_edtTCARedX.SetDefaultCharFormat(cf);
	text.Format("%d", g_ICANDIParams.RGBClkShifts[0].x);
	SetDlgItemText(IDE_TCA_RED_X, text);
	m_edtTCARedX.SendMessage(EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

	rect.left = rect.right+5; rect.right  = rect.left + (int)(m_ButtonWidth*0.35);
	m_edtTCAGrX.Create(WS_CHILD | WS_VISIBLE | ES_CENTER | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER | WS_DISABLED,  
					 rect, this, IDE_TCA_GR_X);
	m_edtTCAGrX.SetDefaultCharFormat(cf);	
	text.Format("%d", g_ICANDIParams.RGBClkShifts[1].x);
	SetDlgItemText(IDE_TCA_GR_X, text);
	m_edtTCAGrX.SendMessage(EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

	rect.left = 365;	rect.right  = rect.left + m_ButtonWidth-90;
	rect.top = rect.bottom+1;	rect.bottom  = rect.top + m_ButtonHeight;	
	m_lblTCAY.Create("dY",  WS_CHILD | WS_VISIBLE | SS_RIGHT, 
					 rect, this, IDL_TCA_Y_LBL);

	rect.left = rect.right+5; rect.right  = rect.left + (int)(m_ButtonWidth*0.35);
	rect.top -= 1;	rect.bottom  = rect.top + m_ButtonHeight;	
	m_edtTCARedY.Create(WS_CHILD | WS_VISIBLE | ES_CENTER | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER | WS_DISABLED,  
					 rect, this, IDE_TCA_RED_Y);
	m_edtTCARedY.SetDefaultCharFormat(cf);
	text.Format("%d", g_ICANDIParams.RGBClkShifts[0].y);
	SetDlgItemText(IDE_TCA_RED_Y, text);
	m_edtTCARedY.SendMessage(EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

	rect.left = rect.right+5; rect.right  = rect.left + (int)(m_ButtonWidth*0.35);
	m_edtTCAGrY.Create(WS_CHILD | WS_VISIBLE | ES_CENTER | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL | ES_NUMBER | WS_DISABLED,  
					 rect, this, IDE_TCA_GR_Y);
	m_edtTCAGrY.SetDefaultCharFormat(cf);
	text.Format("%d", g_ICANDIParams.RGBClkShifts[1].y);
	SetDlgItemText(IDE_TCA_GR_Y, text);
	m_edtTCAGrY.SendMessage(EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

	memcpy(g_RGBClkShiftsAuto, g_ICANDIParams.RGBClkShifts, 3*sizeof(POINT));
	memcpy(g_RGBClkShiftsUser, g_ICANDIParams.RGBClkShifts, 3*sizeof(POINT));
	
//	g_objVirtex5BMD.AppShiftRedX(g_hDevVirtex5, AOM_LATENCYX_RED);
//	g_objVirtex5BMD.AppShiftRedY(g_hDevVirtex5, 0);
//	g_objVirtex5BMD.AppShiftGreenX(g_hDevVirtex5, AOM_LATENCYX_GR);
//	g_objVirtex5BMD.AppShiftGreenY(g_hDevVirtex5, 0);

	if (g_ICANDIParams.ApplyTCA) {
		m_TCAMeasure.EnableWindow(0);
		m_TCAApply.EnableWindow(1);
		m_TCAApply.SetCheck(1);
		m_TCAOverride.SetCheck(1);
		g_bTCAOverride = TRUE;
		ApplyTCA();
	} else if (!g_ICANDIParams.ApplyTCA && (g_RGBClkShiftsUser[0].x || g_RGBClkShiftsUser[0].y || g_RGBClkShiftsUser[1].x || g_RGBClkShiftsUser[1].y)) {				
		m_TCAMeasure.EnableWindow(0);
		m_TCAOverride.SetCheck(1);	
		g_bTCAOverride = TRUE;
		m_TCAApply.EnableWindow(1);
	} else {
		m_TCAMeasure.EnableWindow(1);
		m_TCAApply.EnableWindow(0);
	}

	if (g_bTCAOverride) {
		m_edtTCARedX.EnableWindow(1);
		m_edtTCARedY.EnableWindow(1);
		m_edtTCAGrX.EnableWindow(1);
		m_edtTCAGrY.EnableWindow(1);
	}
	
	rect.top  = rect.bottom + 10; rect.bottom = rect.top + m_ButtonHeight+3;
	rect.left = rect.top=0; rect.right = rect.bottom = 20;
	m_processor.Create(WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | WS_TABSTOP, 
					 rect, this, IDC_PROCESSOR_USED);
	m_processor.EnableWindow(FALSE);
	m_processor.SetDefaultCharFormat(cf);

	ShowTabItems(0);
	fieldSize = 1.8;
		
	UpdateOtherUnits();
	g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, g_usRed_LUT[g_ncurRedPos]);
	g_objVirtex5BMD.AppUpdate14BITsLaserGR(g_hDevVirtex5, g_usGreen_LUT[g_ncurGreenPos]);
	g_objVirtex5BMD.AppUpdateIRLaser(g_hDevVirtex5, (BYTE)g_usIR_LUT[g_ncurIRPos]);
	return 0;
}

void CICANDIView::SaveVideoCommand()
{
	CString     text, folder_name, netmsg;
	bool result = false;
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	GetDlgItemText(ID_SAVE_VIDEO, text);
	
	pDoc->GetSysTime(pDoc->m_VideoTimeStamp);

	int token = 0;
	CString videoname = pDoc->m_videoFileName.Tokenize(_T("_"),token);
	
	// coming from matlab
	if (text == _T("Save Video")) {
		if (pDoc->m_bExtCtrl == TRUE) {
			pDoc->m_aviFileNameA.Format("%s%s_%s%s_%s_V%03d.avi", pDoc->m_VideoFolder, videoname, pDoc->m_VideoTimeStamp, pDoc->m_VideoFolderSuffix, pDoc->m_waveLength, ++pDoc->m_nVideoNum);
			if (g_bFFTIsRunning) {
				pDoc->m_txtFileName.Format ("%s%s_%s%s_%s_V%03d.csv", pDoc->m_VideoFolder, videoname, pDoc->m_VideoTimeStamp, pDoc->m_VideoFolderSuffix, pDoc->m_waveLength, pDoc->m_nVideoNum);
				m_bSaveDewarpImage?pDoc->m_aviFileNameB.Format("%s%s_%s%s_%s_V%03d_stabilized.avi", pDoc->m_VideoFolder, videoname, pDoc->m_VideoTimeStamp, pDoc->m_VideoFolderSuffix, pDoc->m_waveLength, pDoc->m_nVideoNum):0;
			}
		}
		else {
	// coming from within
			if (pDoc->m_VideoFolder == _T("") || pDoc->m_videoFileName == _T(""))createDirectory();
			CString initials;
			m_edtPrefix.GetWindowText(initials);
			initials.TrimLeft();
			initials.TrimRight();
			
			pDoc->m_aviFileNameA.Format("%s%s_%s%s_%s_V%03d.avi", pDoc->m_VideoFolder, initials, pDoc->m_VideoTimeStamp, pDoc->m_VideoFolderSuffix, pDoc->m_waveLength, ++pDoc->m_nVideoNum);
			if (g_bFFTIsRunning) {
				pDoc->m_txtFileName.Format ("%s%s_%s%s_%s_V%03d.csv", pDoc->m_VideoFolder, initials, pDoc->m_VideoTimeStamp, pDoc->m_VideoFolderSuffix, pDoc->m_waveLength, pDoc->m_nVideoNum);
				m_bSaveDewarpImage?pDoc->m_aviFileNameB.Format("%s%s_%s%s_%s_V%03d_stabilized.avi", pDoc->m_VideoFolder, initials, pDoc->m_VideoTimeStamp, pDoc->m_VideoFolderSuffix, pDoc->m_waveLength, pDoc->m_nVideoNum):0;
			}
			CString vidLength;
			m_edtVideoLen.GetWindowText(vidLength);
			netmsg.Format("ICANDI_VIDEOINFO#%s,%s,%s,%s,%s,%03d,%s", initials, pDoc->m_VideoTimeStamp, pDoc->m_VideoFolderSuffix, pDoc->m_waveLength, pDoc->m_VideoFolder, pDoc->m_nVideoNum, vidLength);
			pDoc->SendNetMessage(netmsg);
			
		}
		
		text = pDoc->m_VideoFolder.Left(pDoc->m_VideoFolder.ReverseFind('\\'));
		text = text.Right(text.GetLength()-text.ReverseFind('\\')-1);	
		m_edtFolder.SetWindowText(text);
		text = pDoc->m_aviFileNameA.Right(8);//was: text = pDoc->m_aviFileNameA.Right(pDoc->m_aviFileNameA.GetLength()-pDoc->m_aviFileNameA.ReverseFind('\\')-1);	
		m_edtFilename.SetWindowText(text);

		if (g_bFFTIsRunning) {
			if (pDoc->m_fpStimPos != NULL) fclose(pDoc->m_fpStimPos);
			pDoc->m_fpStimPos = fopen(pDoc->m_txtFileName, "w");
			if (pDoc->m_fpStimPos == NULL) {
				text = "File [" + pDoc->m_txtFileName + "] can not be opened. Stimulus will not be saved.";
				MessageBox(text, "Saving Stimulus Position", MB_ICONWARNING);
			}
		}

		// with valid file name, now create an avi handle to save the raw video
		if (aoslo_movie.avi_handle_on_A == TRUE) 
			//pDoc->m_aviFileA.release();		
			pDoc->m_aviFileA.close();
		CT2CA szaviFileA(_T(pDoc->m_aviFileNameA));
		std::string straviFileA(szaviFileA);
		pDoc->m_aviFileA.open(straviFileA.c_str(), 0, GRABBER_FRAME_RATE /*30*/, pDoc->m_frameSizeA, false);
		if (pDoc->m_aviFileA.isOpened() == TRUE) {
			pDoc->m_bValidAviHandleA = TRUE;
			aoslo_movie.avi_handle_on_A = TRUE;
		} else {
			pDoc->m_bValidAviHandleA = FALSE;
			aoslo_movie.avi_handle_on_A = FALSE;
			MessageBox("AVI initialization failed for raw video.", "Saving Video", MB_ICONWARNING);
			return;
		}

		if (m_bSaveDewarpImage == TRUE && g_bFFTIsRunning) {
			if (aoslo_movie.avi_handle_on_B == TRUE) 
				//pDoc->m_aviFileB.release();
				pDoc->m_aviFileB.close();
			CT2CA szaviFileB(_T(pDoc->m_aviFileNameB));
			std::string straviFileB(szaviFileB);
			pDoc->m_aviFileB.open(straviFileB.c_str(), 0, GRABBER_FRAME_RATE /*30*/, pDoc->m_frameSizeB, false);
			if (pDoc->m_aviFileB.isOpened() == TRUE) {
				pDoc->m_bValidAviHandleB = TRUE;
				aoslo_movie.avi_handle_on_B = TRUE;
			} else {
				pDoc->m_bValidAviHandleB = FALSE;
				aoslo_movie.avi_handle_on_B = FALSE;
				MessageBox("AVI initialization failed for dewarpped video.", "Saving Video", MB_ICONWARNING);
				return;
			}
		}
		
		m_edtVideoLen.GetWindowText(text);
		pDoc->m_nMovieLength = GRABBER_FRAME_RATE/*30*/*atoi(text);
		if (pDoc->m_bValidAviHandleA == TRUE && pDoc->m_bValidAviHandleB == TRUE) {
			// maximum length of video will be truncated to 40 seconds
			if (pDoc->m_nMovieLength > 9000) {
				pDoc->m_nMovieLength = 9000;
			}
		} else if (pDoc->m_bValidAviHandleA == TRUE && pDoc->m_bValidAviHandleB == FALSE) {
			// maximum length of video will be 3 minutes
			if (pDoc->m_nMovieLength > 9000) {
				pDoc->m_nMovieLength = 9000;
			}
		}
		
		text.Format("%d", pDoc->m_nMovieLength/GRABBER_FRAME_RATE/*30*/);
		m_edtVideoLen.SetWindowText(text);

		pDoc->m_iSavedFramesA = 0;
		pDoc->m_iSavedFramesB = 0;

		m_btnDewarpName.EnableWindow(FALSE);
		SetDlgItemText(ID_SAVE_VIDEO, "Stop Saving...");
		if(g_bMatlabCtrl) 
			g_bMatlab_Update = TRUE;
		
		g_bRecord = TRUE;
		Out32(57424,1);
		if(g_bMatlabCtrl || (g_bMatlabVideo && g_bMatlabAVIsavevideo)) 
			g_nCurFlashCount = 0, g_bMatlab_Update = TRUE;
		pDoc->m_bDumpingVideo = FALSE;

	} else {

		if (pDoc->m_fpStimPos != NULL) {
			fclose(pDoc->m_fpStimPos);
			pDoc->m_fpStimPos = NULL;
		}
		
		pDoc->m_iSavedFramesB    = 0;
		pDoc->m_bValidAviHandleA? pDoc->m_aviFileA.close/*release*/(), pDoc->m_bValidAviHandleA = FALSE : 0;
		pDoc->m_bValidAviHandleB? pDoc->m_aviFileB.close/*release*/(), pDoc->m_bValidAviHandleB = FALSE : 0;
		
		m_btnDewarpName.EnableWindow(TRUE);

		SetDlgItemText(ID_SAVE_VIDEO, "Save Video");
		Out32(57424,0);

		aoslo_movie.RandDelivering = FALSE;
		m_array.RemoveAll();
		m_array.Add(0);
		m_array.Add(0);
		m_array.Add(RANDOM_DELIVERY_DONE);
		
		pDoc->UpdateAllViews(this, 0L, &m_array);
		if(pDoc->m_bExtCtrl || g_bMatlabCtrl) {
	//		PlaySound(MAKEINTRESOURCE(IDW_SOUND),AfxGetInstanceHandle(),SND_RESOURCE | SND_ASYNC);
			pDoc->m_bExtCtrl = FALSE;
		//	g_bMatlabCtrl = FALSE;
		}
	}

	OnPaint();
}

void CICANDIView::LoadRawVideoName()
{
	CString     filename;
	int         pos;
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	CFolderDlg dlg(TRUE, pDoc->m_aviFileNameA, this);
	dlg.SetTitle("Folder Browser");
	if (dlg.DoModal() == IDCANCEL) return;
	pDoc->m_VideoFolder = dlg.GetFolderName() + "\\";
	createDirectory();
	pDoc->m_aviFileNameA = pDoc->m_videoFileName;
	pDoc->m_aviFileNameA = pDoc->m_aviFileNameA + filename + "_.avi";

	pDoc->m_aviFileNameA.TrimLeft();
	pDoc->m_aviFileNameA.TrimRight();
	if (pDoc->m_aviFileNameA.GetLength() == 0) {
		SetDlgItemText(IDL_VIDEO_FILENAME, _T(""));
	} else {
		SetDlgItemText(IDL_VIDEO_FILENAME, pDoc->m_aviFileNameA);
	}

	if (m_bSaveDewarpImage == TRUE) {
		GetDlgItemText(IDL_VIDEO_FILENAME, pDoc->m_aviFileNameA);

		pos = pDoc->m_aviFileNameA.ReverseFind('.');
		filename = pDoc->m_aviFileNameA.Left(pos);
		filename = filename + _T("_stabilized.avi");

		SetDlgItemText(IDC_EDIT_DEWARP_NAME, filename);
		pDoc->m_aviFileNameB = filename;
	}

	OnPaint();
}

void CICANDIView::EnableOCTSync() {
	CICANDIDoc* pDoc = GetDocument();	
	if (m_chkSyncOCT.GetCheck() == 0) {
		m_chkSyncOCT.SetCheck(1);
		//check if the folder is populated and disable save video button
		if (pDoc->m_VideoFolder == _T("") || pDoc->m_videoFileName == _T(""))createDirectory();
		m_btnSaveVideo.EnableWindow(FALSE);
		g_bSyncOCT = TRUE;
		PulseEvent(g_eSyncOCT);
		// start the thread for 
	} else if (m_chkSyncOCT.GetCheck() == 1) {
		m_chkSyncOCT.SetCheck(0);
		g_bSyncOCT = FALSE;
		m_btnSaveVideo.EnableWindow(TRUE);
	} else {
		m_chkSyncOCT.SetCheck(0);
		g_bSyncOCT = FALSE;
		MessageBox("Undertermined");
	}	
}

void CICANDIView::RawNameKillFocus() {
	CString     filename;
	int         pos;
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bSaveDewarpImage == TRUE) {
		GetDlgItemText(IDL_VIDEO_FILENAME, pDoc->m_aviFileNameA);

		pos = pDoc->m_aviFileNameA.ReverseFind('.');
		filename = pDoc->m_aviFileNameA.Left(pos);
		filename = filename + _T("_stabilized.avi");

		SetDlgItemText(IDC_EDIT_DEWARP_NAME, filename);
		pDoc->m_aviFileNameB = filename;
	}
}

void CICANDIView::LoadDewarpedVideo()
{
	CString     filename;
//	int         pos;
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (m_btnDewarpName.GetCheck() == 0 || m_bSaveDewarpImage == 0) {
		m_btnDewarpName.SetCheck(1);
		m_bSaveDewarpImage = TRUE;
	} else if (m_btnDewarpName.GetCheck() == 1 || m_bSaveDewarpImage == 1) {
		m_btnDewarpName.SetCheck(0);
	//	SetDlgItemText(IDC_EDIT_DEWARP_NAME, _T(""));
		m_bSaveDewarpImage = FALSE;
	} else {
		m_btnDewarpName.SetCheck(0);
		MessageBox("Undertermined");
	}

	OnPaint();
}

// Click this button to start FFT stabilization
void CICANDIView::StablizationGo()
{
	CString text;

	if (g_bTimingTest) {
		fopen_s(&g_fpTimeLog, "TimeLog.txt", "w");
		if (g_fpTimeLog == NULL) AfxMessageBox("can't open file");
		fprintf(g_fpTimeLog, "Time log\n");
	}

	m_bStablizationOn  = TRUE;
	g_bFFTIsRunning    = TRUE;


	m_start = clock();

	g_frameIndex    = 0;
	g_BlockIndex    = 0;
	
	GetDlgItemText(ID_STABLIZE_GO, text);
	if (text == _T("Go")) {
		SetDlgItemText(ID_STABLIZE_GO, "Reset");
		LoadDewarpedVideo();
	} else {
		CICANDIDoc* pDoc = GetDocument();
		ASSERT_VALID(pDoc);

		/*// add these codes to clear issues with 'reset' button
		StablizationSuspend();
		Sleep(5);
		SetDlgItemText(ID_STABLIZE_GO, "Reset");
		m_bStablizationOn  = TRUE;
		g_bFFTIsRunning    = TRUE;*/

		// send message to the other three panels to show or hide real-time information
		m_array.RemoveAll();
		//m_array.Add(CLEAR_STIMULUS_POINT);
		m_array.Add(0);
		m_array.Add(0);
		m_array.Add(RESET_REF_FRAME);
		
		g_objVirtex5BMD.AppMotionTraces(g_hDevVirtex5, 0.f, 0.f, 0.0f, 0.0f, NULL); // reset mirror motion
		pDoc->UpdateAllViews(this, 0L, &m_array);
	}
}

// Click this button to suspend FFT stabilization
void CICANDIView::StablizationSuspend()
{
	double      elapsed_time;
	CString     msg;
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	if (g_bTimingTest && g_fpTimeLog != NULL) {
		fclose(g_fpTimeLog);
	}

	m_btnGo.EnableWindow(TRUE);
	g_bFFTIsRunning = FALSE;

	g_StimulusPos.x     = -1;
	g_StimulusPos.y     = -1;

	m_finish = clock();
	elapsed_time = (1.0*(m_finish - m_start))/CLOCKS_PER_SEC;
	if (m_bStablizationOn == TRUE) {
		msg.Format("Elapsed time: %f seconds, process %d frames", elapsed_time, g_frameIndex);
//		MessageBox(msg);
		m_bStablizationOn = FALSE;
	}

	g_frameIndex          = 0;
	g_BlockIndex          = 0;
	pDoc->m_iSavedFramesA = 0;
	pDoc->m_iSavedFramesB = 0;
	m_bStablizationOn     = FALSE;

	GetDlgItemText(ID_STABLIZE_GO, msg);
	if (msg == _T("Reset")) {
		SetDlgItemText(ID_STABLIZE_GO, "Go");
		LoadDewarpedVideo();
	}
	g_objVirtex5BMD.AppMotionTraces(g_hDevVirtex5, 0.f, 0.f, 0.0f, 0.0f, NULL); // reset mirror motion
}

void CICANDIView::EnableSLR(){
	CICANDIDoc* pDoc = GetDocument();
	g_bRunSLR = FALSE;	
	if (m_chkSLR.GetCheck() == 0) {
		g_bRunSLR = TRUE;	
		m_chkSLR.SetCheck(1);
		UpdateOldRef();
	} else if (m_chkSLR.GetCheck() == 1) {
		m_chkSLR.SetCheck(0);
		g_bRunSLR = FALSE;
	} else {
		m_chkSLR.SetCheck(0);
		g_bRunSLR = FALSE;
		MessageBox("Undertermined");
	}	
}

void CICANDIView::UpdateOldRef()
{	
	CICANDIDoc* pDoc = GetDocument();	
	char     BASED_CODE szFilter[] = "TIF Files (*.tif)|*.tif|";
	CString oldreffname, oldreffname_bk;
	// open an old ref file
	CFileDialog fd(TRUE, "tif", NULL, NULL, szFilter);
	if (fd.DoModal() != IDOK) return;
	
	// get the path and file name of this bmp file
	oldreffname = fd.GetPathName();
	oldreffname_bk = fd.GetPathName();
	
	oldreffname = oldreffname.Left(oldreffname.GetLength()-(oldreffname.GetLength()-oldreffname.ReverseFind('.')));
	pDoc->m_cpOldLoc_bk.y = pDoc->m_cpOldLoc.y = atoi(oldreffname.Right(oldreffname.GetLength()-oldreffname.ReverseFind('_')-1));
	oldreffname = oldreffname.Left(oldreffname.GetLength()-(oldreffname.GetLength()-oldreffname.ReverseFind('_')));
	pDoc->m_cpOldLoc_bk.x = pDoc->m_cpOldLoc.x = atoi(oldreffname.Right(oldreffname.GetLength()-oldreffname.ReverseFind('_')-1));
	
	if (pDoc->m_cpOldLoc.x <= 0 || pDoc->m_cpOldLoc.y <= 0 || pDoc->m_cpOldLoc.x > (aoslo_movie.width-1) || pDoc->m_cpOldLoc.y > (aoslo_movie.height-1)) {
		MessageBox("Old reference stimulus location not found", "ICANDI", MB_ICONWARNING);
		pDoc->m_cpOldLoc_bk.x = pDoc->m_cpOldLoc.x = 0;
		pDoc->m_cpOldLoc_bk.y = pDoc->m_cpOldLoc.y = 0;
		return;
	}

	try {
		Mat extRef;
		CT2CA szaviFileA(_T(oldreffname_bk));
		std::string straviFileA(szaviFileA);
		extRef = imread(straviFileA, 0);
		memcpy(pDoc->m_pOldRef, extRef.data, aoslo_movie.height*aoslo_movie.width);
	}
	catch (...)
	{
		AfxMessageBox("Unable to load reference image", MB_ICONWARNING);
	}

	return;	
}

LRESULT CICANDIView::OnSendMovie(WPARAM wParam, LPARAM lParam)
{
	m_msgID = lParam;
	CICANDIDoc* pDoc = GetDocument();
	CString initials;
//	ASSERT_VALID(pDoc);

	switch (m_msgID) {
	case MOVIE_HEADER_ID:
		m_btnGo.EnableWindow(TRUE);
		m_btnSaveVideo.EnableWindow(TRUE);
		m_chkSyncOCT.EnableWindow(TRUE);
		if (pDoc->m_bMatlab) {
			m_chkSLR.EnableWindow(TRUE);
			m_btnUpdateOldRef.EnableWindow(TRUE);
		}
		break;

	case SENDING_MOVIE:		// event when main control is sending movie
		m_bMessageArrive = TRUE;
		break;

	case SEND_MOVIE_DONE:	// event when main control stops sending movie 
		m_bStablizationOn  = FALSE;
		m_btnGo.EnableWindow(FALSE);
		m_btnSaveVideo.EnableWindow(FALSE);
		if (pDoc->m_bValidAviHandleA == TRUE || pDoc->m_bValidAviHandleB == TRUE) 
			SaveVideoCommand(); 
		break;

	case STABILIZATION_GO:
		StablizationGo();
		break;

	case STABILIZATION_STOP:
		if (pDoc->m_bValidAviHandleA == TRUE || pDoc->m_bValidAviHandleB == TRUE) 
			SaveVideoCommand();
		StablizationSuspend();
		break;

	case ID_GRAB_STOP:		// event when matrox grabber is closed
		StablizationSuspend();
		m_btnGo.EnableWindow(FALSE);
		m_bMessageArrive = FALSE;
		pDoc->m_bCameraConnected = FALSE;

		break;
	case SAVE_VIDEO_FLAG:
		SaveVideoCommand();
		break;
	case UPDATE_LASERS:
		UpdateLasersPowerStatus();
		break;
	case UPDATE_USER_TCA:		
		initials.Format("%d", g_RGBClkShiftsUser[0].x);				
		SetDlgItemText(IDE_TCA_RED_X, initials);
		initials.Format("%d", g_RGBClkShiftsUser[0].y);				
		SetDlgItemText(IDE_TCA_RED_Y, initials);
		initials.Format("%d", g_RGBClkShiftsUser[1].x);				
		SetDlgItemText(IDE_TCA_GR_X, initials);
		initials.Format("%d", g_RGBClkShiftsUser[1].y);				
		SetDlgItemText(IDE_TCA_GR_Y, initials);
		UpdateUserTCA();
		break;
	case UPDATE_AOMS_STATE:
		m_chkIRAOM.SetCheck(aoslo_movie.bIRAomOn);
		m_chkRedAOM.SetCheck(aoslo_movie.bRedAomOn);
		m_chkGrAOM.SetCheck(aoslo_movie.bGrAomOn);
		break;
	case UPDATE_PROCESSOR:
		OnPaint();
		Invalidate(FALSE);
		break;
	default:
		break;
	}
	
	if (pDoc->m_bDumpingVideo == TRUE) {
	}

	if (m_bMessageArrive) {
		OnPaint();
		Invalidate(FALSE);
	} 

	return 0;
}


// show real-time information
void CICANDIView::OnPaint() 
{
	CPaintDC dc(this); // device context for painting

	m_processor.SetWindowText(_T("FFT on GPU"));
}

// handle two slider bars for adjusting white/black reference levels of
// the Matrox grabber.
void CICANDIView::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CString msg;
    int		nTemp1, nTemp2, upBound;
//	UINT32  regR, regW;
	CICANDIDoc* pDoc = GetDocument();
	CString filename;

	nTemp1 = pScrollBar->GetScrollPos();

	switch (pScrollBar->GetDlgCtrlID()) {
	case ID_STIMULUS_PREDICTION_SLIDER:
		upBound = 15;
		break;
	case ID_FLASH_FREQ_SLIDER:
		upBound = 7;
		break;
	case ID_FLASH_DUTY_SLIDER:
		upBound = GRABBER_FRAME_RATE / aoslo_movie.FlashFrequency - 2;
		break;
	case IDC_LASER_POWER_SLIDER:
		upBound = 100;
		break;
	case ID_VOLTS_PER_DEG_SLIDER:
		upBound = 401;
		break;
	case ID_MOTION_ANGLE_SLIDER:
		upBound = 300;
		break;
	case ID_STIM_GAIN_CONTROL:
		upBound = 600;
		break;
	case ID_MASK_GAIN_CONTROL:
		upBound = 600;
		break;	
	case IDC_LASER_POWER_14BITS:
		m_irLaser.GetCheck() == 1?(m_bCalib?upBound = 255 /*calibration*/:upBound = 100/*real system*/):(m_bCalib?upBound = 16383/*calibration*/:upBound = 1000/*real system*/);
		break;
	default:
		return;
	}

    switch(nSBCode) {
    case SB_THUMBPOSITION:
        pScrollBar->SetScrollPos(nPos);
		nTemp1 = nPos;
        break;
    case SB_LINELEFT: // left arrow button
		m_bCalib?(m_irLaser.GetCheck() == 1?nTemp2 = 10:nTemp2 = 250)/*calibration*/:nTemp2 = 1/*real system*/;
        if ((nTemp1 - nTemp2) >= 0) {
            nTemp1 -= nTemp2;
        } else {
            nTemp1 = 0;
        }
        pScrollBar->SetScrollPos(nTemp1);
        break;
    case SB_LINERIGHT: // right arrow button
		m_bCalib?(m_irLaser.GetCheck() == 1?nTemp2 = 10:nTemp2 = 250)/*calibration*/:nTemp2 = 1/*real system*/;		
        if ((nTemp1 + nTemp2) < upBound) {
            nTemp1 += nTemp2;
        } else {
            nTemp1 = upBound;
        }
        pScrollBar->SetScrollPos(nTemp1);
        break;
    case SB_PAGELEFT: // left arrow button
        nTemp2 = 10;
        if ((nTemp1 - nTemp2) > 0) {
            nTemp1 -= nTemp2;
        } else {
            nTemp1 = 1;
        }
        pScrollBar->SetScrollPos(nTemp1);
        break;
    case SB_PAGERIGHT: // right arrow button
        nTemp2 = 10;
        if ((nTemp1 + nTemp2) < upBound) {
            nTemp1 += nTemp2;
        } else {
            nTemp1 = upBound;
        }
        pScrollBar->SetScrollPos(nTemp1);
        break;
	case SB_THUMBTRACK:
        pScrollBar->SetScrollPos(nPos);
		nTemp1 = nPos;
		break;
    } 
	
	UpdateData(TRUE);

	switch (pScrollBar->GetDlgCtrlID()) {	
	case ID_FLASH_FREQ_SLIDER:
		if (nTemp1 < 15) {
			aoslo_movie.FlashFrequency = m_StimFreq[nTemp1];
			msg.Format("%d Hz", aoslo_movie.FlashFrequency);
		} else {
			msg = "err";
		}
		m_lblFlashFrequency.SetWindowText(msg);
		// update flash duty cycle
		upBound = GRABBER_FRAME_RATE / aoslo_movie.FlashFrequency - 2;
		m_sldDutyCycle.SetScrollRange(0, upBound, TRUE);		// set flash duty cycle
		m_sldDutyCycle.SetScrollPos(0, TRUE);			// default, first position
		msg.Format("1:%d", upBound+1);
		m_lblFlashDutyCycle.SetWindowText(msg);
		
		aoslo_movie.FlashDist = GRABBER_FRAME_RATE/aoslo_movie.FlashFrequency;
		aoslo_movie.FlashDutyCycle = 0;

		break;
	case ID_FLASH_DUTY_SLIDER:
		msg.Format("%d:%d", nTemp1+1, upBound-nTemp1+1);
		m_lblFlashDutyCycle.SetWindowText(msg);
		aoslo_movie.FlashDutyCycle = nTemp1;
		break;
	case ID_VOLTS_PER_DEG_SLIDER:
		if(m_bVoltsPerDegY) {
			g_ICANDIParams.VRangePerDegY = (0.199+(float)nTemp1/1000.);
			g_Motion_ScalerY = (float)(g_ICANDIParams.VRangePerDegY*128./g_ICANDIParams.PixPerDegY);
			msg.Format("%1.3f V/d", g_ICANDIParams.VRangePerDegY);
			m_lblVoltsPerDegValue.SetWindowText(msg);
			// save the value
			filename = g_ICANDIParams.m_strConfigPath + _T("\\AppParams.ini");
			msg.Format("%1.3f", g_ICANDIParams.VRangePerDegY);
			::WritePrivateProfileString("ApplicationInfo", "VoltsPerDegY", msg, filename);
		}
		else {
			g_ICANDIParams.VRangePerDegX = (0.199+(float)nTemp1/1000.);
			g_Motion_ScalerX = (float)(g_ICANDIParams.VRangePerDegX*128./g_ICANDIParams.PixPerDegX);
			msg.Format("%1.3f V/d", g_ICANDIParams.VRangePerDegX);
			m_lblVoltsPerDegValue.SetWindowText(msg);
			// save the value
			filename = g_ICANDIParams.m_strConfigPath + _T("\\AppParams.ini");
			msg.Format("%1.3f", g_ICANDIParams.VRangePerDegX);
			::WritePrivateProfileString("ApplicationInfo", "VoltsPerDegX", msg, filename);
		}
		break;
	case ID_MOTION_ANGLE_SLIDER:
		if(m_bMotionAngleY) {
			g_ICANDIParams.MotionAngleY = float(nTemp1-150)/5.;			
			msg.Format("%2.2f Deg", g_ICANDIParams.MotionAngleY);
			m_lblMotionAngleValue.SetWindowText(msg);
			// save the value
			filename = g_ICANDIParams.m_strConfigPath + _T("\\AppParams.ini");
			msg.Format("%2.2f", g_ICANDIParams.MotionAngleY);
			::WritePrivateProfileString("ApplicationInfo", "MotionAngleY", msg, filename);
		}
		else {			
			g_ICANDIParams.MotionAngleX = float(nTemp1-150)/5.;			
			msg.Format("%2.2f Deg", g_ICANDIParams.MotionAngleX);
			m_lblMotionAngleValue.SetWindowText(msg);
			// save the value
			filename = g_ICANDIParams.m_strConfigPath + _T("\\AppParams.ini");
			msg.Format("%2.2f", g_ICANDIParams.MotionAngleX);
			::WritePrivateProfileString("ApplicationInfo", "MotionAngleX", msg, filename);
		}
		break;
	case ID_STIM_GAIN_CONTROL:
		aoslo_movie.fStabGainStim = (float)((nTemp1-300.)/100.);
		msg.Format("%3.2f", aoslo_movie.fStabGainStim);
		m_lblStimGainVal.SetWindowText(msg);
		break;
	case ID_MASK_GAIN_CONTROL:
		aoslo_movie.fStabGainMask = (float)((nTemp1-300.)/100.);
		msg.Format("%3.2f", aoslo_movie.fStabGainMask);
		m_lblMaskGainVal.SetWindowText(msg);
		break;
	case IDC_LASER_POWER_14BITS:
		double currentpower;
		if (m_redLaser.GetCheck() == 1) {
			g_ncurRedPos = nTemp1;
			if (m_chkRedAOM.GetCheck() == 1) {
				m_bCalib? g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, g_ncurRedPos)/*calibration*/
					:g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, g_usRed_LUT[g_ncurRedPos]),
					aoslo_movie.nLaserPowerRed = g_usRed_LUT[g_ncurRedPos]/*real system*/; 				
			} else {
				aoslo_movie.nLaserPowerRed = 0;
			}
			!m_bCalib?m_nRedPowerOld = g_usRed_LUT[g_ncurRedPos]/*real system*/:0;
			currentpower = g_dRedMax*g_ncurRedPos/1000.;
			msg.Format("%3.1f",((double)g_ncurRedPos)*0.1); //to display current power in percentage
			msg +="%";		
			m_lblLaserPowerPer.SetWindowText(msg);
		} else if (m_grLaser.GetCheck() == 1) {			
			g_ncurGreenPos = nTemp1;
			if (m_chkGrAOM.GetCheck() == 1) {
				m_bCalib? g_objVirtex5BMD.AppUpdate14BITsLaserGR(g_hDevVirtex5, g_ncurGreenPos)/*calibration*/
					:g_objVirtex5BMD.AppUpdate14BITsLaserGR(g_hDevVirtex5, g_usGreen_LUT[g_ncurGreenPos]),
					aoslo_movie.nLaserPowerGreen = g_usGreen_LUT[g_ncurGreenPos]/*real system*/;				
			} else {
				aoslo_movie.nLaserPowerGreen = 0;
			}					
			!m_bCalib?m_nGreenPowerOld = g_usGreen_LUT[g_ncurGreenPos]/*real system*/:0; 
			currentpower = g_dGreenMax*g_ncurGreenPos/1000.;
			msg.Format("%3.1f",((double)g_ncurGreenPos)*0.1); //to display current power in percentage
			msg +="%";
			m_lblLaserPowerPer.SetWindowText(msg);
		} else if (m_irLaser.GetCheck() == 1) {			
			g_ncurIRPos = nTemp1;			
			if (m_chkIRAOM.GetCheck() == 1) {
				m_bCalib? g_objVirtex5BMD.AppUpdateIRLaser(g_hDevVirtex5, (BYTE)g_ncurIRPos)/*calibration*/
					:g_objVirtex5BMD.AppUpdateIRLaser(g_hDevVirtex5, g_usIR_LUT[g_ncurIRPos]),
					aoslo_movie.nLaserPowerIR = g_usIR_LUT[g_ncurIRPos]/*real system*/;				
			} else {
				aoslo_movie.nLaserPowerIR = 0;
			}
			!m_bCalib?m_nIRPowerOld = g_usIR_LUT[g_ncurIRPos]/*real system*/:0;

			currentpower = g_dIRMax*g_ncurIRPos/100.;
			msg.Format("%3.1f",((double)g_ncurIRPos)); //to display current power in percentage
			msg +="%";
			m_lblLaserPowerPer.SetWindowText(msg);			
		}
		if (!currentpower)
			msg = "0.000";
		else {
			msg.Format("%3.3f",currentpower);
		}
		m_edtLaserPower.SetWindowText(msg);
		UpdateOtherUnits();	

		// draw stimulus when there is no live video
		if (!((CICANDIApp*)AfxGetApp())->m_isGrabStarted && g_StimulusPosBak.x>0 && g_StimulusPosBak.y>0) {
			pDoc->DrawStimulus();
		}
		break;
	}

	CView::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CICANDIView::OnEnablePupilMask(){
	aoslo_movie.bPupilMask = FALSE;
	if (m_chkPupilMask.GetCheck() == 0) {
		aoslo_movie.bPupilMask = TRUE;
		m_chkPupilMask.SetCheck(1);
		m_lblMaskGain.EnableWindow(TRUE);
		m_lblMaskGainVal.EnableWindow(TRUE);
		m_sldMaskGainCtrl.EnableWindow(TRUE);
		
	} else if (m_chkPupilMask.GetCheck() == 1) {
		aoslo_movie.bPupilMask = FALSE;
		m_chkPupilMask.SetCheck(0);		
		m_lblMaskGain.EnableWindow(FALSE);
		m_lblMaskGainVal.EnableWindow(FALSE);
		m_sldMaskGainCtrl.EnableWindow(FALSE);
	} else {
		m_chkPupilMask.SetCheck(0);	
		aoslo_movie.bPupilMask = FALSE;
		m_lblMaskGain.EnableWindow(FALSE);
		m_lblMaskGainVal.EnableWindow(FALSE);
		m_sldMaskGainCtrl.EnableWindow(FALSE);	
		MessageBox("Undertermined");
	}
	ApplyPupilMask();
}

void CICANDIView::ApplyPupilMask() {
	aoslo_movie.bPupilMask?g_objVirtex5BMD.PupilMaskOn(g_hDevVirtex5):g_objVirtex5BMD.PupilMaskOff(g_hDevVirtex5);	
}


void CICANDIView::ApplyOneFrameDelay()
{
	if (m_chkOneFrameDelay.GetCheck() == 0) {
		m_chkOneFrameDelay.SetCheck(1);
		aoslo_movie.bOneFrameDelay = TRUE;
	} else if (m_chkOneFrameDelay.GetCheck() == 1) {
		m_chkOneFrameDelay.SetCheck(0);
		aoslo_movie.bOneFrameDelay = FALSE;
	} else {
		m_chkOneFrameDelay.SetCheck(0);
		aoslo_movie.bOneFrameDelay = FALSE;
	}
}


void CICANDIView::SwitchREDAOM() {
	if (m_chkRedAOM.GetCheck() == 0) {
		m_chkRedAOM.SetCheck(1);
		// this operation turns AOM on physically.
		//g_objVirtex5BMD.AppAOM_On(g_hDevVirtex5);
		aoslo_movie.nLaserPowerRed = m_nRedPowerOld;
		aoslo_movie.bRedAomOn = TRUE;
	} else if (m_chkRedAOM.GetCheck() == 1) {
		m_chkRedAOM.SetCheck(0);
		// this operation turns AOM off physically.
		//g_objVirtex5BMD.AppAOM_Off(g_hDevVirtex5);
		m_nRedPowerOld = aoslo_movie.nLaserPowerRed;
		aoslo_movie.nLaserPowerRed = 0;
		aoslo_movie.bRedAomOn = FALSE;
	} else {
		m_chkRedAOM.SetCheck(0);
		MessageBox("Undertermined");
	}
}

void CICANDIView::SwitchGreenAOM() {
	if (m_chkGrAOM.GetCheck() == 0) {
		m_chkGrAOM.SetCheck(1);
		aoslo_movie.nLaserPowerGreen= m_nGreenPowerOld;
		aoslo_movie.bGrAomOn		= TRUE;
	} else if (m_chkGrAOM.GetCheck() == 1) {
		m_chkGrAOM.SetCheck(0);
		m_nGreenPowerOld = aoslo_movie.nLaserPowerGreen;
		aoslo_movie.nLaserPowerGreen= 0;
		aoslo_movie.bGrAomOn		= FALSE;
	} else {
		m_chkGrAOM.SetCheck(0);
		MessageBox("Undertermined");
	}
}

void CICANDIView::SwitchIRAOM() {
	if (m_chkIRAOM.GetCheck() == 0) {
		m_chkIRAOM.SetCheck(1);
		g_objVirtex5BMD.AppIRAOM_On(g_hDevVirtex5);
		aoslo_movie.nLaserPowerIR = m_nIRPowerOld;
		aoslo_movie.bIRAomOn = TRUE;
	} else if (m_chkIRAOM.GetCheck() == 1) {
		m_chkIRAOM.SetCheck(0);
		g_objVirtex5BMD.AppIRAOM_Off(g_hDevVirtex5);
		m_nIRPowerOld = aoslo_movie.nLaserPowerIR;
		aoslo_movie.nLaserPowerIR = 0;
		aoslo_movie.bIRAomOn = FALSE;
	} else {
		m_chkIRAOM.SetCheck(0);		
		MessageBox("Undertermined");
	}
}

void CICANDIView::CalibrateRed() {
	if (m_chkRedCal.GetCheck() == 0) {
		m_chkRedCal.SetCheck(1);
		m_bCalib = true;
		g_objVirtex5BMD.RedCalibrationOn(g_hDevVirtex5);
	} else if (m_chkRedCal.GetCheck() == 1) {
		m_chkRedCal.SetCheck(0);
		m_bCalib = false;
		g_objVirtex5BMD.RedCalibrationOff(g_hDevVirtex5);
	} else {
		m_chkRedCal.SetCheck(0);
		m_bCalib = false;
		MessageBox("Undertermined");
	}

	if (m_redLaser.GetCheck() == 1)
		UpdateRedLaser();
	else if (m_grLaser.GetCheck() == 1)
		UpdateGreenLaser();
	else if (m_irLaser.GetCheck() == 1)
		UpdateIRLaser();
}

void CICANDIView::UpdateRedLaser() {
	CString text;
	CICANDIDoc* pDoc = GetDocument();
	if (m_redLaser.GetCheck() == 0) {
		m_redLaser.SetCheck(1);
		m_grLaser.SetCheck(0);
		m_irLaser.SetCheck(0);		
		m_sldLaserPow14.SetScrollPos(g_ncurRedPos, TRUE);
		if (!g_dRedMax) {
			m_sldLaserPow14.EnableWindow(false);
			m_edtLaserPower.SetWindowText("0.000");
			m_lblLaserPowerTl.SetWindowText("0 Td");
			m_lblLaserPowerPer.SetWindowText("0%");
		} else {	
			m_sldLaserPow14.SetScrollPos(g_ncurRedPos, TRUE);
			m_sldLaserPow14.EnableWindow(true);
			double currentpower = g_dRedMax*(((double)g_ncurRedPos)/1000.);
			text.Format("%3.3f",currentpower);
//			text.TrimRight('0');
			m_edtLaserPower.SetWindowText(text);
			text.Format("%3.1f",((double)g_ncurRedPos)*0.1);
			text +="%";
			m_lblLaserPowerPer.SetWindowText(text);		
			UpdateOtherUnits();
		}
	} else if (m_redLaser.GetCheck() == 1) {
		m_grLaser.SetCheck(0);
		m_irLaser.SetCheck(0);
	} else {
		m_redLaser.SetCheck(1);
		m_grLaser.SetCheck(0);
		m_irLaser.SetCheck(0);
		MessageBox("Undertermined");
	}
	m_bCalib?m_sldLaserPow14.SetScrollRange(0,16383)/*calibration*/:m_sldLaserPow14.SetScrollRange(0,1000)/*real system*/; 
}

void CICANDIView::UpdateGreenLaser() {
	CString text;
	CICANDIDoc* pDoc = GetDocument();
	if (m_grLaser.GetCheck() == 0) {
		m_grLaser.SetCheck(1);
		m_redLaser.SetCheck(0);
		m_irLaser.SetCheck(0);
		m_sldLaserPow14.SetScrollPos(g_ncurGreenPos,TRUE);
		if (!g_dGreenMax) {
			m_sldLaserPow14.EnableWindow(false);
			m_edtLaserPower.SetWindowText("0.000");
			m_lblLaserPowerTl.SetWindowText("0 Td");
			m_lblLaserPowerPer.SetWindowText("0%");
		} else {	
			m_sldLaserPow14.SetScrollPos(g_ncurGreenPos, TRUE);
			m_sldLaserPow14.EnableWindow(true);
			double currentpower = g_dGreenMax*(((double)g_ncurGreenPos)/1000.);
			text.Format("%3.3f",currentpower);
//			text.TrimRight('0');
			m_edtLaserPower.SetWindowText(text);
			text.Format("%3.1f",((double)g_ncurGreenPos)*0.1);
			text +="%";
			m_lblLaserPowerPer.SetWindowText(text);		
			UpdateOtherUnits();
		}
	} else if (m_grLaser.GetCheck() == 1) {
		m_redLaser.SetCheck(0);
		m_irLaser.SetCheck(0);
	} else {
		m_grLaser.SetCheck(1);
		m_irLaser.SetCheck(0);
		m_redLaser.SetCheck(0);
		MessageBox("Undertermined");
	}
	m_bCalib?m_sldLaserPow14.SetScrollRange(0,16383)/*calibration*/:m_sldLaserPow14.SetScrollRange(0,1000)/*real system*/; 
}

void CICANDIView::UpdateIRLaser() {
	CString text;
	CICANDIDoc* pDoc = GetDocument();
	if (m_irLaser.GetCheck() == 0) {
		m_irLaser.SetCheck(1);
		m_redLaser.SetCheck(0);
		m_grLaser.SetCheck(0);		
		m_sldLaserPow14.SetScrollPos(g_ncurIRPos,TRUE);
		if (!g_dIRMax) {
			m_sldLaserPow14.EnableWindow(false);
			m_edtLaserPower.SetWindowText("0.000");
			m_lblLaserPowerTl.SetWindowText("0 Td");
			m_lblLaserPowerPer.SetWindowText("0%");
		} else {	
			m_sldLaserPow14.SetScrollPos(g_ncurIRPos, TRUE);
			m_sldLaserPow14.EnableWindow(true);
			double currentpower = g_dIRMax*(((double)g_ncurIRPos)/100.);
			text.Format("%3.3f",currentpower);
//			text.TrimRight('0');
			m_edtLaserPower.SetWindowText(text);
			text.Format("%3.1f",((double)g_ncurIRPos));
			text +="%";
			m_lblLaserPowerPer.SetWindowText(text);		
			UpdateOtherUnits();
		}
	} else if (m_irLaser.GetCheck() == 1) {
		m_redLaser.SetCheck(0);
		m_grLaser.SetCheck(0);
	} else {
		m_irLaser.SetCheck(1);
		m_grLaser.SetCheck(0);
		m_redLaser.SetCheck(0);
		MessageBox("Undertermined");
	}
	m_bCalib?m_sldLaserPow14.SetScrollRange(0,255)/*calibration*/:m_sldLaserPow14.SetScrollRange(0,100)/*real system*/; 
}

void CICANDIView::UpdateMaxPower(NMHDR* wParam, LRESULT *plr) {
	CString text;
	MSGFILTER * lpMsgFilter = (MSGFILTER *)wParam;
	CICANDIDoc* pDoc = GetDocument();
	
	switch (lpMsgFilter->msg) { // Assuming that you only have one control and don't need to verify the event's source.
	case WM_KEYDOWN:{
			int data=lpMsgFilter->wParam;
			if (data == 13){
				m_edtLaserPower.GetWindowText(text);
				double currentpower;
				double power = atof(text);
				int position = m_sldLaserPow14.GetScrollPos();
				if (m_redLaser.GetCheck() == 1){
					g_dRedMax = atof(text);
					if (g_dRedMax != 0.0){
						m_sldLaserPow14.EnableWindow(true);						
						currentpower = g_dRedMax*((double)position)/1000.; //power display
					}
					else{
						m_sldLaserPow14.SetScrollPos(0, TRUE);
						m_sldLaserPow14.EnableWindow(false);						
						currentpower = 0.;						
					}
				}
				else if (m_grLaser.GetCheck() == 1){
					g_dGreenMax = atof(text);
					if (g_dGreenMax != 0.0){
						m_sldLaserPow14.EnableWindow(true);
						currentpower = g_dGreenMax*((double)position)/1000.; //power display
					}
					else{
						m_sldLaserPow14.SetScrollPos(0,TRUE);
						m_sldLaserPow14.EnableWindow(false);						
						currentpower = 0.;
					}
				}
				else if (m_irLaser.GetCheck() == 1){
					g_dIRMax = atof(text);
					if (g_dIRMax != 0.0){
						m_sldLaserPow14.EnableWindow(true);
						currentpower = g_dIRMax*((double)position)/100.; //power display
					}
					else{
						m_sldLaserPow14.SetScrollPos(0,TRUE);
						m_sldLaserPow14.EnableWindow(false);						
						currentpower = 0.;
					}
				}
				if (!currentpower)
					text = "0.";
				else {
				text.Format("%3.3f",currentpower);
//				text.TrimRight('0');
				}
				m_edtLaserPower.SetWindowText(text);
				UpdateOtherUnits();
				m_sldLaserPow14.SetFocus();
			}
		}
		break;
	}		
}

void CICANDIView::UpdateFieldSize(NMHDR* wParam, LRESULT *plr) {
	CString text;
	MSGFILTER * lpMsgFilter = (MSGFILTER *)wParam;
	
	switch (lpMsgFilter->msg) { // Assuming that you only have one control and don't need to verify the event's source.
	case WM_KEYDOWN:{
			int data=lpMsgFilter->wParam;
			if (data == 13){
				UpdateOtherUnits();
			}
		}
		break;
	}
}

void CICANDIView::UpdateOtherUnits() {
	CString text;
	CICANDIDoc* pDoc = GetDocument();
	double fieldsize, trolands, currentpower;
	int position;
	m_edtFieldSize.GetWindowText(text);
	fieldsize = atof(text);
	position = m_sldLaserPow14.GetScrollPos();
	if (m_redLaser.GetCheck() == 1) {		
		currentpower = ((double)position/1000.)*g_dRedMax;
		trolands = (currentpower/fieldsize)*REDVLAMBDA*2.242*pow(10.0,6);		
	}
	else if (m_grLaser.GetCheck() == 1) {
		currentpower = ((double)position/1000.)*g_dGreenMax;
		trolands = (currentpower/fieldsize)*GRVLAMBDA*2.242*pow(10.0,6);
	}
	else if (m_irLaser.GetCheck() == 1) {
		currentpower = ((double)position/100.)*g_dIRMax;
		trolands = (currentpower/fieldsize)*IRVLAMBDA*2.242*pow(10.0,6);
	}
	text.Format("%3.2f",trolands);
//	text.TrimRight('0');
	text +=" Td";//candles/m2
	m_lblLaserPowerTl.SetWindowText(text);
}

void CICANDIView::LoadStimulus() {	

	CString filename, msg, filetype;
	CICANDIDoc* pDoc = GetDocument();
	BOOL    isAVI;
	BOOL	isFolder;

	// open a stimulus file
	//CFileDialog fd(TRUE, NULL, NULL, NULL, "14 Bit Files (*.buf)|*.buf|Bitmaps (*.bmp)|*.bmp|Targa Files (*.tif)|*.tif|");
	m_btnStimulus.GetWindowText(msg);
	if (msg == _T("Load")) {
		filetype = "8-bit Bitmaps (*.bmp)|*.bmp|14-bit Files (*.buf)|*.buf|";
		isAVI    = FALSE;
		isFolder = FALSE;
	} else 	if (msg == _T("Load Video")){
		filetype = "8-bit/24-bit Uncompressed AVI files (*.avi)|*.avi";
		isAVI    = TRUE;
		isFolder = FALSE;
	}/* else {
		isAVI    = TRUE;
		isFolder = TRUE;
	}*/

	if (!isFolder){
		CFileDialog fd(TRUE, NULL, NULL, NULL, filetype);		
		fd.m_ofn.lpstrInitialDir  = g_ICANDIParams.StimuliPath;

		if (fd.DoModal() != IDOK) {
			if (!pDoc->m_bSymbol)
				MessageBox("No stimulus file is selected, a default 17x17 square will be used as the stimulus pattern", "Loading Stimulus", MB_ICONWARNING);
			return;
		} else {
			pDoc->m_bSymbol = TRUE;
			// get the path and file name of this stimulus file
			filename = fd.GetPathName();		
			filename.TrimLeft();
			filename.TrimRight();
			if (isAVI == FALSE) {
				pDoc->LoadSymbol(filename);
			} else {
				pDoc->LoadStimVideo(&filename, 1);
			}			
		}
	}
	else {
		CString *filenames;		
		filenames = NULL;
		filenames = new CString[20];
		CFolderDlg dlg(TRUE, pDoc->m_aviFileNameA, this);
		dlg.SetTitle("Folder Browser");
		if (dlg.DoModal() == IDCANCEL) return;
		CString foldername;
		foldername = dlg.GetFolderName() ;
		char x[50];	
		CString filename;
		CString data;
		filename = "dir ";
		filename += foldername;
		filename += " /b >temp.txt";	
		system (filename);
		std::ifstream infile;
		infile.open("temp.txt");
		if (!infile) 
			MessageBox("Unable to open file", "List file", MB_ICONERROR);
		int i=-1;
		while (infile.getline(x, 50))
		{
			data = x;
			data.MakeLower();
			if (data.Find(".avi")>0)
			{					
				data = foldername;
				data += "\\";
				data += x;
				filenames[++i] = data;
			}
		}

		if (i == -1) {
			MessageBox("No avi video file found", "Load avi video files", MB_ICONWARNING);
			delete [] filenames;
			return;
		}

		infile.close();
		filename.Empty();
		filename = "del temp.txt";
		system(filename);
		pDoc->LoadStimVideo(filenames, ++i);
		delete [] filenames;		
		aoslo_movie.bWithStimVideo = TRUE;
	}
}

void CICANDIView::LoadMultiStim() {
	CICANDIDoc* pDoc = GetDocument();

	if (m_btnMultiStim.GetCheck() == 0) {
		m_btnMultiStim.SetCheck(1);
		m_btnStimulus.EnableWindow(FALSE);
		pDoc->LoadMultiStimuli();
	} else if (m_btnMultiStim.GetCheck() == 1) {
		m_btnMultiStim.SetCheck(0);
		m_btnStimulus.EnableWindow(TRUE);
		g_bMultiStimuli = FALSE;
	} else {
		m_btnMultiStim.SetCheck(0);
		MessageBox("Undertermined");
		m_btnStimulus.EnableWindow(TRUE);
		g_bMultiStimuli = FALSE;
	}
}

void CICANDIView::MotionScalerChkY() {
	CString msg;
	if (m_chkVoltsPerDegY.GetCheck() == 0) {
		m_chkVoltsPerDegY.SetCheck(1);
		m_bVoltsPerDegY = TRUE;
		m_sldVoltsPerDeg.SetScrollPos(g_ICANDIParams.VRangePerDegY*1000-200, TRUE);
		msg.Format("%1.3f V/d", g_ICANDIParams.VRangePerDegY);
		m_lblVoltsPerDegValue.SetWindowText(msg);
	} else if (m_chkVoltsPerDegY.GetCheck() == 1) {
		m_chkVoltsPerDegY.SetCheck(0);
		m_bVoltsPerDegY = FALSE;	
		m_sldVoltsPerDeg.SetScrollPos(g_ICANDIParams.VRangePerDegX*1000-200, TRUE);
		msg.Format("%1.3f V/d", g_ICANDIParams.VRangePerDegX);
		m_lblVoltsPerDegValue.SetWindowText(msg);
	} else {
		m_chkVoltsPerDegY.SetCheck(0);
		MessageBox("Undertermined");
		m_bVoltsPerDegY = FALSE;
	}
}

void CICANDIView::MotionAngleChkY() {
	CString msg;
	if (m_chkMotionAngleY.GetCheck() == 0) {
		m_chkMotionAngleY.SetCheck(1);
		m_bMotionAngleY = TRUE;
		m_sldMotionAngle.SetScrollPos(g_ICANDIParams.MotionAngleY*5+150, TRUE);
		msg.Format("%2.2f Deg", g_ICANDIParams.MotionAngleY);
		m_lblMotionAngleValue.SetWindowText(msg);
	} else if (m_chkMotionAngleY.GetCheck() == 1) {
		m_chkMotionAngleY.SetCheck(0);
		m_bMotionAngleY = FALSE;	
		m_sldMotionAngle.SetScrollPos(g_ICANDIParams.MotionAngleX*5+150, TRUE);
		msg.Format("%2.2f Deg", g_ICANDIParams.MotionAngleX);
		m_lblMotionAngleValue.SetWindowText(msg);
	} else {
		m_chkMotionAngleY.SetCheck(0);
		MessageBox("Undertermined");
		m_bMotionAngleY = FALSE;
	}
}

void CICANDIView::StimulusVideo() {

	if (m_chkStimVideo.GetCheck() == 0) {
		m_chkStimVideo.SetCheck(1);
		m_btnStimulus.SetWindowText("Load Video");
		m_chkRewindVideo.EnableWindow(TRUE);
		if (aoslo_movie.nStimFrameNum > 0 && aoslo_movie.stim_video != NULL) 
			aoslo_movie.bWithStimVideo = TRUE;
	} else if (m_chkStimVideo.GetCheck() == 1) {
		m_chkStimVideo.SetCheck(0);
		m_btnStimulus.SetWindowText("Load");
		aoslo_movie.bWithStimVideo = FALSE;
		m_chkRewindVideo.EnableWindow(FALSE);
	} else {
		m_chkStimVideo.SetCheck(0);
		MessageBox("Undertermined");
		m_btnStimulus.SetWindowText("Load Video");
		m_chkRewindVideo.EnableWindow(FALSE);
		aoslo_movie.bWithStimVideo = FALSE;
	}
}


void CICANDIView::RewindVideo() {
	if (m_chkRewindVideo.GetCheck() == 0) {
		m_chkRewindVideo.SetCheck(1);
		aoslo_movie.bStimRewind = TRUE;
	} else if (m_chkRewindVideo.GetCheck() == 1) {
		m_chkRewindVideo.SetCheck(0);
		aoslo_movie.bStimRewind = FALSE;
	} else {
		m_chkRewindVideo.SetCheck(0);
		MessageBox("Undertermined");
		aoslo_movie.bStimRewind = FALSE;
	}
}

void CICANDIView::UpdateGain0TrackingStatus() {
	if (m_chkGain0Tracking.GetCheck() == 0) {
		m_chkGain0Tracking.SetCheck(1);
		g_bGain0Tracking = TRUE;
	} else if (m_chkGain0Tracking.GetCheck() == 1) {
		m_chkGain0Tracking.SetCheck(0);
		g_bGain0Tracking = FALSE;
	} else {
		m_chkGain0Tracking.SetCheck(0);
		MessageBox("Undertermined");
		g_bGain0Tracking = FALSE;
	}
}

void CICANDIView::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	CICANDIDoc* pDoc = GetDocument();

	if (pHint == NULL) {
	} else {
		CByteArray  *msgArray = (CByteArray*)pHint;

		BYTE  id0 = msgArray->GetAt(0);
		BYTE  id1 = msgArray->GetAt(1);
		BYTE  id2 = msgArray->GetAt(2);

		if (id2 == SAVE_VIDEO_FLAG) {
			SaveVideoCommand();
		}

		if (id2 == RANDOM_DELIVERY_FLAG) {
			
			ASSERT_VALID(pDoc);
			CString text;
			pDoc->m_nMovieLength = 30*id0*aoslo_movie.StimulusNum;
			text.Format("%d", id0*aoslo_movie.StimulusNum);
			m_edtVideoLen.SetWindowText(text);
			SaveVideoCommand();
		}		
	}

	UpdateWaveLength();
}

void CICANDIView::UpdateDeliveryMode1()
{
	m_btnDeliveryMode1.SetCheck(1);
	m_btnDeliveryMode2.SetCheck(0);
	m_btnDeliveryMode3.SetCheck(0);

	aoslo_movie.DeliveryMode = 0;
	
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_array.RemoveAll();
	m_array.Add(0);
	m_array.Add(0);
	m_array.Add(DELIVERY_MODE_FLAG);		

	pDoc->UpdateAllViews(this, 0L, &m_array);
}

void CICANDIView::UpdateDeliveryMode2()
{
	m_btnDeliveryMode1.SetCheck(0);
	m_btnDeliveryMode2.SetCheck(1);
	m_btnDeliveryMode3.SetCheck(0);

	aoslo_movie.DeliveryMode = 1;
	
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_array.RemoveAll();
	m_array.Add(0);
	m_array.Add(0);
	m_array.Add(DELIVERY_MODE_FLAG);		

	pDoc->UpdateAllViews(this, 0L, &m_array);
}

void CICANDIView::UpdateDeliveryMode3()
{
	m_btnDeliveryMode1.SetCheck(0);
	m_btnDeliveryMode2.SetCheck(0);
	m_btnDeliveryMode3.SetCheck(1);

	aoslo_movie.DeliveryMode = 2;
	
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);

	m_array.RemoveAll();
	m_array.Add(0);
	m_array.Add(0);
	m_array.Add(DELIVERY_MODE_FLAG);		

	pDoc->UpdateAllViews(this, 0L, &m_array);
}

void CICANDIView::SwitchTo788(){
	g_ICANDIParams.Imaging840 = false;
	m_Imaging840.SetCheck(false);
	m_Imaging788.SetCheck(true);
}

void CICANDIView::SwitchTo840(){
	g_ICANDIParams.Imaging840 = true;
	m_Imaging840.SetCheck(true);
	m_Imaging788.SetCheck(false);
}

void CICANDIView::UpdateWaveLength(){
	
	CICANDIDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	m_waveLength.GetLBText(m_waveLength.GetCurSel(), pDoc->m_waveLength);

}

void CICANDIView::UpdateConstPwr(){

	if (g_bConstPwr == true){
		m_chkConstPwr.SetCheck(0);
		g_bConstPwr = false;
	}
	else{
		m_chkConstPwr.SetCheck(1);
		g_bConstPwr = true;
	}

}

void CICANDIView::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	m_tabControl.SetWindowPos(&wndBottom, 0, 0, cx, cy, SWP_SHOWWINDOW);

	m_processor.MoveWindow(cx-100, 0, cx, 20, TRUE);
	m_tabControl.SetWindowPos(&wndBottom, 0, 0, cx, cy, SWP_SHOWWINDOW);
}


void CICANDIView::OnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult)
{
	int   idx = m_tabControl.GetCurSel();

	ShowTabItems(idx);

	*pResult = 0;
}

void CICANDIView::ShowTabItems(int tabIndex) 
{	
	m_frameOne.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_frameTwo.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lasPowerControl.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_redLaser.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_grLaser.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_irLaser.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	//	m_sldLaserPower.ShowWindow(SW_SHOW);
	m_sldLaserPow14.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblLaserPowerPer.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_edtLaserPower.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblLaserPower.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblLaserPowerTl.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_edtFieldSize.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblFeildSize.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_btnGo.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_btnUpdateOldRef.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_chkSLR.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_btnSaveVideo.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_chkRedAOM.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_chkGrAOM.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_chkPupilMask.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_chkOneFrameDelay.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_chkIRAOM.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_chkRedCal.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_fraVideoName.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_edtFolder.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblVideoFolder.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblVideoFileName.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_edtFilename.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_btnDewarpName.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblPrefix.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_edtPrefix.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);		
	m_chkSyncOCT.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_lblVideoLen.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_edtVideoLen.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);
	m_fraStimulusSetup.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);	
	m_lblVoltsPerDeg.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);	
	m_sldVoltsPerDeg.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblVoltsPerDegValue.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_chkVoltsPerDegY.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblMotionAngle.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);	
	m_sldMotionAngle.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblMotionAngleValue.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_chkMotionAngleY.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblFlashFreq.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_sldFlashFreq.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblFlashFrequency.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblFlashDuty.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_sldDutyCycle.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblFlashDutyCycle.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_sldStimGainCtrl.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblStimGain.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblStimGainVal.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);		
	m_sldMaskGainCtrl.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblMaskGain.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblMaskGainVal.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);				
	m_btnDeliveryMode1.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_btnDeliveryMode2.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_btnDeliveryMode3.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_btnMultiStim.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_btnStimulus.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_chkStimVideo.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_chkRewindVideo.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_frameThree.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_fraTCASetup.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_chkGain0Tracking.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);	
	m_fraTCASetup.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_frameFour.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblTCARed.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblTCAGr.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblTCAX.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_lblTCAY.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_TCAMeasure.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_TCAApply.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_TCAOverride.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_edtTCARedX.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_edtTCARedY.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_edtTCAGrX.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_edtTCAGrY.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_chkConstPwr.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_Imaging840.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_Imaging788.ShowWindow(tabIndex==1?SW_SHOW:SW_HIDE);
	m_waveLength.ShowWindow(tabIndex==0?SW_SHOW:SW_HIDE);

}

void CICANDIView::UpdatePrefix(NMHDR* wParam, LRESULT *plr) {
	CString text;
	MSGFILTER * lpMsgFilter = (MSGFILTER *)wParam;
	CICANDIDoc* pDoc = (CICANDIDoc *)GetDocument();
	ASSERT_VALID(pDoc);
	
	switch (lpMsgFilter->msg) { // Assuming that you only have one control and don't need to verify the event's source.
	case WM_KEYDOWN:{
			int data=lpMsgFilter->wParam;
			if (data == 13){
				createDirectory();
			}
		}
		break;
	}		
}

void CICANDIView::createDirectory()
{
	CICANDIDoc* pDoc = (CICANDIDoc *)GetDocument();
	ASSERT_VALID(pDoc);

	CString initials;
	m_edtPrefix.GetWindowText(initials);
	initials.TrimLeft();
	initials.TrimRight();	
	pDoc->GetSysTime(pDoc->m_VideoTimeStamp);
		
	pDoc->m_VideoFolder = g_ICANDIParams.VideoFolderPath + initials + "\\";
	CreateDirectory(pDoc->m_VideoFolder,NULL);	
	pDoc->m_VideoFolder = pDoc->m_VideoFolder + pDoc->m_VideoTimeStamp + pDoc->m_VideoFolderSuffix + "\\";
	CreateDirectory(pDoc->m_VideoFolder,NULL);
	pDoc->m_videoFileName = pDoc->m_VideoFolder+initials;
	pDoc->m_nVideoNum = 0;	

}

void CICANDIView::UpdateLasersPowerStatus() //updates status of both laser powers and respective display items
{
	CICANDIDoc* pDoc = (CICANDIDoc *)GetDocument();
	ASSERT_VALID(pDoc);	
	CString msg;
	double currentpower;
	
	if (m_chkRedAOM.GetCheck() == 1) {
		g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, g_usRed_LUT[g_ncurRedPos]);
		g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, g_usGreen_LUT[g_ncurGreenPos]);
		aoslo_movie.nLaserPowerRed = g_usRed_LUT[g_ncurRedPos];
		aoslo_movie.nLaserPowerGreen = g_usGreen_LUT[g_ncurGreenPos];
	} else {
		aoslo_movie.nLaserPowerRed = 0;
		aoslo_movie.nLaserPowerGreen = 0;
	}
	m_nRedPowerOld = g_usRed_LUT[g_ncurRedPos];
	m_nGreenPowerOld = g_usGreen_LUT[g_ncurGreenPos];
	m_nIRPowerOld = g_usIR_LUT[g_ncurIRPos];
	g_objVirtex5BMD.AppUpdate14BITsLaserRed(g_hDevVirtex5, m_nRedPowerOld);
	g_objVirtex5BMD.AppUpdate14BITsLaserGR(g_hDevVirtex5, m_nGreenPowerOld);
	//g_objVirtex5BMD.AppUpdate8BITsLaserIR(g_hDevVirtex5, (BYTE)m_nIRPowerOld); //<------ need update
	g_objVirtex5BMD.AppUpdateIRLaser(g_hDevVirtex5, (BYTE)m_nIRPowerOld);

	if (m_redLaser.GetCheck() == 1) {
		currentpower = g_dRedMax*((double)g_ncurRedPos/1000.);
		m_sldLaserPow14.SetScrollPos(g_ncurRedPos, TRUE);
		msg.Format("%3.1f",(((double)g_ncurRedPos)*0.1));
	}
	else if (m_grLaser.GetCheck() == 1) {
		currentpower = g_dGreenMax*((double)g_ncurGreenPos/1000.);
		m_sldLaserPow14.SetScrollPos(g_ncurGreenPos, TRUE);
		msg.Format("%3.1f",(((double)g_ncurGreenPos)*0.1));
	}
	else {
		currentpower = g_dIRMax*((double)g_ncurIRPos/100.);
		m_sldLaserPow14.SetScrollPos(g_ncurIRPos, TRUE);
		msg.Format("%3.1f",((double)g_ncurIRPos));
	}
	//update display	
	msg +="%";
	m_lblLaserPowerPer.SetWindowText(msg);
	if (!currentpower)
		msg = "0";
	else {
		msg.Format("%3.3f",currentpower);
//		msg.TrimRight('0');
	}
	m_edtLaserPower.SetWindowText(msg);
	UpdateOtherUnits();
}

void CICANDIView::OnMeasureTCA() {
	CString text;
	GetDlgItemText(IDB_TCA_MEASURE, text);
	if (text == _T("Measure")) {
		m_TCAOverride.SetCheck(0);
		m_TCAOverride.EnableWindow(0);
		m_TCAApply.EnableWindow(1);
		m_edtTCARedX.EnableWindow(0);
		m_edtTCARedY.EnableWindow(0);
		m_edtTCAGrX.EnableWindow(0);
		m_edtTCAGrY.EnableWindow(0);
		// Reset values to 0 needed???
		// Turn on plots and individual frames display

		SetDlgItemText(IDB_TCA_MEASURE, "Stop");
	}
	else {
		SetDlgItemText(IDB_TCA_MEASURE, "Measure");
		m_TCAOverride.EnableWindow(1);
		m_TCAApply.EnableWindow(1);
	}
}

void CICANDIView::OnApplyTCA() {
	if (m_TCAApply.GetCheck() == 0) {
		m_TCAApply.SetCheck(1);
		g_ICANDIParams.ApplyTCA = TRUE;
		// If measuring TCA, stop it 
		//OnMeasureTCA();
		m_TCAApply.EnableWindow(1);	
		ApplyTCA();
	} else if (m_TCAApply.GetCheck() == 1) {
		m_TCAApply.SetCheck(0);	
		g_ICANDIParams.ApplyTCA = FALSE;
		if (m_TCAOverride.GetCheck() == 1) {
			m_TCAMeasure.EnableWindow(0);
		} else {
			m_TCAMeasure.EnableWindow(1);
		}
		// Do not apply TCA, make values to 0
		ZeroMemory(g_ICANDIParams.RGBClkShifts, 3*sizeof(POINT));
	} else {
		m_TCAApply.SetCheck(0);		
		MessageBox("Undertermined");
	}
}

void CICANDIView::OnTCAOverride() {	
	CString initials;
	if (m_TCAOverride.GetCheck() == 0) {		
		m_TCAOverride.SetCheck(1);
		g_bTCAOverride = TRUE;
		// stop if auto measuring is enabled and keep using last measured values and enable user to enter values
		m_TCAMeasure.EnableWindow(0);
		m_edtTCARedX.EnableWindow(1);
		m_edtTCARedY.EnableWindow(1);
		m_edtTCAGrX.EnableWindow(1);
		m_edtTCAGrY.EnableWindow(1);
		m_TCAApply.EnableWindow(1);
		UpdateUserTCA();
	} else if (m_TCAOverride.GetCheck() == 1) {
		m_TCAOverride.SetCheck(0);			
		g_bTCAOverride = FALSE;		
		m_edtTCARedX.EnableWindow(0);
		m_edtTCARedY.EnableWindow(0);
		m_edtTCAGrX.EnableWindow(0);
		m_edtTCAGrY.EnableWindow(0);		
		// use and/or populate the automated values
		if (m_TCAApply.GetCheck() == 1) { 
			m_TCAMeasure.EnableWindow(0);
		} else { 
			m_TCAMeasure.EnableWindow(1);
		}		
		if (m_TCAApply.GetCheck() == 1)
			OnApplyTCA();
	} else {
		m_TCAOverride.SetCheck(0);		
		MessageBox("Undertermined");
	}

	initials.Format("%d", g_ICANDIParams.RGBClkShifts[0].x);				
	SetDlgItemText(IDE_TCA_RED_X, initials);
	initials.Format("%d", g_ICANDIParams.RGBClkShifts[0].y);				
	SetDlgItemText(IDE_TCA_RED_Y, initials);
	initials.Format("%d", g_ICANDIParams.RGBClkShifts[1].x);				
	SetDlgItemText(IDE_TCA_GR_X, initials);
	initials.Format("%d", g_ICANDIParams.RGBClkShifts[1].y);				
	SetDlgItemText(IDE_TCA_GR_Y, initials);
}

void CICANDIView::UpdateTCAValues(NMHDR* wParam, LRESULT *plr) {
	CString text;
	MSGFILTER * lpMsgFilter = (MSGFILTER *)wParam;
	
	switch (lpMsgFilter->msg) { // Assuming that you only have one control and don't need to verify the event's source.
	case WM_KEYDOWN:{
			int data=lpMsgFilter->wParam;
			if (data == 13){
				CString text;
				m_edtTCARedX.GetWindowText(text);
				g_RGBClkShiftsUser[0].x = atof(text);
				m_edtTCARedY.GetWindowText(text);
				g_RGBClkShiftsUser[0].y = atof(text);
				m_edtTCAGrX.GetWindowText(text);
				g_RGBClkShiftsUser[1].x = atof(text);
				m_edtTCAGrY.GetWindowText(text);
				g_RGBClkShiftsUser[1].y = atof(text);
				UpdateUserTCA();
			}
		}
		break;
	}
}

void CICANDIView::ApplyTCA()
{
	if (g_bTCAOverride) // use user entered values
		memcpy(g_ICANDIParams.RGBClkShifts, g_RGBClkShiftsUser, 3*sizeof(POINT));
	else			
		memcpy(g_ICANDIParams.RGBClkShifts, g_RGBClkShiftsAuto, 3*sizeof(POINT));
}

void CICANDIView::UpdateUserTCA()
{	
	if (!g_RGBClkShiftsUser[0].x && !g_RGBClkShiftsUser[0].y && !g_RGBClkShiftsUser[1].x && !g_RGBClkShiftsUser[1].y) {// all zeros
		if (g_ICANDIParams.ApplyTCA) { // no need to apply anything
			OnApplyTCA();
		}
	} else {
		if (!g_ICANDIParams.ApplyTCA)
			OnApplyTCA();
		else
			ApplyTCA();
	}
}
