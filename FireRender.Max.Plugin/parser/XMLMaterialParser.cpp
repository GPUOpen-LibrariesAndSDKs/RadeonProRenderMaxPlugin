/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Parse Radeon ProRender XML material format into 3ds MAX material nodes
*********************************************************************************************************************************/

#pragma once
#include "Common.h"

#include <maxscript/mxsPlugin/mxsPlugin.h>
#include <xref/iXrefMaterial.h>
#include <shaders.h>
#include "CoronaDeclarations.h"
#include "plugin/FireRenderDiffuseMtl.h"
#include "plugin/FireRenderBlendMtl.h"
#include "plugin/FireRenderAddMtl.h"
#include "plugin/FireRenderMicrofacetMtl.h"
#include "plugin/FireRenderReflectionMtl.h"
#include "plugin/FireRenderArithmMtl.h"
#include "plugin/FireRenderInputLUMtl.h"
#include "plugin/FireRenderBlendValueMtl.h"
#include "plugin/FireRenderRefractionMtl.h"
#include "plugin/FireRenderMFRefractionMtl.h"
#include "plugin/FireRenderTransparentMtl.h"
#include "plugin/FireRenderWardMtl.h"
#include "plugin/FireRenderEmissiveMtl.h"
#include "plugin/FireRenderFresnelMtl.h"
#include "plugin/FireRenderStandardMtl.h"
#include "plugin/FireRenderOrenNayarMtl.h"
#include "plugin/FireRenderNormalMtl.h"
#include "plugin/FireRenderDiffuseRefractionMtl.h"
#include "plugin/FireRenderFresnelSchlickMtl.h"
#include "XMLMaterialParser.h"


extern "C" {
	#include "AxfConverterDll.h"
}
//#include "utils\Utils.h"

FIRERENDER_NAMESPACE_BEGIN;

#define LRPR_MATERIAL_NODE_DIFFUSE L"DIFFUSE"
#define LRPR_MATERIAL_NODE_MICROFACET L"MICROFACET"
#define LRPR_MATERIAL_NODE_REFLECTION L"REFLECTION"
#define LRPR_MATERIAL_NODE_REFRACTION L"REFRACTION"
#define LRPR_MATERIAL_NODE_MICROFACET_REFRACTION L"MICROFACET_REFRACTION"
#define LRPR_MATERIAL_NODE_TRANSPARENT L"TRANSPARENT"
#define LRPR_MATERIAL_NODE_EMISSIVE L"EMISSIVE"
#define LRPR_MATERIAL_NODE_WARD L"WARD"
#define LRPR_MATERIAL_NODE_ADD L"ADD"
#define LRPR_MATERIAL_NODE_BLEND L"BLEND"
#define LRPR_MATERIAL_NODE_ARITHMETIC L"ARITHMETIC"
#define LRPR_MATERIAL_NODE_FRESNEL L"FRESNEL"
#define LRPR_MATERIAL_NODE_NORMAL_MAP L"NORMAL_MAP"
#define LRPR_MATERIAL_NODE_IMAGE_TEXTURE L"IMAGE_TEXTURE"
#define LRPR_MATERIAL_NODE_NOISE2D_TEXTURE L"NOISE2D_TEXTURE"
#define LRPR_MATERIAL_NODE_DOT_TEXTURE L"DOT_TEXTURE"
#define LRPR_MATERIAL_NODE_GRADIENT_TEXTURE L"GRADIENT_TEXTURE"
#define LRPR_MATERIAL_NODE_CHECKER_TEXTURE L"CHECKER_TEXTURE"
#define LRPR_MATERIAL_NODE_CONSTANT_TEXTURE L"CONSTANT_TEXTURE"
#define LRPR_MATERIAL_NODE_INPUT_LOOKUP L"INPUT_LOOKUP"
#define LRPR_MATERIAL_NODE_STANDARD L"STANDARD"
#define LRPR_MATERIAL_NODE_BLEND_VALUE L"BLEND_VALUE"
#define LRPR_MATERIAL_NODE_PASSTHROUGH L"PASSTHROUGH"
#define LRPR_MATERIAL_NODE_ORENNAYAR L"ORENNAYAR"
#define LRPR_MATERIAL_NODE_FRESNEL_SCHLICK L"FRESNEL_SCHLICK"
#define LRPR_MATERIAL_NODE_DIFFUSE_REFRACTION L"DIFFUSE_REFRACTION"
#define LRPR_MATERIAL_NODE_BUMP_MAP L"BUMP_MAP"

