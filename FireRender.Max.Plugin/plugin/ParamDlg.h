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

#include "Common.h"
#include "ManagerBase.h"
#include "ParamBlock.h"

#include <3dsmaxport.h>
#include <stdint.h>
#include <iparamm2.h>

#include <map>
#include <memory>
#include <unordered_map>

FIRERENDER_NAMESPACE_BEGIN;

class FireRenderer;
class FireRenderParamDlg;

/// Tab ids for renderer settings
static Class_ID kSettingsTabID(0x2b765571, 0x8cd2fbe1);
static Class_ID kAdvSettingsTabID(0x178a87bc, 0x7a5cdf72);
static Class_ID kScriptsTabID(0x1cae4307, 0x18c26f6b);

	class CRollout
	{
	private:
		std::wstring mName;

	public:
		CRollout();
		virtual ~CRollout();

		void DeleteRollout();

		virtual void Init(IRendParams* ir, int tmpl, const MCHAR *title, FireRenderParamDlg *owner, bool closed = false, const Class_ID* tabId = NULL);
		
		virtual void InitDialog();
		virtual void DestroyDialog();

		virtual INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) = 0;

		virtual void AcceptParams(IParamBlock2* pb);
		
		static INT_PTR CALLBACK HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		inline HWND Hwnd() const
		{
			return mHwnd;
		}

		inline bool Ready() const
		{
			return mIsReady;
		}

		void SetControlTooltip(int controlId, const MCHAR* tooltip);

		class CTexmapButton
		{
		public:
			HWND mWnd;
			ICustButton *mButton;
			IParamBlock2* mPb;
			ParamID mParamId;
			int16_t mControlIdEdit;
			CRollout *mOwner;

			CTexmapButton(CRollout* owner, HWND wnd, IParamBlock2* pb,  const ParamID paramId, const int16_t controlIdEdit);
			~CTexmapButton();

			// This function fully performs assignment - stores value to mValue, to ParamBlock, and updates UI
			void SetValue(Texmap* value);

			inline Texmap* GetValue() const
			{
				return mValue;
			}

			void UpdateUI();

		private:
			Texmap* mValue;
		};

	protected:
		HWND mHwnd;
		FireRenderParamDlg *mOwner;
		IRendParams *mRenderParms;
		const Class_ID *mTabId;

		// quickly find a parameter associated to a spinner
		typedef struct SSpinnerData
		{
			int type;
			ParamID paramId;
			ISpinnerControl *spinner;
			SSpinnerData()
			{
			}
			SSpinnerData(int t, ParamID p, ISpinnerControl *s)
			{
				type = t;
				paramId = p;
				spinner = s;
			}
		} SpinnerData;

		std::map<int16_t, SpinnerData> mSpinnerMap;

		enum
		{
			SPINNER_TYPE_INT,
			SPINNER_TYPE_FLOAT,
		};

		void AssociateSpinnerFloat(ISpinnerControl* spinner, ParamID id, int16_t control)
		{
			auto ii = mSpinnerMap.find(control);
			if (ii == mSpinnerMap.end())
				mSpinnerMap.insert(std::make_pair(control, SpinnerData(SPINNER_TYPE_FLOAT , id, spinner)));
		}
		void AssociateSpinnerInt(ISpinnerControl* spinner, ParamID id, int16_t control)
		{
			auto ii = mSpinnerMap.find(control);
			if (ii == mSpinnerMap.end())
				mSpinnerMap.insert(std::make_pair(control, SpinnerData(SPINNER_TYPE_INT, id, spinner)));
		}
		bool GetSpinnerParam(int16_t control, SpinnerData &out)
		{
			bool res = false;
			auto ii = mSpinnerMap.find(control);
			if (ii != mSpinnerMap.end())
			{
				out = ii->second;
				res = true;
			}
			return res;
		}
		void RemoveAllSpinnerAssociations()
		{
			mSpinnerMap.clear();
		}
		BOOL CommitSpinnerToParam_p(ManagerMaxBase &man, int16_t control)
		{
			BOOL res = FALSE;
			SpinnerData data;
			if (GetSpinnerParam(control, data))
			{
				if (data.type == SPINNER_TYPE_FLOAT)
					res = man.SetProperty(data.paramId, data.spinner->GetFVal());
				else
					res = man.SetProperty(data.paramId, data.spinner->GetIVal());
			}
			return res;
		}
		BOOL CommitSpinnerToParam_p(IParamBlock2* pb, int16_t control)
		{
			BOOL res = FALSE;
			SpinnerData data;
			if (GetSpinnerParam(control, data))
			{
				if (data.type == SPINNER_TYPE_FLOAT)
					res = pb->SetValue(data.paramId, GetCOREInterface()->GetTime(), data.spinner->GetFVal());
				else
					res = pb->SetValue(data.paramId, GetCOREInterface()->GetTime(), data.spinner->GetIVal());
				FASSERT(res);
			}

			return res;
		}
	public:
		BOOL CommitSpinnerToParam(ManagerMaxBase &man, int16_t control) // background manager version
		{
			return CommitSpinnerToParam_p(man, control);
		}
		BOOL CommitSpinnerToParam(IParamBlock2* pb, int16_t control)
		{
			return CommitSpinnerToParam_p(pb, control);
		}

	protected:
		/// True if the dialog is constructed and ready to be used. False if it is currently being constructed/destroyed, and 
		/// for example any value changes should not be saved in IParamBlock2
		bool mIsReady;

		/// Convenience function that reads value from a checkbox and writes it to given IParamBlock2
		/// \param paramId ID of the parameter in the paramblock
		/// \param controlId ID of the win32 control in the dialog
		void SaveCheckbox(IParamBlock2* pb, const ParamID paramId, const int16_t controlId) {
			FASSERT(GetDlgItem(mHwnd, controlId));
			const BOOL value = IsDlgButtonChecked(mHwnd, controlId);
			BOOL res = pb->SetValue(paramId, 0, value);
			FASSERT(res);
			res = res;
		}

		/// Convenience function that reads value from a checkbox and writes it to given IParamBlock2
		/// \param paramId ID of the parameter in the paramblock
		/// \param controlId ID of the win32 control in the dialog
		void SaveRadioButton(IParamBlock2* pb, const ParamID paramId, const int16_t controlId) {
			FASSERT(GetDlgItem(mHwnd, controlId));
			const BOOL value = IsDlgButtonChecked(mHwnd, controlId);
			BOOL res = pb->SetValue(paramId, 0, value);
			FASSERT(res);
			res = res;
		}

		bool GetRadioButtonValue(const int16_t controlId)
		{
			FASSERT(GetDlgItem(mHwnd, controlId));
			return bool_cast(IsDlgButtonChecked(mHwnd, controlId));
		}

		CTexmapButton* SetupTexmapButton(IParamBlock2* pb, const ParamID paramId, const int16_t controlIdEdit);  // background manager version
		
		IColorSwatch* SetupSwatch(IParamBlock2* pb, const ParamID paramId, const int16_t controlIdEdit);
		IColorSwatch* SetupSwatch(const ParamID paramId, const int16_t controlIdEdit); // background manager version

		/// Convenience function that sets up a 3ds Max-style spinner in the win32 dialog. It also sets its value to match the value
		/// stored in the param block. Spinners in 3ds Max are created from 2 linked win32 windows - text input and up/down arrows 
		/// control. The up/down control is considered the "main" window of the control by 3ds Max
		/// \param paramId ID of the parameter in the paramblock
		/// \param controlIDEdit win32 window ID of the text edit part of the control
		/// \param controlIdSpin win32 window ID of the up/down spinner part of the control
		ISpinnerControl* SetupSpinner(IParamBlock2* pb, const ParamID paramId, const int16_t controlIdEdit, const int16_t controlIdSpin);
		ISpinnerControl* SetupSpinnerFloat(ManagerMaxBase &man, const ParamID paramId, const int16_t controlIdEdit, const int16_t controlIdSpin, const float &minVal, const float &maxVal); // background manager version
		ISpinnerControl* SetupSpinnerInt(ManagerMaxBase &man, const ParamID paramId, const int16_t controlIdEdit, const int16_t controlIdSpin, int minVal, int maxVal); // background manager version

		/// Convenience function that sets up a radio button in the win32 dialog (by setting its initial value to match the value of 
		/// the corresponding parameter in IParamBlock2)
		/// \param paramId ID of the parameter in the paramblock
		/// \param controlId ID of the win32 control in the dialog
		void CheckRadioButton(const int16_t controlId) {
			FASSERT(GetDlgItem(mHwnd, controlId));
			CheckDlgButton(mHwnd, controlId, BST_CHECKED);
		}
		void UnCheckRadioButton(const int16_t controlId) {
			FASSERT(GetDlgItem(mHwnd, controlId));
			CheckDlgButton(mHwnd, controlId, BST_UNCHECKED);
		}

		/// Convenience function that sets up a checkbox in the win32 dialog (by setting its initial value to match the value of 
		/// the corresponding parameter in IParamBlock2)
		/// \param paramId ID of the parameter in the paramblock
		/// \param controlId ID of the win32 control in the dialog
		void SetupCheckbox(IParamBlock2* pb, const ParamID paramId, const int16_t controlId)
		{
			BOOL value;
			BOOL res = pb->GetValue(paramId, GetCOREInterface()->GetTime(), value, FOREVER);
			FASSERT(res);
			res = res;
			SetCheckboxValue( bool_cast(value), controlId);
		}

		void SetupCheckbox(ManagerMaxBase &man, const ParamID paramId, const int16_t controlId) // background manager version
		{ 
			BOOL value;
			BOOL res = man.GetProperty(paramId, value);
			FASSERT(res);
			res = res;
			SetCheckboxValue( bool_cast(value), controlId);
		}

		int GetComboBoxValue(const int16_t controlId)
		{
			HWND ctrl = GetDlgItem(mHwnd, controlId);
			FASSERT(ctrl);
			int dummy = int(SendMessage(ctrl, CB_GETCURSEL, 0, 0));
			FASSERT(dummy != CB_ERR);
			const int ctrlValue = int(SendMessage(ctrl, CB_GETITEMDATA, WPARAM(dummy), 0));
			FASSERT(ctrlValue != CB_ERR);
			return ctrlValue;
		}

		void SetCheckboxValue(bool value, const int16_t controlId)
		{
			FASSERT(GetDlgItem(mHwnd, controlId));
			CheckDlgButton(mHwnd, controlId, value ? BST_CHECKED : BST_UNCHECKED);
		}

		static BOOL CALLBACK DisableWindowsProc(HWND hWnd, LPARAM lParam)
		{
			if (hWnd)
				EnableWindow(hWnd, (BOOL)lParam);
			return TRUE;
		}

		void DisableAllControls()
		{
			if (mHwnd)
				EnumChildWindows(mHwnd, DisableWindowsProc, FALSE);
		}

		void EnableControl(int idCtrl, BOOL enable)
		{
			HWND ctrl = GetDlgItem(mHwnd, idCtrl);
			FASSERT(ctrl);
			EnableWindow(ctrl, enable);
		}

		void SetVisibleDlg(bool visible, const int16_t dlgID);
	};


