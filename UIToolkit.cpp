#include <windows.h>
#include <d2d1.h>
#include <iostream>
#include <string>
#include <vector>
#pragma comment(lib, "d2d1")

#include "basewin.h"
#include "WICViewerD2D.h"
#include "LoadBitmapFromFile.h"
#include "SafeRelease.h"

#include "FormControl.h"
#include "SlabDecomposition.h"
#include "stringToLPCWSTR.h"

using std::string;
using std::vector;

double clamp(double value, double min, double max) {
	return min(max(value, min), max);
}

int clamp(int value, int min, int max) {
	return min(max(value, min), max);
}

bool dragging = false;
float prevX;
float prevY;
float pageX = 0.0;
float pageY = 0.0;
SlabContainer *mySlabContainer = new SlabContainer;
Region* selRegion;
vector<FormControl*> controls = {};
IWICImagingFactory* m_pWICFactory;
ID2D1Bitmap* m_pBitmap;
ID2D1HwndRenderTarget* pRenderTarget = NULL;
ID2D1SolidColorBrush* pFillBrush = nullptr;
ID2D1SolidColorBrush* pStrokeBrush = nullptr;
IDWriteFactory* m_pDWriteFactory = nullptr;
//IDWriteTextFormat* m_pTextFormat = nullptr;
float lineWidth = 1.0;
void FillRect(float x, float y, float width, float height) {
	pRenderTarget->FillRectangle(D2D1::RectF(x, y, x + width, y + height), pFillBrush);
}
void StrokeRect(float x, float y, float width, float height) {
	pRenderTarget->DrawRectangle(D2D1::RectF(x, y, x + width, y + height), pStrokeBrush, lineWidth);
}
void FillEllipse(float x, float y, float width, float height) {
	pRenderTarget->FillEllipse(D2D1::Ellipse(D2D1::Point2F(
		x + width / 2.0,
		y + height / 2.0
	), width / 2.0, height / 2.0), pFillBrush);
}
void StrokeEllipse(float x, float y, float width, float height) {
	pRenderTarget->DrawEllipse(D2D1::Ellipse(D2D1::Point2F(
		x + width / 2.0,
		y + height / 2.0
	), width / 2.0, height / 2.0), pStrokeBrush, lineWidth);
}
HRESULT SetFillColor(float red, float green, float blue) {
	if (pFillBrush != nullptr) {
		SafeRelease(pFillBrush);
	}
	const D2D1_COLOR_F color = D2D1::ColorF(red / 255.0, green / 255.0, blue / 255.0);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(color, &pFillBrush);
	return hr;
}
HRESULT SetFillColor(Color newColor) {
	if (pFillBrush != nullptr) {
		SafeRelease(pFillBrush);
	}
	const D2D1_COLOR_F color = D2D1::ColorF(
		newColor.red / 255.0,
		newColor.green / 255.0,
		newColor.blue / 255.0,
		newColor.alpha
	);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(color, &pFillBrush);
	return hr;
}
HRESULT SetStrokeColor(float red, float green, float blue) {
	if (pStrokeBrush != nullptr) {
		SafeRelease(pStrokeBrush);
	}
	const D2D1_COLOR_F color = D2D1::ColorF(red / 255.0, green / 255.0, blue / 255.0);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(color, &pStrokeBrush);
	return hr;
}
HRESULT SetStrokeColor(Color newColor) {
	if (pStrokeBrush != nullptr) {
		SafeRelease(pStrokeBrush);
	}
	const D2D1_COLOR_F color = D2D1::ColorF(newColor.red / 255.0, newColor.green / 255.0, newColor.blue / 255.0);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(color, &pStrokeBrush);
	return hr;
}
void SetLineWidth(float newLineWidth) {
	lineWidth = newLineWidth;
}

/*void SetFont(string fontName, static const float fontSize, bool bold, bool italic) {
	if (m_pTextFormat != nullptr) {
		SafeRelease(m_pTextFormat);
	}
	HRESULT hr = m_pDWriteFactory->CreateTextFormat(
		stringToLPCWSTR(fontName),
		NULL,
		bold ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
		italic ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		fontSize,
		L"", //locale
		&m_pTextFormat
	);
	if (SUCCEEDED(hr))
	{
		// Center the text horizontally and vertically.
		m_pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
		m_pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
	}
}*/

DWRITE_TEXT_METRICS MeasureText(string text, IDWriteTextFormat* m_pTextFormat) {
	IDWriteTextLayout* pTextLayout_ = NULL;
	string s1 = text;
	std::wstring widestr = std::wstring(s1.begin(), s1.end());
	HRESULT hr = m_pDWriteFactory->CreateTextLayout(
		widestr.c_str(),      // The string to be laid out and formatted.
		text.size(),  // The length of the string.
		m_pTextFormat,  // The text format to apply to the string (contains font information, etc).
		1366,         // The width of the layout box.
		768,        // The height of the layout box.
		&pTextLayout_  // The IDWriteTextLayout interface pointer.
	);

	DWRITE_TEXT_METRICS metrics;
	pTextLayout_->GetMetrics(&metrics);

	SafeRelease(&pTextLayout_);

	return metrics;
}

DWRITE_TEXT_METRICS FillText(string text, float x, float y, IDWriteTextFormat* m_pTextFormat) {
	IDWriteTextLayout* pTextLayout_ = NULL;
	string s1 = text;
	std::wstring widestr = std::wstring(s1.begin(), s1.end());
	HRESULT hr = m_pDWriteFactory->CreateTextLayout(
		widestr.c_str(),      // The string to be laid out and formatted.
		text.size(),  // The length of the string.
		m_pTextFormat,  // The text format to apply to the string (contains font information, etc).
		1366,         // The width of the layout box.
		768,        // The height of the layout box.
		&pTextLayout_  // The IDWriteTextLayout interface pointer.
	);

	DWRITE_TEXT_METRICS metrics;
	pTextLayout_->GetMetrics(&metrics);

	pRenderTarget->DrawText(
		widestr.c_str(),
		text.size(),
		m_pTextFormat,
		D2D1::RectF(x, y, x + metrics.widthIncludingTrailingWhitespace, y + metrics.height),
		pFillBrush
	);

	SafeRelease(&pTextLayout_);

	return metrics;
}

void DrawBitmap(string src, float x, float y, float width, float height) {
	HRESULT hr = LoadBitmapFromFile(
		pRenderTarget,
		m_pWICFactory,
		stringToLPCWSTR(src),
		0,
		0,
		&m_pBitmap
	);

	if (SUCCEEDED(hr))
	{
		// Retrieve the size of the bitmap.
		D2D1_SIZE_F size = m_pBitmap->GetSize();

		// Draw a bitmap.
		pRenderTarget->DrawBitmap(
			m_pBitmap,
			D2D1::RectF(x, y, x + width, y + height)
		);

		SafeRelease(&m_pBitmap);
	}
}

