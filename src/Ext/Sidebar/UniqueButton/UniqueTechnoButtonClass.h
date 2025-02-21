#pragma once
#include <ControlClass.h>

class UniqueTechnoButtonClass : public ControlClass
{
public:
	UniqueTechnoButtonClass() = default;
	UniqueTechnoButtonClass(unsigned int id, int x, int y, int width, int height);

	~UniqueTechnoButtonClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;
	virtual bool Action(GadgetFlag fags, DWORD* pKey, KeyModifier modifier) override;

	static constexpr int StartID = 2200;

	bool Hovering { false };
};
