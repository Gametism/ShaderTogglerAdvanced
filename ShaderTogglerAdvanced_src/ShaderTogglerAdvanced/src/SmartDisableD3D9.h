// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include "SmartDisable.h"
#include "DiagnosticLog.h"
#include <reshade_api_device.hpp>
#include <d3d9.h>
#include <unordered_map>
#include <vector>

namespace ShaderToggler::smart
{
    struct Status
    {
        Choice choice;
        bool attempted = false;
        bool applied = false;
        const char* reason = "Waiting for this shader to draw.";
    };
    struct ShaderEntry
    {
        IDirect3DPixelShader9* original = nullptr;
        DWORD version = 0;
        bool compatible = false;
        std::map<std::array<DWORD, 5>, IDirect3DPixelShader9*> variants;
    };
    struct __declspec(uuid("8E908DFA-EE6D-45AB-863D-B0DCD745ED19")) DeviceData
    {
        std::recursive_mutex mutex;
        std::map<IDirect3DPixelShader9*, ShaderEntry> shaders;
        std::unordered_map<IDirect3DPixelShader9*, IDirect3DPixelShader9*> originals;
        std::map<uint32_t, Method> suggestions;
        std::map<uint32_t, Status> statuses;
        DWORD renderTargets = 1;
        size_t variantCount = 0;
        bool restoreFailure = false;
        ~DeviceData()
        {
            for (auto& [pointer, entry] : shaders)
            {
                for (auto& [key, shader] : entry.variants) if (shader) shader->Release();
                if (entry.original) entry.original->Release();
            }
        }
    };
    inline IDirect3DDevice9* native(reshade::api::device* device)
    {
        return device && device->get_api() == reshade::api::device_api::d3d9
            ? reinterpret_cast<IDirect3DDevice9*>(static_cast<uintptr_t>(device->get_native())) : nullptr;
    }
    inline void initDevice(reshade::api::device* device)
    {
        if (auto* d3d = native(device))
        {
            auto& state = device->create_private_data<DeviceData>();
            D3DCAPS9 caps{};
            state.renderTargets = SUCCEEDED(d3d->GetDeviceCaps(&caps)) ? caps.NumSimultaneousRTs : 4;
            diagnostics::Write("[SMART] Native DirectX 9 replacement available; render_targets=%lu", state.renderTargets);
        }
    }

