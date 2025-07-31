#include "MessageColumn.h"

#include <BitFont.h>
#include <MessageListClass.h>
#include <MouseClass.h>
#include <WWMouseClass.h>

MessageColumnClass MessageColumnClass::Instance;

// --------------------------------------------------

MessageBoardClass::MessageBoardClass(int x, int y, int width, int height)
	: GadgetClass(x, y, width, height, GadgetFlag::LeftPress | GadgetFlag::LeftHeld | GadgetFlag::LeftRelease, true)
{
	this->Disabled = true;
}

bool MessageBoardClass::Draw(bool forced)
{
	return false;
}

void MessageBoardClass::OnMouseEnter()
{
	this->Hovering = true;
	MessageColumnClass::Instance.MouseEnter();
}

void MessageBoardClass::OnMouseLeave()
{
	this->Hovering = false;
	MessageColumnClass::Instance.MouseLeave();
}

bool MessageBoardClass::Clicked(DWORD* pKey, GadgetFlag flags, int x, int y, KeyModifier modifier)
{
	if (!MessageColumnClass::IsStickyButton(this))
	{
		if (!(flags & GadgetFlag::LeftPress) || !MessageColumnClass::Instance.IsExpanded() || !this->Hovering)
			return false;

		this->LastY = y;
		this->LastScroll = MessageColumnClass::Instance.GetScroll();
	}
	else if (flags & GadgetFlag::LeftHeld)
	{
		const int indexOffset = (this->LastY - y) / MessageToggleClass::ButtonSide;
		MessageColumnClass::Instance.SetScroll(this->LastScroll + indexOffset);
	}

	return this->Action(flags, pKey, modifier);
}

void MessageBoardClass::DrawShape() const
{
	if ((MessageColumnClass::Instance.IsHovering() && Phobos::Config::MessageApplyHoverState)
		|| MessageColumnClass::Instance.IsExpanded()
		|| ScenarioClass::Instance->UserInputLocked)
	{
		RectangleStruct drawRect { this->X, this->Y, this->Width, this->Height };
		ColorStruct color { 0, 0, 0 };
		DSurface::Composite->FillRectTrans(&drawRect, &color, MessageColumnClass::LowOpacity);
	}
}

// --------------------------------------------------

MessageToggleClass::MessageToggleClass(int id, int x, int y, int width, int height)
	: GadgetClass(x, y, width, height, GadgetFlag::LeftPress, false)
	, ID(id)
{
	this->Disabled = true;
}

bool MessageToggleClass::Draw(bool forced)
{
	return false;
}

void MessageToggleClass::OnMouseEnter()
{
	this->Hovering = true;
	MessageColumnClass::Instance.MouseEnter(true);
}

void MessageToggleClass::OnMouseLeave()
{
	this->Hovering = false;
	MessageColumnClass::Instance.MouseLeave(true);
}

bool MessageToggleClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (flags & GadgetFlag::LeftPress)
	{
		if (this->ID) // Button_Clear
			MessageColumnClass::Instance.PackUp(true);
		else // Button_Expand
			MessageColumnClass::Instance.Toggle();
	}

	this->GadgetClass::Action(flags, pKey, KeyModifier::None);
	return true;
}

