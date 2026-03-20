///////////////////////////////////////////////////////////////////////
//
// Part of ShaderToggler Advanced – A shader toggler add-on for ReShade 5+
// which allows you to define groups of shaders to toggle them on/off
// with one key press.
//
// Based on the original ShaderToggler by Frans 'Otis_Inf' Bouma.
// (c) Frans 'Otis_Inf' Bouma. All rights reserved.
//
// https://github.com/FransBouma/ShaderToggler
//
// Modifications (including active-at-startup - x86, group reordering, duplication and UI changes)
// (c) 2026 Sven 'Gametism' Königsmann. All rights reserved.
//
/////////////////////////////////////////////////////////////////////////

#define IMGUI_DISABLE_INCLUDE_IMCONFIG_H
#define ImTextureID unsigned long long

#include <imgui.h>
#include <reshade.hpp>
#include "crc32_hash.hpp"
#include "ShaderManager.h"
#include "CDataFile.h"
#include "ToggleGroup.h"
#include "KeyData.h"
#include <vector>
#include <filesystem>
#include <Windows.h>
#include <chrono>
#include <algorithm>
#include <atomic>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <unordered_map>
#include <cmath>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

void saveShaderTogglerIniFile();

using namespace reshade::api;
using namespace ShaderToggler;

extern "C" __declspec(dllexport) const char *NAME = "Shader Toggler";
extern "C" __declspec(dllexport) const char *DESCRIPTION = "Add-on which allows you to define groups of game shaders to toggle on/off with one key press.";

struct __declspec(uuid("038B03AA-4C75-443B-A695-752D80797037")) CommandListDataContainer
{
	uint64_t activePixelShaderPipeline;
	uint64_t activeVertexShaderPipeline;
	uint64_t activeComputeShaderPipeline;
	uint32_t lastVertexCount;
	uint32_t lastIndexCount;
	bool lastDrawIndexed;

	uint64_t currentRenderTargetView;
	bool hasViewport;
	float viewportX;
	float viewportY;
	float viewportWidth;
	float viewportHeight;
};

struct PassSignature
{
	uint64_t pixelPipeline = 0;
	uint64_t renderTargetView = 0;
	bool hasViewport = false;
	float viewportX = 0.0f;
	float viewportY = 0.0f;
	float viewportWidth = 0.0f;
	float viewportHeight = 0.0f;
	uint32_t vertices = 0;
	uint32_t indices = 0;
	bool indexed = false;
};

#define FRAMECOUNT_COLLECTION_PHASE_DEFAULT 250
#define HASH_FILE_NAME "ShaderToggler.ini"

static ShaderManager g_pixelShaderManager;
static ShaderManager g_vertexShaderManager;
static ShaderManager g_computeShaderManager;
static KeyData g_keyCollector;
static std::atomic_uint32_t g_activeCollectorFrameCounter = 0;
static std::vector<ToggleGroup> g_toggleGroups;
static std::atomic_int g_toggleGroupIdKeyBindingEditing = -1;
static std::atomic_int g_toggleGroupIdShaderEditing = -1;
static float g_overlayOpacity = 1.0f;
static int g_startValueFramecountCollectionPhase = FRAMECOUNT_COLLECTION_PHASE_DEFAULT;
static std::string g_iniFileName = "";

// Per-group hotkey edge/debounce state
static std::unordered_map<int, bool> g_groupHotkeyWasDown;
static std::unordered_map<int, std::chrono::steady_clock::time_point> g_groupHotkeyLastToggleTime;
static const int g_groupHotkeyDebounceMs = 150;

// Hold-to-cycle state
static std::chrono::steady_clock::time_point s_lastNP1, s_lastNP2, s_lastNP4, s_lastNP5, s_lastNP7, s_lastNP8;
static bool s_np1Held = false, s_np2Held = false, s_np4Held = false, s_np5Held = false, s_np7Held = false, s_np8Held = false;
static int s_holdRepeatMs = 200;

// Watermark / hidden signature
static const char* GT_CREATOR = "Gametism";
static const char* GT_CACHE_KEY = "CacheStamp";
static const char* GT_HEADER =
	"; ==========================================\n"
	"; ShaderToggler Advanced Configuration\n"
	"; Created by Gametism\n"
	"; ==========================================\n\n";

static const char* GT_FOOTER =
	"\n; ==========================================\n"
	"; End of file – Gametism\n"
	"; ==========================================\n";

// Experimental pass-control state
static bool g_blockLikelyFullscreenPasses = false;
static int g_fullscreenPassMaxVertices = 6;
static int g_fullscreenPassMaxIndices = 12;
static bool g_showPassDebugInfo = true;
static uint32_t g_runtimeWidth = 0;
static uint32_t g_runtimeHeight = 0;

// New viewport / RT / corner UI controls
static bool g_matchCapturedPassViewport = true;
static bool g_matchCapturedPassRenderTarget = true;
static bool g_blockLikelyCornerUiPasses = false;
static float g_cornerUiMaxAreaRatio = 0.20f;
static float g_cornerUiMaxWidthRatio = 0.55f;
static float g_cornerUiMaxHeightRatio = 0.55f;
static float g_cornerUiEdgeToleranceRatio = 0.08f;
static float g_seenMaxViewportWidth = 0.0f;
static float g_seenMaxViewportHeight = 0.0f;

// Last blocked / seen / captured pass info
static PassSignature g_lastBlockedPass;
static PassSignature g_lastSeenPass;
static PassSignature g_capturedPass;
static bool g_enableCapturedPassBlocker = false;
static bool g_hasCapturedPass = false;

static bool is_key_down_numpad_or_nav(reshade::api::effect_runtime* runtime, int vk_numpad, int vk_nav)
{
	bool down = runtime->is_key_down(vk_numpad) || runtime->is_key_down(vk_nav);
	down = down || ((GetAsyncKeyState(vk_numpad) & 0x8000) != 0) || ((GetAsyncKeyState(vk_nav) & 0x8000) != 0);
	return down;
}

static bool is_key_pressed_numpad_or_nav(reshade::api::effect_runtime* runtime, int vk_numpad, int vk_nav)
{
	return runtime->is_key_pressed(vk_numpad) || runtime->is_key_pressed(vk_nav);
}

static uint64_t fnv1a64(const std::string& text)
{
	uint64_t hash = 14695981039346656037ull;
	for (unsigned char c : text)
	{
		hash ^= static_cast<uint64_t>(c);
		hash *= 1099511628211ull;
	}
	return hash;
}

static std::string toHex64(uint64_t value)
{
	char buf[17] = {};
	snprintf(buf, sizeof(buf), "%016llX", static_cast<unsigned long long>(value));
	return std::string(buf);
}

static bool floatNearlyEqual(float a, float b, float eps = 0.5f)
{
	return std::fabs(a - b) <= eps;
}

static std::string buildIniSignature()
{
	std::string data;
	data += "Creator=";
	data += GT_CREATOR;
	data += ";AmountGroups=";
	data += std::to_string(g_toggleGroups.size());

	for (const auto& group : g_toggleGroups)
	{
		data += "|Name=" + group.getName();
		data += "|Key=" + std::to_string(group.getToggleKey().toInt());
		data += "|Startup=" + std::to_string(group.isActiveAtStartup() ? 1 : 0);

		for (auto v : group.getPixelShaderHashes())
			data += "|P=" + std::to_string(v);
		for (auto v : group.getVertexShaderHashes())
			data += "|V=" + std::to_string(v);
		for (auto v : group.getComputeShaderHashes())
			data += "|C=" + std::to_string(v);
	}

	data += "|BlockLikelyFullscreenPasses=" + std::to_string(g_blockLikelyFullscreenPasses ? 1 : 0);
	data += "|FullscreenPassMaxVertices=" + std::to_string(g_fullscreenPassMaxVertices);
	data += "|FullscreenPassMaxIndices=" + std::to_string(g_fullscreenPassMaxIndices);
	data += "|ShowPassDebugInfo=" + std::to_string(g_showPassDebugInfo ? 1 : 0);

	data += "|EnableCapturedPassBlocker=" + std::to_string(g_enableCapturedPassBlocker ? 1 : 0);
	data += "|HasCapturedPass=" + std::to_string(g_hasCapturedPass ? 1 : 0);
	data += "|MatchCapturedPassViewport=" + std::to_string(g_matchCapturedPassViewport ? 1 : 0);
	data += "|MatchCapturedPassRenderTarget=" + std::to_string(g_matchCapturedPassRenderTarget ? 1 : 0);

	data += "|CapturedPipeline=" + std::to_string(static_cast<unsigned long long>(g_capturedPass.pixelPipeline));
	data += "|CapturedRTV=" + std::to_string(static_cast<unsigned long long>(g_capturedPass.renderTargetView));
	data += "|CapturedHasViewport=" + std::to_string(g_capturedPass.hasViewport ? 1 : 0);
	data += "|CapturedVPX=" + std::to_string(g_capturedPass.viewportX);
	data += "|CapturedVPY=" + std::to_string(g_capturedPass.viewportY);
	data += "|CapturedVPW=" + std::to_string(g_capturedPass.viewportWidth);
	data += "|CapturedVPH=" + std::to_string(g_capturedPass.viewportHeight);
	data += "|CapturedVertices=" + std::to_string(g_capturedPass.vertices);
	data += "|CapturedIndices=" + std::to_string(g_capturedPass.indices);
	data += "|CapturedIndexed=" + std::to_string(g_capturedPass.indexed ? 1 : 0);

	data += "|BlockLikelyCornerUiPasses=" + std::to_string(g_blockLikelyCornerUiPasses ? 1 : 0);
	data += "|CornerUiMaxAreaRatio=" + std::to_string(g_cornerUiMaxAreaRatio);
	data += "|CornerUiMaxWidthRatio=" + std::to_string(g_cornerUiMaxWidthRatio);
	data += "|CornerUiMaxHeightRatio=" + std::to_string(g_cornerUiMaxHeightRatio);
	data += "|CornerUiEdgeToleranceRatio=" + std::to_string(g_cornerUiEdgeToleranceRatio);

	return toHex64(fnv1a64(data));
}

