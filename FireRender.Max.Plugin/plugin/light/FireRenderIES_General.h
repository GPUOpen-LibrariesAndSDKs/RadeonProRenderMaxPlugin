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


#pragma once

#include "FireRenderIES_Panel.h"
#include "Resource.h"
#include "IESLightParameter.h"

FIRERENDER_NAMESPACE_BEGIN

/* This class manages IES lights general options */
class IES_General :
	public IES_Panel<IES_General>
{
public:
	// IES_Panel static interface implementation
	static const int DialogId;
	static const TCHAR* PanelName;

	// Use base class constructor
	using BasePanel::BasePanel;

	// These methods move values from ui to the light object
	bool UpdateEnabledParam(TimeValue t);
	bool UpdateTargetedParam(TimeValue t);
	bool UpdateTargetDistanceParam(TimeValue t);
	bool UpdateAreaWidthParam(TimeValue t);
	bool UpdateRotationXParam(TimeValue t);
	bool UpdateRotationYParam(TimeValue t);
	bool UpdateRotationZParam(TimeValue t);
	void ImportProfile();
	bool ActivateSelectedProfile(TimeValue t);
	bool DeleteSelectedProfile(TimeValue t);

	// IES_Panel overrides
	bool InitializePage(TimeValue time);
	void UninitializePage();
	bool HandleControlCommand(TimeValue t, WORD code, WORD controlId);
	bool OnEditChange(TimeValue t, int editId, HWND editHWND);
	bool OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging);
	const TCHAR* GetAcceptMessage(WORD controlId) const;
	void UpdateUI(TimeValue t);
	void Enable();
	void Disable();

protected:
	void UpdateProfiles(TimeValue time);
	void UpdateDeleteProfileButtonState();

	MaxButton m_importButton;
	MaxButton m_deleteCurrentButton;
	WinCombobox m_profilesComboBox;
	WinButton m_enabledControl;
	WinButton m_targetedControl;
	MaxEditAndSpinner m_areaWidthControl;
	MaxEditAndSpinner m_targetDistanceControl;
	MaxEditAndSpinner m_RotateX;
	MaxEditAndSpinner m_RotateY;
	MaxEditAndSpinner m_RotateZ;
};

FIRERENDER_NAMESPACE_END
