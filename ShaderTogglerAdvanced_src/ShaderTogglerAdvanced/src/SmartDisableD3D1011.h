// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include "SmartDisableD3D9.h"
// The Windows SDK requires the 10.1 header first when d3dcompiler.h is used.
// It also includes the DirectX 10 interfaces needed below.
#include <d3d10_1.h>
#include <d3d11.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <type_traits>

namespace ShaderToggler::smart::modern
{
    using reshade::api::device;
    using reshade::api::device_api;
    using reshade::api::command_list;

    template<class T> struct Ref
    {
        T* pointer = nullptr;
        Ref() = default;
        Ref(const Ref&) = delete;
        Ref& operator=(const Ref&) = delete;
        ~Ref() { if (pointer) pointer->Release(); }
        T* operator->() const { return pointer; }
    };
    struct ShaderInfo
    {
        std::vector<uint8_t> code;
        bool checked = false;
        bool compatible = false;
    };
    struct Variant
    {
        ID3D10PixelShader* shader10 = nullptr;
        ID3D11PixelShader* shader11 = nullptr;
    };
    struct __declspec(uuid("81999426-476F-43FC-9D05-3B8AF4D6C81E")) DeviceData
    {
        std::recursive_mutex mutex;
        std::map<uint64_t, ShaderInfo> shaders;
        std::map<std::array<uint32_t, 4>, Variant> variants;
        std::map<uint32_t, Method> suggestions;
        std::map<uint32_t, Status> statuses;
        size_t codeBytes = 0;
        bool compilerTried = false;
        HMODULE compiler = nullptr;
        decltype(&D3DCompile) compile = nullptr;
        decltype(&D3DReflect) reflect = nullptr;
        ~DeviceData()
        {
            for (auto& [key, variant] : variants)
            {
                if (variant.shader10) variant.shader10->Release();
                if (variant.shader11) variant.shader11->Release();
            }
            if (compiler) FreeLibrary(compiler);
        }
    };
    inline bool supported(device* dev)
    {
        return dev && (dev->get_api() == device_api::d3d10 || dev->get_api() == device_api::d3d11);
    }
    inline ID3D10Device* native10(device* dev)
    {
        return reinterpret_cast<ID3D10Device*>(static_cast<uintptr_t>(dev->get_native()));
    }
    inline ID3D11Device* native11(device* dev)
    {
        return reinterpret_cast<ID3D11Device*>(static_cast<uintptr_t>(dev->get_native()));
    }
    inline ID3D11DeviceContext* context11(command_list* commands)
    {
        return reinterpret_cast<ID3D11DeviceContext*>(static_cast<uintptr_t>(commands->get_native()));
    }
    inline void initDevice(device* dev)
    {
        if (!supported(dev)) return;
        dev->create_private_data<DeviceData>();
        diagnostics::Write("[SMART] DirectX %u scoped pixel replacement available.", dev->get_api() == device_api::d3d10 ? 10u : 11u);
    }
    inline void destroyDevice(device* dev)
    {
        if (!supported(dev)) return;
        dev->destroy_private_data<DeviceData>();
        diagnostics::Write("[SMART] Released DirectX 10/11 replacement cache.");
    }
    inline void capture(device* dev, uint64_t handle, const reshade::api::shader_desc& desc)
    {
        if (!supported(dev) || !handle) return;
        auto& state = dev->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        if (auto old = state.shaders.find(handle); old != state.shaders.end())
        {
            state.codeBytes -= old->second.code.size();
            state.shaders.erase(old);
        }
        if (!desc.code || !desc.code_size || desc.code_size > 4 * 1024 * 1024 ||
            state.codeBytes + desc.code_size > 64 * 1024 * 1024 || state.shaders.size() >= 65536) return;
        ShaderInfo info;
        const auto* bytes = static_cast<const uint8_t*>(desc.code);
        info.code.assign(bytes, bytes + desc.code_size);
        state.codeBytes += info.code.size();
        state.shaders.emplace(handle, std::move(info));
    }
    inline void forget(device* dev, uint64_t handle)
    {
        if (!supported(dev)) return;
        auto& state = dev->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        if (auto found = state.shaders.find(handle); found != state.shaders.end())
        {
            state.codeBytes -= found->second.code.size();
            state.shaders.erase(found);
        }
    }
    inline bool loadCompiler(DeviceData& state)
    {
        if (!state.compilerTried)
        {
            state.compilerTried = true;
            state.compiler = LoadLibraryExW(L"d3dcompiler_47.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (state.compiler)
            {
                state.compile = reinterpret_cast<decltype(state.compile)>(GetProcAddress(state.compiler, "D3DCompile"));
                state.reflect = reinterpret_cast<decltype(state.reflect)>(GetProcAddress(state.compiler, "D3DReflect"));
            }
            if (!state.compile || !state.reflect)
                diagnostics::Write("[ERROR] SMART DirectX 10/11 system d3dcompiler_47.dll is unavailable.");
        }
        return state.compile && state.reflect;
    }
    inline bool compatible(ID3D11ShaderReflection* reflection)
    {
        D3D11_SHADER_DESC desc{};
        if (FAILED(reflection->GetDesc(&desc)) || (desc.Version >> 16) != 0 ||
            desc.OutputParameters != 1) return false;
        D3D11_SIGNATURE_PARAMETER_DESC output{};
        if (FAILED(reflection->GetOutputParameterDesc(0, &output)) || output.SystemValueType != D3D_NAME_TARGET ||
            output.SemanticIndex != 0 || output.Register != 0 || output.ComponentType != D3D_REGISTER_COMPONENT_FLOAT32) return false;
        for (UINT i = 0; i < desc.BoundResources; ++i)
        {
            D3D11_SHADER_INPUT_BIND_DESC binding{};
            if (FAILED(reflection->GetResourceBindingDesc(i, &binding))) return false;
            switch (binding.Type)
            {
            case D3D_SIT_CBUFFER: case D3D_SIT_TBUFFER: case D3D_SIT_TEXTURE:
            case D3D_SIT_SAMPLER: case D3D_SIT_STRUCTURED: case D3D_SIT_BYTEADDRESS: break;
            default: return false; 
            }
        }
        return true;
    }
    inline bool checkShader(DeviceData& state, ShaderInfo& info)
    {
        if (!info.checked)
        {
            info.checked = true;
            Ref<ID3D11ShaderReflection> reflection;
            if (SUCCEEDED(state.reflect(info.code.data(), info.code.size(), __uuidof(ID3D11ShaderReflection),
                reinterpret_cast<void**>(&reflection.pointer))) && reflection.pointer)
                info.compatible = compatible(reflection.pointer);
            state.codeBytes -= info.code.size();
            std::vector<uint8_t>().swap(info.code);
        }
        return info.compatible;
    }
    inline std::array<uint32_t, 4> colourKey(const Choice& choice)
    {
        std::array<uint32_t, 4> key{};
        for (size_t i = 0; i < key.size(); ++i) std::memcpy(&key[i], &choice.rgba[i], sizeof(float));
        return key;
    }
    inline std::string shaderSource(const Choice& choice)
    {
        const auto key = colourKey(choice);
        return "float4 main() : SV_Target0 { return asfloat(uint4(" + std::to_string(key[0]) + "u," +
            std::to_string(key[1]) + "u," + std::to_string(key[2]) + "u," + std::to_string(key[3]) + "u)); }";
    }
    inline Variant* replacement(device* dev, DeviceData& state, const Choice& choice)
    {
        const auto key = colourKey(choice);
        if (auto found = state.variants.find(key); found != state.variants.end()) return &found->second;
        if (state.variants.size() >= 512) return nullptr;
        auto& variant = state.variants[key];
        const auto source = shaderSource(choice);
        Ref<ID3DBlob> code, errors;
        HRESULT result = state.compile(source.data(), source.size(), "ShaderTogglerAdvanced", nullptr, nullptr,
            "main", "ps_4_0", D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &code.pointer, &errors.pointer);
        if (SUCCEEDED(result) && code.pointer)
        {
            if (dev->get_api() == device_api::d3d10)
                result = native10(dev)->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), &variant.shader10);
            else
                result = native11(dev)->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &variant.shader11);
        }
        if (FAILED(result))
        {
            if (variant.shader10) { variant.shader10->Release(); variant.shader10 = nullptr; }
            if (variant.shader11) { variant.shader11->Release(); variant.shader11 = nullptr; }
            diagnostics::Write("[ERROR] SMART DirectX 10/11 replacement creation failed HRESULT=%08lX", static_cast<unsigned long>(result));
        }
        return &variant;
    }
    template<class T> inline bool singleTarget(T* context)
    {
        using View = std::conditional_t<std::is_same_v<T, ID3D10Device>, ID3D10RenderTargetView, ID3D11RenderTargetView>;
        View* targets[8]{};
        context->OMGetRenderTargets(8, targets, nullptr);
        bool single = targets[0] != nullptr;
        for (size_t i = 0; i < 8; ++i)
        {
            if (i && targets[i]) single = false;
            if (targets[i]) targets[i]->Release();
        }
        return single;
    }
    inline Method blendSuggestion(bool enabled, UINT source, UINT destination, UINT operation)
    {
        if (enabled && operation == D3D11_BLEND_OP_ADD)
        {
            if ((source == D3D11_BLEND_DEST_COLOR && destination == D3D11_BLEND_ZERO) ||
                (source == D3D11_BLEND_ZERO && destination == D3D11_BLEND_SRC_COLOR)) return Method::White;
            if (source == D3D11_BLEND_SRC_ALPHA && destination == D3D11_BLEND_INV_SRC_ALPHA) return Method::OpaqueBlack;
        }
        return Method::TransparentBlack;
    }
    inline void observe(command_list* commands, uint32_t hash)
    {
        auto* dev = commands->get_device();
        if (!supported(dev) || !hash) return;
        auto& state = dev->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        if (state.suggestions.count(hash) || state.suggestions.size() >= 4096) return;
        Method result = Method::TransparentBlack;
        if (dev->get_api() == device_api::d3d10)
        {
            Ref<ID3D10BlendState> blend;
            native10(dev)->OMGetBlendState(&blend.pointer, nullptr, nullptr);
            if (blend.pointer)
            {
                D3D10_BLEND_DESC desc{}; blend->GetDesc(&desc);
                result = blendSuggestion(desc.BlendEnable[0], desc.SrcBlend, desc.DestBlend, desc.BlendOp);
            }
        }
        else
        {
            auto* context = context11(commands);
            if (context->GetType() != D3D11_DEVICE_CONTEXT_IMMEDIATE) return;
            Ref<ID3D11BlendState> blend;
            context->OMGetBlendState(&blend.pointer, nullptr, nullptr);
            if (blend.pointer)
            {
                D3D11_BLEND_DESC desc{}; blend->GetDesc(&desc);
                const auto& target = desc.RenderTarget[0];
                result = blendSuggestion(target.BlendEnable, target.SrcBlend, target.DestBlend, target.BlendOp);
            }
        }
        state.suggestions[hash] = result;
    }
    inline Method suggestion(device* dev, uint32_t hash)
    {
        auto& state = dev->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        const auto found = state.suggestions.find(hash);
        return found == state.suggestions.end() ? Method::TransparentBlack : found->second;
    }
    inline Status status(device* dev, uint32_t hash)
    {
        auto& state = dev->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        const auto found = state.statuses.find(hash);
        return found == state.statuses.end() ? Status{} : found->second;
    }
    inline void report(device* dev, DeviceData& state, uint32_t hash, Choice choice, bool applied, const char* reason)
    {
        if (!state.statuses.count(hash) && state.statuses.size() >= 4096) return;
        auto& previous = state.statuses[hash];
        if (!previous.attempted || previous.choice != choice || previous.applied != applied || previous.reason != reason)
            diagnostics::Write("[SMART] DirectX %u shader=%08X method=%s applied=%u: %s",
                dev->get_api() == device_api::d3d10 ? 10u : 11u, hash, name(choice.method), applied, reason);
        previous = {choice, true, applied, reason};
    }
    struct ScopedShader10
    {
        ID3D10Device* context;
        Ref<ID3D10PixelShader> original;
        bool bound = false;
        explicit ScopedShader10(ID3D10Device* value) : context(value) { context->PSGetShader(&original.pointer); }
        ~ScopedShader10() { if (bound) context->PSSetShader(original.pointer); }
    };
    struct ScopedShader11
    {
        ID3D11DeviceContext* context;
        Ref<ID3D11PixelShader> original;
        ID3D11ClassInstance* classes[256]{};
        UINT count = 256;
        bool bound = false;
        explicit ScopedShader11(ID3D11DeviceContext* value) : context(value)
        { context->PSGetShader(&original.pointer, classes, &count); }
        ~ScopedShader11()
        {
            if (bound) context->PSSetShader(original.pointer, classes, count);
            for (auto* instance : classes) if (instance) instance->Release();
        }
    };
    template<class Draw> inline bool draw(command_list* commands, uint64_t expected, uint32_t hash, Choice choice, Draw&& issueDraw)
    {
        auto* dev = commands->get_device();
        if (!supported(dev) || !valid(choice) || choice.method == Method::Skip) return false;
        auto& state = dev->get_private_data<DeviceData>();
        std::lock_guard lock(state.mutex);
        const auto fail = [&](const char* reason) { report(dev, state, hash, choice, false, reason); return false; };
        if (dev->get_api() == device_api::d3d11)
        {
            if (context11(commands)->GetType() != D3D11_DEVICE_CONTEXT_IMMEDIATE)
                return fail("This pass uses a deferred context; colour replacement is unavailable. Original pass retained.");
            if (native11(dev)->GetFeatureLevel() < D3D_FEATURE_LEVEL_10_0)
                return fail("This device uses a DirectX 9 feature level; original pass retained.");
        }
        if (!loadCompiler(state)) return fail("System shader compiler is unavailable; original pass retained. See ShaderTogglerAdvanced.log.");
        auto found = state.shaders.find(expected);
        if (found == state.shaders.end()) return fail("Shader bytecode was not captured or the cache is full; original pass retained.");
        if (!checkShader(state, found->second))
            return fail("This shader has extra/depth/integer outputs, writable resources or unreadable metadata; original pass retained.");
        if (dev->get_api() == device_api::d3d10)
        {
            auto* context = native10(dev);
            ScopedShader10 scope(context);
            if (!scope.original.pointer || reinterpret_cast<uintptr_t>(scope.original.pointer) != expected)
                return fail("The game changed shader state; original pass retained.");
            if (!singleTarget(context)) return fail("This pass needs a single colour target; original pass retained.");
            auto* variant = replacement(dev, state, choice);
            if (!variant || !variant->shader10) return fail("Replacement creation failed or cache is full; original pass retained. See ShaderTogglerAdvanced.log.");
            scope.bound = true;
            context->PSSetShader(variant->shader10);
            issueDraw();
        }
        else
        {
            auto* context = context11(commands);
            ScopedShader11 scope(context);
            if (!scope.original.pointer || reinterpret_cast<uintptr_t>(scope.original.pointer) != expected || scope.count > 256)
                return fail("The game changed shader state; original pass retained.");
            if (!singleTarget(context)) return fail("This pass needs a single colour target; original pass retained.");
            auto* variant = replacement(dev, state, choice);
            if (!variant || !variant->shader11) return fail("Replacement creation failed or cache is full; original pass retained. See ShaderTogglerAdvanced.log.");
            scope.bound = true;
            context->PSSetShader(variant->shader11, nullptr, 0);
            issueDraw();
        }
        report(dev, state, hash, choice, true, "Replacement is running.");
        return true;
    }
}
