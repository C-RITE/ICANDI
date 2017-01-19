// ViewDewarpVideo.cpp : implementation file
//

#include "stdafx.h"
#include "ICANDI.h"
#include "ICANDIDoc.h"
#include "ViewDewarpVideo.h"

#include "ICANDIParams.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern ICANDIParams    g_ICANDIParams;
extern CView       *g_viewDewVideo;
extern AOSLO_MOVIE  aoslo_movie;
extern POINT        g_StimulusPos;
extern POINT        g_StimulusPos0G;
extern POINT		g_nStimPosBak_Gain0Track;
extern POINT        g_StimulusPosBak;
extern POINT		g_nStimPosBak_Matlab;
extern POINT		g_nStimPosBak_Matlab_Retreive;
extern BOOL         g_bFFTIsRunning;
extern long         g_sampling_counter;
extern POINT		g_nStimPos;
extern CVirtex5BMD  g_objVirtex5BMD;
extern WDC_DEVICE_HANDLE g_hDevVirtex5;
extern HANDLE		g_eRunSLR;
extern BOOL			g_bRunSLR;
extern BOOL         g_bMatlab_Update;
extern BOOL         g_bStimulusOn;
extern BOOL		    g_bGain0Tracking;		//Track stimulus when gain is 0 (on/off) 10/21/2011
extern POINT        g_StimulusPos0Gain;		// stimulus position at gain 0

/////////////////////////////////////////////////////////////////////////////
// CViewDewarpVideo

IMPLEMENT_DYNCREATE(CViewDewarpVideo, CView)

CViewDewarpVideo::CViewDewarpVideo()
{
	g_viewDewVideo         = this;
	m_bMessageArrive       = FALSE;
	m_BitmapInfoDewarp     = NULL;
	m_bResetFlag           = FALSE;

	m_iMousePts = 20;
	for (int i = 0; i < m_iMousePts; i ++) 
		m_ptMousePos[i].x = m_ptMousePos[i].y = -20;
	m_iMousePts = 0;

	g_StimulusPos.x        = -1;
	g_StimulusPos.y        = -1;
	g_StimulusPosBak.x     = -1;
	g_StimulusPosBak.y     = -1;
	m_stimulusPos.x        = -1;
	m_stimulusPos.y        = -1;

	m_bUpdateBackground    = FALSE;
	m_bClientCreated       = FALSE;
	m_ClientRect.left      = 0;
	m_ClientRect.right     = 0;
	m_ClientRect.top       = 0;
	m_ClientRect.bottom    = 0;

	m_intCaptionSize.x     = 80;
	m_intCaptionSize.y     = 80;

//	m_image0               = NULL;
//	m_image1               = NULL;
//	m_image2               = NULL;
	m_rect.left            = 0;
	m_rect.top             = 0;
	m_rect.right           = 0;
	m_rect.bottom          = 0;
	m_offset.x             = 0;
	m_offset.y             = 0;
	m_IsMouseDown          = FALSE;
	m_coneRadius           = 3;
	m_nConeNumber          = 0;
	m_nFrameID             = 0;
//	m_bPatchIsSelected     = FALSE;
	m_tempVideoOut		   = NULL;
	m_bUpdateStimLoc	   = FALSE;

	// create a red pen
	m_PenRed.CreatePen(PS_SOLID, 1, RGB(255,0,0));
	// create a gree pen
	m_PenGreen.CreatePen(PS_SOLID, 2, RGB(0, 255, 0));
	// create a yellow pen
	m_PenYellow.CreatePen(PS_SOLID, 2, RGB(255, 255, 0));
}

CViewDewarpVideo::~CViewDewarpVideo()
{
	if(m_BitmapInfoDewarp) free(m_BitmapInfoDewarp);
//	if (m_image0 != NULL) delete [] m_image0;	
//	if (m_image1 != NULL) delete [] m_image1;	
//	if (m_image2 != NULL) delete [] m_image2;
	if(m_tempVideoOut) delete [] m_tempVideoOut;
}


BEGIN_MESSAGE_MAP(CViewDewarpVideo, CView)
	//{{AFX_MSG_MAP(CViewDewarpVideo)
	ON_MESSAGE(WM_MOVIE_SEND, OnSendMovie)
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_SIZE()
	ON_WM_KEYDOWN()
	ON_WM_CREATE()
	ON_BN_CLICKED(ID_DEWARP_GO, RunRandomStim)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewDewarpVideo drawing

void CViewDewarpVideo::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here
}

/////////////////////////////////////////////////////////////////////////////
// CViewDewarpVideo diagnostics

#ifdef _DEBUG
void CViewDewarpVideo::AssertValid() const
{
	CView::AssertValid();
}

void CViewDewarpVideo::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CViewDewarpVideo message handlers

LRESULT CViewDewarpVideo::OnSendMovie(WPARAM wParam, LPARAM lParam)
{
	int    width, height;

	m_msgID = lParam;

	switch (m_msgID) {
	case MOVIE_HEADER_ID:
		width  = aoslo_movie.dewarp_sx;
		height = aoslo_movie.dewarp_sy;
		g_StimulusPos.x = -1;
		g_StimulusPos.y = -1;
		g_StimulusPosBak.x = -1;
		g_StimulusPosBak.y = -1;

		if(m_BitmapInfoDewarp) free(m_BitmapInfoDewarp);
		if((m_BitmapInfoDewarp = CreateDIB(width, height)) == NULL)	return FALSE;
		if (m_tempVideoOut == NULL) m_tempVideoOut = new BYTE[aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy];

//		m_image0 = new int  [width*height];
//		m_image1 = new BYTE [width*height];
//		m_image2 = new BYTE [width*height];

		break;
	case SENDING_MOVIE:
		m_bMessageArrive = TRUE;
		break;
	case SEND_MOVIE_DONE:
		m_bMessageArrive = FALSE;

//		delete [] m_image0;
//		delete [] m_image1;
//		delete [] m_image2;
//		m_image0 = NULL;
//		m_image1 = NULL;
//		m_image2 = NULL;

		break;
	case UPDATE_STIM_POS:
		OnLButtonDblClk(0, g_nStimPos);
		break;
	case UPDATE_STIM_POS_MATLAB:
		UpdateStimPos(g_nStimPos);
	default:
		break;
	}
	
	if (m_bMessageArrive) {
		OnPaint();
		Invalidate(FALSE);
	}

	return 0;
}


