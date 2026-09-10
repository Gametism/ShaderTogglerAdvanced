// Font-table prefix from ReShade (BSD-3-Clause OR MIT):
// Copyright (C) 2021 Patrick Mours; Copyright (C) 2014-2026 Omar Cornut.
// https://github.com/crosire/reshade/blob/358c345ca2fe64f86e67c694f8379c356627adcb/source/imgui_function_table_19250.hpp
// Bridge implementation (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// See PROPRIETARY_LICENSE.txt for the add-on modifications.
#pragma once
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstring>

namespace sta_ui
{
struct ModernIO;
struct ModernStyle;
struct ModernDrawList;
struct ModernFont;
// The following table declaration is used under the MIT option above.
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
struct ModernFontTable
{
	ModernIO &(*GetIO)();
	ModernStyle &(*GetStyle)();
	const char *(*GetVersion)();
	bool(*Begin)(const char *name, bool *p_open, ImGuiWindowFlags flags);
	void(*End)();
	bool(*BeginChild)(const char *str_id, const ImVec2 &size, int child_flags, ImGuiWindowFlags window_flags);
	bool(*BeginChild2)(ImGuiID id, const ImVec2 &size, int child_flags, ImGuiWindowFlags window_flags);
	void(*EndChild)();
	bool(*IsWindowAppearing)();
	bool(*IsWindowCollapsed)();
	bool(*IsWindowFocused)(ImGuiFocusedFlags flags);
	bool(*IsWindowHovered)(ImGuiHoveredFlags flags);
	ModernDrawList *(*GetWindowDrawList)();
	float(*GetWindowDpiScale)();
	ImVec2(*GetWindowPos)();
	ImVec2(*GetWindowSize)();
	float(*GetWindowWidth)();
	float(*GetWindowHeight)();
	void(*SetNextWindowPos)(const ImVec2 &pos, ImGuiCond cond, const ImVec2 &pivot);
	void(*SetNextWindowSize)(const ImVec2 &size, ImGuiCond cond);
	void(*SetNextWindowSizeConstraints)(const ImVec2 &size_min, const ImVec2 &size_max, ImGuiSizeCallback custom_callback, void *custom_callback_data);
	void(*SetNextWindowContentSize)(const ImVec2 &size);
	void(*SetNextWindowCollapsed)(bool collapsed, ImGuiCond cond);
	void(*SetNextWindowFocus)();
	void(*SetNextWindowScroll)(const ImVec2 &scroll);
	void(*SetNextWindowBgAlpha)(float alpha);
	void(*SetWindowPos)(const ImVec2 &pos, ImGuiCond cond);
	void(*SetWindowSize)(const ImVec2 &size, ImGuiCond cond);
	void(*SetWindowCollapsed)(bool collapsed, ImGuiCond cond);
	void(*SetWindowFocus)();
	void(*SetWindowPos2)(const char *name, const ImVec2 &pos, ImGuiCond cond);
	void(*SetWindowSize2)(const char *name, const ImVec2 &size, ImGuiCond cond);
	void(*SetWindowCollapsed2)(const char *name, bool collapsed, ImGuiCond cond);
	void(*SetWindowFocus2)(const char *name);
	float(*GetScrollX)();
	float(*GetScrollY)();
	void(*SetScrollX)(float scroll_x);
	void(*SetScrollY)(float scroll_y);
	float(*GetScrollMaxX)();
	float(*GetScrollMaxY)();
	void(*SetScrollHereX)(float center_x_ratio);
	void(*SetScrollHereY)(float center_y_ratio);
	void(*SetScrollFromPosX)(float local_x, float center_x_ratio);
	void(*SetScrollFromPosY)(float local_y, float center_y_ratio);
	void(*PushFont)(ModernFont *font, float font_size_base_unscaled);
	void(*PopFont)();
};

using GetTable = const void*(*)(uint32_t);

struct ModernFontApi
{
    ModernFontTable table{};
    uint32_t version=0;

    static ModernFontApi resolve(GetTable getTable,const char* hostVersion)
    {
        ModernFontApi result;
        int major=0,minor=0,patch=0;
        if(!getTable || !hostVersion || std::sscanf(hostVersion,"%d.%d.%d",&major,&minor,&patch)!=3)
            return result;
        if(major<1 || (major==1 && (minor<92 || (minor==92 && patch<2)))) return result;
        const uint32_t candidates[]={19250,19222,19220};
        for(uint32_t version:candidates)
        {
            if(version==19250 && major==1 && minor==92 && patch<5) continue;
            if(const void* source=getTable(version))
            {
                std::memcpy(&result.table,source,sizeof(result.table));
                if(result.table.GetStyle && result.table.PushFont && result.table.PopFont)
                    result.version=version;
                break;
            }
        }
        return result;
    }

    bool push(float factor) const
    {
        if(!version) return false;
        float base=0;
        std::memcpy(&base,&table.GetStyle(),sizeof(base));
        if(!std::isfinite(base) || base<=0 || !std::isfinite(base*factor) || factor<=0) return false;
        table.PushFont(nullptr,base*factor);
        return true;
    }
    void pop() const { table.PopFont(); }
};
}
