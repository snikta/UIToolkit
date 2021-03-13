#pragma once
#include <string>

enum FormControlType { FormComboBox, FormButton, FormTextbox, FormRadioButton, FormCheckbox };

class FormControl {
public:
	FormControlType type;
	std::string label;
	float x;
	float y;
	float width;
	float height;
	bool focused = false;
	bool toggled = false;
	bool hover = false;
	void (*clickHandler)() = nullptr;

	FormControl(std::string label, float x, float y) : label(label), x(x), y(y) {};
	virtual void Render() {};
};