class Label : public FormControl {
public:
	FormControl* target = nullptr;
	Label(string label, float x, float y) : FormControl(label, x, y) {
		type = FormLabel;
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		width = metrics.width;
		height = metrics.height;
	};
	void Render() {
		SetFillColor(getBackColor());
		FillRect(x, y, width, height);
		SetFillColor(getForeColor());
		FillText(label, x, y, m_pTextFormat);
	}
};

class BitmapImage : public FormControl {
public:
	string src;
	BitmapImage(string src, float x, float y, float width, float height) : FormControl("", x, y) {
		this->type = FormBitmapImage;
		this->src = src;
		this->width = width;
		this->height = height;
	}
	void Render() {
		DrawBitmap(src, x, y, width, height);
	}
};

class MenuBar;

class MenuBarItem {
public:
	string label;
	MenuBar *menuBar = nullptr;
	MenuBarItem(string label) : label(label) {};
	MenuBarItem(string label, MenuBar* menuBar) : label(label), menuBar(menuBar) {};
};

class MenuBar : public FormControl {
private:
	vector<MenuBarItem> items;
	float itemHeight;
	int hoveredItem = -1;
public:
	MenuBar* parent = nullptr;
	bool visible = false;
	MenuBar(float x, float y) : FormControl("", x, y) {
		type = FormMenuBar;
		width = 0;
		height = 0;
		setBackColor(Color(211.0, 211.0, 211.0, 1.0));
		setForeColor(Color(0.0, 0.0, 0.0, 1.0));
		setItemHeight(30.0);
	};
	int getMenuBarItemIndex(MenuBar *menuBar) {
		for (int i = 0, len = items.size(); i < len; i++) {
			if (items[i].menuBar == menuBar) {
				return i;
			}
		}
		return -1;
	}
	int getHoveredItem() {
		return hoveredItem;
	}
	void setHoveredItem(int newHoveredItem) {
		hoveredItem = clamp(newHoveredItem, -1, items.size());
	}
	int getItemCount() {
		return items.size();
	}
	MenuBarItem& getItem(int itemIndex) {
		return items[itemIndex];
	}
	float getItemHeight() {
		return itemHeight;
	}
	void setItemHeight(float newItemHeight) {
		itemHeight = newItemHeight;
	}
	void AddItem(string label, MenuBar *menuBar) {
		items.push_back(MenuBarItem(label, menuBar));
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		menuBar->parent = this;
		menuBar->x = x + metrics.widthIncludingTrailingWhitespace;
		menuBar->y = y + height;
		menuBar->width = max(menuBar->width, metrics.widthIncludingTrailingWhitespace);
		width = max(width, metrics.widthIncludingTrailingWhitespace);
		height = items.size() * getItemHeight();
	}
	void AddItem(string label) {
		items.push_back(MenuBarItem(label));
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		width = max(width, metrics.widthIncludingTrailingWhitespace);
		height = items.size() * getItemHeight();
	}
	void RepositionRelativeToParent() {
		MenuBar* parentMenu = parent;
		for (int i = 0, len = getItemCount(); i < len; i++) {
			MenuBarItem& menuBarItem = getItem(i);
			if (menuBarItem.menuBar != nullptr) {
				menuBarItem.menuBar->RepositionRelativeToParent();
			}
		}
		while (parentMenu != nullptr) {
			x = parentMenu->x + parentMenu->width;
			y = parentMenu->y + parentMenu->getMenuBarItemIndex(this) * parentMenu->getItemHeight();
			parentMenu = parentMenu->parent;
		}
	}
	void Render() {
		SetFillColor(getBackColor());
		FillRect(x, y, width, height);
		int menuY = y;
		for (int i = 0, len = items.size(); i < len; i++) {
			if (i == getHoveredItem()) {
				SetFillColor(Color(161.0, 161.0, 161.0, 1.0));
				FillRect(x, menuY, width, getItemHeight());
			}
			SetFillColor(getForeColor());
			FillText(items[i].label, x, menuY, m_pTextFormat);
			menuY += getItemHeight();
			if (items[i].menuBar != nullptr) {
				if (items[i].menuBar->visible) {
					items[i].menuBar->x = x + width;
					if (!dragging) {
						items[i].menuBar->Render();
					}
				}
			}
		}
	}
};

class ToolbarIcon {
public:
	string label;
	string src;
	ToolbarIcon(string label, string src, void (*clickHandler)(ToolbarIcon& icon)) : label(label), src(src) {
		if (clickHandler != nullptr) {
			this->clickHandler = clickHandler;
		}
	};
	void (*clickHandler)(ToolbarIcon &icon) = nullptr;
};

class Toolbar : public FormControl {
private:
	vector<ToolbarIcon> icons = {};
	float iconWidth;
	float iconHeight;
	float margin;
	int hoveredIcon = -1;
public:
	Toolbar(float x, float y) : FormControl("", x, y) {
		type = FormToolbar;
		setIconWidth(16.0);
		setIconHeight(16.0);
		setMargin(8.0);
		setBackColor(Color(211.0, 211.0, 211.0, 1.0));
		width = 0;
		height = getIconHeight() + getMargin() * 2;
	}
	ToolbarIcon& getIcon(int iconIndex) {
		if (iconIndex >= 0 && iconIndex <= icons.size() - 1) {
			return icons[iconIndex];
		}
		return icons[0];
	}
	int getHoveredIcon() {
		return hoveredIcon;
	}
	void setHoveredIcon(int newHoveredIcon) {
		hoveredIcon = clamp(newHoveredIcon, -1, icons.size());
	}
	float getMargin() {
		return margin;
	}
	void setMargin(float newMargin) {
		margin = newMargin;
	}
	float getIconWidth() {
		return iconWidth;
	}
	float getIconHeight() {
		return iconHeight;
	}
	void setIconWidth(float newWidth) {
		iconWidth = newWidth;
	}
	void setIconHeight(float newHeight) {
		iconHeight = newHeight;
	}
	void Render() {
		float x = this->x;
		float y = this->y;
		SetFillColor(getBackColor());
		FillRect(x, y, width, height);
		for (int i = 0, len = icons.size(); i < len; i++) {
			if (i == getHoveredIcon()) {
				SetFillColor(Color(161.0, 161.0, 161.0, 1.0));
				FillRect(x, y, getIconWidth() + getMargin() * 2, getIconHeight() + getMargin() * 2);
				SetFillColor(getBackColor());
			}
			DrawBitmap(icons[i].src, x + getMargin(), y + getMargin(), getIconWidth(), getIconHeight());
			x += getIconWidth() + getMargin() * 2;
		}
	}
	void AddIcon(string label, string src, void (*clickHandler)(ToolbarIcon& icon)) {
		icons.push_back(ToolbarIcon(label, src, clickHandler));
		width = icons.size() * (getIconWidth() + getMargin() * 2);
	}
};