/// Class that constructs the render settings UI dialog, and which saves the changes user makes into a IParamBlock2
class FireRenderParamDlg : public RendParamDlg
{
	friend class CRollout;

public:
	static bool mIsUncertifiedMessageShown;

protected:
    FireRenderer* renderer;

    /// 3ds Max interface that adds and deletes rollouts in the render dialog
    IRendParams* iRendParams;

	BOOL isReadOnly;

	class CGeneralSettings : public CRollout
	{
	public:
		struct {
			ISpinnerControl* samplesMin;
			ISpinnerControl* samplesMax;
			ISpinnerControl* adaptiveThresh;
			ISpinnerControl* timeLimitS;
			ISpinnerControl* timeLimitM;
			ISpinnerControl* timeLimitH;
			ISpinnerControl* contextIterations;
		} controls;

		CGeneralSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void UpdateRenderLimitControls();
		void SetQualityPresets(int qualityLevel);
	};

	class CHardwareSettings : public CRollout
	{
		const int cpuThreadsDefault = 0;
		const int cpuThreadsMin = 2;
		const int cpuThreadsMax = 128; // as defined in RPR on Jul 2018

	public:
		struct {
			ISpinnerControl* spinnerCpuThreadsNum;
		} controls;

		CHardwareSettings()
		{
			controls.spinnerCpuThreadsNum = nullptr;
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void InitGPUCompatibleList();
		void UpdateHwSelected(int gpuIdx);
		void LoadAttributeSettings();
		void SaveAttributeSettings();
	};

