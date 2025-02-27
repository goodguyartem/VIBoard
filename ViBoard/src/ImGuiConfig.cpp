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

#include "ImGuiConfig.h"

#ifdef VI_MSVC
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

#include <imgui_impl_sdl3.cpp>
#include <imgui_impl_opengl3.cpp>
#include <imgui.h>

#ifdef VI_MSVC
#pragma warning(pop)
#endif

namespace vi {
	void setDarkTheme() noexcept {
		ImGui::StyleColorsClassic();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* col = style.Colors;
		
		col[ImGuiCol_WindowBg] =  ImVec4(0.05f, 0.05f, 0.08f, 1.0f);
	}

	void setLightTheme() noexcept {
		ImGui::StyleColorsLight();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4* col = style.Colors;

		col[ImGuiCol_Button] = ImVec4(0.50f, 0.55f, 0.76f, 0.63f);
		col[ImGuiCol_ButtonHovered] = ImVec4(0.45f, 0.50f, 0.71f, 0.94f);
		col[ImGuiCol_ButtonActive] = ImVec4(0.40f, 0.45f, 0.66f, 0.98f);

		col[ImGuiCol_Tab] = ImVec4(0.55f, 0.60f, 0.81f, 0.68f);
		col[ImGuiCol_TabHovered] = ImVec4(0.43f, 0.48f, 0.69f, 1.0f);
		col[ImGuiCol_TabActive] = ImVec4(0.50f, 0.55f, 0.76f, 0.88f);
		col[ImGuiCol_TabSelectedOverline] = ImVec4(0.33f, 0.38f, 0.59f, 0.8f);
		col[ImGuiCol_TabDimmed] = ImVec4(0.50f, 0.55f, 0.76f, 0.38f);
		col[ImGuiCol_TabDimmedSelected] = ImVec4(0.50f, 0.55f, 0.76f, 0.58f);

		col[ImGuiCol_FrameBg] = col[ImGuiCol_Button];
		col[ImGuiCol_FrameBgHovered] = col[ImGuiCol_ButtonHovered];
		col[ImGuiCol_FrameBgActive] = col[ImGuiCol_ButtonActive];

		col[ImGuiCol_CheckMark] = ImVec4(0.20f, 0.25f, 0.46f, 0.9f);
		col[ImGuiCol_SliderGrab] = ImVec4(0.20f, 0.25f, 0.46f, 0.5f);
		col[ImGuiCol_SliderGrabActive] = ImVec4(0.20f, 0.25f, 0.46f, 0.7f);
		
		col[ImGuiCol_Border] = ImVec4(0.50f, 0.55f, 0.76f, 0.25f);
		col[ImGuiCol_BorderShadow] = ImVec4(0.50f, 0.55f, 0.76f, 0.35f);
		col[ImGuiCol_ResizeGripHovered] = ImVec4(0.50f, 0.55f, 0.76f, 0.75f);
		col[ImGuiCol_ResizeGripActive] = ImVec4(0.50f, 0.55f, 0.76f, 1.0f);

		col[ImGuiCol_Header] = col[ImGuiCol_Button];
		col[ImGuiCol_HeaderHovered] = col[ImGuiCol_ButtonHovered];
		col[ImGuiCol_HeaderActive] = col[ImGuiCol_ButtonActive];

		col[ImGuiCol_DockingPreview] = ImVec4(0.35f, 0.40f, 0.61f, 0.68f);
	}
}