static bool fileContainsTopAndBottomWatermark(const std::string& filename)
{
	std::ifstream inFile(filename, std::ios::binary);
	if (!inFile.is_open())
		return false;

	std::string content((std::istreambuf_iterator<char>(inFile)),
	                    std::istreambuf_iterator<char>());
	inFile.close();

	const bool hasHeader = content.rfind(GT_HEADER, 0) == 0;
	const bool hasFooter = content.find(GT_FOOTER) != std::string::npos;

	return hasHeader && hasFooter;
}

static void rewriteIniWithTopAndBottomWatermark(const std::string& filename)
{
	std::ifstream inFile(filename, std::ios::binary);
	if (!inFile.is_open())
		return;

	std::string content((std::istreambuf_iterator<char>(inFile)),
	                    std::istreambuf_iterator<char>());
	inFile.close();

	if (content.rfind(GT_HEADER, 0) == 0)
	{
		content.erase(0, std::strlen(GT_HEADER));
	}

	size_t footerPos = content.find(GT_FOOTER);
	if (footerPos != std::string::npos)
	{
		content.erase(footerPos, std::strlen(GT_FOOTER));
	}

	std::ofstream outFile(filename, std::ios::binary | std::ios::trunc);
	if (!outFile.is_open())
		return;

	outFile << GT_HEADER << content << GT_FOOTER;
	outFile.close();
}

static uint32_t calculateShaderHash(void* shaderData)
{
	if (nullptr == shaderData)
	{
		return 0;
	}

	const auto shaderDesc = *static_cast<shader_desc *>(shaderData);
	return compute_crc32(static_cast<const uint8_t *>(shaderDesc.code), shaderDesc.code_size);
}

static PassSignature buildCurrentPassSignature(command_list* commandList, uint32_t vertex_count, uint32_t index_count, bool indexed)
{
	PassSignature sig{};

	if (commandList == nullptr)
		return sig;

	const CommandListDataContainer& data = commandList->get_private_data<CommandListDataContainer>();

	sig.pixelPipeline = data.activePixelShaderPipeline;
	sig.renderTargetView = data.currentRenderTargetView;
	sig.hasViewport = data.hasViewport;
	sig.viewportX = data.viewportX;
	sig.viewportY = data.viewportY;
	sig.viewportWidth = data.viewportWidth;
	sig.viewportHeight = data.viewportHeight;
	sig.vertices = vertex_count;
	sig.indices = index_count;
	sig.indexed = indexed;

	return sig;
}

static bool isLikelyFullscreenPass(command_list* commandList, uint32_t vertex_count, uint32_t index_count, bool indexed)
{
	if (!g_blockLikelyFullscreenPasses || commandList == nullptr)
		return false;

	if (g_runtimeWidth == 0 || g_runtimeHeight == 0)
		return false;

	const CommandListDataContainer& data = commandList->get_private_data<CommandListDataContainer>();

	if (data.activePixelShaderPipeline == static_cast<uint64_t>(-1) || data.activePixelShaderPipeline == 0)
		return false;

	if (indexed)
	{
		if (index_count >= 3 && index_count <= static_cast<uint32_t>(g_fullscreenPassMaxIndices))
			return true;
	}
	else
	{
		if (vertex_count >= 3 && vertex_count <= static_cast<uint32_t>(g_fullscreenPassMaxVertices))
			return true;
	}

	return false;
}

static bool doesCapturedPassMatch(command_list* commandList, uint32_t vertex_count, uint32_t index_count, bool indexed)
{
	if (!g_enableCapturedPassBlocker || !g_hasCapturedPass || commandList == nullptr)
		return false;

	const PassSignature current = buildCurrentPassSignature(commandList, vertex_count, index_count, indexed);

	if (current.pixelPipeline != g_capturedPass.pixelPipeline)
		return false;

	if (current.indexed != g_capturedPass.indexed)
		return false;

	if (current.indexed)
	{
		if (current.indices != g_capturedPass.indices)
			return false;
	}
	else
	{
		if (current.vertices != g_capturedPass.vertices)
			return false;
	}

	if (g_matchCapturedPassRenderTarget)
	{
		if (current.renderTargetView != g_capturedPass.renderTargetView)
			return false;
	}

	if (g_matchCapturedPassViewport)
	{
		if (current.hasViewport != g_capturedPass.hasViewport)
			return false;

		if (current.hasViewport)
		{
			if (!floatNearlyEqual(current.viewportX, g_capturedPass.viewportX) ||
				!floatNearlyEqual(current.viewportY, g_capturedPass.viewportY) ||
				!floatNearlyEqual(current.viewportWidth, g_capturedPass.viewportWidth) ||
				!floatNearlyEqual(current.viewportHeight, g_capturedPass.viewportHeight))
			{
				return false;
			}
		}
	}

	return true;
}

static bool isLikelyCornerUiPass(command_list* commandList, uint32_t vertex_count, uint32_t index_count, bool indexed)
{
	if (!g_blockLikelyCornerUiPasses || commandList == nullptr)
		return false;

	const CommandListDataContainer& data = commandList->get_private_data<CommandListDataContainer>();

	if (data.activePixelShaderPipeline == static_cast<uint64_t>(-1) || data.activePixelShaderPipeline == 0)
		return false;

	if (!data.hasViewport)
		return false;

	if (g_seenMaxViewportWidth <= 1.0f || g_seenMaxViewportHeight <= 1.0f)
		return false;

	const float vpX = data.viewportX;
	const float vpY = data.viewportY;
	const float vpW = data.viewportWidth;
	const float vpH = data.viewportHeight;

	if (vpW <= 0.0f || vpH <= 0.0f)
		return false;

	const float maxW = g_seenMaxViewportWidth;
	const float maxH = g_seenMaxViewportHeight;

	const float widthRatio = vpW / maxW;
	const float heightRatio = vpH / maxH;
	const float areaRatio = (vpW * vpH) / (maxW * maxH);

	if (widthRatio > g_cornerUiMaxWidthRatio)
		return false;
	if (heightRatio > g_cornerUiMaxHeightRatio)
		return false;
	if (areaRatio > g_cornerUiMaxAreaRatio)
		return false;

	const float tolX = maxW * g_cornerUiEdgeToleranceRatio;
	const float tolY = maxH * g_cornerUiEdgeToleranceRatio;

	const bool nearLeft = vpX <= tolX;
	const bool nearTop = vpY <= tolY;
	const bool nearRight = (vpX + vpW) >= (maxW - tolX);
	const bool nearBottom = (vpY + vpH) >= (maxH - tolY);

	const bool nearCorner =
		(nearLeft && nearTop) ||
		(nearRight && nearTop) ||
		(nearLeft && nearBottom) ||
		(nearRight && nearBottom);

	const bool lowComplexity =
		(indexed && index_count <= 512) ||
		(!indexed && vertex_count <= 512);

	return nearCorner && lowComplexity;
}