class ComboBox : public FormControl {
public:
	vector<string> options;
	int selectedIndex = -1;
	float itemHeight;
	ComboBox(vector<string> options, float x, float y) : FormControl("", x, y) {
		type = FormComboBox;
		width = 150.0;
		height = 30.0;
		this->options = options;
		if (options.size() > 0) {
			selectedIndex = 0;
		}
		setBackColor(Color(255.0, 255.0, 255.0, 1.0));
	}
	void Render() {
		if (focused) {
			SetFillColor(211.0, 211.0, 211.0);
		}
		else {
			SetFillColor(getBackColor());
		}
		SetStrokeColor(161.0, 161.0, 161.0);
		FillRect(x, y, width, height);
		StrokeRect(x, y, width, height);
		SetFillColor(getForeColor());
		DWRITE_TEXT_METRICS metrics = FillText(selectedIndex == -1 ? "" : options[selectedIndex], x + 10, y + 5, m_pTextFormat);
		DrawBitmap("menu_open.png", x + width - 24 - 5, y + 5, 24, 24);
		if (toggled) {
			SetFillColor(getBackColor());
			StrokeRect(x, y + height, width, (metrics.height + 20) * options.size() + 10);
			FillRect(x, y + height, width, (metrics.height + 20) * options.size() + 10);
			float optionY = y + height + 10.0;
			SetFillColor(getForeColor());
			for (int i = 0, len = options.size(); i < len; i++) {
				if (pageY > (optionY - 10.0) && pageY < (optionY + metrics.height + 10.0)) {
					SetFillColor(211.0, 211.0, 211.0);
					FillRect(x, optionY - 10.0, width, metrics.height + 20);
					SetFillColor(getForeColor());
				}
				FillText(options[i], x + 10, optionY, m_pTextFormat);
				optionY += metrics.height + 20;
			}
			itemHeight = metrics.height + 20;
		}
	}
};

class Textbox : public FormControl {
public:
	std::string value = "";
	int charIndex = 0;
	Textbox(std::string label, float x, float y) : FormControl(label, x, y) {
		type = FormTextbox;
		width = 150.0;
		height = 30.0;
		setBackColor(Color(255.0, 255.0, 255.0, 1.0));
	};
	void Render() {
		if (focused) {
			SetFillColor(211.0, 211.0, 211.0);
		}
		else {
			SetFillColor(getBackColor());
		}
		SetStrokeColor(161.0, 161.0, 161.0);
		FillRect(x, y, width, height);
		StrokeRect(x, y, width, height);
		SetFillColor(0.0, 0.0, 0.0);
		FillText(value, x + 10, y + 5, m_pTextFormat);
		DWRITE_TEXT_METRICS metrics = MeasureText(value.substr(0, charIndex), m_pTextFormat);
		SetStrokeColor(30.0, 30.0, 30.0);
		StrokeRect(x + 10 + metrics.widthIncludingTrailingWhitespace, y + 5, 1, metrics.height);
	}
};

class Button : public FormControl {
public:
	Button(std::string label, float x, float y) : FormControl(label, x, y) {
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		type = FormButton;
		width = metrics.width + 20;
		height = metrics.height + 20;
		setBackColor(Color(211.0, 211.0, 211.0, 1.0));
	};
	void Render() {
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		if (hover) {
			SetFillColor(161.0, 161.0, 161.0);
		}
		else {
			SetFillColor(getBackColor());
		}
		FillRect(x, y, 20 + metrics.width, 20 + metrics.height);
		SetStrokeColor(0.0, 0.0, 0.0);
		StrokeRect(x, y, 20 + metrics.width, 20 + metrics.height);
		Color foreColor = getForeColor();
		SetFillColor(foreColor.red, foreColor.green, foreColor.blue);
		FillText(label, x + 10, y + 10, m_pTextFormat);
	}
};

class RadioButton : public FormControl {
public:
	string groupName;
	RadioButton(std::string label, float x, float y) : FormControl(label, x, y) {
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		type = FormRadioButton;
		width = metrics.height + 10 + metrics.width;
		height = metrics.height + 10;
	};
	void Render() {
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		SetFillColor(getBackColor());
		FillRect(x, y, width, height);
		SetFillColor(getForeColor());
		string src = toggled ? "radiobutton.png" : "radiobutton_unchecked.png";
		DrawBitmap(src, x, y, metrics.height, metrics.height);
		FillText(label, x + metrics.height + 10, y, m_pTextFormat);
	}
};

class Checkbox : public FormControl {
public:
	Checkbox(std::string label, float x, float y) : FormControl(label, x, y) {
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		type = FormCheckbox;
		width = metrics.height + 10 + metrics.width;
		height = metrics.height + 10;
	};
	void Render() {
		DWRITE_TEXT_METRICS metrics = MeasureText(label, m_pTextFormat);
		SetFillColor(getBackColor());
		FillRect(x, y, width, height);
		SetFillColor(getForeColor());
		string src = toggled ? "checkbox.png" : "checkbox_unchecked.png";
		DrawBitmap(src, x, y, metrics.height, metrics.height);
		FillText(label, x + metrics.height + 10, y, m_pTextFormat);
	}
};

BitmapImage* BitmapImage1 = nullptr;
MenuBar* ContextMenu1 = nullptr;
FormControl* currentControl = nullptr;
Textbox* TextboxInFocus = nullptr;
ComboBox* ComboBoxInFocus = nullptr;
ComboBox* ComboBox1 = nullptr;

class MainWindow : public BaseWindow<MainWindow>
{
	ID2D1Factory* pFactory;
	bool success;
	float slabLeft, slabRight, regionTop, regionBottom;
	void    CalculateLayout();
	HRESULT CreateGraphicsResources();
	void    DiscardGraphicsResources();
	void    OnPaint();
	void    Resize();
	void    OnMouseMove(int pixelX, int pixelY, DWORD flags);
	void    OnLButtonDown(int pixelX, int pixelY, DWORD flags);
	void    OnRButtonDown(int pixelX, int pixelY, DWORD flags);
	void	OnLButtonUp();

public:

	MainWindow() : pFactory(NULL)
	{
	}

	PCWSTR  ClassName() const { return L"Circle Window Class"; }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
};

// Recalculate drawing layout when the size of the window changes.

void MainWindow::CalculateLayout()
{
	if (pRenderTarget != NULL)
	{
		D2D1_SIZE_F size = pRenderTarget->GetSize();
		const float x = size.width / 2;
		const float y = size.height / 2;
		const float radius = min(x, y);
	}
}

class DPIScale
{
	static float scaleX;
	static float scaleY;

public:
	static void Initialize(ID2D1Factory* pFactory)
	{
		FLOAT dpiX, dpiY;
		pFactory->GetDesktopDpi(&dpiX, &dpiY);
		scaleX = dpiX / 96.0f;
		scaleY = dpiY / 96.0f;
	}

	template <typename T>
	static D2D1_POINT_2F PixelsToDips(T x, T y)
	{
		return D2D1::Point2F(static_cast<float>(x) / scaleX, static_cast<float>(y) / scaleY);
	}
};

float DPIScale::scaleX = 1.0f;
float DPIScale::scaleY = 1.0f;

