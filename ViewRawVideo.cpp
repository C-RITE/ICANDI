// ViewRawVideo.cpp : implementation file
//

#include "stdafx.h"
#include "ICANDI.h"
#include "ICANDIDoc.h"
#include "ViewRawVideo.h"

#include "ICANDIParams.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CView       *g_viewRawVideo;
extern AOSLO_MOVIE  aoslo_movie;
extern PatchParams  g_PatchParamsA;		// parameters for handling matrox block-level sampling
extern POINT		g_StimulusPos;
extern long         g_sampling_counter;
extern BOOL         g_bFFTIsRunning;
extern BOOL         g_bStimulusOn;
extern UINT         g_iSavedFrames;       // frame counter for grabbed frames
extern float        g_qxCenter;
extern float        g_qyCenter;
extern UINT         g_frameIndex;			// counter indicating frame number processed by FFT
extern PatchParams  g_PatchParamsB;		// parameters for handling matrox block-level sampling
extern ICANDIParams    g_ICANDIParams;			// struct for storing ICANDI parameters

extern CAnimateCtrl *g_AnimateCtrl;
extern BOOL			g_bPlaybackUpdate;
extern BOOL			g_bValidSignal[33];

extern FILE        *g_fp;

extern short		g_nCurFlashCount;
extern short		g_nFlashCount;


extern int			g_nBrightness;
extern int			g_nContrast;
extern CVirtex5BMD       g_objVirtex5BMD;
extern WDC_DEVICE_HANDLE g_hDevVirtex5;

extern FILE			*temptest;

/////////////////////////////////////////////////////////////////////////////
// CViewRawVideo

IMPLEMENT_DYNCREATE(CViewRawVideo, CView)

CViewRawVideo::CViewRawVideo()
{
	g_viewRawVideo      = this;
	m_bMessageArrive    = FALSE;
	m_BitmapInfoWarp    = NULL;
	m_bLabelCreated     = FALSE;
	m_bUpdateBackground = FALSE;
	m_ClientRect.left   = 0;
	m_ClientRect.right  = 0;
	m_ClientRect.top    = 0;
	m_ClientRect.bottom = 0;
	m_bTCABoxOn         = FALSE;
	m_TCAcenter.x       = -1;
	m_TCAcenter.y       = -1;
	
	m_MemPoolID         = 0;
	m_tempVideoIn = NULL;
	m_tempVideoOut = NULL;

	m_PenGreen.CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
	m_PenYellow.CreatePen(PS_SOLID, 1, RGB(255, 255, 0));
}

CViewRawVideo::~CViewRawVideo()
{
	if(m_BitmapInfoWarp) free(m_BitmapInfoWarp);
	if(m_tempVideoIn !=  NULL) delete [] m_tempVideoIn;
	if(m_tempVideoOut !=  NULL) delete [] m_tempVideoOut;
}

void CViewRawVideo::DoDataExchange(CDataExchange* pDX)
{
	CView::DoDataExchange(pDX);
	DDX_Control(pDX, IDS_BRIGHTNESS, m_sldBrightness);
	DDX_Control(pDX, IDL_BRIGHTNESS, m_lblBrightness);
	DDX_Control(pDX, IDL_CONTRAST, m_lblContrast);
	DDX_Control(pDX, IDS_CONTRAST, m_sldContrast);
}

BEGIN_MESSAGE_MAP(CViewRawVideo, CView)
	//{{AFX_MSG_MAP(CViewRawVideo)
	ON_MESSAGE(WM_MOVIE_SEND, OnSendMovie)
	ON_WM_PAINT()
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_HSCROLL()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewRawVideo drawing

void CViewRawVideo::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CViewRawVideo diagnostics

#ifdef _DEBUG
void CViewRawVideo::AssertValid() const
{
	CView::AssertValid();
}

void CViewRawVideo::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CViewRawVideo message handlers

