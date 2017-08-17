#pragma once

#include "frScope.h"

FIRERENDER_NAMESPACE_BEGIN

class FireRenderIESLight;

template<typename ControlTraits>
class MaxControl
{
	using TControl = typename ControlTraits::TControl;

public:
	MaxControl() :
		m_ctrl(nullptr)
	{}
	MaxControl(MaxControl&& that)
	{
		MoveFrom(that);
	}
	MaxControl(const MaxControl&) = delete;
	~MaxControl()
	{
		Release();
	}

	void Capture(HWND dialog, int control)
	{
		// Release previous control
		Release();

		auto controlHwnd = GetDlgItem(dialog, control);

		if (controlHwnd == nullptr)
		{
			FASSERT(!"Failed to capture the control");
			return;
		}

		m_ctrl = ControlTraits::Capture(controlHwnd);

		FASSERT(m_ctrl != nullptr);
	}

	void Release()
	{
		if (m_ctrl != nullptr)
		{
			ControlTraits::Release(m_ctrl);
			m_ctrl = nullptr;
		}
	}

	TControl* GetControl() const { return m_ctrl; }

	MaxControl& operator=(const MaxControl&) = delete;
	MaxControl& operator=(MaxControl&& that)
	{
		Release();
		MoveFrom(that);
		return *this;
	}

protected:
	void MoveFrom(MaxControl& that)
	{
		m_ctrl = that.m_ctrl;
		that.m_ctrl = nullptr;
	}

	TControl* m_ctrl;
};

class MaxSpinnerTraits
{
public:
	using TControl = ISpinnerControl;

	static TControl* Capture(HWND wnd)
	{
		return GetISpinner(wnd);
	}
	
	static void Release(TControl* ctrl)
	{
		ReleaseISpinner(ctrl);
	}
};

class MaxEditTraits
{
public:
	using TControl = ICustEdit;

	static TControl* Capture(HWND wnd)
	{
		return GetICustEdit(wnd);
	}

	static void Release(TControl* ctrl)
	{
		ReleaseICustEdit(ctrl);
	}
};

class MaxSpinner :
	public MaxControl<MaxSpinnerTraits>
{
	using ParentClass = MaxControl<MaxSpinnerTraits>;
public:
	using ParentClass::ParentClass;
	using ParentClass::operator=;

	template<typename T>
	void SetLimits(T min, T max)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetLimits(min, max);
	}

	template<typename T>
	void SetResetValue(T resetValue)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetResetValue(resetValue);
	}

	void SetScale(float scale)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetScale(scale);
	}

	template<typename T>
	void SetValue(T value)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetValue(value, TRUE);
	}
};

class MaxEdit :
	public MaxControl<MaxEditTraits>
{
	using ParentClass = MaxControl<MaxEditTraits>;
public:
	using ParentClass::ParentClass;
	using ParentClass::operator=;
};

class MaxEditAndSpinner
{
public:
	MaxEditAndSpinner() = default;
	MaxEditAndSpinner(MaxEditAndSpinner&&) = default;
	MaxEditAndSpinner(const MaxEditAndSpinner&) = delete;

	void Capture(HWND hWnd, int editId, int spinnerId)
	{
		m_edit.Capture(hWnd, editId);
		m_spinner.Capture(hWnd, spinnerId);
	}

	void Bind(EditSpinnerType editType)
	{
		auto pEdit = m_edit.GetControl();
		auto pSpinner = m_spinner.GetControl();

		pSpinner->LinkToEdit(pEdit->GetHwnd(), editType);
	}

	void Release()
	{
		m_edit.Release();
		m_spinner.Release();
	}

	MaxEdit& GetEdit() { return m_edit; }
	MaxSpinner& GetSpinner() { return m_spinner; }

	MaxEditAndSpinner& operator=(MaxEditAndSpinner&&) = default;
	MaxEditAndSpinner& operator=(const MaxEditAndSpinner&) = delete;
private:
	MaxEdit m_edit;
	MaxSpinner m_spinner;
};

template<typename Derived>
class IES_Panel
{
public:
	using BasePanel = IES_Panel;

	IES_Panel(FireRenderIESLight* parent) :
		m_panel(nullptr),
		m_parent(parent)
	{}

	void BeginEdit(IObjParam* objParam, ULONG flags, Animatable* prev)
	{
		FASSERT(m_panel == nullptr);

		auto _this = static_cast<Derived*>(this);

		m_panel = objParam->AddRollupPage(
			fireRenderHInstance,
			MAKEINTRESOURCE(Derived::DialogId),
			Derived::DlgProc,
			Derived::PanelName,
			(LPARAM)_this);

		auto wndContext = reinterpret_cast<LONG_PTR>(_this);
		auto prevLong = SetWindowLongPtr(m_panel, GWLP_USERDATA, wndContext);

		_this->InitializePage();
		objParam->RegisterDlgWnd(m_panel);
		
		FASSERT(prevLong == 0);
	}

	void EndEdit(IObjParam* objParam, ULONG flags, Animatable* next)
	{
		FASSERT(m_panel != nullptr);
		auto _this = static_cast<Derived*>(this);
		_this->UninitializePage();
		objParam->DeleteRollupPage(m_panel);
		m_panel = nullptr;
	}

	// Default empty implementations
	bool InitializePage() { return true; }
	void UninitializePage() {}
	INT_PTR HandleControlCommand(WORD code, WORD controlId) { return FALSE; }
	INT_PTR HandleCustomEditEnter(int editId, HWND editHWND) { return FALSE; }
	INT_PTR HandleSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging) { return FALSE; }


protected:
	IParamBlock2* GetParamBlock()
	{
		auto pBlock = m_parent->GetParamBlock(0);
		FASSERT(pBlock != nullptr);

		return pBlock;
	}

	HWND m_panel;
	FireRenderIESLight* m_parent;

private:

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			case WM_INITDIALOG:
				return TRUE;
				break;

			case WM_COMMAND:
			{
				if (lParam != 0)
				{
					auto code = HIWORD(wParam);
					auto controlId = LOWORD(wParam);
					auto _this = GetAttachedThis(hWnd);

					return _this->HandleControlCommand(code, controlId);
				}
			}
			break;

			case WM_CUSTEDIT_ENTER:
			{
				auto customEditId = wParam;
				auto customEditHWND = reinterpret_cast<HWND>(lParam);
				auto _this = GetAttachedThis(hWnd);

				return _this->HandleCustomEditEnter(customEditId, customEditHWND);
			}
			break;

			case CC_SPINNER_CHANGE:
			{
				auto spinner = reinterpret_cast<ISpinnerControl*>(lParam);
				auto spinnerId = LOWORD(wParam);
				auto isDragging = HIWORD(wParam);
				auto _this = GetAttachedThis(hWnd);

				return _this->HandleSpinnerChange(spinner, spinnerId, isDragging);
			}
			break;
		}

		return FALSE;
	}

	static Derived* GetAttachedThis(HWND hWnd)
	{
		auto wndUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
		auto _this = reinterpret_cast<Derived*>(wndUserData);
		FASSERT(_this != nullptr);

		return _this;
	}
};

FIRERENDER_NAMESPACE_END
