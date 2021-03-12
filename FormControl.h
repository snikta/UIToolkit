#pragma once
#include <string>

class FormControl {
public:
	std::string label;
	float x;
	float y;
	float width;
	float height;
	bool pressed = false;
	bool hover = false;

	FormControl(std::string label, float x, float y) : label(label), x(x), y(y) {};
	virtual void Render() {};
};