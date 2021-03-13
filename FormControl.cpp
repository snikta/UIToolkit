#pragma once
#include <string>
#include "FormControl.h"
#include "WICViewerD2D.h"
#include "SafeRelease.h"
#include "stringToLPCWSTR.h"

using std::string;

void FormControl::setTextFormat() {
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
void FormControl::setFontName(string newFontName) {
	fontName = newFontName;
	setTextFormat();
}
void FormControl::setFontSize(int newFontSize) {
	fontSize = newFontSize;
	setTextFormat();
}
void FormControl::setBold(bool boldValue) {
	bold = boldValue;
	setTextFormat();
}
void FormControl::setItalic(bool italicValue) {
	italic = italicValue;
	setTextFormat();
}
void FormControl::setForeColor(Color newForeColor) {
	foreColor = newForeColor;
	setTextFormat();
}
void FormControl::setBackColor(Color newBackColor) {
	backColor = newBackColor;
	setTextFormat();
}
std::string FormControl::getFontName() {
	return fontName;
}
int FormControl::getFontSize() {
	return fontSize;
}
bool FormControl::getBold() {
	return bold;
}
bool FormControl::getItalic() {
	return italic;
}
Color FormControl::getForeColor() {
	return foreColor;
}
Color FormControl::getBackColor() {
	return backColor;
}