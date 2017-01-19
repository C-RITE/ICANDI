// CalDesinu.cpp : implementation file
//

#include "stdafx.h"
#include "MSCGui.h"
#include "CalDesinu.h"

#include <math.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CCalDesinu dialog


CCalDesinu::CCalDesinu(CWnd* pParent /*=NULL*/)
	: CDialog(CCalDesinu::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCalDesinu)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	m_info_height    = 100;
	m_curve_height   = 280;
	m_margin_edgeH   = 20;
	m_margin_edgeV   = 20;
	m_margin_middle  = 40;
	m_button_width   = 90;
	m_button_height  = 25;
	m_img_pix_bnd    = 256;

	m_nGridSize      = 0.1;
	m_nGridNumb      = 30;
	m_nFreqHorz      = 20000000;
	m_nFreqVert      = 15785;

	m_frames         = 0;
	m_width          = 0;
	m_height         = 0;
	m_nFrameIdx      = 1;
	m_SliderStartPos = 1;
	m_SliderEndPos   = 1;
	m_pStream        = NULL;
	m_dispBufferRaw  = NULL;
	m_dispBufferDew  = NULL;
	m_BitmapInfo     = NULL;

	strcpy(m_LUTfname, "");
	m_bHasLUT        = FALSE;
	m_UnMatrixIdx    = NULL;
	m_UnMatrixVal    = NULL;

	m_AveGridX       = NULL;
	m_AveGridY       = NULL;

	m_DispGridX0     = new POINT[2048];
	m_DispGridY0     = new POINT[2048];
	m_DispGridX1     = new POINT[2048];
	m_DispGridY1     = new POINT[2048];
	for (int i = 0; i < 2048; i ++) {
		m_DispGridX0[i].x = m_DispGridX0[i].y = m_DispGridY0[i].x = m_DispGridY0[i].y = 0;
		m_DispGridX1[i].x = m_DispGridX1[i].y = m_DispGridY1[i].x = m_DispGridY1[i].y = 0;
	}

	m_localMinX      = new POINT[50];
	m_localMinY      = new POINT[50];
	for (int j = 0; j < 50; j ++) {
		m_localMinX[j].x = m_localMinX[j].y = 0;
		m_localMinY[j].x = m_localMinY[j].y = 0;
	}
	m_localPtsX      = 0;
	m_localPtsY      = 0;
}


void CCalDesinu::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCalDesinu)
		// NOTE: the ClassWizard will add DDX and DDV calls here
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCalDesinu, CDialog)
	//{{AFX_MSG_MAP(CCalDesinu)
	ON_WM_PAINT()
	ON_WM_DESTROY()
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_LOAD_GRIDS, OnLoadGrids)
	ON_BN_CLICKED(IDC_START_FRAME, OnStartFrame)
	ON_BN_CLICKED(IDC_END_FRAME, OnEndFrame)
	ON_BN_CLICKED(IDC_RUN_DESINU, OnRunDesinu)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONDBLCLK()
	ON_BN_CLICKED(IDC_SAVE_LUT, OnSaveLut)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CCalDesinu message handlers

BOOL CCalDesinu::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_hdc = this->GetDC()->m_hDC;
	

	COLORREF bkColor = GetDC()->GetBkColor();

	m_penBkColor.CreatePen(PS_SOLID, 1, bkColor);
	m_penRed.CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
	m_penBlue.CreatePen(PS_SOLID, 1, RGB(0, 0, 255));

	SetDlgItemText(IDC_EDIT_FREQ_HORZ, "20000000");
	SetDlgItemText(IDC_EDIT_FREQ_VERT, "15785");
	SetDlgItemText(IDC_EDIT_GRID_SIZE, "0.1");
	SetDlgItemText(IDC_EDIT_GRID_NUMB, "10");

	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CCalDesinu::OnPaint() 
{
	CPaintDC dc(this); // device context for painting
	
	if (m_dispBufferRaw != NULL) {
		::StretchDIBits(m_hdc, m_margin_edgeH, m_margin_edgeV*2+m_margin_middle+m_img_pix_bnd, m_width, m_height,
						0, 0, m_width, m_height, m_dispBufferRaw,
						m_BitmapInfo, NULL, SRCCOPY);
		DrawCurves(&dc);
	}
	if (m_bHasLUT == TRUE) {
		::StretchDIBits(m_hdc, m_margin_edgeH/2+m_margin_middle+m_width, m_margin_edgeV*2+m_margin_middle+m_img_pix_bnd, m_width, m_height,
						0, 0, m_width, m_height, m_dispBufferDew,
						m_BitmapInfo, NULL, SRCCOPY);
	}
}


void CCalDesinu::OnDestroy() 
{
	CDialog::OnDestroy();
	
	//close the stream after finishing the task
    if (m_pStream != NULL) {
		AVIStreamRelease(m_pStream);
		AVIFileExit();
	}

	if (m_dispBufferRaw != NULL) delete [] m_dispBufferRaw;
	if (m_dispBufferDew != NULL) delete [] m_dispBufferDew;
	if (m_BitmapInfo) free(m_BitmapInfo);

	if (m_AveGridX != NULL) delete [] m_AveGridX;
	if (m_AveGridY != NULL) delete [] m_AveGridY;
	
	if (m_UnMatrixIdx != NULL)   delete [] m_UnMatrixIdx;
	if (m_UnMatrixVal != NULL)   delete [] m_UnMatrixVal;

	delete [] m_DispGridX0;
	delete [] m_DispGridY0;
	delete [] m_DispGridX1;
	delete [] m_DispGridY1;
	delete [] m_localMinX;
	delete [] m_localMinY;
}


// here we override the virtual function of 
//     OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pSliderCtrl) 
void CCalDesinu::OnHScroll(UINT nSBCode, UINT nPos, CSliderCtrl* pSliderCtrl) 
{
    int		nTemp1, nTemp2;
	CString sFrameID;

    nTemp1 = pSliderCtrl->GetPos();

    switch(nSBCode) {
    case TB_THUMBPOSITION:
        pSliderCtrl->SetPos(nPos);
		nTemp1 = nPos;
        break;
    case TB_LINEUP: // left arrow button
        nTemp2 = 1;
        if ((nTemp1 - nTemp2) > 0) {
            nTemp1 -= nTemp2;
        } else {
            nTemp1 = 1;
        }
        pSliderCtrl->SetPos(nTemp1);
        break;
    case TB_LINEDOWN: // right arrow button
        nTemp2 = 1;
        if ((nTemp1 + nTemp2) < m_frames) {
            nTemp1 += nTemp2;
        } else {
            nTemp1 = m_frames;
        }
        pSliderCtrl->SetPos(nTemp1);
        break;
    case TB_PAGEUP: // left arrow button
        nTemp2 = 10;
        if ((nTemp1 - nTemp2) > 0) {
            nTemp1 -= nTemp2;
        } else {
            nTemp1 = 1;
        }
        pSliderCtrl->SetPos(nTemp1);
        break;
    case TB_PAGEDOWN: // right arrow button
        nTemp2 = 10;
        if ((nTemp1 + nTemp2) < m_frames) {
            nTemp1 += nTemp2;
        } else {
            nTemp1 = m_frames;
        }
        pSliderCtrl->SetPos(nTemp1);
        break;
	case TB_THUMBTRACK:
        pSliderCtrl->SetPos(nPos);
		nTemp1 = nPos;
		break;
    } 
	
	sFrameID.Format("%d/%d [%d~%d]", nTemp1, m_frames, m_SliderStartPos, m_SliderEndPos);
	CStatic *txtFrameID = (CStatic *)GetDlgItem(IDC_FRAME_ID_MSG);
	txtFrameID->SetWindowText(sFrameID);
	m_nFrameIdx = nTemp1;

	if (LoadVideo(m_pStream, m_nFrameIdx-1) == TRUE) {
		CalcXandY();

		m_localPtsX = 0;
		m_localPtsY = 0;

		if (m_bHasLUT == TRUE) UnwarpImage();

		OnPaint();
		RefreshFigures(TRUE, TRUE, TRUE, TRUE);
	}
	
//	CDialog::OnHScroll(nSBCode, nPos, pSliderCtrl);
}



