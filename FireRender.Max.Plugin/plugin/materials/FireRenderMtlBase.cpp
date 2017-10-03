#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN

FireRenderMtlBase::FireRenderMtlBase(IParamBlock2* block) :
	pblock(block)
{
}

void FireRenderMtlBase::SetCurrentTime(TimeValue t)
{
	currentTime = t;
}

void FireRenderMtlBase::DeleteThis()
{
	delete this;
}

RefResult FireRenderMtlBase::NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS)
{
	return REF_SUCCEED;
}


RefTargetHandle FireRenderMtlBase::GetReference(int i)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	return pblock;
}

void FireRenderMtlBase::SetReference(int i, RefTargetHandle rtarg)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	this->pblock = dynamic_cast<IParamBlock2*>(rtarg);
}

TSTR FireRenderMtlBase::SubAnimName(int i)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	return _T("Pblock");
}

int FireRenderMtlBase::NumRefs()
{
	return 1;
}

int FireRenderMtlBase::NumSubs()
{
	return NumRefs();
}

Animatable* FireRenderMtlBase::SubAnim(int i)
{
	return GetReference(i);
}

int FireRenderMtlBase::SubNumToRefNum(int subNum)
{
	return subNum;
}

float FireRenderMtlBase::GetSelfIllum(int mtlNum, BOOL backFace)
{
	return 0.f;
}

Color FireRenderMtlBase::GetSelfIllumColor(int mtlNum, BOOL backFace)
{
	return Color(0.f, 0.f, 0.f);
}

Color FireRenderMtlBase::GetDiffuse(int mtlNum, BOOL backFace)
{
	return Color(0.f, 0.f, 0.f);
}

Color FireRenderMtlBase::GetSpecular(int mtlNum, BOOL backFace)
{
	return Color(0.f, 0.f, 0.f);
}

float FireRenderMtlBase::GetShininess(int mtlNum, BOOL backFace)
{
	return 0.f;
}

float FireRenderMtlBase::GetShinStr(int mtlNum, BOOL backFace)
{
	return 0.f;
}

Color FireRenderMtlBase::GetAmbient(int mtlNum, BOOL backFace)
{
	return Color(0.f, 0.f, 0.f);
}

float FireRenderMtlBase::GetXParency(int mtlNum, BOOL backFace)
{
	return 0.0f;
}

void FireRenderMtlBase::SetAmbient(Color c, TimeValue t)
{
}

void FireRenderMtlBase::SetDiffuse(Color c, TimeValue t)
{
}

void FireRenderMtlBase::SetSpecular(Color c, TimeValue t)
{
}

void FireRenderMtlBase::SetShininess(float v, TimeValue t)
{
}

void FireRenderMtlBase::Shade(::ShadeContext& context)
{
}

FIRERENDER_NAMESPACE_END
