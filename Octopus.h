///////////////////////////////////////////////////////////////////////////
// Octopus.h : main header file for the Octopus application
///////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_Octopus)
#define AFX_H_Octopus

#include "stdafx.h"
#include "resource.h"       // main symbols

class COctopusApp : public CWinApp
{

public:
	COctopusApp();
	virtual BOOL InitInstance();
	virtual int ExitInstance();

};

#endif