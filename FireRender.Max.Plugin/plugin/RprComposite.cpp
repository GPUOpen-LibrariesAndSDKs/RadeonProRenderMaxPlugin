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


#include "RprComposite.h"
#include "Common.h"

namespace
{
	void checkStatus(rpr_int status)
	{
		FCHECK(status);
	}
}

RprComposite::RprComposite(rpr_context context, rpr_composite_type type) : 
	mData(0), 
	mContext(context)
{
	if (!mContext)
	{
		checkStatus(RPR_ERROR_INTERNAL_ERROR);
		return;
	}

	rpr_int res = rprContextCreateComposite(context, type, &mData);
	FCHECK_CONTEXT(res, context, "rprContextCreateComposite");
}

RprComposite::~RprComposite()
{
	if (mData)
	{
		rpr_int status = rprObjectDelete(mData);
		checkStatus(status);
	}
}

RprComposite::operator rpr_composite()
{
	return mData;
}

void RprComposite::SetInputC(const char *inputName, rpr_composite input)
{
	rpr_int status = rprCompositeSetInputC(mData, inputName, input);
	checkStatus(status);
}

void RprComposite::SetInputFb(const char *inputName, rpr_framebuffer input)
{
	rpr_int status = rprCompositeSetInputFb(mData, inputName, input);
	checkStatus(status);
}

void RprComposite::SetInput4f(const char *inputName, float r, float g, float b, float a)
{
	rpr_int status = rprCompositeSetInput4f(mData, inputName, r, g, b, a);
	checkStatus(status);
}

void RprComposite::SetInputOp(const char *inputName, rpr_material_node_arithmetic_operation op)
{
	rpr_int status = rprCompositeSetInputOp(mData, inputName, op);
	checkStatus(status);
}