LRESULT CViewRawVideo::OnSendMovie(WPARAM wParam, LPARAM lParam)
{
	int    width, height;
	double elapsed_time;
	CString  msg;

	m_msgID = lParam;

	switch (m_msgID) {
	case MOVIE_HEADER_ID:
		width  = aoslo_movie.width;
		height = aoslo_movie.height;

		if(m_BitmapInfoWarp) free(m_BitmapInfoWarp);
		if((m_BitmapInfoWarp = CreateDIB(width, height)) == NULL) return FALSE;
		if (m_tempVideoIn == NULL) m_tempVideoIn = new BYTE[aoslo_movie.width*aoslo_movie.height];
		if (m_tempVideoOut == NULL) m_tempVideoOut = new BYTE[aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy];

		m_start = clock();

		break;

	case SENDING_MOVIE:
		m_bMessageArrive = TRUE;
		break;
	case SEND_MOVIE_DONE:
		m_bMessageArrive = FALSE;
		break;
	case ID_GRAB_STOP:
		m_bMessageArrive = FALSE;
		m_finish = clock();
		elapsed_time = (1.0*(m_finish - m_start))/CLOCKS_PER_SEC;
		msg.Format("Elapsed time: %f seconds, frames sampled: %d", elapsed_time, g_sampling_counter);
//		MessageBox(msg);
		break;
	case INIT_ANIMATE_CTRL:
		m_bMessageArrive = FALSE;
		CreateAnimateCtrl();
	default:
		break;
	}
	
	if (m_bMessageArrive) {
		OnPaint();
		Invalidate(FALSE);
	}

	return 0;
}

void CViewRawVideo::CreateAnimateCtrl()
{
	if (g_AnimateCtrl == NULL)
	{
		int dx, dy, width, height;
		RECT rect;
		GetClientRect(&rect);
		width  = 512;//aoslo_movie.width;
		height = 512;//aoslo_movie.height;
		dx = (rect.right - rect.left - width) / 2;
		dy = (rect.bottom - rect.top - height) / 2;
		rect.top = dy;
		rect.left = dx;
		rect.bottom = dy+height;
		rect.right = dx+width;		
		g_AnimateCtrl = new CAnimateCtrl;
		g_AnimateCtrl->Create(WS_CHILD|!WS_VISIBLE|ACS_CENTER, rect, (CWnd*)(this), ID_ANIMATE_CTRL);
	}
}