HRESULT MainWindow::CreateGraphicsResources()
{
	HRESULT hr = S_OK;
	if (pRenderTarget == NULL)
	{
		RECT rc;

		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		hr = pFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hwnd, size),
			&pRenderTarget);

		HRESULT hr = CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&m_pWICFactory)
		);

		if (SUCCEEDED(hr))
		{
			if (SUCCEEDED(hr))
			{
				CalculateLayout();
			}
		}
	}
	return hr;
}

void MainWindow::DiscardGraphicsResources()
{
	SafeRelease(&pRenderTarget);
}

void SelectImage(FormControl* control) {
	float width;
	float height;
	if (control->label == "logo.png") {
		width = 160.0;
		height = 50.0;
	}
	else if (control->label == "apollo.jpg") {
		width = 232.0;
		height = 152.0;
	}
	else if (control->label == "browser_small.png") {
		width = 320.0;
		height = 164.0;
	}
	else if (control->label == "jpeg_small.png") {
		width = 320.0;
		height = 240.0;
	}
	else if (control->label == "stratolabs_.png") {
		width = 335.0;
		height = 339.0;
	}
	else {
		return;
	}
	BitmapImage1->src = control->label;
	BitmapImage1->width = width;
	BitmapImage1->height = height;
}

Textbox* Textbox1;

void addComboBoxItem(FormControl* control) {
	ComboBox1->options.push_back(Textbox1->value);
}

void removeComboBoxItem(FormControl* control) {
	if (ComboBox1->selectedIndex != -1) {
		ComboBox1->options.erase(ComboBox1->options.begin() + ComboBox1->selectedIndex);
		ComboBox1->selectedIndex = clamp(ComboBox1->selectedIndex - 1, 0, ComboBox1->options.size() - 1);
	}
}

void ClickBack(ToolbarIcon& icon) {
	MessageBox(NULL, L"Going back one page...", L"Back", MB_OK);
}

void ClickForward(ToolbarIcon& icon) {
	MessageBox(NULL, L"Going forward one page...", L"Forward", MB_OK);
}

void ClickHome(ToolbarIcon& icon) {
	MessageBox(NULL, L"Loading the home page!", L"Home", MB_OK);
}

vector<RadioButton*> getRadioButtonsByGroupName(string groupName) {
	vector<RadioButton*> retval;
	for (int i = 0, len = controls.size(); i < len; i++) {
		if (controls[i]->type == FormRadioButton) {
			RadioButton* control = (RadioButton*)controls[i];
			if (control->groupName == groupName) {
				retval.push_back(control);
			}
		}
	}
	return retval;
}

