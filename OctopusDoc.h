/////////////////////////////////////////////////////////////////////////////
// OctopusDoc.h : interface of the COctopusDoc class
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_H_OctopusDoc)
#define AFX_H_OctopusDoc

#include "stdafx.h"

class COctopusDoc : public CDocument
{

protected:
	COctopusDoc();
	DECLARE_DYNCREATE(COctopusDoc)

public:
	virtual BOOL OnNewDocument();
	virtual ~COctopusDoc();

};

#endif