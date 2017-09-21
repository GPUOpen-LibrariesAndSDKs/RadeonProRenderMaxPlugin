/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Converting all material types we support from 3ds Max to Radeon ProRender format
*********************************************************************************************************************************/

#pragma once
#include "parser/MaterialParser.h"
#include <maxscript/mxsPlugin/mxsPlugin.h>
#include <xref/iXrefMaterial.h>
#include <shaders.h>
#include "CoronaDeclarations.h"
#include "3dsMaxDeclarations.h"
#include "ParamBlock.h"
#include "ScopeManager.h"
#include "FireRenderDiffuseMtl.h"
#include "FireRenderBlendMtl.h"
#include "FireRenderAddMtl.h"
#include "FireRenderMicrofacetMtl.h"
#include "FireRenderReflectionMtl.h"
#include "FireRenderArithmMtl.h"
#include "FireRenderInputLUMtl.h"
#include "FireRenderBlendValueMtl.h"
#include "FireRenderRefractionMtl.h"
#include "FireRenderMFRefractionMtl.h"
#include "FireRenderTransparentMtl.h"
#include "FireRenderWardMtl.h"
#include "FireRenderEmissiveMtl.h"
#include "FireRenderFresnelMtl.h"
#include "FireRenderStandardMtl.h"
#include "FireRenderColorMtl.h"
#include "FireRenderAvgMtl.h"
#include "FireRenderOrenNayarMtl.h"
#include "FireRenderNormalMtl.h"
#include "FireRenderMaterialMtl.h"
#include "FireRenderDiffuseRefractionMtl.h"
#include "FireRenderFresnelSchlickMtl.h"
#include "FireRenderUberMtl.h"
#include "FireRenderUberMtlv2.h"
#include "FireRenderVolumeMtl.h"
#include "FireRenderPbrMtl.h"
#include "utils/KelvinToColor.h"

#include <icurvctl.h>
#include <max.h>

FIRERENDER_NAMESPACE_BEGIN;

#define CMP_SIZE 64 // string comparisons; shouldn'mT go past this size

namespace
{
	class Float16
	{
		union _float16
		{
			short v;
			struct
			{
				// type determines alignment!
				uint16_t m : 10;
				uint16_t e : 5;
				uint16_t s : 1;
			};
		} f16;

		union _float32
		{
			float v;
			struct
			{
				uint32_t m : 23;
				uint32_t e : 8;
				uint32_t s : 1;
			};
		};

	public:
		Float16()
		{
			f16.v = 0;
		}

		Float16(float v)
		{
			_float32 f32 = { v }; 

			f16.s = f32.s;
			f16.e = std::max(-15, std::min(16, int(f32.e - 127))) + 15;
			f16.m = f32.m >> 13;
		}

		operator float() const
		{
			_float32 f32 = {};
			f32.s = f16.s;
			f32.e = (f16.e - 15) + 127; // safe in this direction
			f32.m = uint32_t(f16.m) << 13;
			return f32.v;
		}
	};

	struct RgbFloat16
	{
		Float16 r, g, b;
	};

	struct RgbFloat32
	{
		float r, g, b;
	};

	struct RgbByte
	{
		unsigned char r, g, b;
	};
}

frw::Image MaterialParser::createImageFromMap(Texmap* input, const int flags, bool force)
{
	debugPrint("createImageFromMap\n");
	FASSERT(input);

	frw::Image image;
	HashValue key;
	
	if (!force)
	{
		key << input << getMaterialHash(input);
		image = mScope.GetImage(key);
		if (image)
			return image;
	}

	BOOL res;
	int textureResolution = 512;
	res = mPblock->GetValue(PARAM_TEXMAP_RESOLUTION, mT, textureResolution, Interval());
	FASSERT(res);

	int width = textureResolution;
	int height = textureResolution;

	// special case, a bitmap with no transformation applied
	if (auto map = dynamic_cast<BitmapTex*>(input->GetInterface(BITMAPTEX_INTERFACE)))
	{
		if (auto bmp = map->GetBitmap(0))
		{
			width = bmp->Width();
			height = bmp->Height();

			Matrix3 tm;
			input->GetUVTransform(tm);
			if (IsIdentity(tm))
				return createImage(bmp, HashValue() << input << getMaterialHash(input), flags, map->GetMapName());
		}
	}

	// the following fixes a MAX's issue with texture baking occasionally hanging or producing wrong outputs
	{
		BitmapInfo bi;
		bi.SetWidth(8);
		bi.SetHeight(8);
		bi.SetType(BMM_FLOAT_RGBA_32);
		bi.SetFlags(MAP_HAS_ALPHA);
		bi.SetCustomFlag(0);

		Bitmap* bmp = TheManager->Create(&bi);

		input->RenderBitmap(mT, bmp);

		bmp->DeleteThis();
	}
	
	if (!image)
	{
		if (flags & MAP_FLAG_WANTSHDR)
		{
			rpr_image_desc imgDesc = {};
			imgDesc.image_width = width;
			imgDesc.image_height = height;

			RgbFloat32 *buffer = new RgbFloat32[imgDesc.image_width*imgDesc.image_height];

			// To sample the map we calculate UVW coords and store them in UvwContext, then call Texmap::EvalColor.
			// We use OMP to parallelize this.
			UvwContext shadeContext;

			float iw = 1.f / float(imgDesc.image_width);
			float ih = 1.f / float(imgDesc.image_height);

#pragma omp parallel for private(shadeContext)
			for (int x = 0; x < imgDesc.image_width; x++)
			{
				for (int y = 0; y < imgDesc.image_height; y++)
				{
					Point3 uvw(float(x) * iw, 1.f - float(y) * ih, 0.f);
					shadeContext.setup(uvw, this->mT, imgDesc.image_width, imgDesc.image_height);
					const int index = y*imgDesc.image_width + x;
					auto color = input->EvalColor(shadeContext);

					if (flags & MAP_FLAG_CLAMP)
					{
						color.r = std::min(std::max(color.r, 0.f), 1.f);
						color.g = std::min(std::max(color.g, 0.f), 1.f);
						color.b = std::min(std::max(color.b, 0.f), 1.f);
					}

					FASSERT(IsReal(color.r) && IsReal(color.g) && IsReal(color.b));

					buffer[index].r = color.r;
					buffer[index].g = color.g;
					buffer[index].b = color.b;
				}
			}

			image = frw::Image(mScope, { 3, RPR_COMPONENT_TYPE_FLOAT32 }, imgDesc, buffer);
			
			delete[] buffer;
		}
		else
		{
			BMM_Color_24 *buffer = new BMM_Color_24[width * height];

			// To sample the map we calculate UVW coords and store them in UvwContext, then call Texmap::EvalColor. We use OMP to 
			// parallelize this.
			UvwContext shadeContext;

			const float iw = 1.f / float(width);
			const float ih = 1.f / float(height);

#pragma omp parallel for private(shadeContext)
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < width; x++)
				{
					Point3 uvw(float(x) * iw, 1.f - float(y) * ih, 0.f);
					shadeContext.setup(uvw, this->mT, width, height);

					auto color = input->EvalColor(shadeContext);

					// always need to clamp these ones
					color.r = std::min(std::max(color.r, 0.f), 1.f);
					color.g = std::min(std::max(color.g, 0.f), 1.f);
					color.b = std::min(std::max(color.b, 0.f), 1.f);

					FASSERT(IsReal(color.r) && IsReal(color.g) && IsReal(color.b));
					buffer[y * width + x] = BMM_Color_24(color);
				}
			}

			rpr_image_desc imgDesc = {};
			imgDesc.image_width = width;
			imgDesc.image_height = height;
			image = frw::Image(mScope, { 3, RPR_COMPONENT_TYPE_UINT8 }, imgDesc, buffer);

			delete[] buffer;
		}
		
		if (!force)
			mScope.SetImage(key, image);
	}

	return image;
}

frw::Image MaterialParser::createImage(Bitmap* bitmap, const HashValue &kkey, const int flags, std::wstring name)
{
	if (!bitmap)
		return frw::Image();

	HashValue key = kkey;
	key << getBitmapHash(bitmap) << flags;

	auto image = mScope.GetImage(key);

	bool floatingPoint = false;

	bool resize = false;
	int maxres = 512;

	if (!image)
	{
		floatingPoint = bitmap->IsHighDynamicRange();

		rpr_image_desc imgDesc = {};

		imgDesc.image_width = bitmap->Width();
		imgDesc.image_height = bitmap->Height();

		if (floatingPoint)
		{
			size_t bufsize = imgDesc.image_width * imgDesc.image_height;
			BMM_Color_fl *buffer = 0;
			RgbFloat32 *buffer32 = 0;

			try
			{
				buffer = new BMM_Color_fl[imgDesc.image_width];
				buffer32 = new RgbFloat32[bufsize];
			}
			catch (const std::bad_alloc& e)
			{
				MessageBox(GetCOREInterface()->GetMAXHWnd(), L"Out of memory error.\nThis scene and its textures requires more memory than is available.\nTo render you will need to upgrade your hardware", L"Radeon ProRender", MB_OK | MB_ICONERROR);
				mScope.SetImage(key, image);
				return image;
			}

			for (int y = 0; y < imgDesc.image_height; y++)
			{
				if (flags & MAP_FLAG_NOGAMMA)
					bitmap->GetPixels(0, y, imgDesc.image_width, buffer);
				else
					bitmap->GetLinearPixels(0, y, imgDesc.image_width, buffer);
				int raster = y * imgDesc.image_width;
				for (int x = 0; x < imgDesc.image_width; x++)
				{
					const BMM_Color_fl *src = buffer + x;
					RgbFloat32 *dest = buffer32 + raster + x;
					memcpy(dest, src, sizeof(float) * 3);
				}
			}

			image = frw::Image(mScope, { 3, RPR_COMPONENT_TYPE_FLOAT32 }, imgDesc, buffer32);
			mScope.SetImage(key, image);

			delete[] buffer32;
			delete[] buffer;
		}
		else
		{
			int bmWidth = imgDesc.image_width;
			int bmHeight = imgDesc.image_height;
			Bitmap *bm = bitmap;
			Bitmap* downsampled = 0;
			BitmapInfo bds;
			
			if (resize && (imgDesc.image_width > maxres || imgDesc.image_height > maxres))
			{
				if (imgDesc.image_width > imgDesc.image_height)
				{
					bmWidth = maxres;
					bmHeight = float(bmWidth) * float(imgDesc.image_height / imgDesc.image_width);
				}
				else
				{
					bmHeight = maxres;
					bmWidth = float(bmHeight) * float(imgDesc.image_width / imgDesc.image_height);
				}
				
				bds.SetWidth(bmWidth);
				bds.SetHeight(bmHeight);
				bds.SetType(BMM_TRUE_32);
				bds.SetCustomFlag(0);
				downsampled = TheManager->Create(&bds);
				int res = bitmap->CopyImage(downsampled, COPY_IMAGE_RESIZE_HI_QUALITY, BMM_Color_fl(0), &bds);
				FASSERT(res);
				bm = downsampled;
				imgDesc.image_width = bmWidth;
				imgDesc.image_height = bmHeight;
			}
			else if (flags & MAP_FLAG_NOGAMMA)
			{
				int type;
				void *bmstorage = bm->GetStoragePtr(&type);
				if (bmstorage && (type == BMM_TRUE_24))
				{
					frw::Image img = frw::Image(mScope, { 3, RPR_COMPONENT_TYPE_UINT8 }, imgDesc, bmstorage);
					std::string name_s(name.begin(), name.end());
					img.SetName(name_s.c_str());
					return img;
				}
			}
			
			BMM_Color_fl *buffer = 0;
			RgbByte *buffer8 = 0;

			try
			{
				buffer = new BMM_Color_fl[bmWidth];
				buffer8 = new RgbByte[bmWidth * bmHeight];
			}
			catch (const std::bad_alloc& e)
			{
				MessageBox(GetCOREInterface()->GetMAXHWnd(), L"Out of memory error.\nThis scene and its textures requires more memory than is available.\nTo render you will need to upgrade your hardware", L"Radeon ProRender", MB_OK | MB_ICONERROR);
				mScope.SetImage(key, image);
				return image;
			}
						
			for (int y = 0; y < bmHeight; y++)
			{
				if (flags & MAP_FLAG_NOGAMMA)
					bm->GetPixels(0, y, bmWidth, buffer);
				else
					bm->GetLinearPixels(0, y, bmWidth, buffer);
				int raster = y * bmWidth;
				for (int x = 0; x < bmWidth; x++)
				{
					const BMM_Color_fl &src = buffer[x];
					RgbByte &dest = buffer8[raster + x];
					dest.r = unsigned char(src.r * 255.f);
					dest.g = unsigned char(src.g * 255.f);
					dest.b = unsigned char(src.b * 255.f);
				}
			}

			image = frw::Image(mScope, { 3, RPR_COMPONENT_TYPE_UINT8 }, imgDesc, buffer8);
			mScope.SetImage(key, image);

			delete[] buffer8;
			delete[] buffer;

			if (downsampled)
				downsampled->DeleteThis();
		}
	}

	std::string name_s(name.begin(), name.end());
	image.SetName(name_s.c_str());

	return image;
}

HashValue MaterialParser::getBitmapHash(Bitmap *bm)
{
	HashValue hash;
	if (!bm)
		return hash;
	hash << bm->Flags();
	hash << bm->Width();
	hash << bm->Height();
	hash << bm->Gamma();
	return hash;
}

HashValue MaterialParser::getMaterialHash(MtlBase *mat, bool bReloadMaterial /*= true*/)
{
	std::map<Animatable*, HashValue> hash_visited;
	return GetHashValue(mat, mT, hash_visited, syncTimestamp, bReloadMaterial);
}

