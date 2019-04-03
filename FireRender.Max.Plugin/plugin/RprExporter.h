/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2019 AMD
* All Rights Reserved
*
* Plugin for exporting Radeon ProRenderer scene 
*********************************************************************************************************************************/

#pragma once

#if defined(MAX_PLUGIN)
	#include <max.h>
#endif

FIRERENDER_NAMESPACE_BEGIN;

class RprExporter : public SceneExport {
public:

	static ClassDesc* GetClassDesc();

	virtual int	ExtCount() { return 1; };

	virtual const MCHAR *	Ext(int n);					
															
	virtual const MCHAR *	LongDesc();

	virtual const MCHAR *	ShortDesc();
										
	virtual const MCHAR *	AuthorName();
														
	virtual const MCHAR *	CopyrightMessage();
															
	virtual const MCHAR *	OtherMessage1();
												
	virtual const MCHAR *	OtherMessage2();
															
	virtual unsigned int	Version() { return 100; };			
															
	virtual void			ShowAbout(HWND hWnd) {};


	virtual int				DoExport(const MCHAR *name, ExpInterface *ei, Interface *i, BOOL suppressPrompts = FALSE, DWORD options = 0);
};

FIRERENDER_NAMESPACE_END;