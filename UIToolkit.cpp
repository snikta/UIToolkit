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

using std::string;
using std::vector;

double clamp(double value, double min, double max) {
	return min(max(value, min), max);
}

int clamp(int value, int min, int max) {
	return min(max(value, min), max);
}

// START: https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode
std::wstring s2ws(const string& s)
{
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}
LPCWSTR stringToLPCWSTR(string str) {
	std::wstring stemp = s2ws(str);
	LPCWSTR result = stemp.c_str();
	return result;
}
// END: https://stackoverflow.com/questions/27220/how-to-convert-stdstring-to-lpcwstr-in-c-unicode*/

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
IDWriteFactory* m_pDWriteFactory;
IDWriteTextFormat* m_pTextFormat = nullptr;
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
HRESULT SetStrokeColor(float red, float green, float blue) {
	if (pStrokeBrush != nullptr) {
		SafeRelease(pStrokeBrush);
	}
	const D2D1_COLOR_F color = D2D1::ColorF(red / 255.0, green / 255.0, blue / 255.0);
	HRESULT hr = pRenderTarget->CreateSolidColorBrush(color, &pStrokeBrush);
	return hr;
}
void SetLineWidth(float newLineWidth) {
	lineWidth = newLineWidth;
}

void SetFont(string fontName, static const float fontSize, bool bold, bool italic) {
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
}

