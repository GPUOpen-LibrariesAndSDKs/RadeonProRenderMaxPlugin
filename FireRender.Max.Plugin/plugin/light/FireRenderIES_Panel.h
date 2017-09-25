#pragma once

#include <functional>

#include "frScope.h"
#include "utils/KelvinToColor.h"

FIRERENDER_NAMESPACE_BEGIN

class FireRenderIESLight;

/* This class contains common code for 3dsMax specific controls
	'ControlTraits' must have:
		TControl typedef which represents 3dsMax control
		static TControl* Capture(HWND) method
		static void Release(TControl*) method
*/
template<typename ControlTraits>
class MaxControl
{
	using TControl = typename ControlTraits::TControl;

public:
	using Traits = ControlTraits;

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

		HWND controlHwnd = GetDlgItem(dialog, control);

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

	void Enable()
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->Enable(TRUE);
	}

	void Disable()
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->Enable(FALSE);
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
	static TControl* Capture(HWND wnd) { return GetISpinner(wnd); }
	static void Release(TControl* ctrl) { ReleaseISpinner(ctrl); }
};

class MaxEditTraits
{
public:
	using TControl = ICustEdit;
	static TControl* Capture(HWND wnd) { return GetICustEdit(wnd); }
	static void Release(TControl* ctrl) { ReleaseICustEdit(ctrl); }
};

class MaxButtonTraits
{
public:
	using TControl = ICustButton;
	static TControl* Capture(HWND wnd) { return GetICustButton(wnd); }
	static void Release(TControl* ctrl) { ReleaseICustButton(ctrl); }
};

class MaxColorSwatchTraits
{
public:
	using TControl = IColorSwatch;
	static TControl* Capture(HWND wnd) { return GetIColorSwatch(wnd); }
	static void Release(TControl* ctrl) { ReleaseIColorSwatch(ctrl); }
};

/* This class wraps 3dsMax ISpinnerControl */
class MaxSpinner :
	public MaxControl<MaxSpinnerTraits>
{
	using ParentClass = MaxControl<MaxSpinnerTraits>;
public:
	using ParentClass::ParentClass;
	using ParentClass::operator=;

	struct DefaultFloatSettings
	{
		static const float Min;
		static const float Max;
		static const float Default;
		static const float Delta;
	};

	struct DefaultRotationSettings
	{
		static const float Min;
		static const float Max;
		static const float Default;
		static const float Delta;
	};

	template<typename T>
	void SetLimits(T min, T max, bool limitCurrent = false)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetLimits(min, max, limitCurrent);
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
	void SetValue(T value, bool notify = false)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetValue(value, notify);
	}

	template<typename Settings>
	void SetSettings()
	{
		SetLimits(Settings::Min, Settings::Max);
		SetResetValue(Settings::Default);
		SetScale(Settings::Delta);
	}

	template<typename T>
	T GetValue() const
	{
		FASSERT(m_ctrl != nullptr);
		return static_cast<T>(GetValueHelper<T>::GetValue(m_ctrl));
	}

private:
	template<typename T, typename Enable = void>
	struct GetValueHelper;

	// Specialization for integral types
	template<typename T>
	struct GetValueHelper<T, std::enable_if_t<std::is_integral<T>::value>>
	{
		static int GetValue(Traits::TControl* pControl)
		{
			return pControl->GetIVal();
		}
	};

	// Specialization for floating-point types
	template<typename T>
	struct GetValueHelper<T, std::enable_if_t<std::is_floating_point<T>::value>>
	{
		static float GetValue(Traits::TControl* pControl)
		{
			return pControl->GetFVal();
		}
	};
};

