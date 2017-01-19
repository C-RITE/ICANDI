// ICANDIView.h : interface of the CICANDIView class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_ICANDIVIEW_H__40CEB7E1_13FD_47AC_973E_1C6CBC9DECC8__INCLUDED_)
#define AFX_ICANDIVIEW_H__40CEB7E1_13FD_47AC_973E_1C6CBC9DECC8__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
#define  REDVLAMBDA	0.0169708
#define  IRVLAMBDA	0.0000003427
#define  GRVLAMBDA	1.0				// <---------double check this value to make sure it corresponds with the wavelength thats being used

#include "ICANDIDoc.h"

void _stdcall Out32(short PortAddress, short data);

class CICANDIView : public CView
{
protected: // create from serialization only
	CICANDIView();
	DECLARE_DYNCREATE(CICANDIView)

// Attributes
public:
	CICANDIDoc* GetDocument();

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CICANDIView)
	public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	//}}AFX_VIRTUAL

// Implementation
public:
	void FontFormat(CHARFORMAT& cf, int sel);
	void ShowTabItems(int tabIndex);
	void ApplyPupilMask();
	void ApplyTCA();
	void UpdateUserTCA();
	virtual ~CICANDIView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	void StablizationSuspend();

// Generated message map functions
protected:
	//{{AFX_MSG(CICANDIView)		
	afx_msg void UpdatePrefix(NMHDR* wParam, LRESULT *plr);
	afx_msg void UpdateMaxPower(NMHDR* wParam, LRESULT *plr);
	afx_msg void UpdateFieldSize(NMHDR* wParam, LRESULT *plr);
	afx_msg void UpdateTCAValues(NMHDR* wParam, LRESULT *plr);
	afx_msg void UpdateRedLaser();
	afx_msg void UpdateGreenLaser();
	afx_msg void UpdateIRLaser();
	afx_msg void UpdateDeliveryMode1();
	afx_msg void UpdateDeliveryMode2();
	afx_msg void UpdateDeliveryMode3();
	afx_msg void LoadStimulus();
	afx_msg void LoadMultiStim();
	afx_msg void MotionScalerChkY();
	afx_msg void MotionAngleChkY();
	afx_msg void StimulusVideo();
	afx_msg void UpdateGain0TrackingStatus();
	afx_msg void RewindVideo();
	afx_msg void StablizationGo();
	afx_msg void EnableSLR();
	afx_msg void UpdateOldRef();
	afx_msg void OnEnablePupilMask();
	afx_msg void ApplyOneFrameDelay();
	afx_msg void SwitchREDAOM();
	afx_msg void SwitchGreenAOM();
	afx_msg void SwitchIRAOM();
	afx_msg void CalibrateRed();
	afx_msg void SaveVideoCommand();
	afx_msg void EnableOCTSync();
	afx_msg void LoadRawVideoName();
	afx_msg void LoadDewarpedVideo();
	afx_msg void RawNameKillFocus();
	afx_msg void OnMeasureTCA();
	afx_msg void OnApplyTCA();
	afx_msg void OnTCAOverride();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg LRESULT	OnSendMovie(WPARAM wParam, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnSelchangeTabMain(NMHDR* pNMHDR, LRESULT* pResult);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:

	UINT m_ButtonDist;
	UINT m_ButtonHeight;
	UINT m_ButtonWidth;

	CRichEditCtrl m_processor;
	CButton m_btnGo;
	CButton m_chkSLR;
	CButton m_btnUpdateOldRef;
	CButton m_chkIRAOM;
	CButton m_chkRedAOM;
	CButton m_chkGrAOM;
	CButton m_chkPupilMask;
	CButton m_chkSyncOCT;
	CButton m_chkOneFrameDelay;
	CButton m_chkRedCal;
	CButton m_btnSaveVideo;
	CButton m_btnDewarpName;
	CButton m_redLaser;
	CButton m_grLaser;
	CButton m_irLaser;
	CButton m_btnStimulus;
	CButton m_btnMultiStim;
	CButton m_chkStimVideo;
	CButton m_chkStimProj;
	CButton m_chkRewindVideo;
	CButton m_btnDeliveryMode1;
	CButton m_btnDeliveryMode2;
	CButton m_btnDeliveryMode3;
	CButton m_chkVoltsPerDegY;
	CButton m_chkMotionAngleY;
	
	CScrollBar	m_sldLaserPow14;

	CStatic m_lasPowerControl;
	CStatic m_frameOne;
	CStatic m_fraVideoName;
	CStatic m_frameTwo;
	CStatic m_fraStimulusSetup;
	CStatic m_frameThree;
	CStatic m_lblStimGain;
	CStatic m_lblStimGainVal;	
	CStatic m_lblMaskGain;
	CStatic m_lblMaskGainVal;
	CStatic m_fraTCASetup;
	CStatic m_frameFour;
	CStatic m_lblTCARed;
	CStatic m_lblTCAGr;
	CStatic m_lblTCAX;
	CStatic m_lblTCAY;

	CButton m_TCAMeasure;
	CButton m_TCAApply;
	CButton m_TCAOverride;

	CRichEditCtrl   m_edtTCARedX;
	CRichEditCtrl   m_edtTCARedY;
	CRichEditCtrl   m_edtTCAGrX;
	CRichEditCtrl   m_edtTCAGrY;

	CRichEditCtrl m_edtFolder;
	CRichEditCtrl m_edtFilename;
	CRichEditCtrl m_edtDewarpName;

	// Laser Control GUI
	CScrollBar      m_sldLaserPower;
	CRichEditCtrl   m_edtLaserPower;
	CStatic			m_lblLaserPower;
	CStatic			m_lblLaserPowerTl;
	CStatic			m_lblLaserPowerPer;
	CRichEditCtrl   m_edtFieldSize;
	CStatic			m_lblFeildSize;

	CScrollBar      m_sldFlashFreq;
	CScrollBar      m_sldDutyCycle;
	CScrollBar      m_sldStimGainCtrl;
	CScrollBar      m_sldMaskGainCtrl;
	CScrollBar      m_sldVoltsPerDeg;			// volts/degree
	CScrollBar      m_sldMotionAngle;       	// Motion trace angle

	// GUI controls for setting up stimulus delivery
	CStatic         m_lblVoltsPerDeg;
	CStatic         m_lblMotionAngle;
	CStatic         m_lblFlashFreq;
	CStatic         m_lblFlashDuty;
	CStatic         m_lblVoltsPerDegValue;
	CStatic         m_lblMotionAngleValue;
	CStatic         m_lblFlashFrequency;
	CStatic         m_lblFlashDutyCycle;

	CRichEditCtrl   m_edtPrefix;
	CRichEditCtrl   m_edtVideoLen;
	CStatic         m_lblPrefix;
	CStatic         m_lblVideoLen;
	CStatic			m_lblVideoFolder;
	CStatic			m_lblVideoFileName;

	CButton			m_chkGain0Tracking; // 10/21/2011

	CTabCtrl        m_tabControl;

	BOOL            m_bStabCurves;
	BOOL            m_bStablizationOn;
	BOOL            m_bSaveDewarpImage;
	BOOL            m_bMessageArrive;
	long            m_msgID;
	CByteArray      m_array;
	int             m_StimFreq[15];
	int             m_FreqCount;
	BOOL			m_bCalib;

	double          fieldSize;

	clock_t         m_start;
	clock_t         m_finish;

	int             m_nRedPowerOld;
	int             m_nGreenPowerOld;
	int             m_nIRPowerOld;
	BOOL			m_bVoltsPerDegY;
	BOOL			m_bMotionAngleY;

	void UpdateOtherUnits();
	void createDirectory();	
	void UpdateLasersPowerStatus();
};

#ifndef _DEBUG  // debug version in ICANDIView.cpp
inline CICANDIDoc* CICANDIView::GetDocument()
   { return (CICANDIDoc*)m_pDocument; }
#endif

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_ICANDIVIEW_H__40CEB7E1_13FD_47AC_973E_1C6CBC9DECC8__INCLUDED_)
