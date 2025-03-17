#pragma once
#include <ControlClass.h>

class SelectedCameoClass : public ControlClass
{
public:
	SelectedCameoClass() = default;
	SelectedCameoClass(unsigned int id, int x, int y);

	~SelectedCameoClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;
	virtual bool Action(GadgetFlag fags, DWORD* pKey, KeyModifier modifier) override;

	inline int GetButtonIndex() const;
	void DrawInfo() const;

	bool Hovering { false };
};

class SelectedMainCameoClass : public ControlClass
{
public:
	SelectedMainCameoClass() = default;
	SelectedMainCameoClass(unsigned int id, int x, int y);

	~SelectedMainCameoClass() = default;

	virtual bool Draw(bool forced) override;
	virtual void OnMouseEnter() override;
	virtual void OnMouseLeave() override;

	bool Hovering { false };
};