static bool blockPassRuleForDraw(command_list* commandList, uint32_t vertex_count, uint32_t index_count, bool indexed)
{
	if (doesCapturedPassMatch(commandList, vertex_count, index_count, indexed))
	{
		g_lastBlockedPass = buildCurrentPassSignature(commandList, vertex_count, index_count, indexed);
		return true;
	}

	if (isLikelyCornerUiPass(commandList, vertex_count, index_count, indexed))
	{
		g_lastBlockedPass = buildCurrentPassSignature(commandList, vertex_count, index_count, indexed);
		return true;
	}

	if (isLikelyFullscreenPass(commandList, vertex_count, index_count, indexed))
	{
		g_lastBlockedPass = buildCurrentPassSignature(commandList, vertex_count, index_count, indexed);
		return true;
	}

	return false;
}

void addDefaultGroup()
{
	ToggleGroup toAdd("Default", ToggleGroup::getNewGroupId());
	toAdd.setToggleKey(VK_CAPITAL, false, false, false);
	g_toggleGroups.push_back(toAdd);
}

void loadShaderTogglerIniFile()
{
	CDataFile iniFile;
	if (!iniFile.Load(g_iniFileName))
	{
		return;
	}

	int numberOfGroups = iniFile.GetInt("GTAmountGroups", "General");
	bool usingCustomFormat = true;

	if (numberOfGroups == INT_MIN)
	{
		numberOfGroups = iniFile.GetInt("AmountGroups", "General");
		usingCustomFormat = false;
	}

	if (numberOfGroups == INT_MIN)
	{
		addDefaultGroup();
		g_toggleGroups[0].loadState(iniFile, -1, false);
		saveShaderTogglerIniFile();
		return;
	}

	g_toggleGroups.clear();
	for (int i = 0; i < numberOfGroups; i++)
	{
		g_toggleGroups.push_back(ToggleGroup("", ToggleGroup::getNewGroupId()));
		g_toggleGroups.back().loadState(iniFile, i, usingCustomFormat);
	}

	g_blockLikelyFullscreenPasses = iniFile.GetBool("BlockLikelyFullscreenPasses", "General");

	int loadedMaxVerts = iniFile.GetInt("FullscreenPassMaxVertices", "General");
	if (loadedMaxVerts != INT_MIN)
		g_fullscreenPassMaxVertices = loadedMaxVerts;

	int loadedMaxIndices = iniFile.GetInt("FullscreenPassMaxIndices", "General");
	if (loadedMaxIndices != INT_MIN)
		g_fullscreenPassMaxIndices = loadedMaxIndices;

	const t_Str showPassDebugValue = iniFile.GetValue("ShowPassDebugInfo", "General");
	if (!showPassDebugValue.empty())
		g_showPassDebugInfo = iniFile.GetBool("ShowPassDebugInfo", "General");

	const t_Str enableCapturedPassBlockerValue = iniFile.GetValue("EnableCapturedPassBlocker", "General");
	if (!enableCapturedPassBlockerValue.empty())
		g_enableCapturedPassBlocker = iniFile.GetBool("EnableCapturedPassBlocker", "General");

	const t_Str hasCapturedPassValue = iniFile.GetValue("HasCapturedPass", "General");
	if (!hasCapturedPassValue.empty())
		g_hasCapturedPass = iniFile.GetBool("HasCapturedPass", "General");

	const t_Str matchCapturedPassViewportValue = iniFile.GetValue("MatchCapturedPassViewport", "General");
	if (!matchCapturedPassViewportValue.empty())
		g_matchCapturedPassViewport = iniFile.GetBool("MatchCapturedPassViewport", "General");

	const t_Str matchCapturedPassRTValue = iniFile.GetValue("MatchCapturedPassRenderTarget", "General");
	if (!matchCapturedPassRTValue.empty())
		g_matchCapturedPassRenderTarget = iniFile.GetBool("MatchCapturedPassRenderTarget", "General");

	const t_Str blockLikelyCornerUiPassesValue = iniFile.GetValue("BlockLikelyCornerUiPasses", "General");
	if (!blockLikelyCornerUiPassesValue.empty())
		g_blockLikelyCornerUiPasses = iniFile.GetBool("BlockLikelyCornerUiPasses", "General");

	float loadedCornerArea = iniFile.GetFloat("CornerUiMaxAreaRatio", "General");
	if (loadedCornerArea != FLT_MIN)
		g_cornerUiMaxAreaRatio = loadedCornerArea;

	float loadedCornerWidth = iniFile.GetFloat("CornerUiMaxWidthRatio", "General");
	if (loadedCornerWidth != FLT_MIN)
		g_cornerUiMaxWidthRatio = loadedCornerWidth;

	float loadedCornerHeight = iniFile.GetFloat("CornerUiMaxHeightRatio", "General");
	if (loadedCornerHeight != FLT_MIN)
		g_cornerUiMaxHeightRatio = loadedCornerHeight;

	float loadedCornerEdgeTol = iniFile.GetFloat("CornerUiEdgeToleranceRatio", "General");
	if (loadedCornerEdgeTol != FLT_MIN)
		g_cornerUiEdgeToleranceRatio = loadedCornerEdgeTol;

	uint32_t loadedCapturedVerts = static_cast<uint32_t>(iniFile.GetUInt("CapturedPassVertices", "General"));
	if (loadedCapturedVerts != UINT_MAX)
		g_capturedPass.vertices = loadedCapturedVerts;

	uint32_t loadedCapturedIndices = static_cast<uint32_t>(iniFile.GetUInt("CapturedPassIndices", "General"));
	if (loadedCapturedIndices != UINT_MAX)
		g_capturedPass.indices = loadedCapturedIndices;

	const t_Str capturedIndexedValue = iniFile.GetValue("CapturedPassIndexed", "General");
	if (!capturedIndexedValue.empty())
		g_capturedPass.indexed = iniFile.GetBool("CapturedPassIndexed", "General");

	const t_Str capturedHasViewportValue = iniFile.GetValue("CapturedPassHasViewport", "General");
	if (!capturedHasViewportValue.empty())
		g_capturedPass.hasViewport = iniFile.GetBool("CapturedPassHasViewport", "General");

	float loadedVPX = iniFile.GetFloat("CapturedPassViewportX", "General");
	if (loadedVPX != FLT_MIN)
		g_capturedPass.viewportX = loadedVPX;

	float loadedVPY = iniFile.GetFloat("CapturedPassViewportY", "General");
	if (loadedVPY != FLT_MIN)
		g_capturedPass.viewportY = loadedVPY;

	float loadedVPW = iniFile.GetFloat("CapturedPassViewportW", "General");
	if (loadedVPW != FLT_MIN)
		g_capturedPass.viewportWidth = loadedVPW;

	float loadedVPH = iniFile.GetFloat("CapturedPassViewportH", "General");
	if (loadedVPH != FLT_MIN)
		g_capturedPass.viewportHeight = loadedVPH;

	uint32_t loadedCapturedPipelineLo = iniFile.GetUInt("CapturedPassPipelineLo", "General");
	uint32_t loadedCapturedPipelineHi = iniFile.GetUInt("CapturedPassPipelineHi", "General");
	if (loadedCapturedPipelineLo != UINT_MAX && loadedCapturedPipelineHi != UINT_MAX)
	{
		g_capturedPass.pixelPipeline =
			(static_cast<uint64_t>(loadedCapturedPipelineHi) << 32) |
			static_cast<uint64_t>(loadedCapturedPipelineLo);
	}

	uint32_t loadedCapturedRTLo = iniFile.GetUInt("CapturedPassRTVLo", "General");
	uint32_t loadedCapturedRTHi = iniFile.GetUInt("CapturedPassRTVHi", "General");
	if (loadedCapturedRTLo != UINT_MAX && loadedCapturedRTHi != UINT_MAX)
	{
		g_capturedPass.renderTargetView =
			(static_cast<uint64_t>(loadedCapturedRTHi) << 32) |
			static_cast<uint64_t>(loadedCapturedRTLo);
	}

	if (g_fullscreenPassMaxVertices < 3) g_fullscreenPassMaxVertices = 3;
	if (g_fullscreenPassMaxVertices > 128) g_fullscreenPassMaxVertices = 128;
	if (g_fullscreenPassMaxIndices < 3) g_fullscreenPassMaxIndices = 3;
	if (g_fullscreenPassMaxIndices > 256) g_fullscreenPassMaxIndices = 256;

	if (g_cornerUiMaxAreaRatio < 0.01f) g_cornerUiMaxAreaRatio = 0.01f;
	if (g_cornerUiMaxAreaRatio > 1.0f) g_cornerUiMaxAreaRatio = 1.0f;
	if (g_cornerUiMaxWidthRatio < 0.05f) g_cornerUiMaxWidthRatio = 0.05f;
	if (g_cornerUiMaxWidthRatio > 1.0f) g_cornerUiMaxWidthRatio = 1.0f;
	if (g_cornerUiMaxHeightRatio < 0.05f) g_cornerUiMaxHeightRatio = 0.05f;
	if (g_cornerUiMaxHeightRatio > 1.0f) g_cornerUiMaxHeightRatio = 1.0f;
	if (g_cornerUiEdgeToleranceRatio < 0.0f) g_cornerUiEdgeToleranceRatio = 0.0f;
	if (g_cornerUiEdgeToleranceRatio > 0.5f) g_cornerUiEdgeToleranceRatio = 0.5f;

	if (usingCustomFormat)
	{
		const std::string creator = iniFile.GetValue("Creator", "General");
		const std::string savedStamp = iniFile.GetValue(GT_CACHE_KEY, "General");
		const std::string currentStamp = buildIniSignature();

		bool needsRepair = false;

		if (creator != GT_CREATOR)
			needsRepair = true;

		if (savedStamp != currentStamp)
			needsRepair = true;

		if (!fileContainsTopAndBottomWatermark(g_iniFileName))
			needsRepair = true;

		if (needsRepair)
		{
			saveShaderTogglerIniFile();
		}
	}
}

