#pragma once
#include <ControlClass.h>

class SelectedColumnClass : public ControlClass
{
public:
	SelectedColumnClass() = default;
	SelectedColumnClass(unsigned int id, int x, int y, int width, int height);

	~SelectedColumnClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;

	void DrawInfo() const;
};

class SelectedBottomClass : public ControlClass
{
public:
	SelectedBottomClass() = default;
	SelectedBottomClass(unsigned int id, int x, int y, int width, int height);

	~SelectedBottomClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;

	void DrawInfo() const;
};
