/*********************************************************************************************************************************
* Radeon ProRender for 3ds Max plugin
* Copyright (C) 2017 AMD
* All Rights Reserved
*
* 3ds Max render settings UI dialog implementation:
* Custom dialog box code for the various render settings rollouts
*********************************************************************************************************************************/

#pragma once
#include "Common.h"
#include <3dsmaxport.h>
#include <stdint.h>
#include <iparamm2.h>
#include <map>
#include <memory>
#include "plugin/ManagerBase.h"

FIRERENDER_NAMESPACE_BEGIN;

class FireRenderer;
class FireRenderParamDlg;

/// Tab ids for renderer settings
static Class_ID kSettingsTabID(0x2b765571, 0x8cd2fbe1);
static Class_ID kAdvSettingsTabID(0x178a87bc, 0x7a5cdf72);
static Class_ID kScriptsTabID(0x1cae4307, 0x18c26f6b);

class CRollout
{
public:
	CRollout();
	CRollout(CRollout&&);
	CRollout(const CRollout&) = delete;
	virtual ~CRollout();

	void DeleteRollout();

	virtual void Init(IRendParams* ir, int tmpl, const MCHAR *title, FireRenderParamDlg *owner, bool closed = false, const Class_ID* tabId = NULL);
	virtual void InitDialog();
	virtual void DestroyDialog();
	virtual INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) = 0;
	virtual void AcceptParams(IParamBlock2* pb);
	
	static INT_PTR CALLBACK HandleMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

	HWND Hwnd() const;
	bool Ready() const;

	void SetControlTooltip(int controlId, const MCHAR* tooltip);

	CRollout& operator=(const CRollout&) = delete;
	CRollout& operator=(CRollout&&);

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
	class Impl;
	std::unique_ptr<Impl> m_d;

	void RemoveAllSpinnerAssociations();
	BOOL CommitSpinnerToParam(ManagerMaxBase &man, int16_t control);
	BOOL CommitSpinnerToParam(IParamBlock2* pb, int16_t control);

protected:
	/// Convenience function that reads value from a checkbox and writes it to given IParamBlock2
	/// \param paramId ID of the parameter in the paramblock
	/// \param controlId ID of the win32 control in the dialog
	void SaveCheckbox(IParamBlock2* pb, const ParamID paramId, const int16_t controlId);

	/// Convenience function that reads value from a checkbox and writes it to given IParamBlock2
	/// \param paramId ID of the parameter in the paramblock
	/// \param controlId ID of the win32 control in the dialog
	void SaveRadioButton(IParamBlock2* pb, const ParamID paramId, const int16_t controlId);

	bool GetRadioButtonValue(const int16_t controlId) const;

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

	/// Convenience function that sets up a radio button in the win32 dialog (by setting its initial value to match the value of 
	/// the corresponding parameter in IParamBlock2)
	/// \param paramId ID of the parameter in the paramblock
	/// \param controlId ID of the win32 control in the dialog
	void CheckRadioButton(const int16_t controlId);
	void UnCheckRadioButton(const int16_t controlId);

	/// Convenience function that sets up a checkbox in the win32 dialog (by setting its initial value to match the value of 
	/// the corresponding parameter in IParamBlock2)
	/// \param paramId ID of the parameter in the paramblock
	/// \param controlId ID of the win32 control in the dialog
	void SetupCheckbox(IParamBlock2* pb, const ParamID paramId, const int16_t controlId);
	void SetupCheckbox(ManagerMaxBase &man, const ParamID paramId, const int16_t controlId);// background manager version
	int GetComboBoxValue(const int16_t controlId);
	void SetCheckboxValue(bool value, const int16_t controlId);
	static BOOL CALLBACK DisableWindowsProc(HWND hWnd, LPARAM lParam);
	void DisableAllControls();
	void EnableControl(int idCtrl, BOOL enable);
	void SetVisibleDlg(bool visible, const int16_t dlgID);
};


/// Class that constructs the render settings UI dialog, and which saves the changes user makes into a IParamBlock2
class FireRenderParamDlg : public RendParamDlg
{
	friend class CRollout;
protected:
    FireRenderer* renderer;

    /// 3ds Max interface that adds and deletes rollouts in the render dialog
    IRendParams* iRendParams;

protected:
	BOOL isReadOnly;

	class CGeneralSettings : public CRollout
	{
	public:
		struct {
			ISpinnerControl* timeLimitS;
			ISpinnerControl* timeLimitM;
			ISpinnerControl* timeLimitH;
			ISpinnerControl* passLimit;
		} controls;

		CGeneralSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void EnableRenderLimitControls(BOOL enablePassLimit, BOOL disableAll);
	};

	class CHardwareSettings : public CRollout
	{
	public:
		CHardwareSettings()
		{
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void InitGPUCompatibleList();
		void UpdateGPUSelected(int gpuIdx);
		void UpdateUI();
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
		void EnableExportModelControls(BOOL enableExport);
		BOOL GetExportFileName();
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

	class CGroundSettings : public CRollout
	{
	private:

	public:
		struct {
			ISpinnerControl* heigth;
			ISpinnerControl* radius;
			ISpinnerControl* reflStrength;
			ISpinnerControl* reflRoughness;
			IColorSwatch* reflColor;
		} controls;

		CGroundSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;
	};

	class CAntialiasSettings : public CRollout
	{
	public:
		struct {
			ISpinnerControl* aaFilterWidth;
			ISpinnerControl* aaSampleCount;
			ISpinnerControl* aaGridSize;
		} controls;

		CAntialiasSettings()
		{
			memset(&controls, 0, sizeof(controls));
		}

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void SetQualityPresets(int qualityLevel);
	};

	class CQualitySettings : public CRollout
	{
	public:
		CQualitySettings()
		{
		}

		void setupUIFromData();

		void InitDialog() override;
		void DestroyDialog() override;
		INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

		void SetQualityPresets(int qualityLevel);
	};

	CGeneralSettings mGeneralSettings;
	CHardwareSettings mHardwareSetting;
	CCameraSettings mCameraSetting;
	CTonemapSettings mTonemapSetting;
	CAdvancedSettings mAdvancedSettings;
	CScripts mScripts;
	CBackgroundSettings mBackgroundSettings;
	CGroundSettings mGroundSettings;
	CAntialiasSettings mAntialiasSettings;
	CQualitySettings mQualitySettings;


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
		mGroundSettings.DestroyDialog();
		mGeneralSettings.InitDialog(); // this should set everything to defaults
		mHardwareSetting.InitDialog();
		mCameraSetting.InitDialog();
		mAdvancedSettings.InitDialog();
		mScripts.InitDialog();
		mTonemapSetting.InitDialog();
		mBackgroundSettings.InitDialog();
		mGroundSettings.InitDialog();
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
