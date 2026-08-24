#pragma once

class BSurface;

namespace Mutation
{
	// Test-only art helper: maps a mutation IconIndex to a few well-known
	// unit CameoPCX surfaces (E1, E2, APOC, MAMM, COMA).
	BSurface* GetTestCameoSurface(int iconIndex);
}
