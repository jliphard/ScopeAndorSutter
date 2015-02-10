#if !defined(AFX_H_OctopusLog)
#define AFX_H_OctopusLog

#include "stdafx.h"
#include "OctopusGlobals.h"
#include <stdio.h>

using namespace std;

class COctopusLog
{

public:
	
	COctopusLog();
	~COctopusLog();
	void Write( CString step );	

protected:

	CTime systemtime;
	bool Open_File( void );
	FILE * pFile;
	u32 entry;
};

#endif
