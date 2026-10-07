#pragma once

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "RE/Skyrim.h"
#include "SKSE/SKSE.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/msvc_sink.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace std::literals;
namespace logger = SKSE::log;
