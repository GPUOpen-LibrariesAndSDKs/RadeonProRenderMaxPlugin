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

FIRERENDER_NAMESPACE_BEGIN

/* This class manages IES lights intensity options */
class IES_Intensity :
	public IES_Panel<IES_Intensity>
{
public:
	// IES_Panel static interface implementation
	static const int DialogId;
	static const TCHAR* PanelName;

	// Use base class constructor
	using BasePanel::BasePanel;

	bool UpdateIntensityParam(TimeValue t);
	bool UpdateColorModeParam(TimeValue t);
	bool UpdateColorParam(TimeValue t);
	bool UpdateTemperatureParam(TimeValue t);

	// IES_Panel overrides
	bool InitializePage(TimeValue time);
	void UninitializePage();
	bool HandleControlCommand(TimeValue t, WORD code, WORD controlId);
	bool OnEditChange(TimeValue t, int editId, HWND editHWND);
	bool OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging);
	bool OnColorSwatchChange(TimeValue t, IColorSwatch* colorSwatch, WORD controlId, bool final);
	const TCHAR* GetAcceptMessage(WORD controlId) const;
	void Enable();
	void Disable();

private:
	void UpdateControlsEnabled(TimeValue time);

	MaxEditAndSpinner m_intensityControl;
	WinButton m_rgbModeControl;
	WinButton m_kelvinModeControl;
	MaxColorSwatch m_colorControl;
	MaxKelvinColor m_temperatureControl;
};

FIRERENDER_NAMESPACE_END