void CViewDewarpVideo::DrawMovie(CDC *pDC)
{
	int     i, width, height, cx, cy;

	width  = aoslo_movie.width;
	height = aoslo_movie.height;

	if (m_msgID == SENDING_MOVIE) {
		m_iMousePts = 0;
		m_stimulusPos.x = m_stimulusPos.y = -1;
	} else if (m_msgID > 0) {
		if (aoslo_movie.no_dewarp_video == TRUE) {
			//			g_StimulusPos.x = -1;
			//			g_StimulusPos.y = -1;
			aoslo_movie.StimulusResX = 0;
			aoslo_movie.StimulusResY = 0;
		} else {
			if (m_stimulusPos.x == -1 && m_stimulusPos.y == -1) {
				g_StimulusPos.x = -1;//aoslo_movie.width/4;
				g_StimulusPos.y = -1;//aoslo_movie.height/4;
				g_StimulusPosBak.x = -1;//aoslo_movie.width/4;
				g_StimulusPosBak.y = -1;//aoslo_movie.height/4;
			} else {
				if (aoslo_movie.RandDelivering == FALSE && g_bMatlab_Update == FALSE) {
					g_StimulusPos.x = m_stimulusPos.x;
					g_StimulusPos.y = m_stimulusPos.y;
				} else {
					m_stimulusPos.x = g_StimulusPos.x;
					m_stimulusPos.y = g_StimulusPos.y;
				}
			}

			//  calculate deltas and assign it to somewhere
			if (m_iMousePts >= 1 && g_StimulusPos.x > 0 && g_StimulusPos.y > 0) {
				SearchCentroidPos(m_stimulusPos, &cx, &cy);
				aoslo_movie.StimulusResX = (float)(g_StimulusPos.x - cx);
				aoslo_movie.StimulusResY = (float)(g_StimulusPos.y - cy);
			} else {
				aoslo_movie.StimulusResX = 0;
				aoslo_movie.StimulusResY = 0;
			}

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
				case 3: //flip along X axis
					memcpy(m_tempVideoOut, aoslo_movie.video_out, aoslo_movie.dewarp_sx*aoslo_movie.dewarp_sy);
					MUB_Cols_rev(&m_tempVideoOut, aoslo_movie.dewarp_sx, aoslo_movie.dewarp_sy);
					break;
				default:
					;
			}

			if (aoslo_movie.DeliveryMode == 0) {
				::StretchDIBits(m_hdc, 0, 0, aoslo_movie.dewarp_sx, aoslo_movie.dewarp_sy,
					0, 0, aoslo_movie.dewarp_sx, aoslo_movie.dewarp_sy, m_tempVideoOut,
					m_BitmapInfoDewarp, NULL, SRCCOPY);
			} else {
				::StretchDIBits(m_hdc, 0, 0, aoslo_movie.dewarp_sx-m_intCaptionSize.x/2, aoslo_movie.dewarp_sy,
					0, 0, aoslo_movie.dewarp_sx-m_intCaptionSize.x/2, aoslo_movie.dewarp_sy, m_tempVideoOut,
					m_BitmapInfoDewarp, NULL, SRCCOPY);
			}

			if (aoslo_movie.stimulus_loc_flag) {
				switch (g_ICANDIParams.FRAME_ROTATION)	{
					case 0: //no rotation or flip
						if (aoslo_movie.DeliveryMode == 0) {
							// draw stimulus positions
							m_PenOld = pDC->SelectObject(&m_PenRed);
							for (i = 0; i < m_iMousePts-1; i ++) {
								pDC->Arc(m_ptMousePos[i].x+5, m_ptMousePos[i].y+5, m_ptMousePos[i].x-5, m_ptMousePos[i].y-5,
									m_ptMousePos[i].x+5, m_ptMousePos[i].y+5, m_ptMousePos[i].x+5, m_ptMousePos[i].y+5);
							}
						} else {
							// draw red circles for selected positions
							m_PenOld = pDC->SelectObject(&m_PenRed);
							for (i = 1; i < aoslo_movie.StimulusNum; i ++) {
								if (aoslo_movie.StimulusPos[i].x == m_stimulusPos.x &&
									aoslo_movie.StimulusPos[i].y == m_stimulusPos.y) {
										pDC->Arc(aoslo_movie.StimulusPos[i].x+5, aoslo_movie.StimulusPos[i].y+5, 
											aoslo_movie.StimulusPos[i].x-5, aoslo_movie.StimulusPos[i].y-5,
											aoslo_movie.StimulusPos[i].x+5, aoslo_movie.StimulusPos[i].y+5, 
											aoslo_movie.StimulusPos[i].x+5, aoslo_movie.StimulusPos[i].y+5);
								}
							}

							// draw a yellow circle for reference spot
							if (aoslo_movie.StimulusPos[0].x == m_stimulusPos.x &&
								aoslo_movie.StimulusPos[0].y == m_stimulusPos.y) {
							} else {
								if (aoslo_movie.StimulusNum > 0) {
									m_PenOld = pDC->SelectObject(&m_PenYellow);							
									pDC->Arc(aoslo_movie.StimulusPos[0].x+5, aoslo_movie.StimulusPos[0].y+5, 
										aoslo_movie.StimulusPos[0].x-5, aoslo_movie.StimulusPos[0].y-5,
										aoslo_movie.StimulusPos[0].x+5, aoslo_movie.StimulusPos[0].y+5, 
										aoslo_movie.StimulusPos[0].x+5, aoslo_movie.StimulusPos[0].y+5);
								}
							}
						}
						// draw a green circle for the current stimulus position
						pDC->SelectObject(m_PenOld);             // newPen is deselected  
						if (m_iMousePts >= 1) {
							m_PenOld = pDC->SelectObject(&m_PenGreen);
							if (g_bGain0Tracking && aoslo_movie.fStabGainStim == 0.) {		//Track stimulus when gain is 0 (on/off) 10/21/2011
								pDC->Arc(g_StimulusPos0Gain.x+5, g_StimulusPos0Gain.y+6, g_StimulusPos0Gain.x-7, g_StimulusPos0Gain.y-6,
									g_StimulusPos0Gain.x+5, g_StimulusPos0Gain.y+6, g_StimulusPos0Gain.x+5, g_StimulusPos0Gain.y+6);
							} else {
								pDC->Arc(m_stimulusPos.x+5, m_stimulusPos.y+5, m_stimulusPos.x-5, m_stimulusPos.y-5,
									m_stimulusPos.x+5, m_stimulusPos.y+5, m_stimulusPos.x+5, m_stimulusPos.y+5);
							}				
							pDC->SelectObject(m_PenOld);             // newPen is deselected  
						}
						break;
					case 1: //rotate 90 deg
						if (aoslo_movie.DeliveryMode == 0) {
							// draw red circles for selected positions
							m_PenOld = pDC->SelectObject(&m_PenRed);
							for (i = 0; i < m_iMousePts-1; i ++) {
								pDC->Arc(aoslo_movie.dewarp_sy-m_ptMousePos[i].y+4, m_ptMousePos[i].x+5, 
									aoslo_movie.dewarp_sy-m_ptMousePos[i].y-6, m_ptMousePos[i].x-5,
									aoslo_movie.dewarp_sy-m_ptMousePos[i].y+4, m_ptMousePos[i].x+5, 
									aoslo_movie.dewarp_sy-m_ptMousePos[i].y+4, m_ptMousePos[i].x+5);
							}
						} else {
							// draw red circles for selected positions
							m_PenOld = pDC->SelectObject(&m_PenRed);
							for (i = 1; i < aoslo_movie.StimulusNum; i ++) {
								if (aoslo_movie.StimulusPos[i].x == m_stimulusPos.x &&
									aoslo_movie.StimulusPos[i].y == m_stimulusPos.y) continue;
								pDC->Arc(aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[i].y+4, aoslo_movie.StimulusPos[i].x+5, 
									aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[i].y-6, aoslo_movie.StimulusPos[i].x-5,
									aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[i].y+4, aoslo_movie.StimulusPos[i].x+5, 
									aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[i].y+4, aoslo_movie.StimulusPos[i].x+5);
							}

							// draw a yellow circle for reference spot
							if (aoslo_movie.StimulusPos[0].x == m_stimulusPos.x &&
								aoslo_movie.StimulusPos[0].y == m_stimulusPos.y) {
							} else {
								if (aoslo_movie.StimulusNum > 0) {
									m_PenOld = pDC->SelectObject(&m_PenYellow);
									pDC->Arc(aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[0].y+4, aoslo_movie.StimulusPos[0].x+5, 
										aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[0].y-6, aoslo_movie.StimulusPos[0].x-5,
										aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[0].y+4, aoslo_movie.StimulusPos[0].x+5, 
										aoslo_movie.dewarp_sy-aoslo_movie.StimulusPos[0].y+4, aoslo_movie.StimulusPos[0].x+5);
								}
							}
						}
						// draw a green circle for the current stimulus position
						pDC->SelectObject(m_PenOld);             // newPen is deselected  
						if (m_iMousePts >= 1 && aoslo_movie.stimulus_loc_flag) {
							m_PenOld = pDC->SelectObject(&m_PenGreen);
							if (g_bGain0Tracking && aoslo_movie.fStabGainStim == 0.) {		//Track stimulus when gain is 0 (on/off) 10/21/2011
								pDC->Arc(aoslo_movie.dewarp_sy-g_StimulusPos0Gain.y+5, g_StimulusPos0Gain.x+6, aoslo_movie.dewarp_sy-g_StimulusPos0Gain.y-7, g_StimulusPos0Gain.x-6,
									aoslo_movie.dewarp_sy-g_StimulusPos0Gain.y+5, g_StimulusPos0Gain.x+6, aoslo_movie.dewarp_sy-g_StimulusPos0Gain.y+5, g_StimulusPos0Gain.x+6);
							} else {
								pDC->Arc(aoslo_movie.dewarp_sy-m_stimulusPos.y+5, m_stimulusPos.x+6, aoslo_movie.dewarp_sy-m_stimulusPos.y-7, m_stimulusPos.x-6,
									aoslo_movie.dewarp_sy-m_stimulusPos.y+5, m_stimulusPos.x+6, aoslo_movie.dewarp_sy-m_stimulusPos.y+5, m_stimulusPos.x+6);
							}				
							pDC->SelectObject(m_PenOld);             // newPen is deselected  
						}
						break;
					case 2: //flip along Y axis
						//	point.x = aoslo_movie.dewarp_sx-point.x-1;
						if (aoslo_movie.DeliveryMode == 0) {
							// draw stimulus positions
							m_PenOld = pDC->SelectObject(&m_PenRed);
							for (i = 0; i < m_iMousePts-1; i ++) {
								pDC->Arc(aoslo_movie.dewarp_sx-m_ptMousePos[i].x-1+5, m_ptMousePos[i].y+5, aoslo_movie.dewarp_sx-m_ptMousePos[i].x-1-5, m_ptMousePos[i].y-5,
									aoslo_movie.dewarp_sx-m_ptMousePos[i].x-1+5, m_ptMousePos[i].y+5, aoslo_movie.dewarp_sx-m_ptMousePos[i].x-1+5, m_ptMousePos[i].y+5);
							}
						} else {
							// draw red circles for selected positions
							m_PenOld = pDC->SelectObject(&m_PenRed);
							for (i = 1; i < aoslo_movie.StimulusNum; i ++) {
								if (aoslo_movie.StimulusPos[i].x == m_stimulusPos.x &&
									aoslo_movie.StimulusPos[i].y == m_stimulusPos.y) {
										pDC->Arc(aoslo_movie.StimulusPos[i].x+5, aoslo_movie.StimulusPos[i].y+5, 
											aoslo_movie.StimulusPos[i].x-5, aoslo_movie.StimulusPos[i].y-5,
											aoslo_movie.StimulusPos[i].x+5, aoslo_movie.StimulusPos[i].y+5, 
											aoslo_movie.StimulusPos[i].x+5, aoslo_movie.StimulusPos[i].y+5);
								}
							}

							// draw a yellow circle for reference spot
							if (aoslo_movie.StimulusPos[0].x == m_stimulusPos.x &&
								aoslo_movie.StimulusPos[0].y == m_stimulusPos.y) {
							} else {
								if (aoslo_movie.StimulusNum > 0) {
									m_PenOld = pDC->SelectObject(&m_PenYellow);							
									pDC->Arc(aoslo_movie.StimulusPos[0].x+5, aoslo_movie.StimulusPos[0].y+5, 
										aoslo_movie.StimulusPos[0].x-5, aoslo_movie.StimulusPos[0].y-5,
										aoslo_movie.StimulusPos[0].x+5, aoslo_movie.StimulusPos[0].y+5, 
										aoslo_movie.StimulusPos[0].x+5, aoslo_movie.StimulusPos[0].y+5);
								}
							}
						}
						// draw a green circle for the current stimulus position
						pDC->SelectObject(m_PenOld);             // newPen is deselected  
						if (m_iMousePts >= 1) {
							m_PenOld = pDC->SelectObject(&m_PenGreen);
							if (g_bGain0Tracking && aoslo_movie.fStabGainStim == 0.) {		//Track stimulus when gain is 0 (on/off) 10/21/2011
								pDC->Arc(g_StimulusPos0Gain.x+5, g_StimulusPos0Gain.y+6, g_StimulusPos0Gain.x-7, g_StimulusPos0Gain.y-6,
									g_StimulusPos0Gain.x+5, g_StimulusPos0Gain.y+6, g_StimulusPos0Gain.x+5, g_StimulusPos0Gain.y+6);
							} else {
								pDC->Arc(aoslo_movie.dewarp_sx-m_stimulusPos.x+5, m_stimulusPos.y+5, aoslo_movie.dewarp_sx-m_stimulusPos.x-5, m_stimulusPos.y-5,
									aoslo_movie.dewarp_sx-m_stimulusPos.x+5, m_stimulusPos.y+5, aoslo_movie.dewarp_sx-m_stimulusPos.x+5, m_stimulusPos.y+5);
							}				
							pDC->SelectObject(m_PenOld);             // newPen is deselected  
						}
						break;
					default:
						;
				}

				if (m_rect.left > 0 && m_rect.right > 0 && m_rect.top > 0 && m_rect.bottom > 0 &&
					aoslo_movie.DeliveryMode == 2) {
						m_PenOld = pDC->SelectObject(&m_PenRed);
						pDC->MoveTo(m_rect.left,    m_rect.top);
						pDC->LineTo(m_rect.left,    m_rect.bottom-1);
						pDC->LineTo(m_rect.right-1, m_rect.bottom-1);
						pDC->LineTo(m_rect.right-1, m_rect.top);
						pDC->LineTo(m_rect.left,    m_rect.top);
						pDC->SelectObject(m_PenOld);
				}
			}
		}
	} else if (m_msgID == SEND_MOVIE_DONE) {
		m_iMousePts         = 0;
		g_StimulusPos.x     = -1;
		g_StimulusPos.y     = -1;
		g_StimulusPosBak.x  = m_stimulusPos.x;
		g_StimulusPosBak.y  = m_stimulusPos.y;
	} else if (m_msgID == -1) {
		m_iMousePts         = 0;
		g_StimulusPos.x     = -1;
		g_StimulusPos.y     = -1;
		g_StimulusPosBak.x  = m_stimulusPos.x;
		g_StimulusPosBak.y  = m_stimulusPos.y;
	}
}