    inline IDirect3DPixelShader9* originalOf(DeviceData& state, IDirect3DPixelShader9* shader)
    {
        const auto found = state.originals.find(shader);
        return found == state.originals.end() ? shader : found->second;
    }
    inline void restore(reshade::api::device* device)
    {
        auto* d3d = native(device);
        if (!d3d) return;
        auto& state = device->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        if (state.originals.empty()) return;
        IDirect3DPixelShader9* current = nullptr;
        if (SUCCEEDED(d3d->GetPixelShader(&current)) && current)
        {
            auto* original = originalOf(state, current);
            if (original != current)
            {
                const bool failed = FAILED(d3d->SetPixelShader(original));
                if (failed && !state.restoreFailure)
                    diagnostics::Write("[ERROR] SMART could not restore the original pixel shader.");
                state.restoreFailure = failed;
            }
            current->Release();
        }
    }
    inline void destroyDevice(reshade::api::device* device)
    {
        if (!native(device)) return;
        restore(device);
        device->destroy_private_data<DeviceData>();
        diagnostics::Write("[SMART] Released replacement cache for device destruction/reset.");
    }
    inline void present(reshade::api::command_queue* queue, reshade::api::swapchain*,
        const reshade::api::rect*, const reshade::api::rect*, uint32_t, const reshade::api::rect*)
    {
        if (queue) restore(queue->get_device());
    }
    inline uint64_t pixelHandle(reshade::api::command_list* commands, uint64_t tracked)
    {
        auto* device = commands->get_device();
        auto* d3d = native(device);
        if (!d3d) return tracked;
        auto& state = device->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        if (state.originals.empty()) return tracked;
        IDirect3DPixelShader9* current = nullptr;
        if (FAILED(d3d->GetPixelShader(&current))) return 0;
        const auto result = reinterpret_cast<uintptr_t>(originalOf(state, current));
        if (current) current->Release();
        return result;
    }
    inline void observe(reshade::api::device* device, uint32_t hash)
    {
        auto* d3d = native(device);
        if (!d3d || !hash) return;
        auto& state = device->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        if (state.suggestions.count(hash) || state.suggestions.size() >= 4096) return;
        DWORD enabled = 0, source = 0, destination = 0, operation = 0;
        Method suggested = Method::TransparentBlack;
        if (SUCCEEDED(d3d->GetRenderState(D3DRS_ALPHABLENDENABLE, &enabled)) && enabled &&
            SUCCEEDED(d3d->GetRenderState(D3DRS_SRCBLEND, &source)) &&
            SUCCEEDED(d3d->GetRenderState(D3DRS_DESTBLEND, &destination)) &&
            SUCCEEDED(d3d->GetRenderState(D3DRS_BLENDOP, &operation)) && operation == D3DBLENDOP_ADD)
        {
            if ((source == D3DBLEND_DESTCOLOR && destination == D3DBLEND_ZERO) ||
                (source == D3DBLEND_ZERO && destination == D3DBLEND_SRCCOLOR)) suggested = Method::White;
            else if (source == D3DBLEND_SRCALPHA && destination == D3DBLEND_INVSRCALPHA) suggested = Method::OpaqueBlack;
        }
        state.suggestions[hash] = suggested;
    }
    inline Method suggestion(reshade::api::device* device, uint32_t hash)
    {
        if (!native(device)) return Method::TransparentBlack;
        auto& state = device->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        const auto found = state.suggestions.find(hash);
        return found == state.suggestions.end() ? Method::TransparentBlack : found->second;
    }
    inline Status status(reshade::api::device* device, uint32_t hash)
    {
        if (!native(device)) return {};
        auto& state = device->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        const auto found = state.statuses.find(hash);
        return found == state.statuses.end() ? Status{} : found->second;
    }
    inline void report(DeviceData& state, uint32_t hash, Choice choice, bool applied, const char* reason)
    {
        if (!state.statuses.count(hash) && state.statuses.size() >= 4096) return;
        auto& previous = state.statuses[hash];
        // One report per transition, never an ordinary per-draw log.
        if (!previous.attempted || previous.choice != choice || previous.applied != applied || previous.reason != reason)
            diagnostics::Write("[SMART] shader=%08X method=%s applied=%u: %s", hash, name(choice.method), applied, reason);
        previous = {choice, true, applied, reason};
    }
    inline bool compatibleShader(const std::vector<DWORD>& code)
    {
        if (code.size() < 2 || (code[0] != D3DPS_VERSION(2, 0) && code[0] != D3DPS_VERSION(3, 0))) return false;
        bool colour0 = false;
        for (size_t i = 1; i < code.size();)
        {
            const DWORD opcode = code[i] & D3DSI_OPCODE_MASK;
            if (opcode == D3DSIO_END) return colour0 && i + 1 == code.size();
            if (opcode == D3DSIO_COMMENT)
            {
                const size_t length = (code[i] & D3DSI_COMMENTSIZE_MASK) >> D3DSI_COMMENTSIZE_SHIFT;
                if (length >= code.size() - i) return false;
                i += 1 + length;
                continue;
            }
            const size_t length = (code[i] & D3DSI_INSTLENGTH_MASK) >> D3DSI_INSTLENGTH_SHIFT;
            if (length >= code.size() - i) return false;
            // DEF/DEFI/DEFB contain raw immediate bits, not register tokens.
            if (opcode != D3DSIO_DEF && opcode != D3DSIO_DEFI && opcode != D3DSIO_DEFB)
            {
                for (size_t j = 1; j <= length; ++j)
                {
                    const DWORD token = code[i + j];
                    if (!(token & 0x80000000u)) continue;
                    const DWORD type = ((token & D3DSP_REGTYPE_MASK) >> D3DSP_REGTYPE_SHIFT) |
                        ((token & D3DSP_REGTYPE_MASK2) >> D3DSP_REGTYPE_SHIFT2);
                    if (type == D3DSPR_DEPTHOUT) return false;
                    if (type == D3DSPR_COLOROUT)
                    {
                        if ((token & D3DSP_REGNUM_MASK) != 0) return false;
                        colour0 = true;
                    }
                }
            }
            i += 1 + length;
        }
        return false;
    }
    inline std::array<DWORD, 11> constantShader(DWORD version, const Choice& choice)
    {
        std::array<DWORD, 11> code = {version, 0x05000051u, 0xA00F0000u,
            0, 0, 0, 0, 0x02000001u, 0x800F0800u, 0xA0E40000u, 0x0000FFFFu};
        for (size_t i = 0; i < 4; ++i) std::memcpy(&code[3 + i], &choice.rgba[i], sizeof(float));
        return code;
    }
    inline bool apply(reshade::api::device* device, uint64_t expectedHandle, uint32_t hash, Choice choice)
    {
        auto* d3d = native(device);
        if (!d3d || !valid(choice)) return false;
        auto& state = device->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        IDirect3DPixelShader9* current = nullptr;
        if (FAILED(d3d->GetPixelShader(&current)) || !current)
        {
            report(state, hash, choice, false, "Could not read the current pixel shader; original pass retained.");
            return false;
        }
        auto* original = originalOf(state, current);
        const auto fail = [&](const char* reason)
        {
            if (original != current && FAILED(d3d->SetPixelShader(original)))
                reason = "The device could not restore the original shader. See ShaderTogglerAdvanced.log.";
            current->Release();
            report(state, hash, choice, false, reason);
            return false;
        };
        if (reinterpret_cast<uintptr_t>(original) != expectedHandle)
            return fail("The game changed shader state; original pass retained.");
        for (DWORD slot = 1; slot < state.renderTargets; ++slot)
        {
            IDirect3DSurface9* target = nullptr;
            const HRESULT result = d3d->GetRenderTarget(slot, &target);
            if (target) { target->Release(); return fail("Multiple render targets: replacement unavailable; original pass retained."); }
            if (FAILED(result) && result != D3DERR_NOTFOUND)
                return fail("Render targets could not be checked; original pass retained.");
        }
        auto found = state.shaders.find(original);
        if (found == state.shaders.end())
        {
            if (state.shaders.size() >= 512) return fail("Replacement cache is full; original pass retained.");
            UINT bytes = 0;
            if (FAILED(original->GetFunction(nullptr, &bytes)) || bytes < 8 || bytes > 1024 * 1024 || bytes % 4)
                return fail("Shader bytecode is unavailable; original pass retained.");
            std::vector<DWORD> code(bytes / 4);
            if (FAILED(original->GetFunction(code.data(), &bytes)))
                return fail("Shader bytecode could not be read; original pass retained.");
            ShaderEntry entry;
            entry.original = original; original->AddRef();
            entry.version = code[0];
            entry.compatible = compatibleShader(code);
            found = state.shaders.emplace(original, std::move(entry)).first;
        }
        auto& entry = found->second;
        if (!entry.compatible)
            return fail("This shader writes depth/extra outputs or uses an unsupported format; original pass retained.");
        std::array<DWORD, 5> key = {static_cast<DWORD>(choice.method), 0, 0, 0, 0};
        for (size_t i = 0; i < 4; ++i) std::memcpy(&key[i + 1], &choice.rgba[i], sizeof(float));
        auto variant = entry.variants.find(key);
        if (variant == entry.variants.end())
        {
            if (state.variantCount >= 512) return fail("Replacement cache is full; original pass retained.");
            IDirect3DPixelShader9* replacement = nullptr;
            const auto code = constantShader(entry.version, choice);
            ++state.variantCount;
            const HRESULT result = d3d->CreatePixelShader(code.data(), &replacement);
            // Remember failed creations too, to avoid retrying every draw.
            variant = entry.variants.emplace(key, replacement).first;
            if (SUCCEEDED(result) && replacement) state.originals[replacement] = original;
            else
            {
                if (replacement) replacement->Release();
                variant->second = nullptr;
                diagnostics::Write("[ERROR] SMART CreatePixelShader failed shader=%08X HRESULT=%08lX", hash, static_cast<unsigned long>(result));
            }
        }
        if (!variant->second) return fail("Replacement creation failed; original pass retained. See ShaderTogglerAdvanced.log.");
        if (current != variant->second && FAILED(d3d->SetPixelShader(variant->second)))
            return fail("Replacement binding failed; original pass retained. See ShaderTogglerAdvanced.log.");
        current->Release();
        report(state, hash, choice, true, "Replacement is running.");
        return true;
    }
}
