#include "stdafx.h"
#include "Octopus.h"
#include "OctopusMPC200.h"
#include "OctopusGlobals.h"
#include "OctopusClock.h"
#include "OctopusCameraDlg.h"
#include "OctopusMotor.h"
#include "roe200.h"
#include "OctopusLog.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <direct.h>

extern COctopusGlobals	    B;
extern COctopusGoodClock*   glob_m_pGoodClock;
extern COctopusCamera*      glob_m_pCamera;
extern COctopusLog*         glob_m_pLog;
extern COctopusMotor*       glob_m_pMotor;
using namespace std;

DWORD WINAPI MPC200CommandBuffer( LPVOID lpParameter )
{
	COctopusMPC200* this_ = (COctopusMPC200*)lpParameter; 

	while ( this_ -> run_the_wait_thread )
	{
		this_ -> GetPosition();
		
		Sleep(300);
		
		if ( this_ -> run_the_wait_thread ) 
		{ 
			if( this_->m_SeqList.GetCount() > 0 )
			{
				this_->ExecuteNextCommand();
			}
		}
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////
// COctopusMPC200 dialog

COctopusMPC200::COctopusMPC200(CWnd* pParent)
	: CDialog(COctopusMPC200::IDD, pParent)
{

	actuator = NULL;
	actuator = new ROE200;
	
	moving_x = false;
	m_AXIS   = 1; // 0 = X

	m_nNewX   = 0;
	m_nNewY   = 0;
	m_nNewZ   = 0;

	m_nID     = 1;
	m_nSpeed  = 3;
	
	m_nXCurr_Micron = 0;
	m_nYCurr_Micron = 0;
	m_nZCurr_Micron = 0;

	   XCurr_Step   = 0;
	   YCurr_Step   = 0;
	   ZCurr_Step   = 0;

	m_nStepX  = 0;
	m_nStepY  = 0;
	m_nStepZ  = 0;
	
	m_nFB_Step    = 100;
	m_nFB_Speed   = 3;
	m_nFB_Cycles  = 1;
	m_nFB_Angle   = 32.0;

	m_nFB_Fast_Amplitude	=  100;
	m_nFB_Fast_Speed		=    0;
	m_nFB_Fast_Cycles		=    3;
	m_nFB_Fast_Wait_ms		= 5000;

	m_nFB_Slow_Amplitude	=  100;
	m_nFB_Slow_Speed		=   20;	
	m_nFB_Slow_Cycles		=    3;
	m_nFB_Slow_Wait_ms		= 5000;

	m_nCFFL_K				=  0.1;
	m_nCFFL_Target_force	=  100;
	m_nCFFL_Margin			=   10;
	m_nCFFL_Speed			=    1;
	
	m_SPREAD_SS          =  3;
	m_SPREAD_DELAY       = 60; /*step every 60 sec*/
	m_SPREAD_CYCLES      =  3;
	m_SPREAD_CYCLES_DONE =  0;
	spreading            =  false;

	d_FORCE   = 0.0;
	d_VOLTAGE = 0.0;
	d_OFFSET  = 0.0;

	m_nFB_Probe_Sensitivity_uNperV = 50.7;
	m_nFB_Lower_Mov_Limit          = 22000;

	VERIFY(m_bmp_no.LoadBitmap(IDB_NO));
	VERIFY(m_bmp_yes.LoadBitmap(IDB_YES));

	feedback = false;
	m_nStep_FB_microns = 0.0;

	B.MPC200_loaded     = false;
	moving              = false;
	run_the_wait_thread = false;

	systemtime = CTime::GetCurrentTime();
    CString t  = systemtime.Format(_T("%H_%M_%S"));
	CString d  = systemtime.Format(_T("%m_%d_%y/"));
	std::string dd=(CT2CA(d));
	std::string tt=(CT2CA(t));
	string directory = "C:/Users/Lab/Documents/data/forceprobe/";
	directory.append(dd);
	const char * dir = directory.c_str();
	mkdir(dir);

	directory.append("logs/");
	const char * dir2 = directory.c_str();
	mkdir(dir2);

	std::string logout = directory;
	logout.append("logfile_");
	logout.append(tt);
	logout.append(".txt");
	const char *c = logout.c_str();
	pFile = fopen(c, "wt");

	if( Create(COctopusMPC200::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	SetTimer( TIMER_MPC, 100, NULL ); 
	
}

BOOL COctopusMPC200::OnInitDialog() 
{
	CDialog::OnInitDialog();

	if( actuator->Initialize() == 0 )
	{
		B.MPC200_loaded = true;
		wait_thread_cmd = NULL;
		run_the_wait_thread = true;
		DWORD wait_thread_cmd_id = NULL;
		m_SeqList.ResetContent();
		wait_thread_cmd = CreateThread( 0, 4096, MPC200CommandBuffer, (LPVOID)this, 0, &wait_thread_cmd_id );
	};

	return TRUE;
}

void COctopusMPC200::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);

	DDX_Control( pDX, IDC_POS_X, m_X_Now ); 
	DDX_Control( pDX, IDC_POS_Y, m_Y_Now ); 
	DDX_Control( pDX, IDC_POS_Z, m_Z_Now ); 

	DDX_Control( pDX, IDC_MPC_FF_VOLTAGE,   m_VOLTAGE_Now ); 
	DDX_Control( pDX, IDC_MPC_FF_FORCE,     m_FORCE_Now ); 
	DDX_Control( pDX, IDC_MPC_FF_FORCE_AVG, m_FORCE_AVG_Now ); 

	DDX_Text(pDX, IDC_MPC_RE_SPEED, m_nSpeed);
	DDV_MinMaxInt(pDX, m_nSpeed, 0, 15);

	DDX_Text(pDX, IDC_MPC_RE_dX, m_nStepX);
	DDV_MinMaxLong(pDX, m_nStepX, -2000, 2000);

	DDX_Text(pDX, IDC_MPC_RE_dY, m_nStepY);
	DDV_MinMaxLong(pDX, m_nStepY, -2000, 2000);

	DDX_Text(pDX, IDC_MPC_RE_dZ, m_nStepZ);
	DDV_MinMaxLong(pDX, m_nStepZ, -2000, 2000);
	
	DDX_Radio( pDX, IDC_MPC_X, m_AXIS );

	DDX_Text(pDX, IDC_MPC_FB_CYCLES_FAST, m_nFB_Fast_Cycles);
	DDV_MinMaxLong(pDX, m_nFB_Fast_Cycles, 1, 10);

	DDX_Text(pDX, IDC_MPC_FB_STEP_FAST, m_nFB_Fast_Amplitude);
	DDV_MinMaxLong(pDX, m_nFB_Fast_Amplitude, 0, 1000);

	DDX_Text(pDX, IDC_MPC_FB_SPEED_FAST, m_nFB_Fast_Speed);
	DDV_MinMaxLong(pDX, m_nFB_Fast_Speed, 0, 15);

	DDX_Text(pDX, IDC_MPC_FB_WAIT_FAST, m_nFB_Fast_Wait_ms);
	DDV_MinMaxLong(pDX, m_nFB_Fast_Wait_ms, 500, 10000);

	DDX_Text(pDX, IDC_MPC_FB_CYCLES_SLOW, m_nFB_Slow_Cycles);
	DDV_MinMaxLong(pDX, m_nFB_Slow_Cycles, 1, 10);

	DDX_Text(pDX, IDC_MPC_FB_STEP_SLOW, m_nFB_Slow_Amplitude);
	DDV_MinMaxLong(pDX, m_nFB_Slow_Amplitude, 0, 1000);

	DDX_Text(pDX, IDC_MPC_FB_SPEED_SLOW, m_nFB_Slow_Speed);
	DDV_MinMaxLong(pDX, m_nFB_Slow_Speed, 1, 20);

	DDX_Text(pDX, IDC_MPC_FB_WAIT_SLOW, m_nFB_Slow_Wait_ms);
	DDV_MinMaxLong(pDX, m_nFB_Slow_Wait_ms, 500, 10000);

	DDX_Text(pDX, IDC_MPC_CFFL_K, m_nCFFL_K);
	DDV_MinMaxDouble(pDX, m_nCFFL_K, 0, 1);
	
	DDX_Text(pDX, IDC_MPC_CFFL_TARGET_FORCE, m_nCFFL_Target_force);
	DDV_MinMaxDouble(pDX, m_nCFFL_Target_force, -100, 100);

	DDX_Text(pDX, IDC_MPC_CFFL_MARGIN, m_nCFFL_Margin);
	DDV_MinMaxDouble(pDX, m_nCFFL_Margin, 0, 100);

	DDX_Text(pDX, IDC_MPC_CFFL_PROBE_SENS, m_nFB_Probe_Sensitivity_uNperV);
	DDV_MinMaxDouble(pDX, m_nFB_Probe_Sensitivity_uNperV, 10.0, 1000.0);

	DDX_Text(pDX, IDC_MPC_CFFL_MOV_LIMIT, m_nFB_Lower_Mov_Limit);
	DDV_MinMaxLong(pDX, m_nFB_Lower_Mov_Limit, 10000, 25000);

	DDX_Text(pDX, IDC_MPC_CFFL_SPEED, m_nCFFL_Speed);
	DDV_MinMaxLong(pDX, m_nCFFL_Speed, 0, 15);

	DDX_Text(pDX, IDC_MPC_SPREAD_DELAY, m_SPREAD_DELAY);
	DDV_MinMaxLong(pDX, m_SPREAD_DELAY, 1, 200);

	DDX_Text(pDX, IDC_MPC_SPREAD_SS, m_SPREAD_SS);
	DDV_MinMaxLong(pDX, m_SPREAD_SS, 1, 100);

	DDX_Text(pDX, IDC_MPC_SPREAD_CYCLES, m_SPREAD_CYCLES);
	DDV_MinMaxLong(pDX, m_SPREAD_CYCLES, 1, 1000);
	
	DDX_Control(pDX, IDC_MPC_CFFL_ONOFF_BMP,    m_status_feedback);
	DDX_Control(pDX, IDC_MPC_SPREAD_ONOFF_BMP,  m_status_spreading);

	DDX_Control(pDX, IDC_MPC200_LIST_SEQUENCE, m_SeqList);

}

BEGIN_MESSAGE_MAP(COctopusMPC200, CDialog)
	//{{AFX_MSG_MAP(COctopusMPC200)
	ON_BN_CLICKED(	IDC_MPC_STOP,       OnStop)
	ON_BN_CLICKED(	IDC_MPC_GO_CENTER,  OnGotoCenter) 
	ON_BN_CLICKED(  IDC_MPC_GO_XYZ,     OnStepXYZ)
	
	ON_BN_CLICKED(  IDC_MPC_FB_GO_FAST,    OnMoveFwdBackFast) 
	ON_BN_CLICKED(  IDC_MPC_FB_GO_SLOW,    OnMoveFwdBackSlow)
	ON_BN_CLICKED(  IDC_MPC_FF_ZERO,       OnForceZero)

	ON_BN_CLICKED(  IDC_MPC_CFFL_START, OnFeedbackStart)
	ON_BN_CLICKED(  IDC_MPC_CFFL_STOP,  OnFeedbackStop)

	ON_BN_CLICKED(  IDC_MPC_SPREAD_START, OnSpreadStart)
	ON_BN_CLICKED(  IDC_MPC_SPREAD_STOP,  OnSpreadStop)

	ON_BN_CLICKED(IDC_MPC_X,			OnAxisX)
	ON_BN_CLICKED(IDC_MPC_Z,			OnAxisZ)

	ON_EN_KILLFOCUS(IDC_MPC_RE_SPEED,   OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_RE_dX,      OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_RE_dY,      OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_RE_dZ,      OnKillfocusGeneral)
	
	ON_EN_KILLFOCUS(IDC_MPC_FB_CYCLES_SLOW, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_FB_SPEED_SLOW,  OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_FB_STEP_SLOW,   OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_FB_WAIT_SLOW,   OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_MPC_FB_CYCLES_FAST, OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_FB_SPEED_FAST,  OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_FB_STEP_FAST,   OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_FB_WAIT_FAST,   OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_MPC_CFFL_K,				OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_CFFL_TARGET_FORCE,	OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_CFFL_MARGIN,		OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_CFFL_SPEED,			OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_CFFL_RUNTIME,		OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_CFFL_PROBE_SENS,	OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_CFFL_MOV_LIMIT,	    OnKillfocusGeneral)

	ON_EN_KILLFOCUS(IDC_MPC_SPREAD_DELAY,	OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_SPREAD_SS,		OnKillfocusGeneral)
	ON_EN_KILLFOCUS(IDC_MPC_SPREAD_CYCLES,	OnKillfocusGeneral)

	ON_WM_TIMER()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void COctopusMPC200::OnAxisX() { m_AXIS = 0; moving_x = true ; UpdateData( false );}
void COctopusMPC200::OnAxisZ() { m_AXIS = 1; moving_x = false; UpdateData( false );}

void COctopusMPC200::OnTimer( UINT_PTR nIDEvent ) 
{
	if( nIDEvent == TIMER_MPC ) 
	{
		CString str;
		
		/* update the values*/
		d_VOLTAGE   =  B.ADC_20Hz;
		d_FORCE     = (B.ADC_20Hz - d_OFFSET) * m_nFB_Probe_Sensitivity_uNperV;
		d_FORCE_AVG = (B.ADC_01Hz - d_OFFSET) * m_nFB_Probe_Sensitivity_uNperV;	
		
		/* show the values*/
		str.Format(_T("%0.4f"), d_VOLTAGE    ); m_VOLTAGE_Now.SetWindowText( str );
		str.Format(_T("%0.4f"), d_FORCE      ); m_FORCE_Now.SetWindowText( str );
		str.Format(_T("%0.4f"), d_FORCE_AVG  ); m_FORCE_AVG_Now.SetWindowText( str );

		str.Format(_T("X: %0.3f"), m_nXCurr_Micron); m_X_Now.SetWindowText( str );
		str.Format(_T("Y: %0.3f"), m_nYCurr_Micron); m_Y_Now.SetWindowText( str );
		str.Format(_T("Z: %0.3f"), m_nZCurr_Micron); m_Z_Now.SetWindowText( str );

		/* write to file */
		CString out;
		systemtime = CTime::GetCurrentTime();
		CString t  = systemtime.Format(_T(" %m/%d/%y %H:%M:%S "));
		out.Format(_T("T:%.3f "), (double)glob_m_pGoodClock->End());
		out.Append( t );
		t.Format(_T("MPC200 POS: %f %f %f "), m_nXCurr_Micron, m_nYCurr_Micron, m_nZCurr_Micron);
		out.Append( t );
		t.Format(_T("FORCE: %f %f %f "), d_VOLTAGE, d_FORCE, d_FORCE_AVG);
		out.Append( t );
		out.Append(_T("\n"));
		fprintf( pFile, out );	
		fflush( pFile );

		if ( feedback || spreading ) 
		{
			CString timeforce;
			timeforce.Format(_T("%.3f "), (double)glob_m_pGoodClock->End());
			t.Format(_T(" %f %f %f %f"), d_FORCE, d_FORCE_AVG, m_nXCurr_Micron, m_nZCurr_Micron);
			timeforce.Append(t);
			timeforce.Append(_T("\n"));
			fprintf(fFile, timeforce);
			fflush( fFile);
		}
	}

	if( nIDEvent == TIMER_MPC_FB ) 
	{
		/*feedback if needed*/
		if( feedback ) 
		{
			ConstantForceFeedback( m_nCFFL_K, m_nCFFL_Target_force, m_nCFFL_Margin, m_nCFFL_Speed, d_FORCE );
		}
	}

	if( nIDEvent == TIMER_MPC_SPREAD ) 
	{
		if ( glob_m_pCamera != NULL)
			glob_m_pCamera->TakePicture();

		m_SeqList.AddString("STEPXFB"); 

		if ( glob_m_pMotor != NULL)
			glob_m_pMotor->MoveRel( double(m_SPREAD_SS) * 0.001 );

		m_SPREAD_CYCLES_DONE++;
		
		if( m_SPREAD_CYCLES_DONE >= m_SPREAD_CYCLES ) 
			OnSpreadStop(); 
	}
	
	CDialog::OnTimer(nIDEvent);
}

void COctopusMPC200::ConstantForceFeedback( double K, double Target_Force, double Margin, int v, double d_FORCE_AVG )
{
	// The direction of feedback
	// PULL: target force should be less than zero
	// PUSH: target force greater than zero

	double error = d_FORCE_AVG - Target_Force;

	/*
	If the force is too HIGH, then the actuator 
	needs to move FORWARD, by INCREASING the position
	*/

	if ( moving_x && (m_nXCurr_Micron > (m_nFB_Lower_Mov_Limit - 10) ) )
	{
		//Yipes - we're close to the X emergency limit!!!!!!
		//get out of here!
		OnFeedbackStop();
		AfxMessageBox(_T("Emergency Feedback Stop!\nRan into stop!"));
		return;
	}
	else if (         m_nZCurr_Micron > (m_nFB_Lower_Mov_Limit - 10) )
	{
		//Yipes - we're close to the Z emergency limit!!!!!!
		//get out of here!
		OnFeedbackStop();
		AfxMessageBox(_T("Emergency Feedback Stop!\nRan into stop!"));
		return;
	}

	if ( abs(error) > Margin ) 
		{
			// Calculate compensation in Microns for x-direction
			if( error < 0 ) //force is TOO LOW
				//move actuator backward, by DECREASING the numerical position
				m_nStep_FB_microns = -1 * K * abs(error);
			else //force is too high
				//move actuator forward, by INCREASING the numerical position
				m_nStep_FB_microns = +1 * K * abs(error);
				//double dx = -K * error;
			
			if ( moving_x )
				m_SeqList.AddString("STEPXFB"); 
			else
				m_SeqList.AddString("STEPZFB"); 
			//cout << "every step"<< count << endl;
		}
}

void COctopusMPC200::OnFeedbackStart() 
{ 
	feedback = true;
	m_status_feedback.SetBitmap( m_bmp_yes );
	SetTimer( TIMER_MPC_FB, 1000, NULL ); 

	systemtime = CTime::GetCurrentTime();
    CString t  = systemtime.Format(_T("%H_%M_%S"));
	CString d  = systemtime.Format(_T("%m_%d_%y/"));
	std::string dd=(CT2CA(d));
	std::string tt=(CT2CA(t));
	
	string directory = "C:/Users/Lab/Documents/data/forceprobe/";
	directory.append(dd);
	const char *dir = directory.c_str();
	mkdir(dir);
	directory.append("feedbacklog/");
	const char *dir2 = directory.c_str();
	mkdir(dir2);

	std::string fblog = directory;
	fblog.append("feedbacklog_");
	fblog.append(tt);
	fblog.append(".txt");
	const char *c = fblog.c_str();
	fFile = fopen(c, "wt");
}

void COctopusMPC200::OnSpreadStart() 
{ 
	spreading = true;
	m_status_spreading.SetBitmap( m_bmp_yes );

	SetTimer( TIMER_MPC_SPREAD, m_SPREAD_DELAY*1000, NULL ); 
	m_SPREAD_CYCLES_DONE = 0;
	m_nStep_FB_microns   = -m_SPREAD_SS;

	systemtime = CTime::GetCurrentTime();
    CString t  = systemtime.Format(_T("%H_%M_%S"));
	CString d  = systemtime.Format(_T("%m_%d_%y/"));
	std::string dd=(CT2CA(d));
	std::string tt=(CT2CA(t));
	
	string directory = "C:/Users/Lab/Documents/data/forceprobe/";
	directory.append(dd);
	const char *dir = directory.c_str();
	mkdir(dir);
	directory.append("feedbacklog/");
	const char *dir2 = directory.c_str();
	mkdir(dir2);

	std::string fblog = directory;
	fblog.append("feedbacklog_");
	fblog.append(tt);
	fblog.append(".txt");
	const char *c = fblog.c_str();
	fFile = fopen(c, "wt");
}

void COctopusMPC200::OnSpreadStop() 
{ 
	spreading = false; 
	KillTimer( TIMER_MPC_SPREAD );
	m_status_spreading.SetBitmap( m_bmp_no );
}

void COctopusMPC200::OnFeedbackStop() 
{ 
	feedback = false; 
	KillTimer( TIMER_MPC_FB );
	m_status_feedback.SetBitmap( m_bmp_no );
}

/////////////////////////////////////////////////////////////////////////////

BOOL COctopusMPC200::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int id = LOWORD(wParam); // Notification code
	if( id == 2 ) return FALSE; // Trap ESC key
	if( id == 1 ) return FALSE; // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusMPC200::OnKillfocusGeneral() { UpdateData( true ); }

//***********************************************************************
//***********************************************************************
//***********************************************************************
//***********************************************************************
//***********************************************************************

void COctopusMPC200::ExecuteNextCommand( void )
{

	if (run_the_wait_thread == false) return;

	CString command;
	m_SeqList.GetText(0, command);
	
	if (command.Left(6) == "CENTER")
	{
		actuator->GotoCenter();
		m_SeqList.DeleteString(0);
	} 
	else if ( command.Left(16) == "XFASTMOVEFWDBACK") //good
	{
		actuator->MoveFwdBackFast_X( m_nFB_Fast_Amplitude, m_nFB_Fast_Cycles, m_nFB_Fast_Speed, m_nFB_Fast_Wait_ms );
		m_SeqList.DeleteString(0);
	}
	else if ( command.Left(16) == "ZFASTMOVEFWDBACK") //good
	{
		actuator->MoveFwdBackFast_Z( m_nFB_Fast_Amplitude, m_nFB_Fast_Cycles, m_nFB_Fast_Speed, m_nFB_Fast_Wait_ms );
		m_SeqList.DeleteString(0);
	}
	else if ( command.Left(16) == "XSLOWMOVEFWDBACK")
	{
		m_SeqList.DeleteString(0);
		actuator->MoveFwdBackSlow_X( m_nFB_Slow_Amplitude, m_nFB_Slow_Cycles, m_nFB_Slow_Speed, m_nFB_Slow_Wait_ms );
	}
	else if ( command.Left(16) == "ZSLOWMOVEFWDBACK")
	{
		m_SeqList.DeleteString(0);
		actuator->MoveFwdBackSlow_Z( m_nFB_Slow_Amplitude, m_nFB_Slow_Cycles, m_nFB_Slow_Speed, m_nFB_Slow_Wait_ms );
	}
	else if ( command.Left(7) == "STEPXYZ")
	{
		actuator->StepXYZ( m_nStepX, m_nStepY, m_nStepZ, m_nSpeed );
		m_SeqList.DeleteString(0);
	}
	else if ( command.Left(7) == "STEPXFB")
	{
		actuator->StepFB_X( m_nStep_FB_microns );
		m_SeqList.DeleteString(0);
	}
	else if ( command.Left(7) == "STEPZFB")
	{
		actuator->StepFB_Z( m_nStep_FB_microns );
		m_SeqList.DeleteString(0);
	}
	else if ( command.Left(7) == "STOP")
	{
		actuator->StopMovingNow(); 
		m_SeqList.DeleteString(0);
	}
}

//**********************************************************************************

void COctopusMPC200::StopEverything( void ) 
{
	KillTimer( TIMER_MPC );
	run_the_wait_thread = false;
	Sleep(100);
	WaitForSingleObject(wait_thread_cmd, 2000);
	Sleep(100);
}

COctopusMPC200::~COctopusMPC200() 
{
	if( actuator != NULL )
	{
		actuator->Close();
		delete actuator;
		actuator = NULL;
	}

	B.MPC200_loaded = false;
}

//****************************************************************************************

void COctopusMPC200::GetPosition( void ) 
{
	if ( run_the_wait_thread == false ) return;

	actuator->TryUpdatePosition();
	
	m_nXCurr_Micron = actuator->GetX();
	m_nYCurr_Micron = actuator->GetY();
	m_nZCurr_Micron = actuator->GetZ();

	B.MPC_m_nXCurr_Micron = m_nXCurr_Micron;
	B.MPC_m_nYCurr_Micron = m_nYCurr_Micron;
	B.MPC_m_nZCurr_Micron = m_nZCurr_Micron;
}

void COctopusMPC200::OnGotoCenter( void )     { m_SeqList.AddString("CENTER");         }

void COctopusMPC200::OnMoveFwdBackFast( void ) 
{ 
	if( moving_x )
		m_SeqList.AddString("XFASTMOVEFWDBACK"); 
	else
		m_SeqList.AddString("ZFASTMOVEFWDBACK"); 
}

void COctopusMPC200::OnMoveFwdBackSlow( void ) 
{
	if( moving_x )
		m_SeqList.AddString("XSLOWMOVEFWDBACK"); 
	else
		m_SeqList.AddString("ZSLOWMOVEFWDBACK"); 
}

void COctopusMPC200::OnStepXYZ( void )        { m_SeqList.AddString("STEPXYZ");        }
void COctopusMPC200::OnStop( void )           { m_SeqList.AddString("STOP");           }
void COctopusMPC200::OnForceZero( void )      { d_OFFSET = B.ADC_20Hz; }