void MessageToggleClass::DrawShape() const
{
	if (this->Disabled)
		return;

	RectangleStruct drawRect { this->X, this->Y, this->Width, this->Height };
	ColorStruct color { 255, 255, 255 };

	if (this->ID) // Button_Clear
	{
		constexpr int offset = 4;
		constexpr int iconSide = MessageToggleClass::ButtonSide - (offset * 2);

		DSurface::Composite->FillRectTrans(&drawRect, &color, this->Hovering ? MessageColumnClass::MediumOpacity : MessageColumnClass::LowOpacity);

		color.G = 0;
		color.B = 0;
		drawRect.X += offset;
		drawRect.Y += offset;
		drawRect.Width = iconSide;
		drawRect.Height = iconSide;

		DSurface::Composite->FillRectTrans(&drawRect, &color, this->Hovering ? MessageColumnClass::HighOpacity : MessageColumnClass::MediumOpacity);
	}
	else // Button_Expand
	{
		DSurface::Composite->FillRectTrans(&drawRect, &color, this->Hovering ? MessageColumnClass::MediumOpacity : MessageColumnClass::LowOpacity);

		color.R = 0;
		color.G = 0;
		drawRect.X += 1;
		drawRect.Y += (MessageToggleClass::ButtonSide - MessageToggleClass::ButtonIconWidth) / 2;
		drawRect.Width = MessageToggleClass::ButtonSide - 2;
		drawRect.Height = MessageToggleClass::ButtonIconWidth;
		const int opacity = this->Hovering ? MessageColumnClass::HighOpacity : MessageColumnClass::MediumOpacity;

		DSurface::Composite->FillRectTrans(&drawRect, &color, opacity);

		if (!MessageColumnClass::Instance.IsExpanded())
		{
			drawRect.X = this->X + ((MessageToggleClass::ButtonSide - MessageToggleClass::ButtonIconWidth) / 2);
			drawRect.Y = this->Y + 1;
			drawRect.Width = MessageToggleClass::ButtonIconWidth;
			drawRect.Height = (MessageToggleClass::ButtonSide - MessageToggleClass::ButtonIconWidth) / 2 - 1;

			DSurface::Composite->FillRectTrans(&drawRect, &color, opacity);

			drawRect.Y = this->Y + (((MessageToggleClass::ButtonSide - MessageToggleClass::ButtonIconWidth) / 2) + MessageToggleClass::ButtonIconWidth);

			DSurface::Composite->FillRectTrans(&drawRect, &color, opacity);
		}
	}
}

// --------------------------------------------------

MessageButtonClass::MessageButtonClass(int id, int x, int y, int width, int height)
	: MessageToggleClass(id, x, y, width, height)
{
	this->IsSticky = true;
	this->Disabled = true;
	this->Flags |= GadgetFlag::LeftHeld | GadgetFlag::LeftRelease;
}

bool MessageButtonClass::Action(GadgetFlag flags, DWORD* pKey, KeyModifier modifier)
{
	if (this->ID) // Button_Down
	{
		if (!MessageColumnClass::IsStickyButton(this))
		{
			if ((flags & GadgetFlag::LeftPress) && this->Hovering)
			{
				this->CheckTime = MessageColumnClass::GetSystemTime() + 60;
				MessageColumnClass::Instance.ScrollDown();
			}
		}
		else if (flags & GadgetFlag::LeftHeld)
		{
			const int timeExpired = MessageColumnClass::GetSystemTime() - this->CheckTime;

			if (timeExpired > 0)
			{
				this->CheckTime += 30;
				MessageColumnClass::Instance.ScrollDown();
			}
		}
	}
	else // Button_Up
	{
		if (!MessageColumnClass::IsStickyButton(this))
		{
			if ((flags & GadgetFlag::LeftPress) && this->Hovering)
			{
				this->CheckTime = MessageColumnClass::GetSystemTime() + 60;
				MessageColumnClass::Instance.ScrollUp();
			}
		}
		else if (flags & GadgetFlag::LeftHeld)
		{
			const int timeExpired = MessageColumnClass::GetSystemTime() - this->CheckTime;

			if (timeExpired > 0)
			{
				this->CheckTime += 30;
				MessageColumnClass::Instance.ScrollUp();
			}
		}
	}

	this->GadgetClass::Action(flags, pKey, KeyModifier::None);
	return true;
}

