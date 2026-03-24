// This file is part of IFStile project
// Copyright (C)2026 Dmitry Mekhontsev <mekhontsev@gmail.com>

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// at your option any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#ifndef __STDAFX_H__
#define __STDAFX_H__

#include "pch_core.h"

#ifdef __cplusplus

////////////////////////////////////////////////////////////////////////////////
#ifdef __ANDROID__
#include <jni.h>
#endif//__ANDROID__

#if defined(__EMSCRIPTEN__)
#define SDL_MAIN_HANDLED
#endif // __EMSCRIPTEN__
#include <SDL3/SDL.h>

#if defined(__EMSCRIPTEN__)
#define GL_GLEXT_PROTOTYPES 1
#include <emscripten.h>
#include <emscripten/html5.h>
#endif // __EMSCRIPTEN__

#if defined(__EMSCRIPTEN__) || defined(__ANDROID__)
#define IMS_USE_GL3
#endif //defined(__EMSCRIPTEN__) || defined(__ANDROID__)

#if defined(IMS_USE_DX11)
#include <d3d11.h>
#elif defined(IMS_USE_GL3)
#include <SDL3/SDL_opengles2.h>
#else
#include <SDL3/SDL_opengl.h>
#endif

#ifdef _MSC_VER
#undef max
#undef min
#endif //_MSC_VER

#if defined(NDEBUG) && !defined(DEVELOPER_VERSION)
#define IMGUI_DISABLE_DEMO_WINDOWS 1
#define IMGUI_DISABLE_DEBUG_TOOLS 1
#endif

#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS 1
#define IMGUI_DISABLE_OBSOLETE_KEYIO 1
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui.h>
#include <imgui_stdlib.h>


#endif //__cplusplus

#endif// __STDAFX_H__