// draw sampled movie on client area
void CViewRawVideo::DrawMovie(CDC *pDC)
{
	int     width, height;
	int     dx, dy, dewarp_sx, dewarp_sy, x, y;
	int     i, j;
	long    offset;
	RECT    rect;
	CString msg;

	CICANDIDoc* pDoc = (CICANDIDoc*)GetDocument();

	dewarp_sx = (aoslo_movie.dewarp_sx - aoslo_movie.width) / 2;
	dewarp_sy = (aoslo_movie.dewarp_sy - aoslo_movie.height) / 2;
	width  = aoslo_movie.width;
	height = aoslo_movie.height;

	x = g_StimulusPos.x - dewarp_sx;		// position X on the reference frame
	y = g_StimulusPos.y - dewarp_sy;		// position Y on the reference frame

	switch (g_ICANDIParams.FRAME_ROTATION)	{
		case 0: //no rotation or flip
			memcpy(m_tempVideoIn, aoslo_movie.video_in, aoslo_movie.height*aoslo_movie.width);
			break;
		case 1: //rotate 90 deg
			MUB_rotate90(&m_tempVideoIn, &aoslo_movie.video_in, aoslo_movie.height, aoslo_movie.width);
			break;
		case 2: //flip along Y axis
			memcpy(m_tempVideoIn, aoslo_movie.video_in, aoslo_movie.height*aoslo_movie.width);
			MUB_Rows_rev(&m_tempVideoIn, aoslo_movie.height, aoslo_movie.width);
				break;
		case 3: //rotate 270 deg and flip along Y axis
			//memcpy(m_tempVideoIn, aoslo_movie.video_in, aoslo_movie.height*aoslo_movie.width);
			MUB_rotate270(&m_tempVideoIn, &aoslo_movie.video_in, aoslo_movie.height, aoslo_movie.width);
			MUB_Rows_rev(&m_tempVideoIn, aoslo_movie.height, aoslo_movie.width);
			
			break;

		default:
			;
	}

	if (pDoc->m_bValidAviHandleA == TRUE) {
		if (pDoc->m_iSavedFramesA == 0)
			m_MemPoolID = 0;

		// Temporary store raw video to a big buffer
		if (pDoc->m_iSavedFramesA%2 == 0 && 
			pDoc->m_iSavedFramesA/2 <= pDoc->m_nMovieLength &&
			aoslo_movie.video_saveA1 != NULL &&
			aoslo_movie.video_saveA2 != NULL &&
			aoslo_movie.video_saveA3 != NULL) {

			j = MEMORY_POOL_LENGTH;
			i = pDoc->m_iSavedFramesA%(j*2);
			i = i/2;
			offset = i * aoslo_movie.height * aoslo_movie.width;
			if (m_MemPoolID == 0) {
				memcpy(&aoslo_movie.video_saveA1[offset], m_tempVideoIn, aoslo_movie.height*aoslo_movie.width*sizeof(BYTE));
			} else if (m_MemPoolID == 1) {
				memcpy(&aoslo_movie.video_saveA2[offset], m_tempVideoIn, aoslo_movie.height*aoslo_movie.width*sizeof(BYTE));
			} else if (m_MemPoolID == 2) {
				memcpy(&aoslo_movie.video_saveA3[offset], m_tempVideoIn, aoslo_movie.height*aoslo_movie.width*sizeof(BYTE));
			}

	/*		if (pDoc->m_fpStimPos != NULL) {
				fprintf(pDoc->m_fpStimPos, "%d, %5.3f, %5.3f\n", 
					//pDoc->m_iSavedFramesA/2, aoslo_movie.StimulusDeltaX+x, aoslo_movie.StimulusDeltaY+y);
					pDoc->m_iSavedFramesA/2, g_qxCenter, g_qyCenter);
			}*/
			if (pDoc->m_fpStimPos != NULL) { // 10/21/2011
				fprintf(pDoc->m_fpStimPos, "%d, %3.2f, %3.2f", 
					//pDoc->m_iSavedFramesA/2, aoslo_movie.StimulusDeltaX+x, aoslo_movie.StimulusDeltaY+y);
					pDoc->m_iSavedFramesA/2, g_qxCenter, g_qyCenter);
			
				// add motion traces to the .csv file, Oct 25, 2011
				for (j = 0; j < g_ICANDIParams.PATCH_CNT; j ++) {
					if (g_frameIndex == 0) {	// this is the reference frame, without motion, so the title line is added here
						fprintf(pDoc->m_fpStimPos, ",dx-%d,dy-%d", j+1, j+1);
					} else {
						if (aoslo_movie.no_dewarp_video == FALSE) {
							// with motion traces
							 fprintf(pDoc->m_fpStimPos, ",%3.2f,%3.2f", g_PatchParamsB.shift_xy[j+1], g_PatchParamsB.shift_xy[j+g_ICANDIParams.PATCH_CNT+1]);
						} else {
							// without motion traces, because FFT does not return anything as a result of saccade, blink, or other reasons.
							fprintf(pDoc->m_fpStimPos, "%d,%d", 0, 0);
						}
					}
				}
				// log valid/invalid signal data
				g_bValidSignal[32] = g_bValidSignal[31];
				for (j = 0; j < g_ICANDIParams.PATCH_CNT; j ++) {
					if (g_frameIndex == 0) {	// this is the reference frame, without motion, so the title line is added here
						fprintf(pDoc->m_fpStimPos, ",dx-%d,dy-%d", j+1, j+1);
					} else {
						if (aoslo_movie.no_dewarp_video == FALSE) {
							// with motion traces
							fprintf(pDoc->m_fpStimPos, ",%d", g_bValidSignal[j]);
						} else {
							// without motion traces, because FFT does not return anything as a result of saccade, blink, or other reasons.
							fprintf(pDoc->m_fpStimPos, ",%d", 0);
						}
					}
				}
				fprintf(pDoc->m_fpStimPos, "\n");
			}
		}
	}

	// Temporary store stabilized video to a big buffer
	if (pDoc->m_bValidAviHandleB == TRUE && g_bFFTIsRunning == TRUE) {
		switch (g_ICANDIParams.FRAME_ROTATION)	{
			case 0: //no rotation or flip
				memcpy(m_tempVideoOut, aoslo_movie.video_out, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy); 
				break;
			case 1: //rotate 90 deg
				MUB_rotate90(&m_tempVideoOut, &aoslo_movie.video_out, aoslo_movie.dewarp_sx, aoslo_movie.dewarp_sy);
				break;
			case 2: //flip along Y axis
				memcpy(m_tempVideoOut, aoslo_movie.video_out, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);
				MUB_Rows_rev(&m_tempVideoOut, aoslo_movie.dewarp_sx, aoslo_movie.dewarp_sy);
				break;
			case 3: //rotate 270 deg and flip along Y axis
				//memcpy(m_tempVideoOut, aoslo_movie.video_out, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);
				MUB_rotate270(&m_tempVideoOut, &aoslo_movie.video_out, aoslo_movie.dewarp_sx, aoslo_movie.dewarp_sy);
				MUB_Rows_rev(&m_tempVideoOut, aoslo_movie.dewarp_sx, aoslo_movie.dewarp_sy);
				break;
			default:
				;
		}

		if (pDoc->m_iSavedFramesB%2 == 0 && 
			pDoc->m_iSavedFramesB/2 <= pDoc->m_nMovieLength &&
			aoslo_movie.video_saveB1 != NULL &&
			aoslo_movie.video_saveB2 != NULL &&
			aoslo_movie.video_saveB3 != NULL) {

			j = MEMORY_POOL_LENGTH;
			i = pDoc->m_iSavedFramesB%(j*2);
			i = i/2;
			offset = i * aoslo_movie.dewarp_sx * aoslo_movie.dewarp_sy;
			if (m_MemPoolID == 0) {
				memcpy(&aoslo_movie.video_saveB1[offset], m_tempVideoOut, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy*sizeof(BYTE));
			} else if (m_MemPoolID == 1) {
				memcpy(&aoslo_movie.video_saveB2[offset], m_tempVideoOut, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy*sizeof(BYTE));
			} else if (m_MemPoolID == 2) {
				memcpy(&aoslo_movie.video_saveB3[offset], m_tempVideoOut, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy*sizeof(BYTE));
			}
		}
		pDoc->m_iSavedFramesB ++;
	}

	if (pDoc->m_bValidAviHandleA == TRUE) {
		// Temporary store raw video to a big buffer
		if (pDoc->m_iSavedFramesA%2 == 0 && 
			pDoc->m_iSavedFramesA/2 <= pDoc->m_nMovieLength) {

			j = MEMORY_POOL_LENGTH;
			if ((2+pDoc->m_iSavedFramesA)%(j*2) == 0) {						
				aoslo_movie.memory_pool_ID = m_MemPoolID;
				if (m_MemPoolID == 0)       m_MemPoolID = 1;
				else if (m_MemPoolID == 1) 	m_MemPoolID = 2;
				else if (m_MemPoolID == 2) 	m_MemPoolID = 0;

				// turn flag on streaming videos from RAM to HDD
				pDoc->m_bDumpingVideo = TRUE;		
			}
		}

		pDoc->m_iSavedFramesA ++;

	/*	if (pDoc->m_iSavedFramesA/2 >= pDoc->m_nMovieLength+1 ||
			pDoc->m_iSavedFramesB/2 >= pDoc->m_nMovieLength+1) {
			// 
			// notify ICANDIView to turn off saving command !!!!!!!!!!!!!!!!!!!!!!!!
			//
			m_array.RemoveAll();
			m_array.Add(0);
			m_array.Add(0);
			m_array.Add(SAVE_VIDEO_FLAG);
		
			pDoc->UpdateAllViews(this, 0, &m_array);
		}*/
	}


	GetClientRect(&rect);
	dx = (rect.right - rect.left - width) / 2;
	dy = (rect.bottom - rect.top - height) / 2;
	::StretchDIBits(m_hdc, dx, dy, width, height,
					0, 0, width, height, m_tempVideoIn,//aoslo_movie.video_ins,
					m_BitmapInfoWarp, NULL, SRCCOPY);

	if (m_bTCABoxOn) {
		CPen *m_PenOld = pDC->SelectObject(&m_PenYellow);
		pDC->Arc(m_TCAcenter.x-5, m_TCAcenter.y-5, m_TCAcenter.x+5, m_TCAcenter.y+5,
				 m_TCAcenter.x-5, m_TCAcenter.y-5, m_TCAcenter.x-5, m_TCAcenter.y-5);
		pDC->SelectObject(m_PenOld);             // newPen is deselected  
	}
	
//	if (aoslo_movie.stimulus_flag == TRUE && g_bFFTIsRunning && x > 0 && y > 0) {
	if (g_bFFTIsRunning && x > 0 && y > 0) {
		// Draw stimulus point on the raw video
	//	if (g_StimulusPos.x == aoslo_movie.width/2 && 
	//		g_StimulusPos.y == aoslo_movie.height/2) {
	//	} else {
			DrawStimulusPoint(pDC, dx, dy);
			m_msgStimulusSt.SetWindowText("Stimulus on  ");
			m_msgStimulusSt.SetBackgroundColor(FALSE, RGB(0, 255, 0));
			if (g_ICANDIParams.ApplyTCA) {
				m_msgTCASt.SetWindowText("TCA on");
				m_msgTCASt.SetBackgroundColor(FALSE, RGB(255, 0, 0));
			} else {
				m_msgTCASt.SetWindowText("TCA off");
				m_msgTCASt.SetBackgroundColor(FALSE, RGB(255, 0, 255));
			}
	//	}
	} else {
		m_msgStimulusSt.SetWindowText("Stimulus off  ");
		m_msgStimulusSt.SetBackgroundColor(FALSE, RGB(255, 255, 0));
		m_msgTCASt.SetWindowText("TCA off");
		m_msgTCASt.SetBackgroundColor(FALSE, RGB(255, 0, 255));
	}

	if (pDoc->m_iSavedFramesA > 0) {		
		if (pDoc->m_iSavedFramesA%60 == 0 || pDoc->m_iSavedFramesA%60 == 1) {
			CString msg;
			msg.Format("...Grabbing... %d sec             ", (int)(pDoc->m_iSavedFramesA/60));//
			m_msgTimeElapse.SetWindowText(msg);
		}
	} else {
		if ((g_sampling_counter+1)%30 == 0) {
			CString msg;
			msg.Format("Time elapsed: %d sec             ", (int)((g_sampling_counter+1)/30));
			m_msgTimeElapse.SetWindowText(msg);
		}
	}
}