/* This class wraps 3dsMax ICustEdit control */
class MaxEdit :
	public MaxControl<MaxEditTraits>
{
	using ParentClass = MaxControl<MaxEditTraits>;
public:
	using ParentClass::ParentClass;
	using ParentClass::operator=;

	template<typename T>
	T GetValue() const
	{
		FASSERT(m_ctrl != nullptr);
		return static_cast<T>(GetValueHelper<T>::GetValue(m_ctrl));
	}

private:
	template<typename T, typename Enable = void>
	struct GetValueHelper;

	// Specialization for integral types
	template<typename T>
	struct GetValueHelper<T, std::enable_if_t<std::is_integral<T>::value>>
	{
		static int GetValue(Traits::TControl* pControl)
		{
			return pControl->GetInt();
		}
	};

	// Specialization for floating-point types
	template<typename T>
	struct GetValueHelper<T, std::enable_if_t<std::is_floating_point<T>::value>>
	{
		static float GetValue(Traits::TControl* pControl)
		{
			return pControl->GetFloat();
		}
	};
};

/* This class wraps 3dsMax ICustButton control */
class MaxButton :
	public MaxControl<MaxButtonTraits>
{
	using ParentClass = MaxControl<MaxButtonTraits>;
public:
	using ParentClass::ParentClass;
	using ParentClass::operator=;

	void SetType(CustButType type)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetType(type);
	}

	void SetButtonDownNotify(bool notify)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetButtonDownNotify(notify);
	}

	RECT GetRect() const
	{
		FASSERT(m_ctrl != nullptr);

		RECT rect;
		BOOL ret = GetWindowRect(m_ctrl->GetHwnd(), &rect);
		FASSERT(ret);

		return rect;
	}

	bool PointIsOver(POINT pt) const
	{
		RECT rect = GetRect();
		return PtInRect(&rect, pt);
	}

	bool CursorIsOver() const
	{
		POINT cursorPos;
		BOOL ret = GetCursorPos(&cursorPos);
		FASSERT(ret);

		return PointIsOver(cursorPos);
	}
};

class MaxColorSwatch :
	public MaxControl<MaxColorSwatchTraits>
{
	using ParentClass = MaxControl<MaxColorSwatchTraits>;
public:
	using ParentClass::ParentClass;
	using ParentClass::operator=;

	Color GetColor() const
	{
		FASSERT(m_ctrl != nullptr);
		COLORREF c = m_ctrl->GetColor();

		Color result;
		result.r = GetRValue(c) / 255.f;
		result.g = GetGValue(c) / 255.f;
		result.b = GetBValue(c) / 255.f;

		return result;
	}

	void SetColor(Color c)
	{
		FASSERT(m_ctrl != nullptr);
		m_ctrl->SetColor(c);
	}
};

/* This class contains common code for Windows controls */
class WinControl
{
public:
	WinControl() :
		m_hWnd(nullptr)
	{}

	void Capture(HWND parentWindow, int controlId)
	{
		m_hWnd = GetDlgItem(parentWindow, controlId);
		CheckControl();
	}

	void Release()
	{
		m_hWnd = nullptr;
	}

protected:
	void CheckControl() const
	{
		FASSERT(m_hWnd != nullptr);
	}

	HWND m_hWnd;
};

// Combines 3dsMax edit and spinner controls in one object
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
		ICustEdit* pEdit = m_edit.GetControl();
		ISpinnerControl* pSpinner = m_spinner.GetControl();

		pSpinner->LinkToEdit(pEdit->GetHwnd(), editType);
	}

	void Release()
	{
		m_edit.Release();
		m_spinner.Release();
	}

	void Enable()
	{
		m_edit.Enable();
		m_spinner.Enable();
	}

	void Disable()
	{
		m_edit.Disable();
		m_spinner.Disable();
	}

	template<typename T>
	T GetValue() const
	{
		return m_spinner.GetValue<T>();
	}

	MaxEdit& GetEdit() { return m_edit; }
	MaxSpinner& GetSpinner() { return m_spinner; }

	const MaxEdit& GetEdit() const { return m_edit; }
	const MaxSpinner& GetSpinner() const { return m_spinner; }

	MaxEditAndSpinner& operator=(MaxEditAndSpinner&&) = default;
	MaxEditAndSpinner& operator=(const MaxEditAndSpinner&) = delete;

private:
	MaxEdit m_edit;
	MaxSpinner m_spinner;
};

// Combines 3dsMax colors watch, edit and spinner controls in one object
class MaxKelvinColor
{
public:
	MaxKelvinColor() = default;
	MaxKelvinColor(MaxKelvinColor&&) = default;
	MaxKelvinColor(const MaxKelvinColor&) = delete;

