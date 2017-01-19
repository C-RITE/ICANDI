#if !defined(AFX_VIEWRAWVIDEO_H__BEE6C3AD_9F0F_466D_B3B4_3CD27735C8A5__INCLUDED_)
#define AFX_VIEWRAWVIDEO_H__BEE6C3AD_9F0F_466D_B3B4_3CD27735C8A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// ViewRawVideo.h : header file
//
#include "ICANDIDoc.h"
#define START_STEP	25
/////////////////////////////////////////////////////////////////////////////
// CViewRawVideo view

class CViewRawVideo : public CView
{
private:
	CPen m_PenGreen;
	CPen m_PenYellow;
	HDC  m_hdc;
	LPBITMAPINFO  m_BitmapInfoWarp;
	BOOL m_bMessageArrive;
//	BOOL m_bSaveVideo;
	BOOL m_bUpdateBackground;
	BOOL m_bTCABoxOn;
	long m_msgID;

	CRichEditCtrl	m_msgTimeElapse;
	CRichEditCtrl	m_msgStimulusSt;
	CRichEditCtrl	m_msgTCASt;
	BOOL			m_bLabelCreated;
	RECT			m_ClientRect;	
	POINT           m_TCAcenter;
	CStatic m_lblBrightness;
	CStatic m_lblContrast;
	CScrollBar	m_sldBrightness;
	CScrollBar m_sldContrast;

	clock_t        m_start;
	clock_t        m_finish;
	CByteArray     m_array;
	BYTE		   *m_tempVideoIn;
	BYTE		   *m_tempVideoOut;
	int            m_MemPoolID;	
	BYTE	m_regAddr;           
	UINT	m_regVal;

protected:
	CViewRawVideo();           // protected constructor used by dynamic creation
	DECLARE_DYNCREATE(CViewRawVideo)

// Attributes


public:

// Operations
public:
	int GetSinusoidX(int xc_i, BYTE dx, int *xs_o, int *dx_o);
	void DrawStimulusPoint(CDC *pDC, int dx, int dy);
	LPBITMAPINFO CreateDIB(int cx, int cy);
	void DrawMovie(CDC *pDC);
	void CreateAnimateCtrl();
	void UpdateBrightnessContrast(BOOL brightness);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CViewRawVideo)
	public:
	virtual void OnInitialUpdate();
	protected:
	virtual void OnDraw(CDC* pDC);      // overridden to draw this view
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
protected:
	virtual ~CViewRawVideo();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCalDesinu)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL
	// Generated message map functions
protected:
	//{{AFX_MSG(CViewRawVideo)
	afx_msg LRESULT	OnSendMovie(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIEWRAWVIDEO_H__BEE6C3AD_9F0F_466D_B3B4_3CD27735C8A5__INCLUDED_)