void CViewRawVideo::OnPaint() 
{
	if (!g_bPlaybackUpdate) {
	CPaintDC dc(this); // device context for painting

	//if (m_bMessageArrive && m_bShowVideo) {	
	if (m_bMessageArrive) {
		DrawMovie(&dc);
	} else {
		dc.FillSolidRect(&m_ClientRect, RGB(0, 0, 0));
	}
	if (m_bUpdateBackground == TRUE) {
		dc.FillSolidRect(&m_ClientRect, RGB(0, 0, 0));
		m_bUpdateBackground = FALSE;
	}
	}
}

// receive all updated information from CDocument and redraw the window
void CViewRawVideo::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
/*	if (pHint == NULL) {
	} else {
		CByteArray  *msgArray = (CByteArray*)pHint;

		BYTE  id0 = msgArray->GetAt(0);
		BYTE  id1 = msgArray->GetAt(1);
		BYTE  id2 = msgArray->GetAt(2);

	//		m_bSaveVideo = (id0 == 0) ? FALSE : TRUE;
	}*/
}

LPBITMAPINFO CViewRawVideo::CreateDIB(int cx, int cy)
{
	LPBITMAPINFO lpBmi;
	int iBmiSize;

	iBmiSize = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256;

	// Allocate memory for the bitmap info header.
	if((lpBmi = (LPBITMAPINFO)malloc(iBmiSize)) == NULL)
	{
		MessageBox("Error allocating BitmapInfo!", "Warning", MB_ICONWARNING);
		return NULL;
	}
	ZeroMemory(lpBmi, iBmiSize);

	// Initialize bitmap info header
	lpBmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	lpBmi->bmiHeader.biWidth = cx;
	lpBmi->bmiHeader.biHeight = -cy;
	lpBmi->bmiHeader.biPlanes = 1;
	lpBmi->bmiHeader.biSizeImage = 0;
	lpBmi->bmiHeader.biXPelsPerMeter = 0;
	lpBmi->bmiHeader.biYPelsPerMeter = 0;
	lpBmi->bmiHeader.biClrUsed = 0;
	lpBmi->bmiHeader.biClrImportant = 0;
	lpBmi->bmiHeader.biCompression = 0;

	// After initializing the bitmap info header we need to store some
	// more information depending on the bpp of the bitmap.
	// For the 8bpp DIB we will create a simple grayscale palette.
	for(int i = 0; i < 256; i++)
	{
		lpBmi->bmiColors[i].rgbRed		= (BYTE)i;
		lpBmi->bmiColors[i].rgbGreen	= (BYTE)i;
		lpBmi->bmiColors[i].rgbBlue		= (BYTE)i;
		lpBmi->bmiColors[i].rgbReserved	= (BYTE)0;
	}

	// Set the bpp for this DIB to 8bpp.
	lpBmi->bmiHeader.biBitCount = 8;

	return lpBmi;
}