	class CCameraSettings : public CRollout
	{
	public:
		struct {
			ISpinnerControl* cameraBlades;
			ISpinnerControl* cameraPerspectiveFocalDist;
			ISpinnerControl* cameraFStop;
			ISpinnerControl* cameraSensorSize;
			ISpinnerControl* cameraExposure;
			ISpinnerControl* cameraFocalLength;
			ISpinnerControl* cameraFOV;
			ISpinnerControl* motionBlurScale;
		} controls;

		CCameraSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void InitCameraTypesDialog();
		void SetVisibleDOFSettings(bool visible);
		void ApplyFOV(BOOL useFOV);

		float CalcFov(float sensorWidth, float focalLength);
		float CalcFocalLength(float sensorWidth, float fov);
	};

	class CTonemapSettings : public CRollout, public ManagerPropertyChangeCallback
	{
	public:
		struct {
			ISpinnerControl* burn;
			ISpinnerControl* preScale;
			ISpinnerControl* postScale;

			ISpinnerControl* iso;
			ISpinnerControl* fstop;
			ISpinnerControl* shutterspeed;

			ISpinnerControl* exposure;
			ISpinnerControl* contrast;
			ISpinnerControl* whitebalance;
			IColorSwatch* tintcolor;

		} controls;

		CTonemapSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender) override;
	};

