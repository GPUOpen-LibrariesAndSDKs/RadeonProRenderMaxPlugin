/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Radeon ProRender dedicated custom material plugin
*********************************************************************************************************************************/
#include "FireRenderNormalMtl.h"
#include "Resource.h"
#include "parser\MaterialParser.h"
#include "maxscript\mxsplugin\mxsPlugin.h"
#include "ParamBlock.h"
#include "PluginContext\PluginContext.h"

#include <emmintrin.h>
#include <functional>
#include <smmintrin.h> // for  _mm_packus_epi32

FIRERENDER_NAMESPACE_BEGIN;

IMPLEMENT_FRMTLCLASSDESC(NormalMtl)

FRMTLCLASSDESCNAME(NormalMtl) FRMTLCLASSNAME(NormalMtl)::ClassDescInstance;

// All parameters of the material plugin. See FIRE_MAX_PBDESC definition for notes on backwards compatibility
static ParamBlockDesc2 pbDesc(
	0, _T("NormalMtlPbdesc"), 0, &FRMTLCLASSNAME(NormalMtl)::ClassDescInstance, P_AUTO_CONSTRUCT + P_AUTO_UI + P_VERSION, FIRERENDERMTLVER_LATEST, 0,
    //rollout
	IDD_FIRERENDER_NORMALMTL, IDS_FR_MTL_NORMAL, 0, 0, NULL,

	FRNormalMtl_COLOR_TEXMAP, _T("Map"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRNormalMtl_TEXMAP_COLOR, p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_TEXMAP, PB_END,

	FRNormalMtl_STRENGTH, _T("strength"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f,
	p_range, -999.f, 999.f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_NORMAL_STRENGTH, IDC_NORMAL_STRENGTH_S, SPIN_AUTOSCALE, PB_END,

	FRNormalMtl_ISBUMP, _T("isBump"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_ISBUMP, PB_END,
		
	FRNormalMtl_ADDITIONALBUMP, _T("AdditionalBump"), TYPE_TEXMAP, 0, 0,
	p_subtexno, FRNormalMtl_TEXMAP_ADDITIONALBUMP, p_ui, TYPE_TEXMAPBUTTON, IDC_NORMAL_ADDITIONALBUMP_TEXMAP, PB_END,

	FRNormalMtl_SWAPRG, _T("swapRG"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_SWAPRG, PB_END,

	FRNormalMtl_INVERTR, _T("invertR"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_INVERTR, PB_END,

	FRNormalMtl_INVERTG, _T("invertG"), TYPE_BOOL, P_ANIMATABLE, 0,
	p_default, FALSE, p_ui, TYPE_SINGLECHEKBOX, IDC_NORMAL_INVERTG, PB_END,

	FRNormalMtl_BUMPSTRENGTH, _T("bumpStrength"), TYPE_FLOAT, P_ANIMATABLE, 0,
	p_default, 1.0f,
	p_range, -999.f, 999.f,
	p_ui, TYPE_SPINNER, EDITTYPE_FLOAT, IDC_BUMP_STRENGTH, IDC_BUMP_STRENGTH_S, SPIN_AUTOSCALE, PB_END,
		
    PB_END
    );

std::map<int, std::pair<ParamID, MCHAR*>> FRMTLCLASSNAME(NormalMtl)::TEXMAP_MAPPING = {
	{ FRNormalMtl_TEXMAP_COLOR,{ FRNormalMtl_COLOR_TEXMAP, _T("Map") } },
	{ FRNormalMtl_TEXMAP_ADDITIONALBUMP,{ FRNormalMtl_ADDITIONALBUMP, _T("Additional Bump") } }
};

FRMTLCLASSNAME(NormalMtl)::~FRMTLCLASSNAME(NormalMtl)()
{
}

namespace
{
frw::ValueNode findUV(frw::ValueNode map){
	if(!map)
		return frw::ValueNode();
	size_t num_params = 0;
	auto status = rprMaterialNodeGetInfo(frw::ValueNode(map).Handle(), RPR_MATERIAL_NODE_INPUT_COUNT, sizeof(size_t), &num_params, NULL);
	assert(status ==  RPR_SUCCESS);

	rpr_material_node uv = 0;

	auto handle  = map.Handle();
	for(int i=0;i!=num_params;++i ){
		size_t name = 0;
        rpr_int in_type;
        status = rprMaterialNodeGetInputInfo(handle, i, RPR_MATERIAL_NODE_INPUT_TYPE, sizeof(in_type), &in_type, nullptr);
		assert(status ==  RPR_SUCCESS);
        if (in_type == RPR_MATERIAL_NODE_INPUT_TYPE_NODE)
        {
            rpr_material_node in_handle = nullptr;
            status = rprMaterialNodeGetInputInfo(handle, i, RPR_MATERIAL_NODE_INPUT_VALUE, sizeof(in_handle), &in_handle, nullptr);
			assert(status ==  RPR_SUCCESS);
			if(in_handle) {
				auto in_node = map.FindRef<frw::ValueNode>(in_handle);

				auto status = rprMaterialNodeGetInputInfo(handle, i, RPR_MATERIAL_NODE_INPUT_NAME, sizeof(size_t), &name, NULL);
				assert(status ==  RPR_SUCCESS);
				if(RPR_MATERIAL_INPUT_UV==name){
					return in_node;
				}
				
				if(frw::ValueNode value = findUV(in_node)){
					return value;
				}
			}
		}
	}
	return frw::ValueNode();
}
}

// scout for a bitmap up the shader graph
//
Bitmap *FRMTLCLASSNAME(NormalMtl)::findBitmap(const TimeValue t, MtlBase *mat)
{
	if (auto map = dynamic_cast<BitmapTex*>(mat->GetInterface(BITMAPTEX_INTERFACE)))
	{
		if (auto bmp = map->GetBitmap(0))
			return bmp;
	}
	int npb = mat->NumParamBlocks();
	for (int j = 0; j < npb; j++)
	{
		if (auto pb = mat->GetParamBlock(j))
		{
			auto pbd = pb->GetDesc();
			for (USHORT i = 0; i < pbd->count; i++)
			{
				ParamID id = pbd->IndextoID(i);
				ParamDef &pdef = pbd->GetParamDef(id);
				switch (pdef.type)
				{
				case TYPE_MTL:
				{
					Mtl *v = 0;
					pb->GetValue(id, t, v, FOREVER);
					if (v)
					{
						Bitmap *res = findBitmap(t, v);
						if (res)
							return res;
					}
				} break;
				case TYPE_TEXMAP:
				{
					Texmap *v = 0;
					pb->GetValue(id, t, v, FOREVER);
					if (v)
					{
						Bitmap *res = findBitmap(t, v);
						if (res)
							return res;
					}
				} break;

				case TYPE_TEXMAP_TAB:
				{
					int count = pb->Count(id);
					for (int i = 0; i < count; i++)
					{
						Texmap *v = 0;
						pb->GetValue(id, t, v, FOREVER, i);
						if (v)
						{
							Bitmap *res = findBitmap(t, v);
							if (res)
								return res;
						}
					}
				} break;
				}
			}
		}
	}
	return NULL;
}

namespace {
	struct RgbByte
	{
		unsigned char r, g, b;

		RgbByte()
		{
		}

		RgbByte(unsigned char rr, unsigned char gg, unsigned char bb) :
			r(rr), g(gg), b(bb)
		{
		}
	};

	struct RgbByteF
	{
		float r, g, b;
		
		operator const float*() const { return &r; }
	};
}


inline BMM_Color_fl FRMTLCLASSNAME(NormalMtl)::SampleBitmap(Bitmap *bmp, float u, float v)
{
	while (u < 0.f) u += 1.f;
	while (v < 0.f) v += 1.f;
	while (u >= 1.f) u -= 1.f;
	while (v >= 1.f) v -= 1.f;

	int ww = bmp->Width();
	int hh = bmp->Height();

	float uu = u * float(ww);
	float vv = v * float(hh);

	float xint, yint, xfra, yfra;
	xfra = modf(uu, &xint);
	yfra = modf(vv, &yint);
	if (xfra > 0.5f && xint < ww - 1)
		xint += 1.f;
	if (yfra > 0.5f && yint < hh - 1)
		yint += 1.f;

	int _x = int(xint);
	int _y = int(yint);

	BMM_Color_fl pix;
	bmp->GetPixels(_x, _y, 1, &pix);
	return pix;
}

inline BMM_Color_fl FRMTLCLASSNAME(NormalMtl)::SampleBitmap(Bitmap *bmp, int u, int v)
{
	int ww = bmp->Width();
	int hh = bmp->Height();

	while (u < 0) u += ww;
	while (v < 0) v += hh;
	while (u >= ww) u -= ww;
	while (v >= hh) v -= hh;

	BMM_Color_fl pix;
	bmp->GetPixels(u, v, 1, &pix);
	return pix;
}

bool FRMTLCLASSNAME(NormalMtl)::IsGrayScale(Bitmap *bm)
{
	int bumpW = bm->Width();
	int bumpH = bm->Height();

	int rasters[4] = {
		bumpH - 1,
		bumpH >> 1,
		bumpH >> 2,
		bumpH >> 4
	};

	bool gray[4] = { true, true, true, true };

#pragma omp parallel for
	for (int i = 0; i < 4; i++)
	{
		std::vector<BMM_Color_fl> buffer(bumpW);
		bm->GetPixels(0, rasters[i], bumpW, &buffer[0]);
		for (int j = 0; j < bumpW; j++)
		{
			const BMM_Color_fl &pix = buffer[j];
			if ((abs(pix.r - pix.g) > 0.0000001f) || (abs(pix.r - pix.b) > 0.0000001f))
			{
				gray[i] = false;
				break;
			}
		}
	}

	return gray[0] && gray[1] && gray[2] && gray[3];
}

Bitmap *FRMTLCLASSNAME(NormalMtl)::BumpToNormalRPR(Bitmap *bumpBitmap, float strength)
{
	// if the main normal map is an actual normal map, we need to convert the additional bump
	// to a normal map

	float basemap_amp = -1.f;
	int bumpW = bumpBitmap->Width();
	int bumpH = bumpBitmap->Height();

	BitmapInfo bi;
	bi.SetType(BMM_TRUE_32);
	bi.SetFlags(MAP_HAS_ALPHA);
	bi.SetWidth((WORD)bumpW);
	bi.SetHeight((WORD)bumpH);
	bi.SetCustomFlag(0);
	Bitmap *bitmap = TheManager->Create(&bi);

	// bump to normal conversion
	//
#pragma omp parallel for
	for (int y = 0; y < bumpH; y++)
	{
		std::vector<BMM_Color_fl> buffer(bumpW);
		for (int x = 0; x < bumpW; x++)
		{
			
			const float tex00 = SampleBitmap(bumpBitmap, x - 1, y - 1).r;
			const float tex10 = SampleBitmap(bumpBitmap, x, y - 1).r;
			const float tex20 = SampleBitmap(bumpBitmap, x + 1, y - 1).r;

			const float tex01 = SampleBitmap(bumpBitmap, x - 1, y).r;
			const float tex21 = SampleBitmap(bumpBitmap, x + 1, y).r;

			const float tex02 = SampleBitmap(bumpBitmap, x - 1, y + 1).r;
			const float tex12 = SampleBitmap(bumpBitmap, x, y + 1).r;
			const float tex22 = SampleBitmap(bumpBitmap, x + 1, y + 1).r;
			
			const float Gx = tex00 - tex20 + 2.0f * tex01 - 2.0f * tex21 + tex02 - tex22;
			const float Gy = tex00 + 2.0f * tex10 + tex20 - tex02 - 2.0f * tex12 - tex22;

			Point3 n(Gx, Gy, 1.f / strength);
			n = (n * 0.5f) + 0.5f;
			buffer[x] = BMM_Color_fl(std::min(std::max(n.x, 0.f), 1.f),
				std::min(std::max(n.y, 0.f), 1.f),
				std::min(std::max(n.z, 0.f), 1.f));
		}
		bitmap->PutPixels(0, y, bumpW, &buffer[0]);
	}
	return bitmap;
}

Bitmap *FRMTLCLASSNAME(NormalMtl)::ProcessBitmap(Bitmap *bumpBitmap, BOOL swapRG, BOOL flipR, BOOL flipG)
{
	int bumpW = bumpBitmap->Width();
	int bumpH = bumpBitmap->Height();

	BitmapInfo bi;
	bi.SetType(BMM_TRUE_32);
	bi.SetFlags(MAP_HAS_ALPHA);
	bi.SetWidth((WORD)bumpW);
	bi.SetHeight((WORD)bumpH);
	bi.SetCustomFlag(0);
	Bitmap *bitmap = TheManager->Create(&bi);
	
#pragma omp parallel for
	for (int y = 0; y < bumpH; y++)
	{
		std::vector<BMM_Color_fl> buffer(bumpW);
		bumpBitmap->GetPixels(0, y, bumpW, &buffer[0]);
		for (int x = 0; x < bumpW; x++)
		{
			const BMM_Color_fl &hc = buffer[x];
			Point3 n(hc.r, hc.g, hc.b);
			n = n * 2.f - 1.f;

			if (swapRG)
				std::swap(n.x, n.y);
			if (flipR)
				n.x *= -1.f;
			if (flipG)
				n.y *= -1.f;

			n = n * 0.5f + 0.5f;
						
			buffer[x] = BMM_Color_fl(std::min(std::max(n.x, 0.f), 1.f),
				std::min(std::max(n.y, 0.f), 1.f),
				std::min(std::max(n.z, 0.f), 1.f));
		}
		bitmap->PutPixels(0, y, bumpW, &buffer[0]);
	}
	return bitmap;
}

frw::Value FRMTLCLASSNAME(NormalMtl)::translateGenericBump(const TimeValue t, Texmap *bumpMap, const float& strength, MaterialParser& mtlParser)
{
	if (bumpMap)
	{
		if (bumpMap->ClassID() == FIRERENDER_NORMALMTL_CID)
		{
			return dynamic_cast<FRMTLCLASSNAME(NormalMtl)*>(bumpMap)->getShader(t, mtlParser);
		}

		// COMPUTE HASH
		BOOL isBump = FALSE;
		BOOL swapRG = FALSE;
		BOOL invertR = FALSE;
		BOOL invertG = FALSE;
		HashValue hash;
		hash << bumpMap;
		hash << mtlParser.getMaterialHash(bumpMap);
		hash << swapRG;
		hash << invertR;
		hash << invertG;

		auto normalImage = mtlParser.getScope().GetImage(hash);

		if (!normalImage)
		{
			// get the main normal map
			bool deleteNormalBitmapAfterwards = false; // if true, the bitmap was baked, so we need to destroy it when done
			Bitmap *pNormalBitmap = 0;
			// fetch or bake the normal map (in case it's procedural or comes through a procedural node)
			pNormalBitmap = createImageFromMap(t, bumpMap, mtlParser, deleteNormalBitmapAfterwards); 

			if (pNormalBitmap && IsGrayScale(pNormalBitmap)) // If normal bitmap is grayscale then it's a bump map and must be converted to normal map
			{
				Bitmap *temp = BumpToNormalRPR(pNormalBitmap);

				if (deleteNormalBitmapAfterwards) 
				{
					pNormalBitmap->DeleteThis();
				}

				pNormalBitmap = temp;
				deleteNormalBitmapAfterwards = true;
			}

			rpr_image_desc imgDesc = {};
			if (pNormalBitmap)
				normalImage = bitmap2image(pNormalBitmap, mtlParser);
			// cleanup
			if (deleteNormalBitmapAfterwards)
				pNormalBitmap->DeleteThis();
			if (normalImage)
				mtlParser.getScope().SetImage(hash, normalImage);
		}

		// create the actual shader node
		//
		if (normalImage)
		{
			if (normalImage.IsGrayScale())
			{
				frw::BumpMapNode node(mtlParser.materialSystem);
				frw::ImageNode imgNode(mtlParser.materialSystem);

				imgNode.SetMap(normalImage);
				node.SetMap(imgNode);
				node.SetValue("bumpscale", frw::Value(strength));

				return node;
			}
			else
			{
			frw::ValueNode res;
			frw::NormalMapNode node(mtlParser.materialSystem);
			frw::ImageNode imgNode(mtlParser.materialSystem);

			imgNode.SetMap(normalImage);
			node.SetMap(imgNode);
			node.SetValue("bumpscale", frw::Value(strength));

			return node;
		}
	}
	}
	return mtlParser.materialSystem.ValueLookupN();
}

frw::Image FRMTLCLASSNAME(NormalMtl)::bitmap2image(Bitmap *bmp, MaterialParser& mtlParser)
{
	std::function<__m128i(__m128i, __m128i)> mm_packus_epi32_impl = [](__m128i a, __m128i b) {
		return _mm_packus_epi32(a, b);
	};

	if ( !PluginContext::instance().HasSSE41() )
	{
		// use SSE2 to perform calculations
		mm_packus_epi32_impl = [](__m128i a, __m128i b) {
			__m128i t0 = _mm_slli_epi32(a, 16);
			t0 = _mm_srai_epi32(t0, 16);
			__m128i t1 = _mm_slli_epi32(b, 16);
			t1 = _mm_srai_epi32(t1, 16);
			return _mm_packs_epi32(t0, t1);
		};
	}

	rpr_image_desc imgDesc = {};
	imgDesc.image_width = bmp->Width();
	imgDesc.image_height = bmp->Height();
	frw::Image img;

	int type;
	void* bmstorage = bmp->GetStoragePtr(&type);

	if (bmstorage && (type == BMM_TRUE_24))
	{
		img = frw::Image(mtlParser.getScope(), { 3, RPR_COMPONENT_TYPE_UINT8 }, imgDesc, bmstorage);
	}
	else
	{
		RgbByte* buffer8 = new RgbByte[imgDesc.image_width * imgDesc.image_height + 1];

		__m128 va = _mm_set1_ps(255.f);

#pragma omp parallel for
		for (int y = 0; y < imgDesc.image_height; y++)
		{
			std::vector<BMM_Color_fl> buffer(imgDesc.image_width);

			bmp->GetPixels(0, y, imgDesc.image_width, &buffer[0]);
			int raster = y * imgDesc.image_width;

			for (int x = 0; x < imgDesc.image_width; x++)
			{
				__m128 vb = _mm_load_ps(buffer[x]); // load RGBA in
				__m128 vc = _mm_mul_ps(va, vb); // multiply by 255
				__m128i y = _mm_cvtps_epi32(vc); // convert to integers

				y = mm_packus_epi32_impl(y, y); // pack to 16 bits
				y = _mm_packus_epi16(y, y); // pack to 8 bits

				*(int*)(&buffer8[raster + x]) = _mm_cvtsi128_si32(y); // store lower 32 bits
			}
		}

		img = frw::Image(mtlParser.getScope(), { 3, RPR_COMPONENT_TYPE_UINT8 }, imgDesc, buffer8);

		delete[] buffer8;
	}

	return img;
}

frw::Value FRMTLCLASSNAME(NormalMtl)::getShader(const TimeValue t, MaterialParser& mtlParser)
{
	auto ms = mtlParser.materialSystem;

	auto texmap = GetFromPb<Texmap*>(pblock, FRNormalMtl_COLOR_TEXMAP);

	// is there an additional bump?
	auto bumpTexmap = GetFromPb<Texmap*>(pblock, FRNormalMtl_ADDITIONALBUMP);

	if (texmap || bumpTexmap)
	{
		float weight = GetFromPb<float>(pblock, FRNormalMtl_STRENGTH); // overall bump weight
		float bumpStrength = GetFromPb<float>(pblock, FRNormalMtl_BUMPSTRENGTH); // strength of additional bump
		
		// is isBump is true, we are hinted that the inoput map is not a normal map.
		// this is useful when a pseudo bump map has colors but should be treates as a poor man's bump
		BOOL isBump = GetFromPb<BOOL>(pblock, FRNormalMtl_ISBUMP);
		BOOL swapRG = GetFromPb<BOOL>(pblock, FRNormalMtl_SWAPRG);
		BOOL invertR = GetFromPb<BOOL>(pblock, FRNormalMtl_INVERTR);
		BOOL invertG = GetFromPb<BOOL>(pblock, FRNormalMtl_INVERTG);
		
		// COMPUTE HASH
		HashValue hash;
		hash << texmap;
		hash << mtlParser.getMaterialHash(texmap);
		hash << swapRG;
		hash << invertR;
		hash << invertG;
		if (bumpTexmap)
	{
			hash << bumpTexmap;
			hash << mtlParser.getMaterialHash(bumpTexmap);
			hash << bumpStrength;
		}
		
		// check: do we have this bump map in cache (reflecting the current settings?)
		auto normalImage = mtlParser.getScope().GetImage(hash);

		bool requireGrayScaleBump = false;

		// if not, rebuild it
		if (!normalImage)
		{
			// get the main normal map
			Bitmap *pNormalBitmap = 0;
			bool deleteNormalBitmapAfterwards = false; // if true, the bitmap was baked, so we need to destroy it when done
			Bitmap *pBumpBitmap = 0;
			bool deleteBumpBitmapAfterwards = false;
			
			// fetch or bake the normal map (in case it's procedural or comes through a procedural node)
			if (texmap)
				pNormalBitmap = createImageFromMap(t, texmap, mtlParser, deleteNormalBitmapAfterwards);

			// fast lane cases : only one normal or bump is being used
			if (pNormalBitmap && !bumpTexmap && !swapRG && !invertR && !invertG)
			{
				normalImage = bitmap2image(pNormalBitmap, mtlParser);
				if (isBump || IsGrayScale(pNormalBitmap))
					requireGrayScaleBump = true;
			}
			else if (!pNormalBitmap && bumpTexmap)
			{
				bool deleteBumpBitmapAfterwards = false;
				if (bumpTexmap && bumpStrength != 0.f)
				{
					// fetch or bake the bump map
					pBumpBitmap = createImageFromMap(t, bumpTexmap, mtlParser, deleteBumpBitmapAfterwards);
					if (pBumpBitmap)
					{
						normalImage = bitmap2image(pBumpBitmap, mtlParser);
						if (IsGrayScale(pBumpBitmap))
							requireGrayScaleBump = true;
					}
				}
			}
			else
			{
			if (pNormalBitmap && (isBump || IsGrayScale(pNormalBitmap)))
			{
				Bitmap *temp = BumpToNormalRPR(pNormalBitmap);
				if (deleteNormalBitmapAfterwards)
					pNormalBitmap->DeleteThis();
				pNormalBitmap = temp;
				deleteNormalBitmapAfterwards = true;
			}

			if (swapRG || invertR || invertG)
			{
				Bitmap *temp = ProcessBitmap(pNormalBitmap, swapRG, invertR, invertG);
				if (deleteNormalBitmapAfterwards)
					pNormalBitmap->DeleteThis();
				pNormalBitmap = temp;
				deleteNormalBitmapAfterwards = true;
			}

			// get the additional bump map
			if (bumpTexmap && bumpStrength != 0.f)
			{
				// fetch or bake the bump map
				pBumpBitmap = createImageFromMap(t, bumpTexmap, mtlParser, deleteBumpBitmapAfterwards);

				// if the main normal map is an actual normal map, we need to convert the additional bump
				// to a normal map
				if (pBumpBitmap && IsGrayScale(pBumpBitmap))
				{
					// convert the bump map to normal map
						Bitmap *temp = BumpToNormalRPR(pBumpBitmap);
					if (deleteBumpBitmapAfterwards)
						pBumpBitmap->DeleteThis();
					pBumpBitmap = temp;
					deleteBumpBitmapAfterwards = true;
				}
			}

			// At this point we may have one, both or neither of:
			// pBumpBitmap - a bitmap that contains the additional normal map
			//						if NULL, there is no additional bump bitmap
			//
			// pNormalBitmap - a bitmap that contains the main normal map
			//						if NULL, there is no normal map
			//
			// actions:
			// if both pNormalBitmap and pBumpBitmap are not NULL, they are mixed into the destination image
			// else if pNormalBitmap is not NULL, it is copied into the destination image
			// else if pBumpBitmap is not NULL, it is copied into the destination image
			if (pNormalBitmap && pBumpBitmap)
			{
				rpr_image_desc imgDesc = {};
				imgDesc.image_width = pNormalBitmap->Width();
				imgDesc.image_height = pNormalBitmap->Height();
					RgbByte *buffer8 = new RgbByte[imgDesc.image_width * imgDesc.image_height + 1];

				// ADD NORMALS
				float n_sx = 1.f / float(pNormalBitmap->Width());
				float n_sy = 1.f / float(pNormalBitmap->Height());
				float b_sx = 1.f / float(pBumpBitmap->Width());
				float b_sy = 1.f / float(pBumpBitmap->Height());
				float i_bumpStrength = 1.f / bumpStrength;
					__m128 va = _mm_set1_ps(255.f);
			#pragma omp parallel for
				for (int i = 0; i < pNormalBitmap->Height(); i++)
				{
					int raster = i * imgDesc.image_width;
					for (int j = 0; j < pNormalBitmap->Width(); j++)
					{
						float u = float(j) * n_sx;
						float v = float(i) * n_sy;
						auto n_pix = SampleBitmap(pNormalBitmap, u, v);
						auto b_pix = SampleBitmap(pBumpBitmap, u, v);
						// detail oriented blending
						{
							Point3 t = Point3(n_pix.r, n_pix.g, n_pix.b) * Point3(2, 2, 2) + Point3(-1, -1, 0);
							Point3 u = Point3(b_pix.r, b_pix.g, b_pix.b) * Point3(-2, -2, 2) + Point3(1, 1, -1);
							u.z *= abs(i_bumpStrength);
							float dot = DotProd(t, u);
							Point3 r = t * dot - u * t.z;
							r = r.Normalize();
							r = r * 0.5f + 0.5f;
							buffer8[raster + j] = RgbByte(
								unsigned char(r.x * 255.f),
								unsigned char(r.y * 255.f),
								unsigned char(r.z * 255.f));
						}
					}
				}

				if (buffer8)
					normalImage = frw::Image(mtlParser.getScope(), { 3, RPR_COMPONENT_TYPE_UINT8 }, imgDesc, buffer8);

					delete[] buffer8;
			}
			else if (pBumpBitmap)
			{
				normalImage = bitmap2image(pBumpBitmap, mtlParser);
			}
			else if (pNormalBitmap)
			{
				normalImage = bitmap2image(pNormalBitmap, mtlParser);
			}
			}
						
			// cleanup
			if (pBumpBitmap && deleteBumpBitmapAfterwards)
				pBumpBitmap->DeleteThis();
			if (deleteNormalBitmapAfterwards)
				pNormalBitmap->DeleteThis();

			if (normalImage)
				mtlParser.getScope().SetImage(hash, normalImage);
		}
		else
			requireGrayScaleBump = isBump;

		// create the actual shader node
		//
		frw::ValueNode res;
		if (requireGrayScaleBump)
		{
			frw::ImageNode imgNode(ms);
			imgNode.SetMap(normalImage);

			frw::BumpMapNode node(ms);
			node.SetMap(imgNode);
			res = node;
		}
		else
		{
			frw::ImageNode imgNode(ms);
			imgNode.SetMap(normalImage);

			frw::NormalMapNode node(ms);
			node.SetMap(imgNode);
			res = node;
		}
		
		res.SetValue("bumpscale", frw::Value(weight));
		
		return res;
	}
	return ms.ValueLookupN();
}

void FRMTLCLASSNAME(NormalMtl)::Update(TimeValue t, Interval& valid) 
{
    for (int i = 0; i < NumSubTexmaps(); ++i)
	{
        // we are required to recursively call Update on all our submaps
        Texmap* map = GetSubTexmap(i);
        if (map != NULL) 
            map->Update(t, valid);
        }
    this->pblock->GetValidity(t, valid);
}

Bitmap *FRMTLCLASSNAME(NormalMtl)::createImageFromMap(const TimeValue t, Texmap* input, MaterialParser& mtlParser, bool &deleteAfterwards)
{
	debugPrint("NormalMtl::createImageFromMap\n");
	FASSERT(input);

	deleteAfterwards = true;

	BOOL res;
	int textureResolution = 512;
	res = mtlParser.GetPBlock()->GetValue(PARAM_TEXMAP_RESOLUTION, t, textureResolution, Interval());
	FASSERT(res);

	//some basic defaults, but if possible derive optimal resolution
	int width = textureResolution;
	int height = textureResolution;

	bool bitmapFound = false;

	// special case, a bitmap with no transformation applied
	if (auto map = dynamic_cast<BitmapTex*>(input->GetInterface(BITMAPTEX_INTERFACE)))
	{
		if (auto bmp = map->GetBitmap(0))
		{
			width = bmp->Width();
			height = bmp->Height();
			bitmapFound = true;

			Matrix3 tm;
			input->GetUVTransform(tm);
			if (IsIdentity(tm))
			{
				deleteAfterwards = false;
				return bmp;
			}
		}
	}

	// attempts to find a more reasonable size for the baked map
	if (!bitmapFound)
	{
		Bitmap *bmp = findBitmap(t, input);
		if (bmp)
		{
			width = bmp->Width();
			height = bmp->Height();
		}
    }

	// the following fixes the issue with texture baking occasionally hanging or producing wrong outputs
	{
		BitmapInfo bi;
		bi.SetWidth(8);
		bi.SetHeight(8);
		bi.SetType(BMM_FLOAT_RGBA_32);
		bi.SetFlags(MAP_HAS_ALPHA);
		bi.SetCustomFlag(0);

		Bitmap* bmp = TheManager->Create(&bi);

		input->RenderBitmap(t, bmp);

		bmp->DeleteThis();
	}

	//creating BitmapInfo 
	BitmapInfo bi;
	bi.SetType(BMM_TRUE_32);
	bi.SetFlags(MAP_HAS_ALPHA);
	bi.SetWidth((WORD)width);
	bi.SetHeight((WORD)height);
	bi.SetCustomFlag(0);
	Bitmap *bitmap = TheManager->Create(&bi);

	// To sample the map we calculate UVW coords and store them in UvwContext, then call Texmap::EvalColor. We use OMP to 
	// parallelize this.
	UvwContext shadeContext;

	const float iw = 1.f / float(width);
	const float ih = 1.f / float(height);

	__m128 vzero = _mm_set1_ps(0.f);
	__m128 vone = _mm_set1_ps(1.f);

#pragma omp parallel for private(shadeContext)
	for (int y = 0; y < height; y++)
	{
		std::vector<BMM_Color_fl> raster(width);
		for (int x = 0; x < width; x++)
		{
			Point3 uvw(float(x) * iw, 1.f - float(y) * ih, 0.f);
			shadeContext.setup(uvw, t, width, height);
			__declspec(align(16)) auto color = input->EvalColor(shadeContext);
			// if (clamp) - always need to clamp these ones
			__m128 v = _mm_load_ps(color);
			v = _mm_max_ps(v, vzero);
			v = _mm_min_ps(v, vone);
			_mm_store_ps(raster[x], v);
		}
		bitmap->PutPixels(0, y, width, &raster[0]);
	}
		
	return bitmap;
}

FIRERENDER_NAMESPACE_END;
