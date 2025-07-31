#pragma once
#include <GadgetClass.h>

class SelectedColumnClass : public GadgetClass
{
public:
	SelectedColumnClass() = default;
	SelectedColumnClass(int x, int y, int width, int height);

	~SelectedColumnClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;

	void DrawInfo() const;
};

class SelectedBottomClass : public GadgetClass
{
public:
	SelectedBottomClass() = default;
	SelectedBottomClass(int x, int y, int width, int height);

	~SelectedBottomClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;

	void DrawInfo() const;
};
