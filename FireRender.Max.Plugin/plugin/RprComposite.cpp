#include "RprComposite.h"
#include "Common.h"

void checkStatus(rpr_int status)
{
	FASSERT(status == RPR_SUCCESS);
}

RprComposite::RprComposite(rpr_context context, rpr_composite_type type)
{
	if (!context) checkStatus(RPR_ERROR_INTERNAL_ERROR);
	mContext = context;

	rpr_int status = rprContextCreateComposite(context, type, &mData);
	checkStatus(status);
}

RprComposite::~RprComposite()
{
	if (mData) rprObjectDelete(mData);
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
