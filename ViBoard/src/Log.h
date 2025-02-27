/*
	ViBoard - Lightweight free & open-source soundboard.
	Copyright (C) 2025-EndOfTime  goodguyartem <https://www.github.com/goodguyartem>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#pragma once

#include <SDL3/SDL.h>

#include <tuple>

#if VI_LOG_LEVEL >= 6
#define VI_VERBOSE(fmt, ...) SDL_LogVerbose(SDL_LOG_CATEGORY_APPLICATION, fmt, __VA_ARGS__)
#else
#define VI_VERBOSE(fmt, ...) std::ignore = fmt;
#endif 

#if VI_LOG_LEVEL >= 5
#define VI_INFO(fmt, ...) SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, fmt, __VA_ARGS__)
#else
#define VI_INFO(fmt, ...) std::ignore = fmt;
#endif 

#if VI_LOG_LEVEL >= 4
#define VI_DEBUG(fmt, ...) SDL_LogDebug(SDL_LOG_CATEGORY_APPLICATION, fmt, __VA_ARGS__)
#else
#define VI_DEBUG(fmt, ...) std::ignore = fmt;
#endif 

#if VI_LOG_LEVEL >= 3
#define VI_WARN(fmt, ...) SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, fmt, __VA_ARGS__)
#else
#define VI_WARN(fmt, ...) std::ignore = fmt;
#endif 

#if VI_LOG_LEVEL >= 2
#define VI_ERROR(fmt, ...) SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, fmt, __VA_ARGS__)
#else
#define VI_ERROR(fmt, ...) std::ignore = fmt;
#endif 

#if VI_LOG_LEVEL >= 1
#define VI_CRITICAL(fmt, ...) SDL_LogCritical(SDL_LOG_CATEGORY_APPLICATION, fmt, __VA_ARGS__)
#else
#define VI_CRITICAL(fmt, ...) std::ignore = fmt;
#endif 