void CViewRawVideo::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	m_hdc = GetDC()->m_hDC;
}

void CViewRawVideo::DrawStimulusPoint(CDC *pDC, int dx, int dy)
{
	int    x, y, xr, yr, dewarp_sx, dewarp_sy;
	float  uj, vj;
	
	dewarp_sx = (aoslo_movie.dewarp_sx - aoslo_movie.width) / 2;
	dewarp_sy = (aoslo_movie.dewarp_sy - aoslo_movie.height) / 2;
	x = g_StimulusPos.x - dewarp_sx;		// position X on the reference frame
	y = g_StimulusPos.y - dewarp_sy;		// position Y on the reference frame

	if (x < 0 || y < 0) return;

	if (y < aoslo_movie.height) {
		uj = aoslo_movie.StimulusDeltaX;
		vj = aoslo_movie.StimulusDeltaY;

		if (g_bStimulusOn == TRUE) {
			switch (g_ICANDIParams.FRAME_ROTATION)	{
				case 0: //no rotation or flip	
					xr = x + (int)uj;					// translation X of the target frame
					yr = y + (int)vj;					// translation Y of the target frame
					break;
				case 1: //rotate 90 deg
					xr = aoslo_movie.height - y ;					
					yr = x;	
					break;
				case 2: //flip along Y axis
					xr = aoslo_movie.width - x + (int)uj;					
					yr = y + (int)vj;
					break;
				default:
					;
			}
		} else {
			return;
		}

		//Draw a green cross mark on the interface to show the stimulus location in real time
		if (xr > 0 && xr < aoslo_movie.width && yr > 0 && yr < aoslo_movie.height && aoslo_movie.stimulus_loc_flag) {
			CPen *pOldPen = pDC->SelectObject(&m_PenGreen);
			pDC->MoveTo(xr-5+dx, yr+dy+1);
			pDC->LineTo(xr+5+dx, yr+dy+1);
			pDC->MoveTo(xr+dx, yr-5+dy);
			pDC->LineTo(xr+dx, yr+5+dy);
			pDC->SelectObject(pOldPen);
		}
	}
}


