// ============================================================================
// SAMPLE - kept as a runnable UI test.
// ============================================================================
#include "MutationArt.h"

namespace Mutation
{
	namespace
	{
		// TEMP DEBUG: use the mod's own custom mutator PCX files instead of the
		// built-in game cameo PCX. This tests whether the sample's right-side
		// IconStrip crashes with the same assets used by DynamicPatcher.
		const char* CustomPcxFiles[] =
		{
			"AggressiveDeployment.pcx",
			"AlienInfested.pcx",
			"Avenger.pcx",
			"BlackDeath.pcx",
			"Blizzard.pcx",
			"BoomBots.pcx",
			"BossNuke.pcx",
			"ChaosStudios.pcx",
			"Concussive.pcx",
			"Darkness.pcx",
			"Diffusion.pcx",
			"DoubleEdge.pcx",
			"EminentDomain.pcx",
			"EvasiveManeuvers.pcx",
			"FatalAttraction.pcx",
			"Fear.pcx",
			"GoingNuclear.pcx",
			"HardWill.pcx",
			"HeroesFromTheStorm.pcx",
			"Inspiration.pcx",
			"JustDie.pcx",
			"KillBots.pcx",
			"LaserDrill.pcx",
			"LavaBurst.pcx",
			"LifeLeech.pcx",
			"LongRange.pcx",
			"Magnificent.pcx",
			"MicroTransactions.pcx",
			"MineralShields.pcx",
			"Minesweeper.pcx",
			"MissileCommand.pcx",
			"MutBarrier.pcx",
			"MutRandom.pcx",
			"OrbitalStrike.pcx",
			"Outbreak.pcx",
			"Photon.pcx",
			"Polarity.pcx",
			"PowerOverwhelming.pcx",
			"Propagators.pcx",
			"PurifierBeam.pcx",
			"Recruit.pcx",
			"Scorch.pcx",
			"SelfDestruction.pcx",
			"ShortSighted.pcx",
			"Silence.pcx",
			"SlimPackings.pcx",
			"Speedrun.pcx",
			"TemporalField.pcx",
			"TimeWarp.pcx",
			"Transmutation.pcx",
			"Twister.pcx",
			"VoidReanimators.pcx",
			"VoidRifts.pcx",
			"WalkingDead.pcx",
			"WeMoveUnseen.pcx"
		};
	}

	int GetCustomPcxCount()
	{
		return static_cast<int>(sizeof(CustomPcxFiles) / sizeof(CustomPcxFiles[0]));
	}

	const char* GetCustomPcxFile(int index)
	{
		if (index < 0)
			index = 0;

		return CustomPcxFiles[index % GetCustomPcxCount()];
	}

	const char* GetTestCameoFile(int iconIndex)
	{
		return GetCustomPcxFile(iconIndex);
	}
}