FireRenderClassDesc_MaterialImport fireRenderMaterialImporterClassDesc;

//////////////////////////////////////////////////////////////////////////////
//

class FRMaterialImporter : public SceneImport
{
public:
	FRMaterialImporter() {
	}

	~FRMaterialImporter() {
	}

	const bool doesAxfConverterDllExists()
	{
		HINSTANCE hGetProcIDDLL = LoadLibrary(_T("AxfConverter.dll"));
		
		if (hGetProcIDDLL)
		{
			FreeLibrary(hGetProcIDDLL);
			return true;
		}
		else
			return false;
	}

	// Number of extensions supported
	int ExtCount() override
	{
		if (doesAxfConverterDllExists())
			return 2;
		else
			return 1;
	}

	// Extension #n
	const TCHAR *Ext(int n) override
	{
		if (n == 0)
			return _T("XML");
		else
			return _T("AxF");
	}

	// Long ASCII description
	const TCHAR *LongDesc() override {
		if (doesAxfConverterDllExists()) {
			return _T("Radeon ProRender Materials and Appearance eXchange Format");
		}
		else {
			return _T("Radeon ProRender Materials");
		}
	}

	// Short ASCII description
	const TCHAR *ShortDesc() override {
		if (doesAxfConverterDllExists()) {
			return _T("Radeon ProRender and Appearance eXchange Format Materials");
		}
		else {
			return _T("Radeon ProRender Materials");
		}
	}

	// ASCII Author name
	const TCHAR *AuthorName() override {
		return _T("AMD");
	}

	// ASCII Copyright message
	const TCHAR *CopyrightMessage() override {
		return _T("(C) AMD");
	}

	// Other message #1
	const TCHAR *OtherMessage1() override {
		return _T("");
	}

	// Other message #2
	const TCHAR *OtherMessage2() override {
		return _T("");
	}

	// Version number * 100 (i.e. v3.01 = 301)
	unsigned int Version() override {
		return 100;
	}

	// Show DLL's "About..." box
	void ShowAbout(HWND hWnd) override {
	}

#define PARAM_ITERATE\
	std::vector<XMLParserBase::CElement*> params; \
	e->collectElements(L"param", params); \
	if (!params.empty()) \
	{ \
	for (std::vector<XMLParserBase::CElement*>::const_iterator jj = params.begin(); jj != params.end(); jj++) \
	{ \
	XMLParserBase::CElement* param = *jj; \
	std::wstring pname = param->getAttribute(_T("name")); \
	std::wstring ptype = param->getAttribute(_T("type")); \
	std::wstring pvalue = param->getAttribute(_T("value"));

#define END_PARAM_ITERATE\
	}\
	}

#define CASE_PARM(X)\
	if (pname == _T(X)) \
	{ \
	std::wstring value; \
	bool isConnection = (ptype == L"connection");

#define IFCONNECTION(X)\
	if (isConnection) \
		connections.push_back(SCONNECTION(pvalue, name, X));

#define IFVALUE(X)\
	if (!isConnection) \
	{ \
		setParameterValue(ptype, maxMat, X, pvalue); \
	}

#define ELSE_CASE_PARM(X)\
	} else \
	CASE_PARM(X)

#define END_CASE_PARM\
	}

	typedef std::map<std::wstring, MtlBase*> CLOADEDMATERIALS;
	typedef struct S_CONNECTION
	{
	public:
		std::wstring source;
		std::wstring target;
		ParamID targetPlug;
		S_CONNECTION(const std::wstring &s, const std::wstring & t, ParamID p)
			: source(s), target(t), targetPlug(p) {
		}
	} SCONNECTION;
	typedef std::vector<SCONNECTION> CCONNECTIONS;

	// maps an image texture (key), which is a proxy, to the actual loaded material (value)
	typedef std::map<std::wstring, std::wstring> CLOADEDIMAGETEXTURES;

#define CREATE_MAX_MATERIAL(CID, CNAME)\
	Object* obj = static_cast<Object*>(CreateInstance(MATERIAL_CLASS_ID, CID));\
	FireRender::FRMTLCLASSNAME(CNAME)* maxMat = (FireRender::FRMTLCLASSNAME(CNAME)*)(obj); \
	maxMat->SetName(MSTR(name.c_str())); \
	loadedMaterials.insert(std::make_pair(name, maxMat));

