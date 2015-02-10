//////////////////////////////////////////////////////////////////////
// OctopusClock.h: interface for the COctopusGoodClock class.
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_OctopusClock)
#define AFX_H_OctopusClock

#include "stdafx.h"

class COctopusGoodClock  
{

private:
	bool    NotInitialized;
            __int64 Frequency;
            __int64 BeginTime;
	mutable __int64 EndTime;

public:
	COctopusGoodClock();
	virtual ~COctopusGoodClock();
	double End(void) const;	

};

#endif