void MessageButtonClass::DrawShape() const
{
	if (this->Disabled)
		return;

	RectangleStruct drawRect { this->X, this->Y, this->Width, this->Height };
	ColorStruct color { 255, 255, 255 };

	if (this->ID) // Button_Down
	{
		const bool can = MessageColumnClass::Instance.CanScrollDown();
		const bool ntr = can && this->Hovering;

		DSurface::Composite->FillRectTrans(&drawRect, &color, ntr ? MessageColumnClass::MediumOpacity : MessageColumnClass::LowOpacity);

		color = ColorStruct { 0, static_cast<BYTE>(can ? 255 : 0), 0 };
		drawRect.X += 5;
		drawRect.Y += 1;
		drawRect.Width -= 10;
		drawRect.Height = MessageToggleClass::ButtonIconWidth;

		DSurface::Composite->FillRectTrans(&drawRect, &color, ntr ? MessageColumnClass::HighOpacity : MessageColumnClass::MediumOpacity);
	}
	else // Button_Up
	{
		const bool can = MessageColumnClass::Instance.CanScrollUp();
		const bool ntr = can && this->Hovering;

		DSurface::Composite->FillRectTrans(&drawRect, &color, ntr ? MessageColumnClass::MediumOpacity : MessageColumnClass::LowOpacity);

		color = ColorStruct { 0, static_cast<BYTE>(can ? 255 : 0), 0 };
		drawRect.X += 5;
		drawRect.Y += 1;
		drawRect.Width -= 10;
		drawRect.Height = MessageToggleClass::ButtonIconWidth;

		DSurface::Composite->FillRectTrans(&drawRect, &color, ntr ? MessageColumnClass::HighOpacity : MessageColumnClass::MediumOpacity);
	}
}

// --------------------------------------------------

MessageLabelClass::MessageLabelClass(int x, int y, size_t id, int deleteTime, bool animate, int drawDelay)
	: GadgetClass(x, y, 1, 1, GadgetFlag(0), false)
	, ID(id)
	, DeleteTime(deleteTime)
	, Animate(animate)
	, DrawDelay(drawDelay)
{}

bool MessageLabelClass::Draw(bool forced)
{
	if (this->DrawDelay > MessageColumnClass::GetSystemTime())
		return false;

	if (!GadgetClass::Draw(forced))
		return false;

	if (!ColorScheme::Array.Count)
		return false;

	const auto pBit = BitFont::Instance;
	const int surfaceWidth = DSurface::Temp->GetWidth();
	RectangleStruct rect { this->X, this->Y, MessageColumnClass::Instance.GetWidth(), pBit->field_1C };
	const int remainWidth = surfaceWidth - rect.X;

	if (rect.Width > remainWidth)
	{
		rect.Width = remainWidth;

		if (rect.Width <= 0 || rect.Height <= 0)
			return false;
	}

	const wchar_t* text = this->GetText();
	size_t textLen = wcslen(text);

	if (this->Animate)
	{
#pragma warning(suppress: 4996)
		const auto time = Imports::TimeGetTime()();
		const size_t animPos = this->AnimPos;
		this->AnimPos = animPos ? (animPos + ((time - this->AnimTiming) >> 4u)) : 1u;

		if (this->AnimPos != animPos)
			this->AnimTiming = time;

		if (this->AnimPos <= textLen)
			VocClass::PlayGlobal(RulesClass::Instance->MessageCharTyped, 0x2000, 1.0f);
	}

	reinterpret_cast<void(__fastcall*)(DSurface*, RectangleStruct*, const wchar_t*, size_t, BitFont*, uint32_t, size_t*, bool, bool, bool, size_t)>(0x623880)
		(DSurface::Temp, &rect, text, textLen, pBit, MessageColumnClass::Instance.GetColor(), &this->DrawPos, this->IsFocused(), false, true, this->AnimPos);

	return true;
}

// --------------------------------------------------

MessageColumnClass::~MessageColumnClass()
{
	this->Initialize();
}

