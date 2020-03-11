/**********************************************************************
Copyright 2020 Advanced Micro Devices, Inc
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
********************************************************************/

#pragma once

#if defined(MAX_PLUGIN)
	#include <max.h>
#endif

FIRERENDER_NAMESPACE_BEGIN

class RprExporter : public SceneExport
{
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

FIRERENDER_NAMESPACE_END
