/*****************************************************************************
 Octopus microscope control software
 
 Copyright (C) 2004-2015 Jan Liphardt (jan.liphardt@stanford.edu)
 
 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License along
 with this program; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 ******************************************************************************/

/***************************Basically ready for Mindy*************************/
/*****************************************************************************/

#if !defined(AFX_H_OctopusMPC200)
#define AFX_H_OctopusMPC200

#include "stdafx.h"
#include "OctopusGlobals.h"
#include "roe200.h"

class COctopusMPC200 : public CDialog
{

public:
	
    COctopusMPC200(CWnd* pParent = NULL);
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

	CBitmap m_bmp_yes;
	CBitmap m_bmp_no;

	int	m_nID;
	int	m_nSpeed;
	
	long   	XCurr_Step;
	long   	YCurr_Step;
	long   	ZCurr_Step;

	double	m_nXCurr_Micron;
	double	m_nYCurr_Micron;
	double	m_nZCurr_Micron;

	CStatic	m_X_Now;
	CStatic	m_Y_Now;
	CStatic	m_Z_Now;

	bool run_the_wait_thread;
	CListBox m_SeqList;

	void ExecuteNextCommand( void );
	void StopEverything( void );

protected:

	FILE * pFile;
	FILE * fFile;

	CTime systemtime;

	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	afx_msg void OnKillfocusGeneral();
	virtual BOOL OnInitDialog();

	afx_msg void OnStop( void );
	afx_msg void OnGotoCenter( void );
	afx_msg void OnStepXYZ( void );

	HANDLE WaitThreadMove;
	HANDLE wait_thread_cmd;

	ROE200* actuator;
    
	bool moving;

	afx_msg void OnTimer(UINT_PTR nIDEvent);

	DECLARE_MESSAGE_MAP()
};

#endif // !defined(AFX_H_OctopusMPC200)




	
