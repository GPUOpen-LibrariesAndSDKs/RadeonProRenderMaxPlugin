#pragma once

#include "frScope.h"

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

/* This class wraps 3dsMax ISpinnerControl */
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
		static decltype(auto) GetValue(Traits::TControl* pControl)
		{
			return pControl->GetInt();
		}
	};

	// Specialization for floating-point types
	template<typename T>
	struct GetValueHelper<T, std::enable_if_t<std::is_floating_point<T>::value>>
	{
		static decltype(auto) GetValue(Traits::TControl* pControl)
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
		FASSERT(m_hWnd != nullptr);
	}

	void Release()
	{
		m_hWnd = nullptr;
	}

protected:
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

/* Wraps Windows check box control */
class WinCheckbox :
	public WinControl
{
public:
	bool IsChecked() const
	{
		FASSERT(m_hWnd != nullptr);

		auto result = Button_GetCheck(m_hWnd);
		FASSERT(result != BST_INDETERMINATE);

		return result == BST_CHECKED;
	}

	void SetCheck(bool checked)
	{
		FASSERT(m_hWnd != nullptr);
		Button_SetCheck(m_hWnd, checked ? BST_CHECKED : BST_UNCHECKED);
	}
};

/* Wraps Windows combo box control */
class WinCombobox :
	public WinControl
{
	using TString = std::basic_string<TCHAR>;

public:
	// Returns the index of selected item.
	// Returns -1 if nothing is selected
	int GetSelectedIndex() const
	{
		FASSERT(m_hWnd != nullptr);

		auto curSel = ComboBox_GetCurSel(m_hWnd);
		FASSERT(curSel != CB_ERR);

		return curSel;
	}

	TString GetItemText(int index) const
	{
		FASSERT(m_hWnd != nullptr);

		auto chars = ComboBox_GetLBTextLen(m_hWnd, index);
		FASSERT(chars != CB_ERR && "Index out of range");

		TString result;
		auto len = chars + 1;
		result.resize(len);
		auto getLBText_result = ComboBox_GetLBText(m_hWnd, index, &result[0]);
		FASSERT(getLBText_result != CB_ERR && getLBText_result == chars);

		return result;
	}

	int AddItem(const TCHAR* name)
	{
		FASSERT(m_hWnd != nullptr);

		auto index = ComboBox_AddString(m_hWnd, name);
		FASSERT(index != CB_ERR);
		FASSERT(index != CB_ERRSPACE);

		return index;
	}

	void SetItemData(int index, size_t data)
	{
		FASSERT(m_hWnd != nullptr);

		auto ret = ComboBox_SetItemData(m_hWnd, index, data);
		FASSERT(ret != CB_ERR);
	}

	// -1 to clear selection
	void SetSelected(int index)
	{
		FASSERT(m_hWnd != nullptr);

		auto ret = ComboBox_SetCurSel(m_hWnd, index);

		// -1 is valid input but ComboBox_SetCurSel returns CB_ERR in this case
		FASSERT(ret == -1 || ret != CB_ERR);
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

	// These methods are optional to override in the derived class
	bool InitializePage() { return true; }
	void UninitializePage() {}
	INT_PTR HandleControlCommand(WORD code, WORD controlId) { return FALSE; }
	INT_PTR OnEditChange(int editId, HWND editHWND) { return FALSE; }
	INT_PTR OnSpinnerChange(ISpinnerControl* spinner, WORD controlId, bool isDragging) { return FALSE; }
	INT_PTR OnButtonClick(WORD controlId) { return FALSE; }

protected:
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

				return _this->OnEditChange(customEditId, customEditHWND);
			}
			break;

			case CC_SPINNER_CHANGE:
			{
				auto spinner = reinterpret_cast<ISpinnerControl*>(lParam);
				auto spinnerId = LOWORD(wParam);
				auto isDragging = HIWORD(wParam);
				auto _this = GetAttachedThis(hWnd);

				return _this->OnSpinnerChange(spinner, spinnerId, isDragging);
			}
			break;

			case WM_MENUSELECT:
			{
				if (lParam != 0)
				{
					auto controlId = LOWORD(wParam);
					auto _this = GetAttachedThis(hWnd);

					return _this->OnButtonClick(controlId);
				}
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
