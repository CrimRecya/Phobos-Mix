#pragma once

#include "Panel.h"

#include "../UIRoot.h"

#include <string>

namespace UIExt
{
	// A panel with a title bar. Modal behaviour is configured when opening via UIRoot.
	class Dialog : public Panel
	{
	public:
		Dialog();
		Dialog(int x, int y, int width, int height, std::wstring title);

		Dialog& SetTitle(std::wstring title);

		bool Draw(bool forced) override;

	protected:
		std::wstring Title { };
	};
}