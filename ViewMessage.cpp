// ViewMessage.cpp : implementation file
//

#include "stdafx.h"
#include "ICANDI.h"
#include "ViewMessage.h"

#include "ICANDIParams.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CViewMessage

extern AOSLO_MOVIE  aoslo_movie;
extern CView       *g_viewDltVideo;
extern ICANDIParams    g_ICANDIParams;
extern POINT        g_StimulusPos;
extern BOOL         g_bHistgram;

IMPLEMENT_DYNCREATE(CViewMessage, CView)

CViewMessage::CViewMessage()
{
	m_PenWhite.CreatePen(PS_SOLID, 1, RGB(255, 255, 255));
	m_PenRed.CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
	m_PenBlue.CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
	m_PenWhiteThick.CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
	m_PenBlueThick.CreatePen(PS_SOLID, 2, RGB(0, 0, 255));

	m_PenGreen.CreatePen(PS_SOLID, 1, RGB(0, 127, 0));
	m_PenGray.CreatePen(PS_SOLID, 1, RGB(127, 127, 127));
	m_PenGreenD.CreatePen(PS_DOT, 1, RGB(0, 127, 0));
	m_PenGrayD.CreatePen(PS_DOT, 1, RGB(127, 127, 127));

	m_FontGraph.CreateFont(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
							ANSI_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							DEFAULT_PITCH | FF_MODERN, "Arial");
	m_FontMeanVal.CreateFont(32, 0, 0, 0, FW_NORMAL, FALSE, FALSE, 0,
							ANSI_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							DEFAULT_PITCH | FF_MODERN, "Arial");
	m_bShowDeltas    = TRUE;
	m_bMessageArrive = FALSE;
	m_bWindowCreated = FALSE;
	m_FramesNumber   = 0;
	m_msgID          = 0;
	m_sizeValidRect.cx = m_sizeValidRect.cy = 0;
	m_nCount0        = 0;
	m_nCount1        = 0;

	g_viewDltVideo   = this;

	m_ptsDeltaX0     = new POINT [1250];
	m_ptsDeltaY0     = new POINT [1250];
	m_ptsDeltaX1     = new POINT [1250];
	m_ptsDeltaY1     = new POINT [1250];
	m_initialized    = FALSE;
	m_ScalingFactor  = 2;

	g_bHistgram      = TRUE;
	m_HistCordOld    = new POINT [256];
	m_HistCordNew    = new POINT [256];
	for (int i = 0; i < 256; i ++) {
		m_HistCordOld[i].x = m_HistCordNew[i].x = 0;
		m_HistCordOld[i].y = m_HistCordNew[i].y = 0;
	}
}

CViewMessage::~CViewMessage()
{
	if (m_ptsDeltaX0)   delete [] m_ptsDeltaX0;
	if (m_ptsDeltaY0)   delete [] m_ptsDeltaY0;
	if (m_ptsDeltaX1)   delete [] m_ptsDeltaX1;
	if (m_ptsDeltaY1)   delete [] m_ptsDeltaY1;
	delete [] m_HistCordOld;
	delete [] m_HistCordNew;
}


BEGIN_MESSAGE_MAP(CViewMessage, CView)
	//{{AFX_MSG_MAP(CViewMessage)
	ON_WM_PAINT()
	ON_MESSAGE(WM_MOVIE_SEND, OnSendMovie)
	ON_WM_SIZE()
	ON_WM_CREATE()
	ON_BN_CLICKED(ID_HIST_RADIO, UpdateHistogram)
	ON_BN_CLICKED(ID_STAB_RADIO, UpdateStabilization)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CViewMessage drawing

void CViewMessage::OnDraw(CDC* pDC)
{
	CDocument* pDoc = GetDocument();
	// TODO: add draw code here

	//CPaintDC dc(this); // device context for painting
}

/////////////////////////////////////////////////////////////////////////////
// CViewMessage diagnostics

#ifdef _DEBUG
void CViewMessage::AssertValid() const
{
	CView::AssertValid();
}

