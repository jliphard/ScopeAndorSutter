
#include "stdafx.h"
#include "ftd2xx.h"
#include "Octopus.h"
#include "OctopusDoc.h"
#include "OctopusView.h"
#include "OctopusGlobals.h"
#include "OctopusShutter.h"

extern COctopusGlobals B;

COctopusShutterAndWheel::COctopusShutterAndWheel(CWnd* pParent)
	: CDialog(COctopusShutterAndWheel::IDD, pParent)
{    

	USB_ready               =  false;
	m_ShutterOpen			=  2; // 2 means that the shutter is closed
	bus_busy				=  0;
	board_present			=  0;
	B.filter_wheel          =  0;
	ND_value                =  0;

	if( Create(COctopusShutterAndWheel::IDD, pParent) ) 
		ShowWindow( SW_SHOW );

}

void COctopusShutterAndWheel::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX,	IDC_SHUTTER_SLIDER,				m_Slider);
	DDX_Control(pDX,	IDC_SHUTTER_SLIDER_SETTING,		m_Slider_Setting);
	DDX_Radio(pDX,		IDC_RADIO_FILTER_0,				m_Filter);
}  

BEGIN_MESSAGE_MAP(COctopusShutterAndWheel, CDialog)
	ON_BN_CLICKED(IDC_RADIO_FILTER_0,			OnRadioFilter0)
	ON_BN_CLICKED(IDC_RADIO_FILTER_1,			OnRadioFilter1)
	ON_BN_CLICKED(IDC_RADIO_FILTER_2,			OnRadioFilter2)
	ON_BN_CLICKED(IDC_RADIO_FILTER_3,			OnRadioFilter3)
	ON_BN_CLICKED(IDC_RADIO_FILTER_4,			OnRadioFilter4)
	ON_BN_CLICKED(IDC_RADIO_FILTER_5,			OnRadioFilter5)
	ON_BN_CLICKED(IDC_RADIO_FILTER_6,			OnRadioFilter6)
	ON_BN_CLICKED(IDC_RADIO_FILTER_7,			OnRadioFilter7)
	ON_BN_CLICKED(IDC_RADIO_FILTER_8,			OnRadioFilter8)
	ON_BN_CLICKED(IDC_RADIO_FILTER_9,			OnRadioFilter9)
	ON_WM_TIMER()
	ON_NOTIFY(NM_RELEASEDCAPTURE, IDC_SHUTTER_SLIDER, OnNMCustomdrawExecute)
END_MESSAGE_MAP()

BOOL COctopusShutterAndWheel::OnInitDialog() {

	CDialog::OnInitDialog();

	m_nTimer = SetTimer(TIMER_LAMBDA, 200, NULL); // check for new USB data in the buffer every 10 ms
	
	Loadem();									  // load DLL and test for presence of DLP-USB1 and DLP-USB2
	Filter( 2 );								  // set the filter to the default
	ShutterClose();								  // close the shutter
	    
	m_Slider.SetRange(0, 100);
	m_Slider.SetPos( (int) ND_value );
	m_Slider.SetTicFreq(10);
    
	return TRUE;
}

COctopusShutterAndWheel::~COctopusShutterAndWheel() 
{
	Close();
	FreeLibrary(m_hmodule);
}

/***************************************************************************/
/***************************************************************************/

