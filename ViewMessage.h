#if !defined(AFX_VIEWMESSAGE_H__A2FE95A5_93A9_4E2C_B2F2_0717BD40A8D6__INCLUDED_)
#define AFX_VIEWMESSAGE_H__A2FE95A5_93A9_4E2C_B2F2_0717BD40A8D6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewMessage.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CViewMessage view

class CViewMessage : public CView
{
protected:
	CViewMessage();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CViewMessage)

// Attributes
public:

// Operations
public:
	void CalcHistogram(BYTE *mean_val, int *sd, int *sat);
	void DrawGraphFrame(CPaintDC *pDC);
	void DrawGraphLegends(CPaintDC *pDC);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewMessage)
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CViewMessage();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	// Generated message map functions
protected:
	int CopyDeltas2Coords();
	//{{AFX_MSG(CViewMessage)
	afx_msg void OnPaint();
	afx_msg LRESULT	OnSendMovie(WPARAM wParam, LPARAM lParam);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void UpdateHistogram();
	afx_msg void UpdateStabilization();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	long   m_msgID;
	UINT   m_FramesNumber;
	BOOL   m_bShowDeltas;
	BOOL   m_bMessageArrive;
	BOOL   m_bWindowCreated;
	CPen   m_PenGreen;
	CPen   m_PenGray;
	CPen   m_PenGreenD;
	CPen   m_PenGrayD;
	CPen   m_PenRed;
	CPen   m_PenBlue;
	CPen   m_PenWhite;
	CPen   m_PenBlueThick;
	CPen   m_PenWhiteThick;
	CPen  *m_PenOld;
	CFont  m_FontGraph;
	CFont  m_FontMeanVal;
	CFont *m_FontOld;

	POINT *m_ptsDeltaX0;
	POINT *m_ptsDeltaY0;
	POINT *m_ptsDeltaX1;
	POINT *m_ptsDeltaY1;
	POINT *m_ptsDeltas;
	POINT *m_HistCordOld;
	POINT *m_HistCordNew;

	UINT   m_nCount0;
	UINT   m_nCount1;
	SIZE   m_sizeValidRect;

	BOOL   m_initialized;
	int    m_offsetY;
	int    m_ScalingFactor;

	CButton  m_histCurve;
	CButton  m_stabCurve;
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWMESSAGE_H__A2FE95A5_93A9_4E2C_B2F2_0717BD40A8D6__INCLUDED_)
