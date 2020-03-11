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


#include "Synchronizer.h"
#include "plugin/ParamBlock.h"
#include "plugin/TmManager.h"
#include "plugin/CamManager.h"

FIRERENDER_NAMESPACE_BEGIN

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
				context.SetParameter(RPR_CONTEXT_MAX_RECURSION, v.mSimpleValue.int_val);
			}
			break;

			case PARAM_RENDER_MODE:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_RENDER_MODE, v.mSimpleValue.int_val);
			}
			break;

			case PARAM_IMAGE_FILTER:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_IMAGE_FILTER_TYPE, v.mSimpleValue.int_val);
			}
			break;

			case PARAM_USE_TEXTURE_COMPRESSION:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_TEXTURE_COMPRESSION, v.mSimpleValue.int_val);
			}
			break;

			case PARAM_IMAGE_FILTER_WIDTH:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_IMAGE_FILTER_BOX_RADIUS, v.mSimpleValue.float_val);
				context.SetParameter(RPR_CONTEXT_IMAGE_FILTER_GAUSSIAN_RADIUS, v.mSimpleValue.float_val);
				context.SetParameter(RPR_CONTEXT_IMAGE_FILTER_TRIANGLE_RADIUS, v.mSimpleValue.float_val);
				context.SetParameter(RPR_CONTEXT_IMAGE_FILTER_MITCHELL_RADIUS, v.mSimpleValue.float_val);
				context.SetParameter(RPR_CONTEXT_IMAGE_FILTER_LANCZOS_RADIUS, v.mSimpleValue.float_val);
				context.SetParameter(RPR_CONTEXT_IMAGE_FILTER_BLACKMANHARRIS_RADIUS, v.mSimpleValue.float_val);
			}
			break;

			case PARAM_USE_IRRADIANCE_CLAMP:
			{
				auto v = mParametersTracker.GetParam(pp);
				if (v.mSimpleValue.int_val)
				{
					float irradianceClamp = FLT_MAX;
					mBridge->GetPBlock()->GetValue(PARAM_IRRADIANCE_CLAMP, 0, irradianceClamp, Interval());
					context.SetParameter(RPR_CONTEXT_RADIANCE_CLAMP, irradianceClamp);
				}
				else
					context.SetParameter(RPR_CONTEXT_RADIANCE_CLAMP, FLT_MAX);
			}
			break;

			case PARAM_IRRADIANCE_CLAMP:
			{
				BOOL useIrradianceClamp = FALSE;
				mBridge->GetPBlock()->GetValue(PARAM_USE_IRRADIANCE_CLAMP, 0, useIrradianceClamp, Interval());
				if (useIrradianceClamp)
				{
					auto v = mParametersTracker.GetParam(pp);
					context.SetParameter(RPR_CONTEXT_RADIANCE_CLAMP, v.mSimpleValue.float_val);
				}
			}
			break;

			case PARAM_CONTEXT_ITERATIONS:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_ITERATIONS, v.mSimpleValue.int_val);
			}

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

			case PARAM_QUALITY_RAYCAST_EPSILON:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_RAY_CAST_EPISLON, v.mSimpleValue.float_val);
			}
			break;

			case PARAM_ADAPTIVE_NOISE_THRESHOLD:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_ADAPTIVE_SAMPLING_THRESHOLD, v.mSimpleValue.float_val);
			}
			break;

			case PARAM_ADAPTIVE_TILESIZE:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_ADAPTIVE_SAMPLING_TILE_SIZE, v.mSimpleValue.int_val);
			}
			break;

			case PARAM_SAMPLES_MIN:
			{
				auto v = mParametersTracker.GetParam(pp);
				context.SetParameter(RPR_CONTEXT_ADAPTIVE_SAMPLING_MIN_SPP, v.mSimpleValue.int_val);
			}
			break;
		}
	}
}

FIRERENDER_NAMESPACE_END
