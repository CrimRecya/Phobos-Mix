#include "UniqueTechnoColumnClass.h"

UniqueTechnoColumnClass UniqueTechnoColumnClass::Instance;

void UniqueTechnoColumnClass::InitClear()
{
	for (int i = 0; i < 8; ++i)
	{
		if (auto& pButton = this->Buttons[i])
		{
			GScreenClass::Instance->RemoveButton(pButton);
			pButton = nullptr;
		}
	}
}

void UniqueTechnoColumnClass::InitIO()
{
	if (Unsorted::ArmageddonMode)
		return;

	Point2D position { DSurface::Composite->GetWidth() - 65, 35 };

	for (int i = 0; i < 8; ++i)
	{
		const auto pButton = GameCreate<UniqueTechnoButtonClass>(UniqueTechnoButtonClass::StartID + i, position.X, position.Y, 60, 48);
		position.Y += 50;

		pButton->Zap();
		GScreenClass::Instance->AddButton(pButton);
		this->Buttons[i] = pButton;
	}
}

void UniqueTechnoColumnClass::SwitchVisible()
{
	this->Visible = !this->Visible;

	for (int i = 0; i < 8; ++i)
	{
		if (auto& pButton = this->Buttons[i])
			pButton->Disabled = !this->Visible;
	}
}