void CViewDewarpVideo::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
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

// this routine handle mouse events
// when FFT is running, double clicking left key draws a red
// circle with radius 5 pixels at the position of mouse.
void CViewDewarpVideo::OnLButtonDblClk(UINT nFlags, CPoint point) 
{	
	int    i, rad_sq = 25;//, cx, cy;
	BOOL   b_overlap = FALSE;

	if (point.x < 0 || point.y < 0 || point.x >= aoslo_movie.dewarp_sx || point.y >= aoslo_movie.dewarp_sy) return;

	switch (g_ICANDIParams.FRAME_ROTATION)	{
		case 0: //no rotation or flip				
			break;
		case 1: //rotate 90 deg
			i = point.x;
			point.x = point.y;
			point.y = aoslo_movie.dewarp_sx-i-1;
			break;
		case 2: //flip along Y axis
			point.x = aoslo_movie.dewarp_sx-point.x-1;
			break;
		default:
			;
	}

	if (g_bFFTIsRunning || aoslo_movie.fStabGainStim == 0.) {
		g_nStimPosBak_Gain0Track.x = g_StimulusPos0G.x = m_stimulusPos.x = g_StimulusPosBak.x = point.x;
		g_nStimPosBak_Gain0Track.y = g_StimulusPos0G.y = m_stimulusPos.y = g_StimulusPosBak.y = point.y;

		// TODO: Add your message handler code here and/or call default
		if (m_bMessageArrive == TRUE && m_msgID > 0) {
			if (point.x < 0 || point.y < 0 || point.x >= aoslo_movie.dewarp_sx || point.y >= aoslo_movie.dewarp_sy) return;

			for (i = 0; i < m_iMousePts; i ++) {
				if ((m_ptMousePos[i].x-point.x)*(m_ptMousePos[i].x-point.x) +
					(m_ptMousePos[i].y-point.y)*(m_ptMousePos[i].y-point.y) <= rad_sq) {
						b_overlap = TRUE;
						break;
				}
			}

			m_stimulusPos.x = point.x;
			m_stimulusPos.y = point.y;
			g_nStimPosBak_Matlab.x = g_StimulusPosBak.x = point.x;
			g_nStimPosBak_Matlab.y = g_StimulusPosBak.y = point.y;

			if (b_overlap == TRUE) {
				m_ptMousePos[i].x = m_stimulusPos.x;
				m_ptMousePos[i].y = m_stimulusPos.y;
			} else {
				m_ptMousePos[m_iMousePts].x = m_stimulusPos.x;
				m_ptMousePos[m_iMousePts].y = m_stimulusPos.y;
				m_iMousePts ++;

				if (m_iMousePts >= 20) {
					for (i = 0; i < 19; i ++) {
						m_ptMousePos[i].x = m_ptMousePos[i+1].x;
						m_ptMousePos[i].y = m_ptMousePos[i+1].y;
					}
					m_iMousePts = 19;
				}
			}	
			aoslo_movie.stimulus_loc_flag = TRUE;
			g_bRunSLR?PulseEvent(g_eRunSLR):0;
		} else {
			if (point.x < 0 || point.y < 0 || point.x >= aoslo_movie.dewarp_sx || point.y >= aoslo_movie.dewarp_sy) return;
			g_StimulusPosBak.x = point.x;
			g_StimulusPosBak.y = point.y;

			CICANDIDoc* pDoc = (CICANDIDoc*)GetDocument();
			pDoc->DrawStimulus();
		}
	}

	SetFocus();

	CView::OnLButtonDblClk(nFlags, point);
}

