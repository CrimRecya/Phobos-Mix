#include <ColorScheme.h>
#include <GameOptionsClass.h>
#include <FPSCounter.h>
#include <SessionClass.h>
#include <Phobos.h>
#include <BitFont.h>
#include <InputManagerClass.h>

#include <Ext/Rules/Body.h>
#include <Utilities/Macro.h>

namespace TimerValueTemp
{
	static int oldValue;
};

DEFINE_HOOK(0x6D4B50, PrintTimerOnTactical_Start, 0x6)
{
	if (!Phobos::Config::RealTimeTimers)
		return 0;

	REF_STACK(int, value, STACK_OFFSET(0, 0x4));
	TimerValueTemp::oldValue = value;

	if (Phobos::Config::RealTimeTimers_Adaptive
		|| GameOptionsClass::Instance->GameSpeed == 0
		|| (Phobos::Misc::CustomGS && !SessionClass::IsMultiplayer()))
	{
		value = (int)((double)value / (std::max((double)FPSCounter::CurrentFrameRate, 1.0) / 15.0));
		return 0;
	}

	switch (GameOptionsClass::Instance->GameSpeed)
	{
	case 1:	// 60 FPS
		value = value / 4;
		break;
	case 2:	// 30 FPS
		value = value / 2;
		break;
	case 3:	// 20 FPS
		value = (value * 3) / 4;
		break;
	case 4:	// 15 FPS
		break;
	case 5:	// 12 FPS
		value = (value * 5) / 4;
		break;
	case 6:	// 10 FPS
		value = (value * 3) / 2;
		break;
	default:
		break;
	}

	return 0;
}

DEFINE_HOOK(0x6D4C68, PrintTimerOnTactical_End, 0x8)
{
	if (!Phobos::Config::RealTimeTimers)
		return 0;

	REF_STACK(int, value, STACK_OFFSET(0x654, 0x4));
	value = TimerValueTemp::oldValue;
	return 0;
}

DEFINE_HOOK(0x6D4CD9, PrintTimerOnTactical_BlinkColor, 0x6)
{
	enum { SkipGameCode = 0x6D4CE2 };

	R->EDI(ColorScheme::Array->GetItem(RulesExt::Global()->TimerBlinkColorScheme));

	return SkipGameCode;
}

DEFINE_HOOK(0x6D4CE6, PrintTimerOnTactical_RectTrans, 0x6)
{
	enum { SkipGameCode = 0x6D4DA2 };

	GET(const int, index, EAX);
	GET(BitFont* const, pBitInst, EBX);
	GET(ColorScheme* const, pNameScheme, EBP);
	GET(ColorScheme* const, pTimeScheme, EDI);
	GET_STACK(const int, timeWidth, STACK_OFFSET(0x644, -0x634));
	LEA_STACK(const wchar_t* const, pText, STACK_OFFSET(0x644, -0x200));
	LEA_STACK(const wchar_t* const, pNameText, STACK_OFFSET(0x644, -0x400));
	LEA_STACK(const wchar_t* const, pTimeText, STACK_OFFSET(0x644, -0x600));

	int width = 0;
	pBitInst->GetTextDimension(pText, &width, nullptr, DSurface::ViewBounds->Width);
	width += 6;
	const int lineSpace = pBitInst->field_1C + 2;
	Point2D location { DSurface::ViewBounds->Width, (DSurface::ViewBounds->Height - ((index + 1) * lineSpace)) };
	RectangleStruct rect { (location.X - width), location.Y, width, lineSpace };

	ColorStruct fillColor { 0, 0, 0 };
	DSurface::Composite->FillRectTrans(&rect, &fillColor, (InputManagerClass::Instance->IsForceMoveKeyPressed() ? 80 : 40));
	Point2D top { rect.X - 1, rect.Y };
	Point2D bot { top.X, top.Y + lineSpace - 1 };
	DSurface::Composite->DrawLine(&top, &bot, COLOR_BLACK);

	location.X -= 3;
	const RectangleStruct bounds = DSurface::Composite->GetRect();
	constexpr TextPrintType flag = TextPrintType::UseGradPal | TextPrintType::Right | TextPrintType::NoShadow | TextPrintType::Metal12;
	Point2D tmp { 0, 0 };
	Fancy_Text_Print_Wide(tmp, pTimeText, DSurface::Composite, bounds, location, pTimeScheme, nullptr, flag);
	location.X -= timeWidth;
	Fancy_Text_Print_Wide(tmp, pNameText, DSurface::Composite, bounds, location, pNameScheme, nullptr, flag);

	return SkipGameCode;
}
