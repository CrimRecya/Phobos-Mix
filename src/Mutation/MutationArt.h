#pragma once
// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================

namespace Mutation
{
	// Test-only art helper: maps a mutation IconIndex to a PCX filename
	// based on a few well-known unit cameo PCX files (E1, E2, APOC, MAMM, COMA).
	const char* GetTestCameoFile(int iconIndex);
}