void CCalDesinu::OnLButtonDblClk(UINT nFlags, CPoint point) 
{
	int   left, right, top, bottom;
	int   i, j, idx, idxL, idxR, deltaX, deltaY;
	int   min, offsetY, offsetX;

	if (m_AveGridX == NULL || m_AveGridY == NULL) return;

	// define a rectangular box limited to the coordinate for curve X
	top    = m_margin_edgeV*3/2;
	left   = m_margin_edgeH;
	bottom = top + m_img_pix_bnd;
	right  = left + m_width;
	CRect  rectX(left, top, right, bottom);

	// define a rectangular box limited to the coordinate for curve Y
	top    = m_margin_edgeV*3/2;
	left   = m_margin_edgeH/2 + m_margin_middle + m_width;
	bottom = top + m_img_pix_bnd;
	right  = left + m_height;
	CRect  rectY(left, top, right, bottom);

	if (m_nGridNumb < 1) 	m_nGridNumb = 10;
	deltaX = (int)(m_width/(m_nGridNumb*2.5));
	deltaY = (int)(m_height/(m_nGridNumb*2.5));
	
	offsetY = m_margin_edgeV*3/2 + m_img_pix_bnd;

	// click is dropped on curve X
	if (rectX.PtInRect(point) == TRUE) {
		//MessageBox("Click on curve X");
		offsetX = m_margin_edgeH;
		idx = point.x - offsetX;
		idxL = (idx - deltaX) >= 0 ? (idx - deltaX) : 0;
		idxR = (idx + deltaX) < m_width ? (idx + deltaX) : m_width-1;
		min = 1024; 
		for (i = idxL; i <= idxR; i ++) {
			if (min > m_AveGridX[i]) {
				min = m_AveGridX[i];
				idx = i;
			}
		}

		j = -1;
		for (i = 0; i < m_localPtsX; i ++) {
			if (m_localMinX[i].x == idx+offsetX) {
				j = i;
				break;
			}
		}
		if (j == -1) {
			for (i = 0; i < m_localPtsX; i ++) 
				if (m_localMinX[i].x > idx + offsetX) break;

			//for (j = i+1; j < m_localPtsX+1; j ++) {
			for (j = m_localPtsX; j >= i+1; j --) {
				m_localMinX[j].x = m_localMinX[j-1].x;
				m_localMinX[j].y = m_localMinX[j-1].y;
			}
			m_localMinX[i].x = idx + offsetX;
			m_localMinX[i].y = offsetY - min;

//			m_localMinX[m_localPtsX].x = idx + offsetX;
//			m_localMinX[m_localPtsX].y = offsetY - min;
			m_localPtsX ++;

			OnPaint();
			RefreshFigures(TRUE, FALSE, FALSE, FALSE);
		}
	/*
		CString  msg;
		msg.Format("Click on curve X, raw cord : [%d,%d], client cord: [%d,%d]", idx, min, idx+offsetX, offsetY-min);
		MessageBox(msg);*/
	}

	// click is dropped on curve Y
	if (rectY.PtInRect(point) == TRUE) {
		//MessageBox("Click on curve Y");
		offsetX = m_margin_edgeH/2 + m_margin_middle + m_width;
		idx = point.x - offsetX;
		idxL = (idx - deltaY) >= 0 ? (idx - deltaY) : 0;
		idxR = (idx + deltaY) < m_height ? (idx + deltaY) : m_height-1;
		min = 1024; 
		for (i = idxL; i <= idxR; i ++) {
			if (min > m_AveGridY[i]) {
				min = m_AveGridY[i];
				idx = i;
			}
		}

		j = -1;
		for (i = 0; i < m_localPtsY; i ++) {
			if (m_localMinY[i].x == idx+offsetX) {
				j = i;
				break;
			}
		}
		if (j == -1) {
			m_localMinY[m_localPtsY].x = idx + offsetX;
			m_localMinY[m_localPtsY].y = offsetY - min;
			m_localPtsY ++;

			OnPaint();
			RefreshFigures(FALSE, TRUE, FALSE, FALSE);
		}
	/*
		CString  msg;
		msg.Format("Click on curve X, raw cord : [%d,%d], client cord: [%d,%d]", idx, min, idx+offsetX, offsetY-min);
		MessageBox(msg);*/
	}
	
	// draw a "+" at the position of the local min found above

	CDialog::OnLButtonDblClk(nFlags, point);
}

void CCalDesinu::OnRButtonDown(UINT nFlags, CPoint point) 
{
	int   left, right, top, bottom;
	int   i, idx;

	// define a rectangular box limited to the coordinate for curve X
	top    = m_margin_edgeV*3/2;
	left   = m_margin_edgeH;
	bottom = top + m_img_pix_bnd;
	right  = left + m_width;
	CRect  rectX(left, top, right, bottom);

	// define a rectangular box limited to the coordinate for curve Y
	top    = m_margin_edgeV*3/2;
	left   = m_margin_edgeH/2 + m_margin_middle + m_width;
	bottom = top + m_img_pix_bnd;
	right  = left + m_height;
	CRect  rectY(left, top, right, bottom);

	// click is dropped on curve X, remove selected mark "+"
	if (rectX.PtInRect(point) == TRUE) {
		idx = -1;
		for (i = 0; i < m_localPtsX; i ++) {
			if (abs(point.x - m_localMinX[i].x) < 5 && abs(point.y - m_localMinX[i].y) < 5) {
				idx = i;
			}
		}
		// one mark will be removed
		if (idx >= 0) {
			for (i = idx+1; i < m_localPtsX; i ++) {
				m_localMinX[i-1].x = m_localMinX[i].x;
				m_localMinX[i-1].y = m_localMinX[i].y;
			}
			m_localMinX[m_localPtsX-1].x = 0;
			m_localMinX[m_localPtsX-1].y = 0;
			
			// descrease total marks by 1
			m_localPtsX --;
	
			// refresh figure
			OnPaint();
			RefreshFigures(TRUE, FALSE, FALSE, FALSE);
		}
	}
	
	// click is dropped on curve Y, remove selected mark "+"
	if (rectY.PtInRect(point) == TRUE) {
		idx = -1;
		for (i = 0; i < m_localPtsY; i ++) {
			if (abs(point.x - m_localMinY[i].x) < 5 && abs(point.y - m_localMinY[i].y) < 5) {
				idx = i;
			}
		}
		// one mark will be removed
		if (idx >= 0) {
			for (i = idx+1; i < m_localPtsX; i ++) {
				m_localMinY[i-1].x = m_localMinY[i].x;
				m_localMinY[i-1].y = m_localMinY[i].y;
			}
			m_localMinY[m_localPtsX-1].x = 0;
			m_localMinY[m_localPtsX-1].y = 0;

			// descrease total marks by 1
			m_localPtsY --;
	
			// refresh figure
			OnPaint();
			RefreshFigures(FALSE, TRUE, FALSE, FALSE);
		}
	}
	
	
	CDialog::OnRButtonDown(nFlags, point);
}


