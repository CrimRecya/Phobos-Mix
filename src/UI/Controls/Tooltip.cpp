#include "Tooltip.h"

#include <Drawing.h>
#include <Surface.h>
#include <YRMath.h>

#include <vector>

namespace UIExt
{
	namespace
	{
		// Greedy word-wrap for a single hard line. Words are broken at spaces;
		// words that cannot fit on a line of their own (the usual case for CJK
		// text without spaces) are broken character by character. Content that
		// already fits passes through unchanged.
		std::vector<std::wstring> WrapHardLine(const std::wstring& line, int maxWidth)
		{
			const auto fits = [maxWidth](const std::wstring& candidate)
			{
				Point2D origin { 0, 0 };
				return Drawing::GetTextDimensions(candidate.c_str(), origin, 0, 3, 2).Width <= maxWidth;
			};

			if (fits(line))
				return { line };

			std::vector<std::wstring> lines;
			std::wstring current;
			size_t pos = 0;

			while (pos < line.size())
			{
				size_t start = pos;

				while (start < line.size() && line[start] == L' ')
					++start;

				size_t end = start;

				while (end < line.size() && line[end] != L' ')
					++end;

				const std::wstring spaces = line.substr(pos, start - pos);
				const std::wstring word = line.substr(start, end - start);
				pos = end;

				const std::wstring candidate = current.empty() ? word : current + spaces + word;

				if (fits(candidate))
				{
					current = candidate;
					continue;
				}

				if (!current.empty())
				{
					lines.push_back(std::move(current));
					current.clear();
				}

				if (fits(word))
				{
					current = word;
					continue;
				}

				for (const wchar_t c : word)
				{
					if (!current.empty())
					{
						std::wstring charCandidate = current + c;

						if (fits(charCandidate))
						{
							current = std::move(charCandidate);
							continue;
						}

						lines.push_back(std::move(current));
						current.clear();
					}

					current.push_back(c);
				}
			}

			if (!current.empty())
				lines.push_back(std::move(current));

			return lines;
		}

		// Splits text at '\n' into hard lines (empty lines kept).
		std::vector<std::wstring> SplitLines(const std::wstring& text)
		{
			std::vector<std::wstring> lines;
			size_t start = 0;

			while (start <= text.size())
			{
				const size_t found = text.find(L'\n', start);
				const size_t stop = found == std::wstring::npos ? text.size() : found;
				lines.push_back(text.substr(start, stop - start));

				if (stop == text.size())
					break;

				start = stop + 1;
			}

			return lines;
		}

		// Balanced wrap: greedy wrapping at a fixed width produces ragged last
		// lines (e.g. a single trailing character for a 16-char text wrapped at
		// 15 chars). Binary-search the smallest width that still yields the same
		// line count, which evens the lines out (16 chars become 8 + 8).
		std::vector<std::wstring> BalanceWrapHardLine(const std::wstring& line, int maxWidth)
		{
			auto result = WrapHardLine(line, maxWidth);
			const auto lineCount = result.size();

			if (lineCount <= 1)
				return result;

			int lo = 1;
			int hi = maxWidth;

			while (lo < hi)
			{
				const int mid = lo + (hi - lo) / 2;

				if (WrapHardLine(line, mid).size() <= lineCount)
					hi = mid;
				else
					lo = mid + 1;
			}

			// The minimal width with count <= lineCount always yields exactly
			// lineCount lines: the count is monotonically non-increasing in the
			// width and equals lineCount at maxWidth.
			return WrapHardLine(line, lo);
		}

		// Word-wraps the whole text; existing \n characters are kept as hard
		// breaks (including empty lines). Each hard line is balanced on its own.
		std::vector<std::wstring> WrapText(const std::wstring& text, int maxWidth)
		{
			std::vector<std::wstring> lines;

			for (const auto& segment : SplitLines(text))
			{
				for (auto& wrappedLine : BalanceWrapHardLine(segment, maxWidth))
					lines.push_back(std::move(wrappedLine));
			}

			return lines;
		}
	}

