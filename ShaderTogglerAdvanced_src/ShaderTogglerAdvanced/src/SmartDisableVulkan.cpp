// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#include "SmartDisableOther.h"
#define VK_NO_PROTOTYPES 
#include "Vendor/Khronos/vulkan_core.h"
#include "Vendor/SPIRV-Reflect/spirv_reflect.h"
#include "Vendor/MinHook/include/MinHook.h"
#include <memory>
#include <vector>
#include <algorithm>
#include <set>
namespace ShaderToggler::smart::other::vk
{
    template<class T> uint64_t key(T value) { return (uint64_t)value; }
    struct Functions
    {
#define STA_VK_FUNCTIONS(X) \
        X(CreateGraphicsPipelines) X(DestroyPipeline) X(CreateShaderModule) X(DestroyShaderModule) \
        X(CreatePipelineLayout) X(DestroyPipelineLayout) X(CreateRenderPass) X(CreateRenderPass2) X(CreateRenderPass2KHR) X(DestroyRenderPass) \
        X(CmdBindPipeline) X(DeviceWaitIdle)
#define STA_VK_MEMBER(n) PFN_vk ##n n = nullptr;
        STA_VK_FUNCTIONS(STA_VK_MEMBER)
#undef STA_VK_MEMBER
    };
    struct Budget { size_t bytes=0; };
    struct Code
    {
        std::vector<uint32_t> words;
        std::shared_ptr<Budget> budget;
        ~Code() { if(budget) budget->bytes-=words.size()*4; }
    };
    struct Dependency
    {
        VkDevice device=VK_NULL_HANDLE;
        VkPipelineLayout layout=VK_NULL_HANDLE;
        VkRenderPass pass=VK_NULL_HANDLE;
        PFN_vkDestroyPipelineLayout destroyLayout=nullptr;
        PFN_vkDestroyRenderPass destroyPass=nullptr;
        ~Dependency() { Internal own; if(layout) destroyLayout(device,layout,nullptr); if(pass) destroyPass(device,pass,nullptr); }
    };
    struct Chain
    {
        std::vector<std::vector<uint64_t>> nodes;
        std::vector<VkFormat> formats;
        std::vector<VkVertexInputBindingDivisorDescriptionEXT> divisors;
        std::vector<VkBool32> colourWrites;
        bool copy(const void* input)
        {
            std::set<VkStructureType> seen;
            auto* p=static_cast<const VkBaseInStructure*>(input);
            for(unsigned count=0;p;p=p->pNext,++count)
            {
                if(count>=32 || !seen.insert(p->sType).second) return false;
                size_t size=0;
#define FIXED(st,t) case VK_STRUCTURE_TYPE_ ##st:size=sizeof(t);break
                switch(p->sType)
                {
                FIXED(PIPELINE_RENDERING_CREATE_INFO,VkPipelineRenderingCreateInfo);
                FIXED(PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT,VkPipelineVertexInputDivisorStateCreateInfoEXT);
                FIXED(PIPELINE_COLOR_WRITE_CREATE_INFO_EXT,VkPipelineColorWriteCreateInfoEXT);
                FIXED(PIPELINE_TESSELLATION_DOMAIN_ORIGIN_STATE_CREATE_INFO,VkPipelineTessellationDomainOriginStateCreateInfo);
                FIXED(PIPELINE_RASTERIZATION_DEPTH_CLIP_STATE_CREATE_INFO_EXT,VkPipelineRasterizationDepthClipStateCreateInfoEXT);
                FIXED(PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT,VkPipelineRasterizationConservativeStateCreateInfoEXT);
                FIXED(PIPELINE_RASTERIZATION_PROVOKING_VERTEX_STATE_CREATE_INFO_EXT,VkPipelineRasterizationProvokingVertexStateCreateInfoEXT);
                FIXED(PIPELINE_RASTERIZATION_LINE_STATE_CREATE_INFO_EXT,VkPipelineRasterizationLineStateCreateInfoEXT);
                FIXED(PIPELINE_RASTERIZATION_STATE_RASTERIZATION_ORDER_AMD,VkPipelineRasterizationStateRasterizationOrderAMD);
                FIXED(PIPELINE_VIEWPORT_DEPTH_CLIP_CONTROL_CREATE_INFO_EXT,VkPipelineViewportDepthClipControlCreateInfoEXT);
                FIXED(PIPELINE_SHADER_STAGE_REQUIRED_SUBGROUP_SIZE_CREATE_INFO,VkPipelineShaderStageRequiredSubgroupSizeCreateInfo);
                FIXED(PIPELINE_ROBUSTNESS_CREATE_INFO_EXT,VkPipelineRobustnessCreateInfoEXT);
                case VK_STRUCTURE_TYPE_PIPELINE_CREATION_FEEDBACK_CREATE_INFO:continue;
                case VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO:continue;
                default:return false;
                }
#undef FIXED
                nodes.emplace_back((size+7)/8);
                auto* node=reinterpret_cast<VkBaseOutStructure*>(nodes.back().data());
                std::memcpy(node,p,size);node->pNext=nullptr;
                if(p->sType==VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO)
                {
                    auto& v=*reinterpret_cast<VkPipelineRenderingCreateInfo*>(node);
                    if(v.colorAttachmentCount!=1 || !v.pColorAttachmentFormats) return false;
                    formats.assign(v.pColorAttachmentFormats,v.pColorAttachmentFormats+v.colorAttachmentCount);
                    v.pColorAttachmentFormats=formats.data();
                }
                else if(p->sType==VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_DIVISOR_STATE_CREATE_INFO_EXT)
                {
                    auto& v=*reinterpret_cast<VkPipelineVertexInputDivisorStateCreateInfoEXT*>(node);
                    if(v.vertexBindingDivisorCount>64 || (v.vertexBindingDivisorCount&&!v.pVertexBindingDivisors)) return false;
                    if(v.vertexBindingDivisorCount) divisors.assign(v.pVertexBindingDivisors,v.pVertexBindingDivisors+v.vertexBindingDivisorCount);
                    v.pVertexBindingDivisors=divisors.data();
                }
                else if(p->sType==VK_STRUCTURE_TYPE_PIPELINE_COLOR_WRITE_CREATE_INFO_EXT)
                {
                    auto& v=*reinterpret_cast<VkPipelineColorWriteCreateInfoEXT*>(node);
                    if(v.attachmentCount!=1 || !v.pColorWriteEnables) return false;
                    colourWrites.assign(v.pColorWriteEnables,v.pColorWriteEnables+v.attachmentCount);v.pColorWriteEnables=colourWrites.data();
                }
            }
            for(size_t i=1;i<nodes.size();++i)
                reinterpret_cast<VkBaseOutStructure*>(nodes[i-1].data())->pNext=reinterpret_cast<VkBaseOutStructure*>(nodes[i].data());
            return true;
        }
        const void* head() const { return nodes.empty()?nullptr:nodes.front().data(); }
    };
    struct Stage
    {
        VkPipelineShaderStageCreateInfo info{};
        std::string entry;
        std::shared_ptr<Code> code;
        std::vector<VkSpecializationMapEntry> entries;
        std::vector<uint8_t> constants;
        VkSpecializationInfo specialization{};
        Chain chain;
    };
    struct Pipeline
    {
        VkGraphicsPipelineCreateInfo info{};
        std::vector<Stage> stages;
        std::vector<VkPipelineShaderStageCreateInfo> stageInfos;
        std::shared_ptr<Dependency> layout,pass;
        Chain chain,vertexChain,assemblyChain,tessChain,viewportChain,rasterChain,multiChain,depthChain,blendChain,dynamicChain;
        VkPipelineVertexInputStateCreateInfo vertex{};
        VkPipelineInputAssemblyStateCreateInfo assembly{};
        VkPipelineTessellationStateCreateInfo tess{};
        VkPipelineViewportStateCreateInfo viewport{};
        VkPipelineRasterizationStateCreateInfo raster{};
        VkPipelineMultisampleStateCreateInfo multi{};
        VkPipelineDepthStencilStateCreateInfo depth{};
        VkPipelineColorBlendStateCreateInfo blend{};
        VkPipelineDynamicStateCreateInfo dynamic{};
        std::vector<VkVertexInputBindingDescription> bindings;
        std::vector<VkVertexInputAttributeDescription> attributes;
        std::vector<VkViewport> viewports;
        std::vector<VkRect2D> scissors;
        std::vector<VkSampleMask> masks;
        std::vector<VkPipelineColorBlendAttachmentState> attachments;
        std::vector<VkDynamicState> dynamics;
        size_t fragment=0;
        bool checked=false,compatible=false;
        std::map<std::array<uint32_t,4>,VkPipeline> variants;
    };
    struct State
    {
        std::recursive_mutex mutex;
        device* owner=nullptr;
        VkDevice deviceHandle=VK_NULL_HANDLE;
        Functions f;
        bool ready=false;
        std::shared_ptr<Budget> budget=std::make_shared<Budget>();
        std::map<uint64_t,std::shared_ptr<Code>> modules;
        std::map<uint64_t,std::shared_ptr<Dependency>> layouts,passes;
        std::map<uint64_t,std::shared_ptr<Pipeline>> pipelines;
        std::vector<std::pair<VkPipeline,std::shared_ptr<Pipeline>>> retained;
        std::map<uint32_t,Status> statuses;
        std::map<uint32_t,Method> suggestions;
        size_t captured=0,declined=0;
        ~State()
        {
            Internal own;
            for(auto& [p,snapshot]:retained) if(p && f.DestroyPipeline) f.DestroyPipeline(deviceHandle,p,nullptr);
            retained.clear();pipelines.clear();passes.clear();layouts.clear();
        }
    };
    struct __declspec(uuid("8AD44321-FC34-4E65-9896-3B6F33DF4772")) DeviceData { std::shared_ptr<State> state; };
    struct __declspec(uuid("22AF54CB-48CA-4D83-82F4-149468393B50")) Commands { uint64_t graphics=0,restore=0; bool singleTarget=false; };
    std::recursive_mutex registryMutex;
    std::map<VkDevice,std::shared_ptr<State>> devices;
    std::map<void*,void*> exportOriginals;
    std::vector<void*> hookTargets;
    std::vector<HMODULE> hookModules;
    PFN_vkGetDeviceProcAddr getDeviceProc=nullptr;
    HMODULE loader=nullptr;
    bool hooksReady=false;
    std::shared_ptr<State> find(VkDevice d) { std::lock_guard lock(registryMutex);auto i=devices.find(d);return i==devices.end()?nullptr:i->second; }
    State& state(device* d) { return *d->get_private_data<DeviceData>().state; }
    void* unhooked(void* address)
    { std::lock_guard lock(registryMutex);auto i=exportOriginals.find(address);return i==exportOriginals.end()?address:i->second; }
    void resolve(State& s)
    {
        std::lock_guard lock(s.mutex);
        if(s.ready || !getDeviceProc) return;
#define STA_VK_LOAD(n) s.f.n=reinterpret_cast<PFN_vk ##n>(unhooked(reinterpret_cast<void*>(getDeviceProc(s.deviceHandle,"vk" #n))));
        STA_VK_FUNCTIONS(STA_VK_LOAD)
#undef STA_VK_LOAD
        s.ready=s.f.CreateGraphicsPipelines && s.f.DestroyPipeline && s.f.CreateShaderModule && s.f.DestroyShaderModule &&
            s.f.CreatePipelineLayout && s.f.DestroyPipelineLayout && s.f.CreateRenderPass && s.f.DestroyRenderPass && s.f.CmdBindPipeline;
    }
    std::shared_ptr<Code> code(State& s,const uint32_t* words,size_t bytes)
    {
        if(!words || bytes<20 || bytes%4 || bytes>4*1024*1024 || s.budget->bytes+bytes>192*1024*1024) return {};
        auto c=std::make_shared<Code>();c->words.assign(words,words+bytes/4);c->budget=s.budget;s.budget->bytes+=bytes;return c;
    }
    template<class T> bool array(std::vector<T>& out,const T*& pointer,uint32_t count,uint32_t cap,bool optional=false)
    {
        if(count>cap || (count&&!pointer&&!optional)) return false;
        if(pointer && count) out.assign(pointer,pointer+count);
        pointer=out.empty()?nullptr:out.data();return true;
    }
    bool snapshot(State& s,Pipeline& p,const VkGraphicsPipelineCreateInfo& input)
    {
        p.info=input;
        if(input.stageCount<2 || input.stageCount>5 || !input.pStages ||
            (input.flags&VK_PIPELINE_CREATE_LIBRARY_BIT_KHR) || !input.pColorBlendState ||
            input.pColorBlendState->attachmentCount!=1) return false;
        auto l=s.layouts.find(key(input.layout));if(l==s.layouts.end()) return false;p.layout=l->second;p.info.layout=p.layout->layout;
        if(input.renderPass)
        { auto r=s.passes.find(key(input.renderPass));if(r==s.passes.end()) return false;p.pass=r->second;p.info.renderPass=p.pass->pass; }
        if(!p.chain.copy(input.pNext)) return false;p.info.pNext=p.chain.head();
        if(!input.renderPass && p.chain.formats.size()!=1) return false;
        p.info.flags&=~(VK_PIPELINE_CREATE_DERIVATIVE_BIT|VK_PIPELINE_CREATE_FAIL_ON_PIPELINE_COMPILE_REQUIRED_BIT);
        p.info.basePipelineHandle=VK_NULL_HANDLE;p.info.basePipelineIndex=-1;
        p.stages.resize(input.stageCount);p.stageInfos.resize(input.stageCount);
        unsigned seen=0;
        for(uint32_t i=0;i<input.stageCount;++i)
        {
            auto& st=p.stages[i];st.info=input.pStages[i];
            const auto stage=st.info.stage;
            if((stage!=VK_SHADER_STAGE_VERTEX_BIT && stage!=VK_SHADER_STAGE_FRAGMENT_BIT && stage!=VK_SHADER_STAGE_GEOMETRY_BIT &&
                stage!=VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT && stage!=VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) || (seen&stage) || !st.info.pName) return false;
            seen|=stage;st.entry=st.info.pName;st.info.pName=st.entry.c_str();
            if(stage==VK_SHADER_STAGE_FRAGMENT_BIT) p.fragment=i;
            if(st.info.module)
            { auto m=s.modules.find(key(st.info.module));if(m==s.modules.end()) return false;st.code=m->second; }
            else
            {
                auto* chain=static_cast<const VkBaseInStructure*>(st.info.pNext);unsigned walked=0;
                for(;chain && walked<32;chain=chain->pNext,++walked)
                    if(chain->sType==VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO)
                    { auto& m=*reinterpret_cast<const VkShaderModuleCreateInfo*>(chain);st.code=code(s,m.pCode,m.codeSize);break; }
                if(!st.code) return false;
            }
            if(!st.chain.copy(st.info.pNext)) return false;st.info.pNext=st.chain.head();
            if(st.info.pSpecializationInfo)
            {
                st.specialization=*st.info.pSpecializationInfo;
                if(!array(st.entries,st.specialization.pMapEntries,st.specialization.mapEntryCount,4096) || st.specialization.dataSize>65536 ||
                    (st.specialization.dataSize&&!st.specialization.pData)) return false;
                const auto* b=static_cast<const uint8_t*>(st.specialization.pData);
                if(st.specialization.dataSize) st.constants.assign(b,b+st.specialization.dataSize);
                st.specialization.pData=st.constants.data();st.info.pSpecializationInfo=&st.specialization;
            }
            p.stageInfos[i]=st.info;
        }
        if(!(seen&VK_SHADER_STAGE_VERTEX_BIT) || !(seen&VK_SHADER_STAGE_FRAGMENT_BIT)) return false;
        p.info.pStages=p.stageInfos.data();
#define COPY_STATE(source,member,chain) if(input.source) { p.member=*input.source;if(!p.chain.copy(p.member.pNext)) return false;p.member.pNext=p.chain.head();p.info.source=&p.member; }
        COPY_STATE(pVertexInputState,vertex,vertexChain);
        COPY_STATE(pInputAssemblyState,assembly,assemblyChain);
        COPY_STATE(pTessellationState,tess,tessChain);
        COPY_STATE(pViewportState,viewport,viewportChain);
        COPY_STATE(pRasterizationState,raster,rasterChain);
        COPY_STATE(pMultisampleState,multi,multiChain);
        COPY_STATE(pDepthStencilState,depth,depthChain);
        COPY_STATE(pColorBlendState,blend,blendChain);
        COPY_STATE(pDynamicState,dynamic,dynamicChain);
#undef COPY_STATE
        if(!array(p.bindings,p.vertex.pVertexBindingDescriptions,p.vertex.vertexBindingDescriptionCount,64) ||
            !array(p.attributes,p.vertex.pVertexAttributeDescriptions,p.vertex.vertexAttributeDescriptionCount,64) ||
            !array(p.viewports,p.viewport.pViewports,p.viewport.viewportCount,32,true) ||
            !array(p.scissors,p.viewport.pScissors,p.viewport.scissorCount,32,true) ||
            !array(p.attachments,p.blend.pAttachments,p.blend.attachmentCount,1) ||
            !array(p.dynamics,p.dynamic.pDynamicStates,p.dynamic.dynamicStateCount,128)) return false;
        const uint32_t samples=static_cast<uint32_t>(p.multi.rasterizationSamples);
        if(input.pMultisampleState && (!samples || samples>64 || (samples&(samples-1)))) return false;
        if(!array(p.masks,p.multi.pSampleMask,(samples+31)/32,2,true)) return false;
        return true;
    }
    bool compatible(const Stage& stage)
    {
        const auto& words=stage.code->words;
        for(size_t i=5;i<words.size();)
        {
            const auto count=words[i]>>16,op=words[i]&65535;
            if(!count || count>words.size()-i) return false;
            if(op==SpvOpCapability && count>=2)
            {
                switch(words[i+1]) {
                case SpvCapabilityPhysicalStorageBufferAddresses:case SpvCapabilityAddresses:
                case SpvCapabilityRayQueryKHR:case SpvCapabilityRayTracingKHR:return false;
                default:break;
                }
            }
            i+=count;
        }
        SpvReflectShaderModule module{};
        if(spvReflectCreateShaderModule(words.size()*4,words.data(),&module)!=SPV_REFLECT_RESULT_SUCCESS) return false;
        struct Cleanup { SpvReflectShaderModule& m;~Cleanup(){spvReflectDestroyShaderModule(&m);} } cleanup{module};
        const auto* entry=spvReflectGetEntryPoint(&module,stage.entry.c_str());
        if(!entry || entry->shader_stage!=SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT || entry->output_variable_count!=1) return false;
        const auto* output=entry->output_variables[0];
        if(!output || output->location!=0 || output->member_count || output->array.dims_count || output->numeric.scalar.width!=32 ||
            (output->decoration_flags&SPV_REFLECT_DECORATION_BUILT_IN) || !output->type_description ||
            !(output->type_description->type_flags&SPV_REFLECT_TYPE_FLAG_FLOAT)) return false;
        for(size_t i=5;i<words.size();i+=words[i]>>16)
            if((words[i]&65535)==SpvOpDecorate && (words[i]>>16)>=4 && words[i+1]==output->spirv_id &&
                ((words[i+2]==SpvDecorationIndex && words[i+3]!=0) || words[i+2]==SpvDecorationComponent)) return false;
        uint32_t count=0;
        if(spvReflectEnumerateEntryPointDescriptorBindings(&module,stage.entry.c_str(),&count,nullptr)!=SPV_REFLECT_RESULT_SUCCESS || count>4096) return false;
        std::vector<SpvReflectDescriptorBinding*> bindings(count);
        if(count && spvReflectEnumerateEntryPointDescriptorBindings(&module,stage.entry.c_str(),&count,bindings.data())!=SPV_REFLECT_RESULT_SUCCESS) return false;
        for(const auto* b:bindings)
            switch(b->descriptor_type) {
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:break;
            default:return false;
            }
        return true;
    }
    std::vector<uint32_t> colour(Choice choice)
    {
        const auto c=modern::colourKey(choice);
        return {0x07230203,0x00010000,0,15,0,
            (2u<<16)|SpvOpCapability,SpvCapabilityShader,
            (3u<<16)|SpvOpMemoryModel,SpvAddressingModelLogical,SpvMemoryModelGLSL450,
            (6u<<16)|SpvOpEntryPoint,SpvExecutionModelFragment,12,0x6e69616d,0,6,
            (3u<<16)|SpvOpExecutionMode,12,SpvExecutionModeOriginUpperLeft,
            (4u<<16)|SpvOpDecorate,6,SpvDecorationLocation,0,
            (2u<<16)|SpvOpTypeVoid,1,
            (3u<<16)|SpvOpTypeFunction,2,1,
            (3u<<16)|SpvOpTypeFloat,3,32,
            (4u<<16)|SpvOpTypeVector,4,3,4,
            (4u<<16)|SpvOpTypePointer,5,SpvStorageClassOutput,4,
            (4u<<16)|SpvOpVariable,5,6,SpvStorageClassOutput,
            (4u<<16)|SpvOpConstant,3,7,c[0],(4u<<16)|SpvOpConstant,3,8,c[1],
            (4u<<16)|SpvOpConstant,3,9,c[2],(4u<<16)|SpvOpConstant,3,10,c[3],
            (7u<<16)|SpvOpConstantComposite,4,11,7,8,9,10,
            (5u<<16)|SpvOpFunction,1,12,SpvFunctionControlMaskNone,2,
            (2u<<16)|SpvOpLabel,13,(3u<<16)|SpvOpStore,6,11,
            (1u<<16)|SpvOpReturn,(1u<<16)|SpvOpFunctionEnd};
    }
    void report(State& s,uint32_t hash,Choice choice,bool applied,const char* reason)
    {
        if(!s.statuses.count(hash)&&s.statuses.size()>=4096) return;
        auto& old=s.statuses[hash];
        if(!old.attempted || old.choice!=choice || old.applied!=applied || old.reason!=reason)
            diagnostics::Write("[SMART] Vulkan shader=%08X method=%s applied=%u: %s",hash,name(choice.method),applied,reason);
        old={choice,true,applied,reason};
    }
    VkPipeline replacement(State& s,const std::shared_ptr<Pipeline>& p,Choice choice)
    {
        const auto key=modern::colourKey(choice);
        if(auto i=p->variants.find(key);i!=p->variants.end()) return i->second;
        if(s.retained.size()>=512) return VK_NULL_HANDLE;
        auto& result=p->variants[key];
        s.retained.emplace_back(VK_NULL_HANDLE,p);
        Internal own;
        auto stages=p->stageInfos;
        std::vector<VkShaderModule> modules;
        struct Cleanup { State& s;std::vector<VkShaderModule>& m;~Cleanup(){for(auto v:m)s.f.DestroyShaderModule(s.deviceHandle,v,nullptr);} } cleanup{s,modules};
        auto constant=colour(choice);
        for(size_t i=0;i<stages.size();++i)
        {
            const auto& words=i==p->fragment?constant:p->stages[i].code->words;
            VkShaderModuleCreateInfo info{VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO};info.codeSize=words.size()*4;info.pCode=words.data();
            VkShaderModule shader=VK_NULL_HANDLE;
            const auto hr=s.f.CreateShaderModule(s.deviceHandle,&info,nullptr,&shader);
            if(hr!=VK_SUCCESS || !shader) return VK_NULL_HANDLE;
            modules.push_back(shader);stages[i].module=shader;
            if(i==p->fragment) {stages[i].pName="main";stages[i].pSpecializationInfo=nullptr;}
        }
        auto info=p->info;info.pStages=stages.data();
        const auto hr=s.f.CreateGraphicsPipelines(s.deviceHandle,VK_NULL_HANDLE,1,&info,nullptr,&result);
        if(hr!=VK_SUCCESS)
        {
            if(result) s.f.DestroyPipeline(s.deviceHandle,result,nullptr);result=VK_NULL_HANDLE;
            diagnostics::Write("[ERROR] SMART Vulkan replacement pipeline creation failed VkResult=%d",static_cast<int>(hr));
        }
        s.retained.back().first=result;return result;
    }
    template<class Fn> Fn fallback(VkDevice d,const char* name)
    {
        if(getDeviceProc) if(auto p=getDeviceProc(d,name))
            return reinterpret_cast<Fn>(unhooked(reinterpret_cast<void*>(p)));
        return loader?reinterpret_cast<Fn>(unhooked(reinterpret_cast<void*>(GetProcAddress(loader,name)))):nullptr;
    }
#define GET_ORIGINAL(n) auto s=find(deviceHandle);if(s)resolve(*s);auto original=s&&s->f.n?reinterpret_cast<PFN_vk ##n>(unhooked(reinterpret_cast<void*>(s->f.n))):fallback<PFN_vk ##n>(deviceHandle,"vk" #n)
    VkResult VKAPI_CALL createShaderModule(VkDevice deviceHandle,const VkShaderModuleCreateInfo* info,const VkAllocationCallbacks* allocator,VkShaderModule* output)
    {
        GET_ORIGINAL(CreateShaderModule);if(!original)return VK_ERROR_INITIALIZATION_FAILED;
        auto result=original(deviceHandle,info,allocator,output);
        if(result==VK_SUCCESS && s && s->ready && !internal()) try
        {
            std::lock_guard lock(s->mutex);s->modules.erase(key(*output));
            if(!info->pNext && s->modules.size()<65536)
                if(auto c=code(*s,info->pCode,info->codeSize))s->modules[key(*output)]=std::move(c);
        }catch(...){diagnostics::Write("[ERROR] SMART Vulkan shader capture failed; original retained.");}
        return result;
    }
    void VKAPI_CALL destroyShaderModule(VkDevice deviceHandle,VkShaderModule object,const VkAllocationCallbacks* allocator)
    {
        GET_ORIGINAL(DestroyShaderModule);
        if(s&&!internal()){std::lock_guard lock(s->mutex);s->modules.erase(key(object));}
        if(original)original(deviceHandle,object,allocator);
    }
    VkResult VKAPI_CALL createPipelineLayout(VkDevice deviceHandle,const VkPipelineLayoutCreateInfo* info,const VkAllocationCallbacks* allocator,VkPipelineLayout* output)
    {
        GET_ORIGINAL(CreatePipelineLayout);if(!original)return VK_ERROR_INITIALIZATION_FAILED;
        auto result=original(deviceHandle,info,allocator,output);
        if(result==VK_SUCCESS && s && s->ready && !internal()) try
        {
            std::lock_guard lock(s->mutex);s->layouts.erase(key(*output));
            if(s->layouts.size()<4096)
            {
                auto copy=std::make_shared<Dependency>();copy->device=deviceHandle;copy->destroyLayout=s->f.DestroyPipelineLayout;
                Internal own;
                if(original(deviceHandle,info,nullptr,&copy->layout)==VK_SUCCESS)s->layouts[key(*output)]=std::move(copy);
            }
        }catch(...){diagnostics::Write("[ERROR] SMART Vulkan layout capture failed; original retained.");}
        return result;
    }
    void VKAPI_CALL destroyPipelineLayout(VkDevice deviceHandle,VkPipelineLayout object,const VkAllocationCallbacks* allocator)
    {
        GET_ORIGINAL(DestroyPipelineLayout);
        if(s&&!internal()){std::lock_guard lock(s->mutex);s->layouts.erase(key(object));}
        if(original)original(deviceHandle,object,allocator);
    }
    template<class Info,class Fn> void copyPass(State& s,const Info* info,VkRenderPass original,Fn create)
    {
        std::lock_guard lock(s.mutex);s.passes.erase(key(original));
        if(s.passes.size()>=4096)return;
        auto copy=std::make_shared<Dependency>();copy->device=s.deviceHandle;copy->destroyPass=s.f.DestroyRenderPass;
        Internal own;
        if(create(s.deviceHandle,info,nullptr,&copy->pass)==VK_SUCCESS)s.passes[key(original)]=std::move(copy);
    }
    VkResult VKAPI_CALL createRenderPass(VkDevice deviceHandle,const VkRenderPassCreateInfo* info,const VkAllocationCallbacks* allocator,VkRenderPass* output)
    {
        GET_ORIGINAL(CreateRenderPass);if(!original)return VK_ERROR_INITIALIZATION_FAILED;
        auto result=original(deviceHandle,info,allocator,output);
        if(result==VK_SUCCESS&&s&&s->ready&&!internal())try{copyPass(*s,info,*output,original);}catch(...){diagnostics::Write("[ERROR] SMART Vulkan render-pass capture failed.");}
        return result;
    }
    VkResult VKAPI_CALL createRenderPass2(VkDevice deviceHandle,const VkRenderPassCreateInfo2* info,const VkAllocationCallbacks* allocator,VkRenderPass* output)
    {
        GET_ORIGINAL(CreateRenderPass2);if(!original)return VK_ERROR_INITIALIZATION_FAILED;
        auto result=original(deviceHandle,info,allocator,output);
        if(result==VK_SUCCESS&&s&&s->ready&&!internal())try{copyPass(*s,info,*output,original);}catch(...){diagnostics::Write("[ERROR] SMART Vulkan render-pass-2 capture failed.");}
        return result;
    }
    VkResult VKAPI_CALL createRenderPass2KHR(VkDevice deviceHandle,const VkRenderPassCreateInfo2* info,const VkAllocationCallbacks* allocator,VkRenderPass* output)
    {
        GET_ORIGINAL(CreateRenderPass2KHR);if(!original)return VK_ERROR_INITIALIZATION_FAILED;
        auto result=original(deviceHandle,info,allocator,output);
        if(result==VK_SUCCESS&&s&&s->ready&&!internal())try{copyPass(*s,info,*output,original);}catch(...){diagnostics::Write("[ERROR] SMART Vulkan render-pass-2 KHR capture failed.");}
        return result;
    }
    void VKAPI_CALL destroyRenderPass(VkDevice deviceHandle,VkRenderPass object,const VkAllocationCallbacks* allocator)
    {
        GET_ORIGINAL(DestroyRenderPass);
        if(s&&!internal()){std::lock_guard lock(s->mutex);s->passes.erase(key(object));}
        if(original)original(deviceHandle,object,allocator);
    }
    VkResult VKAPI_CALL createGraphicsPipelines(VkDevice deviceHandle,VkPipelineCache cache,uint32_t count,const VkGraphicsPipelineCreateInfo* infos,const VkAllocationCallbacks* allocator,VkPipeline* output)
    {
        GET_ORIGINAL(CreateGraphicsPipelines);if(!original)return VK_ERROR_INITIALIZATION_FAILED;
        auto result=original(deviceHandle,cache,count,infos,allocator,output);
        if(s&&s->ready&&!internal()&&output&&count<=65536)try
        {
            std::lock_guard lock(s->mutex);
            for(uint32_t i=0;i<count;++i)if(output[i])
            {
                s->pipelines.erase(key(output[i]));
                auto copy=std::make_shared<Pipeline>();
                if(s->pipelines.size()<32768&&snapshot(*s,*copy,infos[i])){s->pipelines[key(output[i])]=std::move(copy);++s->captured;}
                else ++s->declined;
            }
        }catch(...){diagnostics::Write("[ERROR] SMART Vulkan pipeline capture failed; original retained.");}
        return result;
    }
    void VKAPI_CALL destroyPipeline(VkDevice deviceHandle,VkPipeline object,const VkAllocationCallbacks* allocator)
    {
        GET_ORIGINAL(DestroyPipeline);
        if(s&&!internal()){std::lock_guard lock(s->mutex);s->pipelines.erase(key(object));}
        if(original)original(deviceHandle,object,allocator);
    }
#undef GET_ORIGINAL
    void* callback(const char* name)
    {
#define STA_VK_CALLBACK(n,fn) if(std::strcmp(name,"vk" #n)==0)return reinterpret_cast<void*>(&fn)
        STA_VK_CALLBACK(CreateShaderModule,createShaderModule);STA_VK_CALLBACK(DestroyShaderModule,destroyShaderModule);
        STA_VK_CALLBACK(CreatePipelineLayout,createPipelineLayout);STA_VK_CALLBACK(DestroyPipelineLayout,destroyPipelineLayout);
        STA_VK_CALLBACK(CreateRenderPass,createRenderPass);STA_VK_CALLBACK(CreateRenderPass2,createRenderPass2);STA_VK_CALLBACK(CreateRenderPass2KHR,createRenderPass2KHR);STA_VK_CALLBACK(DestroyRenderPass,destroyRenderPass);
        STA_VK_CALLBACK(CreateGraphicsPipelines,createGraphicsPipelines);STA_VK_CALLBACK(DestroyPipeline,destroyPipeline);
#undef STA_VK_CALLBACK
        return nullptr;
    }
    bool install(void* target,void* replacement,void** out=nullptr);
    PFN_vkVoidFunction VKAPI_CALL deviceProc(VkDevice d,const char* name)
    {
        auto fn=getDeviceProc(d,name);
        if(fn && !internal())if(auto s=find(d))
        {
            if(auto replacement=callback(name))
                install(reinterpret_cast<void*>(fn),replacement);
            resolve(*s);
        }
        return fn;
    }
    bool install(void* target,void* replacement,void** out)
    {
        std::lock_guard lock(registryMutex);
        if(!target)return false;
        if(auto i=exportOriginals.find(target);i!=exportOriginals.end()){if(out)*out=i->second;return true;}
        HMODULE module=nullptr;
        if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<const wchar_t*>(target),&module))
            return false;
        void* original=nullptr;auto result=MH_CreateHook(target,replacement,&original);
        if(result!=MH_OK){FreeLibrary(module);diagnostics::Write("[ERROR] SMART Vulkan capture hook creation failed status=%d",static_cast<int>(result));return false;}
        exportOriginals[target]=original;if(out)*out=original;
        result=MH_EnableHook(target);
        if(result!=MH_OK){MH_RemoveHook(target);exportOriginals.erase(target);if(out)*out=nullptr;FreeLibrary(module);return false;}
        hookTargets.push_back(target);hookModules.push_back(module);return true;
    }
    void initDevice(device* dev)
    {
        if(!isVK(dev))return;
        auto value=std::make_shared<State>();value->owner=dev;value->deviceHandle=(VkDevice)(uintptr_t)dev->get_native();
        dev->create_private_data<DeviceData>().state=value;
        std::lock_guard lock(registryMutex);devices[value->deviceHandle]=value;
        if(!hooksReady)
        {
            loader=GetModuleHandleW(L"vulkan-1.dll");
            auto result=MH_Initialize();
            if(!loader || (result!=MH_OK&&result!=MH_ERROR_ALREADY_INITIALIZED))return;
            if(!install(reinterpret_cast<void*>(GetProcAddress(loader,"vkGetDeviceProcAddr")),reinterpret_cast<void*>(&deviceProc),reinterpret_cast<void**>(&getDeviceProc)))return;
            const char* names[]{"vkCreateShaderModule","vkDestroyShaderModule","vkCreatePipelineLayout","vkDestroyPipelineLayout",
                "vkCreateRenderPass","vkCreateRenderPass2","vkCreateRenderPass2KHR","vkDestroyRenderPass","vkCreateGraphicsPipelines","vkDestroyPipeline"};
            for(auto n:names)if(auto target=GetProcAddress(loader,n))install(reinterpret_cast<void*>(target),callback(n));
            hooksReady=true;
        }
        diagnostics::Write("[SMART] Vulkan native capture initialized; constant SPIR-V shaders require no compiler DLL.");
    }
    void destroyDevice(device* dev)
    {
        if(!isVK(dev))return;
        auto handle=(VkDevice)(uintptr_t)dev->get_native();
        {std::lock_guard lock(registryMutex);devices.erase(handle);}
        dev->destroy_private_data<DeviceData>();
    }
    void initCommands(command_list* cmd){if(isVK(cmd->get_device()))cmd->create_private_data<Commands>();}
    void destroyCommands(command_list* cmd){if(isVK(cmd->get_device()))cmd->destroy_private_data<Commands>();}
    void reset(command_list* cmd){if(cmd&&isVK(cmd->get_device()))cmd->get_private_data<Commands>()={};}
    void bound(command_list* cmd,reshade::api::pipeline_stage stages,uint64_t handle)
    {
        if(!cmd||!isVK(cmd->get_device())||internal())return;
        if((stages&reshade::api::pipeline_stage::pixel_shader)==reshade::api::pipeline_stage::pixel_shader)
        {auto& d=cmd->get_private_data<Commands>();d.graphics=handle;d.restore=0;}
    }
    void restore(command_list* cmd)
    {
        if(!cmd||!isVK(cmd->get_device())||internal())return;
        auto& d=cmd->get_private_data<Commands>();if(!d.restore)return;
        auto& s=state(cmd->get_device());Internal own;
        if(s.f.CmdBindPipeline)s.f.CmdBindPipeline((VkCommandBuffer)(uintptr_t)cmd->get_native(),VK_PIPELINE_BIND_POINT_GRAPHICS,(VkPipeline)d.restore);
        d.restore=0;
    }
    void beginPass(command_list* cmd,uint32_t count,const reshade::api::render_pass_render_target_desc* targets,const reshade::api::render_pass_depth_stencil_desc*)
    {
        if(!isVK(cmd->get_device())||internal())return;
        restore(cmd);auto& d=cmd->get_private_data<Commands>();
        d.singleTarget=count==1&&targets&&targets[0].view.handle;
    }
    void endPass(command_list* cmd){if(isVK(cmd->get_device())&&!internal()){restore(cmd);cmd->get_private_data<Commands>().singleTarget=false;}}
    bool apply(command_list* cmd,uint64_t handle,uint32_t hash,Choice choice)
    {
        if(!cmd||!isVK(cmd->get_device())||internal()||!valid(choice))return false;
        auto& s=state(cmd->get_device());std::lock_guard lock(s.mutex);auto& d=cmd->get_private_data<Commands>();
        auto fail=[&](const char* why){report(s,hash,choice,false,why);return false;};
        if(!d.singleTarget||!handle||d.graphics!=handle)return fail("This Vulkan pass needs one known colour target; original pass retained.");
        auto found=s.pipelines.find(handle);
        if(!s.ready||found==s.pipelines.end())return fail("Vulkan pipeline state was not captured or uses unsupported extensions; original pass retained.");
        auto p=found->second;
        if(!p->checked){p->checked=true;p->compatible=compatible(p->stages[p->fragment]);}
        if(!p->compatible)return fail("SPIR-V outputs or writable resources are unsupported; original pass retained.");
        auto replacementPipeline=replacement(s,p,choice);
        if(!replacementPipeline)return fail("Vulkan replacement could not be created or its cache is full; original pass retained.");
        {Internal own;s.f.CmdBindPipeline((VkCommandBuffer)(uintptr_t)cmd->get_native(),VK_PIPELINE_BIND_POINT_GRAPHICS,replacementPipeline);}
        d.restore=handle;report(s,hash,choice,true,"Colour replacement is running.");return true;
    }
    void observe(command_list* cmd,uint64_t handle,uint32_t hash)
    {
        auto& s=state(cmd->get_device());std::lock_guard lock(s.mutex);
        if(s.suggestions.count(hash)||s.suggestions.size()>=4096)return;
        auto i=s.pipelines.find(handle);if(i==s.pipelines.end()||i->second->attachments.empty())return;
        auto& b=i->second->attachments[0];Method method=Method::TransparentBlack;
        if(b.blendEnable&&b.colorBlendOp==VK_BLEND_OP_ADD&&(b.srcColorBlendFactor==VK_BLEND_FACTOR_DST_COLOR||b.dstColorBlendFactor==VK_BLEND_FACTOR_SRC_COLOR))method=Method::White;
        s.suggestions[hash]=method;
    }
    Status status(device* dev,uint32_t hash){auto& s=state(dev);std::lock_guard lock(s.mutex);auto i=s.statuses.find(hash);return i==s.statuses.end()?Status{}:i->second;}
    Method suggestion(device* dev,uint32_t hash){auto& s=state(dev);std::lock_guard lock(s.mutex);auto i=s.suggestions.find(hash);return i==s.suggestions.end()?Method::TransparentBlack:i->second;}
    void summary(device* dev)
    {
        if(!isVK(dev))return;auto& s=state(dev);std::unique_lock lock(s.mutex,std::try_to_lock);
        if(lock.owns_lock())diagnostics::Write("[SMART] Vulkan captured=%zu declined=%zu live=%zu code_bytes=%zu replacement_attempts=%zu",s.captured,s.declined,s.pipelines.size(),s.budget->bytes,s.retained.size());
    }
    void shutdown(bool processExit)
    {
        if(processExit)return;
        std::vector<std::shared_ptr<State>> live;
        {std::lock_guard lock(registryMutex);for(auto& [handle,s]:devices)live.push_back(s);}
        for(auto& s:live){resolve(*s);if(s->f.DeviceWaitIdle)s->f.DeviceWaitIdle(s->deviceHandle);}
        {std::lock_guard lock(registryMutex);devices.clear();}
        for(auto& s:live)s->owner->destroy_private_data<DeviceData>();live.clear();
        for(auto p:hookTargets)MH_DisableHook(p);
        for(auto p:hookTargets)MH_RemoveHook(p);
        hookTargets.clear();exportOriginals.clear();hooksReady=false;getDeviceProc=nullptr;
        for(auto module:hookModules)FreeLibrary(module);hookModules.clear();loader=nullptr;
    }
}