void COctopusShutterAndWheel::Loadem()
{
	unsigned char txbuf[25], rxbuf[25];
	DWORD ret_bytes;
	FT_STATUS status;
	
	//CString	m_NameNmbr = _T("Sutter Instrument Lambda 10-B");

	m_hmodule = LoadLibrary(_T("Ftd2xx.dll"));	
	
	m_pWrite            = (PtrToWrite)GetProcAddress(m_hmodule, "FT_Write");
	m_pRead             = (PtrToRead)GetProcAddress(m_hmodule, "FT_Read");
	m_pOpen             = (PtrToOpen)GetProcAddress(m_hmodule, "FT_Open");
	m_pOpenEx           = (PtrToOpenEx)GetProcAddress(m_hmodule, "FT_OpenEx");
	m_pListDevices      = (PtrToListDevices)GetProcAddress(m_hmodule, "FT_ListDevices");
	m_pClose            = (PtrToClose)GetProcAddress(m_hmodule, "FT_Close");
	m_pResetDevice      = (PtrToResetDevice)GetProcAddress(m_hmodule, "FT_ResetDevice");
	m_pPurge            = (PtrToPurge)GetProcAddress(m_hmodule, "FT_Purge");
	m_pSetTimeouts      = (PtrToSetTimeouts)GetProcAddress(m_hmodule, "FT_SetTimeouts");
	m_pGetQueueStatus   = (PtrToGetQueueStatus)GetProcAddress(m_hmodule, "FT_GetQueueStatus");
	m_pSetBaudRate      = (PtrToSetBaudRate)GetProcAddress(m_hmodule, "FT_SetBaudRate");
	m_pSetLatencyTimer  = (PtrToSetLatencyTimer)GetProcAddress(m_hmodule, "FT_SetLatencyTimer");
	m_pSetUSBParameters = (PtrToSetUSBParameters)GetProcAddress(m_hmodule, "FT_SetUSBParameters");

	OpenEx((PVOID)LPCTSTR("Sutter Instrument Lambda 10-B"), FT_OPEN_BY_DESCRIPTION);
	
	SetTimeouts(100, 100);
	
	Close();
	
	status = OpenEx((PVOID)LPCTSTR("Sutter Instrument Lambda 10-B"), FT_OPEN_BY_DESCRIPTION);
	
	if(status > 0)
	{
		USB_ready = false;
		board_present = 0;
	}
	else
	{
		ResetDevice();
		SetTimeouts(1000, 1000);
		Purge(FT_PURGE_RX || FT_PURGE_TX);
		SetLatencyTimer(2);
		SetBaudRate(128000);
		SetUSBParameters(64,0);
		Sleep(150);
		txbuf[0] = 0xEE;
		Write(txbuf, 1, &ret_bytes);
		Read(rxbuf, 2, &ret_bytes);
		Sleep(150);

		if(ret_bytes==0) Read(rxbuf, 2, &ret_bytes);
		
		if(rxbuf[0] != 0xEE)
		{
			USB_ready = false;
			board_present = 0;
			Close();
		}	
		else
		{
			USB_ready = true;	
			board_present = 1;
		}
	}

	B.load_wheel_failed = false;

	if ( board_present == 0 ) 
		B.load_wheel_failed = true;

	SetTimeouts(100, 100);
	UpdateData(FALSE);
}

/***************************************************************************/
/***************************************************************************/

void COctopusShutterAndWheel::ShutterOpen() 
{
	DWORD ret_bytes;
	unsigned char txbuf[25];

	if( USB_ready )
	{
		txbuf[0] = 0xDC;
		Write(txbuf, 1, &ret_bytes);
		txbuf[0] = 0xAA;
		Write(txbuf, 1, &ret_bytes);
		B.nd_setting = 100;
		UpdateShutterVal();
	}

	SetTimeouts(100, 100);
}

void COctopusShutterAndWheel::ShutterPartial( u8 val ) 
{
	DWORD ret_bytes;
	char txbuf[3];
	
	double temp = (double)val * 1.44;
	
	u8 out = 0;

	if( temp < 0.0 ) 
		out = 0;
	else if ( temp > 144.0) 
		out = 144;
	else 
		out = (u8) temp;
	
	if( USB_ready )
	{
		txbuf[0] = 0xDE;			// ND mode
		txbuf[1] = (char) out;		// value
		txbuf[2] = 0xAA;			// open
		Write(txbuf, 3, &ret_bytes);
		B.nd_setting = val;
		UpdateShutterVal();
	}

	SetTimeouts(100, 100);
}

void COctopusShutterAndWheel::ShutterClose() 
{
	DWORD ret_bytes;
	unsigned char txbuf[25];

	if( USB_ready )
	{
		txbuf[0] = 0xDC;
		Write(txbuf, 1, &ret_bytes);
		txbuf[0] = 0xAC;
		Write(txbuf, 1, &ret_bytes);
		m_ShutterOpen = 2;
		B.nd_setting = 0;
		UpdateShutterVal();
	}

	SetTimeouts(100, 100);
}

void COctopusShutterAndWheel::Filter( u8 filter ) 
{
	
	unsigned char txbuf[25];
	DWORD ret_bytes;

	if( USB_ready )
	{
		B.filter_wheel = filter;
		m_Filter = filter;
		u8 speed = 6;
		txbuf[0] = speed * 16 + filter;
		Write(txbuf, 1, &ret_bytes);
	}

	SetTimeouts(100, 100);
	UpdateData(FALSE);
}

void COctopusShutterAndWheel::OnRadioFilter0() { Filter ( 0 ); }
void COctopusShutterAndWheel::OnRadioFilter1() { Filter ( 1 ); }
void COctopusShutterAndWheel::OnRadioFilter2() { Filter ( 2 ); }
void COctopusShutterAndWheel::OnRadioFilter3() { Filter ( 3 ); }
void COctopusShutterAndWheel::OnRadioFilter4() { Filter ( 4 ); }
void COctopusShutterAndWheel::OnRadioFilter5() { Filter ( 5 ); }
void COctopusShutterAndWheel::OnRadioFilter6() { Filter ( 6 ); }
void COctopusShutterAndWheel::OnRadioFilter7() { Filter ( 7 ); }
void COctopusShutterAndWheel::OnRadioFilter8() { Filter ( 8 ); }
void COctopusShutterAndWheel::OnRadioFilter9() { Filter ( 9 ); }