// remove selected mark
void CViewDewarpVideo::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int  i, rad_sq = 25;
	BOOL b_overlap = FALSE;

	// TODO: Add your message handler code here and/or call default
	if (m_bMessageArrive == TRUE && m_msgID > 0) {
		for (i = 0; i < m_iMousePts; i ++) {
			if ((m_ptMousePos[i].x-point.x)*(m_ptMousePos[i].x-point.x) +
				(m_ptMousePos[i].y-point.y)*(m_ptMousePos[i].y-point.y) <= rad_sq) {
				b_overlap = TRUE;
				break;
			}
		}
		if (b_overlap == TRUE) {
			for (int j = i+1; j < m_iMousePts; j ++) {
				m_ptMousePos[j-1].x = m_ptMousePos[j].x;
				m_ptMousePos[j-1].y = m_ptMousePos[j].y;
			}

			m_iMousePts --;
		}
	}

	CView::OnRButtonDown(nFlags, point);
}

// turn stimulus beam off, remove all the marks
void CViewDewarpVideo::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	// clear all stimulus positions
	m_iMousePts     = 0;
	m_stimulusPos.x = -1;
	m_stimulusPos.y = -1;
	g_StimulusPos.x = -1;
	g_StimulusPos.y = -1;
	g_StimulusPosBak.x  = -1;
	g_StimulusPosBak.y  = -1;
	g_StimulusPos0G.x = -1;
	g_StimulusPos0G.y = -1;

	g_objVirtex5BMD.AppWriteStimAddrShift(g_hDevVirtex5, 0, ((CICANDIApp*)AfxGetApp())->m_imageSizeX, 0, 0, 0, -1, -100);
	g_objVirtex5BMD.AppWriteStimLUT(g_hDevVirtex5, false, 0, 0, 0, 0, 0, 0, 0, ((CICANDIApp*)AfxGetApp())->m_imageSizeX, 0, 
										0, 0, 0, 0, 0, 0, 0, 0, 0);

	CView::OnRButtonDblClk(nFlags, point);
}

void CViewDewarpVideo::OnLButtonDown(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_bUpdateStimLoc = TRUE;
	
	CView::OnLButtonDown(nFlags, point);
}

void CViewDewarpVideo::OnLButtonUp(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	m_bUpdateStimLoc = FALSE;
	
	CView::OnLButtonDown(nFlags, point);
}

void CViewDewarpVideo::OnMouseMove(UINT nFlags, CPoint point) 
{
	// TODO: Add your message handler code here and/or call default
	if ((point.x > 0 && point.y > 0 && point.x < aoslo_movie.dewarp_sx && point.y < aoslo_movie.dewarp_sy) && m_bUpdateStimLoc)
	{
		OnLButtonDblClk(nFlags, point);
	}
	CView::OnMouseMove(nFlags, point);
}

