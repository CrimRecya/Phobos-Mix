#include "UniqueTechnoColumnClass.h"

UniqueTechnoColumnClass UniqueTechnoColumnClass::Instance;

void UniqueTechnoColumnClass::InitClear()
{
	for (int i = 0; i < 8; ++i)
	{
		if (auto& pButton = this->Buttons[i])
		{
			GScreenClass::Instance.RemoveButton(pButton);
			GameDelete(pButton);
			pButton = nullptr;
		}
	}

	this->Hovering = -1;
}

void UniqueTechnoColumnClass::InitIO()
{
	if (Unsorted::ArmageddonMode)
		return;

	Point2D position { DSurface::Composite->GetWidth() - 65, 35 };

	for (int i = 0; i < 8; ++i)
	{
		const auto pButton = GameCreate<UniqueTechnoButtonClass>(i, position.X, position.Y);
		position.Y += 50;

		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->Buttons[i] = pButton;
	}
}

void UniqueTechnoColumnClass::SwitchVisible()
{
	this->Visible = !this->Visible;
	VocClass::PlayGlobal(RulesClass::Instance->GUIMainButtonSound, 0x2000, 1.0);

	for (int i = 0; i < 8; ++i)
	{
		if (const auto& pButton = this->Buttons[i])
			pButton->Disabled = !this->Visible;
	}
}
