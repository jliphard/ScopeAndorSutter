// COctopusShutterAndWheel.h : header file

#if !defined(AFX_H_OctopusShutterAndWheel)
#define AFX_H_OctopusShutterAndWheel

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "ftd2xx.h"

class COctopusShutterAndWheel : public CDialog
{

public:
	
	COctopusShutterAndWheel(CWnd* pParent = NULL);
	virtual ~COctopusShutterAndWheel();

	enum { IDD = IDC_LAMBDA };

protected:

	int board_present;
	int bus_busy;
	
	UINT m_nTimer;

	double      ND_value;
	CStatic     m_Slider_Setting;
	CSliderCtrl m_Slider;
	CString     m_Slider_Setting_String;
	int			m_ShutterOpen;
	int			m_Filter;
	HMODULE		m_hmodule;
	FT_HANDLE	m_ftHandle;
	bool		partialmode;
	bool        USB_ready;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);

	void Loadem();

	FT_STATUS Write(LPVOID, DWORD, LPDWORD);
	FT_STATUS SetTimeouts(ULONG, ULONG);
	FT_STATUS Read(LPVOID, DWORD, LPDWORD);

	typedef FT_STATUS (WINAPI *PtrToOpen)(PVOID, FT_HANDLE *); 
	PtrToOpen m_pOpen; 
	FT_STATUS Open(PVOID);

	typedef FT_STATUS (WINAPI *PtrToOpenEx)(PVOID, DWORD, FT_HANDLE *); 
	PtrToOpenEx m_pOpenEx; 
	FT_STATUS OpenEx(PVOID, DWORD);

	typedef FT_STATUS (WINAPI *PtrToListDevices)(PVOID, PVOID, DWORD);
	PtrToListDevices m_pListDevices; 
	FT_STATUS ListDevices(PVOID, PVOID, DWORD);

	typedef FT_STATUS (WINAPI *PtrToClose)(FT_HANDLE);
	PtrToClose m_pClose;
	FT_STATUS Close();

	typedef FT_STATUS (WINAPI *PtrToRead)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
	PtrToRead m_pRead;

	typedef FT_STATUS (WINAPI *PtrToWrite)(FT_HANDLE, LPVOID, DWORD, LPDWORD);
	PtrToWrite m_pWrite;

	typedef FT_STATUS (WINAPI *PtrToResetDevice)(FT_HANDLE);
	PtrToResetDevice m_pResetDevice;
	FT_STATUS ResetDevice();
	
	typedef FT_STATUS (WINAPI *PtrToPurge)(FT_HANDLE, ULONG);
	PtrToPurge m_pPurge;
	FT_STATUS Purge(ULONG);
	
	typedef FT_STATUS (WINAPI *PtrToSetTimeouts)(FT_HANDLE, ULONG, ULONG);
	PtrToSetTimeouts m_pSetTimeouts;

	typedef FT_STATUS (WINAPI *PtrToGetQueueStatus)(FT_HANDLE, LPDWORD);
	PtrToGetQueueStatus m_pGetQueueStatus;
	FT_STATUS GetQueueStatus(LPDWORD);

	typedef FT_STATUS (WINAPI *PtrToSetBaudRate)(FT_HANDLE, ULONG);
	PtrToSetBaudRate m_pSetBaudRate;
	FT_STATUS SetBaudRate(ULONG);

	typedef FT_STATUS (WINAPI *PtrToSetLatencyTimer)(FT_HANDLE, UCHAR);
	PtrToSetLatencyTimer m_pSetLatencyTimer;
	FT_STATUS SetLatencyTimer(UCHAR);

	typedef FT_STATUS (WINAPI *PtrToSetUSBParameters)(FT_HANDLE, DWORD, DWORD);
	PtrToSetUSBParameters m_pSetUSBParameters;
	FT_STATUS SetUSBParameters(DWORD, DWORD);

	virtual BOOL OnInitDialog();
	
	void UpdateShutterVal( void );

	afx_msg void OnRadioFilter0();
	afx_msg void OnRadioFilter1();
	afx_msg void OnRadioFilter2();
	afx_msg void OnRadioFilter3();
	afx_msg void OnRadioFilter4();
	afx_msg void OnRadioFilter5();
	afx_msg void OnRadioFilter6();
	afx_msg void OnRadioFilter7();
	afx_msg void OnRadioFilter8();
	afx_msg void OnRadioFilter9();
	
	DECLARE_MESSAGE_MAP()

public:

	bool ShutterReady ( void );
	void ShutterOpen( void );
	void ShutterClose( void );
	void ShutterPartial( u8 val );
	void Filter( u8 filter ); 

	afx_msg void OnNMCustomdrawExecute( NMHDR* pNMHDR, LRESULT* pResult );
};

#endif
