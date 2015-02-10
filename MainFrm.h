/////////////////////////////////////////////////////////////////////////////
// MainFrm.h : interface of the CMainFrame class
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_MAINFRAME)
#define AFX_H_MAINFRAME

#include "stdafx.h"

class CMainFrame : public CFrameWnd
{

protected:
	CMainFrame();
	DECLARE_DYNCREATE(CMainFrame)

public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual ~CMainFrame();

};

#endif