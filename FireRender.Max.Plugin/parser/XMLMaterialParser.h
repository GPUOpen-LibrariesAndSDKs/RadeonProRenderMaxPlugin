/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Parse Radeon ProRender XML material format into 3ds MAX material nodes
*********************************************************************************************************************************/

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