void saveShaderTogglerIniFile()
{
	CDataFile iniFile;

	iniFile.SetInt("GTAmountGroups", static_cast<int>(g_toggleGroups.size()), "", "General");
	iniFile.SetValue("Creator", GT_CREATOR, "", "General");
	iniFile.SetValue(GT_CACHE_KEY, buildIniSignature(), "", "General");

	iniFile.SetBool("BlockLikelyFullscreenPasses", g_blockLikelyFullscreenPasses, "", "General");
	iniFile.SetInt("FullscreenPassMaxVertices", g_fullscreenPassMaxVertices, "", "General");
	iniFile.SetInt("FullscreenPassMaxIndices", g_fullscreenPassMaxIndices, "", "General");
	iniFile.SetBool("ShowPassDebugInfo", g_showPassDebugInfo, "", "General");

	iniFile.SetBool("EnableCapturedPassBlocker", g_enableCapturedPassBlocker, "", "General");
	iniFile.SetBool("HasCapturedPass", g_hasCapturedPass, "", "General");
	iniFile.SetBool("MatchCapturedPassViewport", g_matchCapturedPassViewport, "", "General");
	iniFile.SetBool("MatchCapturedPassRenderTarget", g_matchCapturedPassRenderTarget, "", "General");

	iniFile.SetUInt("CapturedPassVertices", g_capturedPass.vertices, "", "General");
	iniFile.SetUInt("CapturedPassIndices", g_capturedPass.indices, "", "General");
	iniFile.SetBool("CapturedPassIndexed", g_capturedPass.indexed, "", "General");
	iniFile.SetBool("CapturedPassHasViewport", g_capturedPass.hasViewport, "", "General");
	iniFile.SetFloat("CapturedPassViewportX", g_capturedPass.viewportX, "", "General");
	iniFile.SetFloat("CapturedPassViewportY", g_capturedPass.viewportY, "", "General");
	iniFile.SetFloat("CapturedPassViewportW", g_capturedPass.viewportWidth, "", "General");
	iniFile.SetFloat("CapturedPassViewportH", g_capturedPass.viewportHeight, "", "General");
	iniFile.SetUInt("CapturedPassPipelineLo", static_cast<uint32_t>(g_capturedPass.pixelPipeline & 0xFFFFFFFFull), "", "General");
	iniFile.SetUInt("CapturedPassPipelineHi", static_cast<uint32_t>((g_capturedPass.pixelPipeline >> 32) & 0xFFFFFFFFull), "", "General");
	iniFile.SetUInt("CapturedPassRTVLo", static_cast<uint32_t>(g_capturedPass.renderTargetView & 0xFFFFFFFFull), "", "General");
	iniFile.SetUInt("CapturedPassRTVHi", static_cast<uint32_t>((g_capturedPass.renderTargetView >> 32) & 0xFFFFFFFFull), "", "General");

	iniFile.SetBool("BlockLikelyCornerUiPasses", g_blockLikelyCornerUiPasses, "", "General");
	iniFile.SetFloat("CornerUiMaxAreaRatio", g_cornerUiMaxAreaRatio, "", "General");
	iniFile.SetFloat("CornerUiMaxWidthRatio", g_cornerUiMaxWidthRatio, "", "General");
	iniFile.SetFloat("CornerUiMaxHeightRatio", g_cornerUiMaxHeightRatio, "", "General");
	iniFile.SetFloat("CornerUiEdgeToleranceRatio", g_cornerUiEdgeToleranceRatio, "", "General");

	for (int i = 0; i < static_cast<int>(g_toggleGroups.size()); i++)
	{
		g_toggleGroups[i].saveState(iniFile, i, true);
	}

	iniFile.SetFileName(g_iniFileName);
	iniFile.Save();

	rewriteIniWithTopAndBottomWatermark(g_iniFileName);
}

static void onInitCommandList(command_list *commandList)
{
	commandList->create_private_data<CommandListDataContainer>();
	CommandListDataContainer &data = commandList->get_private_data<CommandListDataContainer>();
	data.activePixelShaderPipeline = static_cast<uint64_t>(-1);
	data.activeVertexShaderPipeline = static_cast<uint64_t>(-1);
	data.activeComputeShaderPipeline = static_cast<uint64_t>(-1);
	data.lastVertexCount = 0;
	data.lastIndexCount = 0;
	data.lastDrawIndexed = false;
	data.currentRenderTargetView = 0;
	data.hasViewport = false;
	data.viewportX = 0.0f;
	data.viewportY = 0.0f;
	data.viewportWidth = 0.0f;
	data.viewportHeight = 0.0f;
}

static void onDestroyCommandList(command_list *commandList)
{
	commandList->destroy_private_data<CommandListDataContainer>();
}

static void onResetCommandList(command_list *commandList)
{
	CommandListDataContainer &data = commandList->get_private_data<CommandListDataContainer>();
	data.activePixelShaderPipeline = static_cast<uint64_t>(-1);
	data.activeVertexShaderPipeline = static_cast<uint64_t>(-1);
	data.activeComputeShaderPipeline = static_cast<uint64_t>(-1);
	data.lastVertexCount = 0;
	data.lastIndexCount = 0;
	data.lastDrawIndexed = false;
	data.currentRenderTargetView = 0;
	data.hasViewport = false;
	data.viewportX = 0.0f;
	data.viewportY = 0.0f;
	data.viewportWidth = 0.0f;
	data.viewportHeight = 0.0f;
}

static void onInitPipeline(device *, pipeline_layout, uint32_t subobjectCount, const pipeline_subobject *subobjects, pipeline pipelineHandle)
{
	for (uint32_t i = 0; i < subobjectCount; ++i)
	{
		switch (subobjects[i].type)
		{
		case pipeline_subobject_type::vertex_shader:
			g_vertexShaderManager.addHashHandlePair(calculateShaderHash(subobjects[i].data), pipelineHandle.handle);
			break;
		case pipeline_subobject_type::pixel_shader:
			g_pixelShaderManager.addHashHandlePair(calculateShaderHash(subobjects[i].data), pipelineHandle.handle);
			break;
		case pipeline_subobject_type::compute_shader:
			g_computeShaderManager.addHashHandlePair(calculateShaderHash(subobjects[i].data), pipelineHandle.handle);
			break;
		default:
			break;
		}
	}
}

static void onDestroyPipeline(device *, pipeline pipelineHandle)
{
	g_pixelShaderManager.removeHandle(pipelineHandle.handle);
	g_vertexShaderManager.removeHandle(pipelineHandle.handle);
	g_computeShaderManager.removeHandle(pipelineHandle.handle);
}

static void onBindRenderTargetsAndDepthStencil(command_list* commandList, uint32_t count, const resource_view* rtvs, resource_view)
{
	if (commandList == nullptr)
		return;

	CommandListDataContainer& data = commandList->get_private_data<CommandListDataContainer>();
	data.currentRenderTargetView = (count > 0 && rtvs != nullptr) ? rtvs[0].handle : 0;
}

static void onBindViewports(command_list* commandList, uint32_t first, uint32_t count, const viewport* viewports)
{
	if (commandList == nullptr || viewports == nullptr || count == 0)
		return;

	if (first != 0)
		return;

	CommandListDataContainer& data = commandList->get_private_data<CommandListDataContainer>();

	data.hasViewport = true;
	data.viewportX = viewports[0].x;
	data.viewportY = viewports[0].y;
	data.viewportWidth = viewports[0].width;
	data.viewportHeight = viewports[0].height;

	if (data.viewportWidth > g_seenMaxViewportWidth)
		g_seenMaxViewportWidth = data.viewportWidth;
	if (data.viewportHeight > g_seenMaxViewportHeight)
		g_seenMaxViewportHeight = data.viewportHeight;
}

