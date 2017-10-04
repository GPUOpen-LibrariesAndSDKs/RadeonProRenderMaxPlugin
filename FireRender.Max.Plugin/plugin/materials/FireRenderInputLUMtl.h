#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;

enum InputLUMtl_ParamID : ParamID {
	InputLUMtl_VALUE = 1000
};

class FireRenderInputLUMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Input Lookup"); }
	static Class_ID ClassId() { return FIRERENDER_INPUTLUMTL_CID; }
};

class FireRenderInputLUMtl :
	public FireRenderTex<FireRenderInputLUMtlTraits, FireRenderInputLUMtl>
{
public:
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
