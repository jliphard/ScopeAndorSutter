#if !defined(AFX_H_COctopusMotor)
#define AFX_H_COctopusMotor

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "CDMG17Motor.h"

class COctopusMotor : public CDialog
{

public:
	
	COctopusMotor(CWnd* pParent = NULL);
	virtual ~COctopusMotor();

	enum { IDD = IDC_MOTOR };

	void MoveUp( void );
	void MoveDown( void );
	void MoveRel( double dist_mm );
	void MoveTo(  double dist_mm );

protected:

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnNMCustomdrawExecute( NMHDR* pNMHDR, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()

	CStatic m_Pos;
	
    bool first_tick;

	double save_mm; 
	double stepsize_mm;
	double middle_mm;
	double max_mm;
	double min_mm;
	double current_mm;

	int	m_Radio_S;
	CStatic m_status;

	CString debug;
	CString result;

	void   ShowPosition( void );
	double GetPosition( void );

	void WriteLogString( CString logentry );

public:

	afx_msg void OnStepSize1();
	afx_msg void OnStepSize2();

	afx_msg void OnSave();
	afx_msg void OnSaveGoTo();

	virtual BOOL OnInitDialog();

	afx_msg void OnKillfocusGeneral();
	//afx_msg void OnTimer(UINT nIDEvent);

	afx_msg void OnTimer(UINT_PTR nIDEvent);
	
	void Initialize( void );
	void Close( void );
	afx_msg void Center();

public:

	CDMG17Motor m_FocusMotor;
	//CMgmotorctrl1 M_Motor;
};

#endif