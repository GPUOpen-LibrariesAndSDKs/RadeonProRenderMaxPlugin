/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (c) 2017 AMD
* All Rights Reserved
*
* Implementation of reactions to render settings changes (Synchronizer class)
*********************************************************************************************************************************/

#include "Synchronizer.h"
#include "plugin/ParamBlock.h"
#include "plugin/TmManager.h"
#include "plugin/CamManager.h"

FIRERENDER_NAMESPACE_BEGIN;

void Synchronizer::UpdateRenderSettings(const std::vector<int> &changes)
{
	auto context = mScope.GetContext();

	for (auto pp : changes)
	{
		switch (pp)
		{
			case PARAM_MAX_RAY_DEPTH:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter("maxRecursion", v.mSimpleValue.int_val);
			}
			break;

			case PARAM_RENDER_MODE:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter("rendermode", v.mSimpleValue.int_val);
			}
			break;

			case PARAM_IMAGE_FILTER:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter("imagefilter.type", v.mSimpleValue.int_val);
			}
			break;

			case PARAM_USE_TEXTURE_COMPRESSION:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter("texturecompression", v.mSimpleValue.int_val);
			}
			break;

			case PARAM_IMAGE_FILTER_WIDTH:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter("imagefilter.box.radius", v.mSimpleValue.float_val);
				context.SetParameter("imagefilter.gaussian.radius", v.mSimpleValue.float_val);
				context.SetParameter("imagefilter.triangle.radius", v.mSimpleValue.float_val);
				context.SetParameter("imagefilter.mitchell.radius", v.mSimpleValue.float_val);
				context.SetParameter("imagefilter.lanczos.radius", v.mSimpleValue.float_val);
				context.SetParameter("imagefilter.blackmanharris.radius", v.mSimpleValue.float_val);
			}
			break;

			case PARAM_USE_IRRADIANCE_CLAMP:
			{
				auto v = mParametersTracker.GetParam(pp);
				if (v.mSimpleValue.int_val)
				{
					float irradianceClamp = FLT_MAX;
					mBridge->GetPBlock()->GetValue(PARAM_IRRADIANCE_CLAMP, 0, irradianceClamp, Interval());
					context.SetParameter("radianceclamp", irradianceClamp);
				}
				else
					context.SetParameter("radianceclamp", FLT_MAX);
			}
			break;

			case PARAM_IRRADIANCE_CLAMP:
			{
				BOOL useIrradianceClamp = FALSE;
				mBridge->GetPBlock()->GetValue(PARAM_USE_IRRADIANCE_CLAMP, 0, useIrradianceClamp, Interval());
				if (useIrradianceClamp)
				{
					auto v = mParametersTracker.GetParam(pp);
					context.SetParameter("radianceclamp", v.mSimpleValue.float_val);
				}
			}
			break;

			case PARAM_AA_SAMPLE_COUNT:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter("aasamples", v.mSimpleValue.int_val);
			}
			break;

			case PARAM_AA_GRID_SIZE:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter("aacellsize", v.mSimpleValue.int_val);
			}
			break;

			case PARAM_TIME_LIMIT:
			{
				auto v = mParametersTracker.GetParam(pp);
				mBridge->SetTimeLimit(v.mSimpleValue.int_val);
			}
			break;

			case PARAM_PASS_LIMIT:
			{
				auto v = mParametersTracker.GetParam(pp);
				mBridge->SetPassLimit(v.mSimpleValue.int_val);
			}
			break;
			
			case PARAM_RENDER_LIMIT:
			{
				auto v = mParametersTracker.GetParam(pp);
				mBridge->SetLimitType(v.mSimpleValue.int_val);
			}
			break;
		}
	}
}

FIRERENDER_NAMESPACE_END;