static void displayIsPartOfToggleGroup()
{
	ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
	ImGui::SameLine();
	ImGui::Text(" Shader is part of this toggle group.");
	ImGui::PopStyleColor();
}

static void displayShaderManagerInfo(ShaderManager& toDisplay, const char* shaderType)
{
	if (toDisplay.isInHuntingMode())
	{
		ImGui::Text("# of %s shaders active: %d. # of %s shaders in group: %d",
			shaderType, toDisplay.getAmountShaderHashesCollected(), shaderType, toDisplay.getMarkedShaderCount());
		ImGui::Text("Current selected %s shader: %d / %d.",
			shaderType, toDisplay.getActiveHuntedShaderIndex(), toDisplay.getAmountShaderHashesCollected());
		if (toDisplay.isHuntedShaderMarked())
		{
			displayIsPartOfToggleGroup();
		}
	}
}

static void displayShaderManagerStats(ShaderManager& toDisplay, const char* shaderType)
{
	ImGui::Text("# of pipelines with %s shaders: %d. # of different %s shaders gathered: %d.",
		shaderType, toDisplay.getPipelineCount(), shaderType, toDisplay.getShaderCount());
}

static void displayPassSignatureText(const char* label, const PassSignature& sig)
{
	ImGui::Text("%s: %s, pipeline=%llu, rtv=%llu, verts=%u, indices=%u",
		label,
		sig.indexed ? "indexed" : "non-indexed",
		static_cast<unsigned long long>(sig.pixelPipeline),
		static_cast<unsigned long long>(sig.renderTargetView),
		sig.vertices,
		sig.indices);

	if (sig.hasViewport)
	{
		ImGui::Text("  viewport: x=%.1f y=%.1f w=%.1f h=%.1f",
			sig.viewportX, sig.viewportY, sig.viewportWidth, sig.viewportHeight);
	}
	else
	{
		ImGui::Text("  viewport: unavailable");
	}
}

static void onReshadeOverlay(reshade::api::effect_runtime *)
{
	if (g_toggleGroupIdShaderEditing >= 0)
	{
		ImGui::SetNextWindowBgAlpha(g_overlayOpacity);
		ImGui::SetNextWindowPos(ImVec2(10, 10));
		if (!ImGui::Begin("ShaderTogglerInfo", nullptr,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings))
		{
			ImGui::End();
			return;
		}

		std::string editingGroupName = "";
		for (auto& group : g_toggleGroups)
		{
			if (group.getId() == g_toggleGroupIdShaderEditing)
			{
				editingGroupName = group.getName();
				break;
			}
		}

		displayShaderManagerStats(g_vertexShaderManager, "vertex");
		displayShaderManagerStats(g_pixelShaderManager, "pixel");
		displayShaderManagerStats(g_computeShaderManager, "compute");

		if (g_showPassDebugInfo)
		{
			ImGui::Separator();
			ImGui::Text("Backbuffer size: unavailable in this ReShade API version");
			ImGui::Text("Largest seen viewport: %.1f x %.1f", g_seenMaxViewportWidth, g_seenMaxViewportHeight);
			ImGui::Text("Fullscreen pass blocker: %s", g_blockLikelyFullscreenPasses ? "ON" : "OFF");
			ImGui::Text("Captured pass blocker: %s", g_enableCapturedPassBlocker ? "ON" : "OFF");
			ImGui::Text("Corner/UI pass blocker: %s", g_blockLikelyCornerUiPasses ? "ON" : "OFF");
			displayPassSignatureText("Last seen pass", g_lastSeenPass);
			displayPassSignatureText("Captured pass", g_capturedPass);
			displayPassSignatureText("Last blocked pass", g_lastBlockedPass);
		}

		if (g_activeCollectorFrameCounter > 0)
		{
			const uint32_t counterValue = g_activeCollectorFrameCounter;
			ImGui::Text("Collecting active shaders... frames to go: %d", counterValue);
		}
		else
		{
			if (g_vertexShaderManager.isInHuntingMode() || g_pixelShaderManager.isInHuntingMode() || g_computeShaderManager.isInHuntingMode())
			{
				ImGui::Text("Editing the shaders for group: %s", editingGroupName.c_str());
			}
			displayShaderManagerInfo(g_vertexShaderManager, "vertex");
			displayShaderManagerInfo(g_pixelShaderManager, "pixel");
			displayShaderManagerInfo(g_computeShaderManager, "compute");
		}
		ImGui::End();
	}
}

static void onBindPipeline(command_list* commandList, pipeline_stage stages, pipeline pipelineHandle)
{
	if (nullptr != commandList && pipelineHandle.handle != 0)
	{
		const bool handleHasPixelShaderAttached = g_pixelShaderManager.isKnownHandle(pipelineHandle.handle);
		const bool handleHasVertexShaderAttached = g_vertexShaderManager.isKnownHandle(pipelineHandle.handle);
		const bool handleHasComputeShaderAttached = g_computeShaderManager.isKnownHandle(pipelineHandle.handle);

		if (!handleHasPixelShaderAttached && !handleHasVertexShaderAttached && !handleHasComputeShaderAttached)
		{
			return;
		}

		CommandListDataContainer& commandListData = commandList->get_private_data<CommandListDataContainer>();

		if (g_activeCollectorFrameCounter > 0)
		{
			if (handleHasPixelShaderAttached) g_pixelShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			if (handleHasVertexShaderAttached) g_vertexShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			if (handleHasComputeShaderAttached) g_computeShaderManager.addActivePipelineHandle(pipelineHandle.handle);
		}
		else
		{
			commandListData.activePixelShaderPipeline = handleHasPixelShaderAttached ? pipelineHandle.handle : commandListData.activePixelShaderPipeline;
			commandListData.activeVertexShaderPipeline = handleHasVertexShaderAttached ? pipelineHandle.handle : commandListData.activeVertexShaderPipeline;
			commandListData.activeComputeShaderPipeline = handleHasComputeShaderAttached ? pipelineHandle.handle : commandListData.activeComputeShaderPipeline;
		}

		if ((stages & pipeline_stage::pixel_shader) == pipeline_stage::pixel_shader && handleHasPixelShaderAttached)
		{
			if (g_activeCollectorFrameCounter > 0) g_pixelShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			commandListData.activePixelShaderPipeline = pipelineHandle.handle;
		}
		if ((stages & pipeline_stage::vertex_shader) == pipeline_stage::vertex_shader && handleHasVertexShaderAttached)
		{
			if (g_activeCollectorFrameCounter > 0) g_vertexShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			commandListData.activeVertexShaderPipeline = pipelineHandle.handle;
		}
		if ((stages & pipeline_stage::compute_shader) == pipeline_stage::compute_shader && handleHasComputeShaderAttached)
		{
			if (g_activeCollectorFrameCounter > 0) g_computeShaderManager.addActivePipelineHandle(pipelineHandle.handle);
			commandListData.activeComputeShaderPipeline = pipelineHandle.handle;
		}
	}
}

bool blockDrawCallForCommandList(command_list* commandList)
{
	if (nullptr == commandList)
	{
		return false;
	}

	const CommandListDataContainer &commandListData = commandList->get_private_data<CommandListDataContainer>();

	uint32_t shaderHash = g_pixelShaderManager.getShaderHash(commandListData.activePixelShaderPipeline);
	bool blockCall = g_pixelShaderManager.isBlockedShader(shaderHash);
	for (auto& group : g_toggleGroups)
	{
		for (auto hash : group.getPixelShaderHashes())
		{
			if (group.isActive() && hash == shaderHash)
			{
				blockCall = true;
				break;
			}
		}
	}

	shaderHash = g_vertexShaderManager.getShaderHash(commandListData.activeVertexShaderPipeline);
	blockCall |= g_vertexShaderManager.isBlockedShader(shaderHash);
	for (auto& group : g_toggleGroups)
	{
		for (auto hash : group.getVertexShaderHashes())
		{
			if (group.isActive() && hash == shaderHash)
			{
				blockCall = true;
				break;
			}
		}
	}

	shaderHash = g_computeShaderManager.getShaderHash(commandListData.activeComputeShaderPipeline);
	blockCall |= g_computeShaderManager.isBlockedShader(shaderHash);
	for (auto& group : g_toggleGroups)
	{
		for (auto hash : group.getComputeShaderHashes())
		{
			if (group.isActive() && hash == shaderHash)
			{
				blockCall = true;
				break;
			}
		}
	}

	return blockCall;
}