// fine tune target position
void CViewDewarpVideo::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags) 
{
	int     xDir, yDir;
	CString text;

	// TODO: Add your message handler code here and/or call default
	if (m_bMessageArrive == TRUE && m_msgID > 0) {
		if (m_stimulusPos.x != -1 || m_stimulusPos.y != -1) {
			switch(nChar){
			case VK_LEFT:		// left arrow key
				if ((yDir=m_stimulusPos.y+1) < aoslo_movie.dewarp_sy)
					++m_stimulusPos.y;
			//for no rotation of image
			//	if ((xDir=m_stimulusPos.x-1) > 0)
			//		--m_stimulusPos.x;
				break;
			case VK_UP:		// up arrow key
				if ((xDir=m_stimulusPos.x-1) > 0)
					--m_stimulusPos.x;
			//for no rotation of image
			//	if ((yDir=m_stimulusPos.y-1) > 0)
			//		--m_stimulusPos.y;
				break;
			case VK_RIGHT:		// right arrow key
				if ((yDir=m_stimulusPos.y-1) > 0)
					--m_stimulusPos.y;
			//for no rotation of image
			//	if ((xDir=m_stimulusPos.x+1) < aoslo_movie.dewarp_sx)
			//		++m_stimulusPos.x;
				break;
			case VK_DOWN:		// down arrow key
				if ((xDir=m_stimulusPos.x+1) < aoslo_movie.dewarp_sx)
					++m_stimulusPos.x;
			//for no rotation of image
			//	if ((yDir=m_stimulusPos.y+1) < aoslo_movie.dewarp_sy)
			//		++m_stimulusPos.y;
				break;
			case VK_SPACE:		// space key
				if (aoslo_movie.DeliveryMode == 1) {
					if (aoslo_movie.StimulusNum == 0) {
						m_lstPositions.AddString("01: (0,0)");
						aoslo_movie.StimulusPos[0].x = m_stimulusPos.x;
						aoslo_movie.StimulusPos[0].y = m_stimulusPos.y;
						aoslo_movie.StimulusNum = 1;
					} else if (aoslo_movie.StimulusNum >= MAX_STIMULUS_NUMBER) {
						if (m_bResetFlag == FALSE) {
							text.Format("Maximum stimuli number %d has been reached, can't add in more", MAX_STIMULUS_NUMBER);
							MessageBox(text, "Add Stimulus", MB_ICONWARNING);
						} else {
							UpdateStimulusPos(m_stimulusPos);
							m_bResetFlag = FALSE;
						}
						return;
					} else {
						if (m_bResetFlag == FALSE) {
							aoslo_movie.StimulusPos[aoslo_movie.StimulusNum].x = m_stimulusPos.x;
							aoslo_movie.StimulusPos[aoslo_movie.StimulusNum].y = m_stimulusPos.y;
							text.Format("%02d: (%d,%d)", aoslo_movie.StimulusNum + 1, 
								aoslo_movie.StimulusPos[aoslo_movie.StimulusNum].x-aoslo_movie.StimulusPos[0].x, 
								aoslo_movie.StimulusPos[aoslo_movie.StimulusNum].y-aoslo_movie.StimulusPos[0].y);
							aoslo_movie.StimulusNum ++;
							m_lstPositions.AddString(text);
						} else {
							UpdateStimulusPos(m_stimulusPos);
							m_bResetFlag = FALSE;
						}
					}
				}

				if (aoslo_movie.DeliveryMode == 2 && aoslo_movie.StimulusNum > 0) {
					UpdateStimulusPos(m_stimulusPos);
				}

				break;
			case VK_DELETE:		// delete key
				if (aoslo_movie.DeliveryMode == 1 && aoslo_movie.StimulusNum > 0) {
					m_lstPositions.DeleteString(aoslo_movie.StimulusNum-1);
					aoslo_movie.StimulusNum --;
				}
				break;
			default:
				return;
			}
		}
	}
	
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}


// receive all updated information from CDocument and redraw the window
void CViewDewarpVideo::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if (pHint == NULL) {
	} else {
		CByteArray  *msgArray = (CByteArray*)pHint;

		BYTE  id0 = msgArray->GetAt(0);
		BYTE  id1 = msgArray->GetAt(1);
		BYTE  id2 = msgArray->GetAt(2);

		//if (id0 == CLEAR_STIMULUS_POINT) {
		if (id2 == RESET_REF_FRAME) {
			if (aoslo_movie.fStabGainStim != 0.) {			
				OnRButtonDblClk(NULL, NULL);
			}
			else {				
				m_iMousePts     = 1;
				m_stimulusPos.x = g_StimulusPos.x = g_StimulusPosBak.x = g_StimulusPos0G.x;
				m_stimulusPos.y = g_StimulusPos.y = g_StimulusPosBak.y = g_StimulusPos0G.y;
			}
			m_bResetFlag    = TRUE;
		}

		if (id2 == RANDOM_DELIVERY_DONE) {
			aoslo_movie.RandDelivering = FALSE;
			m_btnGo.EnableWindow(TRUE);
		}

		if (m_bClientCreated && id2 == DELIVERY_MODE_FLAG) {
			UpdateControls();
			aoslo_movie.StimulusNum = 0;
		}
	}

}

LPBITMAPINFO CViewDewarpVideo::CreateDIB(int cx, int cy)
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

	//-- Set the bpp for this DIB to 8bpp.
	lpBmi->bmiHeader.biBitCount = 8;

	return lpBmi;
}

void CViewDewarpVideo::OnInitialUpdate() 
{
	CView::OnInitialUpdate();
	
	m_hdc = GetDC()->m_hDC;
	m_bClientCreated = TRUE;
}


// search centroid position of the selected cone
BOOL CViewDewarpVideo::SearchCentroidPos(POINT point, int *cx, int *cy)
{
	int     i, j, sx, sy, idxI, idxJ, len, offset = 8;
	int    *GridX, *GridY, maxX, maxY, idxX, idxY;

	len = 2*offset + 1;
	GridX = new int [len+1];
	GridY = new int [len+1];

	for (i = 0; i < len; i ++) GridX[i] = GridY[i] = 0;

	for (j = point.y - offset; j <= point.y + offset; j ++) {
		idxJ = j * aoslo_movie.dewarp_sx;
		sy = j - point.y + offset;
		for (i = point.x - offset; i <= point.x + offset; i ++) {
			idxI = idxJ + i;
			sx = i - point.x + offset;
			GridX[sx] += *(aoslo_movie.video_out + idxI);
			GridY[sy] += *(aoslo_movie.video_out + idxI);
		}
	}

	idxX = -1;
	maxX = 0;
	for (i = 0; i < len; i ++) {
		if (maxX < GridX[i]) {
			idxX = i;
			maxX = GridX[i];
		}
	}

	idxY = -1;
	maxY = 0;
	for (i = 0; i < len; i ++) {
		if (maxY < GridY[i]) {
			idxY = i;
			maxY = GridY[i];
		}
	}

	*cx = point.x + idxX - offset;
	*cy = point.y + idxY - offset;

	delete [] GridX;
	delete [] GridY;

	return TRUE; 
}


void CViewDewarpVideo::OnSize(UINT nType, int cx, int cy) 
{
	CView::OnSize(nType, cx, cy);
	
	if (m_bClientCreated) {
		UpdateControls();
	}

	if (m_bMessageArrive) {
		m_bUpdateBackground = TRUE;
	} else {
		m_bUpdateBackground = FALSE;
	}
}


