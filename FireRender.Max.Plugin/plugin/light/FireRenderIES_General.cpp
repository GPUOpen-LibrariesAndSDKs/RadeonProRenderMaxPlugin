#include "FireRenderIESLight.h"

#include "FireRenderIES_General.h"

FIRERENDER_NAMESPACE_BEGIN

bool IES_General::InitDialog()
{
	auto pBlock = m_parent->GetParamBlock(0);

	BOOL enabled = FALSE;
	pBlock->GetValue(IES_PARAM_P0, 0, enabled, FOREVER);

	CheckDlgButton(
		m_panel,
		IDC_FIRERENDER_IES_LIGHT_ENABLED,
		enabled == FALSE ? BST_UNCHECKED : BST_CHECKED);

	return true;
}

FIRERENDER_NAMESPACE_END