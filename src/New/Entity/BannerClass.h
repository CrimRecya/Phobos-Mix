#pragma once

#include <New/Type/BannerTypeClass.h>

class BannerClass final : public EnumerableEntity<BannerClass>
{
public:
	BannerTypeClass* Type;
	int ID;
	Point2D Position;
	int Variable;
	int ShapeFrameIndex;
	bool IsGlobalVariable;
	int Duration;
	int Delay;

	BannerClass()
		: Type {}
		, ID {}
		, Position {}
		, Variable {}
		, ShapeFrameIndex {}
		, IsGlobalVariable {}
		, Duration {}
		, Delay {}
	{ };

	BannerClass(BannerTypeClass* pBannerType, int id, Point2D position, int variable, bool isGlobalVariable)
		: Type { pBannerType }
		, ID { id }
		, Position { static_cast<int>(position.X / 100.0 * DSurface::ViewBounds.Width), static_cast<int>(position.Y / 100.0 * DSurface::ViewBounds.Height) }
		, Variable { variable }
		, ShapeFrameIndex { 0 }
		, IsGlobalVariable { isGlobalVariable }
		, Duration { pBannerType->Duration }
		, Delay { pBannerType->Delay }
	{ };

	void Render();

	bool Load(PhobosStreamReader& Stm, bool RegisterForChange);
	bool Save(PhobosStreamWriter& Stm) const;

private:
	template <typename T>
	bool Serialize(T& Stm);

	void RenderPCX(Point2D position);
	void RenderSHP(Point2D position);
	void RenderCSF(Point2D position);
};
