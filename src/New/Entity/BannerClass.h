#pragma once

#include <GeneralStructures.h>

#include <Ext/Scenario/Body.h>
#include <New/Type/BannerTypeClass.h>
#include <Utilities/EnumerableEntity.h>

class BannerClass final : public EnumerableEntity<BannerClass>
{
public:
	BannerTypeClass* Type;
	int ID;
	Point2D Position;
	int Variable;
	int ShapeFrameIndex;
	bool IsGlobalVariable;

	BannerClass()
		: Type {}
		, ID {}
		, Position {}
		, Variable {}
		, ShapeFrameIndex {}
		, IsGlobalVariable {}
	{ };

	BannerClass(BannerTypeClass* pBannerType, int id, Point2D position, int variable, bool isGlobalVariable)
		: Type { pBannerType }
		, ID { id }
		, Position { static_cast<int>(position.X / 100.0 * DSurface::ViewBounds.Width), static_cast<int>(position.Y / 100.0 * DSurface::ViewBounds.Height) }
		, Variable { variable }
		, ShapeFrameIndex { 0 }
		, IsGlobalVariable { isGlobalVariable }
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