int CViewDewarpVideo::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	RECT   rect;
	CHARFORMAT cf;
	
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	FontFormat(cf);

	// Use the mouse and cursor to identify a cone. 
	// Press space bar to enter into list. 
	// Press delete button to remove last cone location in the list
	// draw a label
	rect.left = 0;	rect.right  = m_intCaptionSize.x;
	rect.top  = 0;  rect.bottom = m_intCaptionSize.y;
	m_lblCaptionArea.Create("\nSelected \nPosition \nList",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_CAPTION);

	m_lblIlluminationTime.Create("\nSeconds per Stimulus",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_TIME);

	m_lblFilling1.Create("",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_FILLING1);

	m_lblFilling2.Create("",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_FILLING2);

	m_lblFilling3.Create("",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_FILLING3);

	m_lblCellSizeX.Create("\n\nCell width (in pixels)",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_CELLSIZE_LABEL_X);

	m_lblCellSizeY.Create("\n\nCell height (in pixels)",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_CELLSIZE_LABEL_Y);

	m_lblStimNumbX.Create("\n\n# of X Cells (odd)",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_STIMNUM_LABEL_X);

	m_lblStimNumbY.Create("\n\n# of Y Cells (odd)",  WS_CHILD | SS_CENTER, 
							rect, this, ID_DEWARP_STIMNUM_LABEL_Y);

	m_lstPositions.Create(WS_CHILD | SS_LEFT | WS_VSCROLL | WS_THICKFRAME | ES_AUTOHSCROLL,
							rect, this, ID_DEWARP_POSLIST);
	
	m_edtIlluminationTime.Create(WS_CHILD | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, ID_DEWARP_ILLUMINATION);
	m_edtIlluminationTime.SetDefaultCharFormat(cf);
	m_edtIlluminationTime.SetWindowText("15");
	
	m_edtCellSizeX.Create(WS_CHILD | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, ID_DEWARP_CELLSIZE_EDIT_X);
	m_edtCellSizeX.SetDefaultCharFormat(cf);
	m_edtCellSizeX.SetWindowText("20");
	
	m_edtCellSizeY.Create(WS_CHILD | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, ID_DEWARP_CELLSIZE_EDIT_X);
	m_edtCellSizeY.SetDefaultCharFormat(cf);
	m_edtCellSizeY.SetWindowText("20");
	
	m_edtStimNumbX.Create(WS_CHILD | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, ID_DEWARP_STIMNUM_EDIT_X);
	m_edtStimNumbX.SetDefaultCharFormat(cf);
	m_edtStimNumbX.SetWindowText("5");
	
	m_edtStimNumbY.Create(WS_CHILD | ES_LEFT | WS_BORDER | WS_TABSTOP | ES_AUTOHSCROLL,  
					 rect, this, ID_DEWARP_STIMNUM_EDIT_X);
	m_edtStimNumbY.SetDefaultCharFormat(cf);
	m_edtStimNumbY.SetWindowText("5");
	
	m_btnGo.Create("Go", WS_CHILD | BS_DEFPUSHBUTTON | WS_TABSTOP, 
					 rect, this, ID_DEWARP_GO);
	
	return 0;
}

void CViewDewarpVideo::UpdateControls()
{
	GetClientRect(&m_ClientRect);

	if (aoslo_movie.DeliveryMode == 0) {
		m_lblCaptionArea.ShowWindow(FALSE);
		m_lstPositions.ShowWindow(FALSE);
		m_lblIlluminationTime.ShowWindow(FALSE);
		m_edtIlluminationTime.ShowWindow(FALSE);
		m_lblFilling1.ShowWindow(FALSE);
		m_lblFilling2.ShowWindow(FALSE);
		m_lblFilling3.ShowWindow(FALSE);
		m_btnGo.ShowWindow(FALSE);
		m_lblStimNumbX.ShowWindow(FALSE);
		m_lblStimNumbY.ShowWindow(FALSE);
		m_lblCellSizeX.ShowWindow(FALSE);
		m_lblCellSizeY.ShowWindow(FALSE);
		m_edtStimNumbX.ShowWindow(FALSE);
		m_edtStimNumbY.ShowWindow(FALSE);
		m_edtCellSizeX.ShowWindow(FALSE);
		m_edtCellSizeY.ShowWindow(FALSE);

		m_bUpdateBackground = TRUE;

	} else if (aoslo_movie.DeliveryMode == 1) {
		m_lblCaptionArea.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 0, m_intCaptionSize.x, 70);
		m_lstPositions.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 70, m_intCaptionSize.x, 390);
		m_lblIlluminationTime.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 460, m_intCaptionSize.x, 70);
		m_edtIlluminationTime.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 530, m_intCaptionSize.x, 25);
		m_lblFilling1.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 555, m_intCaptionSize.x, 20);
		m_btnGo.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 575, m_intCaptionSize.x, 25);
		m_lblFilling2.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 600, m_intCaptionSize.x, m_ClientRect.bottom);

		m_lblCaptionArea.SetWindowText("\nSelected \nPosition \nList");
		m_lblCaptionArea.ShowWindow(TRUE);
		m_lstPositions.ShowWindow(TRUE);
		m_lblIlluminationTime.ShowWindow(TRUE);
		m_edtIlluminationTime.ShowWindow(TRUE);
		m_lblFilling1.ShowWindow(TRUE);
		m_lblFilling2.ShowWindow(TRUE);
		m_lblFilling3.ShowWindow(FALSE);
		m_btnGo.ShowWindow(TRUE);

		m_lblStimNumbX.ShowWindow(FALSE);
		m_lblStimNumbY.ShowWindow(FALSE);
		m_lblCellSizeX.ShowWindow(FALSE);
		m_lblCellSizeY.ShowWindow(FALSE);
		m_edtStimNumbX.ShowWindow(FALSE);
		m_edtStimNumbY.ShowWindow(FALSE);
		m_edtCellSizeX.ShowWindow(FALSE);
		m_edtCellSizeY.ShowWindow(FALSE);

	} else if (aoslo_movie.DeliveryMode == 2) {
		m_lblCellSizeX.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 0, m_intCaptionSize.x, 70);
		m_edtCellSizeX.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 70, m_intCaptionSize.x, 25);

		m_lblCellSizeY.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 95, m_intCaptionSize.x, 70);
		m_edtCellSizeY.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 165, m_intCaptionSize.x, 25);

		m_lblStimNumbX.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 190, m_intCaptionSize.x, 70);
		m_edtStimNumbX.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 260, m_intCaptionSize.x, 25);

		m_lblStimNumbY.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 285, m_intCaptionSize.x, 70);
		m_edtStimNumbY.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 355, m_intCaptionSize.x, 25);

		m_lblIlluminationTime.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 380, m_intCaptionSize.x, 70);
		m_edtIlluminationTime.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 450, m_intCaptionSize.x, 25);

		m_lblFilling2.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 475, m_intCaptionSize.x, 50);

		m_btnGo.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 525, m_intCaptionSize.x, 25);

		m_lblFilling3.MoveWindow(m_ClientRect.right-m_intCaptionSize.x, 550, m_intCaptionSize.x, m_ClientRect.bottom);

		m_lblCaptionArea.ShowWindow(FALSE);
		m_lstPositions.ShowWindow(FALSE);

		m_lblStimNumbX.ShowWindow(TRUE);
		m_lblStimNumbY.ShowWindow(TRUE);
		m_lblCellSizeX.ShowWindow(TRUE);
		m_lblCellSizeY.ShowWindow(TRUE);
		m_edtStimNumbX.ShowWindow(TRUE);
		m_edtStimNumbY.ShowWindow(TRUE);
		m_edtCellSizeX.ShowWindow(TRUE);
		m_edtCellSizeY.ShowWindow(TRUE);

		m_lblIlluminationTime.ShowWindow(TRUE);
		m_edtIlluminationTime.ShowWindow(TRUE);
		m_lblFilling1.ShowWindow(FALSE);
		m_lblFilling2.ShowWindow(TRUE);
		m_lblFilling3.ShowWindow(TRUE);
		m_btnGo.ShowWindow(TRUE);
	}

	m_rect.left            = 0;
	m_rect.top             = 0;
	m_rect.right           = 0;
	m_rect.bottom          = 0;

	aoslo_movie.RandDelivering = FALSE;
}

