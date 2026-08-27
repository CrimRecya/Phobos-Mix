#pragma once
// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================

namespace Mutation
{
	// TEMP DEBUG: maps a mutation IconIndex to one of the mod's custom
	// mutator PCX filenames (BlackDeath.pcx, Blizzard.pcx, etc).
	const char* GetTestCameoFile(int iconIndex);

	int GetCustomPcxCount();
	const char* GetCustomPcxFile(int index);
}