void CCalDesinu::OnRButtonDblClk(UINT nFlags, CPoint point) 
{
	int   left, right, top, bottom, i;

	// define a rectangular box limited to the coordinate for curve X
	top    = m_margin_edgeV*3/2;
	left   = m_margin_edgeH;
	bottom = top + m_img_pix_bnd;
	right  = left + m_width;
	CRect  rectX(left, top, right, bottom);

	// define a rectangular box limited to the coordinate for curve Y
	top    = m_margin_edgeV*3/2;
	left   = m_margin_edgeH/2 + m_margin_middle + m_width;
	bottom = top + m_img_pix_bnd;
	right  = left + m_height;
	CRect  rectY(left, top, right, bottom);

	// click is dropped on curve X, remove all marks "+"
	if (rectX.PtInRect(point) == TRUE) {
		m_localPtsX = 0;
		OnPaint();
		RefreshFigures(TRUE, FALSE, FALSE, FALSE);
		for (i = 0; i < m_localPtsX; i ++) m_localMinX[i].x = m_localMinX[i].y = 0;
	}
	
	// click is dropped on curve Y, remove all marks "+"
	if (rectY.PtInRect(point) == TRUE) {
		m_localPtsY = 0;
		OnPaint();
		RefreshFigures(FALSE, TRUE, FALSE, FALSE);
		for (i = 0; i < m_localPtsX; i ++) m_localMinY[i].x = m_localMinY[i].y = 0;
	}
	
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CCalDesinu::OnLoadGrids() 
{
	// open a .avi file
	char     BASED_CODE szFilter[] = "Uncompressed AVI Files (*.avi)|*.avi|";
	CString  aviFileName, sFrameID;
	int      frames, fWidth, fHeight, iFirstFrame;

	// open an .avi file
	CFileDialog fd(TRUE, "avi", NULL, NULL, szFilter);
	if (fd.DoModal() != IDOK) return;

	// get the path and file name of this .avi file
	aviFileName = fd.GetPathName();

	//close the stream after finishing the task
    if (m_pStream != NULL) {
		AVIStreamRelease(m_pStream);
		AVIFileExit();
	}

	// read .avi file header and the pointer to .avi stream
	m_pStream = GetAviStream(aviFileName, &frames, &fWidth, &fHeight, &iFirstFrame);
	if (m_pStream == NULL) {
		MessageBox("Error: can't open avi stream", "Desinusoiding", MB_ICONWARNING);
	} else {
		m_frames = frames; 
		m_width  = fWidth;
		m_height = fHeight;
		
		// allocate image and diaplay buffers for processing and display
		m_dispBufferRaw = new BYTE [m_width*m_height];
		m_AveGridX      = new int [m_width];
		m_AveGridY      = new int [m_height];

		// resize window and controls inside the window
		SetupWindowSize(fWidth, fHeight);

		// initialize drawing device
		if(m_BitmapInfo) free(m_BitmapInfo);
		if((m_BitmapInfo = CreateDIB(m_width, m_height)) == NULL) {
			MessageBox("Error: can't create bitmap information", "Desinusoiding", MB_ICONWARNING);
			return;
		}

		if (LoadVideo(m_pStream, m_nFrameIdx-1) == FALSE) return;

		// Update status of controls
		(GetDlgItem(IDC_RUN_DESINU))->EnableWindow(TRUE);
		(GetDlgItem(IDC_START_FRAME))->EnableWindow(TRUE);
		(GetDlgItem(IDC_END_FRAME))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT_FREQ_HORZ))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT_FREQ_VERT))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT_GRID_SIZE))->EnableWindow(TRUE);
		(GetDlgItem(IDC_EDIT_GRID_NUMB))->EnableWindow(TRUE);
		(GetDlgItem(IDC_SAVE_LUT))->EnableWindow(FALSE);

		m_nFrameIdx = 1;
		CSliderCtrl *sliderctrl = (CSliderCtrl *)GetDlgItem(IDC_FRAME_ID);
		sliderctrl->EnableWindow(TRUE);
		sliderctrl->SetRange(1, frames, TRUE);
		sliderctrl->SetPos(m_nFrameIdx);
		m_SliderStartPos = 1;
		m_SliderEndPos   = frames;
		sliderctrl->SetSelection(m_SliderStartPos, m_SliderEndPos);
		sFrameID.Format("%d/%d [%d~%d]", m_nFrameIdx, m_frames, m_SliderStartPos, m_SliderEndPos);
		((CStatic *)GetDlgItem(IDC_FRAME_ID_MSG))->SetWindowText(sFrameID);

		m_localPtsX      = 0;
		m_localPtsY      = 0;

		// calculate X and Y positions of the grid
		CalcXandY();

		OnPaint();
		Invalidate(TRUE);

		// we need to update the lookup table for dewarping because of new grids
		m_bHasLUT = FALSE;
	}
}

// determine the start frame for desinusoiding
void CCalDesinu::OnStartFrame() 
{
	CString      sFrameID;
	CSliderCtrl *sliderctrl = (CSliderCtrl *)GetDlgItem(IDC_FRAME_ID);

	m_SliderStartPos = sliderctrl->GetPos();
	sliderctrl->SetSelection(m_SliderStartPos, m_SliderEndPos);

	sFrameID.Format("%d/%d [%d~%d]", m_nFrameIdx, m_frames, m_SliderStartPos, m_SliderEndPos);
	CStatic *txtFrameID = (CStatic *)GetDlgItem(IDC_FRAME_ID_MSG);
	txtFrameID->SetWindowText(sFrameID);

	OnPaint();
	Invalidate(TRUE);
}

// determine the end frame for desinusoiding
void CCalDesinu::OnEndFrame() 
{
	CString      sFrameID;
	CSliderCtrl *sliderctrl = (CSliderCtrl *)GetDlgItem(IDC_FRAME_ID);

	m_SliderEndPos = sliderctrl->GetPos();
	sliderctrl->SetSelection(m_SliderStartPos, m_SliderEndPos);

	sFrameID.Format("%d/%d [%d~%d]", m_nFrameIdx, m_frames, m_SliderStartPos, m_SliderEndPos);
	CStatic *txtFrameID = (CStatic *)GetDlgItem(IDC_FRAME_ID_MSG);
	txtFrameID->SetWindowText(sFrameID);

	if (m_SliderEndPos > m_SliderStartPos) {
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		if (LoadVideo(m_pStream, m_SliderStartPos-1, m_SliderEndPos-1) == TRUE) {
			m_localPtsX      = 0;
			m_localPtsY      = 0;

			CalcXandY();
			OnPaint();
			Invalidate(TRUE);
		}
		SetCursor(LoadCursor(NULL, IDC_ARROW));
	} else {
		MessageBox("No frame selected", "Desinusoiding", MB_ICONWARNING);
	}
}