#define CREATE_MAX_TEXMAP(CID, CNAME)\
	Object* obj = static_cast<Object*>(CreateInstance(TEXMAP_CLASS_ID, CID)); \
	FireRender::FRMTLCLASSNAME(CNAME)* maxMat = (FireRender::FRMTLCLASSNAME(CNAME)*)(obj); \
	maxMat->SetName(MSTR(name.c_str())); \
	loadedMaterials.insert(std::make_pair(name, maxMat));

	bool IsConnection(const std::wstring &object, const CCONNECTIONS &connections)
	{
		bool isConnection = false;
		for (auto i : connections)
		{
			if (i.source == object)
			{
				isConnection = true;
				break;
			}
		}
		return isConnection;
	}


	void toLowerTCHAR(TCHAR *pStr)
	{
		while (*pStr++ = _totlower(*pStr));
	}

	typedef std::basic_string<TCHAR, std::char_traits<TCHAR>, std::allocator<TCHAR> > TString;

	// Do the actual import
	int DoImport(const TCHAR *pFilename, ImpInterface *i, Interface *gi, BOOL suppressPrompts = FALSE)
	{
		TCHAR axfXmlFilePath[255];

		TCHAR curLocale[256];
		GetLocaleInfo( LOCALE_CUSTOM_DEFAULT, LC_NUMERIC, curLocale, 256);
		_wsetlocale(LC_NUMERIC, L"en-US");

		TCHAR tcharFileName[255];
		wcscpy_s(tcharFileName, 255, pFilename);
		toLowerTCHAR(tcharFileName);
		
		TString tStringFileName(tcharFileName);

		bool isAxf = tStringFileName.find(L".axf") != -1;

		if (isAxf) {
			//convert axf to rpr xml using conversion dll:
			HINSTANCE hGetProcIDDLL = LoadLibrary(_T("AxfConverter.dll"));

			if (hGetProcIDDLL) {

				AxfConvertFile convertFile = (AxfConvertFile)GetProcAddress(hGetProcIDDLL, "convertAxFFile");
				
				char xmlFilePath[255];
				
				convertFile(ToAscii(pFilename).c_str(), xmlFilePath);

				FreeLibrary(hGetProcIDDLL);

				bool xmlCreated = strlen(xmlFilePath) != 0;
				if (xmlCreated) {
					std::wstring fileName = ToUnicode(xmlFilePath);
					wcsncpy_s(axfXmlFilePath, fileName.c_str(), 255);
				}
				else {
					return IMPEXP_FAIL;
				}
			}
			else {
				return IMPEXP_FAIL;
			}
		}

		std::wstring data;
		if (LoadUtf8FileToString((isAxf) ? axfXmlFilePath : pFilename, data))
		{
			if (!data.empty())
			{
				XMLParserBase xmlParser;

				xmlParser.mRoot = new XMLParserBase::CElement();
				xmlParser.mEleStack.push(xmlParser.mRoot);
				
				XML_Status res = xmlParser.Parse(data);
				if (res == XML_STATUS_OK)
				{
					std::vector<XMLParserBase::CElement*> nodes;
					xmlParser.collectElements(L"node", nodes);
					if (!nodes.empty()) 
					{
						// maps names of loaded material nodes to actual 3dsmax materials
						CLOADEDMATERIALS loadedMaterials;
						CLOADEDIMAGETEXTURES loadedImageTextures;
						// connections:
						// first string (KEY) is the name of the node whose output is to be connected to another node's input
						// VALUE is a pair where the first string is the name of the node that owns the input, the second string is the ID of the input plug
						// note: if a node's name does not appear in any of this map's keys, then such node is a root node and will be added
						// to the current material library.
						CCONNECTIONS connections;

						for (std::vector<XMLParserBase::CElement*>::const_iterator ii = nodes.begin(); ii != nodes.end(); ii++)
						{
							XMLParserBase::CElement* e = *ii;
							std::wstring type = e->getAttribute(_T("type"));
							std::wstring name = e->getAttribute(_T("name"));

							if (type == LRPR_MATERIAL_NODE_DIFFUSE)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_DIFFUSEMTL_CID, DiffuseMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRDiffuseMtl_COLOR_TEXMAP)
									IFVALUE(FRDiffuseMtl_COLOR)
									ELSE_CASE_PARM("normal")
									IFCONNECTION(FRDiffuseMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_MICROFACET)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_MICROFACETMTL_CID, MicrofacetMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRMicrofacetMtl_COLOR_TEXMAP)
									IFVALUE(FRMicrofacetMtl_COLOR)
									ELSE_CASE_PARM("ior")
									IFVALUE(FRMicrofacetMtl_IOR)
									ELSE_CASE_PARM("roughness")
									IFCONNECTION(FRMicrofacetMtl_ROUGHNESS_TEXMAP)
									IFVALUE(FRMicrofacetMtl_ROUGHNESS)
									ELSE_CASE_PARM("normal")
									IFCONNECTION(FRMicrofacetMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_REFLECTION)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_REFLECTIONMTL_CID, ReflectionMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRReflectionMtl_COLOR_TEXMAP)
									IFVALUE(FRReflectionMtl_COLOR)
									ELSE_CASE_PARM("normal")
									IFCONNECTION(FRReflectionMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_REFRACTION)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_REFRACTIONMTL_CID, RefractionMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRRefractionMtl_COLOR_TEXMAP)
									IFVALUE(FRRefractionMtl_COLOR)
									ELSE_CASE_PARM("ior")
									IFVALUE(FRRefractionMtl_IOR)
									ELSE_CASE_PARM("normal")
									IFCONNECTION(FRRefractionMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_MICROFACET_REFRACTION)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_MFREFRACTIONMTL_CID, MFRefractionMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRMFRefractionMtl_COLOR_TEXMAP)
									IFVALUE(FRMFRefractionMtl_COLOR)
									ELSE_CASE_PARM("ior")
									IFVALUE(FRMFRefractionMtl_IOR)
									ELSE_CASE_PARM("roughness")
									IFCONNECTION(FRMFRefractionMtl_ROUGHNESS_TEXMAP)
									IFVALUE(FRMFRefractionMtl_ROUGHNESS)
									ELSE_CASE_PARM("normal")
									IFCONNECTION(FRMFRefractionMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_TRANSPARENT)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_TRANSPARENTMTL_CID, TransparentMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRTransparentMtl_COLOR_TEXMAP)
									IFVALUE(FRTransparentMtl_COLOR)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_EMISSIVE)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_EMISSIVEMTL_CID, EmissiveMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FREmissiveMtl_COLOR_TEXMAP)
									IFVALUE(FREmissiveMtl_COLOR)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_WARD)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_WARDMTL_CID, WardMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRWardMtl_COLOR_TEXMAP)
									IFVALUE(FRWardMtl_COLOR)
									ELSE_CASE_PARM("rotation")
									IFCONNECTION(FRWardMtl_ROTATION_TEXMAP)
									IFVALUE(FRWardMtl_ROTATION)
									ELSE_CASE_PARM("roughness_x")
									IFCONNECTION(FRWardMtl_ROUGHNESSX_TEXMAP)
									IFVALUE(FRWardMtl_ROUGHNESSX)
									ELSE_CASE_PARM("roughness_y")
									IFCONNECTION(FRWardMtl_ROUGHNESSY_TEXMAP)
									IFVALUE(FRWardMtl_ROUGHNESSY)
									ELSE_CASE_PARM("normal")
									IFCONNECTION(FRWardMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_ADD)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_BLENDMTL_CID, AddMtl)
									PARAM_ITERATE
									CASE_PARM("color0")
									IFCONNECTION(FRAddMtl_COLOR0)
									ELSE_CASE_PARM("color1")
									IFCONNECTION(FRAddMtl_COLOR1)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_BLEND)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_BLENDMTL_CID, BlendMtl)
									PARAM_ITERATE
									CASE_PARM("color0")
									IFCONNECTION(FRBlendMtl_COLOR0)
									ELSE_CASE_PARM("color1")
									IFCONNECTION(FRBlendMtl_COLOR1)
									ELSE_CASE_PARM("weight")
									IFCONNECTION(FRBlendMtl_WEIGHT_TEXMAP)
									IFVALUE(FRBlendMtl_WEIGHT)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_ARITHMETIC)
							{
								CREATE_MAX_TEXMAP(FIRERENDER_ARITHMMTL_CID, ArithmMtl)
									PARAM_ITERATE
									CASE_PARM("color0")
									IFCONNECTION(FRArithmMtl_COLOR0_TEXMAP)
									IFVALUE(FRArithmMtl_COLOR0)
									ELSE_CASE_PARM("color1")
									IFCONNECTION(FRArithmMtl_COLOR1_TEXMAP)
									IFVALUE(FRArithmMtl_COLOR1)
									ELSE_CASE_PARM("op")
									IFVALUE(FRArithmMtl_OP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_FRESNEL)
							{
								CREATE_MAX_TEXMAP(FIRERENDER_FRESNELMTL_CID, FresnelMtl)
									PARAM_ITERATE
									CASE_PARM("invec")
									IFCONNECTION(FRFresnelMtl_INVEC_TEXMAP)
									ELSE_CASE_PARM("n")
									IFCONNECTION(FRFresnelMtl_N_TEXMAP)
									ELSE_CASE_PARM("ior")
									IFCONNECTION(FRFresnelMtl_IOR_TEXMAP)
									IFVALUE(FRFresnelMtl_IOR)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_NORMAL_MAP)
							{
								CREATE_MAX_TEXMAP(FIRERENDER_NORMALMTL_CID, NormalMtl)
									PARAM_ITERATE
									//CASE_PARM("uv")
									//IFCONNECTION(?)
									CASE_PARM("data")
									IFCONNECTION(FRNormalMtl_COLOR_TEXMAP)
									ELSE_CASE_PARM("bumpscale")
									IFVALUE(FRNormalMtl_STRENGTH)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_IMAGE_TEXTURE)
							{
								PARAM_ITERATE
									CASE_PARM("data")
									if (isConnection)
										loadedImageTextures.insert(std::make_pair(name, pvalue));
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_NOISE2D_TEXTURE)
							{
								MessageBox(0, L"LRPR_MATERIAL_NODE_NOISE2D_TEXTURE", L"XML UNSUPPORTED PLEASE IMPLEMENT", MB_OK | MB_ICONEXCLAMATION);
							}
							else if (type == LRPR_MATERIAL_NODE_DOT_TEXTURE)
							{
								MessageBox(0, L"LRPR_MATERIAL_NODE_DOT_TEXTURE", L"XML UNSUPPORTED PLEASE IMPLEMENT", MB_OK | MB_ICONEXCLAMATION);
							}
							else if (type == LRPR_MATERIAL_NODE_GRADIENT_TEXTURE)
							{
								MessageBox(0, L"LRPR_MATERIAL_NODE_GRADIENT_TEXTURE", L"XML UNSUPPORTED PLEASE IMPLEMENT", MB_OK | MB_ICONEXCLAMATION);
							}
							else if (type == LRPR_MATERIAL_NODE_CHECKER_TEXTURE)
							{
								MessageBox(0, L"LRPR_MATERIAL_NODE_CHECKER_TEXTURE", L"XML UNSUPPORTED PLEASE IMPLEMENT", MB_OK | MB_ICONEXCLAMATION);
							}
							else if (type == LRPR_MATERIAL_NODE_CONSTANT_TEXTURE)
							{
								MessageBox(0, L"LRPR_MATERIAL_NODE_CONSTANT_TEXTURE", L"XML UNSUPPORTED PLEASE IMPLEMENT", MB_OK | MB_ICONEXCLAMATION);
							}
							else if (type == LRPR_MATERIAL_NODE_INPUT_LOOKUP)
							{
								CREATE_MAX_TEXMAP(FIRERENDER_INPUTLUMTL_CID, InputLUMtl)
									PARAM_ITERATE
									CASE_PARM("value")
									IFVALUE(InputLUMtl_VALUE)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_STANDARD)
							{
								MessageBox(0, L"LRPR_MATERIAL_NODE_STANDARD", L"XML UNSUPPORTED PLEASE IMPLEMENT", MB_OK | MB_ICONEXCLAMATION);
							}
							else if (type == LRPR_MATERIAL_NODE_BLEND_VALUE)
							{
								CREATE_MAX_TEXMAP(FIRERENDER_BLENDVALUEMTL_CID, BlendValueMtl)
									PARAM_ITERATE
									CASE_PARM("color0")
									IFCONNECTION(FRBlendValueMtl_COLOR0_TEXMAP)
									IFVALUE(FRBlendValueMtl_COLOR0)
									ELSE_CASE_PARM("color1")
									IFCONNECTION(FRBlendValueMtl_COLOR1_TEXMAP)
									IFVALUE(FRBlendValueMtl_COLOR1)
									ELSE_CASE_PARM("weight")
									IFCONNECTION(FRBlendValueMtl_WEIGHT_TEXMAP)
									IFVALUE(FRBlendValueMtl_WEIGHT)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_PASSTHROUGH)
							{
								MessageBox(0, L"LRPR_MATERIAL_NODE_PASSTHROUGH", L"XML UNSUPPORTED PLEASE IMPLEMENT", MB_OK | MB_ICONEXCLAMATION);
							}
							else if (type == LRPR_MATERIAL_NODE_ORENNAYAR)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_ORENNAYARMTL_CID, OrenNayarMtl)
									PARAM_ITERATE
									CASE_PARM("color")
										IFCONNECTION(FROrenNayarMtl_COLOR_TEXMAP)
										IFVALUE(FROrenNayarMtl_COLOR)
									ELSE_CASE_PARM("roughness")
										IFCONNECTION(FROrenNayarMtl_ROUGHNESS_TEXMAP)
										IFVALUE(FROrenNayarMtl_ROUGHNESS)
									ELSE_CASE_PARM("normal")
										IFCONNECTION(FROrenNayarMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_FRESNEL_SCHLICK)
							{
								CREATE_MAX_TEXMAP(FIRERENDER_FRESNELSCHLICKMTL_CID, FresnelSchlickMtl)
									PARAM_ITERATE
									CASE_PARM("normal")
									IFCONNECTION(FRFresnelSchlickMtl_N_TEXMAP)
									ELSE_CASE_PARM("invec")
									IFCONNECTION(FRFresnelSchlickMtl_INVEC_TEXMAP)
									ELSE_CASE_PARM("reflectance")
									IFCONNECTION(FRFresnelSchlickMtl_REFLECTANCE_TEXMAP)
									IFVALUE(FRFresnelSchlickMtl_REFLECTANCE)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_DIFFUSE_REFRACTION)
							{
								CREATE_MAX_MATERIAL(FIRERENDER_DIFFUSEREFRACTIONMTL_CID, DiffuseRefractionMtl)
									PARAM_ITERATE
									CASE_PARM("color")
									IFCONNECTION(FRDiffuseRefractionMtl_COLOR_TEXMAP)
									IFVALUE(FRDiffuseRefractionMtl_COLOR)
									ELSE_CASE_PARM("roughness")
									IFCONNECTION(FRDiffuseRefractionMtl_ROUGHNESS_TEXMAP)
									IFVALUE(FRDiffuseRefractionMtl_ROUGHNESS)
									ELSE_CASE_PARM("normal")
									IFCONNECTION(FRDiffuseRefractionMtl_NORMALMAP)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == LRPR_MATERIAL_NODE_BUMP_MAP)
							{
								CREATE_MAX_TEXMAP(FIRERENDER_NORMALMTL_CID, NormalMtl)
								IParamBlock2* pb = maxMat->GetParamBlock(0);
								pb->SetValue(FRNormalMtl_ISBUMP, 0, TRUE);
								PARAM_ITERATE
									//CASE_PARM("uv")
									//IFCONNECTION(?)
									CASE_PARM("data")
									IFCONNECTION(FRNormalMtl_COLOR_TEXMAP)
									ELSE_CASE_PARM("bumpscale")
									IFVALUE(FRNormalMtl_STRENGTH)
									END_CASE_PARM
									END_PARAM_ITERATE
							}
							else if (type == L"INPUT_TEXTURE")
							{
								Object* obj = static_cast<Object*>(CreateInstance(TEXMAP_CLASS_ID, Class_ID(BMTEX_CLASS_ID, 0)));
								auto maxMat = dynamic_cast<BitmapTex*>(obj->GetInterface(BITMAPTEX_INTERFACE));
								maxMat->SetName(MSTR(name.c_str()));
								loadedMaterials.insert(std::make_pair(name, maxMat));
								std::wstring path;
								std::wstring gamma;
								PARAM_ITERATE
								CASE_PARM("path")
								path = pvalue;
								END_CASE_PARM
									CASE_PARM("gamma")
										gamma = pvalue;
									END_CASE_PARM
								END_PARAM_ITERATE
								if (!path.empty())
								{
									BMMRES status;
									BitmapInfo bi;

									BOOL sm = ::TheManager->SilentMode();
									::TheManager->SetSilentMode(TRUE);
									
									std::wstring location((isAxf) ? axfXmlFilePath : pFilename);
									auto pos = location.find_last_of('\\');
									if (pos == std::wstring::npos)
										pos = location.find_last_of('/');
									if (pos != std::wstring::npos)
									{
										location = location.substr(0, pos + 1);
										MaxSDK::Util::Path imagePath(path.c_str());
										imagePath.StripToLeaf();
										imagePath.SetPath((location + imagePath.GetCStr()).c_str());
										bi.SetPath(imagePath);
										if (!gamma.empty())
										{
											float fgamma = _wtof(gamma.c_str());
											bi.SetCustomFlag(BMM_CUSTOM_GAMMA);
											bi.SetCustomGamma(fgamma);
										}
										Bitmap *bmap = ::TheManager->Load(&bi, &status);
										if (status == BMMRES_SUCCESS)
										{
											maxMat->SetMap(bi.GetAsset());
											maxMat->ReloadBitmapAndUpdate();
											maxMat->fnReload();
										}
										else
										{
											MaxSDK::Util::Path imagePath(path.c_str());
											bi.SetPath(imagePath);
											Bitmap *bmap = ::TheManager->Load(&bi, &status);
											if (status == BMMRES_SUCCESS)
											{
												maxMat->SetMap(bi.GetAsset());
												maxMat->ReloadBitmapAndUpdate();
												maxMat->fnReload();
											}
										}
									}

									::TheManager->SetSilentMode(sm);
								}
							}
						}

						// pass 2 : connect plugs
						for (auto cc : connections)
						{
							auto mm = loadedMaterials.find(cc.source);
							if (mm == loadedMaterials.end())
							{
								auto ii = loadedImageTextures.find(cc.source);
								if (ii != loadedImageTextures.end())
									mm = loadedMaterials.find(ii->second);
							}
							if (mm != loadedMaterials.end())
							{
								MtlBase *output = mm->second;
								Mtl* outputMtl = dynamic_cast<Mtl*>(output);
								Texmap* outputTex = dynamic_cast<Texmap*>(output);
								mm = loadedMaterials.find(cc.target);
								if (mm != loadedMaterials.end())
								{
									MtlBase *input = mm->second;
									const ParamID &inputPlug = cc.targetPlug;
									IParamBlock2* pb = input->GetParamBlock(0);
									if (outputTex)
									{
										FASSERT(pb->SetValue(inputPlug, 0, outputTex));
									}
									else if (outputMtl)
									{
										FASSERT(pb->SetValue(inputPlug, 0, outputMtl));
									}
								}
							}
						}
						
						// pass 3 : add root materials to the current material library
						for (CLOADEDMATERIALS::const_iterator ii = loadedMaterials.begin(); ii != loadedMaterials.end(); ii++)
						{

							bool isConnection = IsConnection(ii->first, connections);
							if (!isConnection)
							{
								for (auto tt : loadedImageTextures)
								{
									if (ii->first == tt.second)
									{
										isConnection = IsConnection(tt.first, connections);
										break;
									}
								}
							}
							if (!isConnection)
								GetCOREInterface()->GetMaterialLibrary().Add(ii->second);
						}

					}

				}
				delete xmlParser.mRoot;
				xmlParser.mRoot = 0;
			}
		}
		
		_wsetlocale(LC_NUMERIC, curLocale);
		
		return IMPEXP_SUCCESS;
	}

