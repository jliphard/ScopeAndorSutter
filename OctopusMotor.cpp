#include "stdafx.h"
#include "Octopus.h"
#include "OctopusMotor.h"
#include "OctopusLog.h"

extern COctopusLog* glob_m_pLog;

COctopusMotor::COctopusMotor(CWnd* pParent)
	: CDialog(COctopusMotor::IDD, pParent)
{    
	if( Create(COctopusMotor::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

	current_mm                  =  0.0;
	stepsize_mm                 =  1.0;
	min_mm                      =  0.0;
	max_mm                      = 12.0;
	middle_mm                   =  6.0;
	save_mm				        =  6.0;

	first_tick                  = true;

	SetTimer( TIMER_MOTOR, 100, NULL );
}

BOOL COctopusMotor::OnInitDialog()
{
     CDialog::OnInitDialog();
     SetWindowPos(NULL, 589, 57, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
     return TRUE;
}

COctopusMotor::~COctopusMotor() {}

void COctopusMotor::Initialize( void ) 
{
	m_FocusMotor.StartCtrl();
	AfxMessageBox(_T("!!!!WARNING - THE ACTUATOR IS ABOUT TO INITIALIZE - MAKE SURE THE TIP IS PROTECTED!!!!\n!!!!WARNING - THE ACTUATOR IS ABOUT TO INITIALIZE - MAKE SURE THE TIP IS PROTECTED!!!!\nInitializaton will start once you click OK."));
	m_FocusMotor.MoveHome(0, true);
}

void COctopusMotor::Close( void ) 
{ 
    KillTimer( TIMER_MOTOR );
	Sleep(500);
	m_FocusMotor.StopCtrl();
	WriteLogString(_T(" Focus motor Close() "));
}

void COctopusMotor::MoveTo( double dist_mm )
{
	double new_target_mm = current_mm + dist_mm;

	if( new_target_mm > min_mm && new_target_mm < max_mm ) 
	{
		current_mm = new_target_mm;
		m_FocusMotor.MoveAbsoluteEx(0, current_mm, 0.0, true);
	}
}

void COctopusMotor::MoveRel( double dist_mm )
{
	double new_target_mm = current_mm + dist_mm;

	if( new_target_mm > min_mm && new_target_mm < max_mm ) 
	{
		current_mm = new_target_mm;
		m_FocusMotor.MoveRelativeEx(0, dist_mm, 0.0, true);
	}
}

void COctopusMotor::MoveUp(   void ) { MoveRel( +stepsize_mm ); }
void COctopusMotor::MoveDown( void ) { MoveRel( -stepsize_mm ); }

void COctopusMotor::OnStepSize1() { m_Radio_S = 0; stepsize_mm = 0.1; UpdateData (false); };
void COctopusMotor::OnStepSize2() { m_Radio_S = 1; stepsize_mm = 1.0; UpdateData (false); };

double COctopusMotor::GetPosition( void ) 
{ 
	return current_mm;
}

void COctopusMotor::ShowPosition( void )
{
	CString str;

	if( IsWindowVisible() ) 
	{
	    str.Format(_T("Current: %.3f\nSave: %.3f"), current_mm, save_mm);
		m_Pos.SetWindowText( str );
	}
}

void COctopusMotor::Center( void ) 
{	
	current_mm = middle_mm;
	m_FocusMotor.MoveAbsoluteEx(0, current_mm, 0.0, true);
}

void COctopusMotor::OnSave( void ) 
{	
	save_mm = current_mm;
	WriteLogString(_T(" COctopusMotor::OnSave( void ) "));
}

void COctopusMotor::OnSaveGoTo( void ) 
{	
	current_mm = save_mm;
	m_FocusMotor.MoveAbsoluteEx(0, current_mm, 0.0, true);
}

void COctopusMotor::OnTimer( UINT_PTR nIDEvent ) 
{

	if( nIDEvent == TIMER_MOTOR ) 
	{
		if( first_tick ) 
		{
			OnStepSize1();
			first_tick = false;
			Initialize(); 
			Center();
			WriteLogString(_T(" Focus motor initialized "));
			first_tick = false;
		}
		else
		{
			ShowPosition();
		}
	}

	CDialog::OnTimer(nIDEvent);
}

void COctopusMotor::DoDataExchange(CDataExchange* pDX) 
{
	CDialog::DoDataExchange( pDX );
	DDX_Radio(pDX,	 IDC_MOTOR_SSIZE_1,	m_Radio_S);
	DDX_Control(pDX, IDC_MOTOR_POS,	    m_Pos);
	DDX_Control(pDX, IDC_MGMOTORCTRL1,  m_FocusMotor);
}  

BEGIN_MESSAGE_MAP(COctopusMotor, CDialog)
	ON_BN_CLICKED(IDC_MOTOR_UP,	         MoveUp)
	ON_BN_CLICKED(IDC_MOTOR_DOWN,        MoveDown)
	ON_BN_CLICKED(IDC_MOTOR_SAVE,	     OnSave)
	ON_BN_CLICKED(IDC_MOTOR_SAVE_GOTO,   OnSaveGoTo)
	ON_BN_CLICKED(IDC_MOTOR_CENTER,		 Center)
	ON_BN_CLICKED(IDC_MOTOR_SSIZE_1,     OnStepSize1)
	ON_BN_CLICKED(IDC_MOTOR_SSIZE_2,     OnStepSize2)
	ON_WM_TIMER()
END_MESSAGE_MAP()

void COctopusMotor::OnKillfocusGeneral() { UpdateData( true ); }

BOOL COctopusMotor::OnCommand(WPARAM wParam, LPARAM lParam) 
{
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusMotor::WriteLogString( CString logentry )
{
	if ( glob_m_pLog != NULL ) glob_m_pLog->Write( logentry );
}