void CCalDesinu::OnRunDesinu() 
{
	int     retCode;

	if (VerifyInputs() == FALSE) return;

	retCode = MessageBox("Are you sure that you have selected all grid positions?", "Desinusoiding", MB_YESNO | MB_ICONQUESTION);
	if (retCode == IDNO) return;

	if (m_localPtsX <= 0 || m_localPtsY <= 0 ) {
		MessageBox("Stop: No grid points selected", "Desinusoiding", MB_ICONWARNING);
		return;
	}

	// now start desinusoiding
	if (m_UnMatrixIdx != NULL)   delete [] m_UnMatrixIdx;
	if (m_UnMatrixVal != NULL)   delete [] m_UnMatrixVal;
	if (m_dispBufferDew != NULL) delete [] m_dispBufferDew;

	m_dispBufferDew = new BYTE [m_width*m_height];
	m_UnMatrixIdx   = new int   [2*(m_width+m_height)];
	m_UnMatrixVal   = new float [2*(m_width+m_height)];
	
	BuildLUT(m_UnMatrixIdx, m_UnMatrixVal);

	UnwarpImage();

	m_bHasLUT = TRUE;
	(GetDlgItem(IDC_SAVE_LUT))->EnableWindow(TRUE);

	OnPaint();
	RefreshFigures(FALSE, FALSE, FALSE, TRUE);
}


// Save lookup table to an external file
void CCalDesinu::OnSaveLut() 
{
	// open a .avi file
	char     BASED_CODE szFilter[] = "Lookup table files (*.lut)|*.lut|", pname[120], fname[40] = "";
	CString  filename;
	int      len, i, ret, *header;
	FILE    *fp;

	header = new int [2];

	// open an .avi file
	CFileDialog fd(FALSE, "lut", NULL, NULL, szFilter);
	if (fd.DoModal() != IDOK) return;

	// get the path and file name of this .avi file
	filename = fd.GetFileName();
	len = filename.GetLength();
	for (i = 0; i < len; i ++) {
		fname[i] = filename.GetAt(i);
	}
	::GetCurrentDirectory(120, pname);
	strcat(pname, "\\");
	strcat(pname, fname);

	fp = fopen(pname, "wb");
	if (fp == NULL) {
		filename.Format("Error: Can't open file <%s> for writting.", pname);
		MessageBox(filename, "Desinusoiding", MB_ICONWARNING);
		return;
	}
	//fwrite
	header[0] = m_width;
	header[1] = m_height;
	ret = fwrite(header, sizeof(int), 2, fp);  // file header
//	ret = fwrite(m_UnMatrixIdx, sizeof(int), m_width*2, fp);
//	ret = fwrite(m_UnMatrixVal, sizeof(float), m_width*2, fp);
	ret = fwrite(m_UnMatrixIdx, sizeof(int), (m_width+m_height)*2, fp);
	ret = fwrite(m_UnMatrixVal, sizeof(float), (m_width+m_height)*2, fp);
	fclose(fp);

	strcpy(m_LUTfname, pname);

	delete [] header;
}


void CCalDesinu::OnOK() 
{
	CString filename;
	int     ret_code;

	filename.Format("%s", m_LUTfname);
	filename.TrimLeft(' ');
	filename.TrimRight(' ');
	if (filename.GetLength() == 0) {
		ret_code = MessageBox("No lookup table is saved. Are you sure to close the window?", "Desinusoiding", MB_YESNO | MB_ICONQUESTION);
		if (ret_code == IDNO) return;
	}
	
	CDialog::OnOK();
}


