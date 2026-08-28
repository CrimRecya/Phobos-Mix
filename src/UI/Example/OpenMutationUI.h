#pragma once
// ============================================================================
// DISABLED EXAMPLE - kept for reference only.
// ============================================================================
#if 0
// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================

#include <Commands/Commands.h>
#include <GameStrings.h>

namespace Mutation
{
	class OpenMutationUICommandClass : public CommandClass
	{
	public:
		virtual const char* GetName() const override;
		virtual const wchar_t* GetUIName() const override;
		virtual const wchar_t* GetUICategory() const override;
		virtual const wchar_t* GetUIDescription() const override;
		virtual void Execute(WWKey eInput) const override;
	};
}
#endif
