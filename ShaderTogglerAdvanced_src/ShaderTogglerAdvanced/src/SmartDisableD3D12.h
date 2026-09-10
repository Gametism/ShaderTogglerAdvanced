// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include "SmartDisableD3D1011.h"
#include <d3d12.h>
#include <d3d12shader.h>
#include "Vendor/DXC/dxcapi.h"
#include <memory>
#include <algorithm>
#include <set>

namespace ShaderToggler::smart::dx12
{
    using reshade::api::device;
    using reshade::api::command_list;
    using modern::Ref;
    struct Budget { size_t bytes = 0; };
    struct Code
    {
        std::vector<uint8_t> bytes;
        std::shared_ptr<Budget> budget;
        ~Code() { if (budget) budget->bytes -= bytes.size(); }
    };
    struct Pipeline
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
        std::array<std::shared_ptr<Code>, 5> code;
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputs;
        std::vector<std::string> semantics;
        bool checked = false, compatible = false, dxil = false;
        // One nibble per SV_Target slot: channels actually written by the shader.
        uint32_t outputChannels = 0;
        uint64_t drawKey = 0;
        std::map<std::array<uint32_t, 4>, ID3D12PipelineState*> variants;
        ~Pipeline() { if (desc.pRootSignature) desc.pRootSignature->Release(); }
    };
    struct State
    {
        std::recursive_mutex mutex;
        std::shared_ptr<Budget> budget = std::make_shared<Budget>();
        std::map<uint64_t, std::weak_ptr<Code>> codePool;
        std::map<uint64_t, std::shared_ptr<Pipeline>> pipelines;
        // Keep recorded replacements alive until device destruction, even after
        // their source PSO is destroyed. Command lists do not own these references.
        std::vector<ID3D12PipelineState*> retained;
        std::map<std::array<uint32_t, 6>, std::vector<uint8_t>> colours;
        std::set<std::array<uint32_t, 8>> targetReports;
        std::map<uint32_t, Status> statuses;
        std::map<uint32_t, Method> suggestions;
        modern::DeviceData compiler;
        HMODULE dxcModule = nullptr;
        bool dxcTried = false;
        Ref<IDxcUtils> utils;
        Ref<IDxcCompiler3> dxc;
        size_t captured = 0, declined = 0;
        ~State()
        {
            for (auto* pso : retained) if (pso) pso->Release();
            // Release compiler objects before unloading their implementation.
            if (dxc.pointer) { dxc.pointer->Release(); dxc.pointer = nullptr; }
            if (utils.pointer) { utils.pointer->Release(); utils.pointer = nullptr; }
            if (dxcModule) FreeLibrary(dxcModule);
        }
    };
    struct __declspec(uuid("DDFCC025-754B-428C-A17E-053CC198583C")) DeviceData { std::shared_ptr<State> state; };
    struct __declspec(uuid("0C4EED70-73B4-42F5-BF56-E73B26D93293")) CommandData
    {
        uint64_t current = 0;
        uint64_t restore = 0;
        enum class Targets : uint32_t { Unknown, Bound, Invalid, Ended } targets = Targets::Unknown;
        uint32_t targetCount = 0, targetMask = 0;
        bool nativeIndirectReady = false;
    };
    inline thread_local bool creatingReplacement = false;
    inline bool supported(device* dev) { return dev && dev->get_api() == reshade::api::device_api::d3d12; }
    inline ID3D12Device* native(device* dev) { return reinterpret_cast<ID3D12Device*>(static_cast<uintptr_t>(dev->get_native())); }
    inline ID3D12GraphicsCommandList* native(command_list* cmd) { return reinterpret_cast<ID3D12GraphicsCommandList*>(static_cast<uintptr_t>(cmd->get_native())); }
    inline State& state(device* dev) { return *dev->get_private_data<DeviceData>().state; }
    void installCapture(device* dev, const std::shared_ptr<State>& value);
    void removeCapture(device* dev);
    void shutdownCapture(bool processExit);
    void initQueue(reshade::api::command_queue* queue);
    void destroyQueue(reshade::api::command_queue* queue);
    void registerCommands(command_list* cmd);
    void unregisterCommands(command_list* cmd);
    void setIndirectCallback(bool (*callback)(command_list*,bool));
    void setFinderIndirectCallback(bool (*callback)(command_list*,uint32_t,uint32_t,uint32_t,bool));
    inline bool canHandleIndirect(command_list* cmd)
    { return cmd && supported(cmd->get_device()) && cmd->get_private_data<CommandData>().nativeIndirectReady; }
    inline void initDevice(device* dev)
    {
        if (!supported(dev)) return;
        auto& data = dev->create_private_data<DeviceData>();
        data.state = std::make_shared<State>();
        installCapture(dev, data.state);
        diagnostics::Write("[SMART] DirectX 12 pipeline replacement initialized; native capture enabled where available.");
    }
    inline void destroyDevice(device* dev)
    {
        if (!supported(dev)) return;
        removeCapture(dev);
        auto& s = state(dev);
        diagnostics::Write("[SMART] DirectX 12 cache: captured=%zu declined=%zu replacements=%zu", s.captured, s.declined, s.retained.size());
        dev->destroy_private_data<DeviceData>();
    }
    inline std::shared_ptr<Code> copyCode(State& s, D3D12_SHADER_BYTECODE source)
    {
        if (!source.pShaderBytecode || !source.BytecodeLength || source.BytecodeLength > 4 * 1024 * 1024) return {};
        const auto* bytes = static_cast<const uint8_t*>(source.pShaderBytecode);
        uint64_t hash = 14695981039346656037ull;
        for (size_t i = 0; i < source.BytecodeLength; ++i) { hash ^= bytes[i]; hash *= 1099511628211ull; }
        if (auto found = s.codePool.find(hash); found != s.codePool.end())
            if (auto code = found->second.lock(); code && code->bytes.size() == source.BytecodeLength &&
                std::memcmp(code->bytes.data(), bytes, source.BytecodeLength) == 0) return code;
        if (s.budget->bytes + source.BytecodeLength > 192 * 1024 * 1024) return {};
        auto code = std::make_shared<Code>();
        code->bytes.assign(bytes, bytes + source.BytecodeLength);
        code->budget = s.budget;
        s.budget->bytes += code->bytes.size();
        if (s.codePool.size() >= 65536)
        {
            for (auto it = s.codePool.begin(); it != s.codePool.end();)
                if (it->second.expired()) it = s.codePool.erase(it); else ++it;
        }
        if (s.codePool.size() < 65536) s.codePool[hash] = code;
        return code;
    }
    inline void capture(State& s, ID3D12PipelineState* pso, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& source)
    {
        std::lock_guard lock(s.mutex);
        const auto handle = reinterpret_cast<uintptr_t>(pso);
        s.pipelines.erase(handle); // A driver may recycle an old handle.
        if (!source.pRootSignature || !source.PS.BytecodeLength || !source.VS.BytecodeLength || source.NumRenderTargets > 8 ||
            source.StreamOutput.NumEntries || source.StreamOutput.NumStrides || source.InputLayout.NumElements > 32 ||
            (source.InputLayout.NumElements && !source.InputLayout.pInputElementDescs) || s.pipelines.size() >= 32768)
        { ++s.declined; return; }
        auto copy = std::make_shared<Pipeline>();
        copy->desc = source;
        source.pRootSignature->AddRef();
        copy->desc.CachedPSO = {}; // Driver caches refer to the old shader.
        D3D12_SHADER_BYTECODE* shaders[] = {&copy->desc.VS, &copy->desc.PS, &copy->desc.DS, &copy->desc.HS, &copy->desc.GS};
        for (size_t i = 0; i < 5; ++i)
        {
            if (!shaders[i]->BytecodeLength) { *shaders[i] = {}; continue; }
            copy->code[i] = copyCode(s, *shaders[i]);
            if (!copy->code[i]) { ++s.declined; return; }
            shaders[i]->pShaderBytecode = copy->code[i]->bytes.data();
        }
        const auto count = source.InputLayout.NumElements;
        copy->inputs.resize(count); copy->semantics.resize(count);
        for (UINT i = 0; i < count; ++i)
        {
            copy->inputs[i] = source.InputLayout.pInputElementDescs[i];
            if (!copy->inputs[i].SemanticName) { ++s.declined; return; }
            copy->semantics[i] = copy->inputs[i].SemanticName;
            copy->inputs[i].SemanticName = copy->semantics[i].c_str();
        }
        copy->desc.InputLayout.pInputElementDescs = copy->inputs.empty() ? nullptr : copy->inputs.data();
        copy->desc.StreamOutput.pSODeclaration = nullptr;
        copy->desc.StreamOutput.pBufferStrides = nullptr;
        uint64_t key=14695981039346656037ull;
        const auto add=[&](uint32_t value) { key=(key^value)*1099511628211ull; };
        add(source.NumRenderTargets);add(source.DSVFormat);add(source.SampleDesc.Count);add(source.SampleDesc.Quality);
        add(source.PrimitiveTopologyType);add(source.SampleMask);add(source.RasterizerState.CullMode);add(source.RasterizerState.FillMode);
        add(source.DepthStencilState.DepthEnable);add(source.DepthStencilState.DepthWriteMask);add(source.DepthStencilState.DepthFunc);
        add(source.DepthStencilState.StencilEnable);add(source.BlendState.AlphaToCoverageEnable);
        for(UINT i=0;i<source.NumRenderTargets;++i)
        {
            add(source.RTVFormats[i]);
            const auto& b=source.BlendState.RenderTarget[source.BlendState.IndependentBlendEnable?i:0];
            add(b.BlendEnable);add(b.LogicOpEnable);add(b.SrcBlend);add(b.DestBlend);add(b.BlendOp);
            add(b.SrcBlendAlpha);add(b.DestBlendAlpha);add(b.BlendOpAlpha);add(b.LogicOp);add(b.RenderTargetWriteMask);
        }
        for(const auto& input:copy->inputs)
        {
            add(input.SemanticIndex);add(input.Format);add(input.InputSlot);add(input.AlignedByteOffset);
            add(input.InputSlotClass);add(input.InstanceDataStepRate);
            for(const char* name=input.SemanticName;*name;++name) add(static_cast<unsigned char>(*name));
            add(0);
        }
        copy->drawKey=key?key:1;
        s.pipelines[handle] = std::move(copy);
        ++s.captured;
    }
    inline void forget(device* dev, uint64_t handle)
    {
        if (!supported(dev)) return;
        auto& s = state(dev); std::lock_guard lock(s.mutex); s.pipelines.erase(handle);
    }
    inline uint64_t drawKey(device* dev,uint64_t handle)
    {
        if(!supported(dev)) return 0;
        auto& s=state(dev);std::lock_guard lock(s.mutex);
        auto found=s.pipelines.find(handle);return found==s.pipelines.end()?0:found->second->drawKey;
    }
    inline bool loadDxc(State& s)
    {
        if (!s.dxcTried)
        {
            s.dxcTried = true;
            if (!GetModuleHandleExW(0, L"dxcompiler.dll", &s.dxcModule))
                s.dxcModule = LoadLibraryExW(L"dxcompiler.dll", nullptr, LOAD_LIBRARY_SEARCH_APPLICATION_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
            if (s.dxcModule)
            {
                const auto create = reinterpret_cast<DxcCreateInstanceProc>(GetProcAddress(s.dxcModule, "DxcCreateInstance"));
                if (create)
                {
                    create(CLSID_DxcUtils, __uuidof(IDxcUtils), reinterpret_cast<void**>(&s.utils.pointer));
                    create(CLSID_DxcCompiler, __uuidof(IDxcCompiler3), reinterpret_cast<void**>(&s.dxc.pointer));
                }
            }
        }
        return s.utils.pointer && s.dxc.pointer;
    }
    template<class Reflection, class Desc, class Output, class Binding>
    inline bool reflectedOutputs(Reflection* r, uint32_t& channels)
    {
        channels = 0;
        Desc desc{};
        if (FAILED(r->GetDesc(&desc)) || (desc.Version >> 16) != 0 || !desc.OutputParameters || desc.OutputParameters > 8) return false;
        uint32_t result = 0;
        for (UINT i = 0; i < desc.OutputParameters; ++i)
        {
            Output output{};
            if (FAILED(r->GetOutputParameterDesc(i, &output)) || output.SystemValueType != D3D_NAME_TARGET ||
                output.SemanticIndex >= 8 || output.Register != output.SemanticIndex || output.Stream ||
                output.ComponentType != D3D_REGISTER_COMPONENT_FLOAT32) return false;
            // For output signatures, ReadWriteMask marks channels NEVER written.
            const uint32_t mask = output.Mask & ~output.ReadWriteMask & 15u;
            if ((mask != 1 && mask != 3 && mask != 7 && mask != 15) || (result & (15u << (output.SemanticIndex * 4)))) return false;
            result |= mask << (output.SemanticIndex * 4);
        }
        for (UINT i = 0; i < desc.BoundResources; ++i)
        {
            Binding resource{};
            if (FAILED(r->GetResourceBindingDesc(i, &resource))) return false;
            switch (resource.Type)
            {
            case D3D_SIT_CBUFFER: case D3D_SIT_TBUFFER: case D3D_SIT_TEXTURE:
            case D3D_SIT_SAMPLER: case D3D_SIT_STRUCTURED: case D3D_SIT_BYTEADDRESS: break;
            default: return false;
            }
        }
        channels = result;
        return true;
    }
    inline bool compatible(ID3D12ShaderReflection* r, uint32_t& channels)
    {
        channels = 0;
        // Direct heap indexing can hide writable resources from BoundResources.
        if (r->GetRequiresFlags() & D3D_SHADER_REQUIRES_RESOURCE_DESCRIPTOR_HEAP_INDEXING) return false;
        return reflectedOutputs<ID3D12ShaderReflection, D3D12_SHADER_DESC, D3D12_SIGNATURE_PARAMETER_DESC,
            D3D12_SHADER_INPUT_BIND_DESC>(r, channels);
    }
    inline bool isDxil(const std::vector<uint8_t>& code)
    {
        if (code.size() < 32 || std::memcmp(code.data(), "DXBC", 4)) return false;
        const auto word = [&](size_t offset) { uint32_t value; std::memcpy(&value, code.data() + offset, 4); return value; };
        const size_t count = word(28);
        if (count > (code.size() - 32) / 4) return false;
        for (size_t i = 0; i < count; ++i)
        {
            const size_t offset = word(32 + i * 4);
            if (offset < 32 + count * 4 || offset > code.size() - 8) return false;
            if (word(offset + 4) > code.size() - offset - 8) return false;
            if (!std::memcmp(code.data() + offset, "DXIL", 4)) return true;
        }
        return false;
    }
    inline bool checkShader(State& s, Pipeline& p)
    {
        if (p.checked) return p.compatible;
        p.checked = true;
        const auto& code = p.code[1]->bytes;
        p.dxil = isDxil(code);
        if (!p.dxil)
        {
            Ref<ID3D11ShaderReflection> legacy;
            if (modern::loadCompiler(s.compiler) && SUCCEEDED(s.compiler.reflect(code.data(), code.size(),
                __uuidof(ID3D11ShaderReflection), reinterpret_cast<void**>(&legacy.pointer))) && legacy.pointer)
                p.compatible = reflectedOutputs<ID3D11ShaderReflection, D3D11_SHADER_DESC, D3D11_SIGNATURE_PARAMETER_DESC,
                    D3D11_SHADER_INPUT_BIND_DESC>(legacy.pointer, p.outputChannels);
            return p.compatible;
        }
        if (!loadDxc(s)) return false;
        Ref<ID3D12ShaderReflection> reflection;
        DxcBuffer buffer{code.data(), code.size(), 0};
        if (SUCCEEDED(s.utils->CreateReflection(&buffer, __uuidof(ID3D12ShaderReflection), reinterpret_cast<void**>(&reflection.pointer))) && reflection.pointer)
            p.compatible = compatible(reflection.pointer, p.outputChannels);
        return p.compatible;
    }
    inline std::string shaderSource(Choice choice, uint32_t channels)
    {
        const auto bits = modern::colourKey(choice);
        std::string source = "struct Outputs { ";
        for (UINT slot = 0; slot < 8; ++slot)
        {
            const auto mask = (channels >> (slot * 4)) & 15u;
            if (!mask) continue;
            const UINT count = mask == 1 ? 1 : mask == 3 ? 2 : mask == 7 ? 3 : 4;
            source += "float" + std::to_string(count) + " target" + std::to_string(slot) + " : SV_Target" + std::to_string(slot) + "; ";
        }
        source += "}; Outputs main() { Outputs o; float4 colour = asfloat(uint4(" + std::to_string(bits[0]) + "u," +
            std::to_string(bits[1]) + "u," + std::to_string(bits[2]) + "u," + std::to_string(bits[3]) + "u)); ";
        for (UINT slot = 0; slot < 8; ++slot)
        {
            const auto mask = (channels >> (slot * 4)) & 15u;
            if (!mask) continue;
            source += "o.target" + std::to_string(slot) + " = colour." + (mask == 1 ? "x" : mask == 3 ? "xy" : mask == 7 ? "xyz" : "xyzw") + "; ";
        }
        return source + "return o; }";
    }
    inline const std::vector<uint8_t>& colour(State& s, Choice choice, bool dxil, uint32_t channels)
    {
        const auto bits = modern::colourKey(choice);
        const std::array<uint32_t, 6> key{bits[0], bits[1], bits[2], bits[3], dxil ? 1u : 0u, channels};
        if (auto found = s.colours.find(key); found != s.colours.end()) return found->second;
        static const std::vector<uint8_t> empty;
        if (s.colours.size() >= 512) return empty;
        auto& result = s.colours[key];
        const auto source = shaderSource(choice, channels);
        if (dxil)
        {
            if (!loadDxc(s)) return result;
            DxcBuffer input{source.data(), source.size(), DXC_CP_UTF8};
            const wchar_t* args[]{L"-E", L"main", L"-T", L"ps_6_0", L"-O3", L"-Ges"};
            Ref<IDxcResult> compiled;
            HRESULT status = E_FAIL;
            if (SUCCEEDED(s.dxc->Compile(&input, args, static_cast<UINT>(std::size(args)), nullptr,
                __uuidof(IDxcResult), reinterpret_cast<void**>(&compiled.pointer))) && compiled.pointer &&
                SUCCEEDED(compiled->GetStatus(&status)) && SUCCEEDED(status))
            {
                Ref<IDxcBlob> blob;
                if (SUCCEEDED(compiled->GetResult(&blob.pointer)) && blob.pointer)
                {
                    auto* bytes = static_cast<const uint8_t*>(blob->GetBufferPointer());
                    result.assign(bytes, bytes + blob->GetBufferSize());
                }
            }
            if (result.empty()) diagnostics::Write("[ERROR] SMART DirectX 12 DXIL constant shader compilation failed HRESULT=%08lX", static_cast<unsigned long>(status));
        }
        else if (modern::loadCompiler(s.compiler))
        {
            Ref<ID3DBlob> blob, errors;
            const auto hr = s.compiler.compile(source.data(), source.size(), "ShaderTogglerAdvanced", nullptr, nullptr, "main", "ps_5_0",
                D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, &blob.pointer, &errors.pointer);
            if (SUCCEEDED(hr) && blob.pointer)
            {
                const auto* bytes = static_cast<const uint8_t*>(blob->GetBufferPointer());
                result.assign(bytes, bytes + blob->GetBufferSize());
            }
            else diagnostics::Write("[ERROR] SMART DirectX 12 DXBC constant shader compilation failed HRESULT=%08lX", static_cast<unsigned long>(hr));
        }
        return result;
    }
    inline void report(State& s, uint32_t hash, Choice choice, bool applied, const char* reason)
    {
        if (!s.statuses.count(hash) && s.statuses.size() >= 4096) return;
        auto& old = s.statuses[hash];
        if (!old.attempted || old.choice != choice || old.applied != applied || old.reason != reason)
            diagnostics::Write("[SMART] DirectX 12 shader=%08X method=%s applied=%u: %s", hash, name(choice.method), applied, reason);
        old = {choice, true, applied, reason};
    }
    inline Status status(device* dev, uint32_t hash)
    {
        auto& s = state(dev); std::lock_guard lock(s.mutex);
        const auto found = s.statuses.find(hash); return found == s.statuses.end() ? Status{} : found->second;
    }
    inline void summary(device* dev)
    {
        if (!supported(dev)) return;
        auto& s = state(dev); std::unique_lock lock(s.mutex, std::try_to_lock);
        if (lock.owns_lock())
            diagnostics::Write("[SMART] DirectX 12 captured=%zu declined=%zu live=%zu code_bytes=%zu replacement_attempts=%zu dxc_ready=%u",
                s.captured, s.declined, s.pipelines.size(), s.budget->bytes, s.retained.size(), s.utils.pointer && s.dxc.pointer);
    }
    inline void observe(command_list* cmd, uint64_t handle, uint32_t hash)
    {
        if (!supported(cmd->get_device())) return;
        auto& s = state(cmd->get_device()); std::lock_guard lock(s.mutex);
        if (s.suggestions.count(hash) || s.suggestions.size() >= 4096) return;
        auto found = s.pipelines.find(handle); if (found == s.pipelines.end()) return;
        const auto& b = found->second->desc.BlendState.RenderTarget[0];
        s.suggestions[hash] = modern::blendSuggestion(b.BlendEnable, b.SrcBlend, b.DestBlend, b.BlendOp);
    }
    inline Method suggestion(device* dev, uint32_t hash)
    {
        auto& s = state(dev); std::lock_guard lock(s.mutex);
        const auto found = s.suggestions.find(hash); return found == s.suggestions.end() ? Method::TransparentBlack : found->second;
    }
    inline void initCommands(command_list* cmd)
    {
        if (!supported(cmd->get_device())) return;
        cmd->create_private_data<CommandData>(); registerCommands(cmd);
    }
    inline void destroyCommands(command_list* cmd)
    {
        if (!supported(cmd->get_device())) return;
        unregisterCommands(cmd); cmd->destroy_private_data<CommandData>();
    }
    inline void resetCommands(command_list* cmd)
    {
        if (!supported(cmd->get_device())) return;
        auto& data = cmd->get_private_data<CommandData>();
        const bool ready = data.nativeIndirectReady;
        data = {};data.nativeIndirectReady = ready;
    }
    inline void restore(command_list* cmd)
    {
        if (!cmd || !supported(cmd->get_device())) return;
        auto& data = cmd->get_private_data<CommandData>();
        if (data.restore)
        {
            native(cmd)->SetPipelineState(reinterpret_cast<ID3D12PipelineState*>(static_cast<uintptr_t>(data.restore)));
            data.restore = 0;
        }
    }
    inline void bound(command_list* cmd, uint64_t handle)
    {
        if (!supported(cmd->get_device())) return;
        auto& data = cmd->get_private_data<CommandData>();
        data.current = handle;
        data.restore = 0; // The game already replaced the current native PSO.
    }
    inline void targets(command_list* cmd, uint32_t count, const reshade::api::resource_view* views, reshade::api::resource_view)
    {
        if (!supported(cmd->get_device())) return;
        auto& data = cmd->get_private_data<CommandData>();
        data.targetCount = count; data.targetMask = 0;
        data.targets = count > 8 || (count && !views) ? CommandData::Targets::Invalid : CommandData::Targets::Bound;
        if (data.targets == CommandData::Targets::Bound)
            for (UINT i = 0; i < count; ++i) if (views[i].handle) data.targetMask |= 1u << i;
    }
    inline void beginPass(command_list* cmd, uint32_t count, const reshade::api::render_pass_render_target_desc* views,
        const reshade::api::render_pass_depth_stencil_desc*)
    {
        if (!supported(cmd->get_device())) return;
        std::array<reshade::api::resource_view, 8> handles{};
        if (views && count <= 8) for (UINT i = 0; i < count; ++i) handles[i] = views[i].view;
        targets(cmd, count, views && count <= 8 ? handles.data() : nullptr, {});
    }
    inline void endPass(command_list* cmd)
    {
        restore(cmd);
        if (supported(cmd->get_device()))
        {
            auto& data = cmd->get_private_data<CommandData>();
            data.targets = CommandData::Targets::Ended; data.targetCount = data.targetMask = 0;
        }
    }
    inline uint32_t outputMask(uint32_t channels)
    {
        uint32_t mask = 0;
        for (UINT slot = 0; slot < 8; ++slot) if ((channels >> (slot * 4)) & 15u) mask |= 1u << slot;
        return mask;
    }
    inline void reportTargets(State& s, uint32_t hash, const CommandData& data, const Pipeline* p)
    {
        if (s.targetReports.size() >= 2048) return;
        uint32_t formatsHash = 0, writeMasks = 0;
        if (p) for (UINT i = 0; i < p->desc.NumRenderTargets; ++i)
        {
            formatsHash = formatsHash * 16777619u ^ static_cast<uint32_t>(p->desc.RTVFormats[i]);
            writeMasks |= (p->desc.BlendState.RenderTarget[p->desc.BlendState.IndependentBlendEnable ? i : 0].RenderTargetWriteMask & 15u) << (i * 4);
        }
        const std::array<uint32_t, 8> key{hash, static_cast<uint32_t>(data.targets), data.targetCount, data.targetMask,
            p ? p->desc.NumRenderTargets : UINT_MAX, p ? p->outputChannels : 0, formatsHash, writeMasks};
        if (!s.targetReports.insert(key).second) return;
        const char* tracking = data.targets == CommandData::Targets::Unknown ? "not-observed" :
            data.targets == CommandData::Targets::Bound ? "observed" : data.targets == CommandData::Targets::Ended ? "pass-ended" : "invalid";
        diagnostics::Write("[SMART] DirectX 12 targets shader=%08X tracking=%s bound_slots=%u view_mask=%02X pipeline_slots=%d output_channels=%08X write_channels=%08X",
            hash, tracking, data.targetCount, data.targetMask, p ? static_cast<int>(p->desc.NumRenderTargets) : -1,
            p ? p->outputChannels : 0, writeMasks);
    }
    inline const char* targetFailure(const CommandData& data, const Pipeline& p)
    {
        if (data.targets == CommandData::Targets::Invalid) return "Colour-target binding information is invalid; original pass retained.";
        if (data.targets == CommandData::Targets::Ended) return "No active colour targets after render-pass end; original pass retained.";
        if (!p.desc.NumRenderTargets) return "This pipeline has no colour targets; original pass retained.";
        const auto outputs = outputMask(p.outputChannels);
        for (UINT slot = 0; slot < 8; ++slot)
        {
            if (!(outputs & (1u << slot))) continue;
            if (slot >= p.desc.NumRenderTargets || p.desc.RTVFormats[slot] == DXGI_FORMAT_UNKNOWN)
                return "A shader output has no declared colour-target format; original pass retained.";
        }
        if (data.targets == CommandData::Targets::Bound)
        {
            if (!data.targetCount) return "This draw has no bound colour targets; original pass retained.";
            if ((outputs & data.targetMask) != outputs)
                return "A colour target required by this shader is missing from the observed bindings; original pass retained.";
        }
        for (UINT slot = 0; slot < p.desc.NumRenderTargets; ++slot)
        {
            const auto& b = p.desc.BlendState.RenderTarget[p.desc.BlendState.IndependentBlendEnable ? slot : 0];
            const auto dual = [](UINT factor) { return factor >= D3D12_BLEND_SRC1_COLOR && factor <= D3D12_BLEND_INV_SRC1_ALPHA; };
            if (b.BlendEnable && (dual(b.SrcBlend) || dual(b.DestBlend) || dual(b.SrcBlendAlpha) || dual(b.DestBlendAlpha)))
                return "Dual-source blending needs a different replacement method; original pass retained.";
        }
        return nullptr;
    }
    inline D3D12_GRAPHICS_PIPELINE_STATE_DESC replacementDescription(const Pipeline& p, const std::vector<uint8_t>& code)
    {
        auto desc = p.desc;
        desc.PS = {code.data(), code.size()};
        for (UINT slot = 0; slot < 8; ++slot)
        {
            auto& b = desc.BlendState.RenderTarget[slot];
            b = p.desc.BlendState.RenderTarget[p.desc.BlendState.IndependentBlendEnable ? slot : 0];
            b.RenderTargetWriteMask &= static_cast<uint8_t>((p.outputChannels >> (slot * 4)) & 15u);
        }
        if (desc.NumRenderTargets > 1) desc.BlendState.IndependentBlendEnable = TRUE;
        return desc;
    }
    inline bool apply(command_list* cmd, uint64_t expected, uint32_t hash, Choice choice)
    {
        auto* dev = cmd->get_device();
        if (!supported(dev) || !valid(choice) || choice.method == Method::Skip) return false;
        auto& s = state(dev); std::lock_guard lock(s.mutex);
        const auto fail = [&](const char* reason) { report(s, hash, choice, false, reason); return false; };
        auto& data = cmd->get_private_data<CommandData>();
        if (data.current != expected || !expected) return fail("The current pipeline could not be verified; original pass retained.");
        auto found = s.pipelines.find(expected);
        if (found == s.pipelines.end())
        {
            reportTargets(s, hash, data, nullptr);
            return fail("This native pipeline was not captured or has unsupported state; original pass retained.");
        }
        auto& p = *found->second;
        if (!p.desc.NumRenderTargets || data.targets == CommandData::Targets::Invalid || data.targets == CommandData::Targets::Ended ||
            (data.targets == CommandData::Targets::Bound && !data.targetCount))
        {
            reportTargets(s, hash, data, &p);
            return fail(targetFailure(data, p));
        }
        if (!checkShader(s, p))
        {
            reportTargets(s, hash, data, &p);
            if (!p.dxil && !s.compiler.reflect) return fail("System d3dcompiler_47.dll is unavailable; original pass retained.");
            if (p.dxil && !s.utils.pointer) return fail("This shader needs dxcompiler.dll in the game folder or system; original pass retained.");
            return fail("This shader has depth/coverage/integer outputs, unsupported channels, writable resources or unreadable metadata; original pass retained.");
        }
        reportTargets(s, hash, data, &p);
        if (const auto reason = targetFailure(data, p)) return fail(reason);
        const auto key = modern::colourKey(choice);
        auto variant = p.variants.find(key);
        if (variant == p.variants.end())
        {
            if (s.retained.size() >= 512) return fail("Replacement pipeline cache is full; original pass retained.");
            auto& replacement = p.variants[key];
            const auto& code = colour(s, choice, p.dxil, p.outputChannels);
            if (!code.empty())
            {
                auto desc = replacementDescription(p, code);
                struct Guard { Guard() { creatingReplacement = true; } ~Guard() { creatingReplacement = false; } } guard;
                const auto hr = native(dev)->CreateGraphicsPipelineState(&desc, __uuidof(ID3D12PipelineState), reinterpret_cast<void**>(&replacement));
                if (FAILED(hr))
                {
                    if (replacement) { replacement->Release(); replacement = nullptr; }
                    diagnostics::Write("[ERROR] SMART DirectX 12 CreateGraphicsPipelineState shader=%08X HRESULT=%08lX", hash, static_cast<unsigned long>(hr));
                }
            }
            s.retained.push_back(replacement); // Failed attempts are bounded too.
            variant = p.variants.find(key);
        }
        if (!variant->second) return fail("Replacement pipeline creation failed; original pass retained. See ShaderTogglerAdvanced.log.");
        data.restore = expected;
        native(cmd)->SetPipelineState(variant->second);
        const auto outputs = outputMask(p.outputChannels);
        report(s, hash, choice, true, (outputs & (outputs - 1)) ?
            "Replacement is running on several outputs. Check lighting and motion before accepting." :
            data.targets == CommandData::Targets::Unknown ? "Replacement is running using the captured pipeline output layout." : "Replacement is running.");
        return true;
    }
}
