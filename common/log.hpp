#pragma once

#include "common.hpp"

void InitLog();
void LogInfo(StringView tag, StringView message);
void LogMilestone(StringView tag, StringView message);
void LogWarning(StringView tag, StringView message);
void LogError(StringView tag, StringView message);
void LogDebug(StringView tag, StringView message);