bool COctopusShutterAndWheel::ShutterReady( void ) {
	
	DWORD bytes_in_buf;
	FT_STATUS status;
	
	status = GetQueueStatus(&bytes_in_buf);
	
	if( status == FT_OK ) 
		return true;
	else 
		return false;
}

/***************************************************************************/
/***************************************************************************/

FT_STATUS COctopusShutterAndWheel::Read(LPVOID lpvBuffer, DWORD dwBuffSize, LPDWORD lpdwBytesRead)
{
	return (*m_pRead)(m_ftHandle, lpvBuffer, dwBuffSize, lpdwBytesRead);
}	

FT_STATUS COctopusShutterAndWheel::SetBaudRate(ULONG nRate)
{
	return (*m_pSetBaudRate)(m_ftHandle, nRate);
}

FT_STATUS COctopusShutterAndWheel::SetLatencyTimer(UCHAR nTimer)
{
	return (*m_pSetLatencyTimer)(m_ftHandle, nTimer);
}

FT_STATUS COctopusShutterAndWheel::SetUSBParameters(DWORD nInTransferSize, DWORD nOutTransferSize)
{
	return (*m_pSetUSBParameters)(m_ftHandle, nInTransferSize, nOutTransferSize);
}

FT_STATUS COctopusShutterAndWheel::Write(LPVOID lpvBuffer, DWORD dwBuffSize, LPDWORD lpdwBytes)
{
	return (*m_pWrite)(m_ftHandle, lpvBuffer, dwBuffSize, lpdwBytes);
}	

FT_STATUS COctopusShutterAndWheel::Open(PVOID pvDevice)
{
	return (*m_pOpen)(pvDevice, &m_ftHandle );
}	

FT_STATUS COctopusShutterAndWheel::OpenEx(PVOID pArg1, DWORD dwFlags)
{
	return (*m_pOpenEx)(pArg1, dwFlags, &m_ftHandle);
}	

FT_STATUS COctopusShutterAndWheel::ListDevices(PVOID pArg1, PVOID pArg2, DWORD dwFlags)
{
	return (*m_pListDevices)(pArg1, pArg2, dwFlags);
}	

FT_STATUS COctopusShutterAndWheel::Close()
{
	return (*m_pClose)(m_ftHandle);
}	

FT_STATUS COctopusShutterAndWheel::ResetDevice()
{
	return (*m_pResetDevice)(m_ftHandle);
}	

FT_STATUS COctopusShutterAndWheel::Purge(ULONG dwMask)
{
	return (*m_pPurge)(m_ftHandle, dwMask);
}	

FT_STATUS COctopusShutterAndWheel::SetTimeouts(ULONG dwReadTimeout, ULONG dwWriteTimeout)
{
	return (*m_pSetTimeouts)(m_ftHandle, dwReadTimeout, dwWriteTimeout);
}	

FT_STATUS COctopusShutterAndWheel::GetQueueStatus(LPDWORD lpdwAmountInRxQueue)
{
	return (*m_pGetQueueStatus)(m_ftHandle, lpdwAmountInRxQueue);
}	


BOOL COctopusShutterAndWheel::OnCommand(WPARAM wParam, LPARAM lParam) {
	int id = LOWORD(wParam);     // Notification code
	if( id == 2 ) return FALSE;  // Trap ESC key
	if( id == 1 ) return FALSE;  // Trap RTN key
    return CDialog::OnCommand(wParam, lParam);
}

void COctopusShutterAndWheel::OnNMCustomdrawExecute( NMHDR *pNMHDR, LRESULT *pResult )
{
	LPNMCUSTOMDRAW pNMCD = reinterpret_cast<LPNMCUSTOMDRAW>(pNMHDR);
	
	int CurPos = m_Slider.GetPos();
		
	if( CurPos == 0) 
		ShutterClose();
	else if ( CurPos == 100 )
		ShutterOpen();
	else
		ShutterPartial( CurPos );

	*pResult = 0;	
}

void COctopusShutterAndWheel::UpdateShutterVal( void ) {

	CString str;
	str.Format(_T("Shutter (0-100 procent): %d"), B.nd_setting);
	
	if( IsWindowVisible() ) {
		m_Slider_Setting.SetWindowText( str );
		m_Slider.SetPos( (int) B.nd_setting );
		UpdateData(FALSE);
	}
}