int CViewRawVideo::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;

	RECT       rect;

	rect.left = 0;	 rect.right  = 200;
	rect.top  = 0;   rect.bottom = 10;
	m_msgTimeElapse.Create(WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY, 
							rect, this, IDC_CUSTOM_TIME_ELAPSE);
	m_msgTimeElapse.SetBackgroundColor(FALSE, RGB(0, 255, 255));

	rect.left = 0;	 rect.right  = 200;
	rect.top  = 0;   rect.bottom = 10;
	m_msgStimulusSt.Create(WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY, 
							rect, this, IDC_CUSTOM_STIMULUS_STATUS);
	m_msgStimulusSt.SetBackgroundColor(FALSE, RGB(255, 255, 0));

	rect.left = 0;	 rect.right  = 200;
	rect.top  = 0;   rect.bottom = 10;
	m_msgTCASt.Create(WS_CHILD | WS_VISIBLE | ES_CENTER | ES_READONLY, 
							rect, this, IDL_CUSTOM_TCA_STATUS);
	m_msgTCASt.SetBackgroundColor(FALSE, RGB(255, 0, 255));

	m_bLabelCreated = TRUE;
		
	CString msg;
	// draw a label for Brightness
	rect.left = 105;	rect.right  = rect.left + 115;
	rect.top  = 512+110; rect.bottom = rect.top + 15;
	m_lblBrightness.Create("Brightness - ",  WS_CHILD | WS_VISIBLE | SS_CENTER, 
					 rect, this, IDL_BRIGHTNESS);
	msg.Format("Brightness - %3d", 255-g_nBrightness);
	m_lblBrightness.SetWindowText(msg);

	rect.left -= 20;	 rect.right  = rect.left + 155;
	rect.top  = 512+130; rect.bottom = rect.top + 15;
	m_sldBrightness.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | SBS_HORZ, rect, this, IDS_BRIGHTNESS);
	m_sldBrightness.EnableWindow(true);
	m_sldBrightness.SetScrollRange(0,255);
	m_sldBrightness.SetScrollPos(255-g_nBrightness);
	m_sldBrightness.SetFocus();

	
	// draw a label for contrast
	rect.left = rect.right + 55;	rect.right  = rect.left + 115;
	rect.top  = 512+110; rect.bottom = rect.top + 15;
	m_lblContrast.Create("Contrast - ",  WS_CHILD | WS_VISIBLE | SS_CENTER, 
					 rect, this, IDL_CONTRAST);
	msg.Format("Contrast - %3d", 127-g_nContrast);
	m_lblContrast.SetWindowText(msg);

	rect.left -= 20;	 rect.right  = rect.left + 155;
	rect.top  = 512+130; rect.bottom = rect.top + 15;
	m_sldContrast.Create(WS_CHILD | WS_VISIBLE | WS_TABSTOP | SBS_HORZ, rect, this, IDS_CONTRAST);
	m_sldContrast.EnableWindow(true);
	m_sldContrast.SetScrollRange(0,127);
	m_sldContrast.SetScrollPos(127-g_nContrast);
	m_sldContrast.SetFocus();
	
	return 0;
}

