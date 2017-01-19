#if !defined(AFX_VIEWDEWARPVIDEO_H__897D369F_4D00_412D_BB1F_217ADDFD9E4E__INCLUDED_)
#define AFX_VIEWDEWARPVIDEO_H__897D369F_4D00_412D_BB1F_217ADDFD9E4E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewDewarpVideo.h : header file
//

//#include "vfw.h"

/////////////////////////////////////////////////////////////////////////////
// CViewDewarpVideo view

class CViewDewarpVideo : public CView
{
private:
	CPen    *m_PenOld;
	CPen     m_PenGreen;
	CPen     m_PenRed;
	CPen     m_PenYellow;
	HDC      m_hdc;
	LPBITMAPINFO  m_BitmapInfoDewarp;

	RECT     m_ClientRect;
	BOOL     m_bClientCreated;
	BOOL     m_bUpdateBackground;
	BOOL     m_bMessageArrive;
	BOOL     m_bResetFlag;
	long     m_msgID;
	POINT    m_ptMousePos[20];
	POINT    m_stimulusPos;
	int      m_iMousePts;
	int      m_intDiameter;
	int      m_intStimNumb;
	
	
	POINT    m_intCaptionSize;
	CStatic  m_lblCaptionArea;
	CStatic  m_lblIlluminationTime;
	CStatic  m_lblCellSizeX;
	CStatic  m_lblCellSizeY;
	CStatic  m_lblStimNumbX;
	CStatic  m_lblStimNumbY;
	CStatic  m_lblFilling1;
	CStatic  m_lblFilling2;
	CStatic  m_lblFilling3;
	CListBox m_lstPositions;
	CButton  m_btnGo;

	CRichEditCtrl m_edtIlluminationTime;
	CRichEditCtrl m_edtCellSizeX;
	CRichEditCtrl m_edtCellSizeY;
	CRichEditCtrl m_edtStimNumbX;
	CRichEditCtrl m_edtStimNumbY;

	CByteArray     m_array;
	BYTE		   *m_tempVideoOut;

	// these parameter are used to do automatic cone searching
	int      m_coneRadius;
	POINT    m_offset;
	BOOL     m_IsMouseDown;
	int      m_nConeNumber;
	BOOL	 m_bUpdateStimLoc;
//	POINT    m_conePositions[30];
//	BYTE    *m_image1;
//	BYTE    *m_image2;
//	int     *m_image0;
	RECT     m_rect;
	int      m_nFrameID;
	BOOL     m_bPatchIsSelected;

protected:
	CViewDewarpVideo();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CViewDewarpVideo)

// Attributes
public:
	void DrawMovie(CDC *pDC);

// Operations
public:
	BOOL GenRandomPath();
	BOOL VerifyInputs();
	void UpdateStimulusPos(POINT pos);
	void FontFormat(CHARFORMAT &cf);
	void UpdateControls();
	BOOL SearchCentroidPos(POINT point, int *cx, int *cy);
	LPBITMAPINFO CreateDIB(int cx, int cy);
	void GetStimPosition(POINT *pt);
	void UpdateStimPos(CPoint point);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewDewarpVideo)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CViewDewarpVideo();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	//{{AFX_MSG(CViewDewarpVideo)
	afx_msg LRESULT	OnSendMovie(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg int  OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void RunRandomStim();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWDEWARPVIDEO_H__897D369F_4D00_412D_BB1F_217ADDFD9E4E__INCLUDED_)