	void TooltipRenderer::Draw(const UIComponent& component, const UIComponent* layoutHost)
	{
		if (component.TooltipTitle.empty() && component.TooltipText.empty())
			return;

		int maxWidth = component.TooltipMaxWidth;

		if (maxWidth > 0)
		{
			const int maxViewWidth = DSurface::ViewBounds.Width - 16;

			if (maxWidth > maxViewWidth)
				maxWidth = maxViewWidth;
		}

		const std::vector<std::wstring> titleLines = component.TooltipTitle.empty()
			? std::vector<std::wstring> { }
			: (maxWidth > 0 ? WrapText(component.TooltipTitle, maxWidth) : SplitLines(component.TooltipTitle));
		const std::vector<std::wstring> bodyLines = component.TooltipText.empty()
			? std::vector<std::wstring> { }
			: (maxWidth > 0 ? WrapText(component.TooltipText, maxWidth) : SplitLines(component.TooltipText));

		// The engine's GetTextDimensions does not accumulate height over '\n'
		// lines, so measure per line: width is the widest line, height is the
		// line count times the tallest line's step (exactly what is drawn).
		Point2D measureOrigin { 0, 0 };
		int contentWidth = 0;
		int lineStep = 0;

		for (const auto& line : titleLines)
		{
			if (line.empty())
				continue;

			const auto rect = Drawing::GetTextDimensions(line.c_str(), measureOrigin, 0, 3, 2);
			contentWidth = Math::max(contentWidth, rect.Width);
			lineStep = Math::max(lineStep, rect.Height);
		}

		for (const auto& line : bodyLines)
		{
			if (line.empty())
				continue;

			const auto rect = Drawing::GetTextDimensions(line.c_str(), measureOrigin, 0, 3, 2);
			contentWidth = Math::max(contentWidth, rect.Width);
			lineStep = Math::max(lineStep, rect.Height);
		}

		lineStep = Math::max(1, lineStep);

		int contentHeight = 0;

		if (!titleLines.empty())
			contentHeight = static_cast<int>(titleLines.size()) * lineStep;

		if (!bodyLines.empty())
		{
			if (contentHeight > 0)
				contentHeight += component.TooltipLineSpacing;

			contentHeight += static_cast<int>(bodyLines.size()) * lineStep;
		}

		const int width = contentWidth + component.TooltipPadding * 2;
		const int height = contentHeight + component.TooltipPadding * 2;

		const int viewLeft = DSurface::ViewBounds.X;
		const int viewTop = DSurface::ViewBounds.Y;
		const int viewRight = DSurface::ViewBounds.X + DSurface::ViewBounds.Width;
		const int viewBottom = DSurface::ViewBounds.Y + DSurface::ViewBounds.Height;

		Point2D location { 0, 0 };

		if (!layoutHost || layoutHost == &component)
		{
			// Default: prefer the right side of the control, but flip to the
			// left when it would cross the right edge; prefer below, but flip
			// above when it would cross the bottom edge.
			location = { component.X + component.Width + 8, component.Y - 3 };

			if (location.X + width > viewRight)
				location.X = component.X - width - 8;

			if (location.Y + height > viewBottom)
				location.Y = component.Y - height - 8;
		}
		else
		{
			// Delegated: attach the tooltip just outside the host edge closest
			// to the source control, so it cannot cover sibling controls.
			// Horizontal sides are preferred; top/bottom are only used when
			// neither horizontal side fits into the visible area.
			const UIComponent& host = *layoutHost;
			const int distLeft = component.X - host.X;
			const int distRight = (host.X + host.Width) - (component.X + component.Width);

			const int xLeft = host.X - width - 8;
			const int xRight = host.X + host.Width + 8;
			const bool leftFits = xLeft >= viewLeft && xLeft + width <= viewRight;
			const bool rightFits = xRight >= viewLeft && xRight + width <= viewRight;
			const bool preferLeft = distLeft <= distRight;

			if (preferLeft ? leftFits : rightFits)
				location = { preferLeft ? xLeft : xRight, component.Y - 3 };
			else if (leftFits)
				location = { xLeft, component.Y - 3 };
			else if (rightFits)
				location = { xRight, component.Y - 3 };
			else
			{
				// Vertical fallback: nearest top/bottom edge of the host.
				const int distTop = component.Y - host.Y;
				const int distBottom = (host.Y + host.Height) - (component.Y + component.Height);

				if (distTop <= distBottom)
					location = { component.X, host.Y - height - 8 };
				else
					location = { component.X, host.Y + host.Height + 8 };
			}

			// Pull the tooltip fully into the visible area vertically when the
			// screen can contain it at all.
			if (height <= viewBottom - viewTop)
			{
				if (location.Y + height > viewBottom)
					location.Y = viewBottom - height;
				else if (location.Y < viewTop)
					location.Y = viewTop;
			}
		}

		// Keep the tooltip inside the visible screen area.
		if (location.X < viewLeft)
			location.X = viewLeft;
		if (location.Y < viewTop)
			location.Y = viewTop;

		RectangleStruct drawRect { location.X, location.Y, width, height };
		ColorStruct bgColor { 0, 0, 0 };
		DSurface::Composite->FillRectTrans(&drawRect, &bgColor, 40);
		DSurface::Composite->DrawRect(&drawRect, COLOR_WHITE);

		Point2D textPos = location + Point2D { component.TooltipPadding, component.TooltipPadding };

		// Surface::DrawText renders a single line and ignores '\n', so draw
		// line by line (same approach as Label::Draw).
		const auto drawLines = [&textPos, lineStep](const std::vector<std::wstring>& lines)
		{
			for (size_t i = 0; i < lines.size(); ++i)
			{
				if (!lines[i].empty())
					DSurface::Composite->DrawText(lines[i].c_str(), &textPos, COLOR_WHITE);

				if (i + 1 < lines.size())
					textPos.Y += lineStep;
			}
		};

		if (!titleLines.empty())
			drawLines(titleLines);

		if (!bodyLines.empty())
		{
			textPos.Y += static_cast<int>(titleLines.size()) * lineStep + component.TooltipLineSpacing;
			drawLines(bodyLines);
		}
	}
}
