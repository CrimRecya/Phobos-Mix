#pragma once

struct ConfData
{
	bool UseOriginalCCFile{ false };

	int Reserved[31];
};

static_assert(sizeof(ConfData) == 128);

ConfData& GetConfigData();
void UseOriginalFileClass(bool Option);
bool UseOriginalFileClass();