protected:

	void setParameterValue(const std::wstring& valueType, MtlBase *mtl, ParamID pid, const std::wstring &value)
	{
		// uint
		// file_path
		if (valueType == L"float4" || valueType == L"float3")
		{
			double x = 1.0, y = 1.0, z = 1.0;
			if (!value.empty())
			{
				std::wstringstream ss(value);
				std::vector<std::wstring> results;
				while (ss.good())
				{
					wchar_t buf[64];
					ss.getline(buf, 64, ',');
					results.push_back(buf);
				}
				std::vector<std::wstring>::size_type sz = results.size();
				if (sz >= 3)
				{
					x = _wtof(results[0].c_str());
					y = _wtof(results[1].c_str());
					z = _wtof(results[2].c_str());
				}
			}
			IParamBlock2* pb = mtl->GetParamBlock(0);
			auto def = pb->GetParamDef(pid);
			switch (def.type)
			{
				case TYPE_FLOAT:
				case TYPE_WORLD:
				case TYPE_ANGLE:
					FASSERT(pb->SetValue(pid, 0, float(x)));
				break;

				case TYPE_RGBA:
				case TYPE_COLOR:
					FASSERT(pb->SetValue(pid, 0, Color(x, y, z)));
				break;

				case TYPE_POINT3:
				{
					FASSERT(pb->SetValue(pid, 0, Point3(x, y, z)));
				}
				break;
			}
		}
		else if (valueType == L"uint")
		{
			int v = 0;
			if (!value.empty())
				v = _wtoi(value.c_str());
			IParamBlock2* pb = mtl->GetParamBlock(0);
			auto def = pb->GetParamDef(pid);
			switch (def.type)
			{
				case TYPE_INT:
				case TYPE_ENUM:
					FASSERT(pb->SetValue(pid, 0, BOOL(v)));
				break;
				
				case TYPE_BOOL:
					FASSERT(pb->SetValue(pid, 0, BOOL(v)));
				break;
			}
		}
	}

	size_t GetSizeOfFile(const wchar_t *path)
	{
		struct _stat fileinfo;
		_wstat(path, &fileinfo);
		return fileinfo.st_size;
	}

	bool LoadUtf8FileToString(const wchar_t *pFilename, std::wstring &buffer)
	{
		bool res = false;
		FILE* f = 0;
		if (!_wfopen_s(&f, pFilename, L"rtS, ccs=UTF-8"))
		{
			if (f)
			{
				size_t filesize = GetSizeOfFile(pFilename);
				if (filesize > 0) {
					buffer.resize(filesize);
					size_t wchars_read = fread(&(buffer.front()), sizeof(wchar_t), filesize, f);
					buffer.resize(wchars_read);
					buffer.shrink_to_fit();
				}
				fclose(f);
				res = true;
			}
		}
		return res;
	}
};

