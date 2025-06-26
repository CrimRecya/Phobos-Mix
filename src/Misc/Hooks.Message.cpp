#include <MessageListClass.h>
#include <WWMouseClass.h>

#include <Ext/Scenario/Body.h>

namespace MessageTemp
{
	bool OnOldMessages = false;
	bool OnNewMessages = false;
	bool NewMessageList = false;
}

static inline bool MouseIsOverOldMessageLists()
{
	const auto pMousePosition = &WWMouseClass::Instance->XY1;
	const auto pMessages = &MessageListClass::Instance;

	if (TextLabelClass* pText = pMessages->MessageList)
	{
		const int textHeight = pMessages->Height;
		int height = pMessages->MessagePos.Y;

		for ( ; pText; pText = static_cast<TextLabelClass*>(pText->GetNext()))
			height += textHeight;

		if (pMousePosition->Y < (height + 2))
			return true;
	}

	return false;
}

static inline bool MouseIsOverNewMessageLists()
{
	const auto pMousePosition = &WWMouseClass::Instance->XY1;

	if (const auto pMessages = ScenarioExt::Global()->NewMessageList.get())
	{
		if (TextLabelClass* pText = pMessages->MessageList)
		{
			if (pMousePosition->Y >= pMessages->MessagePos.Y
				&& pMousePosition->X >= pMessages->MessagePos.X
				&& pMousePosition->X <= pMessages->MessagePos.X + pMessages->Width)
			{
				const int textHeight = pMessages->Height;
				int height = pMessages->MessagePos.Y;

				for ( ; pText; pText = static_cast<TextLabelClass*>(pText->GetNext()))
					height += textHeight;

				if (pMousePosition->Y < (height + 2))
					return true;
			}
		}
	}

	return false;
}

DEFINE_HOOK(0x69300B, ScrollClass_MouseUpdate_NewMessageListManage, 0x6)
{
	if (Phobos::Config::MessageApplyHoverState)
		MessageTemp::OnOldMessages = MouseIsOverOldMessageLists();

	if (Phobos::Config::MessageDisplayInCenter)
		MessageTemp::OnNewMessages = MouseIsOverNewMessageLists();

	return 0;
}

DEFINE_HOOK(0x4F4583, GScreenClass_NewMessageListManage, 0x6)
{
	MessageTemp::NewMessageList = true;

	if (const auto pList = ScenarioExt::Global()->NewMessageList.get())
		pList->Draw();

	MessageTemp::NewMessageList = false;

	return 0;
}

DEFINE_HOOK(0x55DDA0, MainLoop_FrameStep_NewMessageListManage, 0x5)
{
	enum { SkipGameCode = 0x55DDAA };

	if (!Phobos::Config::MessageApplyHoverState || !MessageTemp::OnOldMessages)
		MessageListClass::Instance.Manage();

	if (!MessageTemp::OnNewMessages)
	{
		if (const auto pList = ScenarioExt::Global()->NewMessageList.get())
			pList->Manage();
	}

	return SkipGameCode;
}

DEFINE_HOOK(0x6DE11D, MessageListClass_AddMessage_InCenter, 0x5)
{
	enum { SkipGameCode = 0x6DE122 };

	if (const auto pList = ScenarioExt::Global()->NewMessageList.get())
		R->ECX(pList);
	else // !Phobos::Config::MessageDisplayInCenter
		R->ECX(&MessageListClass::Instance);

	return SkipGameCode;
}

DEFINE_HOOK(0x4A8BCE, DisplayClass_Set_View_Dimensions, 0x5)
{
	if (Phobos::Config::MessageDisplayInCenter)
	{
		const auto& pScenarioExt = ScenarioExt::Global();

		if (!pScenarioExt->NewMessageList) // Load game
			pScenarioExt->NewMessageList = std::make_unique<MessageListClass>();

		const auto& rect = DSurface::ViewBounds;
		const auto sideWidth = rect.Width / 6;
		const auto width = rect.Width - (sideWidth * 2);
		const auto pList = pScenarioExt->NewMessageList.get();

		// Except for X and Y, they are all original values
		pList->Init((rect.X + sideWidth), (rect.Height - rect.Height / 8 - 120), 6, 98, 18, -1, -1, 0, 20, 98, width);
		pList->SetWidth(width);
	}

	return 0;
}

DEFINE_HOOK(0x684AD3, UnknownClass_sub_684620_InitMessageList, 0x5)
{
	auto pMessageList = &MessageListClass::Instance;

	if (Phobos::Config::MessageDisplayInCenter)
	{
		const auto& pScenarioExt = ScenarioExt::Global();

		if (!pScenarioExt->NewMessageList) // Start game
			pScenarioExt->NewMessageList = std::make_unique<MessageListClass>();

		const auto& rect = DSurface::ViewBounds;
		const auto sideWidth = rect.Width / 6;
		const auto width = rect.Width - (sideWidth * 2);
		const auto pList = pScenarioExt->NewMessageList.get();

		// Except for X and Y, they are all original values
		pList->Init((rect.X + sideWidth), (rect.Height - rect.Height / 8 - 120), 6, 98, 18, -1, -1, 0, 20, 98, width);
		pMessageList = pList;
	}

	if (!Phobos::PoweredByEC && !Phobos::HideWarning)
	{
		pMessageList->PrintMessage(L"正在使用Phobos特别合并构建#" _STR(BUILD_NUMBER) L"+" _STR(MERGE_NUMBER) L"_" _STR(MERGE_PATCH) L"。若在使用过程中发生问题，请按说明中的方法反馈。  — 绯红热茶", 480);

		const time_t compileTime = Phobos::GetCompile();
		const time_t currentTime = Phobos::GetCurrent();
		const int daysUsed = static_cast<int>(difftime(currentTime, compileTime) / (60 * 60 * 24));
		const int daysLeft = 15 - daysUsed;
		wchar_t buffer[0x20];
		swprintf_s(buffer, L"剩余试用期：%2d天", daysLeft);

		pMessageList->PrintMessage(buffer, 480);
	}

	return 0;
}

DEFINE_HOOK(0x623A9F, DSurface_sub_623880_DrawBitFontStrings, 0x5)
{
	if (!MessageTemp::NewMessageList)
		return 0;

	enum { SkipGameCode = 0x623AAB };

	GET(RectangleStruct* const, pRect, EAX);
	GET(DSurface* const, pSurface, ECX);
	GET(const int, height, EBP);

	pRect->Height = height;
	auto black = ColorStruct { 0, 0, 0 };
	auto trans = (MessageTemp::OnNewMessages || ScenarioClass::Instance->UserInputLocked) ? 80 : 40;
	pSurface->FillRectTrans(pRect, &black, trans);

	return SkipGameCode;
}