void CViewMessage::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}
#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CViewMessage message handlers

void CViewMessage::OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint) 
{
	if (pHint == NULL) {
	} else {
		CByteArray  *msgArray = (CByteArray*)pHint;
		BYTE  id0 = msgArray->GetAt(0);
		BYTE  id1 = msgArray->GetAt(1);
		BYTE  id2 = msgArray->GetAt(2);
		
		//m_bShowDeltas = id1;

//		if (id0 == CLEAR_STIMULUS_POINT && m_bWindowCreated) {
		if (id2 == RESET_REF_FRAME && m_bWindowCreated) {
			// clear displaying buffer
			for (int i = 0; i < 1250; i ++) {
				m_ptsDeltaX0[i].x = m_ptsDeltaY0[i].x = m_ptsDeltaX1[i].x = m_ptsDeltaY1[i].x = i+6;
				m_ptsDeltaX0[i].y = m_ptsDeltaX1[i].y = m_offsetY*3/4;
				m_ptsDeltaY0[i].y = m_ptsDeltaY1[i].y = m_offsetY*3/2;
			}
		}
	}
}

void CViewMessage::DrawGraphFrame(CPaintDC *pDC)
{
	RECT    rect;
	int     i, deltaX, deltaY;
	double  pos, dx, dy;

	GetClientRect(&rect);
	rect.left   += 5;
	rect.top    += 5;
	rect.bottom -= 5;
	rect.right  -= 5;

	// draw a rectangular frame
	pDC->Rectangle(&rect);

	// draw scale-X on this frame
	deltaX = rect.right - rect.left;
	deltaY = rect.bottom - rect.top;
	dx = deltaX / 20.0;
	dy = deltaY / 10.0;
	for (pos = rect.top; pos < rect.bottom; pos += dy) {
		i = (int)pos;
		pDC->MoveTo(rect.left, i);
		pDC->LineTo(rect.left+5, i);
	}

	for (pos = rect.left; pos < rect.right; pos += dx) {
		i = (int)pos;
		pDC->MoveTo(i, rect.bottom);
		pDC->LineTo(i, rect.bottom-5);
	}
}