	class CAdvancedSettings : public CRollout
	{
	private:
		std::wstring exportFRSceneFileName;

	public:
		struct {
			ISpinnerControl* irradianceClamp;
			ISpinnerControl* meditQuality;
			ISpinnerControl* texmapSize;
			ISpinnerControl* maxRayDepth;
		} controls;

		CAdvancedSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}
		
		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void EnableLimitLightBounceControls(BOOL enableMaxLimit);
		void EnableIrradianceClampControls(BOOL enable);
	};

	class CScripts : public CRollout
	{
	private:
		int selectedScriptID;

	public:
		enum ScriptID
		{
			RPR_SCRIPT_INVALID = -1,
			RPR_CONVERT_MATERIALS_FROM_VRAY_TO_PRORENDER,
		};

		CScripts()
		{
			selectedScriptID = RPR_SCRIPT_INVALID;
		}
		
		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
	};

	class CBackgroundSettings : public CRollout, public ManagerPropertyChangeCallback
	{
	private:
		Matrix3 mPrevSunTm = idTM;

	public:
		struct {
			ISpinnerControl* iblIntensity;
			IColorSwatch* iblColor;
			ISpinnerControl* skyAzimuth;
			ISpinnerControl* skyAltitude;
			ISpinnerControl* skyHours;
			ISpinnerControl* skyMinutes;
			ISpinnerControl* skySeconds;
			ISpinnerControl* skyDay;
			ISpinnerControl* skyMonth;
			ISpinnerControl* skyYear;
			ISpinnerControl* skyTimezone;
			ISpinnerControl* skyLatitude;
			ISpinnerControl* skyLongitude;
			ISpinnerControl* skyHaze;
			IColorSwatch* skyGroundColor;
			IColorSwatch* skyGroundAlbedo;
			IColorSwatch* skyFilterColor;
			ISpinnerControl* skyDiscSize;
			ISpinnerControl* skyIntensity;
			ISpinnerControl* skySaturation;
			CTexmapButton *iblMap;
			CTexmapButton *iblBackplate;
			CTexmapButton *iblReflOverride;
			CTexmapButton *iblRefrOverride;
			CTexmapButton *skyBackplate;
			CTexmapButton *skyReflOverride;
			CTexmapButton *skyRefrOverride;
		} controls;

		CBackgroundSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;

		void UpdateGroupEnables();

		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void propChangedCallback(class ManagerMaxBase *owner, int paramId, int sender) override;
	};

	class CAntialiasSettings : public CRollout
	{
	public:
		struct
		{
			ISpinnerControl* aaFilterWidth;
			ISpinnerControl* iterationsCount;
		} controls;

		CAntialiasSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
	};

	class CQualitySettings : public CRollout
	{
	public:
		struct
		{
			ISpinnerControl* raycastEpsilon;
		} controls;

		CQualitySettings()
		{
		}

		void setupUIFromData();

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void SetQualityPresets(int qualityLevel);
	};

	class CDenoiserSettings : public CRollout
	{
	public:
		struct
		{
			ISpinnerControl* bilateralRadius = nullptr;
			ISpinnerControl* lwrSamples = nullptr;
			ISpinnerControl* lwrRadius = nullptr;
			ISpinnerControl* lwrBandwidth = nullptr;
			ISpinnerControl* eawColor = nullptr;
			ISpinnerControl* eawNormal = nullptr;
			ISpinnerControl* eawDepth = nullptr;
			ISpinnerControl* eawObjectId = nullptr;
		} controls;

		std::unordered_map<int, std::pair<ISpinnerControl*, Parameter>> mParamById;

	public:
		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void setupUIFromData();
	};

	CGeneralSettings     mGeneralSettings;
	CHardwareSettings    mHardwareSetting;
	CCameraSettings      mCameraSetting;
	CTonemapSettings     mTonemapSetting;
	CAdvancedSettings    mAdvancedSettings;
	CScripts             mScripts;
	CBackgroundSettings  mBackgroundSettings;
	CAntialiasSettings   mAntialiasSettings;
	CQualitySettings     mQualitySettings;
	CDenoiserSettings    mDenoiserSettings;

	std::string uncertifiedGpuNames;
	std::string outDatedDriverGpuNames;

