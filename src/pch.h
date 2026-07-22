#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>

#include "RE/Fallout.h"
#include "F4SE/F4SE.h"

// Explicitly imports the unified extra data lists and camera engine layout files
#include "RE/E/ExtraDataList.h"
#include "RE/B/BGSLocation.h"
#include "RE/P/PlayerCamera.h"

#define DLLEXPORT __declspec(dllexport)
