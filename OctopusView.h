/////////////////////////////////////////////////////////////////////////////
// OctopusView.h
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_OctopusView)
#define AFX_H_OctopusView

#include "stdafx.h"
#include "OctopusDoc.h"

class COctopusView : public CFormView {

protected:	
	
	COctopusView();
	DECLARE_DYNCREATE(COctopusView)

public:

	enum { IDD = IDC_BACKGROUND };
	virtual ~COctopusView();

protected:

	afx_msg void OnOpenAndor();
	afx_msg void OnOpenMotor();
	afx_msg void OnOpenMPC200();

	CTime systemtime;

	virtual void OnInitialUpdate();
	virtual void DoDataExchange(CDataExchange* pDX);

	u16 scope;

	DECLARE_MESSAGE_MAP()

	CString debug;

	
};

#endif