void CViewRawVideo::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_bLabelCreated) {
		GetClientRect(&m_ClientRect);
		m_msgTimeElapse.SetWindowPos(&wndTop, 2, 0, m_ClientRect.right/3-3, 20, SWP_SHOWWINDOW);
		m_msgStimulusSt.SetWindowPos(&wndTop, m_ClientRect.right/3, 0, m_ClientRect.right/3-3, 20, SWP_SHOWWINDOW);
		m_msgTCASt.SetWindowPos(&wndTop, 2*m_ClientRect.right/3, 0, m_ClientRect.right/3-3, 20, SWP_SHOWWINDOW);
	}

	//if (m_bMessageArrive && m_bShowVideo) {
	if (m_bMessageArrive) {
		m_bUpdateBackground = TRUE;
	} else {
		m_bUpdateBackground = FALSE;
	}
}


void CViewRawVideo::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	// TODO: Add your message handler code here and/or call default
	CICANDIDoc* pDoc = (CICANDIDoc *)GetDocument();
	ASSERT_VALID(pDoc);

	if (m_bMessageArrive == TRUE && m_msgID > 0) {				
		switch(nChar){
			case VK_SPACE:	// Space Key				
				break;
			case VK_RIGHT:
				break;
			case VK_LEFT:
				break;
			case VK_UP:
				break;
			case VK_DOWN:
				break;				
			case VK_ESCAPE:
				break;
			case VK_NUMPAD0:
				break;
			case VK_NUMPAD1:
				break;
			case VK_NUMPAD2:
				break;
			case VK_NUMPAD3:
				break;
			case VK_NUMPAD4:
				break;
			case VK_NUMPAD5:
				break;
			case VK_NUMPAD6:
				break;
			case VK_NUMPAD7:
				break;
			case VK_NUMPAD8:
				break;
			case VK_NUMPAD9:
				break;
			case 70:
				break;
			case 66:
				break;
			default:
				return;
		}
	}	
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void CViewRawVideo::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	UINT32 x, y, sx, sy;
	int    x0, y0, dx;
	RECT   rect;

	if (aoslo_movie.TCAboxWidth>255 || aoslo_movie.TCAboxHeight>255) {
		MessageBox("The size of TCA box is limited to 255x255.", "TCA Box", MB_ICONWARNING);
		return;
	}

	GetClientRect(&rect);

	sx = (rect.right - rect.left - aoslo_movie.width) / 2;
	sy = (rect.bottom - rect.top - aoslo_movie.height) / 2;

	switch (g_ICANDIParams.FRAME_ROTATION)	{
		case 0: //no rotation or flip
			x = point.x-sx;
			y = point.y-sy;
			break;
		case 1: //rotate 90 deg
			x = point.y-sy;
			y = aoslo_movie.height-point.x+sx;
			break;
		case 2: //flip along Y axis
			x = aoslo_movie.width-point.x-sx;
			y = point.y-sy;
			break;
		default:
			;
	}

	if (x>0 && x<aoslo_movie.width && y>0 && y<aoslo_movie.height) {
		m_bTCABoxOn = TRUE;
		
		if (g_PatchParamsA.StretchFactor == NULL) {
			x0 = (x - aoslo_movie.TCAboxWidth/2);
			//x0 = (x0 < 1) ? 1 : x0;
			dx = aoslo_movie.TCAboxWidth;
		} else {
			GetSinusoidX(x, aoslo_movie.TCAboxWidth, &x0, &dx);
		}

		x0 = x0 - SYSTEM_LATENCY_DAC8 - AOM_LATENCYX_IR;
		x0 = (x0 < 1) ? 1 : x0;
		y0 = y - aoslo_movie.TCAboxHeight/2;
		y0 = (y0 < 1) ? 1 : y0;

//		g_objVirtex5BMD.AppAddTCAbox(g_hDevVirtex5, x0, y0, dx, aoslo_movie.TCAboxHeight, x0, y0, dx, aoslo_movie.TCAboxHeight, x0, y0, dx, aoslo_movie.TCAboxHeight);
//		g_objVirtex5BMD.AppAddTCAbox(g_hDevVirtex5, x0, y0, dx, aoslo_movie.TCAboxHeight);
//		g_objVirtex5BMD.AppAddTCAbox(g_hDevVirtex5, x, y, 120, 120);
		m_TCAcenter.x = point.x;
		m_TCAcenter.y = point.y;
	} else {
		m_bTCABoxOn = FALSE;
		m_TCAcenter.x = -1;
		m_TCAcenter.y = -1;
	}

	CView::OnLButtonDblClk(nFlags, point);
}