void CViewMessage::DrawGraphLegends(CPaintDC *pDC)
{
	RECT    rect;

	GetClientRect(&rect);
	rect.left   += 5;
	rect.top    += 5;
	rect.bottom -= 5;
	rect.right  -= 5;

	// draw legends
	pDC->Rectangle(rect.right-150, rect.top+10, rect.right-10, rect.top+55);

	m_FontOld = pDC->SelectObject(&m_FontGraph);
	pDC->TextOut(rect.right-130, rect.top+15, "Delta X,     Offset X");
	pDC->TextOut(rect.right-130, rect.top+35, "Delta Y,     Offset Y");
	pDC->SelectObject(m_FontOld);

	m_PenOld = pDC->SelectObject(&m_PenBlue);
	pDC->MoveTo(rect.right-135, rect.top+23);
	pDC->LineTo(rect.right-145, rect.top+23);
	pDC->SelectObject(m_PenOld);
	m_PenOld = pDC->SelectObject(&m_PenRed);
	pDC->MoveTo(rect.right-135, rect.top+43);
	pDC->LineTo(rect.right-145, rect.top+43);
	pDC->SelectObject(m_PenOld);

	m_PenOld = pDC->SelectObject(&m_PenGray);
	pDC->MoveTo(rect.right-68, rect.top+23);
	pDC->LineTo(rect.right-78, rect.top+23);
	pDC->SelectObject(m_PenOld);
	m_PenOld = pDC->SelectObject(&m_PenGreen);
	pDC->MoveTo(rect.right-68, rect.top+43);
	pDC->LineTo(rect.right-78, rect.top+43);
	pDC->SelectObject(m_PenOld);

	m_PenOld = pDC->SelectObject(&m_PenGray);
	pDC->MoveTo(rect.left, m_offsetY*3/4);
	pDC->LineTo(rect.right, m_offsetY*3/4);
	pDC->SelectObject(m_PenOld);
	m_PenOld = pDC->SelectObject(&m_PenGrayD);
	pDC->MoveTo(rect.left, m_offsetY*3/4+m_ScalingFactor*3);
	pDC->LineTo(rect.right, m_offsetY*3/4+m_ScalingFactor*3);
	pDC->MoveTo(rect.left, m_offsetY*3/4-m_ScalingFactor*3);
	pDC->LineTo(rect.right, m_offsetY*3/4-m_ScalingFactor*3);
	pDC->SelectObject(m_PenOld);

	m_FontOld = pDC->SelectObject(&m_FontGraph);
	pDC->TextOut(rect.left+10, m_offsetY*3/4-m_ScalingFactor*3-20, "+3 pixels");
	pDC->TextOut(rect.left+10, m_offsetY*3/4+m_ScalingFactor*3+5, "-3 pixels");
	pDC->SelectObject(m_FontOld);


	m_PenOld = pDC->SelectObject(&m_PenGreen);
	pDC->MoveTo(rect.left, m_offsetY*3/2);
	pDC->LineTo(rect.right, m_offsetY*3/2);
	pDC->SelectObject(m_PenOld);
	m_PenOld = pDC->SelectObject(&m_PenGreenD);
	pDC->MoveTo(rect.left, m_offsetY*3/2+m_ScalingFactor*3);
	pDC->LineTo(rect.right, m_offsetY*3/2+m_ScalingFactor*3);
	pDC->MoveTo(rect.left, m_offsetY*3/2-m_ScalingFactor*3);
	pDC->LineTo(rect.right, m_offsetY*3/2-m_ScalingFactor*3);
	pDC->SelectObject(m_PenOld);

	m_FontOld = pDC->SelectObject(&m_FontGraph);
	pDC->TextOut(rect.left+10, m_offsetY*3/2-m_ScalingFactor*3-20, "+3 pixels");
	pDC->TextOut(rect.left+10, m_offsetY*3/2+m_ScalingFactor*3+5,  "-3 pixels");
	pDC->SelectObject(m_FontOld);
	
}

void CViewMessage::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	int      i, x, y, dewarp_sx, dewarp_sy, sd, sat;
	CString  msg1, msg2, msg3;
	BYTE     mean_val;

	DrawGraphFrame(&dc);

	if (m_bMessageArrive) {
		if (g_bHistgram == TRUE) {
			CalcHistogram(&mean_val, &sd, &sat);
			// erase old curve
			m_PenOld = dc.SelectObject(&m_PenWhiteThick);
			dc.Polyline(m_HistCordOld, 256);
			dc.SelectObject(m_PenOld);
			// draw new curves
			m_PenOld = dc.SelectObject(&m_PenBlueThick);
			dc.Polyline(m_HistCordNew, 256);
			dc.SelectObject(m_PenOld);
			// display mean value at upper right conner
			msg1.Format("Mean = %d", mean_val);
			msg2.Format("SD = %d", sd);
			msg3.Format("Sat. = %d", sat);
			m_FontOld = dc.SelectObject(&m_FontMeanVal);
			dc.TextOut(m_sizeValidRect.cx - 185, 30, msg1);
			dc.TextOut(m_sizeValidRect.cx - 185, 60, msg2);
			dc.TextOut(m_sizeValidRect.cx - 185, 90, msg3);
			dc.SelectObject(m_FontOld);
		}

		//if (m_msgID > 0 && m_msgID <= m_FramesNumber) {
		if (m_msgID > 0) {
			if (g_bHistgram == TRUE) {
			} else {
				// copy the new coming data for display
				CopyDeltas2Coords();

				dewarp_sx = (aoslo_movie.dewarp_sx - aoslo_movie.width) / 2;
				dewarp_sy = (aoslo_movie.dewarp_sy - aoslo_movie.height) / 2;
				x = g_StimulusPos.x - dewarp_sx;		// position X on the reference frame
				y = g_StimulusPos.y - dewarp_sy;		// position Y on the reference frame

				if (aoslo_movie.no_dewarp_video == FALSE && x > 0 && y > 0) {
					// erase the background
					if (m_msgID > 1) { 
						m_PenOld = dc.SelectObject(&m_PenWhite);
						dc.Polyline(m_ptsDeltaX0, m_nCount0);
						dc.Polyline(m_ptsDeltaY0, m_nCount0);
						dc.SelectObject(m_PenOld);
					}

					// draw new curves
					m_PenOld = dc.SelectObject(&m_PenBlue);
					dc.Polyline(m_ptsDeltaX1, m_nCount1);
					dc.SelectObject(m_PenOld);
					m_PenOld = dc.SelectObject(&m_PenRed);
					dc.Polyline(m_ptsDeltaY1, m_nCount1);
					dc.SelectObject(m_PenOld);

					// copy current deltas to the old buffer
					for (i = 0; i < m_sizeValidRect.cx; i ++) {
						m_ptsDeltaX0[i].y = m_ptsDeltaX1[i].y;
						m_ptsDeltaY0[i].y = m_ptsDeltaY1[i].y;
					}
				}
				
				// draw legends of the two curves
				DrawGraphLegends(&dc);
			}
		} else if (m_msgID == 0) {
			//dc.TextOut(50, 10, "Stimulus beam is turned off ......                        ");
		} else if (m_msgID == -1) {
			//dc.TextOut(50, 40, "FFT is on idle                                            ");
		}
	}
}