void MessageColumnClass::InitClear()
{
	this->Initialize();

	if (this->Button_Expand)
	{
		GScreenClass::Instance.RemoveButton(this->Button_Expand);
		GameDelete(this->Button_Expand);
		this->Button_Expand = nullptr;
	}

	if (this->Button_Clear)
	{
		GScreenClass::Instance.RemoveButton(this->Button_Clear);
		GameDelete(this->Button_Clear);
		this->Button_Clear = nullptr;
	}

	if (this->Button_Up)
	{
		GScreenClass::Instance.RemoveButton(this->Button_Up);
		GameDelete(this->Button_Up);
		this->Button_Up = nullptr;
	}

	if (this->Button_Down)
	{
		GScreenClass::Instance.RemoveButton(this->Button_Down);
		GameDelete(this->Button_Down);
		this->Button_Down = nullptr;
	}

	if (this->Board)
	{
		GScreenClass::Instance.RemoveButton(this->Board);
		GameDelete(this->Board);
		this->Board = nullptr;
	}
}

void MessageColumnClass::InitIO()
{
	if (Unsorted::ArmageddonMode || !Phobos::Config::MessageDisplayInCenter)
		return;

	const auto& rect = DSurface::ViewBounds;
	const int sideWidth = rect.Width / 6;
	int width = rect.Width - (sideWidth * 2);
	int posX = rect.X + sideWidth;
	int posY = rect.Height - rect.Height / 8;
	const int maxLines = posY / MessageToggleClass::ButtonSide - 1;
	constexpr int maxChars = 112;
	const int maxRecord = Math::clamp(Phobos::Config::MessageDisplayInCenter_RecordsCount, 3, maxLines);
	const int maxCount = Math::clamp(Phobos::Config::MessageDisplayInCenter_LabelsCount, 1, maxLines);

	this->Initialize(posX, posY, maxCount, maxRecord, maxChars, width);

	posX -= 1;
	width += 2;

	// Button_Expand
	{
		const int locX = posX + width - ((MessageToggleClass::ButtonSide * 2) + 2);
		const int locY = posY - MessageToggleClass::ButtonSide;
		const auto pButton = GameCreate<MessageToggleClass>(0, locX, locY, MessageToggleClass::ButtonSide, MessageToggleClass::ButtonSide);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->Button_Expand = pButton;
	}

	// Button_Clear
	{
		const int locX = posX + width - MessageToggleClass::ButtonSide;
		const int locY = posY - MessageToggleClass::ButtonSide;
		const auto pButton = GameCreate<MessageToggleClass>(1, locX, locY, MessageToggleClass::ButtonSide, MessageToggleClass::ButtonSide);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->Button_Clear = pButton;
	}

	// Button_Up
	{
		const int locX = rect.Width * 5 / 12;
		const int locY = posY - (MessageToggleClass::ButtonSide * 12) - 1 - MessageToggleClass::ButtonHeight;
		const auto pButton = GameCreate<MessageButtonClass>(0, locX, locY, sideWidth, MessageToggleClass::ButtonHeight);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->Button_Up = pButton;
	}

	// Button_Down
	{
		const int locX = rect.Width * 5 / 12;
		const int locY = posY;
		const auto pButton = GameCreate<MessageButtonClass>(1, locX, locY, sideWidth, MessageToggleClass::ButtonHeight);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->Button_Down = pButton;
	}

	// Board
	{
		const auto pButton = GameCreate<MessageBoardClass>(posX, posY, width, 1);
		pButton->Zap();
		GScreenClass::Instance.AddButton(pButton);
		this->Board = pButton;
	}

	// 0x6DE0A8
	const int sideIndex = ScenarioClass::Instance->PlayerSideIndex;
	const int player = sideIndex ? (sideIndex != 1 ? 5 : 1) : 2;
	int color = reinterpret_cast<int(__stdcall*)(size_t)>(0x69A310)(player);

	// 0x72A4C5
	if (color < 0 || color >= ColorScheme::Array.Count)
		color = 0;

	if (const auto pScheme = ColorScheme::Array.Items[color])
	{
		ColorStruct adjustedColor;
		reinterpret_cast<ColorStruct*(__thiscall*)(ColorStruct*, ColorStruct*)>(0x517440)(&pScheme->BaseColor, &adjustedColor);
		this->Tint = Drawing::RGB_To_Int(adjustedColor);
		this->Color = (adjustedColor.B << 16) | (adjustedColor.G << 8) | adjustedColor.R;
	}

	this->Update();
}