DWRITE_TEXT_METRICS MeasureText(string text) {
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

DWRITE_TEXT_METRICS FillText(string text, float x, float y) {
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
	}
	void Render() {
		if (focused) {
			SetFillColor(211.0, 211.0, 211.0);
		}
		else {
			SetFillColor(255.0, 255.0, 255.0);
		}
		SetStrokeColor(161.0, 161.0, 161.0);
		FillRect(x, y, width, height);
		StrokeRect(x, y, width, height);
		SetFillColor(0.0, 0.0, 0.0);
		DWRITE_TEXT_METRICS metrics = FillText(selectedIndex == -1 ? "" : options[selectedIndex], x + 10, y + 5);
		DrawBitmap("menu_open.png", x + width - 24 - 5, y + 5, 24, 24);
		if (toggled) {
			SetFillColor(255.0, 255.0, 255.0);
			StrokeRect(x, y + height, width, (metrics.height + 20) * options.size() + 10);
			FillRect(x, y + height, width, (metrics.height + 20) * options.size() + 10);
			float optionY = y + height + 10.0;
			SetFillColor(0.0, 0.0, 0.0);
			for (int i = 0, len = options.size(); i < len; i++) {
				if (pageY > (optionY - 10.0) && pageY < (optionY + metrics.height + 10.0)) {
					SetFillColor(211.0, 211.0, 211.0);
					FillRect(x, optionY - 10.0, width, metrics.height + 20);
					SetFillColor(0.0, 0.0, 0.0);
				}
				FillText(options[i], x + 10, optionY);
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
	};
	void Render() {
		if (focused) {
			SetFillColor(211.0, 211.0, 211.0);
		}
		else {
			SetFillColor(255.0, 255.0, 255.0);
		}
		SetStrokeColor(161.0, 161.0, 161.0);
		FillRect(x, y, width, height);
		StrokeRect(x, y, width, height);
		SetFillColor(0.0, 0.0, 0.0);
		FillText(value, x + 10, y + 5);
		DWRITE_TEXT_METRICS metrics = MeasureText(value.substr(0, charIndex));
		SetStrokeColor(30.0, 30.0, 30.0);
		StrokeRect(x + 10 + metrics.widthIncludingTrailingWhitespace, y + 5, 1, metrics.height);
	}
};

class Button : public FormControl {
public:
	Button(std::string label, float x, float y) : FormControl(label, x, y) {
		DWRITE_TEXT_METRICS metrics = MeasureText(label);
		type = FormButton;
		width = metrics.width + 20;
		height = metrics.height + 20;
	};
	void Render() {
		DWRITE_TEXT_METRICS metrics = MeasureText(label);
		if (hover) {
			SetFillColor(161.0, 161.0, 161.0);
		}
		else {
			SetFillColor(211.0, 211.0, 211.0);
		}
		FillRect(x, y, 20 + metrics.width, 20 + metrics.height);
		SetStrokeColor(0.0, 0.0, 0.0);
		StrokeRect(x, y, 20 + metrics.width, 20 + metrics.height);
		SetFillColor(0.0, 0.0, 0.0);
		FillText(label, x + 10, y + 10);
	}
};

class RadioButton : public FormControl {
public:
	RadioButton(std::string label, float x, float y) : FormControl(label, x, y) {
		DWRITE_TEXT_METRICS metrics = MeasureText(label);
		type = FormRadioButton;
		width = metrics.height + 10 + metrics.width;
		height = metrics.height + 10;
	};
	void Render() {
		DWRITE_TEXT_METRICS metrics = MeasureText(label);
		SetFillColor(0.0, 0.0, 0.0);
		SetStrokeColor(0.0, 0.0, 0.0);
		string src = toggled ? "radiobutton.png" : "radiobutton_unchecked.png";
		DrawBitmap(src, x, y, metrics.height, metrics.height);
		FillText(label, x + metrics.height + 10, y);
	}
};

class Checkbox : public FormControl {
public:
	Checkbox(std::string label, float x, float y) : FormControl(label, x, y) {
		DWRITE_TEXT_METRICS metrics = MeasureText(label);
		type = FormCheckbox;
		width = metrics.height + 10 + metrics.width;
		height = metrics.height + 10;
	};
	void Render() {
		DWRITE_TEXT_METRICS metrics = MeasureText(label);
		SetFillColor(0.0, 0.0, 0.0);
		SetStrokeColor(0.0, 0.0, 0.0);
		string src = toggled ? "checkbox.png" : "checkbox_unchecked.png";
		DrawBitmap(src, x, y, metrics.height, metrics.height);
		FillText(label, x + metrics.height + 10, y);
	}
};

Textbox* TextboxInFocus = nullptr;
ComboBox* ComboBoxInFocus = nullptr;

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

void addComboBoxItem() {
	ComboBoxInFocus->options.push_back(TextboxInFocus->value);
}

void removeComboBoxItem() {
	if (ComboBoxInFocus->selectedIndex != -1) {
		ComboBoxInFocus->options.erase(ComboBoxInFocus->options.begin() + ComboBoxInFocus->selectedIndex);
		ComboBoxInFocus->selectedIndex = clamp(ComboBoxInFocus->selectedIndex - 1, 0, ComboBoxInFocus->options.size() - 1);
	}
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
			}

			for (int i = 0, len = selRegion->shapes.size(); i < len; i++)
			{
				selRegion->shapes[i]->control->hover = true;
				StrokeRect(
					selRegion->shapes[i]->x1,
					selRegion->shapes[i]->y1,
					selRegion->shapes[i]->x2 - selRegion->shapes[i]->x1,
					selRegion->shapes[i]->y2 - selRegion->shapes[i]->y1
				);
			}
		}

		/*SetStrokeColor(0.0, 0.0, 0.0);

		SetFillColor(255.0, 0.0, 0.0);
		FillRect(10, 10, 100, 100);
		StrokeRect(10, 120, 100, 100);

		SetFillColor(255.0, 255.0, 0.0);
		FillEllipse(120, 10, 100, 100);
		StrokeEllipse(120, 120, 100, 100);

		float y = 240.0;
		DrawBitmap("logo.png", 10, y, 160, 50);
		y += 50.0;
		SetFillColor(0.0, 0.0, 0.0);
		y += 20.0;
		DWRITE_TEXT_METRICS metrics = FillText("Copyright 2021 Snikta. All rights reserved.", 10, y);
		y += metrics.height + 10;*/

		SetFont("Arial", 16, false, false);

		if (mySlabContainer->NextAvailableShapeId == 1) {
			float y = 10.0;
			ComboBox* ComboBox1 = new ComboBox({ "Arial", "Tahoma", "Comic Sans MS", "Times New Roman", "Calibri", "Verdana" }, 10.0F, y);
			y += ComboBox1->height + 10.0;
			Textbox* Textbox1 = new Textbox("Textbox1", 10.0F, y);
			y += Textbox1->height + 10.0;
			Button* Button1 = new Button("Add", 10.0F, y);
			Button1->clickHandler = addComboBoxItem;
			y += Button1->height + 10.0;
			Button* Button2 = new Button("Remove", 10.0F, y);
			Button2->clickHandler = removeComboBoxItem;
			y += Button2->height + 10.0;
			RadioButton* RadioButton1 = new RadioButton("RadioButton1", 10.0F, y);
			y += RadioButton1->height + 10.0;
			Checkbox* Checkbox1 = new Checkbox("Checkbox1", 10.0F, y);

			controls.push_back(ComboBox1);
			controls.push_back(Textbox1);
			controls.push_back(Button1);
			controls.push_back(Button2);
			controls.push_back(RadioButton1);
			controls.push_back(Checkbox1);

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
			if (controls[i]->focused) {
				controls[i]->Render();
			}
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

	if (!win.Create(L"Circle", WS_OVERLAPPEDWINDOW))
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

void MainWindow::OnMouseMove(int pixelX, int pixelY, DWORD flags)
{
	const D2D1_POINT_2F dips = DPIScale::PixelsToDips(pixelX, pixelY);
	float x2 = dips.x;
	float y2 = dips.y;

	pageX = x2;
	pageY = y2;

	RedBlackTree* rbt = &(mySlabContainer->RBTSlabLines);
	RedBlackNode* node = rbt->closest(x2);
	Slab* slab = &nilSlab;
	bool success = false, slabExists = false;

	MainWindow::success = false;

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

void MainWindow::OnLButtonDown(int pixelX, int pixelY, DWORD flags) {
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
		controls[i]->focused = false;
	}

	for (int i = 0, len = selRegion->shapes.size(); i < len; i++)
	{
		selRegion->shapes[i]->control->focused = true;
		if (selRegion->shapes[i]->control->clickHandler != nullptr) {
			selRegion->shapes[i]->control->clickHandler();
		}
		if (selRegion->shapes[i]->control->type == FormComboBox) {
			ComboBoxInFocus = (ComboBox*)selRegion->shapes[i]->control;
		}
		if (selRegion->shapes[i]->control->type == FormTextbox) {
			TextboxInFocus = (Textbox*)selRegion->shapes[i]->control;
			int charIndex = 0;
			int x = TextboxInFocus->x + 10;
			while (charIndex < TextboxInFocus->value.size() && x < (TextboxInFocus->x + TextboxInFocus->width)) {
				x = TextboxInFocus->x + 10 + MeasureText(TextboxInFocus->value.substr(0, charIndex + 1)).widthIncludingTrailingWhitespace;
				charIndex++;
				if (x >= pixelX) {
					charIndex--;
					break;
				}
			}
			TextboxInFocus->charIndex = clamp(charIndex, 0, TextboxInFocus->value.size());
			OnPaint();
		}
	}
}

void MainWindow::OnLButtonUp() {
	for (int i = 0, len = selRegion->shapes.size(); i < len; i++)
	{
		selRegion->shapes[i]->control->toggled = !selRegion->shapes[i]->control->toggled;
	}
	OnPaint();
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

	case WM_LBUTTONUP:
		OnLButtonUp();
		return 0;

	case WM_SIZE:
		Resize();
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}