LPBITMAPINFO CCalDesinu::CreateDIB(int cx, int cy)
{
	LPBITMAPINFO lpBmi;
	int iBmiSize;

	iBmiSize = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 256;

	// Allocate memory for the bitmap info header.
	if((lpBmi = (LPBITMAPINFO)malloc(iBmiSize)) == NULL)
	{
		MessageBox("Error: can't allocate memory for Bitmap Information!", "Warning", MB_ICONWARNING);
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


PAVISTREAM CCalDesinu::GetAviStream(CString aviFileName, int *Frames, int *Width, int *Height, int *FirstFrame)
{
    int      index=0;
	CString  msg;

	// now we initialize the .avi file and read it into the buffer
    AVIFileInit();

    PAVIFILE avi;
    int res = AVIFileOpen(&avi, aviFileName, OF_READ, NULL);

    if (res != AVIERR_OK) {
        //an error occures
		SetCursor(LoadCursor(NULL, IDC_ARROW));
		msg = "Error: can't read file <" + aviFileName;
		MessageBox(msg, "Desinusoiding", MB_ICONWARNING);
        if (avi != NULL) AVIFileRelease(avi);
        
        return NULL;
    }

    AVIFILEINFO avi_info;
    AVIFileInfo(avi, &avi_info, sizeof(AVIFILEINFO));

    PAVISTREAM  pStream;
    res = AVIFileGetStream(avi, &pStream, streamtypeVIDEO /*video stream*/, 
                                               0 /*first stream*/);

	*Frames = avi_info.dwLength;
	//*Width  = avi_info.dwWidth;
	*Width  = (avi_info.dwWidth % 4 == 0) ? avi_info.dwWidth : (avi_info.dwWidth/4)*4+4;
	*Height = avi_info.dwHeight;

    if (res != AVIERR_OK) {
		SetCursor(LoadCursor(NULL, IDC_ARROW));
        if (pStream != NULL) AVIStreamRelease(pStream);
		MessageBox("Error: can't get avi file stream.", "Desinusoiding", MB_ICONWARNING);

        AVIFileExit();
        return NULL;
    }

    //do some task with the stream
    *FirstFrame = AVIStreamStart(pStream);
    if (*FirstFrame == -1) {
        //Error getteing the frame inside the stream
        if (pStream != NULL) AVIStreamRelease(pStream);
		MessageBox("Error: can't get frame index from the avi stream.", "Desinusoiding", MB_ICONWARNING);

        AVIFileExit();
        return NULL;
    }

	return pStream;
}


BOOL CCalDesinu::LoadVideo(PAVISTREAM pStream, int fIdxStart, int fIdxEnd)
{
	int              i, j, ret_code;
	CString          errMsg;
	PGETFRAME        pFrame;
	BITMAPINFOHEADER bih;
	BYTE            *imgTemp, *bytBuff;
	int             *imgBuff;

    pFrame = AVIStreamGetFrameOpen(pStream, NULL);

	bytBuff   = new BYTE[m_width*m_height];
	imgBuff   = new int [m_width*m_height];
	for (i = 0; i < m_width*m_height; i ++) imgBuff[i] = bytBuff[i] = 0;

	// read video stream to video buffer
    for (i = fIdxStart; i <= fIdxEnd; i ++)
    {
		// the returned is a packed DIB frame
		imgTemp = (BYTE*) AVIStreamGetFrame(pFrame, i);

		RtlMoveMemory(&bih.biSize, imgTemp, sizeof(BITMAPINFOHEADER));

		//now get the bitmap bits
		if (bih.biSizeImage < 1) {
			errMsg.Format("Error: can't read frame No. %d/%d. Do you want to continue?", i+1, m_frames);
			ret_code = MessageBox(errMsg, "Desinusoiding", MB_YESNO | MB_ICONWARNING);
			if (ret_code == IDNO) return FALSE;
		}
		// get rid of the header information and retrieve the real image 
		RtlMoveMemory(bytBuff, imgTemp+sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256, bih.biSizeImage);
		for (j = 0; j < m_width*m_height; j ++) 
			imgBuff[j] += bytBuff[j];
	}

    AVIStreamGetFrameClose(pFrame);
    
	for (i = 0; i < m_width*m_height; i ++) {
		imgBuff[i] = imgBuff[i] / (fIdxEnd-fIdxStart+1);
		m_dispBufferRaw[i] = (BYTE)imgBuff[i];
	}

	delete [] imgBuff;
	delete [] bytBuff;

	return TRUE;
}


BOOL CCalDesinu::LoadVideo(PAVISTREAM pStream, int frameIdx)
{
	int              ret_code;
	CString          errMsg;
	PGETFRAME        pFrame;
	BITMAPINFOHEADER bih;
	BYTE            *imgTemp;

    pFrame = AVIStreamGetFrameOpen(pStream, NULL);

	// read video stream to video buffer
	// the returned is a packed DIB frame
	imgTemp = (BYTE*) AVIStreamGetFrame(pFrame, frameIdx);

	RtlMoveMemory(&bih.biSize, imgTemp, sizeof(BITMAPINFOHEADER));

	//now get the bitmap bits
	if (bih.biSizeImage < 1) {
		errMsg.Format("Error: can't read frame No. %d/%d. Do you want to continue?", frameIdx+1, m_frames);
		ret_code = MessageBox(errMsg, "Desinusoiding", MB_YESNO | MB_ICONWARNING);
		if (ret_code == IDNO) return FALSE;
	}
	// get rid of the header information and retrieve the real image 
	RtlMoveMemory(m_dispBufferRaw, imgTemp+sizeof(BITMAPINFOHEADER)+sizeof(RGBQUAD)*256, bih.biSizeImage);

    AVIStreamGetFrameClose(pFrame);

	return TRUE;
}


BOOL CCalDesinu::SetupWindowSize(int fWidth, int fHeight)
{
	int     sizeX, sizeY, screenX, screenY, ox, oy, y1, y2;
	CString msg;

//	sizeX = fWidth + fHeight + m_margin_middle + 2*m_margin_edgeH;
	if (fWidth > fHeight) {
		sizeX = fWidth*2 + m_margin_middle + 2*m_margin_edgeH;
	} else {
		sizeX = fWidth + fHeight + m_margin_middle + 2*m_margin_edgeH;
	}
	sizeY = fHeight  + m_margin_middle + m_margin_edgeV + m_info_height + m_curve_height + m_button_height;

	screenX = ::GetSystemMetrics(SM_CXSCREEN);
	screenY = ::GetSystemMetrics(SM_CYSCREEN);

	if (sizeX > screenX || sizeY > screenY) {
		msg.Format("Warn: your screen resolution (%d,%d) pixels is too small to display all required information. Set it up larger than (%d,%d) pixles.", screenX, screenX, sizeX, sizeY);
		MessageBox(msg, "Desinusoiding", MB_ICONWARNING);
		return FALSE;
	}

	ox = (screenX - sizeX) / 2;
	oy = (screenY - sizeY) / 2 - 10;
	
	// resize this popup window
	MoveWindow(ox, oy, sizeX, sizeY, TRUE);

	// -----------------------
	// resize the controls
	// -----------------------

	CWnd  *wnd;

	// resize curve X and frame
	wnd = this->GetDlgItem(IDC_FRAME_X);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH/2, m_margin_edgeV/2, fWidth+m_margin_edgeH, m_img_pix_bnd+m_margin_edgeV*3/2, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	
	// resize curve Y and frame
	wnd = this->GetDlgItem(IDC_FRAME_Y);
	//wnd->SetWindowPos(&wndTop, m_margin_middle+fWidth, m_margin_edgeV/2, fWidth+m_margin_edgeH, m_img_pix_bnd+m_margin_edgeV*3/2, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	wnd->SetWindowPos(&wndTop, m_margin_middle+fWidth, m_margin_edgeV/2, fHeight+m_margin_edgeH, m_img_pix_bnd+m_margin_edgeV*3/2, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	
	// resize warpped image and frame
	wnd = this->GetDlgItem(IDC_FRAME_WARP);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH/2, m_margin_edgeV+m_margin_middle+m_img_pix_bnd, fWidth+m_margin_edgeH, fHeight+m_margin_edgeV*3/2, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	
	// resize unwarpped image and frame
	wnd = this->GetDlgItem(IDC_FRAME_UNWARP);
	wnd->SetWindowPos(&wndTop, m_margin_middle+fWidth, m_margin_edgeV+m_margin_middle+m_img_pix_bnd, fWidth+m_margin_edgeH, fHeight+m_margin_edgeV*3/2, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// resize buttons and the scroll bar
	y1 = m_margin_edgeV*4 + m_img_pix_bnd + m_margin_middle + fHeight - m_button_height/2;
	y2 = m_margin_edgeV*4 + m_img_pix_bnd + m_margin_middle + fHeight + m_button_height;

	// Text "horizontal frequency"
	wnd = this->GetDlgItem(IDC_FREQ_HORZ);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*2/2,                y1+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "horizontal frequency"
	wnd = this->GetDlgItem(IDC_EDIT_FREQ_HORZ);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*3/2+m_button_width, y1,   m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// Text "vertical frequency"
	wnd = this->GetDlgItem(IDC_FREQ_VERT);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*2/2,                y2+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "vertical frequency"
	wnd = this->GetDlgItem(IDC_EDIT_FREQ_VERT);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*3/2+m_button_width, y2,   m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// Text "grid size"
	wnd = this->GetDlgItem(IDC_GRID_SIZE);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*4/2+m_button_width*2, y1+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "grid size"
	wnd = this->GetDlgItem(IDC_EDIT_GRID_SIZE);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*5/2+m_button_width*3, y1, m_button_width/2, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// Text "grid number"
	wnd = this->GetDlgItem(IDC_GRID_NUMBER);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*4/2+m_button_width*2, y2+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "grid number"
	wnd = this->GetDlgItem(IDC_EDIT_GRID_NUMB);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*5/2+m_button_width*3, y2, m_button_width/2, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);


	// button "Load Grids File"
	wnd = this->GetDlgItem(IDC_LOAD_GRIDS);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*6/2+m_button_width*7/2, y1, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// button "Start Frame"
	wnd = this->GetDlgItem(IDC_START_FRAME);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*7/2+m_button_width*9/2, y1, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// button "End"
	wnd = this->GetDlgItem(IDC_END_FRAME);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*8/2+m_button_width*11/2, y1, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// button "Desinusoid"
	wnd = this->GetDlgItem(IDC_RUN_DESINU);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*9/2+m_button_width*13/2,y1, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// slider control "Frame ID"
	wnd = this->GetDlgItem(IDC_FRAME_ID);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*6/2+m_button_width*7/2, y2, m_button_width*2+m_margin_edgeH/2, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// text "Frame ID message"
	wnd = this->GetDlgItem(IDC_FRAME_ID_MSG);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*8/2+m_button_width*11/2, y2+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// button "Save LUT"
	wnd = this->GetDlgItem(IDC_SAVE_LUT);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*9/2+m_button_width*13/2,y2, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	wnd = this->GetDlgItem(IDOK);
	wnd->SetWindowPos(&wndTop, sizeX-m_margin_edgeH-m_button_width, y1, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	wnd = this->GetDlgItem(IDCANCEL);
	wnd->SetWindowPos(&wndTop, sizeX-m_margin_edgeH-m_button_width, y2, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

/*
	// resize buttons and the scroll bar
	y = m_margin_edgeV*4 + m_img_pix_bnd + m_margin_middle + fHeight + m_button_height;

	// button "Load Grids File"
	wnd = this->GetDlgItem(IDC_LOAD_GRIDS);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// button "Desinusoid"
	wnd = this->GetDlgItem(IDC_RUN_DESINU);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*3/2+m_button_width, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// slider control "Frame ID"
	wnd = this->GetDlgItem(IDC_FRAME_ID);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*2+m_button_width*2, y, m_button_width*3/2, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// text "Frame ID message"
	wnd = this->GetDlgItem(IDC_FRAME_ID_MSG);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*5/2+m_button_width*7/2, y+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// button "Start Frame"
	wnd = this->GetDlgItem(IDC_START_FRAME);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*7/2+m_button_width*9/2, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// button "End Frame"
	wnd = this->GetDlgItem(IDC_END_FRAME);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*4+m_button_width*11/2, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	wnd = this->GetDlgItem(IDOK);
	wnd->SetWindowPos(&wndTop, sizeX-m_margin_edgeH*3/2-m_button_width*2, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	wnd = this->GetDlgItem(IDCANCEL);
	wnd->SetWindowPos(&wndTop, sizeX-m_margin_edgeH-m_button_width, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// resize buttons and the scroll bar
	y = m_margin_edgeV*4 + m_img_pix_bnd + m_margin_middle + fHeight - m_button_height/2;
	// Text "horizontal frequency"
	wnd = this->GetDlgItem(IDC_FREQ_HORZ);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH, y+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "horizontal frequency"
	wnd = this->GetDlgItem(IDC_EDIT_FREQ_HORZ);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*3/2+m_button_width, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// Text "vertical frequency"
	wnd = this->GetDlgItem(IDC_FREQ_VERT);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*2+m_button_width*2, y+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "vertical frequency"
	wnd = this->GetDlgItem(IDC_EDIT_FREQ_VERT);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*5/2+m_button_width*3, y, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// Text "grid size"
	wnd = this->GetDlgItem(IDC_GRID_SIZE);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*3+m_button_width*4, y+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "grid size"
	wnd = this->GetDlgItem(IDC_EDIT_GRID_SIZE);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*7/2+m_button_width*5, y, m_button_width/2, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);

	// Text "grid number"
	wnd = this->GetDlgItem(IDC_GRID_NUMBER);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*4+m_button_width*11/2, y+5, m_button_width, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
	// EditBox "grid number"
	wnd = this->GetDlgItem(IDC_EDIT_GRID_NUMB);
	wnd->SetWindowPos(&wndTop, m_margin_edgeH*9/2+m_button_width*13/2, y, m_button_width/2, m_button_height, SWP_SHOWWINDOW | SWP_DRAWFRAME);
*/
	return TRUE;
}



BOOL CCalDesinu::CalcXandY() {
	if (m_dispBufferRaw == NULL) return FALSE;

	int    i, j, k, offsetX, offsetY;

	// average the grid in X and Y directions

	for (i = 0; i < m_width;  i ++) m_AveGridX[i] = 0;
	for (i = 0; i < m_height; i ++) m_AveGridY[i] = 0;

	for (j = 0; j < m_height; j ++) {
		k = j * m_width;
		for (i = 0; i < m_width; i ++) {
			m_AveGridX[i] += *(m_dispBufferRaw+k+i);
			m_AveGridY[j] += *(m_dispBufferRaw+k+i);
		}
	}

	for (i = 0; i < m_width;  i ++) {
		m_AveGridX[i] /= m_height;
	}

	for (i = 0; i < m_height; i ++) {
		m_AveGridY[i] /= m_width;
	}

	// now assign these grid to image buffer for display
	offsetX = m_margin_edgeH;
	offsetY = m_margin_edgeV*3/2 + m_img_pix_bnd;

	// copy current curve to the old buffer
	for (i = 0; i < m_width; i ++) {
		m_DispGridX0[i].y = m_DispGridX1[i].y;
	}
	for (i = 0; i < m_height; i ++) {
		m_DispGridY0[i].y = m_DispGridY1[i].y;
	}
	for (i = 0; i < m_width; i ++) {
		m_DispGridX1[i].x = offsetX + i;
		m_DispGridX1[i].y = offsetY - m_AveGridX[i];
	}

	// update the new buffer, for X
	for (i = 0; i < m_width; i ++) {
		m_DispGridX1[i].x = offsetX + i;
		m_DispGridX1[i].y = offsetY - m_AveGridX[i];
	}

	// update the new buffer, for Y
	offsetX = m_margin_edgeH/2 + m_margin_middle + m_width;
	for (i = 0; i < m_height; i ++) {
		m_DispGridY1[i].x = offsetX + i;
		m_DispGridY1[i].y = offsetY - m_AveGridY[i];
	}

	return TRUE;
}


void CCalDesinu::DrawCurves(CPaintDC *pDC) {
	RECT    rect;
	int     i;

	// rectanle for curve X
	rect.top    = m_margin_edgeV*3/2 - 1;
	rect.left   = m_margin_edgeH - 1;
	rect.bottom = rect.top + m_img_pix_bnd + 2;
	rect.right  = rect.left + m_width + 2;

	pDC->Rectangle(&rect);

	// rectanle for curve Y
	rect.top    = m_margin_edgeV*3/2-1;
	rect.left   = m_margin_edgeH/2 + m_margin_middle + m_width - 1;
	rect.bottom = rect.top + m_img_pix_bnd + 2;
	rect.right  = rect.left + m_height + 2;

	pDC->Rectangle(&rect);

	// erase background
	m_pPenOld = pDC->SelectObject(&m_penBkColor);
	pDC->Polyline(m_DispGridX0, m_width);
	pDC->Polyline(m_DispGridY0, m_height);
	pDC->SelectObject(m_pPenOld);

	// draw new curve X
	m_pPenOld = pDC->SelectObject(&m_penRed);
	pDC->Polyline(m_DispGridX1, m_width);
	pDC->SelectObject(m_pPenOld);
	// add "+" mark for local minima to new curve X
	for (i = 0; i < m_localPtsX; i ++) {
		DrawCross(pDC, m_localMinX[i], &m_penBlue);
	}

	// draw new curve Y
	m_pPenOld = pDC->SelectObject(&m_penBlue);
	pDC->Polyline(m_DispGridY1, m_height);
	pDC->SelectObject(m_pPenOld);
	// add "+" mark for local minima to new curve Y
	for (i = 0; i < m_localPtsY; i ++) {
		DrawCross(pDC, m_localMinY[i], &m_penRed);
	}
}


void CCalDesinu::RefreshFigures(BOOL b11, BOOL b12, BOOL b21, BOOL b22) {
	RECT   rect;

	// refresh curve X
	if (b11 == TRUE) {
		rect.top    = m_margin_edgeV*3/2 - 1;
		rect.left   = m_margin_edgeH - 1;
		rect.bottom = rect.top + m_img_pix_bnd + 2;
		rect.right  = rect.left + m_width + 2;
		InvalidateRect(&rect, TRUE);
	}

	// refresh curve Y
	if (b12 == TRUE) {
		rect.top    = m_margin_edgeV*3/2-1;
		rect.left   = m_margin_edgeH/2 + m_margin_middle + m_width - 1;
		rect.bottom = rect.top + m_img_pix_bnd + 2;
		rect.right  = rect.left + m_height + 2;
		InvalidateRect(&rect, TRUE);
	}

	// refresh raw image
	if (b21 == TRUE) {
		rect.left   = m_margin_edgeH;
		rect.top    = m_margin_edgeV*2+m_margin_middle+m_img_pix_bnd;
		rect.right  = rect.left + m_width;
		rect.bottom = rect.top + m_height;
		InvalidateRect(&rect, TRUE);
	}

	// refresh de-sinusoided image
	if (b22 == TRUE) {
		rect.left   = m_margin_edgeH/2 + m_margin_middle + m_width - 1;
		rect.top    = m_margin_edgeV*2+m_margin_middle+m_img_pix_bnd;
		rect.right  = rect.left + m_width;
		rect.bottom = rect.top + m_height;
		InvalidateRect(&rect, TRUE);
	}
}


void CCalDesinu::DrawCross(CPaintDC *pDC, POINT localMin, CPen *pen) {
	m_pPenOld = pDC->SelectObject(pen);
	pDC->MoveTo(localMin.x-5, localMin.y);
	pDC->LineTo(localMin.x+5, localMin.y);
	pDC->MoveTo(localMin.x, localMin.y-5);  
	pDC->LineTo(localMin.x, localMin.y+5);
	pDC->SelectObject(m_pPenOld);
}


BOOL CCalDesinu::VerifyInputs()
{
	char   *text;
	int     ret_code;

	text  = new char [16];

	GetDlgItemText(IDC_EDIT_FREQ_HORZ, text, 16);
	m_nFreqHorz = atoi(text);
	if (m_nFreqHorz <= 0) {
		MessageBox("Invalid horizontal (raster) frequency", "Desinusoiding", MB_ICONWARNING);
		return FALSE;
	} else if (m_nFreqHorz > 50000000 || m_nFreqHorz < 5000000) {
		ret_code = MessageBox("This horizontal raster frequency is not reasonable, are you sure to continue?", "Desinusoiding", MB_ICONWARNING | MB_YESNO);
		if (ret_code == IDOK) 
			return TRUE;
		else 
			return FALSE;
	}

	GetDlgItemText(IDC_EDIT_FREQ_VERT, text, 16);
	m_nFreqVert = atof(text);
	if (m_nFreqVert <= 0) {
		MessageBox("Invalid vertical (raster) frequency", "Desinusoiding", MB_ICONWARNING);
		return FALSE;
	} else if (m_nFreqVert > 50000 || m_nFreqVert < 5000) {
		ret_code = MessageBox("This vertical raster frequency is not reasonable, are you sure to continue?", "Desinusoiding", MB_ICONWARNING | MB_YESNO);
		if (ret_code == IDOK) 
			return TRUE;
		else 
			return FALSE;
	}

	GetDlgItemText(IDC_EDIT_GRID_SIZE, text, 16);
	m_nGridSize = atof(text);
	if (m_nGridSize <= 0) {
		MessageBox("Invalid grid size", "Desinusoiding", MB_ICONWARNING);
		return FALSE;
	} else if (m_nGridSize > 1 || m_nGridSize < 0.01) {
		ret_code = MessageBox("This grid size is not reasonable, are you sure to continue?", "Desinusoiding", MB_ICONWARNING | MB_YESNO);
		if (ret_code == IDOK) 
			return TRUE;
		else 
			return FALSE;
	}

	GetDlgItemText(IDC_EDIT_GRID_NUMB, text, 16);
	m_nGridNumb = atof(text);
	if (m_nGridNumb <= 0) {
		MessageBox("Invalid grid number", "Desinusoiding", MB_ICONWARNING);
		return FALSE;
	} else if (m_nGridNumb > 50 || m_nGridNumb < 3) {
		ret_code = MessageBox("This grid number is not reasonable, are you sure to continue?", "Desinusoiding", MB_ICONWARNING | MB_YESNO);
		if (ret_code == IDOK) 
			return TRUE;
		else 
			return FALSE;
	}

	return TRUE;
}


// We need to fit the discete points nonlinearly to obtain the two unknown 
// parameters for the equation
//     dS = A*Omega*sin(Omega*(T_0+dT_i))dt_i
//
BOOL CCalDesinu::BuildLUT(int *UnMatrixIdx, float *UnMatrixVal) {
	double   A0, T0, pi, Omega, HGridPoints, lambda;
	long     FreqH, FreqV;
	double  *delta_grid, *Omega_delta_t, *Omega_delta_T, *delta_S;
	int      i, j, offsetX, offsetY, IterNum;
	double   dy, J0, J1, ya1, ya2, a11, a12, a21, a22, b1, b2, da1, da2;
	int      N0, N00, idx1, idx2;
	double  *TimeN, *DistN, *DistE, delta_s, di, dx, a, b, k;


	offsetX = m_margin_edgeH;

	pi = asin(1.0)*2;

	A0       = 0.5;         // initial guess, in degree
	T0       = pi/4;        // initial guess
	FreqH   = m_nFreqHorz;  // sampling frequency of the horizontal raster
	FreqV   = m_nFreqVert;  // sampling frequency of the vertical raster

	Omega   = 2*pi*FreqH;   // angular velocity
	HGridPoints = FreqH/(FreqV*2.0);  // total sampling points along each horizontal line

	delta_S       = new double [m_localPtsX-1];
	delta_grid    = new double [m_localPtsX-1];
	Omega_delta_t = new double [m_localPtsX-1];
	Omega_delta_T = new double [m_localPtsX-1];

	for (i = 0; i < m_localPtsX-1; i ++) {
		delta_grid[i] = m_localMinX[i+1].x - m_localMinX[i].x;
		Omega_delta_t[i] = (delta_grid[i]/HGridPoints) * pi;

		if (i == 0) {
			Omega_delta_T[0] = Omega_delta_t[0]/2;
		} else {
			Omega_delta_T[i] = 0;
			for (j = 0; j < i; j ++)
				Omega_delta_T[i] += Omega_delta_t[j];
			Omega_delta_T[i] += Omega_delta_t[i]/2;
		}

		delta_S[i] = m_nGridSize;  // in degree;
	}
	
	// run several iterations
	// the nonlinear least square below is based on the Levenberg-Marquardt Method
	IterNum = 500;
	lambda  = 0.001;

	for (i = 0; i < IterNum; i ++) {
		// calculate J^2
		J0 = 0;
		a11 = a12 = a21 = a22 = 0;
		b1 = b2 = 0;
		for (j = 0; j < m_localPtsX-1; j ++) {
			dy  = delta_S[j] - A0*(Omega_delta_t[j] * sin(T0+Omega_delta_T[j]));
			J0 += dy * dy;

			ya1  = Omega_delta_t[j] * sin(T0+Omega_delta_T[j]);
			ya2  = A0 * (Omega_delta_t[j] * cos(T0+Omega_delta_T[j]));
			a11 += ya1 * ya1;
			a22 += ya2 * ya2;
			a12 += ya1 * ya2;

			b1  += dy * ya1;
			b2  += dy * ya2;
		}
		a21 = a12;
		a11 = a11 * (1 + lambda);
		a22 = a22 * (1 + lambda);

		da1 = (a22*b1-a12*b2)/(a11*a22-a12*a21); 
		da2 = (a11*b2-a21*b1)/(a11*a22-a12*a21); 

		A0 = A0 + da1;
		T0 = T0 + da2;
    
		J1 = 0;
		for (j = 0; j < m_localPtsX-1; j ++) {
			dy  = delta_S[j] - A0*(Omega_delta_t[j] * sin(T0+Omega_delta_T[j]));
			J1 += dy * dy;
		}
    
		if (fabs(J1-J0)/fabs(J0) < 1e-9) break;
    
		if (J1 >= J0) {
			lambda = lambda*10;
			A0 = A0 + da1;
			T0 = T0 + da2;
		} else {
			lambda = lambda/10;
		}
		
	}
	
	delete [] delta_grid;
	delete [] Omega_delta_t;
	delete [] Omega_delta_T;
	delete [] delta_S;

	// build a lookup table based on linear interpolation
	TimeN  = new double [m_width];
	DistN  = new double [m_width];
	DistE  = new double [m_width];

	N0 = (int)(HGridPoints*T0/pi+0.5);
	N00 = N0 - m_localMinX[0].x + offsetX;

	for (i = 0; i < m_width; i ++) {
		TimeN[i]  = (N00 + i) * pi / HGridPoints;
		DistN[i]  = A0 - A0*cos(TimeN[i]);
	}

	delta_s = (DistN[m_width-1]-DistN[0]) / (m_width-1);
	for (i = 0; i < m_width; i ++) {
		DistE[i] = DistN[0] + i*delta_s;
	}

	for (i = 0; i < m_width; i ++) {
		di = DistE[i];
		for (j = 0; j < m_width; j ++) 
			if (DistN[j] >= di) break;

		idx2 = j;
		if (idx2 == 0) {
			UnMatrixIdx[i] = idx2;
			UnMatrixVal[i] = 1;
			UnMatrixIdx[m_width+i] = 0;
			UnMatrixVal[m_width+i] = 0;
		} else if (idx2 == m_width-1) {
			UnMatrixIdx[i] = 0;
			UnMatrixVal[i] = 0;
			UnMatrixIdx[m_width+i] = idx2;
			UnMatrixVal[m_width+i] = 1;
		} else {
			idx1 = idx2 - 1;
			dx = DistN[idx2] - DistN[idx1];
			a = (DistN[idx2] - DistE[i]) / dx;
			b = (DistE[i] - DistN[idx1]) / dx;
			UnMatrixIdx[i] = idx1;
			UnMatrixVal[i] = (float)a;
			UnMatrixIdx[m_width+i] = idx2;
			UnMatrixVal[m_width+i] = (float)b;
		}
	}

	delete [] TimeN;
	delete [] DistN;
	delete [] DistE;

	// -------------------------------
	// 
	// handle vertical direction, linear fitting
	//
	// -------------------------------
	offsetY = m_margin_edgeH/2 + m_margin_middle + m_width;
	delta_grid    = new double [m_localPtsY-1];
	delta_S       = new double [m_localPtsY-1];
	a = m_localMinY[m_localPtsY-1].x - m_localMinY[0].x;
	a11 = a12 =	a22 = b1 = b2 = 0;
	for (i = 0; i < m_localPtsY-1; i ++) {
		delta_grid[i] = (m_localMinY[i+1].x - m_localMinY[i].x)*(m_localPtsY-1)/a;
		delta_S[i] = (m_localMinY[i+1].x-offsetY + m_localMinY[i].x-offsetY)/2;
		a11 += delta_S[i]*delta_S[i];
		a12 += delta_S[i];
		b1 += delta_S[i]*delta_grid[i];
		b2 += delta_grid[i];
	}
	a21 = a12;
	a22 = m_localPtsY-1;
	k = (a22*b1-a12*b2) / (a11*a22-a12*a21);
	b = (a11*b2-a21*b1) / (a11*a22-a12*a21);
	delete [] delta_grid;
	delete [] delta_S;
	
	//DistN  = new double [m_height];
	UnMatrixIdx[m_width*2] = 0;
	UnMatrixVal[m_width*2] = 1;
	UnMatrixIdx[m_width*2+m_height] = 1;
	UnMatrixVal[m_width*2+m_height] = 0;
	for (i = 1; i < m_height-1; i ++) {
		di = k*i + b;
		if (di < 1.0) {
			UnMatrixIdx[m_width*2+i] = i-1;
			UnMatrixVal[m_width*2+i] = 1-di;
			UnMatrixIdx[m_width*2+i+m_height] = i;
			UnMatrixVal[m_width*2+i+m_height] = di;
		} else if (di > 1.0){
			UnMatrixIdx[m_width*2+i] = i;
			UnMatrixVal[m_width*2+i] = di;
			UnMatrixIdx[m_width*2+i+m_height] = i+1;
			UnMatrixVal[m_width*2+i+m_height] = di-1;
		} else {
			UnMatrixIdx[m_width*2+i] = i;
			UnMatrixVal[m_width*2+i] = di;
			UnMatrixIdx[m_width*2+i+m_height] = 0;
			UnMatrixVal[m_width*2+i+m_height] = 0;
		}
	}
	UnMatrixIdx[m_width*2+m_height-1] = m_height-2;
	UnMatrixVal[m_width*2+m_height-1] = 0;
	UnMatrixIdx[m_width*2+m_height+m_height-1] = m_height-1;
	UnMatrixVal[m_width*2+m_height+m_height-1] = 1;

	return TRUE;
}



// unwarp the raw image from the parameters on the lookup tables
// linear interpolation
BOOL CCalDesinu::UnwarpImage() {
	int      i, j, m, n, p, q, idx1, idx2;
	float    val1, val2;

	for (j = 0; j < m_height; j ++) {
		m = j * m_width;
		for (i = 0; i < m_width; i ++) {
			n    = m + i;
			idx1 = m_UnMatrixIdx[i];
			idx2 = m_UnMatrixIdx[i+m_width];
			val1 = m_UnMatrixVal[i];
			val2 = m_UnMatrixVal[i+m_width];
			p    = m + idx1;
			q    = m + idx2;
			m_dispBufferDew[n] = (BYTE)(m_dispBufferRaw[p]*val1 + m_dispBufferRaw[q]*val2);
		}
	}

	return TRUE;
}