void MessageColumnClass::Initialize(int x, int y, int maxCount, int maxRecord, int maxChars, int width)
{
	this->LabelList = nullptr;
	this->LabelsPos = Point2D { x, y };
	this->MaxCount = maxCount;
	this->MaxRecord = maxRecord;
	this->MaxChars = maxChars;
	this->Height = MessageToggleClass::ButtonSide;
	this->Width = width - MessageColumnClass::TextReservedSpace;
	this->Color = 0;
	this->Tint = 0;
	this->PackUp(true);
	this->Hovering = false;
	this->Drawing = false;
	this->Blocked = false;
}

MessageLabelClass* MessageColumnClass::AddMessage(const wchar_t* name, const wchar_t* message, int timeout, bool silent, int delay)
{
	if (!message)
		return nullptr;

	const auto pBit = BitFont::Instance;

	if (!pBit)
		return nullptr;

	std::wstring buffer;

	if (name)
		buffer = std::wstring(name) + L":";

	int prefixWidth = 0;
	pBit->GetTextDimension(buffer.c_str(), &prefixWidth, nullptr, 0);
	const int availableWidth = this->Width - prefixWidth - MessageColumnClass::TextReservedSpace;

	if (availableWidth <= 0)
		return nullptr;

	const int messageLen = static_cast<int>(wcslen(message));
	const int charsToCopy = reinterpret_cast<int(__thiscall*)(BitFont*, const wchar_t*, int, int, int)>(0x433F50)(pBit, message, availableWidth, 111, 1);

	if (charsToCopy < 0)
		return nullptr;

	buffer.append(message, charsToCopy);

	if (this->MaxCount > 0 && (this->GetLabelCount() + 1) > this->MaxCount)
	{
		if (auto pLabel = this->LabelList)
			this->RemoveTextLabel(pLabel);
		else
			return nullptr;
	}

	const size_t newID = ScenarioExt::Global()->RecordMessages.size();

	if (!this->AddRecordString(buffer))
		return nullptr;

	const int currentTime = MessageColumnClass::GetSystemTime();

	if (!silent)
		VocClass::PlayGlobal(RulesClass::Instance->IncomingMessage, 0x2000, 1.0f);

	const auto pLabel = new MessageLabelClass
		(
			this->LabelsPos.X,
			this->LabelsPos.Y,
			newID,
			(timeout == -1) ? 0 : (timeout + currentTime),
			!silent,
			delay + currentTime
		);

	if (this->LabelList)
		pLabel->AddTail(*this->LabelList);
	else
		this->LabelList = pLabel;

	this->Update();

	if (charsToCopy < messageLen)
	{
		const wchar_t* remainingText = &message[charsToCopy];

		while (*remainingText && *remainingText < 0x20)
			++remainingText;

		if (*remainingText)
		{
			int nextDelay = delay;

			if (!silent)
				nextDelay += (charsToCopy * 2 - 1);

			this->AddMessage(name, remainingText, timeout, silent, nextDelay);
		}
	}

	return pLabel;
}