//////////////////////////////////////////////////////////////////////////////
//

void* FireRenderClassDesc_MaterialImport::Create(BOOL loading)
{
	return new FRMaterialImporter;
}

//////////////////////////////////////////////////////////////////////////////
//

void XMLParserBase::CElement::collectElements(const wchar_t *type, std::vector<CElement*>& res)
{
	for (std::vector<CElement *>::iterator ii = mChildren.begin(); ii != mChildren.end(); ii++)
	{
		if ((*ii)->mTag == type)
			res.push_back(*ii);
		(*ii)->collectElements(type, res);
	}
}

std::wstring XMLParserBase::CElement::getAttribute(const wchar_t *name) const
{
	CAttributes::const_iterator ii = mAttributes.find(name);
	if (ii != mAttributes.end())
		return ii->second;
	return L"";
}

XMLParserBase::XMLParserBase() : mExpat(XML_ParserCreate(L"UTF-16"))
{
}

XMLParserBase::~XMLParserBase()
{
}

XML_Status XMLParserBase::Parse(const std::wstring &data)
{
	XML_SetUserData(mExpat, this);
	XML_SetElementHandler(mExpat, expat_start_elem_handler, expat_end_elem_handler);
	XML_SetCharacterDataHandler(mExpat, expat_text_handler);
	return XML_Parse(mExpat, reinterpret_cast<const char *>(data.c_str()), int(data.length() * sizeof(wchar_t)), 1);
}

