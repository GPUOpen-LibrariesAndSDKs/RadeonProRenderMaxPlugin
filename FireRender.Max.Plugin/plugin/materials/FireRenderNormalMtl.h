#pragma once

#include "FireRenderMtlBase.h"

FIRERENDER_NAMESPACE_BEGIN;;

enum FRNormalMtl_TexmapId {
	FRNormalMtl_TEXMAP_COLOR = 0,
	FRNormalMtl_TEXMAP_ADDITIONALBUMP = 1
};

enum FRNormalMtl_ParamID : ParamID {
	FRNormalMtl_COLOR_TEXMAP = 1000,
	FRNormalMtl_STRENGTH,
	FRNormalMtl_ISBUMP,
	FRNormalMtl_ADDITIONALBUMP,
	FRNormalMtl_SWAPRG,
	FRNormalMtl_INVERTG,
	FRNormalMtl_INVERTR,
	FRNormalMtl_BUMPSTRENGTH
};


class FireRenderNormalMtlTraits
{
public:
	static const TCHAR* InternalName() { return _T("RPR Normal"); }
	static Class_ID ClassId() { return FIRERENDER_NORMALMTL_CID; }
};

class FireRenderNormalMtl :
	public FireRenderTex<FireRenderNormalMtlTraits, FireRenderNormalMtl>
{
public:

	static frw::Value translateGenericBump(const TimeValue t, Texmap *bump, const float& strength, MaterialParser& mtlParser);

	static frw::Image bitmap2image(Bitmap *bmp, MaterialParser& mtlParser);

	static Bitmap *createImageFromMap(const TimeValue t, Texmap* input, MaterialParser& mtlParser, bool &deleteAfterwards);

	static inline BMM_Color_fl SampleBitmap(Bitmap *bmp, float u, float v);
	static inline BMM_Color_fl SampleBitmap(Bitmap *bmp, int u, int v);

	static Bitmap *BumpToNormalRPR(Bitmap *bumpBitmap, float strength = 1.f);
	static Bitmap *ProcessBitmap(Bitmap *bumpBitmap, BOOL swapRG, BOOL flipR, BOOL flipG);

	static bool IsGrayScale(Bitmap *bm);

	static Bitmap *findBitmap(const TimeValue t, MtlBase *mat);

	AColor EvalColor(ShadeContext& sc) override;
	void Update(TimeValue t, Interval& valid) override;
	frw::Value GetShader(const TimeValue t, class MaterialParser& mtlParser) override;
};

FIRERENDER_NAMESPACE_END;
