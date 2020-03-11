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
#include "frWrap.h"

#include "Common.h"
#include <RadeonProRender.h>
#include "RenderParameters.h"
#include <max.h>
#include <stdmat.h>
#include <bitmap.h>
#include "3dsMaxDeclarations.h"
#include <iparamb2.h>
#include <map>
#include <vector>
#include <stack>
#include "UvwContext.h"
#include "expat.h"

FIRERENDER_NAMESPACE_BEGIN;

class FRMaterialImporter;

class XMLParserBase
{
friend class FRMaterialImporter;
public:
	typedef std::map<std::wstring, std::wstring> CAttributes;

	class CElement {
	public:
		CElement()
		{
		}

		~CElement() {
			for (std::vector<CElement *>::iterator ii = mChildren.begin(); ii != mChildren.end(); ii++)
				delete *ii;
		}

		void collectElements(const wchar_t *type, std::vector<CElement*>& res);
		std::wstring getAttribute(const wchar_t *name) const;
		
		CAttributes mAttributes;
		std::wstring mTag;
		std::wstring mText;
		std::vector<CElement *> mChildren;
	};

public:
	XMLParserBase();
	~XMLParserBase();

	XML_Status Parse(const std::wstring &data);

	void collectElements(const wchar_t *type, std::vector<CElement*>& res);

protected:
	static void expat_start_elem_handler(
		void* ptemplate,
		const XML_Char* el,
		const XML_Char** attr);

	static void expat_end_elem_handler(
		void* ptemplate,
		const XML_Char* el);

	static void expat_text_handler(
		void* ptemplate,
		const XML_Char* text,
		int len);

	static void expat_start_cdata_handler(void* ptemplate);
	static void expat_end_cdata_handler(void* ptemplate);

protected:
	XML_Parser mExpat;

	CElement *mRoot;

	std::stack<CElement *> mEleStack; // used to stack parents while parsing
};

FIRERENDER_NAMESPACE_END;
