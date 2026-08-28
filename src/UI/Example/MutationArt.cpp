// ============================================================================
// DISABLED EXAMPLE - kept for reference only.
// ============================================================================
#if 0
// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================
#include "MutationArt.h"

#include <TechnoTypeClass.h>

#include <Ext/TechnoType/Body.h>

namespace Mutation
{
	namespace
	{
		const char* TestCameoNames[] = { "E1", "E2", "APOC", "MAMM", "COMA" };
	}

	const char* GetTestCameoFile(int iconIndex)
	{
		if (iconIndex < 0)
			iconIndex = 0;

		const auto* pType = TechnoTypeClass::Find(TestCameoNames[iconIndex % 5]);
		if (!pType)
			return nullptr;

		const auto* pTypeExt = TechnoTypeExt::Fetch(pType);
		if (!pTypeExt)
			return nullptr;

		const char* filename = pTypeExt->CameoPCX.GetFilename();
		return (filename && *filename) ? filename : nullptr;
	}
}
#endif
