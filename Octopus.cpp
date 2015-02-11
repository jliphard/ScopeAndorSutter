////////////////////////////////////////////////////////////////////////////////
// Octopus.cpp : Defines the class behaviors for the application.
////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Octopus.h"
#include "MainFrm.h"
#include "OctopusView.h"

COctopusApp::COctopusApp() {}

COctopusApp theApp;

BOOL COctopusApp::InitInstance() 
{

	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);
	//EnableVisualStyles();
	
	AfxEnableControlContainer();

	LoadStdProfileSettings();

	CSingleDocTemplate* pDocTemplate;
	
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(COctopusDoc),
		RUNTIME_CLASS(CMainFrame),
		RUNTIME_CLASS(COctopusView)
		);
	
	AddDocTemplate(pDocTemplate);

	CCommandLineInfo cmdInfo;
	
	ParseCommandLine(cmdInfo);
	
	if (!ProcessShellCommand(cmdInfo)) return false;

	return true;
}

int COctopusApp::ExitInstance() {return CWinApp::ExitInstance();}