	void Capture(HWND window, int colorsWatch, int edit, int spinnerId)
	{
		m_color.Capture(window, colorsWatch);
		m_kelvin.Capture(window, edit, spinnerId);
		m_kelvin.Bind(EditSpinnerType::EDITTYPE_FLOAT);

		MaxSpinner& spinner = m_kelvin.GetSpinner();
		spinner.SetLimits(MinKelvin, MaxKelvin);
		spinner.SetResetValue(DefaultKelvin);
		spinner.SetScale(10.f);
	}

	float GetTemperature() const
	{
		return m_kelvin.GetValue<float>();
	}

	Color GetColor() const
	{
		return m_color.GetColor();
	}

	void SetTemperature(float value)
	{
		m_kelvin.GetSpinner().SetValue(value);
		UpdateColor();
	}

	void UpdateColor()
	{
		m_color.SetColor(KelvinToColor(GetTemperature()));
	}

	void Release()
	{
		m_color.Release();
		m_kelvin.Release();
	}

	void Enable()
	{
		m_color.Enable();
		m_kelvin.Enable();
	}

	void Disable()
	{
		m_color.Disable();
		m_kelvin.Disable();
	}

	MaxKelvinColor& operator=(MaxKelvinColor&&) = default;
	MaxKelvinColor& operator=(const MaxKelvinColor&) = delete;

private:
	MaxColorSwatch m_color;
	MaxEditAndSpinner m_kelvin;
};

/* Wraps Windows button control */
class WinButton :
	public WinControl
{
public:
	bool IsChecked() const
	{
		CheckControl();

		int result = Button_GetCheck(m_hWnd);
		FASSERT(result != BST_INDETERMINATE);

		return result == BST_CHECKED;
	}

	void SetCheck(bool checked)
	{
		CheckControl();
		Button_SetCheck(m_hWnd, checked ? BST_CHECKED : BST_UNCHECKED);
	}

	void Enable()
	{
		CheckControl();
		Button_Enable(m_hWnd, TRUE);
	}

	void Disable()
	{
		CheckControl();
		Button_Enable(m_hWnd, FALSE);
	}
};

/* Wraps Windows combo box control */
class WinCombobox :
	public WinControl
{
	using TString = std::basic_string<TCHAR>;

public:
	enum class Style
	{
		Dropdown,
		DropdownList
	};

	// Returns the index of selected item.
	// Returns -1 if nothing is selected
	int GetSelectedIndex() const
	{
		CheckControl();
		return ComboBox_GetCurSel(m_hWnd);
	}

	TString GetItemText(int index) const
	{
		TString result;
		GetItemText(index, result);
		return result;
	}

	void GetItemText(int index, TString& outText) const
	{
		CheckControl();

		int chars = ComboBox_GetLBTextLen(m_hWnd, index);
		FASSERT(chars != CB_ERR && "Index out of range");

		int len = chars + 1;
		outText.resize(len);
		int getLBText_result = ComboBox_GetLBText(m_hWnd, index, &outText[0]);
		FASSERT(getLBText_result != CB_ERR && getLBText_result == chars);
	}

	int AddItem(const TCHAR* name)
	{
		CheckControl();

		int index = ComboBox_AddString(m_hWnd, name);
		FASSERT(index != CB_ERR);
		FASSERT(index != CB_ERRSPACE);

		return index;
	}

	void DeleteItem(int index)
	{
		CheckControl();

		int res = ComboBox_DeleteString(m_hWnd, index);
		FASSERT(res != CB_ERR);
	}

	void SetItemData(int index, size_t data)
	{
		CheckControl();

		int ret = ComboBox_SetItemData(m_hWnd, index, data);
		FASSERT(ret != CB_ERR);
	}

	bool SetSelected(const TCHAR* text)
	{
		FASSERT(text != nullptr);

		return ForEachItem([&](int index, const TString& str)
		{
			if (_tcscmp(text, str.c_str()) == 0)
			{
				SetSelected(index);
				return true;
			}

			return false;
		});
	}

	// -1 to clear selection
	void SetSelected(int index)
	{
		CheckControl();

		int ret = ComboBox_SetCurSel(m_hWnd, index);

		// -1 is valid input but ComboBox_SetCurSel returns CB_ERR in this case
		FASSERT(ret == -1 || ret != CB_ERR);
	}

	int GetItemsCount() const
	{
		CheckControl();

		int result = ComboBox_GetCount(m_hWnd);
		FASSERT(result != CB_ERR);

		return result;
	}

	void Enable()
	{
		CheckControl();
		ComboBox_Enable(m_hWnd, TRUE);
	}

	void Disable()
	{
		CheckControl();
		ComboBox_Enable(m_hWnd, FALSE);
	}

	bool ForEachItem(std::function<bool(int index, const TString& str)> f)
	{
		int cnt = GetItemsCount();
		TString cache;

		for (int i = 0; i < cnt; ++i)
		{
			GetItemText(i, cache);
			
			if (f(i, cache))
			{
				return true;
			}
		}

		return false;
	}
};