LRESULT CViewMessage::OnSendMovie(WPARAM wParam, LPARAM lParam)
{
//	RECT  rect;

	m_msgID = lParam;

	switch (m_msgID) {
	case MOVIE_HEADER_ID:
		m_bMessageArrive = FALSE;
		break;
	case SENDING_MOVIE:
		m_bMessageArrive = TRUE;
		break;
	case SEND_MOVIE_DONE:
		m_bMessageArrive = FALSE;
		break;
	default:
		break;
	}
	
	if (m_bMessageArrive) {
		OnPaint();
		Invalidate(FALSE);
	}

	return 0;
}



void CViewMessage::OnSize(UINT nType, int cx, int cy) 
{
	int i;

	CView::OnSize(nType, cx, cy);
	
	if (m_bWindowCreated == TRUE) {
		RECT    rect;

		GetClientRect(&rect);

		m_sizeValidRect.cx = rect.right - rect.left - 16;
		m_sizeValidRect.cy = rect.bottom - rect.top - 16;

		m_offsetY = (int)(0.5*m_sizeValidRect.cy);

		for (i = 0; i < 1250; i ++) {
			m_ptsDeltaX0[i].x = m_ptsDeltaY0[i].x = m_ptsDeltaX1[i].x = m_ptsDeltaY1[i].x = i+6;
			m_ptsDeltaX0[i].y = m_ptsDeltaX1[i].y = m_offsetY*3/4;
			m_ptsDeltaY0[i].y = m_ptsDeltaY1[i].y = m_offsetY*3/2;
		}

		for (i = 0; i < 256; i ++) {
			m_HistCordOld[i].x = m_HistCordNew[i].x = i*2 + 16;
		}
	}
}