static bool onDraw(command_list* commandList, uint32_t vertex_count, uint32_t, uint32_t, uint32_t)
{
	if (commandList != nullptr)
	{
		CommandListDataContainer& data = commandList->get_private_data<CommandListDataContainer>();
		data.lastVertexCount = vertex_count;
		data.lastIndexCount = 0;
		data.lastDrawIndexed = false;
		g_lastSeenPass = buildCurrentPassSignature(commandList, vertex_count, 0, false);
	}

	return blockDrawCallForCommandList(commandList) ||
	       blockPassRuleForDraw(commandList, vertex_count, 0, false);
}

static bool onDrawIndexed(command_list* commandList, uint32_t index_count, uint32_t, uint32_t, int32_t, uint32_t)
{
	if (commandList != nullptr)
	{
		CommandListDataContainer& data = commandList->get_private_data<CommandListDataContainer>();
		data.lastVertexCount = 0;
		data.lastIndexCount = index_count;
		data.lastDrawIndexed = true;
		g_lastSeenPass = buildCurrentPassSignature(commandList, 0, index_count, true);
	}

	return blockDrawCallForCommandList(commandList) ||
	       blockPassRuleForDraw(commandList, 0, index_count, true);
}

static bool onDrawOrDispatchIndirect(command_list* commandList, indirect_command type, resource, uint64_t, uint32_t, uint32_t)
{
	switch (type)
	{
	case indirect_command::unknown:
	case indirect_command::draw:
	case indirect_command::draw_indexed:
	case indirect_command::dispatch:
		return blockDrawCallForCommandList(commandList);
	default:
		return false;
	}
}

static void onReshadePresent(effect_runtime* runtime)
{
	if (g_activeCollectorFrameCounter > 0)
	{
		--g_activeCollectorFrameCounter;
	}

	if (g_runtimeWidth == 0) g_runtimeWidth = 1;
	if (g_runtimeHeight == 0) g_runtimeHeight = 1;

	for (auto& group : g_toggleGroups)
	{
		const bool isDownNow = group.getToggleKey().isKeyDown(runtime);
		bool& wasDownLastFrame = g_groupHotkeyWasDown[group.getId()];
		auto& lastToggleTime = g_groupHotkeyLastToggleTime[group.getId()];

		const auto nowTime = std::chrono::steady_clock::now();
		const auto msSinceLastToggle =
			std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - lastToggleTime).count();

		if (isDownNow && !wasDownLastFrame && msSinceLastToggle > g_groupHotkeyDebounceMs)
		{
			group.setActive(!group.isActive());
			lastToggleTime = nowTime;

			if (group.getId() == g_toggleGroupIdShaderEditing)
			{
				g_vertexShaderManager.toggleHideMarkedShaders();
				g_pixelShaderManager.toggleHideMarkedShaders();
				g_computeShaderManager.toggleHideMarkedShaders();
			}
		}

		wasDownLastFrame = isDownNow;
	}

	const bool ctrlDown = runtime->is_key_down(VK_CONTROL);
	auto now = std::chrono::steady_clock::now();

	const int NP1 = VK_NUMPAD1, NAV1 = VK_END;
	const int NP2 = VK_NUMPAD2, NAV2 = VK_DOWN;
	const int NP3 = VK_NUMPAD3, NAV3 = VK_NEXT;
	const int NP4 = VK_NUMPAD4, NAV4 = VK_LEFT;
	const int NP5 = VK_NUMPAD5, NAV5 = VK_CLEAR;
	const int NP6 = VK_NUMPAD6, NAV6 = VK_RIGHT;
	const int NP7 = VK_NUMPAD7, NAV7 = VK_HOME;
	const int NP8 = VK_NUMPAD8, NAV8 = VK_UP;
	const int NP9 = VK_NUMPAD9, NAV9 = VK_PRIOR;

	bool np1Down = is_key_down_numpad_or_nav(runtime, NP1, NAV1);
	bool np1Pressed = is_key_pressed_numpad_or_nav(runtime, NP1, NAV1);
	if (np1Pressed)
	{
		g_pixelShaderManager.huntPreviousShader(ctrlDown);
		s_lastNP1 = now;
		s_np1Held = true;
	}
	else if (np1Down && s_np1Held &&
		std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP1).count() >= s_holdRepeatMs)
	{
		g_pixelShaderManager.huntPreviousShader(ctrlDown);
		s_lastNP1 = now;
	}
	else if (!np1Down)
	{
		s_np1Held = false;
	}

	bool np2Down = is_key_down_numpad_or_nav(runtime, NP2, NAV2);
	bool np2Pressed = is_key_pressed_numpad_or_nav(runtime, NP2, NAV2);
	if (np2Pressed)
	{
		g_pixelShaderManager.huntNextShader(ctrlDown);
		s_lastNP2 = now;
		s_np2Held = true;
	}
	else if (np2Down && s_np2Held &&
		std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP2).count() >= s_holdRepeatMs)
	{
		g_pixelShaderManager.huntNextShader(ctrlDown);
		s_lastNP2 = now;
	}
	else if (!np2Down)
	{
		s_np2Held = false;
	}

	if (is_key_pressed_numpad_or_nav(runtime, NP3, NAV3))
	{
		g_pixelShaderManager.toggleMarkOnHuntedShader();
	}

	bool np4Down = is_key_down_numpad_or_nav(runtime, NP4, NAV4);
	bool np4Pressed = is_key_pressed_numpad_or_nav(runtime, NP4, NAV4);
	if (np4Pressed)
	{
		g_vertexShaderManager.huntPreviousShader(ctrlDown);
		s_lastNP4 = now;
		s_np4Held = true;
	}
	else if (np4Down && s_np4Held &&
		std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP4).count() >= s_holdRepeatMs)
	{
		g_vertexShaderManager.huntPreviousShader(ctrlDown);
		s_lastNP4 = now;
	}
	else if (!np4Down)
	{
		s_np4Held = false;
	}

	bool np5Down = is_key_down_numpad_or_nav(runtime, NP5, NAV5);
	bool np5Pressed = is_key_pressed_numpad_or_nav(runtime, NP5, NAV5);
	if (np5Pressed)
	{
		g_vertexShaderManager.huntNextShader(ctrlDown);
		s_lastNP5 = now;
		s_np5Held = true;
	}
	else if (np5Down && s_np5Held &&
		std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP5).count() >= s_holdRepeatMs)
	{
		g_vertexShaderManager.huntNextShader(ctrlDown);
		s_lastNP5 = now;
	}
	else if (!np5Down)
	{
		s_np5Held = false;
	}

	if (is_key_pressed_numpad_or_nav(runtime, NP6, NAV6))
	{
		g_vertexShaderManager.toggleMarkOnHuntedShader();
	}

	bool np7Down = is_key_down_numpad_or_nav(runtime, NP7, NAV7);
	bool np7Pressed = is_key_pressed_numpad_or_nav(runtime, NP7, NAV7);
	if (np7Pressed)
	{
		g_computeShaderManager.huntPreviousShader(ctrlDown);
		s_lastNP7 = now;
		s_np7Held = true;
	}
	else if (np7Down && s_np7Held &&
		std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP7).count() >= s_holdRepeatMs)
	{
		g_computeShaderManager.huntPreviousShader(ctrlDown);
		s_lastNP7 = now;
	}
	else if (!np7Down)
	{
		s_np7Held = false;
	}

	bool np8Down = is_key_down_numpad_or_nav(runtime, NP8, NAV8);
	bool np8Pressed = is_key_pressed_numpad_or_nav(runtime, NP8, NAV8);
	if (np8Pressed)
	{
		g_computeShaderManager.huntNextShader(ctrlDown);
		s_lastNP8 = now;
		s_np8Held = true;
	}
	else if (np8Down && s_np8Held &&
		std::chrono::duration_cast<std::chrono::milliseconds>(now - s_lastNP8).count() >= s_holdRepeatMs)
	{
		g_computeShaderManager.huntNextShader(ctrlDown);
		s_lastNP8 = now;
	}
	else if (!np8Down)
	{
		s_np8Held = false;
	}

	if (is_key_pressed_numpad_or_nav(runtime, NP9, NAV9))
	{
		g_computeShaderManager.toggleMarkOnHuntedShader();
	}
}

void endKeyBindingEditing(bool acceptCollectedBinding, ToggleGroup& groupEditing)
{
	if (acceptCollectedBinding && g_toggleGroupIdKeyBindingEditing == groupEditing.getId() && g_keyCollector.isValid())
	{
		groupEditing.setToggleKey(g_keyCollector);
	}
	g_toggleGroupIdKeyBindingEditing = -1;
	g_keyCollector.clear();
}

