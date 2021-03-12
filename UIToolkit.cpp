#include <windows.h>
#include <d2d1.h>
#include <iostream>
#include <string>
#pragma comment(lib, "d2d1")

#include "basewin.h"
#include "WICViewerD2D.h"
#include "LoadBitmapFromFile.h"
#include "SafeRelease.h"

using std::string;

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

DWRITE_TEXT_METRICS FillText(string text, float x, float y) {
	IDWriteTextLayout* pTextLayout_ = NULL;
	HRESULT hr = m_pDWriteFactory->CreateTextLayout(
		stringToLPCWSTR(text),      // The string to be laid out and formatted.
		text.size(),  // The length of the string.
		m_pTextFormat,  // The text format to apply to the string (contains font information, etc).
		1366,         // The width of the layout box.
		768,        // The height of the layout box.
		&pTextLayout_  // The IDWriteTextLayout interface pointer.
	);

	DWRITE_TEXT_METRICS metrics;
	pTextLayout_->GetMetrics(&metrics);

	pRenderTarget->DrawText(
		stringToLPCWSTR(text),
		text.size(),
		m_pTextFormat,
		D2D1::RectF(x, y, x + metrics.widthIncludingTrailingWhitespace, y + metrics.height),
		pFillBrush
	);

	SafeRelease(pTextLayout_);

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

class MainWindow : public BaseWindow<MainWindow>
{
	ID2D1Factory* pFactory;

	void    CalculateLayout();
	HRESULT CreateGraphicsResources();
	void    DiscardGraphicsResources();
	void    OnPaint();
	void    Resize();

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

void MainWindow::OnPaint()
{
	HRESULT hr = CreateGraphicsResources();
	if (SUCCEEDED(hr))
	{
		PAINTSTRUCT ps;
		BeginPaint(m_hwnd, &ps);

		pRenderTarget->BeginDraw();

		pRenderTarget->Clear(D2D1::ColorF(D2D1::ColorF::SkyBlue));

		hr = DemoApp::CreateDeviceIndependentResources();

		SetStrokeColor(0.0, 0.0, 0.0);

		SetFillColor(255.0, 0.0, 0.0);
		FillRect(10, 10, 100, 100);
		StrokeRect(10, 120, 100, 100);

		SetFillColor(255.0, 255.0, 0.0);
		FillEllipse(120, 10, 100, 100);
		StrokeEllipse(120, 120, 100, 100);

		DrawBitmap("logo.png", 10, 240, 160, 50);
		SetFillColor(0.0, 0.0, 0.0);
		SetFont("Arial", 16, true, false);
		FillText("Copyright 2021 Snikta. All rights reserved.", 10, 310);

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

	case WM_PAINT:
		OnPaint();
		return 0;



	case WM_SIZE:
		Resize();
		return 0;
	}
	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}