public:

    /// Constructs the UI dialog and intializes it with current render settings, and sets itself as the current dialog in the 
    /// parent Radeon ProRender instance
    /// \param readOnly if true, 3ds Max wants us to create a rendering progress dialog. We do not create any dialog in that case.
	FireRenderParamDlg(IRendParams* ir, BOOL readOnly, FireRenderer* renderer);

	/// Un-sets itself from the parent Radeon ProRender instance
    virtual ~FireRenderParamDlg();
    
    // 3dsmax API methods of RendParamDlg:

    /// Saves the settings currently shown in UI into the IParamBlock2
    virtual void AcceptParams() override;

    /// Resets the UI to default values without saving it to IParamBlock2
    virtual void RejectParams() override
	{
		mGeneralSettings.DestroyDialog();
		mHardwareSetting.DestroyDialog();
		mCameraSetting.DestroyDialog();
		mAdvancedSettings.DestroyDialog();
		mScripts.DestroyDialog();
		mTonemapSetting.DestroyDialog();
		mBackgroundSettings.DestroyDialog();
		mGeneralSettings.InitDialog(); // this should set everything to defaults
		mHardwareSetting.InitDialog();
		mCameraSetting.InitDialog();
		mAdvancedSettings.InitDialog();
		mScripts.InitDialog();
		mTonemapSetting.InitDialog();
		mBackgroundSettings.InitDialog();
    }

	virtual void DeleteThis() override;

	void setWarningSuppress(bool value);
	void processUncertifiedGpuWarning();

	HWND GetAnyWindowHandle() const
	{
		return mGeneralSettings.Hwnd();
	}
};

FIRERENDER_NAMESPACE_END;
