// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#include "SmartDisableD3D12.h"
#include "Vendor/MinHook/include/MinHook.h"

namespace ShaderToggler::smart::dx12
{
    namespace
    {
        std::mutex registryMutex;
        std::mutex hookMutex;
        struct DeviceEntry { device* owner; std::shared_ptr<State> state; };
        std::map<void*, DeviceEntry> devices;
        std::map<reshade::api::command_queue*, device*> queues;
        std::map<void*, void*> originals;
        std::set<void*> enabledHooks;
        std::map<std::pair<void*, size_t>, void*> objectOriginals;
        std::map<void*, command_list*> commands;
        std::atomic<bool (*)(command_list*,bool)> indirectCallback{nullptr};
        std::atomic<bool (*)(command_list*,uint32_t,uint32_t,uint32_t,bool)> finderIndirectCallback{nullptr};
        constexpr GUID signatureGuid{0x8A20A36D, 0x18D4, 0x45B2, {0x9C,0xE2,0xC4,0x17,0x7B,0xD0,0x21,0x1A}};
        constexpr GUID finderSignatureGuid{0x337E48B0,0xF202,0x430A,{0xA1,0x1A,0x27,0xF0,0x68,0x91,0xCD,0x31}};
        struct FinderSignature { uint32_t kind=0, stride=0; };
        bool initialized = false;