void startKeyBindingEditing(ToggleGroup& groupEditing)
{
	if (g_toggleGroupIdKeyBindingEditing == groupEditing.getId())
	{
		return;
	}
	if (g_toggleGroupIdKeyBindingEditing >= 0)
	{
		endKeyBindingEditing(false, groupEditing);
	}
	g_toggleGroupIdKeyBindingEditing = groupEditing.getId();
}

void endShaderEditing(bool acceptCollectedShaderHashes, ToggleGroup& groupEditing)
{
	if (acceptCollectedShaderHashes && g_toggleGroupIdShaderEditing == groupEditing.getId())
	{
		groupEditing.storeCollectedHashes(
			g_pixelShaderManager.getMarkedShaderHashes(),
			g_vertexShaderManager.getMarkedShaderHashes(),
			g_computeShaderManager.getMarkedShaderHashes());

		g_pixelShaderManager.stopHuntingMode();
		g_vertexShaderManager.stopHuntingMode();
		g_computeShaderManager.stopHuntingMode();
	}
	g_toggleGroupIdShaderEditing = -1;
}

void startShaderEditing(ToggleGroup& groupEditing)
{
	if (g_toggleGroupIdShaderEditing == groupEditing.getId())
	{
		return;
	}
	if (g_toggleGroupIdShaderEditing >= 0)
	{
		endShaderEditing(false, groupEditing);
	}

	g_toggleGroupIdShaderEditing = groupEditing.getId();
	g_activeCollectorFrameCounter = g_startValueFramecountCollectionPhase;
	g_pixelShaderManager.startHuntingMode(groupEditing.getPixelShaderHashes());
	g_vertexShaderManager.startHuntingMode(groupEditing.getVertexShaderHashes());
	g_computeShaderManager.startHuntingMode(groupEditing.getComputeShaderHashes());

	groupEditing.clearHashes();
}

static void showHelpMarker(const char* desc)
{
	ImGui::TextDisabled("(?)");
	if (ImGui::IsItemHovered())
	{
		ImGui::BeginTooltip();
		ImGui::PushTextWrapPos(450.0f);
		ImGui::TextUnformatted(desc);
		ImGui::PopTextWrapPos();
		ImGui::EndTooltip();
	}
}