void MainWindow::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		pRenderTarget->BeginDraw();

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		hr = DemoApp::CreateDeviceIndependentResources();

		if (MainWindow::success && selRegion != nullptr)
		{
			SetStrokeColor(255.0, 0.0, 0.0);

			for (int i = 0, len = controls.size(); i < len; i++) {
				controls[i]->hover = false;
				if (controls[i]->type == FormToolbar) {
					Toolbar* toolbar = (Toolbar*)controls[i];
					toolbar->setHoveredIcon(-1);
				}
			}

			for (int i = 0, len = selRegion->shapes.size(); i < len; i++)
			{
				FormControl* control = selRegion->shapes[i]->control;
				control->hover = true;
				if (control->type == FormToolbar) {
					Toolbar* toolbar = (Toolbar*)control;
					toolbar->setHoveredIcon(int(floor((pageX - toolbar->x) / (toolbar->getIconWidth() + toolbar->getMargin() * 2))));
				}
				StrokeRect(
					selRegion->shapes[i]->x1,
					selRegion->shapes[i]->y1,
					selRegion->shapes[i]->x2 - selRegion->shapes[i]->x1,
					selRegion->shapes[i]->y2 - selRegion->shapes[i]->y1
				);
			}
		}

		IDWriteTextFormat* m_pTestTextFormat = nullptr;

		HRESULT hr = m_pDWriteFactory->CreateTextFormat(
			L"Courier New",
			NULL,
			DWRITE_FONT_WEIGHT_NORMAL,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			16,
			L"", //locale
			&m_pTestTextFormat
		);
		if (SUCCEEDED(hr))
		{
			// Center the text horizontally and vertically.
			m_pTestTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
			m_pTestTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
		}

		SetStrokeColor(0.0, 0.0, 0.0);

		SetFillColor(255.0, 0.0, 0.0);
		FillRect(10 + 500, 10, 100, 100);
		StrokeRect(10 + 500, 120, 100, 100);

		SetFillColor(255.0, 255.0, 0.0);
		FillEllipse(120 + 500, 10, 100, 100);
		StrokeEllipse(120 + 500, 120, 100, 100);

		float y = 240.0;
		DrawBitmap("logo.png", 10 + 500, y, 160, 50);
		y += 50.0;
		SetFillColor(0.0, 0.0, 0.0);
		y += 20.0;
		DWRITE_TEXT_METRICS metrics = FillText("Copyright 2021 Snikta. All rights reserved.", 10 + 500, y, m_pTestTextFormat);
		y += metrics.height + 10;
		float copyrightNoticeY = y;

		SafeRelease(m_pTestTextFormat);

		if (mySlabContainer->NextAvailableShapeId == 1) {
			float y = 10.0;
			MenuBar* MenuBar1 = new MenuBar(10.0F, y);
			MenuBar1->visible = true;
			MenuBar* FileMenu = new MenuBar(10.0F, y);
			MenuBar* EditMenu = new MenuBar(10.0F, y);
			MenuBar* ViewMenu = new MenuBar(10.0F, y);
			MenuBar* FormatMenu = new MenuBar(10.0F, y);
			MenuBar* LineJoinMenu = new MenuBar(10.0F, y);
			MenuBar* LineCapMenu = new MenuBar(10.0F, y);
			MenuBar* OrderMenu = new MenuBar(10.0F, y);
			MenuBar* AlignMenu = new MenuBar(10.0F, y);
			MenuBar* AlignVerticalMenu = new MenuBar(10.0F, y);
			MenuBar* AlignHorizontalMenu = new MenuBar(10.0F, y);
			MenuBar* CenterOnPageMenu = new MenuBar(10.0F, y);
			MenuBar* MakeSameSizeMenu = new MenuBar(10.0F, y);
			MenuBar* EqualizeGapsMenu = new MenuBar(10.0F, y);
			MenuBar* FlipMenu = new MenuBar(10.0F, y);
			MenuBar* ToolsMenu = new MenuBar(10.0F, y);
			MenuBar* TableMenu = new MenuBar(10.0F, y);
			MenuBar* HelpMenu = new MenuBar(10.0F, y);
			HelpMenu->AddItem("About");
			TableMenu->AddItem("Insert Row Above");
			TableMenu->AddItem("Insert Row Below");
			TableMenu->AddItem("Insert Column Left");
			TableMenu->AddItem("Insert Column Right");
			TableMenu->AddItem("Select Cells");
			TableMenu->AddItem("Merge Cells");
			ToolsMenu->AddItem("Macros");
			ToolsMenu->AddItem("Form Designer");
			FlipMenu->AddItem("Horizontally");
			FlipMenu->AddItem("Vertically");
			FlipMenu->AddItem("Both");
			EqualizeGapsMenu->AddItem("Horizontally");
			EqualizeGapsMenu->AddItem("Vertically");
			EqualizeGapsMenu->AddItem("Both");
			MakeSameSizeMenu->AddItem("Width");
			MakeSameSizeMenu->AddItem("Height");
			CenterOnPageMenu->AddItem("Horizontally");
			CenterOnPageMenu->AddItem("Vertically");
			CenterOnPageMenu->AddItem("Both");
			AlignHorizontalMenu->AddItem("Left");
			AlignHorizontalMenu->AddItem("Center");
			AlignHorizontalMenu->AddItem("Right");
			AlignVerticalMenu->AddItem("Top");
			AlignVerticalMenu->AddItem("Middle");
			AlignVerticalMenu->AddItem("Bottom");
			AlignMenu->AddItem("Vertically", AlignVerticalMenu);
			AlignMenu->AddItem("Horizontally", AlignHorizontalMenu);
			OrderMenu->AddItem("Bring To Front");
			OrderMenu->AddItem("Send To Back");
			OrderMenu->AddItem("Bring Forward");
			OrderMenu->AddItem("Send Backward");
			LineJoinMenu->AddItem("Round");
			LineJoinMenu->AddItem("Bevel");
			LineJoinMenu->AddItem("Miter");
			LineCapMenu->AddItem("Butt");
			LineCapMenu->AddItem("Round");
			LineCapMenu->AddItem("Square");
			FormatMenu->AddItem("Line Join", LineJoinMenu);
			FormatMenu->AddItem("Line Cap", LineCapMenu);
			FormatMenu->AddItem("Order", OrderMenu);
			FormatMenu->AddItem("Align", AlignMenu);
			FormatMenu->AddItem("Center On Page", CenterOnPageMenu);
			FormatMenu->AddItem("Make Same Size", MakeSameSizeMenu);
			FormatMenu->AddItem("Equalize Gaps", EqualizeGapsMenu);
			FormatMenu->AddItem("Flip", FlipMenu);
			ViewMenu->AddItem("Increase Zoom");
			ViewMenu->AddItem("Decrease Zoom");
			ViewMenu->AddItem("Reset Zoom");
			EditMenu->AddItem("Cut");
			EditMenu->AddItem("Copy");
			EditMenu->AddItem("Paste");
			EditMenu->AddItem("Duplicate");
			EditMenu->AddItem("Delete");
			EditMenu->AddItem("Select All");
			EditMenu->AddItem("Find");
			EditMenu->AddItem("Replace");
			MenuBar* MicrosoftMenu = new MenuBar(10.0F, y);
			MenuBar* DocumentMenu = new MenuBar(10.0F, y);
			MenuBar* NewMenu = new MenuBar(10.0F, y);
			MenuBar* GoogleMenu = new MenuBar(10.0F, y);
			GoogleMenu->AddItem("Docs");
			GoogleMenu->AddItem("Sheets");
			GoogleMenu->AddItem("Slides");
			MicrosoftMenu->AddItem("Word");
			MicrosoftMenu->AddItem("Works");
			DocumentMenu->AddItem("Microsoft", MicrosoftMenu);
			DocumentMenu->AddItem("Google", GoogleMenu);
			NewMenu->AddItem("Document", DocumentMenu);
			FileMenu->AddItem("New", NewMenu);
			FileMenu->AddItem("Open");
			FileMenu->AddItem("Save");
			FileMenu->AddItem("Save As");
			FileMenu->AddItem("Export...");
			MenuBar1->AddItem("File", FileMenu);
			MenuBar1->AddItem("Edit", EditMenu);
			MenuBar1->AddItem("View", ViewMenu);
			MenuBar1->AddItem("Format", FormatMenu);
			MenuBar1->AddItem("Tools", ToolsMenu);
			MenuBar1->AddItem("Table", TableMenu);
			MenuBar1->AddItem("Help", HelpMenu);

			ContextMenu1 = new MenuBar(10.0F, y);
			MenuBar* CopyMenu = new MenuBar(10.0F, y);
			CopyMenu->AddItem("Image");
			CopyMenu->AddItem("Text");
			MenuBar* PasteMenu = new MenuBar(10.0F, y);
			PasteMenu->AddItem("HTML");
			PasteMenu->AddItem("RTF");
			MenuBar* DocsMenu = new MenuBar(10.0F, y);
			DocsMenu->AddItem("Wrapped Image");
			DocsMenu->AddItem("Chart");
			DocsMenu->AddItem("Rich Text");
			DocsMenu->AddItem("Table");
			MenuBar* SheetsMenu = new MenuBar(10.0F, y);
			SheetsMenu->AddItem("Formula");
			SheetsMenu->AddItem("Value");
			SheetsMenu->AddItem("Equation");
			SheetsMenu->AddItem("Pivot Table");
			MenuBar* GoogleMenu2 = new MenuBar(10.0F, y);
			GoogleMenu2->AddItem("Docs", DocsMenu);
			GoogleMenu2->AddItem("Sheets", SheetsMenu);
			ContextMenu1->AddItem("Cut");
			ContextMenu1->AddItem("Copy", CopyMenu);
			ContextMenu1->AddItem("Paste", PasteMenu);
			ContextMenu1->AddItem("Duplicate");
			ContextMenu1->AddItem("Delete");
			ContextMenu1->AddItem("Select All");
			ContextMenu1->AddItem("Google", GoogleMenu2);

			y += MenuBar1->height + 10.0;
			Toolbar* Toolbar1 = new Toolbar(10.0F, y);
			Toolbar1->AddIcon("Back", "icons/back.png", ClickBack);
			Toolbar1->AddIcon("Forward", "icons/forward.png", ClickForward);
			Toolbar1->AddIcon("Home", "icons/home.png", ClickHome);
			Toolbar1->AddIcon("Refresh", "icons/refresh.png", nullptr);
			Toolbar1->AddIcon("Stop", "icons/stop.png", nullptr);
			Toolbar1->AddIcon("History", "icons/history.png", nullptr);
			Toolbar1->AddIcon("Downloads", "icons/downloads.png", nullptr);
			Toolbar1->AddIcon("Bookmarks", "icons/bookmarks.png", nullptr);
			y += Toolbar1->height + 10.0;
			Label* Label1 = new Label("Select Item:", 10.0F, y);
			ComboBox1 = new ComboBox({ "Arial", "Tahoma", "Comic Sans MS", "Times New Roman", "Calibri", "Verdana" }, Label1->x + Label1->width + 10.0F, y);
			Label1->target = ComboBox1;
			y += ComboBox1->height + 10.0;
			ComboBox1->setFontName("Calibri");
			ComboBox1->setBold(true);
			ComboBox1->setBackColor(Color(0.0, 0.0, 128.0, 1.0));
			ComboBox1->setForeColor(Color(255.0, 255.0, 255.0, 1.0));
			Label* Label2 = new Label("New Item:", 10.0F, y);
			Textbox1 = new Textbox("Textbox1", Label2->x + Label2->width + 10.0F, y);
			Textbox1->setBackColor(Color(255.0, 255.0, 0.0, 1.0));
			Label2->target = Textbox1;
			Textbox1->setFontName("Times New Roman");
			y += Textbox1->height + 10.0;
			Button* Button1 = new Button("Add", 10.0F, y);
			Button1->clickHandler = addComboBoxItem;
			Button1->setForeColor(Color(255.0F, 255.0F, 255.0F, 1.0F));
			Button1->setBackColor(Color(128.0F, 0.0F, 0.0F, 1.0F));
			Button1->setBold(true);
			Button1->setItalic(true);
			y += Button1->height + 10.0;
			Button* Button2 = new Button("Remove", 10.0F, y);
			Button2->clickHandler = removeComboBoxItem;
			y += Button2->height + 10.0;
			RadioButton* RadioButton1 = new RadioButton("RadioButton1", 10.0F, y);
			y += RadioButton1->height + 10.0;
			Checkbox* Checkbox1 = new Checkbox("Checkbox1", 10.0F, y);
			y += Checkbox1->height + 10.0;
			y = copyrightNoticeY;
			RadioButton* RadioButton2 = new RadioButton("logo.png", 10.0F + 500.0F, y);
			RadioButton2->clickHandler = SelectImage;
			RadioButton2->groupName = "ImageSelect";
			y += RadioButton2->height + 10.0;
			RadioButton* RadioButton3 = new RadioButton("apollo.jpg", 10.0F + 500.0F, y);
			RadioButton3->clickHandler = SelectImage;
			RadioButton3->groupName = "ImageSelect";
			y += RadioButton3->height + 10.0;
			RadioButton* RadioButton4 = new RadioButton("browser_small.png", 10.0F + 500.0F, y);
			RadioButton4->clickHandler = SelectImage;
			RadioButton4->groupName = "ImageSelect";
			y += RadioButton4->height + 10.0;
			RadioButton* RadioButton5 = new RadioButton("jpeg_small.png", 10.0F + 500.0F, y);
			RadioButton5->clickHandler = SelectImage;
			RadioButton5->groupName = "ImageSelect";
			y += RadioButton5->height + 10.0;
			RadioButton* RadioButton6 = new RadioButton("stratolabs_.png", 10.0F + 500.0F, y);
			RadioButton6->clickHandler = SelectImage;
			RadioButton6->groupName = "ImageSelect";
			y += RadioButton6->height + 10.0;
			BitmapImage1 = new BitmapImage("logo.png", 10.0F + 500.0F, y, 160.0, 50.0);

			controls.push_back(MenuBar1);
			controls.push_back(Toolbar1);
			controls.push_back(Label1);
			controls.push_back(ComboBox1);
			controls.push_back(Label2);
			controls.push_back(Textbox1);
			controls.push_back(Button1);
			controls.push_back(Button2);
			controls.push_back(RadioButton1);
			controls.push_back(Checkbox1);
			controls.push_back(RadioButton2);
			controls.push_back(RadioButton3);
			controls.push_back(RadioButton4);
			controls.push_back(RadioButton5);
			controls.push_back(RadioButton6);
			controls.push_back(BitmapImage1);

			vector<Shape*> shapesToPreprocess;
			for (int i = 0, len = controls.size(); i < len; i++)
			{
				FormControl *control = controls[i];
				int shapeId = mySlabContainer->NextAvailableShapeId++;
				int x1, x2, y1, y2;
				Shape* newShape = new Shape;
				newShape->id = shapeId;
				newShape->x1 = control->x;
				newShape->x2 = control->x + control->width;
				newShape->y1 = control->y;
				newShape->y2 = control->y + control->height;
				newShape->control = control;
				shapesToPreprocess.push_back(newShape);
			}
			mySlabContainer->preprocessSubdivision(shapesToPreprocess, 'x', nilSlab);
		}

		for (int i = 0, len = controls.size(); i < len; i++) {
			if (!controls[i]->focused) {
				controls[i]->Render();
			}
		}

		for (int i = 0, len = controls.size(); i < len; i++) {
			if (controls[i]->focused || (controls[i]->type == FormMenuBar && ((MenuBar*)controls[i])->visible)) {
				controls[i]->Render();
			}
		}

		if (ContextMenu1->visible) {
			ContextMenu1->Render();
		}

		hr = pRenderTarget->EndDraw();
		if (FAILED(hr) || hr == D2DERR_RECREATE_TARGET)
		{
			DiscardGraphicsResources();
		}
		EndPaint(m_hwnd, &ps);
	}
}