/* This class contains common code to manage 3dsMax rollup pages.
 * Derived class must define:
 *	static DialogId variable;
 *	static PanelName variable.
 */
template<typename Derived>
class IES_Panel
{
public:
	using BasePanel = IES_Panel;

	IES_Panel(FireRenderIESLight* parent) :
		m_panel(nullptr),
		m_parent(parent)
	{}

	bool IsInitialized() const
	{
		return m_panel != nullptr;
	}

	void BeginEdit(IObjParam* objParam, ULONG flags, Animatable* prev)
	{
		FASSERT(m_panel == nullptr);

		Derived* _this = static_cast<Derived*>(this);

		m_panel = objParam->AddRollupPage(
			fireRenderHInstance,
			MAKEINTRESOURCE(Derived::DialogId),
			Derived::DlgProc,
			Derived::PanelName,
			(LPARAM)_this);

		LONG_PTR wndContext = reinterpret_cast<LONG_PTR>(_this);
		LONG_PTR prevLong = SetWindowLongPtr(m_panel, GWLP_USERDATA, wndContext);

		_this->InitializePage(objParam->GetTime());
		objParam->RegisterDlgWnd(m_panel);
		
		FASSERT(prevLong == 0);
	}

	void EndEdit(IObjParam* objParam, ULONG flags, Animatable* next)
	{
		FASSERT(IsInitialized());
		Derived* _this = static_cast<Derived*>(this);
		_this->UninitializePage();
		objParam->DeleteRollupPage(m_panel);
		m_panel = nullptr;
	}

	// These methods are optional to override in the derived class

	// Capture and initialize page controls
	bool InitializePage(TimeValue time) { return true; }

	// Release page controls
	void UninitializePage() {}

	// WM_COMMAND simplified event
	bool HandleControlCommand(TimeValue t, WORD code, WORD controlId) { return false; }

	// 3ds Max custom edit change event
	bool OnEditChange(TimeValue t, int editId, HWND editHWND) { return false; }

	// Spinner change event
	bool OnSpinnerChange(TimeValue t, ISpinnerControl* spinner, WORD controlId, bool isDragging) { return false; }

	// Color shatch change event
	bool OnColorSwatchChange(TimeValue t, IColorSwatch* colorSwatch, WORD controlId, bool final) { return false; }

	// Returns message for undo / redo stack
	const TCHAR* GetAcceptMessage(WORD controlId) const { return _T("IES light: change parameter"); }

	// Update controls state from param block
	void UpdateUI(TimeValue t) {}

	// Enable controls on the panel
	void Enable() {}

	// Disable controls on the panel
	void Disable() {}

protected:
	HWND m_panel;
	FireRenderIESLight* m_parent;

private:
	static void BeginUndoDelta()
	{
		theHold.Begin();
	}

	static void EndUndoDelta(bool accept, Derived* _this, int control)
	{
		if (accept)
		{
			theHold.Accept(_this->GetAcceptMessage(control));
		}
		else
		{
			theHold.Cancel();
		}
	}