void XMLParserBase::expat_start_elem_handler(void* handler, const XML_Char* el, const XML_Char** attr)
{
	XMLParserBase* ha = reinterpret_cast<XMLParserBase*>(handler);

	CElement *parent = ha->mEleStack.top();
	CElement *newele = new CElement;
	parent->mChildren.push_back(newele);
	ha->mEleStack.push(newele);
	newele->mTag = el;
	int i = 0;
	while (attr[i] != 0) 
	{
		newele->mAttributes.insert(std::make_pair(attr[i], attr[i + 1]));
		i += 2;
	}
}

void XMLParserBase::expat_end_elem_handler(void* handler, const XML_Char* el)
{

	XMLParserBase* ha = reinterpret_cast<XMLParserBase*>(handler);
	ha->mEleStack.pop();
}

void XMLParserBase::expat_text_handler(void* handler, const XML_Char* text, int len)
{
	XMLParserBase* ha = reinterpret_cast<XMLParserBase*>(handler);
	const XML_Char *i = text;
	const XML_Char *e = text + len * sizeof(XML_Char);
	while (1) {
		while ((i < e) && (*i != '<'))
			i++;
		if (i > e)
			return;
		i++;
		if (i > e)
			return;
		if (*i == '/')
			break;
	}
	size_t s = size_t(i - text);
	if (s > 0) {
		XML_Char *tmp = new XML_Char[s + 2];
		wcsncpy_s(tmp, s + 2, text, s - 1);
		tmp[s - 1] = '\0';
		CElement *ele = ha->mEleStack.top();
		ele->mText = tmp;
		delete[] tmp;
	}
}

void XMLParserBase::collectElements(const wchar_t *type, std::vector<CElement*>& res)
{
	mRoot->collectElements(type, res);
}

FIRERENDER_NAMESPACE_END;
