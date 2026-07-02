#include "ReSTIRGISettings.h"

FCriticalSection FReSTIRGISettingsManager::CriticalSection;
FReSTIRGISettings FReSTIRGISettingsManager::Settings;
FReSTIRGIRuntimeStats FReSTIRGISettingsManager::Stats;

FReSTIRGISettings FReSTIRGISettingsManager::GetSettings()
{
	FScopeLock Lock(&CriticalSection);
	return Settings;
}

FReSTIRGIRuntimeStats FReSTIRGISettingsManager::GetStats()
{
	FScopeLock Lock(&CriticalSection);
	return Stats;
}

void FReSTIRGISettingsManager::UpdateSettings(const FReSTIRGISettings& InSettings)
{
	FScopeLock Lock(&CriticalSection);
	const bool bKeepResetRequest = Settings.bResetHistory || InSettings.bResetHistory;
	Settings = InSettings;
	Settings.bResetHistory = bKeepResetRequest;
}

void FReSTIRGISettingsManager::RequestHistoryReset()
{
	FScopeLock Lock(&CriticalSection);
	Settings.bResetHistory = true;
	Stats.bHistoryValid = false;
}

void FReSTIRGISettingsManager::UpdateStats(const FReSTIRGIRuntimeStats& InStats)
{
	FScopeLock Lock(&CriticalSection);
	Stats = InStats;
}
