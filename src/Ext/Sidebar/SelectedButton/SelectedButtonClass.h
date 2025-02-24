#pragma once
#include <ControlClass.h>

class SelectedButtonClass : public ControlClass
{
public:
	SelectedButtonClass() = default;
	SelectedButtonClass(unsigned int id, int x, int y);

	~SelectedButtonClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;
	virtual bool Action(GadgetFlag fags, DWORD* pKey, KeyModifier modifier) override;

	void DrawInfo() const;

	bool Hovering { false };
};

class SelectedNotButtonClass : public ControlClass
{
public:
	SelectedNotButtonClass() = default;
	SelectedNotButtonClass(unsigned int id, int x, int y);

	~SelectedNotButtonClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;

	void DrawInfo() const;

	bool Hovering { false };
};