void MessageColumnClass::MouseEnter(bool block)
{
	this->Hovering = true;

	if (block)
		this->Blocked = true;

	if (const auto pButton = this->Button_Expand)
		pButton->Disabled = false;

	if (const auto pButton = this->Button_Clear)
		pButton->Disabled = false;

	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

void MessageColumnClass::MouseLeave(bool block)
{
	this->Hovering = false;

	if (block)
		this->Blocked = false;

	if (!this->IsExpanded())
	{
		if (const auto pButton = this->Button_Expand)
			pButton->Disabled = true;

		if (const auto pButton = this->Button_Clear)
			pButton->Disabled = true;
	}

	MouseClass::Instance.UpdateCursor(MouseCursorType::Default, false);
}

bool MessageColumnClass::CanScrollUp()
{
	return this->IsExpanded() && this->ScrollIndex > 0;
}

bool MessageColumnClass::CanScrollDown()
{
	return this->IsExpanded() && this->ScrollIndex < this->GetMaxScroll();
}

void MessageColumnClass::ScrollUp()
{
	if (this->CanScrollUp())
		--this->ScrollIndex;
}

void MessageColumnClass::ScrollDown()
{
	if (this->CanScrollDown())
		++this->ScrollIndex;
}

void MessageColumnClass::SetScroll(int index)
{
	this->ScrollIndex = Math::clamp(index, 0, this->GetMaxScroll());
}

void MessageColumnClass::Expand()
{
	this->Expanded = true;
	this->ScrollIndex = this->GetMaxScroll();

	if (const auto pButton = this->Button_Expand)
		pButton->Disabled = false;

	if (const auto pButton = this->Button_Clear)
		pButton->Disabled = false;

	if (const auto pButton = this->Button_Up)
		pButton->Disabled = false;

	if (const auto pButton = this->Button_Down)
		pButton->Disabled = false;

	this->Refresh();
}

void MessageColumnClass::PackUp(bool clear)
{
	this->Expanded = false;
	this->ScrollIndex = this->GetMaxScroll();

	if (const auto pButton = this->Button_Up)
		pButton->Disabled = true;

	if (const auto pButton = this->Button_Down)
		pButton->Disabled = true;

	if (!clear)
	{
		if (auto pLabel = this->GetLastLabel())
		{
			const int currentTime = MessageColumnClass::GetSystemTime();

			for ( ; pLabel; pLabel = static_cast<MessageLabelClass*>(pLabel->GetPrev()))
			{
				if (pLabel->DrawDelay > currentTime)
					pLabel->DrawDelay = currentTime;

				if (pLabel->Animate)
				{
					pLabel->Animate = false;
					pLabel->AnimPos = 0;
				}
			}

			this->Refresh();

			return;
		}
	}

	this->CleanUp();
}

void MessageColumnClass::CleanUp()
{
	for (auto pLabel = this->LabelList; pLabel; pLabel = this->LabelList)
	{
		this->LabelList = static_cast<MessageLabelClass*>(pLabel->Remove());
		delete pLabel;
	}

	if (const auto pButton = this->Board)
	{
		pButton->Y = this->LabelsPos.Y - 1;
		pButton->Height = 1;
		pButton->Disabled = true;
	}

	if (const auto pButton = this->Button_Expand)
		pButton->Disabled = true;

	if (const auto pButton = this->Button_Clear)
		pButton->Disabled = true;
}

void MessageColumnClass::Refresh()
{
	if (const auto pButton = this->Board)
	{
		const int count = this->IsExpanded() ? this->MaxRecord : this->GetLabelCount();
		const int height = this->Height * count;
		pButton->Height = height + 1;
		pButton->Y = this->LabelsPos.Y - pButton->Height;
		pButton->Disabled = height == 0;
	}

	if (!this->IsHovering() && !this->IsExpanded())
	{
		if (const auto pButton = this->Button_Expand)
			pButton->Disabled = true;

		if (const auto pButton = this->Button_Clear)
			pButton->Disabled = true;
	}
}

void MessageColumnClass::Update()
{
	int y = this->LabelsPos.Y;

	for (auto pLabel = this->GetLastLabel(); pLabel; pLabel = static_cast<MessageLabelClass*>(pLabel->GetPrev()))
	{
		y -= this->Height;
		pLabel->Y = y;
	}

	this->Refresh();
}

void MessageColumnClass::Toggle()
{
	if (this->IsExpanded())
		this->PackUp();
	else
		this->Expand();
}

void MessageColumnClass::Manage()
{
	const int currentTime = MessageColumnClass::GetSystemTime();
	bool changed = false;

	for (auto pLabel = this->LabelList; pLabel; )
	{
		if (pLabel->DeleteTime && currentTime > pLabel->DeleteTime)
		{
			const auto pNextLabel = static_cast<MessageLabelClass*>(pLabel->GetNext());
			this->RemoveTextLabel(pLabel);
			pLabel = pNextLabel;
			changed = true;

			continue;
		}

		pLabel = static_cast<MessageLabelClass*>(pLabel->GetNext());
	}

	if (changed)
		this->Update();
}

void MessageColumnClass::DrawAll()
{
	if (const auto pButton = this->Button_Expand)
		pButton->DrawShape();

	if (const auto pButton = this->Button_Clear)
		pButton->DrawShape();

	if (const auto pButton = this->Button_Up)
		pButton->DrawShape();

	if (const auto pButton = this->Button_Down)
		pButton->DrawShape();

	if (const auto pButton = this->Board)
		pButton->DrawShape();

	if (this->IsExpanded())
	{
		const auto& messages = ScenarioExt::Global()->RecordMessages;

		if (messages.empty())
			return;

		int startY = this->LabelsPos.Y;
		const int maxIndex = static_cast<int>(messages.size()) - 1;
		const int startIndex = Math::min(this->ScrollIndex + (this->MaxRecord - 1), maxIndex);
		const int endIndex = Math::max(0, startIndex - this->MaxRecord + 1);

		for (int i = startIndex; i >= endIndex; --i)
		{
			startY -= this->Height;
			Point2D textLocation { this->LabelsPos.X, startY };
			RectangleStruct drawRect { 0, 0, textLocation.X + this->Width, textLocation.Y + this->Height };
			DSurface::Composite->DrawTextA(messages[i].c_str(), &drawRect, &textLocation, this->Tint, 0, (TextPrintType::FullShadow | TextPrintType::Point8));
		}
	}
	else if (const auto pLabel = this->LabelList)
	{
		this->Drawing = true;
		pLabel->DrawAll(true);
		this->Drawing = false;
	}
}

inline int MessageColumnClass::GetSystemTime()
{
	const auto& sysTimer = Make_Global<SysTimerClass>(0x887338);
	int currentTime = sysTimer.TimeLeft;

	if (sysTimer.StartTime != -1)
		currentTime += SystemTimer::GetTime() - sysTimer.StartTime;

	return currentTime;
}

inline bool MessageColumnClass::IsStickyButton(GadgetClass* pButton)
{
	return pButton == Make_Global<GadgetClass*>(0x8B3E88);
}

inline bool MessageColumnClass::AddRecordString(const std::wstring& message, size_t copySize)
{
	if (message.empty())
		return false;

	ScenarioExt::Global()->RecordMessages.push_back(message.substr(0, Math::min(copySize, message.size())));
	return true;
}

inline void MessageColumnClass::RemoveTextLabel(MessageLabelClass* pLabel)
{
	this->LabelList = static_cast<MessageLabelClass*>(pLabel->Remove());
	delete pLabel;
}

inline int MessageColumnClass::GetLabelCount() const
{
	int num = 0;

	for (auto pLabel = this->LabelList; pLabel; pLabel = static_cast<MessageLabelClass*>(pLabel->GetNext()))
		++num;

	return num;
}

inline MessageLabelClass* MessageColumnClass::GetLastLabel() const
{
	auto pLabel = this->LabelList;

	if (!pLabel)
		return nullptr;

	auto pNextLabel = static_cast<MessageLabelClass*>(pLabel->GetNext());

	for ( ; pNextLabel; pNextLabel = static_cast<MessageLabelClass*>(pLabel->GetNext()))
		pLabel = pNextLabel;

	return pLabel;
}

int MessageColumnClass::GetMaxScroll() const
{
	return Math::max(0, static_cast<int>(ScenarioExt::Global()->RecordMessages.size()) - this->MaxRecord);
}

// --------------------------------------------------

DEFINE_HOOK(0x4AAC92, TacticalGadgetClass_Action_ResetMessageColumnStatus, 0x5)
{
	GET_STACK(const GadgetFlag, flags, STACK_OFFSET(0x30, 0x4));

	if ((flags & (GadgetFlag::LeftPress | GadgetFlag::RightPress)) && !MessageColumnClass::Instance.IsHovering())
		MessageColumnClass::Instance.PackUp();

	return 0;
}

namespace MessageTemp
{
	bool OnOldMessages = false;
}

static inline bool MouseOverMessageLists()
{
	const auto& position = WWMouseClass::Instance->XY1;
	const auto& messages = MessageListClass::Instance;

	if (TextLabelClass* pText = messages.MessageList)
	{
		const int textHeight = messages.Height;
		int height = messages.MessagePos.Y;

		for ( ; pText; pText = static_cast<TextLabelClass*>(pText->GetNext()))
			height += textHeight;

		if (position.Y < (height + 2))
			return true;
	}

	return false;
}

DEFINE_HOOK(0x4F43BE, GScreenClass_GetInputAndUpdate_CheckHoverState, 0x7)
{
	if (Phobos::Config::MessageApplyHoverState)
		MessageTemp::OnOldMessages = MouseOverMessageLists();

	return 0;
}

DEFINE_HOOK(0x4F4583, GScreenClass_NewMessageListDraw, 0x6)
{
	MessageColumnClass::Instance.DrawAll();

	return 0;
}

DEFINE_HOOK(0x55DDA0, MainLoop_FrameStep_NewMessageListManage, 0x5)
{
	enum { SkipGameCode = 0x55DDAA };

	if (!MessageTemp::OnOldMessages)
		MessageListClass::Instance.Manage();

	if (!MessageColumnClass::Instance.IsExpanded() && (!MessageColumnClass::Instance.IsHovering() || !Phobos::Config::MessageApplyHoverState))
		MessageColumnClass::Instance.Manage();

	return SkipGameCode;
}

void __fastcall AddTActionMessage(MessageListClass* pThis, void* _, const wchar_t* name, int id, const wchar_t* message, int color, TextPrintType style, int timeout, bool silent)
{
	if (Phobos::Config::MessageDisplayInCenter)
		MessageColumnClass::Instance.AddMessage(name, message, timeout, silent);
	else
		pThis->AddMessage(name, id, message, color, style, timeout, silent);
}
DEFINE_FUNCTION_JUMP(CALL, 0x6DE122, AddTActionMessage)

DEFINE_HOOK(0x623A9F, DSurface_sub_623880_DrawBitFontStrings, 0x5)
{
	if (!MessageColumnClass::Instance.IsDrawing())
		return 0;

	enum { SkipGameCode = 0x623AAB };

	GET(RectangleStruct* const, pRect, EAX);
	GET(DSurface* const, pSurface, ECX);
	GET(const int, height, EBP);

	pRect->Height = height;

	if ((!MessageColumnClass::Instance.IsHovering() || !Phobos::Config::MessageApplyHoverState)
		&& !MessageColumnClass::Instance.IsExpanded()
		&& !ScenarioClass::Instance->UserInputLocked)
	{
		auto black = ColorStruct { 0, 0, 0 };
		pSurface->FillRectTrans(pRect, &black, MessageColumnClass::LowOpacity);
	}

	return SkipGameCode;
}