void CViewDewarpVideo::FontFormat(CHARFORMAT &cf)
{
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask = CFM_BOLD | CFM_COLOR | CFM_FACE |
                CFM_ITALIC | CFM_SIZE | CFM_UNDERLINE;
    cf.dwEffects = CFE_BOLD;
    cf.yHeight = 10 * 20;

    cf.crTextColor = RGB(0, 0, 255);
    strcpy_s(cf.szFaceName, "Arial");
    cf.bCharSet = 0;
    cf.bPitchAndFamily = 0;
}


// after [reset] button is clicked, the list of stimulus positions has
// to be updated when user select a new reference point.
void CViewDewarpVideo::UpdateStimulusPos(POINT pos)
{
	int      i, deltaX, deltaY;
	CString  text;

	deltaX = aoslo_movie.StimulusPos[0].x - pos.x;
	deltaY = aoslo_movie.StimulusPos[0].y - pos.y;
	aoslo_movie.StimulusPos[0].x = pos.x;
	aoslo_movie.StimulusPos[0].y = pos.y;
	for (i = 1; i < aoslo_movie.StimulusNum; i ++) {
		aoslo_movie.StimulusPos[i].x -= deltaX;
		aoslo_movie.StimulusPos[i].y -= deltaY;
	}
}


void CViewDewarpVideo::RunRandomStim() {
	CString  text;
	int      iTime;

	CICANDIDoc* pDoc = (CICANDIDoc *)GetDocument();
	ASSERT_VALID(pDoc);

	if (g_bFFTIsRunning == FALSE) return;
	if (aoslo_movie.DeliveryMode == 1 && aoslo_movie.StimulusNum <= 0) return;

	m_edtIlluminationTime.GetWindowText(text);
	text.TrimLeft();
	text.TrimRight();
	if (text.GetLength() == 0) return;
	iTime = atoi(text);
	if (iTime <= 0) return;
	if (iTime > 200) {
		iTime = 200;
		m_edtIlluminationTime.SetWindowText("200");
	}

	// psudo random grid delivery
	if (aoslo_movie.DeliveryMode == 2) {
		if (VerifyInputs() == FALSE) return;
	}

	for (int i = 0; i < MAX_STIMULUS_NUMBER; i ++)
		aoslo_movie.RandPathIndex[i] = i;

	if (GenRandomPath() == FALSE) {
		MessageBox("Random path can't be generated. Stimulii will be delivered with fixed path.", "Random Path", MB_ICONWARNING);
	}

	m_array.RemoveAll();
	m_array.Add(iTime);
	m_array.Add(0);
	m_array.Add(RANDOM_DELIVERY_FLAG);

	pDoc->UpdateAllViews(this, 0L, &m_array);

	aoslo_movie.RandDelivering = TRUE;
	m_btnGo.EnableWindow(FALSE);
}

BOOL CViewDewarpVideo::VerifyInputs() {
	int     cellX, cellY, StimX, StimY, deltaX, deltaY;
	int     i, j, idx, marginX, marginY;
	CString text;

	m_edtCellSizeX.GetWindowText(text);
	text.TrimLeft();
	text.TrimRight();
	if (text.GetLength() == 0) {
		deltaX = 0;
		return FALSE;
	} else {
		deltaX = atoi(text);
	}

	m_edtCellSizeY.GetWindowText(text);
	text.TrimLeft();
	text.TrimRight();
	if (text.GetLength() == 0) {
		deltaY = 0;
		return FALSE;
	} else {
		deltaY = atoi(text);
	}

	m_edtStimNumbX.GetWindowText(text);
	text.TrimLeft();
	text.TrimRight();
	if (text.GetLength() == 0) {
		StimX = 0;
		return FALSE;
	} else {
		StimX = atoi(text);
	}

	m_edtStimNumbY.GetWindowText(text);
	text.TrimLeft();
	text.TrimRight();
	if (text.GetLength() == 0) {
		StimY = 0;
		return FALSE;
	} else {
		StimY = atoi(text);
	}

	if (m_stimulusPos.x < 0 || m_stimulusPos.y < 0) {
		MessageBox("A reference spot is required", "Parameters", MB_ICONWARNING);
		return FALSE;
	}

	cellX = deltaX * (StimX-1);
	cellY = deltaY * (StimY-1);

	marginX = (aoslo_movie.dewarp_sx - aoslo_movie.width) / 2;
	marginY = (aoslo_movie.dewarp_sy - aoslo_movie.height) / 2;
	if (cellX < 0 || cellY < 0 || StimX < 0 || StimY < 0) {
		MessageBox("Invalid Inputs", "Parameters", MB_ICONWARNING);
		return FALSE;
	}

	if (m_stimulusPos.x-cellX <= marginX || m_stimulusPos.x+cellX >= aoslo_movie.width+marginX) {
		MessageBox("Specified cell width is too big", "Parameters", MB_ICONWARNING);
		return FALSE;
	}

	if (m_stimulusPos.y-cellY <= marginY || m_stimulusPos.y+cellY >= aoslo_movie.height+marginY) {
		MessageBox("Specified cell height is too big", "Parameters", MB_ICONWARNING);
		return FALSE;
	}

	if (StimX % 2 == 0) {
		StimX = (StimX/2)*2 + 1;
		text.Format("%d", StimX);
		m_edtStimNumbX.SetWindowText(text);
	}
	if (StimY % 2 == 0) {
		StimY = (StimY/2)*2 + 1;
		text.Format("%d", StimY);
		m_edtStimNumbY.SetWindowText(text);
	}

	if (StimX > 1) {
		deltaX = cellX / (StimX-1);
	} else {
		deltaX = 0;
	}
	if (StimY > 1) {
		deltaY = cellY / (StimY-1);
	} else {
		deltaY = 0;
	}

	if (aoslo_movie.StimulusPos[0].x > 0 && 
		aoslo_movie.StimulusPos[0].y > 0 &&
		aoslo_movie.DeliveryMode == 2) {
		m_stimulusPos.x = aoslo_movie.StimulusPos[0].x;
		m_stimulusPos.y = aoslo_movie.StimulusPos[0].y;
	}

	for (i = 0; i < StimX; i ++) {
		for (j = 0; j < StimY; j ++) {
			idx = i*StimY + j;
			aoslo_movie.StimulusPos[idx].x = m_stimulusPos.x + (i-StimX/2)*deltaX;
			aoslo_movie.StimulusPos[idx].y = m_stimulusPos.y + (j-StimY/2)*deltaY;
		}
	}

	aoslo_movie.StimulusNum = StimX*StimY;
	// the first index is for reference spot
	aoslo_movie.StimulusPos[StimX*StimY/2].x = aoslo_movie.StimulusPos[0].x;
	aoslo_movie.StimulusPos[StimX*StimY/2].y = aoslo_movie.StimulusPos[0].y;

	aoslo_movie.StimulusPos[0].x = m_stimulusPos.x;
	aoslo_movie.StimulusPos[0].y = m_stimulusPos.y;

	return TRUE;
}