void MainWindow::Resize()
{
	if (pRenderTarget != NULL)
	{
		RECT rc;
		GetClientRect(m_hwnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right, rc.bottom);

		pRenderTarget->Resize(size);
		CalculateLayout();
		InvalidateRect(m_hwnd, NULL, FALSE);
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR, int nCmdShow)
{
	MainWindow win;

	if (!win.Create(L"UIToolkit", WS_OVERLAPPEDWINDOW))
	{
		return 0;
	}

	ShowWindow(win.Window(), nCmdShow);

	// Run the message loop.

	MSG msg = { };
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

vector<MenuBar*> visibleMenus;

void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	const D2D1_POINT_2F dips = DPIScale::PixelsToDips(pixelX, pixelY);
	float x2 = dips.x;
	float y2 = dips.y;

	pageX = x2;
	pageY = y2;

	MainWindow::success = false;

	bool ContextMenuWasVisible = false;
	if (ContextMenu1 != nullptr) {
		ContextMenuWasVisible = ContextMenu1->visible;
	}

	for (int i = 0, len = visibleMenus.size(); i < len; i++) {
		visibleMenus[i]->visible = false;
		visibleMenus[i]->setHoveredItem(-1);
		for (int j = 0, jLen = visibleMenus[i]->getItemCount(); j < jLen; j++) {
			if (visibleMenus[i]->getItem(j).menuBar != nullptr) {
				visibleMenus[i]->getItem(j).menuBar->visible = false;
				visibleMenus[i]->getItem(j).menuBar->setHoveredItem(-1);
			}
		}
	}
	for (int i = 0, len = visibleMenus.size(); i < len; i++) {
		MenuBar* visibleMenu = visibleMenus[i];
		if (
			pageX >= visibleMenu->x &&
			pageX <= (visibleMenu->x + visibleMenu->width) &&
			pageY >= visibleMenu->y &&
			pageY <= (visibleMenu->y + visibleMenu->height)
			) {
			for (int j = 0, jLen = visibleMenu->getItemCount(); j < jLen; j++) {
				if (pageY >= (visibleMenu->y + j * visibleMenu->getItemHeight()) && pageY <= (visibleMenu->y + (j + 1) * visibleMenu->getItemHeight())) {
					MenuBarItem menuBarItem = visibleMenu->getItem(j);
					visibleMenu->setHoveredItem(j);
					if (menuBarItem.menuBar != nullptr) {
						visibleMenus.clear();
						menuBarItem.menuBar->x = visibleMenu->x + visibleMenu->width;
						menuBarItem.menuBar->y = visibleMenu->y + j * visibleMenu->getItemHeight();
						MenuBar* parentMenuBar = menuBarItem.menuBar;
						while (parentMenuBar != nullptr) {
							visibleMenus.push_back(parentMenuBar);
							if (parentMenuBar->parent != nullptr) {
								parentMenuBar->parent->setHoveredItem(parentMenuBar->parent->getMenuBarItemIndex(parentMenuBar));
							}
							parentMenuBar->visible = true;
							parentMenuBar = parentMenuBar->parent;
						}
					}
					else {
						visibleMenus.clear();
						visibleMenus.push_back(visibleMenu);
						visibleMenu->visible = true;
						MenuBar* parentMenuBar = visibleMenu;
						while (parentMenuBar != nullptr) {
							visibleMenus.push_back(parentMenuBar);
							if (parentMenuBar->parent != nullptr) {
								parentMenuBar->parent->setHoveredItem(parentMenuBar->parent->getMenuBarItemIndex(parentMenuBar));
							}
							parentMenuBar->visible = true;
							parentMenuBar = parentMenuBar->parent;
						}
					}
					if (ContextMenuWasVisible) {
						if (ContextMenu1 != nullptr) {
							if (std::find(visibleMenus.begin(), visibleMenus.end(), ContextMenu1) == visibleMenus.end()) {
								visibleMenus.push_back(ContextMenu1);
							}
							ContextMenu1->visible = true;
						}
					}
					return;
				}
			}
		}
	}

	visibleMenus.clear();
	if (selRegion != nullptr) {
		for (int i = 0, len = selRegion->shapes.size(); i < len; i++) {
			FormControl* control = selRegion->shapes[i]->control;
			if (!(pageX >= control->x && pageX <= (control->x + control->width) && pageY >= control->y && pageY <= (control->y + control->height))) {
				continue;
			}
			if (control->type == FormMenuBar) {
				MenuBar* menuBar = (MenuBar*)control;
				int menuBarItemIndex = int(floor(pageY - menuBar->y) / menuBar->getItemHeight());
				if (menuBarItemIndex >= 0 && menuBarItemIndex <= menuBar->getItemCount() - 1) {
					MenuBarItem& menuBarItem = menuBar->getItem(menuBarItemIndex);
					if (menuBarItem.menuBar != nullptr) {
						menuBarItem.menuBar->visible = true;
						if (std::find(visibleMenus.begin(), visibleMenus.end(), menuBarItem.menuBar) == visibleMenus.end()) {
							visibleMenus.push_back(menuBarItem.menuBar);
						}
						menuBar->setHoveredItem(menuBarItemIndex);
						for (int j = 0, jLen = menuBar->getItemCount(); j < jLen; j++) {
							if (j != menuBarItemIndex) {
								MenuBar* itemChildMenu = menuBar->getItem(j).menuBar;
								if (itemChildMenu != nullptr) {
									itemChildMenu->visible = false;
									itemChildMenu->setHoveredItem(-1);
								}
							}
						}
					}
				}
			}
		}
	}

	RedBlackTree* rbt = mySlabContainer->RBTSlabLines;

	if (rbt != nullptr) {
		RedBlackNode* node = rbt->closest(x2);
		Slab* slab = &nilSlab;
		bool success = false, slabExists = false;

		if (node->key > x2)
		{
			int nodeKey = rbt->predecessor(node)->key;
			slabExists = mySlabContainer->SlabLinesByLeft.find(nodeKey) != mySlabContainer->SlabLinesByLeft.end();
			if (slabExists)
			{
				slab = mySlabContainer->SlabLinesByLeft[nodeKey];
			}
		}
		else
		{
			slabExists = mySlabContainer->SlabLinesByLeft.find(node->key) != mySlabContainer->SlabLinesByLeft.end();
			if (slabExists)
			{
				slab = mySlabContainer->SlabLinesByLeft[node->key];
			}
		}

		if (slabExists && x2 >= slab->leftX && x2 <= slab->rightX)
		{
			rbt = slab->RBTRegions;
			node = rbt->closest(y2);

			bool regionExists;
			Region* region = &nilRegion;
			if (node->key > y2)
			{
				int regionKey = rbt->predecessor(node)->key;
				regionExists = slab->RegionsByTop.find(regionKey) != slab->RegionsByTop.end();
				if (regionExists)
				{
					region = slab->RegionsByTop[regionKey];
				}
			}
			else
			{
				regionExists = slab->RegionsByTop.find(node->key) != slab->RegionsByTop.end();
				if (regionExists)
				{
					region = slab->RegionsByTop[node->key];
				}
			}

			if (regionExists && y2 >= region->topY && y2 <= region->bottomY)
			{
				selRegion = region;

				MainWindow::slabLeft = slab->leftX;
				MainWindow::slabRight = slab->rightX;

				MainWindow::regionTop = region->topY;
				MainWindow::regionBottom = region->bottomY;

				MainWindow::success = true;
			}
		}
	}

	if (ContextMenuWasVisible) {
		if (ContextMenu1 != nullptr) {
			if (std::find(visibleMenus.begin(), visibleMenus.end(), ContextMenu1) == visibleMenus.end()) {
				visibleMenus.push_back(ContextMenu1);
			}
			ContextMenu1->visible = true;
		}
	}

	if (currentControl != nullptr) {
		currentControl->x += pageX - prevX;
		currentControl->y += pageY - prevY;
	}
	prevX = pixelX;
	prevY = pixelY;
}

void MainWindow::OnRButtonDown(int pixelX, int pixelY, DWORD flags) {
	ContextMenu1->x = pixelX;
	ContextMenu1->y = pixelY;
	ContextMenu1->visible = true;
	OnPaint();
}

void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags) {
	ContextMenu1->visible = false;
	ContextMenu1->setHoveredItem(-1);
	for (int i = 0, len = controls.size(); i < len; i++) {
		FormControl* control = controls[i];
		if (control->type == FormComboBox && control->focused == true) {
			ComboBox* combo = (ComboBox*)control;
			if (pixelY >= (combo->y + combo->height) && pixelY <= (combo->y + combo->height + combo->itemHeight * combo->options.size())) {
				combo->selectedIndex = clamp(int(floor((pixelY - (combo->y + combo->height)) / combo->itemHeight)), 0, combo->options.size() - 1);
				combo->toggled = false;
				combo->focused = false;
				return;
			}
		}
		if (control->type == FormRadioButton) {

		}
		controls[i]->focused = false;
	}

	if (selRegion != nullptr) {
		for (int i = 0, len = selRegion->shapes.size(); i < len; i++)
		{
			FormControl* control = selRegion->shapes[i]->control;
			if (!(pageX >= control->x && pageX <= (control->x + control->width) && pageY >= control->y && pageY <= (control->y + control->height))) {
				continue;
			}
			control->focused = true;
			if (control->type == FormLabel) {
				Label* label = (Label*)control;
				if (label->target != nullptr) {
					label->target->focused = true;
					if (label->target->type == FormTextbox) {
						TextboxInFocus = (Textbox*)label->target;
					}
					else if (label->target->type == FormComboBox) {
						ComboBoxInFocus = (ComboBox*)label->target;
						ComboBoxInFocus->toggled = true;
					}
				}
			}
			else if (control->type == FormToolbar) {
				Toolbar* toolbar = (Toolbar*)control;
				if (toolbar != nullptr && toolbar->getHoveredIcon() != -1) {
					ToolbarIcon& icon = toolbar->getIcon(toolbar->getHoveredIcon());
					if (icon.clickHandler && icon.clickHandler != nullptr) {
						if (!dragging) {
							icon.clickHandler(icon);
						}
					}
				}
			}
			if (control->clickHandler != nullptr) {
				if (!dragging) {
					control->clickHandler(control);
				}
			}
			if (control->type == FormComboBox) {
				ComboBoxInFocus = (ComboBox*)control;
			}
			if (control->type == FormTextbox) {
				if (!dragging) {
					TextboxInFocus = (Textbox*)control;
					int charIndex = 0;
					int x = TextboxInFocus->x + 10;
					while (charIndex < TextboxInFocus->value.size() && x < (TextboxInFocus->x + TextboxInFocus->width)) {
						x = TextboxInFocus->x + 10 + MeasureText(TextboxInFocus->value.substr(0, charIndex + 1), TextboxInFocus->m_pTextFormat).widthIncludingTrailingWhitespace;
						charIndex++;
						if (x >= pixelX) {
							charIndex--;
							break;
						}
					}
					TextboxInFocus->charIndex = clamp(charIndex, 0, TextboxInFocus->value.size());
				}
			}
		}
	}

	if (selRegion != nullptr) {
		for (int i = 0, len = selRegion->shapes.size(); i < len; i++) {
			FormControl* control = selRegion->shapes[i]->control;
			if (!(pageX >= control->x && pageX <= (control->x + control->width) && pageY >= control->y && pageY <= (control->y + control->height))) {
				continue;
			}
			currentControl = control;
		}
	}
	if (currentControl != nullptr) {
		dragging = true;
	}

	OnPaint();
	prevX = pixelX;
	prevY = pixelY;
}