void CViewRawVideo::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
/*	m_bTCABoxOn = FALSE;
	m_TCAcenter.x = -1;
	m_TCAcenter.y = -1;
	
	//g_objVirtex5BMD.AppRemoveTCAbox(g_hDevVirtex5);
	
*/	
	CView::OnRButtonDblClk(nFlags, point);
}

int CViewRawVideo::GetSinusoidX(int xc_i, BYTE dx, int *xs_o, int *dx_o)
{
	int i, x1, x2;

	x1 = xc_i - dx/2;
	x2 = xc_i + dx/2;

	// presinusoid TCA box
	if (x1 >= 1) {
		for (i = 0; i < aoslo_movie.width; i ++) {
			if (x1 < g_PatchParamsA.StretchFactor[i]) break;
		}
		x1 = i;
	} else {
		x1 = 1;
	}

	if (x2 < aoslo_movie.width) {
		for (i = 0; i < aoslo_movie.width; i ++) {
			if (x2 < g_PatchParamsA.StretchFactor[i]) break;
		}
		x2 = i;
	} else {
		x2 = aoslo_movie.width - 1;
	}

	*xs_o = x1;
	*dx_o = (x2-x1)>255 ? 255 : (x2-x1);

	return 0;
}


void CViewRawVideo::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CString msg;
    int		nTemp1, nTemp2, upBound;
//	UINT32  regR, regW;
	CString filename;

	nTemp1 = pScrollBar->GetScrollPos();

	switch (pScrollBar->GetDlgCtrlID()) {
	case IDS_BRIGHTNESS:
		upBound = 255;
		break;
	case IDS_CONTRAST:
		upBound = 127;
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
		nTemp2 = 1;
        if ((nTemp1 - nTemp2) >= 0) {
            nTemp1 -= nTemp2;
        } else {
            nTemp1 = 0;
        }
        pScrollBar->SetScrollPos(nTemp1);
        break;
    case SB_LINERIGHT: // right arrow button
		nTemp2 = 1;
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
	case IDS_BRIGHTNESS:				
		g_nBrightness = 255-nTemp1;
		//send updated brightness to FPGA
		UpdateBrightnessContrast(TRUE);

		msg.Format("Brightness - %3d",nTemp1);
		m_lblBrightness.SetWindowText(msg);
		break;

	case IDS_CONTRAST:	
		g_nContrast = 127-nTemp1;
		//send updated brightness to FPGA
		UpdateBrightnessContrast(FALSE);

		msg.Format("Contrast - %3d",nTemp1);
		m_lblContrast.SetWindowText(msg);
		break;
	}

//	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void CViewRawVideo::UpdateBrightnessContrast(BOOL brightness)
{
	CString text;
	UINT32  regData;
	BYTE    hiByte, loByte, regValret;

	if (brightness) {//update brightness
		regData = (UINT32)(g_nBrightness);
		regData = regData << 23;
		hiByte = (BYTE)(regData >> 24);
		m_regAddr = 0x0B; m_regVal = hiByte;				
		g_objVirtex5BMD.ReadWriteI2CRegister(g_hDevVirtex5, FALSE, 0x4c000000, m_regAddr, m_regVal, &regValret);
		Sleep(25);
		regData = (UINT32)(g_nBrightness);
		regData = regData << 31;
		loByte = (BYTE)(regData >> 24);
		m_regAddr = 0x0C; m_regVal = loByte;				
		g_objVirtex5BMD.ReadWriteI2CRegister(g_hDevVirtex5, FALSE, 0x4c000000, m_regAddr, m_regVal, &regValret);
		Sleep(25);
	}
	else {//update contrast
		m_regAddr = 0x05; m_regVal = g_nContrast;
		g_objVirtex5BMD.ReadWriteI2CRegister(g_hDevVirtex5, FALSE, 0x4c000000, m_regAddr, m_regVal, &regValret);
		Sleep(25);
		m_regAddr = 0x06; m_regVal = 0x00000000;
		g_objVirtex5BMD.ReadWriteI2CRegister(g_hDevVirtex5, FALSE, 0x4c000000, m_regAddr, m_regVal, &regValret);
		Sleep(25);
	}
}