// generate a psudo-random patch so that stimuli can be
// delivered to positions in different orders in each cycle
BOOL CViewDewarpVideo::GenRandomPath()
{
	int         i, k, x, len, stims, frames, iTime;
	int        *tempPath;
	CString     text;
	CICANDIDoc* pDoc = (CICANDIDoc *)GetDocument();

	m_edtIlluminationTime.GetWindowText(text);
	text.TrimLeft();
	text.TrimRight();
	iTime = atoi(text);

	stims    = aoslo_movie.StimulusNum;
	frames   = 30*iTime*aoslo_movie.StimulusNum;
	if (pDoc->m_bValidAviHandleA == TRUE && pDoc->m_bValidAviHandleB == TRUE) {
		frames = (frames > 9000) ? 9000 : frames;
	} else if (pDoc->m_bValidAviHandleA == TRUE) {
		frames = (frames > 9000) ? 9000 : frames;
	}
	// number of patches
	len = frames / (stims*aoslo_movie.FlashDist) + 5;

	if (aoslo_movie.RandPathIndice != NULL) 
		delete [] aoslo_movie.RandPathIndice;

	tempPath = new int [stims];
	aoslo_movie.RandPathIndice = new int [len*stims];	
	if (tempPath == NULL || aoslo_movie.RandPathIndice == NULL) return FALSE;

	srand(g_sampling_counter);

//	FILE  *fp;
//	fp = fopen("randIdxA.txt", "w");

	for (k = 0; k < len; k ++) {

		for (i = 0; i < stims; i ++)
			tempPath[i] = -1;

		x = rand() % stims;

		for (i = 0; i < stims; i ++)
		{
			if (tempPath[x] == -1) {
				tempPath[x] = i;
			} else {
				i --;
				x = rand() % stims;
			}
		}

//		fprintf(fp, "%d: ", k);
		for (i = 0; i < stims; i ++) {
			aoslo_movie.RandPathIndice[k*stims+i]=tempPath[i];
//			fprintf(fp, "%d,", tempPath[i]);
		}
//		fprintf(fp, "\n");
	}

	g_sampling_counter = (aoslo_movie.StimulusNum*aoslo_movie.FlashDist)*(g_sampling_counter/(aoslo_movie.StimulusNum*aoslo_movie.FlashDist));
	aoslo_movie.FrameCountOffset = g_sampling_counter/(aoslo_movie.StimulusNum*aoslo_movie.FlashDist);
	g_sampling_counter --;

//	fprintf(fp, "frame counter: %d,", g_sampling_counter);

//	fclose(fp);

//	aoslo_movie.fpTesting = fopen("randIdxB.txt", "w");

	delete [] tempPath;

	return TRUE;
}

void CViewDewarpVideo::UpdateStimPos(CPoint point) 
{	
	int    i, rad_sq = 25;//, cx, cy;
	BOOL   b_overlap = FALSE;

	if (point.x < 0 || point.y < 0 || point.x >= aoslo_movie.dewarp_sx || point.y >= aoslo_movie.dewarp_sy) return;

//	g_nStimPosBak_Matlab_Retreive.x = point.x;
//	g_nStimPosBak_Matlab_Retreive.y = point.y;
	switch (g_ICANDIParams.FRAME_ROTATION)	{
		case 0: //no rotation or flip				
			break;
		case 1: //rotate 90 deg
			i = point.x;
			point.x = point.y;
			point.y = aoslo_movie.dewarp_sx-i-1;
			break;
		case 2: //flip along Y axis
			point.x = aoslo_movie.dewarp_sx-point.x-1;
			break;
		default:
			;
	}

	g_nStimPosBak_Gain0Track.x = g_StimulusPos0G.x = m_stimulusPos.x = g_StimulusPosBak.x = point.x;//g_nStimPosBak_Matlab.x = 
	g_nStimPosBak_Gain0Track.y = g_StimulusPos0G.y = m_stimulusPos.y = g_StimulusPosBak.y = point.y;//g_nStimPosBak_Matlab.y = 

	// TODO: Add your message handler code here and/or call default
	if (m_bMessageArrive == TRUE && m_msgID > 0) {
		if (point.x < 0 || point.y < 0 || point.x >= aoslo_movie.dewarp_sx || point.y >= aoslo_movie.dewarp_sy) return;

		for (i = 0; i < m_iMousePts; i ++) {
			if ((m_ptMousePos[i].x-point.x)*(m_ptMousePos[i].x-point.x) +
				(m_ptMousePos[i].y-point.y)*(m_ptMousePos[i].y-point.y) <= rad_sq) {
				b_overlap = TRUE;
				break;
			}
		}

		m_stimulusPos.x = point.x;
		m_stimulusPos.y = point.y;
		g_nStimPosBak_Matlab.x = g_StimulusPosBak.x = point.x;
		g_nStimPosBak_Matlab.y = g_StimulusPosBak.y = point.y;

		if (b_overlap == TRUE) {
			m_ptMousePos[i].x = m_stimulusPos.x;
			m_ptMousePos[i].y = m_stimulusPos.y;
		} else {
			m_ptMousePos[m_iMousePts].x = m_stimulusPos.x;
			m_ptMousePos[m_iMousePts].y = m_stimulusPos.y;
			m_iMousePts ++;

			if (m_iMousePts >= 20) {
				for (i = 0; i < 19; i ++) {
					m_ptMousePos[i].x = m_ptMousePos[i+1].x;
					m_ptMousePos[i].y = m_ptMousePos[i+1].y;
				}
				m_iMousePts = 19;
			}
		}
		aoslo_movie.stimulus_loc_flag = TRUE;
	} else {
		if (point.x < 0 || point.y < 0 || point.x >= aoslo_movie.dewarp_sx || point.y >= aoslo_movie.dewarp_sy) return;
		g_StimulusPosBak.x = point.x;
		g_StimulusPosBak.y = point.y;

		CICANDIDoc* pDoc = (CICANDIDoc*)GetDocument();
		pDoc->DrawStimulus();
	}

	SetFocus();
}