HashValue MaterialParser::GetHashValue(Animatable* mat, TimeValue mT, std::map<Animatable*, HashValue> &hash_visited, DWORD syncTimestamp, bool bReloadMaterial /*= true*/)
{
	HashValue hash;
	if (!mat)
		return hash;

	std::map<Animatable*, HashValue>::iterator hh = hash_visited.find(mat);
	if (hh != hash_visited.end())
		return hh->second;

	MSTR className;
	mat->GetClassName(className);

	int npb = mat->NumParamBlocks();
	hash << mat << npb;

	// some materials has no param block(Tiles)
	// so we mix current time to it to always invalidate
	// used common syncTimestamp so that for one Synchronize call 
	// hash would be still the same
	if(!npb)
		hash << syncTimestamp;

	// bReloadMaterial must modify hash it shouldn'mT depend on syncTimestamp
	if(bReloadMaterial){
		hash << syncTimestamp;
	}

	for (int j = 0; j < npb; j++)
	{
		if (auto pb = mat->GetParamBlock(j))
		{
			auto pbd = pb->GetDesc();
			hash << pb << pbd->count;

			for (USHORT i = 0; i < pbd->count; i++)
			{
				ParamID id = pbd->IndextoID(i);
				ParamDef &pdef = pbd->GetParamDef(id);
				hash << id << pdef.type;

				float v = 0;
				if (pb->GetValue(id, mT, v, FOREVER))
					hash << v;
				else
				{
					int v = 0;
					if (pb->GetValue(id, mT, v, FOREVER))
						hash << v;
					else
					{
						const MCHAR * v = 0;
						if (pb->GetValue(id, mT, v, FOREVER))
							hash << v;
						else
						{
							ReferenceTarget* v = 0;
							if (pb->GetValue(id, mT, v, FOREVER))
							{
								hash << GetHashValue(v, mT, hash_visited, syncTimestamp, bReloadMaterial);
							}
							else
							{
								Point3 v;
								if (pb->GetValue(id, mT, v, FOREVER))
									hash << v;
								else
								{
									Point4 v;
									if (pb->GetValue(id, mT, v, FOREVER))
										hash << v;
									else
									{
										PBBitmap* v = 0;
										if (pb->GetValue(id, mT, v, FOREVER))
											hash << v;
										else
										{
											Matrix3 v;
											if (pb->GetValue(id, mT, v, FOREVER))
												hash << v;
											else
											{
												Color v;
												if (pb->GetValue(id, mT, v, FOREVER))
													hash << v;
												else
												{
												// DebugPrint("Unknown param type\n");
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	

	if (auto uvGen = dynamic_cast<UVGen*>(mat))
	{
		if (auto gen = dynamic_cast<StdUVGen*>(uvGen))
			hash << gen->GetCoordMapping(0);

		hash << uvGen->GetUVWSource();
		Matrix3 tm;
		uvGen->GetUVTransform(tm);
		hash << tm;
	}
	else if (auto gen = dynamic_cast<StdXYZGen*>(mat))
	{
		hash << gen->GetCoordSystem();
		hash << gen->GetScl(0, mT) << gen->GetScl(1, mT) << gen->GetScl(2, mT);
		hash << gen->GetAng(0, mT) << gen->GetAng(1, mT) << gen->GetAng(2, mT);
		hash << gen->GetOffs(0, mT) << gen->GetOffs(1, mT) << gen->GetOffs(2, mT);
	}
	else if (auto texmap = dynamic_cast<Texmap*>(mat))
	{
		Matrix3 tm;
		texmap->GetUVTransform(tm);
		hash << tm;
		hash << getOutputTm(texmap);
		for (int i = 0, n = texmap->NumSubTexmaps(); i < n; i++)
		{
			if (auto sub = texmap->GetSubTexmap(i))
			{
			hash << sub << texmap->SubTexmapOn(i);
			if (auto subMat = dynamic_cast<MtlBase*>(sub))
					hash << GetHashValue(subMat, mT, hash_visited, syncTimestamp, bReloadMaterial);
		}
	}
	}

	for (int i = 0, n = mat->NumSubs(); i < n; i++)
	{
		if (auto sub = mat->SubAnim(i))
		{
			hash << GetHashValue(sub, mT, hash_visited, syncTimestamp, bReloadMaterial);
	}
	}

	hash_visited.insert(std::make_pair(mat, hash));

	return hash;
}


Texmap *MaterialParser::findDisplacementMap(MtlBase *mat)
{
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
						pb->GetValue(id, mT, v, FOREVER);
						if (v)
						{
							Texmap *res = findDisplacementMap(v);
							if (res)
								return res;
						}
					} break;
					case TYPE_TEXMAP:
					{
						Texmap *v = 0;
						pb->GetValue(id, mT, v, FOREVER);
						if (v)
						{
							if (v->ClassID() == FIRERENDER_DISPLACEMENTMTL_CID)
								return v;
							Texmap *res = findDisplacementMap(v);
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
							pb->GetValue(id, mT, v, FOREVER, i);
							if (v)
							{
								if (v->ClassID() == FIRERENDER_DISPLACEMENTMTL_CID)
									return v;
								Texmap *res = findDisplacementMap(v);
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

frw::Value MaterialParser::createMap(Texmap* texmap, const int flags)
{
	if (!texmap)
		return frw::Value();

	std::map<Animatable*, HashValue> hash_visited;
	auto key = GetHashValue(texmap, mT, hash_visited, syncTimestamp, true) << flags;
	
	auto result = mScope.GetValue(key);

	if (result)
		return result;

	MSTR s;		
	texmap->GetClassName(s);
	debugPrint(std::string("texmap class: ") + std::string(s.ToCStr()) + "\n");

	auto classId = texmap->ClassID();

	if(auto frmtl = dynamic_cast<FireRenderMtlBase*>(texmap)){
		frmtl->SetCurrentTime(mT);
	}	

	if (classId == FIRERENDER_ARITHMMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(ArithmMtl)*>(texmap)->getShader(mT, *this);
	else if (classId == FIRERENDER_INPUTLUMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(InputLUMtl)*>(texmap)->getShader(mT, *this);
	else if (classId == FIRERENDER_BLENDVALUEMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(BlendValueMtl)*>(texmap)->getShader(mT, *this);
	else if (classId == FIRERENDER_FRESNELMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(FresnelMtl)*>(texmap)->getShader(mT, *this);
	else if (classId == FIRERENDER_COLORMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(ColorMtl)*>(texmap)->getShader(mT, *this);
	else if (classId == FIRERENDER_AVERAGEMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(AvgMtl)*>(texmap)->getShader(mT, *this);
	else if (classId == FIRERENDER_NORMALMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(NormalMtl)*>(texmap)->getShader(mT, *this);
	else if (classId == FIRERENDER_FRESNELSCHLICKMTL_CID)
		result = dynamic_cast<FRMTLCLASSNAME(FresnelSchlickMtl)*>(texmap)->getShader(mT, *this);
	//else if (classId == NORMALBUMP_CID)
	//	result = createNormalBumpMap(texmap);
	else if (classId == Corona::MIX_TEX_CID && ScopeManagerMax::CoronaOK)
		result = createCoronaMixMap(texmap);
	else if (classId == Corona::COLOR_TEX_CID && ScopeManagerMax::CoronaOK)
		result = createCoronaColorMap(texmap);
	else switch (classId.PartA())
	{
	case NEW_FALLOFF_CLASS_ID:
		result = createFalloffMap(texmap);
		break;
	case CHECKER_CLASS_ID:
		result = createCheckerMap(texmap);
		break;

	case BMTEX_CLASS_ID:	
		result = createTextureMap(texmap, flags);
		break;

	case GRADIENT_CLASS_ID:
		result = createGradientMap(texmap);
		break;

	case NOISE_CLASS_ID:
		result = createNoiseMap(texmap);
		break;

	case COLORCORRECTION_CLASS_ID:
		result = createColorCorrectionMap(texmap);
		break;

	case MIX_CLASS_ID:
		result = createMixMap(texmap);
		break;

	case OUTPUT_CLASS_ID:
		result = createOutputMap(texmap);
		break;

	case RGBMULT_CLASS_ID:
		result = createRgbMultiplyMap(texmap);
		break;
	case TINT_CLASS_ID:
		result = createTintMap(texmap);
		break;

	case MASK_CLASS_ID:
		result = createMaskMap(texmap);
		break;

	case COMPOSITE_CLASS_ID:
		result = createCompositeMap(texmap);
		break;

	default:	// rasterize it if we must
		{
		if (auto map = createImageFromMap(texmap, (flags | (flags & MAP_FLAG_NORMALMAP)) ? MAP_FLAG_NOGAMMA : 0))
		{
			if (flags & MAP_FLAG_NORMALMAP)
			{
				frw::NormalMapNode node(materialSystem);
				frw::ImageNode imgNode(materialSystem);

				imgNode.SetMap(map);
				node.SetMap(imgNode);
				result = node;
			}
			else if (flags & MAP_FLAG_BUMPMAP)
			{
				frw::BumpMapNode node(materialSystem);
				frw::ImageNode imgNode(materialSystem);

				imgNode.SetMap(map);
				node.SetMap(imgNode);

				result = node;
			}
			else
			{
				frw::ImageNode node(materialSystem);
				node.SetMap(map);
				result = node;
			}
		}
		}
		break;
	}

	if(result.IsNode())
	{
		std::wstring name = texmap->GetName();
		std::string name_s( name.begin(), name.end() );
		//frw::ValueNode(result).SetName(name_s.c_str());
	}

	if (!result)
		debugPrint("Create map failed to return a value\n");
	else if (shaderData.mCacheable)
		mScope.SetValue(key, result);
	else
		debugPrint("uncacheable\n");

	return result;
}

frw::Shader MaterialParser::findVolumeMaterial(Mtl* mat)
{
	if (!mat)
		return frw::Shader();

	if (mat->ClassID() == FIRERENDER_VOLUMEMTL_CID)
	{
		return dynamic_cast<FRMTLCLASSNAME(VolumeMtl)*>(mat)->getVolumeShader(mT, *this, 0);
	}
	else if (mat->ClassID() == FIRERENDER_UBERMTL_CID)
	{
		frw::Shader res = dynamic_cast<FRMTLCLASSNAME(UberMtl)*>(mat)->getVolumeShader(mT, *this, 0);
		
		if (res)
			return res;
	}
	else if (mat->ClassID() == FIRERENDER_UBERMTLV2_CID)
	{
		frw::Shader res = dynamic_cast<FRMTLCLASSNAME(UberMtlv2)*>(mat)->getVolumeShader(mT, *this, 0);
		
		if (res)
			return res;
	}
	else if (mat->ClassID() == FIRERENDER_PBRMTL_CID)
	{
		frw::Shader res = dynamic_cast<FRMTLCLASSNAME(PbrMtl)*>(mat)->getVolumeShader(mT, *this, 0);
		
		if (res)
			return res;
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
						pb->GetValue(id, mT, v, FOREVER);
						if (v)
						{
							frw::Shader res = findVolumeMaterial(v);
							if (res)
								return res;
						}
					}
					break;

					case TYPE_MTL_TAB:
					{
						int count = pb->Count(id);
						for (int i = 0; i < count; i++)
						{
							Mtl *v = 0;
							pb->GetValue(id, mT, v, FOREVER, i);
							if (v)
							{
								frw::Shader res = findVolumeMaterial(v);
								if (res)
									return res;
							}
						}
					}
					break;
				}
			}
		}
	}
	return frw::Shader();
}

frw::Shader MaterialParser::createVolumeShader(Mtl* material, INode* node)
{
	if (node)	// first level
	{
		shaderData.mNode = node;
		shaderData.mCacheable = true;
	}
	else
	{
		node = shaderData.mNode;
	}

	if (!material) 
		return frw::Shader();

	frw::Shader shader;

	//auto matKey = getMaterialHash(material);
	//matKey << "volumetric";
	//auto matNodeKey = matKey;
	//matNodeKey << node;

	//shader = mScope.GetShader(matKey);	// a material isn'mT going to change mid-render
	//if (!shader)
	//	shader = mScope.GetShader(matNodeKey);	// ok so maybe we have a more specific version for this node

	if(shader)
		return shader;

		const Class_ID cid = material->ClassID();

	if(auto frmtl = dynamic_cast<FireRenderMtlBase*>(material)){
		frmtl->SetCurrentTime(mT);
	}	

	if(false){}
	else if (cid == FIRERENDER_VOLUMEMTL_CID) {
		shader = dynamic_cast<FRMTLCLASSNAME(VolumeMtl)*>(material)->getVolumeShader(mT, *this, node);
	}
	else if (cid == FIRERENDER_MATERIALMTL_CID) {
		shader = dynamic_cast<FRMTLCLASSNAME(MaterialMtl)*>(material)->getVolumeShader(mT, *this, node);
	}
	else
	{
		// default - nothing else fits - so this is an unsupported material. 
		return frw::Shader();
	}
	
	if(shader)
	{
		std::wstring name = material->GetName();
		std::string name_s( name.begin(), name.end() );
		shader.SetName(name_s.c_str());
#ifdef CODE_TO_PORT
		if (shaderData.cacheable)
			mScope.SetShader(matKey, shader);
		else
			mScope.SetShader(matNodeKey, shader);
#endif
	}

	return shader;
}

frw::Shader MaterialParser::createShader(Mtl* material, INode* node /*= nullptr*/, bool bReloadMaterial /*= false*/)
{
	if (node)	// first level
	{
		shaderData.mNode = node;
		shaderData.mCacheable = true;

		shaderData.mNumEmissive = 0;
		shaderData.mNumVolume = 0;
		shaderData.mNumDisplacement = 0;
		shaderData.mDisplacementDirectlyPlugged = false;
		shaderData.mNormalDirectlyPlugged = false;
	}
	else
	{
		node = shaderData.mNode;
	}

	frw::Shader shader;

	if (!node || !node->GetObjectRef())
		return shader;

	if (node && node->GetObjectRef()->ClassID() == Corona::LIGHT_CID) 
		if (!ScopeManagerMax::CoronaOK)
			return shader;
		else
		material = nullptr; // to correctly handle CoronaLights - we need to ignore any material that might be already assigned

	if (!material) 
	{
		// Parsing Corona light (we will create emissive shader), or no material assigned (we will create diffuse material using 
		// wire color of the object)
		if (node->GetObjectRef()->ClassID() == Corona::LIGHT_CID) 
		{
			if  (ScopeManagerMax::CoronaOK)
			shader = parseCoronaLightObject(node);
		}
		else 
		{
			Color wireColor(node->GetWireColor());
			auto sh = frw::DiffuseShader(materialSystem);
			sh.SetColor(wireColor);
			shader = sh;
		}
	}
	else
	{
		if (!shader)
		{
			const Class_ID cid = material->ClassID();

			if(auto frmtl = dynamic_cast<FireRenderMtlBase*>(material)){
				frmtl->SetCurrentTime(mT);
			}	
			if (auto stdMat2 = dynamic_cast<StdMat2*>(material)) {
				shader = parseStdMat2(stdMat2);
			}
			else if (cid == PHYSICALMATERIAL_CID) {
				shader = parsePhysicalMaterial(material);
			}
			else if (cid == FIRERENDER_DIFFUSEMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(DiffuseMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_BLENDMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(BlendMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_ADDMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(AddMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_MICROFACETMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(MicrofacetMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_REFLECTIONMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(ReflectionMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_REFRACTIONMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(RefractionMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_MFREFRACTIONMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(MFRefractionMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_TRANSPARENTMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(TransparentMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_WARDMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(WardMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_EMISSIVEMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(EmissiveMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_STANDARDMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(StandardMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_ORENNAYARMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(OrenNayarMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_DIFFUSEREFRACTIONMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(DiffuseRefractionMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_MATERIALMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(MaterialMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_UBERMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(UberMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_UBERMTLV2_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(UberMtlv2)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_PBRMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(PbrMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == FIRERENDER_VOLUMEMTL_CID) {
				shader = dynamic_cast<FRMTLCLASSNAME(VolumeMtl)*>(material)->getShader(mT, *this, node);
			}
			else if (cid == Corona::LAYERED_MTL_CID) {
				if (ScopeManagerMax::CoronaOK)
					shader = parseCoronaLayeredMtl(material, node);
			}
			else if (cid == Corona::MTL_CID) {
				if (ScopeManagerMax::CoronaOK)
					shader = parseCoronaMtl(material);
			}
			else if (cid == Corona::LIGHT_MTL_CID) {
				if (ScopeManagerMax::CoronaOK)
					shader = parseCoronaLightMtl(material);
			}
			else if (cid == Corona::PORTAL_MTL_CID) {
				if (ScopeManagerMax::CoronaOK)
					shader = parseCoronaPortalMtl(material);
			}
			else if (cid == Corona::SHADOW_CATCHER_MTL_CID) {
				if (ScopeManagerMax::CoronaOK)
					shader = parseCoronaShadowCatcherMtl(material);
			}
			else if (cid == Corona::VOLUME_MTL_CID) {
				if (ScopeManagerMax::CoronaOK)
					shader = parseCoronaVolumeMtl(material);
			}
			else if (cid == Corona::RAY_SWITCH_MTL_CID) {
				if (ScopeManagerMax::CoronaOK)
					shader = parseCoronaRaySwitchMtl(material);
			}
			else if (cid == Class_ID(BAKE_SHELL_CLASS_ID, 0))
			{
				// this MAX material allows using different materials in viewport and in render.
				// We'll just grab the render material and parse it
				if (auto toParse = material->GetSubMtl(material->GetParamBlock(bakeShell_params)->GetInt(bakeShell_render_n_mtl, this->mT)))
					shader = createShader(toParse);
			}
			else if (auto toParse = dynamic_cast<IXRefMaterial*>(material)) 
			{
				// MAX material automatically assigned to XRef objects (objects linked from another scene).
				// We'll just grab the material it is holding and parse it
				shader = createShader(toParse->GetSourceMaterial(true));
			}
			else if (auto toParse = dynamic_cast<MSMtlXtnd*>(material)) 
			{
				// MAX wrapper around some maxscript materials. Just grab the material it is holding and parse it
				shader = createShader(toParse->delegate);
			}
			else if (cid == Class_ID(0x39e763da, 0x51ed61a0))
			{
				// We get this in material editor when "Display map as 2D" is unchecked in options. We are actually
				// interested in its one and only submap
				FASSERT(material->NumSubTexmaps() == 1 && material->GetSubTexmap(0) != NULL);
				auto sh = frw::DiffuseShader(materialSystem);
				sh.SetColor(createMap(material->GetSubTexmap(0), MAP_FLAG_NOGAMMA));
				shader = sh;
			}
			else if (cid == Class_ID(MIXMAT_CLASS_ID, 0))
			{
				shader = parseBlendMtl(material);
			}
			else
			{
				// default - nothing else fits - so this is an unsupported material. Parse it as diffuse.
				auto sh = frw::DiffuseShader(materialSystem);
				sh.SetColor(material->GetDiffuse());
				shader = sh;
			}

			if(shader)
			{
				std::wstring name = material->GetName();
				std::string name_s( name.begin(), name.end() );
				shader.SetName(name_s.c_str());

				//if (shaderData.mCacheable)
				//	mScope.SetShader(matKey, shader);
				//else
				//	mScope.SetShader(matNodeKey, shader);
			}
		}
	}

	if(shader)
		shader.SetUserData(Animatable::GetHandleByAnim(material));

	return shader;
}

Texmap* getMap(StdMat2* mtl, int id)
{
	auto mapChannel = mtl->StdIDToChannel(id);
	return mtl->SubTexmapOn(mapChannel) ? mtl->GetSubTexmap(mapChannel) : NULL;
}

namespace {

#define STDMAT2_PBVALUETEX(N)\
	Texmap *N##_tex = MaterialParser::getTexture(mtl, N); \
	frw::Value N##_v = createMap(N##_tex, MAP_FLAG_NOGAMMA) | frw::Value(GetFromPb<float>(pb, N, this->mT));
	
#define STDMAT2_PBCOLORTEX(N)\
	Texmap *N##_tex = MaterialParser::getTexture(mtl, N); \
	frw::Value N##_v = createMap(N##_tex, MAP_FLAG_NOFLAGS) | frw::Value(GetFromPb<Color>(pb, N, this->mT));
	
};

frw::Shader MaterialParser::parseStdMat2(StdMat2* mtl) 
{

	// EMISSION
	//
	auto emissiveColor = getValue(getMap(mtl, ID_SI), mtl->GetSelfIllumColor(this->mT));
	if (emissiveColor.NonZeroXYZ())
	{
		shaderData.mNumEmissive++;

		frw::EmissiveShader shader(materialSystem);
		shader.SetColor(emissiveColor);

		return shader;
	}
		
	Shader *maxShader = mtl->GetShader();
	
	if (!maxShader)
		return frw::DiffuseShader(materialSystem);
	
	MSTR shaderClassName;
	maxShader->GetClassName(shaderClassName);

	IParamBlock2* pb = maxShader->GetParamBlock(0);

	// DIFFUSE
	//
	long mapChannel = mtl->StdIDToChannel(ID_BU); //This method returns submap index using a list of predefined map types
	Texmap* bumpMap = mtl->SubTexmapOn(mapChannel) ? mtl->GetSubTexmap(mapChannel) : NULL;

	mapChannel = mtl->StdIDToChannel(ID_DI);
	Texmap* diffuseMap = mtl->SubTexmapOn(mapChannel) ? mtl->GetSubTexmap(mapChannel) : NULL;

	mapChannel = mtl->StdIDToChannel(ID_OP);
	Texmap* opacityMap = mtl->SubTexmapOn(mapChannel) ? mtl->GetSubTexmap(mapChannel) : NULL;

	const float fresnelIor = 2;// 1.f + 10 * specularLevel; // rough match of specularity between scanline and RPR

	frw::Value diffuseColor = getValue(getMap(mtl, ID_DI), mtl->GetDiffuse(this->mT));

	frw::Value opacityLevel = getValue(getMap(mtl, ID_OP), mtl->GetOpacity(this->mT));

	frw::Shader result(materialSystem, frw::ShaderTypeDiffuse);
	result.SetValue("color", diffuseColor);
	
	// NORMAL
	frw::Value normal;
	if (bumpMap)
	{
		float strength = mtl->GetTexmapAmt(ID_BU, mT);
		normal = FRMTLCLASSNAME(NormalMtl)::translateGenericBump(mT, bumpMap, strength, *this);
				result.SetValue("normal", normal);
			}
	
	// SPECULAR REFLECTIONS
	//
	auto specularLevel = getValue(getMap(mtl, ID_SS), mtl->GetShinStr(this->mT));
	auto specularColor = getValue(getMap(mtl, ID_SP), mtl->GetSpecular(this->mT));

	bool translucent_material = false;

	if (shaderClassName == L"Blinn" || shaderClassName == L"Phong")
	{
		auto ior = mtl->GetIOR(this->mT);
		auto roughness = StdMat2_glossinessToRoughness(getValue(getMap(mtl, ID_SH), mtl->GetShininess(this->mT)));


		specularColor = materialSystem.ValueMul(specularColor, specularLevel);

		frw::Shader shader(materialSystem, frw::ShaderTypeMicrofacet);
		shader.SetValue("color", specularColor);
		shader.SetValue("normal", normal);
		shader.SetValue("ior", frw::Value(ior));
		shader.SetValue("roughness", roughness);
		result = materialSystem.ShaderBlend(result, shader, frw::Value(0.5, 0.5, 0.5));
	}
	else if (shaderClassName == L"Anisotropic")
	{
		STDMAT2_PBVALUETEX(an_specular_level)
		STDMAT2_PBVALUETEX(an_diffuse_level)
		STDMAT2_PBVALUETEX(an_glossiness)
		STDMAT2_PBVALUETEX(an_anisotropy)
		STDMAT2_PBVALUETEX(an_orientation)
		
		diffuseColor = materialSystem.ValueMul(diffuseColor, an_diffuse_level_v);
		result.SetValue("color", diffuseColor);

		specularColor = materialSystem.ValueMul(specularColor, an_specular_level_v);

		frw::Shader shader(materialSystem, frw::ShaderTypeWard);
		shader.SetValue("color", specularColor);

		auto roughness = StdMat2_glossinessToRoughness(an_glossiness_v);
		shader.SetValue("roughness_y", roughness);
		shader.SetValue("roughness_x", materialSystem.ValueMul(roughness, materialSystem.ValueSub(frw::Value(1.0), an_anisotropy_v)));
		//shader.SetValue("rotation", materialSystem.ValueMul(an_orientation_v, 0.01745329251994329576923690768489)); // PI/180
		shader.SetValue("rotation", an_orientation_v);
		shader.SetValue("normal", normal);
				
		result = materialSystem.ShaderBlend(result, shader, frw::Value(0.5, 0.5, 0.5));
	}
	else if (shaderClassName == L"Metal")
	{
		auto ior = mtl->GetIOR(this->mT);
		auto roughness = StdMat2_glossinessToRoughness(getValue(getMap(mtl, ID_SH), mtl->GetShininess(this->mT)));

		frw::Shader shader(materialSystem, frw::ShaderTypeMicrofacet);
		shader.SetValue("color", diffuseColor);
		shader.SetValue("normal", normal);
		shader.SetValue("ior", frw::Value(ior));
		shader.SetValue("roughness", roughness);
		result = materialSystem.ShaderBlend(result, shader, specularLevel);
	}
	else if (shaderClassName == L"Multi-Layer")
	{
		STDMAT2_PBCOLORTEX(ml_diffuse)
		STDMAT2_PBVALUETEX(ml_diffuse_level)
		STDMAT2_PBVALUETEX(ml_diffuse_rough)

		STDMAT2_PBCOLORTEX(ml_specular1)
		STDMAT2_PBVALUETEX(ml_specular_level1)
		STDMAT2_PBVALUETEX(ml_glossiness1)
		STDMAT2_PBVALUETEX(ml_anisotropy1)
		STDMAT2_PBVALUETEX(ml_orientation1)

		STDMAT2_PBCOLORTEX(ml_specular2)
		STDMAT2_PBVALUETEX(ml_specular_level2)
		STDMAT2_PBVALUETEX(ml_glossiness2)
		STDMAT2_PBVALUETEX(ml_anisotropy2)
		STDMAT2_PBVALUETEX(ml_orientation2)	

		result = frw::Shader(materialSystem, frw::ShaderTypeOrenNayer);
		result.SetValue("color", materialSystem.ValueMul(diffuseColor, ml_diffuse_level_v));
		result.SetValue("roughness", ml_diffuse_rough_v);
		
		frw::Shader specular1(materialSystem, frw::ShaderTypeWard);
		specular1.SetValue("color", materialSystem.ValueMul(ml_specular1_v, ml_specular_level1_v));
		auto roughness1 = StdMat2_glossinessToRoughness(ml_glossiness1_v);
		specular1.SetValue("roughness_y", roughness1);
		specular1.SetValue("roughness_x", materialSystem.ValueMul(roughness1, materialSystem.ValueSub(frw::Value(1.0), ml_anisotropy1_v)));
		specular1.SetValue("rotation", ml_orientation1_v);
		specular1.SetValue("normal", normal);

		frw::Shader specular2(materialSystem, frw::ShaderTypeWard);
		specular2.SetValue("color", materialSystem.ValueMul(ml_specular2_v, ml_specular_level2_v));
		auto roughness2 = StdMat2_glossinessToRoughness(ml_glossiness2_v);
		specular2.SetValue("roughness_y", roughness2);
		specular2.SetValue("roughness_x", materialSystem.ValueMul(roughness2, materialSystem.ValueSub(frw::Value(1.0), ml_anisotropy2_v)));
		specular2.SetValue("rotation", ml_orientation2_v);
		specular2.SetValue("normal", normal);
		
		frw::Shader specular = materialSystem.ShaderBlend(specular1, specular2, frw::Value(0.5, 0.5, 0.5, 0.5));
		result = materialSystem.ShaderBlend(result, specular, frw::Value(0.5, 0.5, 0.5, 0.5));
	}
	else if (shaderClassName == L"Oren-Nayar-Blinn")
	{
		STDMAT2_PBVALUETEX(onb_glossiness)
		STDMAT2_PBVALUETEX(onb_specular_level)
		STDMAT2_PBVALUETEX(onb_diffuse_level)
		//onb_soften?
		STDMAT2_PBVALUETEX(onb_roughness)

		result = frw::Shader(materialSystem, frw::ShaderTypeOrenNayer);

		diffuseColor = materialSystem.ValueMul(diffuseColor, onb_diffuse_level_v);
		result.SetValue("color", diffuseColor);
		result.SetValue("roughness", onb_roughness_v);
				
		auto ior = mtl->GetIOR(this->mT);
		specularColor = materialSystem.ValueMul(specularColor, onb_specular_level_v);

		frw::Shader shader(materialSystem, frw::ShaderTypeMicrofacet);
		shader.SetValue("color", specularColor);
		shader.SetValue("normal", normal);
		shader.SetValue("ior", frw::Value(ior));
		shader.SetValue("roughness", materialSystem.ValueMul(onb_roughness_v, StdMat2_glossinessToRoughness(onb_glossiness_v)));

		result = materialSystem.ShaderBlend(result, shader, frw::Value(0.5, 0.5, 0.5));
	}
	else if (shaderClassName == L"Strauss")
	{
		STDMAT2_PBVALUETEX(st_glossiness)
		STDMAT2_PBVALUETEX(st_metalness)

		frw::Shader shader(materialSystem, frw::ShaderTypeMicrofacet);
		shader.SetValue("color", materialSystem.ValueAdd(materialSystem.ValueMul(diffuseColor, st_metalness_v), materialSystem.ValueMul(frw::Value(1.0, 1.0, 1.0, 1.0), materialSystem.ValueSub(1.0, st_metalness_v))));
		shader.SetValue("normal", normal);
		shader.SetValue("ior", frw::Value(mtl->GetIOR(this->mT)));
		shader.SetValue("roughness", StdMat2_glossinessToRoughness(st_glossiness_v));
		result = materialSystem.ShaderBlend(shader, result, st_metalness_v);
	}
	else if (shaderClassName == L"Translucent Shader")
	{
		STDMAT2_PBVALUETEX(tl_diffuse_level)
		STDMAT2_PBVALUETEX(tl_glossiness)
		STDMAT2_PBVALUETEX(tl_specular_level)
		STDMAT2_PBCOLORTEX(tl_translucent_color)
		STDMAT2_PBCOLORTEX(tl_filter)

		result.SetValue("color", tl_translucent_color_v);

		frw::Shader microfacet(materialSystem, frw::ShaderTypeMicrofacet);
		microfacet.SetValue("color", materialSystem.ValueMul(diffuseColor, tl_diffuse_level_v));
		microfacet.SetValue("normal", normal);
		microfacet.SetValue("ior", frw::Value(1.5));
		microfacet.SetValue("roughness", frw::Value(0.4));

		result = materialSystem.ShaderBlend(result, microfacet, 0.5);

		frw::Shader filter(materialSystem, frw::ShaderTypeTransparent);
		filter.SetValue("color", tl_filter_v);

		result = materialSystem.ShaderBlend(result, filter, opacityLevel);
		
		frw::Shader specular(materialSystem, frw::ShaderTypeMicrofacet);
		specular.SetValue("color", materialSystem.ValueMul(specularColor, tl_specular_level_v));
		specular.SetValue("normal", normal);
		specular.SetValue("ior", frw::Value(1.5));
		specular.SetValue("roughness", StdMat2_glossinessToRoughness(tl_glossiness_v));

		result = materialSystem.ShaderBlend(result, specular, frw::Value(0.5));

		translucent_material = true;
	}

	// OPACITY AND REFRACTION
	//
	if (!translucent_material)
	{
		float opacity = mtl->GetOpacity(this->mT);
		if (opacity < 1.f || opacityMap)
		{
			frw::Value filterColor = getValue(getMap(mtl, ID_FI), mtl->GetFilter(this->mT));
			frw::Shader transp_shader(materialSystem, frw::ShaderTypeTransparent);
			//transp_shader.SetValue("color", filterColor);
			//transp_shader.SetValue("color", materialSystem.ValueMul(materialSystem.ValueSub(1.f, opacityLevel), filterColor));
			transp_shader.SetValue("color", materialSystem.ValueSub(1.f, opacityLevel));
			result = materialSystem.ShaderBlend(result, transp_shader, materialSystem.ValueSub(1.f, opacityLevel));
		}
	}

	return result;
}

frw::Shader MaterialParser::parsePhysicalMaterial(Mtl* mtl)
{
	IParamBlock2* pb = mtl->GetParamBlock(0);
	int material_mode, aniso_mode, aniso_channel;
	float base_weight, metalness, diff_roughness, brdf_low, brdf_high, brdf_curve,
		anisotropy, anisoangle, transparency, trans_depth, trans_roughness, trans_ior,
		scattering, sss_depth, sss_scale, emission, emit_luminance, emit_kelvin,
		coating, coat_roughness, coat_affect_color, coat_affect_roughness, coat_ior,
		bump_map_amt, clearcoat_bump_map_amt, reflectivity, roughness;
	Color  base_color, refl_color, trans_color, sss_color, sss_scatter_color, emit_color, coat_color;
	BOOL roughness_inv, brdf_mode, trans_roughness_inv, trans_roughness_lock, thin_walled,
		coat_roughness_inv, base_weight_map_on, base_color_map_on, reflectivity_map_on, refl_color_map_on,
		roughness_map_on, metalness_map_on, diff_rough_map_on, anisotropy_map_on, aniso_angle_map_on,
		transparency_map_on, trans_color_map_on, trans_rough_map_on, trans_ior_map_on, scattering_map_on,
		sss_color_map_on, sss_scale_map_on, emission_map_on, emit_color_map_on, coat_map_on,
		coat_color_map_on, coat_rough_map_on, bump_map_on, coat_bump_map_on, cutout_map_on;
	Texmap *base_weight_map = 0, *base_color_map = 0, *reflectivity_map = 0, *refl_color_map = 0, *roughness_map,
		*metalness_map = 0, *diff_rough_map = 0, *anisotropy_map = 0, *aniso_angle_map = 0, *transparency_map = 0, *trans_color_map = 0,
		*trans_rough_map = 0, *trans_ior_map = 0, *scattering_map = 0, *sss_color_map = 0, *sss_scale_map = 0, *emission_map = 0,
		*emit_color_map = 0, *coat_map = 0, *coat_color_map = 0, *coat_rough_map = 0, *bump_map = 0, *coat_bump_map = 0, *cutout_map = 0;
	Texmap *mapM0 = 0, *mapM1 = 0, *mapM2 = 0, *mapM4 = 0, *mapM5 = 0, *mapM3 = 0, *mapM6 = 0, *mapM7 = 0, *mapM8 = 0,
		*mapM9 = 0, *mapM10 = 0, *mapM11 = 0, *mapM12 = 0, *mapM13 = 0, *mapM14 = 0, *mapM15 = 0, *mapM16 = 0, *mapM17 = 0,
		*mapM18 = 0, *mapM19 = 0, *mapM20 = 0;

	auto mT = GetCOREInterface()->GetTime();

	pb->GetValue(phm_material_mode, mT, material_mode, FOREVER);
	pb->GetValue(phm_base_weight, mT, base_weight, FOREVER);
	pb->GetValue(phm_base_color, mT, base_color, FOREVER);
	pb->GetValue(phm_reflectivity, mT, reflectivity, FOREVER);
	pb->GetValue(phm_roughness, mT, roughness, FOREVER);
	pb->GetValue(phm_roughness_inv, mT, roughness_inv, FOREVER);
	pb->GetValue(phm_metalness, mT, metalness, FOREVER);
	pb->GetValue(phm_refl_color, mT, refl_color, FOREVER);
	pb->GetValue(phm_diff_roughness, mT, diff_roughness, FOREVER);
	pb->GetValue(phm_brdf_mode, mT, brdf_mode, FOREVER);
	pb->GetValue(phm_brdf_low, mT, brdf_low, FOREVER);
	pb->GetValue(phm_brdf_high, mT, brdf_high, FOREVER);
	pb->GetValue(phm_brdf_curve, mT, brdf_curve, FOREVER);
	pb->GetValue(phm_anisotropy, mT, anisotropy, FOREVER);
	pb->GetValue(phm_anisoangle, mT, anisoangle, FOREVER);
	pb->GetValue(phm_aniso_mode, mT, aniso_mode, FOREVER);
	pb->GetValue(phm_aniso_channel, mT, aniso_channel, FOREVER);
	pb->GetValue(phm_transparency, mT, transparency, FOREVER);
	pb->GetValue(phm_trans_color, mT, trans_color, FOREVER);
	pb->GetValue(phm_trans_depth, mT, trans_depth, FOREVER);
	pb->GetValue(phm_trans_roughness, mT, trans_roughness, FOREVER);
	pb->GetValue(phm_trans_roughness_inv, mT, trans_roughness_inv, FOREVER);
	pb->GetValue(phm_trans_roughness_lock, mT, trans_roughness_lock, FOREVER);
	pb->GetValue(phm_trans_ior, mT, trans_ior, FOREVER);
	pb->GetValue(phm_thin_walled, mT, thin_walled, FOREVER);
	pb->GetValue(phm_scattering, mT, scattering, FOREVER);
	pb->GetValue(phm_sss_color, mT, sss_color, FOREVER);
	pb->GetValue(phm_sss_depth, mT, sss_depth, FOREVER);
	pb->GetValue(phm_sss_scale, mT, sss_scale, FOREVER);
	pb->GetValue(phm_sss_scatter_color, mT, sss_scatter_color, FOREVER);
	pb->GetValue(phm_emission, mT, emission, FOREVER);
	pb->GetValue(phm_emit_color, mT, emit_color, FOREVER);
	pb->GetValue(phm_emit_luminance, mT, emit_luminance, FOREVER);
	pb->GetValue(phm_emit_kelvin, mT, emit_kelvin, FOREVER);
	pb->GetValue(phm_coating, mT, coating, FOREVER);
	pb->GetValue(phm_coat_color, mT, coat_color, FOREVER);
	pb->GetValue(phm_coat_roughness, mT, coat_roughness, FOREVER);
	pb->GetValue(phm_coat_roughness_inv, mT, coat_roughness_inv, FOREVER);
	pb->GetValue(phm_coat_affect_color, mT, coat_affect_color, FOREVER);
	pb->GetValue(phm_coat_affect_roughness, mT, coat_affect_roughness, FOREVER);
	pb->GetValue(phm_coat_ior, mT, coat_ior, FOREVER);
	pb->GetValue(phm_base_weight_map, mT, base_weight_map, FOREVER);
	pb->GetValue(phm_base_color_map, mT, base_color_map, FOREVER);
	pb->GetValue(phm_reflectivity_map, mT, reflectivity_map, FOREVER);
	pb->GetValue(phm_refl_color_map, mT, refl_color_map, FOREVER);
	pb->GetValue(phm_roughness_map, mT, roughness_map, FOREVER);
	pb->GetValue(phm_metalness_map, mT, metalness_map, FOREVER);
	pb->GetValue(phm_diff_rough_map, mT, diff_rough_map, FOREVER);
	pb->GetValue(phm_anisotropy_map, mT, anisotropy_map, FOREVER);
	pb->GetValue(phm_aniso_angle_map, mT, aniso_angle_map, FOREVER);
	pb->GetValue(phm_transparency_map, mT, transparency_map, FOREVER);
	pb->GetValue(phm_trans_color_map, mT, trans_color_map, FOREVER);
	pb->GetValue(phm_trans_rough_map, mT, trans_rough_map, FOREVER);
	pb->GetValue(phm_trans_ior_map, mT, trans_ior_map, FOREVER);
	pb->GetValue(phm_scattering_map, mT, scattering_map, FOREVER);
	pb->GetValue(phm_sss_color_map, mT, sss_color_map, FOREVER);
	pb->GetValue(phm_sss_scale_map, mT, sss_scale_map, FOREVER);
	pb->GetValue(phm_emission_map, mT, emission_map, FOREVER);
	pb->GetValue(phm_emit_color_map, mT, emit_color_map, FOREVER);
	pb->GetValue(phm_coat_map, mT, coat_map, FOREVER);
	pb->GetValue(phm_coat_color_map, mT, coat_color_map, FOREVER);
	pb->GetValue(phm_coat_rough_map, mT, coat_rough_map, FOREVER);
	pb->GetValue(phm_bump_map, mT, bump_map, FOREVER);
	pb->GetValue(phm_coat_bump_map, mT, coat_bump_map, FOREVER);
	pb->GetValue(phm_cutout_map, mT, cutout_map, FOREVER);
	pb->GetValue(phm_base_weight_map_on, mT, base_weight_map_on, FOREVER);
	pb->GetValue(phm_base_color_map_on, mT, base_color_map_on, FOREVER);
	pb->GetValue(phm_reflectivity_map_on, mT, reflectivity_map_on, FOREVER);
	pb->GetValue(phm_refl_color_map_on, mT, refl_color_map_on, FOREVER);
	pb->GetValue(phm_roughness_map_on, mT, roughness_map_on, FOREVER);
	pb->GetValue(phm_metalness_map_on, mT, metalness_map_on, FOREVER);
	pb->GetValue(phm_diff_rough_map_on, mT, diff_rough_map_on, FOREVER);
	pb->GetValue(phm_anisotropy_map_on, mT, anisotropy_map_on, FOREVER);
	pb->GetValue(phm_aniso_angle_map_on, mT, aniso_angle_map_on, FOREVER);
	pb->GetValue(phm_transparency_map_on, mT, transparency_map_on, FOREVER);
	pb->GetValue(phm_trans_color_map_on, mT, trans_color_map_on, FOREVER);
	pb->GetValue(phm_trans_rough_map_on, mT, trans_rough_map_on, FOREVER);
	pb->GetValue(phm_trans_ior_map_on, mT, trans_ior_map_on, FOREVER);
	pb->GetValue(phm_scattering_map_on, mT, scattering_map_on, FOREVER);
	pb->GetValue(phm_sss_color_map_on, mT, sss_color_map_on, FOREVER);
	pb->GetValue(phm_sss_scale_map_on, mT, sss_scale_map_on, FOREVER);
	pb->GetValue(phm_emission_map_on, mT, emission_map_on, FOREVER);
	pb->GetValue(phm_emit_color_map_on, mT, emit_color_map_on, FOREVER);
	pb->GetValue(phm_coat_map_on, mT, coat_map_on, FOREVER);
	pb->GetValue(phm_coat_color_map_on, mT, coat_color_map_on, FOREVER);
	pb->GetValue(phm_coat_rough_map_on, mT, coat_rough_map_on, FOREVER);
	pb->GetValue(phm_bump_map_on, mT, bump_map_on, FOREVER);
	pb->GetValue(phm_coat_bump_map_on, mT, coat_bump_map_on, FOREVER);
	pb->GetValue(phm_cutout_map_on, mT, cutout_map_on, FOREVER);
	pb->GetValue(phm_bump_map_amt, mT, bump_map_amt, FOREVER);
	pb->GetValue(phm_clearcoat_bump_map_amt, mT, clearcoat_bump_map_amt, FOREVER);
	pb->GetValue(phm_mapM0, mT, mapM0, FOREVER);
	pb->GetValue(phm_mapM1, mT, mapM1, FOREVER);
	pb->GetValue(phm_mapM2, mT, mapM2, FOREVER);
	pb->GetValue(phm_mapM4, mT, mapM4, FOREVER);
	pb->GetValue(phm_mapM5, mT, mapM5, FOREVER);
	pb->GetValue(phm_mapM3, mT, mapM3, FOREVER);
	pb->GetValue(phm_mapM6, mT, mapM6, FOREVER);
	pb->GetValue(phm_mapM7, mT, mapM7, FOREVER);
	pb->GetValue(phm_mapM8, mT, mapM8, FOREVER);
	pb->GetValue(phm_mapM9, mT, mapM9, FOREVER);
	pb->GetValue(phm_mapM10, mT, mapM10, FOREVER);
	pb->GetValue(phm_mapM11, mT, mapM11, FOREVER);
	pb->GetValue(phm_mapM12, mT, mapM12, FOREVER);
	pb->GetValue(phm_mapM13, mT, mapM13, FOREVER);
	pb->GetValue(phm_mapM14, mT, mapM14, FOREVER);
	pb->GetValue(phm_mapM15, mT, mapM15, FOREVER);
	pb->GetValue(phm_mapM16, mT, mapM16, FOREVER);
	pb->GetValue(phm_mapM17, mT, mapM17, FOREVER);
	pb->GetValue(phm_mapM18, mT, mapM18, FOREVER);
	pb->GetValue(phm_mapM19, mT, mapM19, FOREVER);
	pb->GetValue(phm_mapM20, mT, mapM20, FOREVER);

	// NORMAL
	frw::Value BaseBumpMap;
	if (bump_map_on && bump_map)
	{
		BaseBumpMap = FRMTLCLASSNAME(NormalMtl)::translateGenericBump(mT, bump_map, bump_map_amt, *this);
	}
	
	// compute roughness
	frw::Value Roughness;
	if (roughness_map_on && roughness_map)
		Roughness = createMap(roughness_map, MAP_FLAG_NOGAMMA);
	else
		Roughness = frw::Value(roughness, roughness, roughness);
	if (roughness_inv)
		Roughness = materialSystem.ValueSub(frw::Value(1.f), Roughness);

	// compute coating weight
	frw::Value CoatWeight;
	if (coat_map_on && coat_map)
		CoatWeight = createMap(coat_map, MAP_FLAG_NOGAMMA);
	else
		CoatWeight = frw::Value(coating, coating, coating);

	// compute coat roughness
	frw::Value CoatRoughness;
	if (coat_rough_map_on && coat_rough_map)
		CoatRoughness = createMap(coat_rough_map, MAP_FLAG_NOGAMMA);
	else
		CoatRoughness = frw::Value(coat_roughness, coat_roughness, coat_roughness);
	if (coat_roughness_inv)
		CoatRoughness = materialSystem.ValueSub(1.f, CoatRoughness);
	
	frw::Value Affect = materialSystem.ValueMul(CoatWeight, materialSystem.ValueMul(coat_affect_roughness, CoatRoughness));
	Roughness = materialSystem.ValueSub(1.0, materialSystem.ValueMul(materialSystem.ValueSub(1.0, Affect), materialSystem.ValueSub(1.0, Roughness)));
	
	// BASE COLOR (DIFFUSE)
	frw::Shader BaseShader = frw::Shader(materialSystem, frw::ShaderTypeOrenNayer);

	frw::Value BaseColorWeight;
	if (base_weight_map_on && base_weight_map)
		BaseColorWeight = createMap(base_weight_map, MAP_FLAG_NOGAMMA);
	else
		BaseColorWeight = frw::Value(base_weight, base_weight, base_weight);
	frw::Value BaseColor;
	if (base_color_map_on && base_color_map)
		BaseColor = createMap(base_color_map, MAP_FLAG_NOFLAGS);
	else
		BaseColor = frw::Value(base_color.r, base_color.g, base_color.b);
	BaseColor = materialSystem.ValueMul(BaseColorWeight, BaseColor);

	BaseColor = materialSystem.ValuePow(BaseColor, materialSystem.ValueAdd(1.0, materialSystem.ValueMul(CoatWeight, coat_affect_color)));

	frw::Value BaseRoughness(diff_roughness, diff_roughness, diff_roughness);

	BaseShader.SetValue("roughness", BaseRoughness);
	
	BaseShader.SetValue("normal", BaseBumpMap);

	// TRANSPARENCY
	frw::Shader RefrShader = frw::Shader(materialSystem, frw::ShaderTypeMicrofacetRefraction);
	
	frw::Value RefrWeight;
	if (transparency_map_on && transparency_map)
		RefrWeight = createMap(transparency_map, MAP_FLAG_NOGAMMA);
	else
		RefrWeight = frw::Value(transparency, transparency, transparency);
		
	frw::Value RefrColor;
	if (trans_color_map_on && trans_color_map)
		RefrColor = createMap(trans_color_map, MAP_FLAG_NOFLAGS);
	else
		RefrColor = frw::Value(trans_color.r, trans_color.g, trans_color.b);

	frw::Value RefrRoughness;
	if (trans_roughness_lock)
		RefrRoughness = Roughness;
	else
	{
		if (trans_rough_map_on && trans_rough_map)
			RefrRoughness = createMap(trans_rough_map, MAP_FLAG_NOGAMMA);
		else
			RefrRoughness = frw::Value(trans_roughness, trans_roughness, trans_roughness);
		if (trans_roughness_inv)
			RefrRoughness = materialSystem.ValueSub(frw::Value(1.f), RefrRoughness);
	}

	frw::Value RefrIor;
	if (trans_ior_map_on && trans_ior_map)
		RefrIor = createMap(trans_ior_map, MAP_FLAG_NOGAMMA);
	else
		RefrIor = frw::Value(trans_ior, trans_ior, trans_ior);
	
	RefrShader.SetValue("color", RefrColor);
	RefrShader.SetValue("roughness", RefrRoughness);
	RefrShader.SetValue("ior", thin_walled ? frw::Value(1.05, 1.05, 1.05) : RefrIor);
	RefrShader.SetValue("normal", BaseBumpMap);
	
	// COMBINE DIFFUSE AND REFRACTION
	frw::Shader DiffRefrShader = materialSystem.ShaderBlend(BaseShader, RefrShader, RefrWeight);
	
	// REFLECTIONS
	frw::Shader ReflShader(materialSystem, frw::ShaderTypeWard);
	
	frw::Value ReflWeight;
	if (reflectivity_map_on && reflectivity_map)
		ReflWeight = createMap(reflectivity_map, MAP_FLAG_NOGAMMA);
	else
		ReflWeight = frw::Value(reflectivity, reflectivity, reflectivity);

	frw::Value ReflColor;
	if (refl_color_map_on && refl_color_map)
		ReflColor = createMap(refl_color_map, MAP_FLAG_NOFLAGS);
	else
		ReflColor = frw::Value(refl_color.r, refl_color.g, refl_color.b);
		
	frw::Value Anisotropy;
	if (anisotropy_map_on && anisotropy_map)
		Anisotropy = createMap(anisotropy_map, MAP_FLAG_NOGAMMA);
	else
		Anisotropy = frw::Value(anisotropy, anisotropy, anisotropy);
	
	frw::Value AnisoAngle;
	if (aniso_angle_map_on && aniso_angle_map)
		AnisoAngle = createMap(aniso_angle_map, MAP_FLAG_NOGAMMA);
	else
		AnisoAngle = frw::Value(anisoangle, anisoangle, anisoangle);
	AnisoAngle = materialSystem.ValueMul(AnisoAngle, frw::Value(2.0 * PI));
	
	//aniso_mode

	ReflShader.SetValue("rotation", AnisoAngle);
	ReflShader.SetValue("roughness_y", Roughness);
	ReflShader.SetValue("roughness_x", materialSystem.ValueMul(Anisotropy, Roughness));
	ReflShader.SetValue("normal", BaseBumpMap);
	
	// COMBINE DIFFUSE-REFRACTION WITH REFLECTION
	frw::Value ReflBlendWeight;
	if (brdf_mode == 1)
	{
		ReflBlendWeight = materialSystem.ValueFresnel(RefrIor);
	}
	else
	{
		ReflBlendWeight = ReflWeight;
		frw::Value low(brdf_low);
		frw::Value high(brdf_high);
		frw::Value edge_factor(brdf_curve);
		frw::Value edge_level = materialSystem.ValuePow(1.0 - materialSystem.ValueAbs(materialSystem.ValueDot(materialSystem.ValueLookupINVEC(), materialSystem.ValueLookupN())), edge_factor);
		ReflBlendWeight = materialSystem.ValueMul(ReflBlendWeight, materialSystem.ValueAdd(materialSystem.ValueMul(high, edge_level), materialSystem.ValueMul(low, materialSystem.ValueSub(1.0, edge_level))));
	}
	
	ReflShader.SetValue("color", ReflColor);
	
	frw::Shader DiffRefrReflShader = materialSystem.ShaderBlend(DiffRefrShader, ReflShader, materialSystem.ValueMul(ReflWeight, ReflBlendWeight));


	// METALLIC
	frw::Shader ReflShaderM(materialSystem, frw::ShaderTypeWard);
	ReflShaderM.SetValue("color", BaseColor);
	ReflShaderM.SetValue("rotation", AnisoAngle);
	ReflShaderM.SetValue("roughness_y", Roughness);
	ReflShaderM.SetValue("roughness_x", materialSystem.ValueMul(Anisotropy, Roughness));
	ReflShaderM.SetValue("normal", BaseBumpMap);
	frw::Shader MetallicShader = materialSystem.ShaderBlend(ReflShaderM, ReflShader, materialSystem.ValueMul(ReflWeight, ReflBlendWeight));

	// COMBINE BASE AND METALLIC
	frw::Value Metalness;
	if (metalness_map_on && metalness_map)
	{
		Metalness = createMap(metalness_map, MAP_FLAG_NOGAMMA);
	}
	else
		Metalness = frw::Value(metalness, metalness, metalness);
	frw::Shader DiffRefrReflMetallicShader = materialSystem.ShaderBlend(DiffRefrReflShader, MetallicShader, Metalness);

	// CLEAR COAT
	frw::Shader CoatShader(materialSystem, frw::ShaderTypeMicrofacet);

	frw::Value CoatColor;
	if (coat_color_map_on && coat_color_map)
		CoatColor = createMap(coat_color_map, MAP_FLAG_NOFLAGS);
	else
		CoatColor = frw::Value(coat_color.r, coat_color.g, coat_color.b);
	
	frw::Value CoatIor(coat_ior, coat_ior, coat_ior);

	CoatShader.SetValue("roughness", CoatRoughness);
	CoatShader.SetValue("color", frw::Value(1.f, 1.f, 1.f));
		
	frw::Value CoatBumpMap;
	if (coat_bump_map_on && coat_bump_map)
	{
		CoatBumpMap = FRMTLCLASSNAME(NormalMtl)::translateGenericBump(mT, coat_bump_map, clearcoat_bump_map_amt, *this);
	}
	CoatShader.SetValue("normal", CoatBumpMap);

	BaseColor = materialSystem.ValueBlend(BaseColor, materialSystem.ValueMul(BaseColor, CoatColor), CoatWeight);
	BaseShader.SetValue("color", BaseColor);
	
	// COMBINE COAT IN
	frw::Shader DiffRefrReflCoatShader = materialSystem.ShaderBlend(DiffRefrReflMetallicShader, CoatShader, materialSystem.ValueMul(materialSystem.ValueFresnel(CoatIor), CoatWeight));
		
	// EMISSION
	if ((emission_map_on && emission_map) || (emission > 0.f))
	{
		shaderData.mNumEmissive++;

		frw::EmissiveShader EmissionShader(materialSystem);
		
		frw::Value Emission;
		if (emission_map_on && emission_map)
			Emission = createMap(emission_map, MAP_FLAG_WANTSHDR);
		else
			Emission = frw::Value(emission, emission, emission);

		frw::Value EmissionColor;
		if (emit_color_map_on && emit_color_map)
			EmissionColor = createMap(emit_color_map, MAP_FLAG_WANTSHDR);
		else
			EmissionColor = frw::Value(emit_color.r, emit_color.g, emit_color.b);

		float watts = emit_luminance * (1.f / 683.f);
		frw::Value EmissionLuminance(watts, watts, watts);

		Color kcolor = KelvinToColor(emit_kelvin);
		frw::Value EmissionTemperature(kcolor.r, kcolor.g, kcolor.b);

		EmissionColor = materialSystem.ValueMul(EmissionColor, EmissionTemperature);
		EmissionColor = materialSystem.ValueMul(EmissionColor, EmissionLuminance);

		EmissionShader.SetColor(EmissionColor);

		// COMBINE EMISSION

		DiffRefrReflCoatShader = materialSystem.ShaderBlend(DiffRefrReflCoatShader, EmissionShader, Emission);
	}

	// CUTOUT
	frw::Shader FinalShader = DiffRefrReflCoatShader;
	if (cutout_map_on && cutout_map)
	{
		frw::Shader Transp(materialSystem, frw::ShaderTypeTransparent);
		auto map = createMap(cutout_map, MAP_FLAG_NOGAMMA);
		Transp.SetValue("color", frw::Value(1.0, 1.0, 1.0));
		FinalShader = materialSystem.ShaderBlend(Transp, FinalShader, map);
	}
	
	return FinalShader;
	

	return BaseShader;
}

frw::Shader MaterialParser::parseCoronaMtl(Mtl* mtl)
{
	FASSERT(mtl);
	IParamBlock2* pb = mtl->GetParamBlock(0);

	// Get all relevant Corona parameters

	const float lDiffuse = GetFromPb<float>(pb, Corona::MTLP_LEVEL_DIFFUSE, this->mT);
	const float lReflect = GetFromPb<float>(pb, Corona::MTLP_LEVEL_REFLECT, this->mT);
	const float lRefract = GetFromPb<float>(pb, Corona::MTLP_LEVEL_REFRACT, this->mT);
	const float lEmissive = GetFromPb<float>(pb, Corona::MTLP_LEVEL_SELF_ILLUM, this->mT);
	const float lOpacity = GetFromPb<float>(pb, Corona::MTLP_LEVEL_OPACITY, this->mT);

	GETSHADERMAP_USE_AMOUNT(diffuseColorMap, Corona::MTLP_USEMAP_DIFFUSE, Corona::MTLP_TEXMAP_DIFFUSE, Corona::MTLP_MAPAMOUNT_DIFFUSE)
	GETSHADERCOLOR_NOMAP(diffuseColor, Corona::MTLP_COLOR_DIFFUSE, this->mT)
	GETSHADERMAP_USE_AMOUNT(reflectColorMap, Corona::MTLP_USEMAP_REFLECT, Corona::MTLP_TEXMAP_REFLECT, Corona::MTLP_MAPAMOUNT_REFLECT)
	GETSHADERCOLOR_NOMAP(reflectColor, Corona::MTLP_COLOR_REFLECT, this->mT)
	GETSHADERMAP_USE_AMOUNT(refractColorMap, Corona::MTLP_USEMAP_REFRACT, Corona::MTLP_TEXMAP_REFRACT, Corona::MTLP_MAPAMOUNT_REFRACT)
	GETSHADERCOLOR_NOMAP(refractColor, Corona::MTLP_COLOR_REFRACT, this->mT)
	GETSHADERCOLOR_USE_AMOUNT(opacityColor, Corona::MTLP_COLOR_OPACITY, Corona::MTLP_USEMAP_OPACITY, Corona::MTLP_TEXMAP_OPACITY, Corona::MTLP_MAPAMOUNT_OPACITY)
	GETSHADERCOLOR_USE_AMOUNT(emissiveColor, Corona::MTLP_COLOR_SELF_ILLUM, Corona::MTLP_USEMAP_SELF_ILLUM, Corona::MTLP_TEXMAP_SELF_ILLUM, Corona::MTLP_MAPAMOUNT_SELF_ILLUM)
	
	frw::Shader material = frw::Shader(materialSystem, frw::ShaderTypeUber);

	// BUMP
	
	frw::Value bumpMap;
	if (GetFromPb<bool>(pb, Corona::MTLP_USEMAP_BUMP, this->mT))
	{
		Texmap* normalTexmap = GetFromPb<Texmap*>(pb, Corona::MTLP_TEXMAP_BUMP);
		if (normalTexmap)
		{
				float bumpAmount = GetFromPb<float>(pb, Corona::MTLP_MAPAMOUNT_BUMP);
		bumpMap = FRMTLCLASSNAME(NormalMtl)::translateGenericBump(mT, normalTexmap, bumpAmount, *this);
		material.SetValue("diffuse.normal", bumpMap);
		material.SetValue("clearcoat.normal", bumpMap);
		material.SetValue("glossy.normal", bumpMap);
		material.SetValue("refraction.normal", bumpMap);
	}
	}

	GETSHADERFLOAT_USE_AMOUNT(glossinessSpecular, Corona::MTLP_REFLECT_GLOSSINESS, Corona::MTLP_USEMAP_REFLECT_GLOSSINESS, Corona::MTLP_TEXMAP_REFLECT_GLOSSINESS, Corona::MTLP_MAPAMOUNT_REFLECT_GLOSSINESS)
	GETSHADERFLOAT_USE_AMOUNT(fresnelIor, Corona::MTLP_FRESNEL_IOR, Corona::MTLP_USEMAP_FRESNEL_IOR, Corona::MTLP_TEXMAP_FRESNEL_IOR, Corona::MTLP_MAPAMOUNT_FRESNEL_IOR)
	GETSHADERFLOAT_USE_AMOUNT(ior, Corona::MTLP_IOR, Corona::MTLP_USEMAP_IOR, Corona::MTLP_TEXMAP_IOR, Corona::MTLP_MAPAMOUNT_IOR)
	GETSHADERFLOAT_USE_AMOUNT(glossinessRefraction, Corona::MTLP_REFRACT_GLOSSINESS, Corona::MTLP_USEMAP_REFRACT_GLOSSINESS, Corona::MTLP_TEXMAP_REFRACT_GLOSSINESS, Corona::MTLP_MAPAMOUNT_REFRACT_GLOSSINESS)
	GETSHADERFLOAT_USE_AMOUNT(anisotropy, Corona::MTLP_ANISOTROPY, Corona::MTLP_USEMAP_ANISOTROPY, Corona::MTLP_TEXMAP_ANISOTROPY, Corona::MTLP_MAPAMOUNT_ANISOTROPY)
	GETSHADERFLOAT_USE_AMOUNT(rotation, Corona::MTLP_ANISO_ROTATION, Corona::MTLP_USEMAP_ANISO_ROTATION, Corona::MTLP_TEXMAP_ANISO_ROTATION, Corona::MTLP_MAPAMOUNT_ANISO_ROTATION)
	
	const bool thin = GetFromPb<bool>(pb, Corona::MTLP_TWOSIDED, this->mT);
	
	// DIFFUSE
	
	if (diffuseColorMap.IsNull())
		material.SetValue("diffuse.color", materialSystem.ValueMul(diffuseColor, lDiffuse));
	else
	{
		frw::Value diff(materialSystem.ValueMul(diffuseColor, lDiffuse));
		frw::Value difftex(materialSystem.ValueMul(diffuseColorMap, lDiffuse));
		material.SetValue("diffuse.color", materialSystem.ValueBlend(diff, difftex, diffuseColorMap_mapAmount));
	}

	// REFRACTION
	
	material.SetValue("weights.diffuse2refraction", frw::Value(1.f - lRefract));

	if (refractColorMap.IsNull())
		material.SetValue("refraction.color", materialSystem.ValueMul(refractColor, lRefract));
	else
	{
		frw::Value refr(materialSystem.ValueMul(refractColor, lRefract));
		frw::Value refrtex(materialSystem.ValueMul(refractColorMap, lRefract));
		material.SetValue("refraction.color", materialSystem.ValueBlend(refr, refrtex, refractColorMap_mapAmount));
	}
	material.SetValue("refraction.roughness", materialSystem.ValueSub(frw::Value(1.f), glossinessRefraction));
	material.SetValue("refraction.ior", ior);

	// REFLECTION
	//
	if (lReflect > 0.f)
	{
		// corona glossiness to RPR roughness
		// roughly approximated with a gaussian curve
		//= 0.3 * POW(e, -X * X * 6.125)
		frw::Value roughness;
		{
			//= 0.3*EXP(-(A101*A101*0.5*$I$2*$I$2))
			frw::Value x1(glossinessSpecular);
			frw::Value x2(materialSystem.ValueMul(x1, x1));
			x2 = materialSystem.ValueMul(x2, -6.125);
			frw::Value xp(materialSystem.ValuePow(2.718281828459, x2));
			roughness = materialSystem.ValueMul(0.22, xp);
		}

	frw::Value reflColor(materialSystem.ValueMul(reflectColor, lReflect));

	if (reflectColorMap.IsNull())
			material.SetValue("glossy.color", materialSystem.ValueMul(reflectColor, lReflect));
	else
	{
		frw::Value refl(materialSystem.ValueMul(reflectColor, lReflect));
		frw::Value refltex(materialSystem.ValueMul(reflectColorMap, lReflect));
			material.SetValue("glossy.color", materialSystem.ValueBlend(refl, refltex, reflectColorMap_mapAmount));
	}
		material.SetValue("roughness_x", roughness);
		material.SetValue("roughness_y", roughness);

		material.SetValue("weights.glossy2diffuse", materialSystem.ValueFresnel(fresnelIor));
	}
	else
		material.SetValue("weights.glossy2diffuse", frw::Value(0.f));

	material.SetValue("weights.clearcoat2glossy", frw::Value(0.f));

	// OPACITY

	material.SetValue("transparency.color", opacityColor);
	material.SetValue("weights.transparency", materialSystem.ValueSub(frw::Value(1.0), lOpacity));

	DumpFRContents(material.Handle());

	return material;
}

frw::Shader MaterialParser::parseCoronaPortalMtl(Mtl* mtl) 
{
	FASSERT(mtl);
	frw::Shader transparent(materialSystem, frw::ShaderTypeTransparent);
	transparent.SetValue("color", frw::Value(1.0f, 1.0f, 1.0f));
	return transparent;
}

frw::Shader MaterialParser::parseCoronaVolumeMtl(Mtl* mtl) 
{
	FASSERT(mtl);
	frw::Shader transparent(materialSystem, frw::ShaderTypeTransparent);
	transparent.SetValue("color", frw::Value(1.0f, 1.0f, 1.0f));
	return transparent;
}

// Since RPR does not support ray switching, we just parse the "base" sub-material
frw::Shader MaterialParser::parseCoronaRaySwitchMtl(Mtl* mtl) 
{
	return createShader(GetFromPb<Mtl*>(mtl->GetParamBlock(0), Corona::RM_GI_MTL));
}

frw::Shader FireRender::MaterialParser::parseCoronaLightObject(INode* node)
{
	Corona::IFireMaxLightInterface* ip = dynamic_cast<Corona::IFireMaxLightInterface*>(node->GetObjectRef()->GetInterface(Corona::IFIREMAX_LIGHT_INTERFACE));

	int res;

	shaderData.mNumEmissive++;

	frw::EmissiveShader emissive(materialSystem);

	if (ip) 
	{
		Color color;
		float intensity;
		Texmap* texmap;

		Matrix3 tm = node->GetObjTMAfterWSM(mT);
		tm.SetTrans(tm.GetTrans() * float(GetUnitScale()));
		ip->fmGetRenderColor(mT, tm, color, intensity, texmap);
		color *= intensity;
		emissive.SetColor(color);
	}
	else 
	{
		MessageBox(GetCOREInterface()->GetMAXHWnd(), _T("Too old Corona version."), _T("Radeon ProRender warning"), MB_OK);
	}

	return emissive;
}

frw::Shader MaterialParser::parseCoronaShadowCatcherMtl(Mtl* mtl)
{
	FASSERT(mtl);
	frw::DiffuseShader material(materialSystem);

	return material;
}

frw::Shader MaterialParser::parseCoronaLayeredMtl(Mtl *mtl, INode *node)
{
	FASSERT(mtl);
	
	IParamBlock2* pb = mtl->GetParamBlock(0);

	Mtl *baseMaterial = GetFromPb<Mtl*>(pb, Corona::LAYEREDMTL_BASEMTL, this->mT);
	if (!baseMaterial)
		return frw::DiffuseShader(materialSystem);
	frw::Shader layered = createShader(baseMaterial, node);

	for (int i = 0; i < 10; i++)
	{
		Mtl *material = GetFromPb<Mtl*>(pb, Corona::LAYEREDMTL_LAYERS, this->mT, 0, i);
		if (material)
		{
			frw::Value weight;
			Texmap *mixMap = GetFromPb<Texmap*>(pb, Corona::LAYEREDMTL_MIXMAPS, this->mT, 0, i);
			if (!mixMap)
				weight = frw::Value(GetFromPb<float>(pb, Corona::LAYEREDMTL_AMOUNTS, this->mT, 0, i));
			else
				weight = createMap(mixMap, MAP_FLAG_NOGAMMA);
			frw::Shader mat = createShader(material, node);
			layered = materialSystem.ShaderBlend(layered, mat, weight);
		}
	}
	
	return layered;
}

frw::Shader MaterialParser::parseCoronaLightMtl(Mtl* mtl)
{
	FASSERT(mtl);
	IParamBlock2* pb = mtl->GetParamBlock(0);

	Texmap* texmap = GetFromPb<bool>(pb, Corona::LM_TEXMAP_ON, this->mT) ? GetFromPb<Texmap*>(pb, Corona::LM_TEXMAP) : NULL;
	Color color = GetFromPb<Color>(pb, Corona::LM_COLOR, this->mT);
	float intensity = pb->GetFloat(Corona::LM_INTENSITY, this->mT);

	shaderData.mNumEmissive++;

	frw::EmissiveShader emissive(materialSystem);

	emissive.SetColor(materialSystem.ValueMul(createMap(texmap, MAP_FLAG_WANTSHDR) | color, intensity));

	return emissive;
}

/// Parse the standard 3ds Max Blend material - create two sub-material shaders and mix them in one layered shader
frw::Shader MaterialParser::parseBlendMtl(Mtl* mtl)
{
	IParamBlock2* mPblock = mtl->GetParamBlockByID(mixmat_params);

	Mtl* mtl1 = GetFromPb<bool>(mPblock, mixmat_map1_on, this->mT) ? GetFromPb<::Mtl*>(mPblock, mixmat_map1, this->mT) : NULL;
	Mtl* mtl2 = GetFromPb<bool>(mPblock, mixmat_map2_on, this->mT) ? GetFromPb<::Mtl*>(mPblock, mixmat_map2, this->mT) : NULL;
	Texmap* mixMap = GetFromPb<bool>(mPblock, mixmat_mask_on, this->mT) ? GetFromPb<Texmap*>(mPblock, mixmat_mask, this->mT) : NULL;
	const float amount = GetFromPb<float>(mPblock, mixmat_mix, this->mT);

	const bool use1 = mtl1 != NULL;
	const bool use2 = mtl2 != NULL;

	auto shader1 = use1 ? createShader(mtl1) : frw::Shader();
	auto shader2 = use2 ? createShader(mtl2) : frw::Shader();

	// Optimizations when one or both sub-materials are NULL
	if (!use1 && !use2)
		return frw::DiffuseShader(materialSystem);	
	else if (use1 && !use2)
		return shader1;
	else if (!use1 && use2)
		return shader2;

	FASSERT(use1 && use2);

	auto mixValue = materialSystem.ValueSub(1.0, getValue(mixMap, amount));

	return materialSystem.ShaderBlend(shader2, shader1, mixValue);
}


Matrix3 MaterialParser::getOutputTm(ReferenceTarget *p)
{
	Matrix3 tm;
	tm.IdentityMatrix();

	auto texout = dynamic_cast<TextureOutput*>(p);
	if (!texout)
		texout = GetSubAnimByType<TextureOutput>(p);

	if (texout)
	{
		AColor x, y, z, w;
		x = texout->Filter(AColor(1, 0, 0, 0));
		y = texout->Filter(AColor(0, 1, 0, 0));
		z = texout->Filter(AColor(0, 0, 1, 0));
		w = texout->Filter(AColor(0, 0, 0, 0));
		
		x = x - w;
		y = y - w;
		z = z - w;

		tm.SetRow(0, Point3(x.r, x.g, x.b));
		tm.SetRow(1, Point3(y.r, y.g, y.b));
		tm.SetRow(2, Point3(z.r, z.g, z.b));
		tm.SetRow(3, Point3(w.r, w.g, w.b));

		// actual RGB level
		// float level = texout->GetOutputLevel(0); 
		// if (level < 1.f)
		// 	tm.Scale(Point3(level, level, level));

		tm.ValidateFlags();
	}


	return tm;
}

frw::Value MaterialParser::transformValue(ICurveCtl* curves, frw::Value v)
{
	if (!v)
		return v;

	if (curves && curves->GetNumCurves() > 0)
	{
		if (auto curve = curves->GetControlCurve(0))
		{
			if (curve->GetNumPts() == 1) // this probably never happens
				v = curve->GetValue(mT, 0);
			else if (curve->GetNumPts() == 2 
				&& !(curve->GetPoint(mT, 0).flags & CURVEP_BEZIER)
				&& !(curve->GetPoint(mT, 1).flags & CURVEP_BEZIER)
				)
			{
				auto a = curve->GetPoint(mT, 0);
				auto b = curve->GetPoint(mT, 1);
				if (a.p != Point2(0, 0) || b.p != Point2(1, 1))	// avoid unneeded transform
				{
					auto mT = materialSystem.ValueMul(materialSystem.ValueSub(v, a.p.x), 1 / (b.p.x - a.p.x));

					if (curve->GetOutOfRangeType() == CURVE_EXTRAPOLATE_LINEAR)
						v = materialSystem.ValueMix(a.p.y, b.p.y, mT);
					else
						v = materialSystem.ValueBlend(a.p.y, b.p.y, mT);
				}
			}
			else 	// ok we want to just map it at this point
			{
				rpr_image_desc imgDesc = {};
				int dim = 256;

				imgDesc.image_width = dim;
				imgDesc.image_height = 1;

				std::vector<float> buffer(dim);	// floating point pixel data

				for (int i = 0; i < dim; i++)
				{
					auto& px = buffer[i];
					px = curve->GetValue(mT, i * 1.0 / dim);
				}

				frw::Image image(mScope, { 1, RPR_COMPONENT_TYPE_FLOAT32 }, imgDesc, buffer.data());
				frw::ImageNode node(materialSystem);
				node.SetMap(image);
				node.SetValue("uv", v * (1.0 - 2.0/dim) + 1.0/dim);
				v = node;
			}
		}
	}
	return v;
}

namespace
{
	frw::Value spheremap(frw::MaterialSystem& ms, float* camera_pos)
	{
		auto dir = ms.ValueNormalize(ms.ValueSub(ms.ValueLookupP(),
			frw::Value(camera_pos[0], camera_pos[1], camera_pos[2])));

		auto dir_x = ms.ValueSelectX(dir);
		auto dir_y = ms.ValueSelectY(dir);
		auto dir_z = ms.ValueSelectZ(dir);

		auto longitude = ms.ValueArcTan(dir_x, ms.ValueMul(dir_y, -1));

		auto latitude = ms.ValueArcTan(dir_z,
			ms.ValuePow(
				ms.ValueAdd(
					ms.ValuePow(dir_x, 2),
					ms.ValuePow(dir_y, 2)),
				0.5)
		);

		return ms.ValueAdd(ms.ValueDiv(
			ms.ValueCombine(longitude, latitude * 2),
			M_PI * 2), 0.5);
	}
}

// get the uv function for this map
frw::Value MaterialParser::getTexmapUV(Texmap *texmap, bool noDefault)
{
	Matrix3 tm;
	tm.IdentityMatrix();

	frw::Value uv;
	auto uvMap = UVMAP_EXPLICIT;
	auto uvwSrc = UVWSRC_EXPLICIT;
	bool isMappingTypeEnvironment = false;

	if (auto uvGen = texmap->GetTheUVGen())
	{
		isMappingTypeEnvironment = (uvGen->GetSlotType() == MAPSLOT_ENVIRON);

		if (auto gen = dynamic_cast<StdUVGen*>(uvGen))
			uvMap = gen->GetCoordMapping(0);

		uvwSrc = uvGen->GetUVWSource();

		if(uvwSrc == UVWSRC_EXPLICIT)
			uv = materialSystem.ValueLookupUV(uvGen->GetMapChannel() - 1);

		uvGen->GetUVTransform(tm);
	}
	else if (auto xyzGen = texmap->GetTheXYZGen())
	{
		if (auto gen = dynamic_cast<StdXYZGen*>(xyzGen))
		{
			switch (gen->GetCoordSystem())
			{
			case XYZ_COORDS: uvwSrc = UVWSRC_OBJXYZ; break;
			case UVW_COORDS: uvwSrc = UVWSRC_EXPLICIT; break;
			case UVW2_COORDS: uvwSrc = UVWSRC_EXPLICIT2; break;
			case XYZ_WORLD_COORDS: uvwSrc = UVWSRC_WORLDXYZ; break;
			}

			Point3 scale(gen->GetScl(0, 0), gen->GetScl(1, 0), gen->GetScl(2, 0));
			Point3 rotate(gen->GetAng(0, 0), gen->GetAng(1, 0), gen->GetAng(2, 0));
			Point3 offset(gen->GetOffs(0, 0), gen->GetOffs(1, 0), gen->GetOffs(2, 0));

			tm.PreTranslate(offset);
			tm.PreRotateX(rotate.x);	// rotate 90 degrees
			tm.PreRotateY(rotate.y);
			tm.PreRotateZ(rotate.z);
			tm.PreScale(scale);

			uv = materialSystem.ValueLookupUV(gen->GetMapChannel() - 1);
		}
	}
	else
	{
		texmap->GetUVTransform(tm);
	}

	if (texmap->ClassID().PartA() == CHECKER_CLASS_ID)
	{
		tm.Scale(Point3(1,1,1) * 0.25f);
		tm.Translate(Point3(1, 1, 1) * 256.0f);
	}

	bool doXform = !IsIdentity(tm);

	if (noDefault || doXform || uvwSrc != UVWSRC_EXPLICIT || isMappingTypeEnvironment)	// || uvMap != UVMAP_EXPLICIT)
	{
		if (isMappingTypeEnvironment)
		{
			if (uvMap == UVMAP_SPHERE_ENV)
			{
				//uv = frw::Value(1.0, 0.0, 1.0);
				//uv = materialSystem.ValueLookupINVEC();
				Point3 org = shaderData.mCameraTransform.GetRow(3);
				auto& ms = materialSystem;
				
				uv = spheremap(ms, org);
			}
			else
			{
				uv = frw::Value(1.0, 0.0, 0.0);
			}
		}
		else
		{
			switch (uvwSrc)
			{
			case UVWSRC_OBJXYZ:	// local object coord space
				uv = materialSystem.ValueLookupP();
				if (shaderData.mNode)
				{
					auto nodeTm = shaderData.mNode->GetObjectTM(mT);
					auto obj = shaderData.mNode->EvalWorldState(mT).obj;
					
					Box3 bbox;
					obj->GetDeformBBox(mT, bbox);
					bbox.Scale(GetUnitScale());
					nodeTm.PreTranslate(bbox.Min());
					nodeTm.PreScale(bbox.Width());

					nodeTm.Invert();
					tm = nodeTm * tm;
					doXform = true;
					shaderData.mCacheable = false;
				}
				break;
			case UVWSRC_WORLDXYZ:
				uv = materialSystem.ValueLookupP();
				break;
			case UVWSRC_EXPLICIT2:
				uv = materialSystem.ValueLookupN();
				break;
			case UVWSRC_FACEMAP:
			case UVWSRC_HWGEN:
			case UVWSRC_EXPLICIT:
			default:
				uv = materialSystem.ValueLookupUV(texmap->GetMapChannel() - 1);
				break;
			}
		}

		if (doXform && uv)
			uv = materialSystem.ValueTransform(uv, tm);
	}

	return uv;
}

frw::Value MaterialParser::createCheckerMap(Texmap *texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(float, soften, 0x0);
	GET_PARAM(Color, color1, 0x1);
	GET_PARAM(Color, color2, 0x2);
	GET_PARAM(Texmap*, map1, 0x3);
	GET_PARAM(Texmap*, map2, 0x4);
	GET_PARAM(bool, map1Enabled, 0x5);
	GET_PARAM(bool, map2Enabled, 0x6);

	auto uv = getTexmapUV(texmap, true);

	frw::ValueNode checker(materialSystem, frw::ValueTypeCheckerMap);
	checker.SetValue("uv", uv);

	float eps = 0.0001f;
	auto a = getValue(map1Enabled ? map1 : nullptr, color1);
	auto b = getValue(map2Enabled ? map2 : nullptr, color2);
	if(a.IsNode() || b.IsNode()
		|| (MaxVal(color1-Color(0, 0, 0))>eps) || (MaxVal(color2-Color(1, 1, 1))>eps)) {
		return materialSystem.ValueBlend(a, b, checker);
	}

	return checker;
}


frw::Value MaterialParser::createNoiseMap(Texmap *texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(Color, color1, 0x0);
	GET_PARAM(Color, color2, 0x1);
	GET_PARAM(Texmap*, map1, 0x2);
	GET_PARAM(Texmap*, map2, 0x3);
	GET_PARAM(bool, map1Enabled, 0x4);
	GET_PARAM(bool, map2Enabled, 0x5);
	GET_PARAM(float, size, 0x6);
	GET_PARAM(float, phase, 0x7);
	GET_PARAM(float, levels, 0x8);
	GET_PARAM(float, thresholdLow, 0x9);
	GET_PARAM(float, thresholdHigh, 0xA);
	GET_PARAM(int, type, 0xB);
	GET_PARAM(ReferenceTarget*, output, 0xD)

	float min = 0.25 - thresholdLow;
	float max = 0.75 + (1 - thresholdHigh);

	size = 33 / size;

	auto uv = getTexmapUV(texmap, true);

	if (phase != 0)
		uv = materialSystem.ValueAdd(uv, phase);

	if (size != 1)
		uv = materialSystem.ValueMul(uv, size);

	frw::Value v;

	float eps = 0.0001f;

	{	// do first harmonic
		frw::ValueNode noise_node(materialSystem, frw::ValueTypeNoiseMap);
		noise_node.SetValue("uv", uv);

		//optimize out blend node - see AMDMAX-920
		if( (fabs(min-0)>eps) || (fabs(max-1)>eps))
			v = materialSystem.ValueBlend(min, max, noise_node);
		else
			v = noise_node;
	}


	auto a = getValue(map1Enabled ? map1 : nullptr, color1);
	auto b = getValue(map2Enabled ? map2 : nullptr, color2);

	//optimize out blend node - see AMDMAX-920
	bool color1IsNotWhiteOrBlack = MaxVal(color1) > eps && MinVal(color1) < 1 - eps;
	bool color2IsNotWhiteOrBlack = MaxVal(color2) > eps && MinVal(color2) < 1 - eps;
	if (a.IsNode() || b.IsNode() || color1IsNotWhiteOrBlack || color2IsNotWhiteOrBlack)
		v = materialSystem.ValueBlend(a, b, v);

	auto tx = getOutputTm(output);
	if(!tx.IsIdentity())
		v = materialSystem.ValueTransform(v, tx);

	return v;
}

namespace
{
	enum ChannelSource
	{
		RED,
		GREEN,
		BLUE,
		ALPHA,
		RED_INV,
		GREEN_INV,
		BLUE_INV,
		ALPHA_INV,
		MONO,
		ONE,
		ZERO,
	};

	Point4 GetColorChannel(int src)
	{
		switch (src)
		{
		case ChannelSource::RED:
			return Point4(1, 0, 0, 0);
		case ChannelSource::GREEN:
			return Point4(0, 1, 0, 0);
		case ChannelSource::BLUE:
			return Point4(0, 0, 1, 0);
		case ChannelSource::RED_INV:
			return Point4(-1, 0, 0, 1);
		case ChannelSource::GREEN_INV:
			return Point4(0, -1, 0, 1);
		case ChannelSource::BLUE_INV:
			return Point4(0, 0, -1, 1);
		case ChannelSource::MONO:
			return Point4(1, 1, 1, 0) * (1.0/3);
		case ChannelSource::ONE:
			return Point4(0, 0, 0, 1);
		case ChannelSource::ZERO:
		default:
			return Point4(0, 0, 0, 0);
		}
	}
}

frw::Value MaterialParser::createColorCorrectionMap(Texmap *texmap)
{
	auto& ms = materialSystem;

	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(Point4, color, 0x0);
	GET_PARAM(Texmap*, map, 0x1);
	GET_PARAM(int, rewireMode, 0x2);
	GET_PARAM(int, rewireR, 0x3);
	GET_PARAM(int, rewireG, 0x4);
	GET_PARAM(int, rewireB, 0x5);
	GET_PARAM(int, rewireA, 0x6);

	GET_PARAM(float, hueShift, 0x7);
	GET_PARAM(float, saturation, 0x8);
	GET_PARAM(Point4, tint, 0x9);
	GET_PARAM(float, tintStrength, 0xA);
	GET_PARAM(int, lightnessMode, 0xB);
	GET_PARAM(float, contrast, 0xD);
	GET_PARAM(float, brightness, 0xC);


	// NOT IMPLEMENTED: photographic exposure stuff
	//GET_PARAM(01, exposureMode, 0xE);
	//GET_PARAM(04, enableR, 0xF);
	//GET_PARAM(04, enableG, 0x10);
	//GET_PARAM(04, enableB, 0x11);
	//GET_PARAM(00, gainRGB, 0x12);
	//GET_PARAM(00, gainR, 0x13);
	//GET_PARAM(00, gainG, 0x14);
	//GET_PARAM(00, gainB, 0x15);
	//GET_PARAM(00, gammaRGB, 0x16);
	//GET_PARAM(00, gammaR, 0x17);
	//GET_PARAM(00, gammaG, 0x18);
	//GET_PARAM(00, gammaB, 0x19);
	//GET_PARAM(00, pivotRGB, 0x1A);
	//GET_PARAM(00, pivotR, 0x1B);
	//GET_PARAM(00, pivotG, 0x1C);
	//GET_PARAM(00, pivotB, 0x1D);
	//GET_PARAM(00, liftRGB, 0x1E);
	//GET_PARAM(00, liftR, 0x1F);
	//GET_PARAM(00, liftG, 0x20);
	//GET_PARAM(00, liftB, 0x21);
	//GET_PARAM(00, printerLights, 0x22);

	Matrix3 tm;
	tm.IdentityMatrix();

	switch (rewireMode)
	{
	case 0:
		// leave as identity!
		break;
	case 1:
		saturation = -100.f;
		break;
	default:
		tm.SetColumn(0, GetColorChannel(rewireR));
		tm.SetColumn(1, GetColorChannel(rewireG));
		tm.SetColumn(2, GetColorChannel(rewireB));
		tm.ValidateFlags();
		break;
	}

	saturation = saturation * 0.01f + 1;
	contrast = contrast * 0.01f + 1;
	brightness = brightness * 0.01f + 1;

	if (hueShift != 0 || saturation != 0 || brightness != 0)
	{
		hueShift *= DEG_TO_RAD;
		hueShift *= -1;	

		float H = hueShift;
		float S = saturation;
		float V = brightness;
		
		float VSU = V*S*cos(H);
		float VSW = V*S*sin(H);
		
		Matrix3 tx;
		tx.IdentityMatrix();
		tx.SetColumn(0, Point4(	0.299f * V + 0.701f * VSU + 0.168f * VSW,
								0.587f * V - 0.587f * VSU + 0.330f * VSW,
								0.114f * V - 0.114f * VSU - 0.497f * VSW, 0.f));
		tx.SetColumn(1, Point4(	0.299f * V - 0.299f * VSU - 0.328f * VSW,
								0.587f * V + 0.413f * VSU + 0.035f * VSW,
								0.114f * V - 0.114f * VSU + 0.292f * VSW, 0.f));
		tx.SetColumn(2, Point4(	0.299f * V - 0.3f   * VSU + 1.25f  * VSW,
								0.587f * V - 0.588f * VSU - 1.05f  * VSW,
								0.114f * V + 0.886f * VSU - 0.203f * VSW, 0.f));
		tx.ValidateFlags();
		tm = tm * tx;
	}

	float lo = 0;
	float hi = 1;

	if (contrast != 1)
	{
		lo = (lo - 0.5) * contrast + 0.5;
		hi = (hi - 0.5) * contrast + 0.5;
	}

	if (hi - lo != 1)
		tm.Scale(Point3(1,1,1) * (hi - lo));

	if (lo != 0)
		tm.Translate(Point3(1, 1, 1) * (lo));

	tm.ValidateFlags();
	auto v = getValue(map, frw::Value(color.x, color.y, color.z, color.w));

	// Other color transforms
	v = materialSystem.ValueTransform(v, tm);

	return v;
}

frw::Value MaterialParser::createMixMap(Texmap *texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(float, mixAmount, 0x0);
	GET_PARAM(float, lower, 0x1);
	GET_PARAM(float, upper, 0x2);
	GET_PARAM(bool, useCurve, 0x3);
	GET_PARAM(Color, color1, 0x4);
	GET_PARAM(Color, color2, 0x5);
	GET_PARAM(Texmap*, map1, 0x6);
	GET_PARAM(Texmap*, map2, 0x7);
	GET_PARAM(Texmap*, mask, 0x8);
	GET_PARAM(bool, map1Enabled, 0x9);
	GET_PARAM(bool, map2Enabled, 0xA);
	GET_PARAM(bool, maskEnabled, 0xB);
	GET_PARAM(ReferenceTarget*, output, 0xC);

	auto a = getValue(map1Enabled ? map1 : nullptr, color1);
	auto b = getValue(map2Enabled ? map2 : nullptr, color2);
	auto m = getValue(maskEnabled ? mask : nullptr, mixAmount);

	if (useCurve)
	{
		m = materialSystem.ValueAdd(m, -lower);
		m = materialSystem.ValueMul(m, 1.0 / (upper - lower));
	}

	auto v = materialSystem.ValueBlend(a, b, m);

	auto tm = getOutputTm(output);
	v = materialSystem.ValueTransform(v, tm);

	return v;
}

frw::Value MaterialParser::createOutputMap(Texmap *texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(Texmap*, map1, 0x0);
	GET_PARAM(bool, map1Enabled, 0x1);
	GET_PARAM(ReferenceTarget*, output, 0x2);

	auto v = getValue(map1Enabled ? map1 : nullptr, 1);

	auto tm = getOutputTm(output);
	v = materialSystem.ValueTransform(v, tm);

	return v;
}

frw::Value MaterialParser::createRgbMultiplyMap(Texmap * texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(Color, color1, 0x0);
	GET_PARAM(Color, color2, 0x1);
	GET_PARAM(Texmap*, map1, 0x2);
	GET_PARAM(Texmap*, map2, 0x3);
	GET_PARAM(bool, map1Enabled, 0x4);
	GET_PARAM(bool, map2Enabled, 0x5);

	auto a = getValue(map1Enabled ? map1 : nullptr, color1);
	auto b = getValue(map2Enabled ? map2 : nullptr, color2);

	return materialSystem.ValueMul(a, b);
}

frw::Value MaterialParser::createTintMap(Texmap* texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(Color, red, 0x0);
	GET_PARAM(Color, green, 0x1);
	GET_PARAM(Color, blue, 0x2);
	GET_PARAM(Texmap*, map1, 0x3);
	GET_PARAM(bool, map1Enabled, 0x4);

	auto v = getValue(map1Enabled ? map1 : nullptr, 1);
	return materialSystem.ValueTransform(v, red, green, blue);
}

frw::Value MaterialParser::createMaskMap(Texmap* texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(Texmap*, map, 0x0);
	GET_PARAM(Texmap*, mask, 0x1);
	GET_PARAM(bool, mapEnabled, 0x2);
	GET_PARAM(bool, maskEnabled, 0x3);
	GET_PARAM(bool, maskInverted, 0x4);

	auto a = getValue(mapEnabled ? map : nullptr, 1);
	auto b = getValue(maskEnabled ? mask : nullptr, 1);

	if (maskInverted && b)
	{
		b = materialSystem.ValueSub(1, b);
	}

	return materialSystem.ValueMul(a, b);
}


namespace
{
	// from 3dsmax droplist
	enum BlendMode
	{
		BLEND_NORMAL = 0,
		BLEND_AVERAGE,
		BLEND_ADDITION,
		BLEND_SUBTRACT,
		BLEND_DARKEN,
		BLEND_MULTIPLY,
		BLEND_COLOR_BURN,
		BLEND_LINEAR_BURN,
		BLEND_LIGHTEN,
		BLEND_SCREEN,
		BLEND_COLOR_DODGE,
		BLEND_LINEAR_DODGE,
		BLEND_SPOTLIGHT,
		BLEND_SPOTLIGHT_BLEND,
		BLEND_OVERLAY,
		BLEND_SOFT_LIGHT,
		BLEND_HARD_LIGHT,
		BLEND_PIN_LIGHT,
		BLEND_HARD_MIX,
		BLEND_DIFFERENCE,
		BLEND_EXCLUSION,
		BLEND_HUE,
		BLEND_SATURATION,
		BLEND_COLOR,
		BLEND_VALUE,
	};
}

frw::Value MaterialParser::createNormalBumpMap(Texmap* texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	int np = pb->NumParams();

	BOOL map1on = FALSE;
	Texmap *normalMap = 0;
	float mult = 1.f;

	for (int i = 0; i < np; i++)
	{
		ParamID pid = pb->IndextoID(i);
		ParamDef &pd = pb->GetParamDef(pid);
		if (wcsncmp(pd.int_name, _T("mult_spin"), CMP_SIZE) == 0)
			mult = GetFromPb<float>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("map1on"), CMP_SIZE) == 0)
			map1on = GetFromPb<BOOL>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("normal_map"), CMP_SIZE) == 0)
			normalMap = GetFromPb<Texmap*>(pb, pid, timeVal);
	}

	frw::Value res;

	if (normalMap && map1on)
		res = materialSystem.ValueBlend(materialSystem.ValueLookupN(), createMap(normalMap, MAP_FLAG_NORMALMAP | MAP_FLAG_NOGAMMA), mult);
	else
		res = materialSystem.ValueLookupN();

	return res;
}

frw::Value MaterialParser::createCompositeMap(Texmap* texmap)
{
	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;

	frw::Value v;
	int layers = pb->Count(0x1);
	for (int i = 0; i < layers; i++)
	{
		GET_PARAM_N(bool, mapEnabled, 0x1, i);
		GET_PARAM_N(bool, maskEnabled, 0x3, i);
		GET_PARAM_N(int, blendMode, 0x5, i);
		GET_PARAM_N(float, opacity, 0x8, i);
		GET_PARAM_N(Texmap*, map, 0x9, i);
		GET_PARAM_N(Texmap*, mask, 0xA, i);

		opacity /= 100;

		auto color = getValue(mapEnabled ? map : nullptr);
		if (color.IsNull())
			continue;

		auto k = getValue(maskEnabled ? mask : nullptr, 1);
		if (opacity != 1)
			k = materialSystem.ValueMul(k, opacity);

		if (!k.NonZeroXYZ())	// mask is empty
			continue;

		if (v.IsNull())	// no previous layers?
			v = materialSystem.ValueMul(color,k);
		else
		{
			switch (blendMode)
			{
			case BLEND_ADDITION:
				color = materialSystem.ValueAdd(color, v);
				break;

			case BLEND_AVERAGE:
				k = materialSystem.ValueMul(k, 0.5);
				break;

			case BLEND_SUBTRACT:
				color = materialSystem.ValueSub(color, v);
				break;

			case BLEND_MULTIPLY:
				color = materialSystem.ValueMul(color, v);
				break;

			case BLEND_SCREEN:
				color = materialSystem.ValueSub(1, materialSystem.ValueMul(materialSystem.ValueSub(1, color), materialSystem.ValueSub(1, v)));
				break;

			case BLEND_DIFFERENCE:
				color = materialSystem.ValueSub(color, v);
				color = materialSystem.ValueAbs(color);
				break;

			case BLEND_NORMAL:
			default:
				break;
			}
			v = materialSystem.ValueMix(v, color, k);
		}
	}

	return v;
}

frw::Value MaterialParser::createGradientMap(Texmap *tm)
{
	IParamBlock2 *pb = tm->GetParamBlock(0);
	TimeValue timeVal = 0;

	GET_PARAM(Color, color1, 0x0);
	GET_PARAM(Color, color2, 0x1);
	GET_PARAM(Color, color3, 0x2);

	GET_PARAM(Texmap*, map1, 0x3);
	GET_PARAM(Texmap*, map2, 0x4);
	GET_PARAM(Texmap*, map3, 0x5);
	GET_PARAM(bool, map1Enabled, 0x6);
	GET_PARAM(bool, map2Enabled, 0x7);
	GET_PARAM(bool, map3Enabled, 0x8);
	GET_PARAM(float, color2Pos, 0x9);
	GET_PARAM(int, gradientType, 0xA);

	//  GET_PARAM(00, noiseAmount, 0xB)
	//	GET_PARAM(01, noiseType, 0xC)
	//	GET_PARAM(00, noiseSize, 0xD)
	//	GET_PARAM(00, noisePhase, 0xE)
	//	GET_PARAM(00, noiseLevels, 0xF)
	//	GET_PARAM(00, noiseThresholdLow, 0x10)
	//	GET_PARAM(00, noiseThresholdHigh, 0x11)
	//	GET_PARAM(00, noiseThresholdSMooth, 0x12)
	//	GET_PARAM(18, coords, 0x13)

	GET_PARAM(ReferenceTarget*, output, 0x14);

	frw::Value a = getValue(map1Enabled ? map1 : nullptr, color1);
	frw::Value b = getValue(map2Enabled ? map2 : nullptr, color2);
	frw::Value c = getValue(map3Enabled ? map3 : nullptr, color3);

	frw::Value v = getTexmapUV(tm);

	if (v.IsNull())
		materialSystem.ValueLookupUV(tm->GetMapChannel() - 1);

	if (gradientType)
	{
		v = materialSystem.ValueAbs(materialSystem.ValueMod(v, 1));
		v = materialSystem.ValueSub(frw::Value(0.5, 0.5, 0.0, 0.0), v);
		v = materialSystem.ValueMul(v, 2.0);
		v = materialSystem.ValueMagnitude(v);
		frw::Value lerp0 = materialSystem.ValueClamp(materialSystem.ValueDiv(v, color2Pos + 1e-5));
		frw::Value lerp1 = materialSystem.ValueClamp(materialSystem.ValueDiv(materialSystem.ValueClamp(materialSystem.ValueSub(v,color2Pos)), materialSystem.ValueSub(1, color2Pos)));
		return materialSystem.ValueBlend(materialSystem.ValueBlend(c,b, lerp0), a, lerp1);
	}
	else
	{
		// correct for sanity
		color2Pos = 1 - color2Pos;

		rpr_image_desc imgDesc = {};
		int dim = 128;

		imgDesc.image_width = 1;
		imgDesc.image_height = dim;

		std::vector<RgbFloat32> buffer(dim);	// floating point pixel data

		int mid = color2Pos * dim;
		float k = 1.0;

		for (int i = 0; i < dim; i++)
		{
			auto& px = buffer[i];
			if (i < mid)	// first half
			{
				px.r = k * i / mid;
				px.g = 0;
				px.b = 0;
			}
			else
			{
				px.r = 1;
				px.g = k * (i - mid) / (dim - mid);
				px.b = 0;
			}
		}

		frw::Image image(mScope, { 3, RPR_COMPONENT_TYPE_FLOAT32 }, imgDesc, buffer.data());
		frw::ImageNode node(materialSystem);
		node.SetMap(image);
		node.SetValue("uv", v);
		v = materialSystem.ValueBlend(materialSystem.ValueBlend(a, b, materialSystem.ValueSelectX(node)), c, materialSystem.ValueSelectY(node));
	}

	auto tx = getOutputTm(output);
	v = materialSystem.ValueTransform(v, tx);

	return v;
}

frw::Value MaterialParser::createTextureMap(Texmap *texmap, int flags)
{
	TimeValue timeVal = 0;
	frw::Value v;

	if (auto map = dynamic_cast<BitmapTex*>(texmap->GetInterface(BITMAPTEX_INTERFACE)))
	{
		if (auto image = createImage(map->GetBitmap(timeVal), HashValue() << texmap << getMaterialHash(texmap, true), flags, map->GetMapName()))
		{
			frw::ImageNode node(materialSystem);
			node.SetMap(image);
			if(auto uv = getTexmapUV(texmap))
				node.SetValue("uv", getTexmapUV(texmap));
			v = node;

			if (auto output = map->GetTexout())
			{
				auto tx = getOutputTm(output);
				if(!tx.IsIdentity()){
					v = materialSystem.ValueTransform(v, tx);
				}
			}
		}
	}

	return v;
}



enum FalloffType
{
	TowAway = 0,
	PerpPar = 1,
	Fresnel = 2,
	Shadow = 3,
	Distance = 4,
};

enum FalloffDirection
{
	CameraZ = 0,
	CameraX = 1,
	CameraY = 2,
	Object = 3,
	LocalX = 4,
	LocalY = 5,
	LocalZ = 6,
	WorldX = 7,
	WorldY = 8,
	WorldZ = 9,
};


frw::Value MaterialParser::createFalloffMap(Texmap *texmap)
{

	IParamBlock2 *pb = texmap->GetParamBlock(0);
	TimeValue timeVal = 0;
	
	const float epsilon = 0.01f;
	Color color1, color2;
	float map1Amount = 1.f, map2Amount = 1.f;
	bool map1On = false;
	bool map2On = false;
	Texmap *map1 = nullptr;
	Texmap *map2 = nullptr;
	FalloffType type = FalloffType::PerpPar;
	FalloffDirection direction = FalloffDirection::CameraZ;
	float ior = 1;
	float nearDistance = 1, farDistance = 10;
	bool mtlIOROverride;
	bool extrapolateOn;
	INode *node = nullptr;
	int np = pb->NumParams();

	ParamID type_id = -1, direction_id = -1, farDistance_id = -1, nearDistance_id = -1, color1_id = -1, color2_id = -1, map1On_id = -1, map2On_id = -1;
	
	for (int i = 0; i < np; i++)
	{
		ParamID pid = pb->IndextoID(i);
		ParamDef &pd = pb->GetParamDef(pid);

		if (wcsncmp(pd.int_name, _T("color1"), CMP_SIZE) == 0)
		{
			color1 = GetFromPb<Color>(pb, pid, timeVal);
			color1_id = pid;
		}
		else if (wcsncmp(pd.int_name, _T("map1Amount"), CMP_SIZE) == 0) // default setting is 100
			map1Amount = GetFromPb<float>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("map1"), CMP_SIZE) == 0)
			map1 = GetFromPb<Texmap *>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("map1On"), CMP_SIZE) == 0)
		{
			map1On = GetFromPb<bool>(pb, pid, timeVal);
			map1On_id = pid;
		}
		else if (wcsncmp(pd.int_name, _T("color2"), CMP_SIZE) == 0)
		{
			color2 = GetFromPb<Color>(pb, pid, timeVal);
			color2_id = pid;
		}
		else if (wcsncmp(pd.int_name, _T("map2Amount"), CMP_SIZE) == 0)
			map2Amount = GetFromPb<float>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("map2"), CMP_SIZE) == 0)
			map2 = GetFromPb<Texmap *>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("map2On"), CMP_SIZE) == 0)
		{
			map2On = GetFromPb<bool>(pb, pid, timeVal);
			map2On_id = pid;
		}
		else if (wcsncmp(pd.int_name, _T("type"), CMP_SIZE) == 0)
		{
			type = static_cast<FalloffType>(GetFromPb<int>(pb, pid, timeVal));
			type_id = pid;
		}
		else if (wcsncmp(pd.int_name, _T("direction"), CMP_SIZE) == 0)
		{
			direction = static_cast<FalloffDirection>(GetFromPb<int>(pb, pid, timeVal));
			direction_id = pid;
		}
		else if (wcsncmp(pd.int_name, _T("node"), CMP_SIZE) == 0)
			node = GetFromPb<INode *>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("mtlIOROverride"), CMP_SIZE) == 0)
			mtlIOROverride = GetFromPb<bool>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("ior"), CMP_SIZE) == 0)
			ior = GetFromPb<float>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("extrapolateOn"), CMP_SIZE) == 0)
			extrapolateOn = GetFromPb<bool>(pb, pid, timeVal);
		else if (wcsncmp(pd.int_name, _T("nearDistance"), CMP_SIZE) == 0)
		{
			nearDistance = GetFromPb<float>(pb, pid, timeVal);
			nearDistance_id = pid;
		}
		else if (wcsncmp(pd.int_name, _T("farDistance"), CMP_SIZE) == 0)
		{
			farDistance = GetFromPb<float>(pb, pid, timeVal);
			farDistance_id = pid;
	}
	}


	debugPrint( std::string("Falloff type: ") + toString(type) + "\n");

	float bias;
	if (map2Amount == 0)
	{
		if (map1Amount == 0)
			bias = .5f; // even
		else
			bias = 1.f;
	}
	else
		bias = (map1Amount / (map1Amount + map2Amount));

	// these are the two values to blend based on falloff

	frw::Value a(color1);
	frw::Value b(color2);

	if (map1On && map1)
		a = createMap(map1, MAP_FLAG_NOFLAGS) | a;

	if (map2On && map2)
		b = createMap(map2, MAP_FLAG_NOFLAGS) | b;

	// optimisation here for bias == 0 or bias == 1 (one color only)

	if (bias <= 0)
		return b;
	else if (bias >= 1)
		return a;

	frw::Value blendVal;

	frw::Value dotValue(1.0);
	frw::Value n(materialSystem.ValueLookupN());

	frw::Value dir(1, 0, 0);
	frw::Value org(0, 0, 0);

	switch (direction)
	{
	case FalloffDirection::WorldX:
		dir = frw::Value(1, 0, 0);
		break;
	case FalloffDirection::LocalY:
	case FalloffDirection::WorldY:
		dir = frw::Value(0, 1, 0);
		break;
	case FalloffDirection::LocalZ:
	case FalloffDirection::WorldZ:
		dir = frw::Value(0, 0, 1);
		break;

	case FalloffDirection::LocalX:

	case FalloffDirection::Object:
		if (node)
		{
			org = node->GetNodeTM(timeVal).PointTransform(Point3(0, 0, 0));
			dir = materialSystem.ValueSub(materialSystem.ValueLookupP(), org);
			dir = materialSystem.ValueNormalize(dir);
			shaderData.mCacheable = false;
		}

		break;
	case FalloffDirection::CameraX:
		org = shaderData.mCameraTransform.GetRow(3);
		dir = shaderData.mCameraTransform.GetRow(0);
		shaderData.mCacheable = false;
		break;

	case FalloffDirection::CameraY:
		org = shaderData.mCameraTransform.GetRow(3);
		dir = shaderData.mCameraTransform.GetRow(1);
		shaderData.mCacheable = false;
		break;

	case FalloffDirection::CameraZ:
		org = shaderData.mCameraTransform.GetRow(3);
		// note that these values are only the same in ortho mode. For correct materials invec is better
		//dir = shaderData.cameraTransform.GetRow(2); 
		dir = materialSystem.ValueLookupINVEC();
		break;

	default:
		dir = materialSystem.ValueLookupINVEC();
		break;
	}

	switch (type)
	{
	case FalloffType::TowAway:
		dotValue = materialSystem.ValueDot(n, dir);
		dotValue = materialSystem.ValueMul(dotValue, 0.5);
		dotValue = materialSystem.ValueAdd(dotValue, 0.5);
		break;

	case FalloffType::Fresnel:
		// should be make to work for arbitrary n & dir
		dotValue = materialSystem.ValueFresnel(ior);// , n, dir);
		std::swap(a, b);
		break;

	case FalloffType::Distance:
	{
		auto v = materialSystem.ValueSub(materialSystem.ValueLookupP(), org);
		v = materialSystem.ValueMagnitude(v);
		v = materialSystem.ValueSub(v, nearDistance);
		v = materialSystem.ValueDiv(v, farDistance - nearDistance);
		dotValue = v;
		shaderData.mCacheable = false;
	} break;

	case FalloffType::PerpPar:
	default:
		dotValue = materialSystem.ValueDot(n, dir);
		dotValue = materialSystem.ValueAbs(dotValue);
		break;
	}


	dotValue = transformValue(GetSubAnimByType<ICurveCtl>(texmap), (1.0 - dotValue));	// MIX CURVE

	blendVal = materialSystem.ValueBlend(a, b, dotValue);
	
	auto tm = getOutputTm(texmap);	// OUTPUT
	blendVal = materialSystem.ValueTransform(blendVal, tm);

	return blendVal;
}

frw::Value MaterialParser::createCoronaMixMap(Texmap* texmap)
{
	IParamBlock2* pb = texmap->GetParamBlock(0);

	frw::Value result;
	
	int operation = GetFromPb<int>(pb, Corona::MIXTEX_MIXOPERATION, this->mT);
	float amount = GetFromPb<float>(pb, Corona::MIXTEX_MIXAMOUNT, this->mT);
	float multiplierTop = GetFromPb<float>(pb, Corona::MIXTEX_MULTIPLIERTOP, this->mT);
	float ScaleBottom = GetFromPb<float>(pb, Corona::MIXTEX_SCALEBOTTOM, this->mT);
	float offsetTop = GetFromPb<float>(pb, Corona::MIXTEX_OFFSETTOP, this->mT);
	float offsetBottom = GetFromPb<float>(pb, Corona::MIXTEX_OFFFSETBOTTOM, this->mT);
	Texmap *texmapTop = GetFromPb<Texmap*>(pb, Corona::MIXTEX_TEXMAPTOP, this->mT);
	Texmap *texmapBottom = GetFromPb<Texmap*>(pb, Corona::MIXTEX_TEXMAPBOTTOM, this->mT);
	Texmap *texmapMix = GetFromPb<Texmap*>(pb, Corona::MIXTEX_TEXMAPMIX, this->mT);
	Color colorBottom = GetFromPb<Color>(pb, Corona::MIXTEX_COLORBOTTOM, this->mT);
	Color colorTop = GetFromPb<Color>(pb, Corona::MIXTEX_COLORTOP, this->mT);
	BOOL texmapMixOn = GetFromPb<BOOL>(pb, Corona::MIXTEX_TEXMAPMIXON, this->mT);
	BOOL texmapTopOn = GetFromPb<BOOL>(pb, Corona::MIXTEX_TEXMAPTOPON, this->mT);
	BOOL texmapBottomOn = GetFromPb<BOOL>(pb, Corona::MIXTEX_TEXMAPBOTTOMON, this->mT);

	// Top color
	frw::Value top;
	if (texmapTopOn && texmapTop)
		top = createMap(texmapTop, MAP_FLAG_NOFLAGS);
	else
		top = frw::Value(colorTop.r, colorTop.g, colorTop.b);
	if (offsetTop != 0.f)
		top = materialSystem.ValueAdd(top, offsetTop);
	if (multiplierTop != 1.f)
		top = materialSystem.ValueMul(top, multiplierTop);

	// Bottom color
	frw::Value bottom;
	if (texmapBottomOn && texmapBottom)
		bottom = createMap(texmapBottom, MAP_FLAG_NOFLAGS);
	else
		bottom = frw::Value(colorBottom.r, colorBottom.g, colorBottom.b);
	if (ScaleBottom != 1.f)
		bottom = materialSystem.ValueMul(bottom, ScaleBottom);
	if (offsetBottom != 0.f)
		bottom = materialSystem.ValueAdd(bottom, offsetBottom);
	
	// Operation
	if (operation == 0)
	{
		frw::Value mix;
		if (texmapMixOn && texmapMix)
			mix = createMap(texmapMix, MAP_FLAG_NOGAMMA);
		else
			mix = frw::Value(amount);
		result = materialSystem.ValueMix(top, bottom, mix);
	}
	
	return result;
}

frw::Value MaterialParser::createCoronaColorMap(Texmap* texmap)
{
	IParamBlock2* pb = texmap->GetParamBlock(0);

	frw::Value result;

	int method = GetFromPb<int>(pb, Corona::COLORTEX_METHOD, this->mT);

	switch (method)
	{
		// Solid color
		case 0:
		{
			Color color = GetFromPb<Color>(pb, Corona::COLORTEX_COLOR, this->mT);
			result = frw::Value(color.r, color.g, color.b);
		} break;

		// solid HDR
		case 1:
		{
			Point3 colorHDR = GetFromPb<Point3>(pb, Corona::COLORTEX_COLORHDR, this->mT);
			result = frw::Value(colorHDR.x, colorHDR.y, colorHDR.z);
		} break;

		// Temperature
		case 2:
		{
			float kelvin = GetFromPb<float>(pb, Corona::COLORTEX_TEMPERATURE, this->mT);
			Color col = KelvinToColor(kelvin);
			result = frw::Value(col.r, col.g, col.b);
		} break;

		// HEX
		case 3:
		{
			const wchar_t *colorHEX = pb->GetStr(Corona::COLORTEX_HEXCOLOR, this->mT);
			if (wcslen(colorHEX) == 7 && colorHEX[0] == '#')
			{
				
			}
		} break;
	}

	float multiplier = GetFromPb<float>(pb, Corona::COLORTEX_MULTIPLIER, this->mT);
	if (multiplier != 1.f)
		result = materialSystem.ValueMul(result, multiplier);
	
	BOOL inputLinear = GetFromPb<BOOL>(pb, Corona::COLORTEX_INPUTISLINEAR, this->mT);

	
	return result;
}

FIRERENDER_NAMESPACE_END;