        std::shared_ptr<State> findState(void* object)
        {
            std::lock_guard lock(registryMutex);
            const auto found = devices.find(object);
            return found == devices.end() ? nullptr : found->second.state;
        }
        template<class Fn> Fn original(void* object, size_t slot)
        {
            auto* target = (*static_cast<void***>(object))[slot];
            std::lock_guard lock(registryMutex);
            if (const auto found = objectOriginals.find({object, slot}); found != objectOriginals.end())
                return reinterpret_cast<Fn>(found->second);
            const auto found = originals.find(target);
            return found == originals.end() ? nullptr : reinterpret_cast<Fn>(found->second);
        }
        bool install(void* object, size_t slot, void* callback)
        {
            auto* target = (*static_cast<void***>(object))[slot];
            std::lock_guard installation(hookMutex);
            {
                std::lock_guard lock(registryMutex);
                objectOriginals.erase({object, slot});
                if (const auto found = originals.find(target); found != originals.end())
                { objectOriginals[{object, slot}] = found->second; return enabledHooks.count(target)!=0; }
            }
            void* trampoline = nullptr;
            auto result = MH_CreateHook(target, callback, &trampoline);
            if (result == MH_OK)
            {
                { std::lock_guard lock(registryMutex); originals[target] = trampoline; objectOriginals[{object, slot}] = trampoline; }
                result = MH_EnableHook(target);
            }
            if (result != MH_OK)
                diagnostics::Write("[ERROR] SMART DirectX 12 native capture hook slot=%zu failed status=%d", slot, static_cast<int>(result));
            else { std::lock_guard lock(registryMutex); enabledHooks.insert(target); }
            return result == MH_OK;
        }
        void captureResult(void* object, ID3D12PipelineState* pso, const D3D12_GRAPHICS_PIPELINE_STATE_DESC& desc)
        {
            if (creatingReplacement) return;
            if (auto s = findState(object))
            {
                try { capture(*s, pso, desc); }
                catch (...) { diagnostics::Write("[ERROR] SMART DirectX 12 pipeline capture allocation failed; original pipeline retained."); }
            }
        }
        // Each pipeline stream subobject uses pointer alignment, including on x86.
        template<class T> struct alignas(void*) StreamItem { D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type; T data; };
        template<class T> bool readItem(const uint8_t*& cursor, const uint8_t* end, T& out)
        {
            if (static_cast<size_t>(end - cursor) < sizeof(StreamItem<T>)) return false;
            StreamItem<T> item{}; std::memcpy(&item, cursor, sizeof(item));
            out = item.data; cursor += sizeof(item); return true;
        }
        bool streamDescription(const D3D12_PIPELINE_STATE_STREAM_DESC& stream, D3D12_GRAPHICS_PIPELINE_STATE_DESC& d)
        {
            if (!stream.pPipelineStateSubobjectStream || !stream.SizeInBytes || stream.SizeInBytes > 65536) return false;
            d = {};
            d.SampleMask = UINT_MAX; d.SampleDesc.Count = 1;
            d.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
            d.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
            d.RasterizerState.DepthClipEnable = TRUE;
            d.DepthStencilState.DepthEnable = TRUE;
            d.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            d.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            d.DepthStencilState.StencilReadMask = d.DepthStencilState.StencilWriteMask = 0xFF;
            d.DepthStencilState.FrontFace = d.DepthStencilState.BackFace =
                {D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS};
            for (auto& b : d.BlendState.RenderTarget)
            {
                b.SrcBlend = b.SrcBlendAlpha = D3D12_BLEND_ONE;
                b.DestBlend = b.DestBlendAlpha = D3D12_BLEND_ZERO;
                b.BlendOp = b.BlendOpAlpha = D3D12_BLEND_OP_ADD;
                b.LogicOp = D3D12_LOGIC_OP_NOOP; b.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
            }
            const auto* cursor = static_cast<const uint8_t*>(stream.pPipelineStateSubobjectStream);
            const auto* end = cursor + stream.SizeInBytes;
            uint64_t seen = 0;
            while (cursor < end)
            {
                if (end - cursor < sizeof(D3D12_PIPELINE_STATE_SUBOBJECT_TYPE)) return false;
                D3D12_PIPELINE_STATE_SUBOBJECT_TYPE type; std::memcpy(&type, cursor, sizeof(type));
                const auto index = static_cast<unsigned>(type);
                if (index >= 64 || (seen & (1ull << index))) return false;
                seen |= 1ull << index;
#define STA_STREAM_CASE(kind, member) case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_##kind: if (!readItem(cursor, end, d.member)) return false; break
                switch (type)
                {
                STA_STREAM_CASE(ROOT_SIGNATURE, pRootSignature);
                STA_STREAM_CASE(VS, VS); STA_STREAM_CASE(PS, PS); STA_STREAM_CASE(DS, DS);
                STA_STREAM_CASE(HS, HS); STA_STREAM_CASE(GS, GS);
                STA_STREAM_CASE(STREAM_OUTPUT, StreamOutput); STA_STREAM_CASE(BLEND, BlendState);
                STA_STREAM_CASE(SAMPLE_MASK, SampleMask); STA_STREAM_CASE(RASTERIZER, RasterizerState);
                STA_STREAM_CASE(DEPTH_STENCIL, DepthStencilState); STA_STREAM_CASE(INPUT_LAYOUT, InputLayout);
                STA_STREAM_CASE(IB_STRIP_CUT_VALUE, IBStripCutValue); STA_STREAM_CASE(PRIMITIVE_TOPOLOGY, PrimitiveTopologyType);
                STA_STREAM_CASE(DEPTH_STENCIL_FORMAT, DSVFormat); STA_STREAM_CASE(SAMPLE_DESC, SampleDesc);
                STA_STREAM_CASE(NODE_MASK, NodeMask); STA_STREAM_CASE(CACHED_PSO, CachedPSO); STA_STREAM_CASE(FLAGS, Flags);
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_RENDER_TARGET_FORMATS:
                {
                    D3D12_RT_FORMAT_ARRAY formats{}; if (!readItem(cursor, end, formats) || formats.NumRenderTargets > 8) return false;
                    d.NumRenderTargets = formats.NumRenderTargets;
                    std::copy(std::begin(formats.RTFormats), std::end(formats.RTFormats), d.RTVFormats); break;
                }
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1:
                {
                    if (seen & (1ull << D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL)) return false;
                    D3D12_DEPTH_STENCIL_DESC1 depth{}; if (!readItem(cursor, end, depth) || depth.DepthBoundsTestEnable) return false;
                    d.DepthStencilState = {depth.DepthEnable, depth.DepthWriteMask, depth.DepthFunc, depth.StencilEnable,
                        depth.StencilReadMask, depth.StencilWriteMask, depth.FrontFace, depth.BackFace}; break;
                }
                case D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_VIEW_INSTANCING:
                {
                    D3D12_VIEW_INSTANCING_DESC view{}; if (!readItem(cursor, end, view) || view.ViewInstanceCount || view.Flags) return false;
                    break;
                }
                default: return false; // Compute, mesh, depth-bounds and unknown extended state are not cloned.
                }
#undef STA_STREAM_CASE
            }
            const auto depthTypes = (1ull << D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL) |
                (1ull << D3D12_PIPELINE_STATE_SUBOBJECT_TYPE_DEPTH_STENCIL1);
            return cursor == end && (seen & depthTypes) != depthTypes && d.VS.BytecodeLength && d.PS.BytecodeLength;
        }
        void captureStream(void* device, ID3D12PipelineState* pso, const D3D12_PIPELINE_STATE_STREAM_DESC& stream)
        {
            if (creatingReplacement) return;
            D3D12_GRAPHICS_PIPELINE_STATE_DESC desc{};
            if (streamDescription(stream, desc)) captureResult(device, pso, desc);
            else if (auto s = findState(device)) { std::lock_guard lock(s->mutex); ++s->declined; }
        }
        bool graphicsSignature(const D3D12_COMMAND_SIGNATURE_DESC& desc)
        {
            if (!desc.pArgumentDescs || !desc.NumArgumentDescs || desc.NumArgumentDescs > 64) return false;
            for (UINT i = 0; i < desc.NumArgumentDescs; ++i)
            {
                const auto type = desc.pArgumentDescs[i].Type;
                if (i + 1 == desc.NumArgumentDescs)
                    return type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW || type == D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;
                switch (type)
                {
                case D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW:
                case D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW:
                case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT:
                case D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT_BUFFER_VIEW:
                case D3D12_INDIRECT_ARGUMENT_TYPE_SHADER_RESOURCE_VIEW:
                case D3D12_INDIRECT_ARGUMENT_TYPE_UNORDERED_ACCESS_VIEW: break;
                default: return false;
                }
            }
            return false;
        }
        FinderSignature finderSignature(const D3D12_COMMAND_SIGNATURE_DESC& desc)
        {
            FinderSignature info;
            if(desc.pArgumentDescs && desc.NumArgumentDescs && desc.NumArgumentDescs<=64 && (graphicsSignature(desc) || desc.NumArgumentDescs==1))
            {
                const auto last=desc.pArgumentDescs[desc.NumArgumentDescs-1].Type;
                info.kind=last==D3D12_INDIRECT_ARGUMENT_TYPE_DRAW?1u:last==D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED?2u:
                    last==D3D12_INDIRECT_ARGUMENT_TYPE_DISPATCH?3u:0u;
                if(info.kind && desc.NumArgumentDescs>1) info.kind|=0x100; // Graphics binding updates require zero-count native execution.
                info.stride=desc.ByteStride;
            }
            return info;
        }
        using CreateSignature = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_COMMAND_SIGNATURE_DESC*, ID3D12RootSignature*, REFIID, void**);
        HRESULT STDMETHODCALLTYPE createSignature(ID3D12Device* dev, const D3D12_COMMAND_SIGNATURE_DESC* desc, ID3D12RootSignature* root, REFIID iid, void** out)
        {
            const auto next = original<CreateSignature>(dev, 41);
            if (!next) return E_FAIL;
            const auto hr = next(dev, desc, root, iid, out);
            if (SUCCEEDED(hr) && desc && out && *out && iid == __uuidof(ID3D12CommandSignature) && findState(dev))
            {
                // Plain private bytes have no add-on-owned destructor or lifetime.
                const UINT graphics = graphicsSignature(*desc) ? 1u : 0u;
                static_cast<ID3D12CommandSignature*>(*out)->SetPrivateData(signatureGuid, sizeof(graphics), &graphics);
                const auto info=finderSignature(*desc);
                static_cast<ID3D12CommandSignature*>(*out)->SetPrivateData(finderSignatureGuid,sizeof(info),&info);
            }
            return hr;
        }
        using ExecuteIndirect = void(STDMETHODCALLTYPE*)(ID3D12GraphicsCommandList*, ID3D12CommandSignature*, UINT, ID3D12Resource*, UINT64, ID3D12Resource*, UINT64);
        void STDMETHODCALLTYPE executeIndirect(ID3D12GraphicsCommandList* nativeCommands, ID3D12CommandSignature* signature, UINT maxCount,
            ID3D12Resource* arguments, UINT64 offset, ID3D12Resource* counts, UINT64 countOffset)
        {
            const auto next = original<ExecuteIndirect>(nativeCommands, 59);
            if (!next) return;
            command_list* cmd = nullptr;
            { std::lock_guard lock(registryMutex); const auto found = commands.find(nativeCommands); if (found != commands.end()) cmd = found->second; }
            const auto finder=finderIndirectCallback.load(std::memory_order_acquire);
            FinderSignature info;UINT infoBytes=sizeof(info);
            if(cmd && finder && signature && SUCCEEDED(signature->GetPrivateData(finderSignatureGuid,&infoBytes,&info)) && infoBytes==sizeof(info) && info.kind)
            {
                restore(cmd);
                const bool bindings=(info.kind&0x100)!=0;
                const uint32_t kind=info.kind&0xFF;
                const bool canSkip=!(info.kind&~0x1FFu) && kind>=1 && kind<=3 && (!bindings || kind<=2);
                if(finder(cmd,info.kind,maxCount,info.stride,canSkip) && canSkip)
                {
                    // A filtered graphics submission still performs the API's
                    // binding resets. Do not drop a binding-update call outright.
                    if(bindings) next(nativeCommands,signature,0,arguments,offset,counts,countOffset);
                    return;
                }
            }
            UINT graphics = 0, bytes = sizeof(graphics);
            const auto prepare = indirectCallback.load(std::memory_order_acquire);
            if (cmd && prepare && signature)
            {
                struct Restore { command_list* cmd; ~Restore() { restore(cmd); } } restoreAfter{cmd};
                const bool verifiedGraphics = SUCCEEDED(signature->GetPrivateData(signatureGuid, &bytes, &graphics)) && bytes == sizeof(graphics) && graphics == 1;
                restore(cmd);
                const bool blocked = prepare(cmd, verifiedGraphics);
                // Retain ExecuteIndirect's reset of the root/VB/IB bindings it
                // changes, even when original ShaderToggler matching hides the
                // submission. A zero maximum prevents any draw/dispatch work.
                // https://learn.microsoft.com/windows/win32/direct3d12/indirect-drawing
                next(nativeCommands, signature, blocked ? 0 : maxCount, arguments, offset, counts, countOffset);
            }
            else next(nativeCommands, signature, maxCount, arguments, offset, counts, countOffset);
        }
        using CreateGraphics = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device*, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, REFIID, void**);
        HRESULT STDMETHODCALLTYPE createGraphics(ID3D12Device* dev, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, REFIID iid, void** out)
        {
            const auto next = original<CreateGraphics>(dev, 10);
            if (!next) return E_FAIL;
            const auto hr = next(dev, desc, iid, out);
            if (SUCCEEDED(hr) && desc && out && *out && iid == __uuidof(ID3D12PipelineState))
                captureResult(dev, static_cast<ID3D12PipelineState*>(*out), *desc);
            return hr;
        }
        using CreateStream = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device2*, const D3D12_PIPELINE_STATE_STREAM_DESC*, REFIID, void**);
        HRESULT STDMETHODCALLTYPE createStream(ID3D12Device2* dev, const D3D12_PIPELINE_STATE_STREAM_DESC* desc, REFIID iid, void** out)
        {
            const auto next = original<CreateStream>(dev, 47);
            if (!next) return E_FAIL;
            const auto hr = next(dev, desc, iid, out);
            if (SUCCEEDED(hr) && desc && out && *out && iid == __uuidof(ID3D12PipelineState))
                captureStream(dev, static_cast<ID3D12PipelineState*>(*out), *desc);
            return hr;
        }
        using LoadGraphics = HRESULT(STDMETHODCALLTYPE*)(ID3D12PipelineLibrary*, LPCWSTR, const D3D12_GRAPHICS_PIPELINE_STATE_DESC*, REFIID, void**);
        HRESULT STDMETHODCALLTYPE loadGraphics(ID3D12PipelineLibrary* library, LPCWSTR name, const D3D12_GRAPHICS_PIPELINE_STATE_DESC* desc, REFIID iid, void** out)
        {
            const auto next = original<LoadGraphics>(library, 9);
            if (!next) return E_FAIL;
            const auto hr = next(library, name, desc, iid, out);
            if (SUCCEEDED(hr) && desc && out && *out && iid == __uuidof(ID3D12PipelineState))
            {
                Ref<ID3D12Device> dev;
                if (SUCCEEDED(library->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&dev.pointer))))
                    captureResult(dev.pointer, static_cast<ID3D12PipelineState*>(*out), *desc);
            }
            return hr;
        }
        using LoadStream = HRESULT(STDMETHODCALLTYPE*)(ID3D12PipelineLibrary1*, LPCWSTR, const D3D12_PIPELINE_STATE_STREAM_DESC*, REFIID, void**);
        HRESULT STDMETHODCALLTYPE loadStream(ID3D12PipelineLibrary1* library, LPCWSTR name, const D3D12_PIPELINE_STATE_STREAM_DESC* desc, REFIID iid, void** out)
        {
            const auto next = original<LoadStream>(library, 13);
            if (!next) return E_FAIL;
            const auto hr = next(library, name, desc, iid, out);
            if (SUCCEEDED(hr) && desc && out && *out && iid == __uuidof(ID3D12PipelineState))
            {
                Ref<ID3D12Device> dev;
                if (SUCCEEDED(library->GetDevice(__uuidof(ID3D12Device), reinterpret_cast<void**>(&dev.pointer))))
                    captureStream(dev.pointer, static_cast<ID3D12PipelineState*>(*out), *desc);
            }
            return hr;
        }
        using CreateLibrary = HRESULT(STDMETHODCALLTYPE*)(ID3D12Device1*, const void*, SIZE_T, REFIID, void**);
        HRESULT STDMETHODCALLTYPE createLibrary(ID3D12Device1* dev, const void* blob, SIZE_T length, REFIID iid, void** out)
        {
            const auto next = original<CreateLibrary>(dev, 44);
            if (!next) return E_FAIL;
            const auto hr = next(dev, blob, length, iid, out);
            if (SUCCEEDED(hr) && out && *out && findState(dev))
            {
                Ref<ID3D12PipelineLibrary> library;
                auto* object = static_cast<IUnknown*>(*out);
                if (SUCCEEDED(object->QueryInterface(__uuidof(ID3D12PipelineLibrary), reinterpret_cast<void**>(&library.pointer))))
                    install(library.pointer, 9, reinterpret_cast<void*>(&loadGraphics));
                Ref<ID3D12PipelineLibrary1> library1;
                if (SUCCEEDED(object->QueryInterface(__uuidof(ID3D12PipelineLibrary1), reinterpret_cast<void**>(&library1.pointer))))
                    install(library1.pointer, 13, reinterpret_cast<void*>(&loadStream));
            }
            return hr;
        }
    }
    void installCapture(device* dev, const std::shared_ptr<State>& value)
    {
        {
            std::lock_guard lock(hookMutex);
            if (!initialized)
            {
                const auto result = MH_Initialize();
                if (result != MH_OK && result != MH_ERROR_ALREADY_INITIALIZED) { diagnostics::Write("[ERROR] SMART DirectX 12 capture initialization failed status=%d", static_cast<int>(result)); return; }
                initialized = true;
            }
        }
        const auto add = [&](void* pointer) { std::lock_guard lock(registryMutex); devices[pointer] = {dev, value}; };
        auto* d3d = native(dev); add(d3d);
        install(d3d, 10, reinterpret_cast<void*>(&createGraphics));
        install(d3d, 41, reinterpret_cast<void*>(&createSignature));
        Ref<ID3D12Device1> dev1;
        if (SUCCEEDED(d3d->QueryInterface(__uuidof(ID3D12Device1), reinterpret_cast<void**>(&dev1.pointer))))
        { add(dev1.pointer); install(dev1.pointer, 44, reinterpret_cast<void*>(&createLibrary)); }
        Ref<ID3D12Device2> dev2;
        if (SUCCEEDED(d3d->QueryInterface(__uuidof(ID3D12Device2), reinterpret_cast<void**>(&dev2.pointer))))
        { add(dev2.pointer); install(dev2.pointer, 47, reinterpret_cast<void*>(&createStream)); }
    }
    void removeCapture(device* dev)
    {
        std::lock_guard lock(registryMutex);
        for (auto it = devices.begin(); it != devices.end();)
            if (it->second.owner == dev) it = devices.erase(it); else ++it;
    }
    void registerCommands(command_list* cmd)
    {
        auto* object = native(cmd);
        { std::lock_guard lock(registryMutex); commands[object] = cmd; }
        cmd->get_private_data<CommandData>().nativeIndirectReady = initialized && install(object, 59, reinterpret_cast<void*>(&executeIndirect));
    }
    void unregisterCommands(command_list* cmd)
    {
        std::lock_guard lock(registryMutex);
        auto* object = native(cmd); commands.erase(object); objectOriginals.erase({object, 59});
    }
    void setIndirectCallback(bool (*callback)(command_list*,bool))
    {
        indirectCallback.store(callback, std::memory_order_release);
    }
    void setFinderIndirectCallback(bool (*callback)(command_list*,uint32_t,uint32_t,uint32_t,bool))
    {
        finderIndirectCallback.store(callback,std::memory_order_release);
    }
    void initQueue(reshade::api::command_queue* queue)
    {
        if (!supported(queue->get_device())) return;
        std::lock_guard lock(registryMutex); queues[queue] = queue->get_device();
    }
    void destroyQueue(reshade::api::command_queue* queue)
    {
        std::lock_guard lock(registryMutex); queues.erase(queue);
    }
    void shutdownCapture(bool processExit)
    {
        if (!initialized || processExit) return;
        // ReShade stops using the add-on before unloading it. Drain GPU work
        // before releasing PSOs which recorded commands may still reference.
        std::vector<device*> owners;
        { std::lock_guard lock(registryMutex); for (const auto& [key, entry] : devices)
            if (std::find(owners.begin(), owners.end(), entry.owner) == owners.end()) owners.push_back(entry.owner); }
        std::vector<reshade::api::command_queue*> liveQueues;
        { std::lock_guard lock(registryMutex); for (const auto& [queue, owner] : queues) liveQueues.push_back(queue); }
        for (auto* queue : liveQueues) queue->wait_idle();
        indirectCallback.store(nullptr, std::memory_order_release);
        finderIndirectCallback.store(nullptr, std::memory_order_release);
        MH_DisableHook(MH_ALL_HOOKS);
        { std::lock_guard lock(registryMutex); devices.clear(); enabledHooks.clear(); }
        for (auto* owner : owners) owner->destroy_private_data<DeviceData>();
        MH_Uninitialize();
        initialized = false;
    }
}