int CViewMessage::CopyDeltas2Coords()
{
	int    i, m, n;

	m_offsetY = (int)(0.5*m_sizeValidRect.cy);
	m = m_msgID - 1;

	// the current curve length is greater than the size of the displaying frame 
	if (m > m_sizeValidRect.cx) {
		// move the elements one pixel ahead
		for (i = 0; i < m_sizeValidRect.cx-1; i ++) {
			m_ptsDeltaX1[i].y = m_ptsDeltaX1[i+1].y;
			m_ptsDeltaY1[i].y = m_ptsDeltaY1[i+1].y;
		}

		m_nCount0 = m_nCount1 = m_sizeValidRect.cx;
	} else {
		m_nCount0 = m_msgID - 1;
		m_nCount1 = m_msgID;
	}

	// fill the last elements with the deltas from the current frame
	n = m_nCount1-1;
	if (abs(aoslo_movie.StimulusResX) > 8 || abs(aoslo_movie.StimulusResY) > 8) {
		m_ptsDeltaX1[n].y = m_offsetY*3/4;
		m_ptsDeltaY1[n].y = m_offsetY*3/2;
	} else {
		m_ptsDeltaX1[n].y = (int)(aoslo_movie.StimulusResX*m_ScalingFactor + m_offsetY*3/4);
		m_ptsDeltaY1[n].y = (int)(aoslo_movie.StimulusResY*m_ScalingFactor + m_offsetY*3/2);
	}

	return 0;
}

int CViewMessage::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CView::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	m_bWindowCreated = TRUE;
	
	RECT  rect;

	// draw a radio button to select histogram
	rect.left = 20;	 rect.right  = rect.left +100;
	rect.top  = 15;  rect.bottom = rect.top + 20;
	m_histCurve.Create("Histogram", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP, rect, this, ID_HIST_RADIO);
	m_histCurve.SetCheck(1);

	// draw a radio button to select stabilization curve
	rect.left = 120; rect.right  = rect.left + 100;
	rect.top  = 15;  rect.bottom = rect.top + 20;
	m_stabCurve.Create("Stabilization", WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_TABSTOP,  rect, this, ID_STAB_RADIO);


	return 0;
}


void CViewMessage::UpdateHistogram() 
{
	if (m_histCurve.GetCheck() == 0) {
		m_histCurve.SetCheck(1);
		m_stabCurve.SetCheck(0);
	} else if (m_histCurve.GetCheck() == 1) {
		m_stabCurve.SetCheck(0);

	} else {
		m_histCurve.SetCheck(0);
	}
	g_bHistgram = TRUE;
}

void CViewMessage::UpdateStabilization()
{
	if (m_stabCurve.GetCheck() == 0) {
		m_stabCurve.SetCheck(1);
		m_histCurve.SetCheck(0);
	} else if (m_stabCurve.GetCheck() == 1) {
		m_histCurve.SetCheck(0);
	} else {
		m_stabCurve.SetCheck(0);
	}
	g_bHistgram = FALSE;
}

void CViewMessage::CalcHistogram(BYTE *mean_val, int *sd, int *sat)
{
	int   *hist;
	int    i, j, imax, sum_val;
	long   sum_sq;
	BYTE   mval;

	hist = new int [256];

	for (i = 0; i < 256; i ++) hist[i] = 0;
	sum_val = 0;
	for (i = 0; i < aoslo_movie.width*aoslo_movie.height; i ++) {
		j = aoslo_movie.video_ins[i];
		if (j < 0 || j > 255) j = 0;
		hist[j] ++;
		sum_val += aoslo_movie.video_ins[i];
	}
	// calculate mean
	*mean_val = (BYTE)(sum_val*1.0/(aoslo_movie.width*aoslo_movie.height));
	mval = *mean_val;

	imax = 0;
	for (i = 0; i < 256; i ++) {
		imax = hist[i] > imax ? hist[i] : imax;
	}
	sum_sq = 0;
	for (i = 0; i < aoslo_movie.width*aoslo_movie.height; i ++) {
		sum_sq += (aoslo_movie.video_in[i]-mval)*(aoslo_movie.video_in[i]-mval);
	}
	// calculate standard deviation
	*sd = (int)(sqrt(sum_sq*1.0/(aoslo_movie.width*aoslo_movie.height-1)));

	// map histogram to client area
	for (i = 0; i < 256; i ++) {
		m_HistCordOld[i].y = m_HistCordNew[i].y;
		m_HistCordNew[i].y = (int)(m_sizeValidRect.cy*(1-(0.75*hist[i]/imax)));
	}

	*sat = hist[255];
	delete [] hist;
}
