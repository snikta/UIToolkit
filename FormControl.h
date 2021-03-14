#pragma once
#include <string>
#include "WICViewerD2D.h"

extern IDWriteFactory* m_pDWriteFactory;
enum FormControlType { FormLabel, FormComboBox, FormButton, FormTextbox, FormRadioButton, FormCheckbox };

class Color {
public:
	float red;
	float green;
	float blue;
	float alpha;

	Color(float red, float green, float blue, float alpha) : red(red), green(green), blue(blue), alpha(alpha) {};
};

class FormControl {
private:
	std::string fontName = "Arial";
	int fontSize = 16;
	bool bold = false;
	bool italic = false;
	Color foreColor = Color(0.0, 0.0, 0.0, 1.0);
	Color backColor = Color(0.0, 0.0, 0.0, 0.0);
	void setTextFormat();
public:
	IDWriteTextFormat* m_pTextFormat = nullptr;
	FormControlType type;
	std::string label;
	float x;
	float y;
	float width;
	float height;
	bool focused = false;
	bool toggled = false;
	bool hover = false;
	void (*clickHandler)(FormControl *control) = nullptr;
	void setFontName(std::string newFontName);
	void setFontSize(int newFontSize);
	void setBold(bool boldValue);
	void setItalic(bool italicValue);
	void setForeColor(Color newForeColor);
	void setBackColor(Color newBackColor);
	std::string getFontName();
	int getFontSize();
	bool getBold();
	bool getItalic();
	Color getForeColor();
	Color getBackColor();

	FormControl(std::string label, float x, float y) : label(label), x(x), y(y) {
		setTextFormat();
	};
	virtual void Render() {};
};