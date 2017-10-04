#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;


FireRenderTexBase::FireRenderTexBase(IParamBlock2* block) :
	pblock(block)
{}

void FireRenderTexBase::DeleteThis()
{
	delete this;
}

RefResult FireRenderTexBase::NotifyRefChanged(NOTIFY_REF_CHANGED_PARAMETERS)
{
	return REF_SUCCEED;
}

int FireRenderTexBase::NumRefs()
{
	return 1;
}
RefTargetHandle FireRenderTexBase::GetReference(int i)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	return pblock;
}

void FireRenderTexBase::SetReference(int i, RefTargetHandle rtarg)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	pblock = dynamic_cast<IParamBlock2*>(rtarg);
}

int FireRenderTexBase::NumSubs()
{
	return NumRefs();
}

Animatable* FireRenderTexBase::SubAnim(int i)
{
	return GetReference(i);
}
TSTR FireRenderTexBase::SubAnimName(int i)
{
	FASSERT(unsigned(i) < unsigned(NumRefs()));
	return _T("Pblock");
}

int FireRenderTexBase::SubNumToRefNum(int subNum)
{
	return subNum;
}

AColor FireRenderTexBase::EvalColor(ShadeContext &)
{
	return AColor(0.0, 0.0, 0.0);
}

Point3 FireRenderTexBase::EvalNormalPerturb(ShadeContext& sc)
{
	return Point3(0.0, 0.0, 0.0);
}


FireRenderMtlBase::FireRenderMtlBase(IParamBlock2* block) :
	pblock(block)
{
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

void FireRenderMtlBase::Update(TimeValue t, Interval& valid)
{
	for (int i = 0; i < NumSubTexmaps(); ++i)
	{
		Texmap* map = GetSubTexmap(i);

		if (map != NULL)
		{
			map->Update(t, valid);
		}
	}

	pblock->GetValidity(t, valid);
}

FIRERENDER_NAMESPACE_END;
