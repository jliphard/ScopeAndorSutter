#if !defined(AFX_H_OctopusMPC200)
#define AFX_H_OctopusMPC200

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "roe200.h"

class COctopusMPC200 : public CDialog
{

public:
	
	COctopusMPC200(CWnd* pParent = NULL);	// standard constructor
	~COctopusMPC200();

	enum { IDD = IDC_MPC200 };
	
	void    GetPosition( void );
	void	StepXYZ( void );
	
	long	m_nNewX;
	long	m_nNewY;
	long	m_nNewZ;
	
	long    m_nStepX;
	long    m_nStepY;
	long    m_nStepZ;

	long	m_nFB_Step;
	long	m_nFB_Speed;
	long	m_nFB_Cycles;
	double  m_nFB_Angle;

	CBitmap m_bmp_yes;
	CBitmap m_bmp_no;

	CStatic m_status_feedback;
	CStatic m_status_spreading;

	int m_AXIS;

	long	m_nFB_Fast_Cycles;
	long	m_nFB_Fast_Amplitude;
	long	m_nFB_Fast_Speed;
	long    m_nFB_Fast_Wait_ms;

	long	m_nFB_Slow_Cycles;
	long	m_nFB_Slow_Amplitude;
	long	m_nFB_Slow_Speed;
	long    m_nFB_Slow_Wait_ms;

	double  m_nCFFL_K;
	double  m_nCFFL_Target_force;
	double  m_nCFFL_Margin;
	long    m_nCFFL_Speed;

	int		m_nID;
	int		m_nSpeed;
	
	long   XCurr_Step;
	long   YCurr_Step;
	long   ZCurr_Step;

	double	m_nXCurr_Micron;
	double	m_nYCurr_Micron;
	double	m_nZCurr_Micron;

	CStatic	m_X_Now;
	CStatic	m_Y_Now;
	CStatic	m_Z_Now;

	CStatic m_VOLTAGE_Now;
	CStatic m_FORCE_Now;
	CStatic m_FORCE_AVG_Now;

	double	d_FORCE;
	double	d_FORCE_AVG;
	double	d_VOLTAGE;
	double	d_OFFSET;
	double  m_nFB_Probe_Sensitivity_uNperV;
	long    m_nFB_Lower_Mov_Limit;

	long m_SPREAD_SS;
	long m_SPREAD_DELAY;
	long m_SPREAD_CYCLES;
	long m_SPREAD_CYCLES_DONE;

	bool spreading;
	bool feedback;

	//long count;
	//double direct;
	//void StopThread( void );
	
	bool run_the_wait_thread;
	CListBox m_SeqList;

	void ExecuteNextCommand( void );
	void StopEverything( void );

protected:

	void ConstantForceFeedback( double K, double Target_Force, double Margin, int v, double d_FORCE_AVG );

	FILE * pFile;
	FILE * fFile;

	CTime systemtime;

	bool moving_x;

	double m_nStep_FB_microns;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKillfocusGeneral();
	virtual BOOL OnInitDialog();

	afx_msg void OnStop( void );
	afx_msg void OnGotoCenter( void );

	afx_msg void OnStepXYZ( void );

	afx_msg void OnMoveFwdBackFast( void );
	afx_msg void OnMoveFwdBackSlow( void );

	afx_msg void OnForceZero( void );

	afx_msg void OnFeedbackStart( void );
	afx_msg void OnFeedbackStop( void );

	afx_msg void  OnSpreadStart(void);
	afx_msg void  OnSpreadStop(void);

	void OnAxisX();
	void OnAxisZ();

	HANDLE WaitThreadMove;
	HANDLE wait_thread_cmd;

	ROE200* actuator;
	bool moving;

	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_H_OctopusMPC200)




	