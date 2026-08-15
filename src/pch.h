#pragma once

#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>

#include "RE/Fallout.h"
#include "F4SE/F4SE.h"

// Optional extra includes used across the project
#include <shared_mutex>
#include <atomic>
#include <thread>

#define DLLEXPORT __declspec(dllexport)