void MainWindow::OnLButtonUp() {
	if (dragging) {
		if (currentControl != nullptr) {
			Shape* newShape = new Shape;

			mySlabContainer->deleteShape(currentControl->ptrShape);

			selRegion = nullptr;

			int newShapeId = mySlabContainer->NextAvailableShapeId++;
			newShape->id = newShapeId;
			newShape->x1 = currentControl->x;
			newShape->x2 = currentControl->x + currentControl->width;
			newShape->y1 = currentControl->y;
			newShape->y2 = currentControl->y + currentControl->height;
			newShape->control = currentControl;

			mySlabContainer->ShapeMembers[newShapeId] = newShape;
			mySlabContainer->addShape(newShape);

			if (currentControl->type == FormMenuBar) {
				((MenuBar*)currentControl)->RepositionRelativeToParent();
			}
		}
		dragging = false;
		currentControl = nullptr;
	}
	
	if (selRegion != nullptr) {
		for (int i = 0, len = selRegion->shapes.size(); i < len; i++)
		{
			FormControl* control = selRegion->shapes[i]->control;
			control->toggled = !control->toggled;
			if (control->toggled == true) {
				if (control->type == FormRadioButton) {
					RadioButton* radioButton = (RadioButton*)control;
					vector<RadioButton*> radioButtonGroup = getRadioButtonsByGroupName(radioButton->groupName);
					for (int j = 0, jLen = radioButtonGroup.size(); j < jLen; j++) {
						if (radioButtonGroup[j] != radioButton) {
							radioButtonGroup[j]->toggled = false;
						}
					}
				}
			}
		}
		OnPaint();
	}
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		if (FAILED(D2D1CreateFactory(
			D2D1_FACTORY_TYPE_SINGLE_THREADED, &pFactory)))
		{
			return -1;  // Fail CreateWindowEx.
		}
		return 0;

	case WM_DESTROY:
		DiscardGraphicsResources();
		SafeRelease(&pFactory);
		PostQuitMessage(0);
		return 0;

	case WM_KEYDOWN:
		if (TextboxInFocus != nullptr) {
			bool shouldContinue = false;
			switch (wParam) {
			case VK_LEFT:
				TextboxInFocus->charIndex = clamp(TextboxInFocus->charIndex - 1, 0, TextboxInFocus->value.size());
				break;
			case VK_RIGHT:
				TextboxInFocus->charIndex = clamp(TextboxInFocus->charIndex + 1, 0, TextboxInFocus->value.size());
				break;
			case VK_HOME:
				TextboxInFocus->charIndex = 0;
				break;
			case VK_END:
				TextboxInFocus->charIndex = TextboxInFocus->value.size();
				break;
			case VK_DELETE:
				if (TextboxInFocus->charIndex == TextboxInFocus->value.size()) {
					return 0;
				}
				TextboxInFocus->value = TextboxInFocus->value.substr(0, TextboxInFocus->charIndex) + TextboxInFocus->value.substr(TextboxInFocus->charIndex + 1);
				break;
			case VK_BACK:
				if (TextboxInFocus->charIndex == 0) {
					return 0;
				}
				TextboxInFocus->value = TextboxInFocus->value.substr(0, TextboxInFocus->charIndex - 1) + TextboxInFocus->value.substr(TextboxInFocus->charIndex);
				TextboxInFocus->charIndex = max(0, TextboxInFocus->charIndex - 1);
				break;
			default:
				shouldContinue = true;
				break;
			}
			if (!shouldContinue) {
				OnPaint();
				return 0;
			}
		}
		return 0;

	case WM_CHAR:
		if (wParam == VK_BACK) {
			return 0;
		}
		if (TextboxInFocus != nullptr) {
			if (wParam >= 32 && wParam <= 126) {
				TextboxInFocus->value = TextboxInFocus->value.substr(0, TextboxInFocus->charIndex) + char(wParam) + TextboxInFocus->value.substr(min(TextboxInFocus->value.size(), TextboxInFocus->charIndex));
				TextboxInFocus->charIndex++;
			}
		}

		OnPaint();
		return 0;

	case WM_PAINT:
		//OnPaint();
		return 0;

	case WM_MOUSEMOVE:
		OnMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		OnPaint();
		return 0;

	case WM_LBUTTONDOWN:
		OnLButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		return 0;

	case WM_RBUTTONDOWN:
		OnRButtonDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), (DWORD)wParam);
		return 0;

	case WM_LBUTTONUP:
		OnLButtonUp();
		return 0;

	case WM_SIZE:
		Resize();
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}