	static INT_PTR CALLBACK DlgProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		switch (msg)
		{
			case WM_INITDIALOG:
				return TRUE;
				break;

			case BM_CLICK:
				FASSERT(false);
				break;

			case WM_COMMAND:
			{
				if (lParam != 0)
				{
					TimeValue time = GetCOREInterface()->GetTime();
					WORD code = HIWORD(wParam);
					WORD controlId = LOWORD(wParam);
					Derived* _this = GetAttachedThis(hWnd);

					BeginUndoDelta();
					bool accept = _this->HandleControlCommand(time, code, controlId);
					EndUndoDelta(accept, _this, controlId);
				}
			}
			break;

			case WM_CUSTEDIT_ENTER:
			{
				WPARAM customEditId = wParam;
				HWND customEditHWND = reinterpret_cast<HWND>(lParam);
				Derived* _this = GetAttachedThis(hWnd);
				TimeValue time = GetCOREInterface()->GetTime();
			
				BeginUndoDelta();
				bool accept = _this->OnEditChange(time, customEditId, customEditHWND);
				EndUndoDelta(accept, _this, customEditId);
			
				return TRUE;
			}
			break;

			// start dragging the spinner
			case CC_SPINNER_BUTTONDOWN:
				BeginUndoDelta();
			break;

			// on change value in the spinner
			case CC_SPINNER_CHANGE:
			{
				ISpinnerControl* spinner = reinterpret_cast<ISpinnerControl*>(lParam);
				WORD spinnerId = LOWORD(wParam);
				WORD isDragging = HIWORD(wParam);
				Derived* _this = GetAttachedThis(hWnd);
				TimeValue time = GetCOREInterface()->GetTime();

				if (!isDragging)
				{
					BeginUndoDelta();
				}

				bool accept = _this->OnSpinnerChange(time, spinner, spinnerId, isDragging);

				if (!isDragging)
				{
					EndUndoDelta(accept, _this, spinnerId);
				}

				return TRUE;
			}
			break;

			// finish spinner editing
			case CC_SPINNER_BUTTONUP:
			{
				Derived* _this = GetAttachedThis(hWnd);
				ISpinnerControl* spinner = reinterpret_cast<ISpinnerControl*>(lParam);
				WORD spinnerId = LOWORD(wParam);
				WORD accept = HIWORD(wParam);
				TimeValue time = GetCOREInterface()->GetTime();
				bool retVal = _this->OnSpinnerChange(time, spinner, spinnerId, false);

				EndUndoDelta(accept, _this, spinnerId);
			}
			break;

			case CC_COLOR_CHANGE:
			case CC_COLOR_CLOSE:
			{
				IColorSwatch* controlPtr = reinterpret_cast<IColorSwatch*>(lParam);
				WORD controlId = LOWORD(wParam);
				Derived* _this = GetAttachedThis(hWnd);
				TimeValue time = GetCOREInterface()->GetTime();

				BeginUndoDelta();
				bool accept = _this->OnColorSwatchChange(time, controlPtr, controlId, msg == CC_COLOR_CLOSE);
				EndUndoDelta(accept, _this, controlId);
			}
			break;
		}

		return FALSE;
	}

	static Derived* GetAttachedThis(HWND hWnd)
	{
		LONG_PTR wndUserData = GetWindowLongPtr(hWnd, GWLP_USERDATA);
		Derived* _this = reinterpret_cast<Derived*>(wndUserData);
		FASSERT(_this != nullptr);

		return _this;
	}
};

namespace ies_panel_utils
{
	// Need this helper to use partial specialization
	template<typename Control, bool enable>
	struct EnableControlHelper
	{
		static void EnablePanel(Control& control)
		{
			control.Enable();
		}
	};

	// Disable case specialization
	template<typename Control>
	struct EnableControlHelper<Control, false>
	{
		static void EnablePanel(Control& control)
		{
			control.Disable();
		}
	};

	template<bool enable, typename Control>
	void EnableControl(Control& control)
	{
		EnableControlHelper<Control, enable>::EnablePanel(control);
	}

	template<bool enable, typename... Control>
	void EnableControls(Control&... controls)
	{
		// Call EnableControl for each control
		(void)std::initializer_list<int>
		{
			(EnableControl<enable>(controls), 0)...
		};
	}
}

FIRERENDER_NAMESPACE_END