static void displaySettings(reshade::api::effect_runtime* runtime)
{
	if (g_toggleGroupIdKeyBindingEditing >= 0)
	{
		g_keyCollector.collectKeysPressed(runtime);
	}

	if (ImGui::CollapsingHeader("General info and help"))
	{
		ImGui::PushTextWrapPos();
		ImGui::TextUnformatted("The Shader Toggler allows you to create one or more groups with shaders to toggle on/off. You can assign a keyboard shortcut (including using keys like Shift, Alt and Control) to each group, including a handy name. Each group can have one or more vertex, pixel or compute shaders assigned to it. When you press the assigned keyboard shortcut, any draw calls using these shaders will be disabled.");
		ImGui::TextUnformatted("\nThe following keyboard shortcuts are used when you click a group's 'Change shaders' button:");
		ImGui::TextUnformatted("* Numpad 1 / 2: previous/next pixel shader");
		ImGui::TextUnformatted("* Ctrl + Numpad 1 / 2: previous/next marked pixel shader");
		ImGui::TextUnformatted("* Numpad 3: mark/unmark current pixel shader");
		ImGui::TextUnformatted("* Numpad 4 / 5: previous/next vertex shader");
		ImGui::TextUnformatted("* Ctrl + Numpad 4 / 5: previous/next marked vertex shader");
		ImGui::TextUnformatted("* Numpad 6: mark/unmark current vertex shader");
		ImGui::TextUnformatted("* Numpad 7 / 8: previous/next compute shader");
		ImGui::TextUnformatted("* Ctrl + Numpad 7 / 8: previous/next marked compute shader");
		ImGui::TextUnformatted("* Numpad 9: mark/unmark current compute shader");
		ImGui::PopTextWrapPos();
	}

	if (ImGui::CollapsingHeader("Shader selection parameters", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
		ImGui::SliderFloat("Overlay opacity", &g_overlayOpacity, 0.01f, 1.0f);
		ImGui::SliderInt("# of frames to collect", &g_startValueFramecountCollectionPhase, 10, 1000);
		ImGui::SameLine();
		showHelpMarker("This is the number of frames the addon will collect active shaders. Set this to a high number if the shader you want to mark is only used occasionally.");
		ImGui::PopItemWidth();
	}
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Render pass control (experimental)", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::PushTextWrapPos();
		ImGui::TextUnformatted("Experimental render-pass control. This can block likely fullscreen post-process passes, exact captured passes with viewport/RT matching, and likely corner/UI passes.");
		ImGui::PopTextWrapPos();

		ImGui::Checkbox("Block likely fullscreen passes", &g_blockLikelyFullscreenPasses);
		ImGui::SameLine();
		showHelpMarker("Blocks likely fullscreen post-process passes with very low vertex/index counts and an active pixel shader.");

		ImGui::SliderInt("Max fullscreen pass vertices", &g_fullscreenPassMaxVertices, 3, 128);
		ImGui::SliderInt("Max fullscreen pass indices", &g_fullscreenPassMaxIndices, 3, 256);

		ImGui::Separator();

		ImGui::Checkbox("Enable captured pass blocker", &g_enableCapturedPassBlocker);
		ImGui::SameLine();
		showHelpMarker("Blocks an exact captured pass signature.");

		ImGui::Checkbox("Captured pass must match viewport", &g_matchCapturedPassViewport);
		ImGui::Checkbox("Captured pass must match render target", &g_matchCapturedPassRenderTarget);

		if (ImGui::Button("Capture last seen pass"))
		{
			if (g_lastSeenPass.pixelPipeline != 0 && g_lastSeenPass.pixelPipeline != static_cast<uint64_t>(-1))
			{
				g_hasCapturedPass = true;
				g_capturedPass = g_lastSeenPass;
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear captured pass"))
		{
			g_hasCapturedPass = false;
			g_capturedPass = PassSignature{};
		}

		ImGui::Separator();

		ImGui::Checkbox("Block likely corner/UI passes", &g_blockLikelyCornerUiPasses);
		ImGui::SameLine();
		showHelpMarker("Heuristic blocker for small corner/edge viewports that look like HUD/UI passes.");

		ImGui::SliderFloat("Corner/UI max area ratio", &g_cornerUiMaxAreaRatio, 0.01f, 1.0f);
		ImGui::SliderFloat("Corner/UI max width ratio", &g_cornerUiMaxWidthRatio, 0.05f, 1.0f);
		ImGui::SliderFloat("Corner/UI max height ratio", &g_cornerUiMaxHeightRatio, 0.05f, 1.0f);
		ImGui::SliderFloat("Corner/UI edge tolerance", &g_cornerUiEdgeToleranceRatio, 0.0f, 0.5f);

		ImGui::Checkbox("Show pass debug info", &g_showPassDebugInfo);

		ImGui::Text("Backbuffer size: unavailable in this ReShade API version");
		ImGui::Text("Largest seen viewport: %.1f x %.1f", g_seenMaxViewportWidth, g_seenMaxViewportHeight);

		displayPassSignatureText("Last seen pass", g_lastSeenPass);
		displayPassSignatureText("Captured pass", g_capturedPass);
		displayPassSignatureText("Last blocked pass", g_lastBlockedPass);

		ImGui::Separator();
	}

	if (ImGui::CollapsingHeader("List of Toggle Groups", ImGuiTreeNodeFlags_DefaultOpen))
	{
		if (ImGui::Button(" New "))
		{
			addDefaultGroup();
		}
		ImGui::Separator();

		std::vector<int> idsToRemove;

		for (auto& group : g_toggleGroups)
		{
			ImGui::PushID(group.getId());
			ImGui::AlignTextToFramePadding();

			if (ImGui::Button("X"))
			{
				idsToRemove.push_back(group.getId());
			}

			ImGui::SameLine();
			ImGui::Text(" %d ", group.getId());

			ImGui::SameLine();
			if (ImGui::Button("Edit"))
			{
				group.setEditing(true);
			}

			ImGui::SameLine();
			if (ImGui::Button("Duplicate"))
			{
				g_toggleGroups.push_back(group.makeDuplicate());
				saveShaderTogglerIniFile();
			}

			ImGui::SameLine();
			if (g_toggleGroupIdShaderEditing >= 0)
			{
				if (g_toggleGroupIdShaderEditing == group.getId())
				{
					if (ImGui::Button(" Done "))
					{
						endShaderEditing(true, group);
					}
				}
				else
				{
					ImGui::BeginDisabled(true);
					ImGui::Button("      ");
					ImGui::EndDisabled();
				}
			}
			else
			{
				if (ImGui::Button("Change shaders"))
				{
					startShaderEditing(group);
				}
			}

			ImGui::SameLine();
			ImGui::Text(" %s (%s%s)", group.getName().c_str(), group.getToggleKeyAsString().c_str(), group.isActive() ? ", is active" : "");
			if (group.isActiveAtStartup())
			{
				ImGui::SameLine();
				ImGui::Text(" (Active at startup)");
			}

			if (group.isEditing())
			{
				ImGui::Separator();
				ImGui::Text("Edit group %d", group.getId());

				char tmpBuffer[150] = {};
				strncpy_s(tmpBuffer, sizeof(tmpBuffer), group.getName().c_str(), _TRUNCATE);
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
				ImGui::Text("Name");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
				ImGui::InputText("##Name", tmpBuffer, 149);
				group.setName(tmpBuffer);
				ImGui::PopItemWidth();

				bool isKeyEditing = false;
				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.5f);
				ImGui::Text("Key shortcut");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
				std::string textBoxContents = (g_toggleGroupIdKeyBindingEditing == group.getId()) ? g_keyCollector.getKeyAsString() : group.getToggleKeyAsString();

				char keyBuf[128] = {};
				strncpy_s(keyBuf, sizeof(keyBuf), textBoxContents.c_str(), _TRUNCATE);
				ImGui::InputText("##Key shortcut", keyBuf, sizeof(keyBuf), ImGuiInputTextFlags_ReadOnly);

				if (ImGui::IsItemClicked())
				{
					startKeyBindingEditing(group);
				}
				if (g_toggleGroupIdKeyBindingEditing == group.getId())
				{
					isKeyEditing = true;
					ImGui::SameLine();
					if (ImGui::Button("OK"))
					{
						endKeyBindingEditing(true, group);
					}
					ImGui::SameLine();
					if (ImGui::Button("Cancel"))
					{
						endKeyBindingEditing(false, group);
					}
				}
				ImGui::PopItemWidth();

				ImGui::PushItemWidth(ImGui::GetWindowWidth() * 0.7f);
				ImGui::Text(" ");
				ImGui::SameLine(ImGui::GetWindowWidth() * 0.25f);
				bool isDefaultActive = group.isActiveAtStartup();
				ImGui::Checkbox("Is active at startup", &isDefaultActive);
				group.setIsActiveAtStartup(isDefaultActive);
				ImGui::PopItemWidth();

				if (!isKeyEditing)
				{
					if (ImGui::Button("OK"))
					{
						group.setEditing(false);
						g_toggleGroupIdKeyBindingEditing = -1;
						g_keyCollector.clear();
						saveShaderTogglerIniFile();
					}
				}
				ImGui::Separator();
			}

			ImGui::PopID();
		}

		if (!idsToRemove.empty())
		{
			g_toggleGroupIdKeyBindingEditing = -1;
			g_keyCollector.clear();
			g_toggleGroupIdShaderEditing = -1;
			g_pixelShaderManager.stopHuntingMode();
			g_vertexShaderManager.stopHuntingMode();
			g_computeShaderManager.stopHuntingMode();

			g_toggleGroups.erase(
				std::remove_if(g_toggleGroups.begin(), g_toggleGroups.end(),
					[&](const ToggleGroup& g)
					{
						return std::find(idsToRemove.begin(), idsToRemove.end(), g.getId()) != idsToRemove.end();
					}),
				g_toggleGroups.end());

			for (int id : idsToRemove)
			{
				g_groupHotkeyWasDown.erase(id);
				g_groupHotkeyLastToggleTime.erase(id);
			}

			saveShaderTogglerIniFile();
		}

		ImGui::Separator();
		if (!g_toggleGroups.empty())
		{
			if (ImGui::Button("Save all Toggle Groups"))
			{
				saveShaderTogglerIniFile();
			}
		}
	}

	if (ImGui::CollapsingHeader("Group Order", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::TextUnformatted("Drag and drop to reorder toggle groups");
		ImGui::Separator();

		float computedHeight = 45.0f * static_cast<float>(g_toggleGroups.size()) + 20.0f;
		float childHeight = (computedHeight < 600.0f) ? computedHeight : 600.0f;
		ImGui::BeginChild("##grouporder_inline", ImVec2(0, childHeight), true, ImGuiWindowFlags_HorizontalScrollbar);

		for (int i = 0; i < static_cast<int>(g_toggleGroups.size()); ++i)
		{
			ImGui::PushID(i);
			const std::string &name = g_toggleGroups[i].getName();
			ImGui::Selectable(name.c_str(), false);

			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				ImGui::SetDragDropPayload("ST_GROUP_INDEX", &i, sizeof(int));
				ImGui::Text("Move: %s", name.c_str());
				ImGui::EndDragDropSource();
			}

			if (ImGui::BeginDragDropTarget())
			{
				if (const ImGuiPayload *payload = ImGui::AcceptDragDropPayload("ST_GROUP_INDEX"))
				{
					int src = *(const int*)payload->Data;
					if (src != i)
					{
						auto tmp = g_toggleGroups[src];
						g_toggleGroups.erase(g_toggleGroups.begin() + src);
						if (src < i) i--;
						g_toggleGroups.insert(g_toggleGroups.begin() + i, std::move(tmp));
						saveShaderTogglerIniFile();
					}
				}
				ImGui::EndDragDropTarget();
			}
			ImGui::PopID();
		}

		ImGui::EndChild();
	}
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD fdwReason, LPVOID)
{
	switch (fdwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		if (!reshade::register_addon(hModule))
		{
			return FALSE;
		}

		WCHAR buf[MAX_PATH];
		const std::filesystem::path dllPath = GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf)) ? buf : std::filesystem::path();
		const std::filesystem::path basePath = dllPath.parent_path();
		g_iniFileName = (basePath / HASH_FILE_NAME).string();

		reshade::register_event<reshade::addon_event::init_pipeline>(onInitPipeline);
		reshade::register_event<reshade::addon_event::init_command_list>(onInitCommandList);
		reshade::register_event<reshade::addon_event::destroy_command_list>(onDestroyCommandList);
		reshade::register_event<reshade::addon_event::reset_command_list>(onResetCommandList);
		reshade::register_event<reshade::addon_event::destroy_pipeline>(onDestroyPipeline);
		reshade::register_event<reshade::addon_event::bind_pipeline>(onBindPipeline);
		reshade::register_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(onBindRenderTargetsAndDepthStencil);
		reshade::register_event<reshade::addon_event::bind_viewports>(onBindViewports);
		reshade::register_event<reshade::addon_event::reshade_overlay>(onReshadeOverlay);
		reshade::register_event<reshade::addon_event::reshade_present>(onReshadePresent);
		reshade::register_event<reshade::addon_event::draw>(onDraw);
		reshade::register_event<reshade::addon_event::draw_indexed>(onDrawIndexed);
		reshade::register_event<reshade::addon_event::draw_or_dispatch_indirect>(onDrawOrDispatchIndirect);
		reshade::register_overlay(nullptr, &displaySettings);

		loadShaderTogglerIniFile();
	}
	break;

	case DLL_PROCESS_DETACH:
		reshade::unregister_event<reshade::addon_event::reshade_present>(onReshadePresent);
		reshade::unregister_event<reshade::addon_event::destroy_pipeline>(onDestroyPipeline);
		reshade::unregister_event<reshade::addon_event::init_pipeline>(onInitPipeline);
		reshade::unregister_event<reshade::addon_event::bind_pipeline>(onBindPipeline);
		reshade::unregister_event<reshade::addon_event::bind_render_targets_and_depth_stencil>(onBindRenderTargetsAndDepthStencil);
		reshade::unregister_event<reshade::addon_event::bind_viewports>(onBindViewports);
		reshade::unregister_event<reshade::addon_event::reshade_overlay>(onReshadeOverlay);
		reshade::unregister_event<reshade::addon_event::draw>(onDraw);
		reshade::unregister_event<reshade::addon_event::draw_indexed>(onDrawIndexed);
		reshade::unregister_event<reshade::addon_event::draw_or_dispatch_indirect>(onDrawOrDispatchIndirect);
		reshade::unregister_event<reshade::addon_event::init_command_list>(onInitCommandList);
		reshade::unregister_event<reshade::addon_event::destroy_command_list>(onDestroyCommandList);
		reshade::unregister_event<reshade::addon_event::reset_command_list>(onResetCommandList);
		reshade::unregister_overlay(nullptr, &displaySettings);
		reshade::unregister_addon(hModule);
		break;
	}

	return TRUE;
}