/*
 Copyright 2017-2022 Google Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/
#ifndef SPIRV_REFLECT_H
#define SPIRV_REFLECT_H 
#if defined(SPIRV_REFLECT_USE_SYSTEM_SPIRV_H)
#include <spirv/unified1/spirv.h>
#else
#include "./include/spirv/unified1/spirv.h"
#endif
#include <stdint.h>
#include <string.h>
#ifdef _MSC_VER
  #define SPV_REFLECT_DEPRECATED(msg_str) __declspec(deprecated("This symbol is deprecated. Details: " msg_str))
#elif defined(__clang__)
  #define SPV_REFLECT_DEPRECATED(msg_str) __attribute__((deprecated(msg_str)))
#elif defined(__GNUC__)
  #if GCC_VERSION >= 40500
    #define SPV_REFLECT_DEPRECATED(msg_str) __attribute__((deprecated(msg_str)))
  #else
    #define SPV_REFLECT_DEPRECATED(msg_str) __attribute__((deprecated))
  #endif
#else
  #define SPV_REFLECT_DEPRECATED(msg_str)
#endif
typedef enum SpvReflectResult {
  SPV_REFLECT_RESULT_SUCCESS,
  SPV_REFLECT_RESULT_NOT_READY,
  SPV_REFLECT_RESULT_ERROR_PARSE_FAILED,
  SPV_REFLECT_RESULT_ERROR_ALLOC_FAILED,
  SPV_REFLECT_RESULT_ERROR_RANGE_EXCEEDED,
  SPV_REFLECT_RESULT_ERROR_NULL_POINTER,
  SPV_REFLECT_RESULT_ERROR_INTERNAL_ERROR,
  SPV_REFLECT_RESULT_ERROR_COUNT_MISMATCH,
  SPV_REFLECT_RESULT_ERROR_ELEMENT_NOT_FOUND,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_CODE_SIZE,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_MAGIC_NUMBER,
  SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_EOF,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ID_REFERENCE,
  SPV_REFLECT_RESULT_ERROR_SPIRV_SET_NUMBER_OVERFLOW,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_STORAGE_CLASS,
  SPV_REFLECT_RESULT_ERROR_SPIRV_RECURSION,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_INSTRUCTION,
  SPV_REFLECT_RESULT_ERROR_SPIRV_UNEXPECTED_BLOCK_DATA,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_BLOCK_MEMBER_REFERENCE,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_ENTRY_POINT,
  SPV_REFLECT_RESULT_ERROR_SPIRV_INVALID_EXECUTION_MODE,
  SPV_REFLECT_RESULT_ERROR_SPIRV_MAX_RECURSIVE_EXCEEDED,
} SpvReflectResult;
typedef enum SpvReflectModuleFlagBits {
  SPV_REFLECT_MODULE_FLAG_NONE = 0x00000000,
  SPV_REFLECT_MODULE_FLAG_NO_COPY = 0x00000001,
} SpvReflectModuleFlagBits;
typedef uint32_t SpvReflectModuleFlags;
typedef enum SpvReflectTypeFlagBits {
  SPV_REFLECT_TYPE_FLAG_UNDEFINED = 0x00000000,
  SPV_REFLECT_TYPE_FLAG_VOID = 0x00000001,
  SPV_REFLECT_TYPE_FLAG_BOOL = 0x00000002,
  SPV_REFLECT_TYPE_FLAG_INT = 0x00000004,
  SPV_REFLECT_TYPE_FLAG_FLOAT = 0x00000008,
  SPV_REFLECT_TYPE_FLAG_VECTOR = 0x00000100,
  SPV_REFLECT_TYPE_FLAG_MATRIX = 0x00000200,
  SPV_REFLECT_TYPE_FLAG_EXTERNAL_IMAGE = 0x00010000,
  SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLER = 0x00020000,
  SPV_REFLECT_TYPE_FLAG_EXTERNAL_SAMPLED_IMAGE = 0x00040000,
  SPV_REFLECT_TYPE_FLAG_EXTERNAL_BLOCK = 0x00080000,
  SPV_REFLECT_TYPE_FLAG_EXTERNAL_ACCELERATION_STRUCTURE = 0x00100000,
  SPV_REFLECT_TYPE_FLAG_EXTERNAL_MASK = 0x00FF0000,
  SPV_REFLECT_TYPE_FLAG_STRUCT = 0x10000000,
  SPV_REFLECT_TYPE_FLAG_ARRAY = 0x20000000,
  SPV_REFLECT_TYPE_FLAG_REF = 0x40000000,
} SpvReflectTypeFlagBits;
typedef uint32_t SpvReflectTypeFlags;
typedef enum SpvReflectDecorationFlagBits {
  SPV_REFLECT_DECORATION_NONE = 0x00000000,
  SPV_REFLECT_DECORATION_BLOCK = 0x00000001,
  SPV_REFLECT_DECORATION_BUFFER_BLOCK = 0x00000002,
  SPV_REFLECT_DECORATION_ROW_MAJOR = 0x00000004,
  SPV_REFLECT_DECORATION_COLUMN_MAJOR = 0x00000008,
  SPV_REFLECT_DECORATION_BUILT_IN = 0x00000010,
  SPV_REFLECT_DECORATION_NOPERSPECTIVE = 0x00000020,
  SPV_REFLECT_DECORATION_FLAT = 0x00000040,
  SPV_REFLECT_DECORATION_NON_WRITABLE = 0x00000080,
  SPV_REFLECT_DECORATION_RELAXED_PRECISION = 0x00000100,
  SPV_REFLECT_DECORATION_NON_READABLE = 0x00000200,
  SPV_REFLECT_DECORATION_PATCH = 0x00000400,
  SPV_REFLECT_DECORATION_PER_VERTEX = 0x00000800,
  SPV_REFLECT_DECORATION_PER_TASK = 0x00001000,
  SPV_REFLECT_DECORATION_WEIGHT_TEXTURE = 0x00002000,
  SPV_REFLECT_DECORATION_BLOCK_MATCH_TEXTURE = 0x00004000,
} SpvReflectDecorationFlagBits;
typedef uint32_t SpvReflectDecorationFlags;
typedef enum SpvReflectUserType {
  SPV_REFLECT_USER_TYPE_INVALID = 0,
  SPV_REFLECT_USER_TYPE_CBUFFER,
  SPV_REFLECT_USER_TYPE_TBUFFER,
  SPV_REFLECT_USER_TYPE_APPEND_STRUCTURED_BUFFER,
  SPV_REFLECT_USER_TYPE_BUFFER,
  SPV_REFLECT_USER_TYPE_BYTE_ADDRESS_BUFFER,
  SPV_REFLECT_USER_TYPE_CONSTANT_BUFFER,
  SPV_REFLECT_USER_TYPE_CONSUME_STRUCTURED_BUFFER,
  SPV_REFLECT_USER_TYPE_INPUT_PATCH,
  SPV_REFLECT_USER_TYPE_OUTPUT_PATCH,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_BUFFER,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_BYTE_ADDRESS_BUFFER,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_STRUCTURED_BUFFER,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_TEXTURE_1D,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_TEXTURE_1D_ARRAY,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_TEXTURE_2D,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_TEXTURE_2D_ARRAY,
  SPV_REFLECT_USER_TYPE_RASTERIZER_ORDERED_TEXTURE_3D,
  SPV_REFLECT_USER_TYPE_RAYTRACING_ACCELERATION_STRUCTURE,
  SPV_REFLECT_USER_TYPE_RW_BUFFER,
  SPV_REFLECT_USER_TYPE_RW_BYTE_ADDRESS_BUFFER,
  SPV_REFLECT_USER_TYPE_RW_STRUCTURED_BUFFER,
  SPV_REFLECT_USER_TYPE_RW_TEXTURE_1D,
  SPV_REFLECT_USER_TYPE_RW_TEXTURE_1D_ARRAY,
  SPV_REFLECT_USER_TYPE_RW_TEXTURE_2D,
  SPV_REFLECT_USER_TYPE_RW_TEXTURE_2D_ARRAY,
  SPV_REFLECT_USER_TYPE_RW_TEXTURE_3D,
  SPV_REFLECT_USER_TYPE_STRUCTURED_BUFFER,
  SPV_REFLECT_USER_TYPE_SUBPASS_INPUT,
  SPV_REFLECT_USER_TYPE_SUBPASS_INPUT_MS,
  SPV_REFLECT_USER_TYPE_TEXTURE_1D,
  SPV_REFLECT_USER_TYPE_TEXTURE_1D_ARRAY,
  SPV_REFLECT_USER_TYPE_TEXTURE_2D,
  SPV_REFLECT_USER_TYPE_TEXTURE_2D_ARRAY,
  SPV_REFLECT_USER_TYPE_TEXTURE_2DMS,
  SPV_REFLECT_USER_TYPE_TEXTURE_2DMS_ARRAY,
  SPV_REFLECT_USER_TYPE_TEXTURE_3D,
  SPV_REFLECT_USER_TYPE_TEXTURE_BUFFER,
  SPV_REFLECT_USER_TYPE_TEXTURE_CUBE,
  SPV_REFLECT_USER_TYPE_TEXTURE_CUBE_ARRAY,
} SpvReflectUserType;
typedef enum SpvReflectResourceType {
  SPV_REFLECT_RESOURCE_FLAG_UNDEFINED = 0x00000000,
  SPV_REFLECT_RESOURCE_FLAG_SAMPLER = 0x00000001,
  SPV_REFLECT_RESOURCE_FLAG_CBV = 0x00000002,
  SPV_REFLECT_RESOURCE_FLAG_SRV = 0x00000004,
  SPV_REFLECT_RESOURCE_FLAG_UAV = 0x00000008,
} SpvReflectResourceType;
typedef enum SpvReflectFormat {
  SPV_REFLECT_FORMAT_UNDEFINED = 0,
  SPV_REFLECT_FORMAT_R16_UINT = 74,
  SPV_REFLECT_FORMAT_R16_SINT = 75,
  SPV_REFLECT_FORMAT_R16_SFLOAT = 76,
  SPV_REFLECT_FORMAT_R16G16_UINT = 81,
  SPV_REFLECT_FORMAT_R16G16_SINT = 82,
  SPV_REFLECT_FORMAT_R16G16_SFLOAT = 83,
  SPV_REFLECT_FORMAT_R16G16B16_UINT = 88,
  SPV_REFLECT_FORMAT_R16G16B16_SINT = 89,
  SPV_REFLECT_FORMAT_R16G16B16_SFLOAT = 90,
  SPV_REFLECT_FORMAT_R16G16B16A16_UINT = 95,
  SPV_REFLECT_FORMAT_R16G16B16A16_SINT = 96,
  SPV_REFLECT_FORMAT_R16G16B16A16_SFLOAT = 97,
  SPV_REFLECT_FORMAT_R32_UINT = 98,
  SPV_REFLECT_FORMAT_R32_SINT = 99,
  SPV_REFLECT_FORMAT_R32_SFLOAT = 100,
  SPV_REFLECT_FORMAT_R32G32_UINT = 101,
  SPV_REFLECT_FORMAT_R32G32_SINT = 102,
  SPV_REFLECT_FORMAT_R32G32_SFLOAT = 103,
  SPV_REFLECT_FORMAT_R32G32B32_UINT = 104,
  SPV_REFLECT_FORMAT_R32G32B32_SINT = 105,
  SPV_REFLECT_FORMAT_R32G32B32_SFLOAT = 106,
  SPV_REFLECT_FORMAT_R32G32B32A32_UINT = 107,
  SPV_REFLECT_FORMAT_R32G32B32A32_SINT = 108,
  SPV_REFLECT_FORMAT_R32G32B32A32_SFLOAT = 109,
  SPV_REFLECT_FORMAT_R64_UINT = 110,
  SPV_REFLECT_FORMAT_R64_SINT = 111,
  SPV_REFLECT_FORMAT_R64_SFLOAT = 112,
  SPV_REFLECT_FORMAT_R64G64_UINT = 113,
  SPV_REFLECT_FORMAT_R64G64_SINT = 114,
  SPV_REFLECT_FORMAT_R64G64_SFLOAT = 115,
  SPV_REFLECT_FORMAT_R64G64B64_UINT = 116,
  SPV_REFLECT_FORMAT_R64G64B64_SINT = 117,
  SPV_REFLECT_FORMAT_R64G64B64_SFLOAT = 118,
  SPV_REFLECT_FORMAT_R64G64B64A64_UINT = 119,
  SPV_REFLECT_FORMAT_R64G64B64A64_SINT = 120,
  SPV_REFLECT_FORMAT_R64G64B64A64_SFLOAT = 121,
} SpvReflectFormat;
enum SpvReflectVariableFlagBits{
  SPV_REFLECT_VARIABLE_FLAGS_NONE = 0x00000000,
  SPV_REFLECT_VARIABLE_FLAGS_UNUSED = 0x00000001,
  SPV_REFLECT_VARIABLE_FLAGS_PHYSICAL_POINTER_COPY = 0x00000002,
};
typedef uint32_t SpvReflectVariableFlags;
typedef enum SpvReflectDescriptorType {
  SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER = 0,
  SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
  SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
  SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
  SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
  SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
  SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
  SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
  SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
  SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
  SPV_REFLECT_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10,
  SPV_REFLECT_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR = 1000150000
} SpvReflectDescriptorType;
typedef enum SpvReflectShaderStageFlagBits {
  SPV_REFLECT_SHADER_STAGE_VERTEX_BIT = 0x00000001,
  SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT = 0x00000002,
  SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT = 0x00000004,
  SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT = 0x00000008,
  SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT = 0x00000010,
  SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT = 0x00000020,
  SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV = 0x00000040,
  SPV_REFLECT_SHADER_STAGE_TASK_BIT_EXT = SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV,
  SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV = 0x00000080,
  SPV_REFLECT_SHADER_STAGE_MESH_BIT_EXT = SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV,
  SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR = 0x00000100,
  SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR = 0x00000200,
  SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR = 0x00000400,
  SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR = 0x00000800,
  SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR = 0x00001000,
  SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR = 0x00002000,
} SpvReflectShaderStageFlagBits;
typedef enum SpvReflectGenerator {
  SPV_REFLECT_GENERATOR_KHRONOS_LLVM_SPIRV_TRANSLATOR = 6,
  SPV_REFLECT_GENERATOR_KHRONOS_SPIRV_TOOLS_ASSEMBLER = 7,
  SPV_REFLECT_GENERATOR_KHRONOS_GLSLANG_REFERENCE_FRONT_END = 8,
  SPV_REFLECT_GENERATOR_GOOGLE_SHADERC_OVER_GLSLANG = 13,
  SPV_REFLECT_GENERATOR_GOOGLE_SPIREGG = 14,
  SPV_REFLECT_GENERATOR_GOOGLE_RSPIRV = 15,
  SPV_REFLECT_GENERATOR_X_LEGEND_MESA_MESAIR_SPIRV_TRANSLATOR = 16,
  SPV_REFLECT_GENERATOR_KHRONOS_SPIRV_TOOLS_LINKER = 17,
  SPV_REFLECT_GENERATOR_WINE_VKD3D_SHADER_COMPILER = 18,
  SPV_REFLECT_GENERATOR_CLAY_CLAY_SHADER_COMPILER = 19,
} SpvReflectGenerator;
enum {
  SPV_REFLECT_MAX_ARRAY_DIMS = 32,
  SPV_REFLECT_MAX_DESCRIPTOR_SETS = 64,
};
enum {
  SPV_REFLECT_BINDING_NUMBER_DONT_CHANGE = ~0,
  SPV_REFLECT_SET_NUMBER_DONT_CHANGE = ~0
};
typedef struct SpvReflectNumericTraits {
  struct Scalar {
    uint32_t width;
    uint32_t signedness;
  } scalar;
  struct Vector {
    uint32_t component_count;
  } vector;
  struct Matrix {
    uint32_t column_count;
    uint32_t row_count;
    uint32_t stride;
  } matrix;
} SpvReflectNumericTraits;
typedef struct SpvReflectImageTraits {
  SpvDim dim;
  uint32_t depth;
  uint32_t arrayed;
  uint32_t ms;
  uint32_t sampled;
  SpvImageFormat image_format;
} SpvReflectImageTraits;
typedef enum SpvReflectArrayDimType {
  SPV_REFLECT_ARRAY_DIM_RUNTIME = 0,
} SpvReflectArrayDimType;
typedef struct SpvReflectArrayTraits {
  uint32_t dims_count;
  uint32_t dims[SPV_REFLECT_MAX_ARRAY_DIMS];
  uint32_t spec_constant_op_ids[SPV_REFLECT_MAX_ARRAY_DIMS];
  uint32_t stride;
} SpvReflectArrayTraits;
typedef struct SpvReflectBindingArrayTraits {
  uint32_t dims_count;
  uint32_t dims[SPV_REFLECT_MAX_ARRAY_DIMS];
} SpvReflectBindingArrayTraits;
typedef struct SpvReflectTypeDescription {
  uint32_t id;
  SpvOp op;
  const char* type_name;
  const char* struct_member_name;
  SpvStorageClass storage_class;
  SpvReflectTypeFlags type_flags;
  SpvReflectDecorationFlags decoration_flags;
  struct Traits {
    SpvReflectNumericTraits numeric;
    SpvReflectImageTraits image;
    SpvReflectArrayTraits array;
  } traits;
  struct SpvReflectTypeDescription* struct_type_description;
  uint32_t copied;
  uint32_t member_count;
  struct SpvReflectTypeDescription* members;
} SpvReflectTypeDescription;
typedef struct SpvReflectInterfaceVariable {
  uint32_t spirv_id;
  const char* name;
  uint32_t location;
  uint32_t component;
  SpvStorageClass storage_class;
  const char* semantic;
  SpvReflectDecorationFlags decoration_flags;
  SpvBuiltIn built_in;
  SpvReflectNumericTraits numeric;
  SpvReflectArrayTraits array;
  uint32_t member_count;
  struct SpvReflectInterfaceVariable* members;
  SpvReflectFormat format;
  SpvReflectTypeDescription* type_description;
  struct {
    uint32_t location;
  } word_offset;
} SpvReflectInterfaceVariable;
typedef struct SpvReflectBlockVariable {
  uint32_t spirv_id;
  const char* name;
  uint32_t offset;
  uint32_t absolute_offset;
  uint32_t size;
  uint32_t padded_size;
  SpvReflectDecorationFlags decoration_flags;
  SpvReflectNumericTraits numeric;
  SpvReflectArrayTraits array;
  SpvReflectVariableFlags flags;
  uint32_t member_count;
  struct SpvReflectBlockVariable* members;
  SpvReflectTypeDescription* type_description;
  struct {
    uint32_t offset;
  } word_offset;
} SpvReflectBlockVariable;
typedef struct SpvReflectDescriptorBinding {
  uint32_t spirv_id;
  const char* name;
  uint32_t binding;
  uint32_t input_attachment_index;
  uint32_t set;
  SpvReflectDescriptorType descriptor_type;
  SpvReflectResourceType resource_type;
  SpvReflectImageTraits image;
  SpvReflectBlockVariable block;
  SpvReflectBindingArrayTraits array;
  uint32_t count;
  uint32_t accessed;
  uint32_t uav_counter_id;
  struct SpvReflectDescriptorBinding* uav_counter_binding;
  uint32_t byte_address_buffer_offset_count;
  uint32_t* byte_address_buffer_offsets;
  SpvReflectTypeDescription* type_description;
  struct {
    uint32_t binding;
    uint32_t set;
  } word_offset;
  SpvReflectDecorationFlags decoration_flags;
  SpvReflectUserType user_type;
} SpvReflectDescriptorBinding;
typedef struct SpvReflectDescriptorSet {
  uint32_t set;
  uint32_t binding_count;
  SpvReflectDescriptorBinding** bindings;
} SpvReflectDescriptorSet;
typedef enum SpvReflectExecutionModeValue {
  SPV_REFLECT_EXECUTION_MODE_SPEC_CONSTANT = 0xFFFFFFFF
} SpvReflectExecutionModeValue;
typedef struct SpvReflectEntryPoint {
  const char* name;
  uint32_t id;
  SpvExecutionModel spirv_execution_model;
  SpvReflectShaderStageFlagBits shader_stage;
  uint32_t input_variable_count;
  SpvReflectInterfaceVariable** input_variables;
  uint32_t output_variable_count;
  SpvReflectInterfaceVariable** output_variables;
  uint32_t interface_variable_count;
  SpvReflectInterfaceVariable* interface_variables;
  uint32_t descriptor_set_count;
  SpvReflectDescriptorSet* descriptor_sets;
  uint32_t used_uniform_count;
  uint32_t* used_uniforms;
  uint32_t used_push_constant_count;
  uint32_t* used_push_constants;
  uint32_t execution_mode_count;
  SpvExecutionMode* execution_modes;
  struct LocalSize {
    uint32_t x;
    uint32_t y;
    uint32_t z;
  } local_size;
  uint32_t invocations;
  uint32_t output_vertices;
} SpvReflectEntryPoint;
typedef struct SpvReflectCapability {
  SpvCapability value;
  uint32_t word_offset;
} SpvReflectCapability;
typedef struct SpvReflectSpecializationConstant {
  uint32_t spirv_id;
  uint32_t constant_id;
  const char* name;
} SpvReflectSpecializationConstant;
typedef struct SpvReflectShaderModule {
  SpvReflectGenerator generator;
  const char* entry_point_name;
  uint32_t entry_point_id;
  uint32_t entry_point_count;
  SpvReflectEntryPoint* entry_points;
  SpvSourceLanguage source_language;
  uint32_t source_language_version;
  const char* source_file;
  const char* source_source;
  uint32_t capability_count;
  SpvReflectCapability* capabilities;
  SpvExecutionModel spirv_execution_model;
  SpvReflectShaderStageFlagBits shader_stage;
  uint32_t descriptor_binding_count;
  SpvReflectDescriptorBinding* descriptor_bindings;
  uint32_t descriptor_set_count;
  SpvReflectDescriptorSet descriptor_sets[SPV_REFLECT_MAX_DESCRIPTOR_SETS];
  uint32_t input_variable_count;
  SpvReflectInterfaceVariable** input_variables;
  uint32_t output_variable_count;
  SpvReflectInterfaceVariable** output_variables;
  uint32_t interface_variable_count;
  SpvReflectInterfaceVariable* interface_variables;
  uint32_t push_constant_block_count;
  SpvReflectBlockVariable* push_constant_blocks;
  uint32_t spec_constant_count;
  SpvReflectSpecializationConstant* spec_constants;
  struct Internal {
    SpvReflectModuleFlags module_flags;
    size_t spirv_size;
    uint32_t* spirv_code;
    uint32_t spirv_word_count;
    size_t type_description_count;
    SpvReflectTypeDescription* type_descriptions;
  } * _internal;
} SpvReflectShaderModule;
#if defined(__cplusplus)
extern "C" {
#endif
SpvReflectResult spvReflectCreateShaderModule(
  size_t size,
  const void* p_code,
  SpvReflectShaderModule* p_module
);
SpvReflectResult spvReflectCreateShaderModule2(
  SpvReflectModuleFlags flags,
  size_t size,
  const void* p_code,
  SpvReflectShaderModule* p_module
);
SPV_REFLECT_DEPRECATED("renamed to spvReflectCreateShaderModule")
SpvReflectResult spvReflectGetShaderModule(
  size_t size,
  const void* p_code,
  SpvReflectShaderModule* p_module
);
void spvReflectDestroyShaderModule(SpvReflectShaderModule* p_module);
uint32_t spvReflectGetCodeSize(const SpvReflectShaderModule* p_module);
const uint32_t* spvReflectGetCode(const SpvReflectShaderModule* p_module);
const SpvReflectEntryPoint* spvReflectGetEntryPoint(
  const SpvReflectShaderModule* p_module,
  const char* entry_point
);
SpvReflectResult spvReflectEnumerateDescriptorBindings(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectDescriptorBinding** pp_bindings
);
SpvReflectResult spvReflectEnumerateEntryPointDescriptorBindings(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectDescriptorBinding** pp_bindings
);
SpvReflectResult spvReflectEnumerateDescriptorSets(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectDescriptorSet** pp_sets
);
SpvReflectResult spvReflectEnumerateEntryPointDescriptorSets(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectDescriptorSet** pp_sets
);
SpvReflectResult spvReflectEnumerateInterfaceVariables(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
);
SpvReflectResult spvReflectEnumerateEntryPointInterfaceVariables(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
);
SpvReflectResult spvReflectEnumerateInputVariables(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
);
SpvReflectResult spvReflectEnumerateEntryPointInputVariables(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
);
SpvReflectResult spvReflectEnumerateOutputVariables(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
);
SpvReflectResult spvReflectEnumerateEntryPointOutputVariables(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
);
SpvReflectResult spvReflectEnumeratePushConstantBlocks(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectBlockVariable** pp_blocks
);
SPV_REFLECT_DEPRECATED("renamed to spvReflectEnumeratePushConstantBlocks")
SpvReflectResult spvReflectEnumeratePushConstants(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectBlockVariable** pp_blocks
);
SpvReflectResult spvReflectEnumerateEntryPointPushConstantBlocks(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectBlockVariable** pp_blocks
);
SpvReflectResult spvReflectEnumerateSpecializationConstants(
  const SpvReflectShaderModule* p_module,
  uint32_t* p_count,
  SpvReflectSpecializationConstant** pp_constants
);
const SpvReflectDescriptorBinding* spvReflectGetDescriptorBinding(
  const SpvReflectShaderModule* p_module,
  uint32_t binding_number,
  uint32_t set_number,
  SpvReflectResult* p_result
);
const SpvReflectDescriptorBinding* spvReflectGetEntryPointDescriptorBinding(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t binding_number,
  uint32_t set_number,
  SpvReflectResult* p_result
);
const SpvReflectDescriptorSet* spvReflectGetDescriptorSet(
  const SpvReflectShaderModule* p_module,
  uint32_t set_number,
  SpvReflectResult* p_result
);
const SpvReflectDescriptorSet* spvReflectGetEntryPointDescriptorSet(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t set_number,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetInputVariableByLocation(
  const SpvReflectShaderModule* p_module,
  uint32_t location,
  SpvReflectResult* p_result
);
SPV_REFLECT_DEPRECATED("renamed to spvReflectGetInputVariableByLocation")
const SpvReflectInterfaceVariable* spvReflectGetInputVariable(
  const SpvReflectShaderModule* p_module,
  uint32_t location,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetEntryPointInputVariableByLocation(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t location,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetInputVariableBySemantic(
  const SpvReflectShaderModule* p_module,
  const char* semantic,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetEntryPointInputVariableBySemantic(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  const char* semantic,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetOutputVariableByLocation(
  const SpvReflectShaderModule* p_module,
  uint32_t location,
  SpvReflectResult* p_result
);
SPV_REFLECT_DEPRECATED("renamed to spvReflectGetOutputVariableByLocation")
const SpvReflectInterfaceVariable* spvReflectGetOutputVariable(
  const SpvReflectShaderModule* p_module,
  uint32_t location,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetEntryPointOutputVariableByLocation(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  uint32_t location,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetOutputVariableBySemantic(
  const SpvReflectShaderModule* p_module,
  const char* semantic,
  SpvReflectResult* p_result
);
const SpvReflectInterfaceVariable* spvReflectGetEntryPointOutputVariableBySemantic(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  const char* semantic,
  SpvReflectResult* p_result
);
const SpvReflectBlockVariable* spvReflectGetPushConstantBlock(
  const SpvReflectShaderModule* p_module,
  uint32_t index,
  SpvReflectResult* p_result
);
SPV_REFLECT_DEPRECATED("renamed to spvReflectGetPushConstantBlock")
const SpvReflectBlockVariable* spvReflectGetPushConstant(
  const SpvReflectShaderModule* p_module,
  uint32_t index,
  SpvReflectResult* p_result
);
const SpvReflectBlockVariable* spvReflectGetEntryPointPushConstantBlock(
  const SpvReflectShaderModule* p_module,
  const char* entry_point,
  SpvReflectResult* p_result
);
SpvReflectResult spvReflectChangeDescriptorBindingNumbers(
  SpvReflectShaderModule* p_module,
  const SpvReflectDescriptorBinding* p_binding,
  uint32_t new_binding_number,
  uint32_t new_set_number
);
SPV_REFLECT_DEPRECATED("Renamed to spvReflectChangeDescriptorBindingNumbers")
SpvReflectResult spvReflectChangeDescriptorBindingNumber(
  SpvReflectShaderModule* p_module,
  const SpvReflectDescriptorBinding* p_descriptor_binding,
  uint32_t new_binding_number,
  uint32_t optional_new_set_number
);
SpvReflectResult spvReflectChangeDescriptorSetNumber(
  SpvReflectShaderModule* p_module,
  const SpvReflectDescriptorSet* p_set,
  uint32_t new_set_number
);
SpvReflectResult spvReflectChangeInputVariableLocation(
  SpvReflectShaderModule* p_module,
  const SpvReflectInterfaceVariable* p_input_variable,
  uint32_t new_location
);
SpvReflectResult spvReflectChangeOutputVariableLocation(
  SpvReflectShaderModule* p_module,
  const SpvReflectInterfaceVariable* p_output_variable,
  uint32_t new_location
);
const char* spvReflectSourceLanguage(SpvSourceLanguage source_lang);
const char* spvReflectBlockVariableTypeName(
  const SpvReflectBlockVariable* p_var
);
#if defined(__cplusplus)
};
#endif
#if defined(__cplusplus) && !defined(SPIRV_REFLECT_DISABLE_CPP_BINDINGS)
#include <cstdlib>
#include <string>
#include <vector>
namespace spv_reflect {
class ShaderModule {
public:
  ShaderModule();
  ShaderModule(size_t size, const void* p_code, SpvReflectModuleFlags flags = SPV_REFLECT_MODULE_FLAG_NONE);
  ShaderModule(const std::vector<uint8_t>& code, SpvReflectModuleFlags flags = SPV_REFLECT_MODULE_FLAG_NONE);
  ShaderModule(const std::vector<uint32_t>& code, SpvReflectModuleFlags flags = SPV_REFLECT_MODULE_FLAG_NONE);
  ~ShaderModule();
  ShaderModule(ShaderModule&& other);
  ShaderModule& operator=(ShaderModule&& other);
  SpvReflectResult GetResult() const;
  const SpvReflectShaderModule& GetShaderModule() const;
  uint32_t GetCodeSize() const;
  const uint32_t* GetCode() const;
  const char* GetEntryPointName() const;
  const char* GetSourceFile() const;
  uint32_t GetEntryPointCount() const;
  const char* GetEntryPointName(uint32_t index) const;
  SpvReflectShaderStageFlagBits GetEntryPointShaderStage(uint32_t index) const;
  SpvReflectShaderStageFlagBits GetShaderStage() const;
  SPV_REFLECT_DEPRECATED("Renamed to GetShaderStage")
  SpvReflectShaderStageFlagBits GetVulkanShaderStage() const {
    return GetShaderStage();
  }
  SpvReflectResult EnumerateDescriptorBindings(uint32_t* p_count, SpvReflectDescriptorBinding** pp_bindings) const;
  SpvReflectResult EnumerateEntryPointDescriptorBindings(const char* entry_point, uint32_t* p_count, SpvReflectDescriptorBinding** pp_bindings) const;
  SpvReflectResult EnumerateDescriptorSets( uint32_t* p_count, SpvReflectDescriptorSet** pp_sets) const ;
  SpvReflectResult EnumerateEntryPointDescriptorSets(const char* entry_point, uint32_t* p_count, SpvReflectDescriptorSet** pp_sets) const ;
  SpvReflectResult EnumerateInterfaceVariables(uint32_t* p_count, SpvReflectInterfaceVariable** pp_variables) const;
  SpvReflectResult EnumerateEntryPointInterfaceVariables(const char* entry_point, uint32_t* p_count, SpvReflectInterfaceVariable** pp_variables) const;
  SpvReflectResult EnumerateInputVariables(uint32_t* p_count,SpvReflectInterfaceVariable** pp_variables) const;
  SpvReflectResult EnumerateEntryPointInputVariables(const char* entry_point, uint32_t* p_count, SpvReflectInterfaceVariable** pp_variables) const;
  SpvReflectResult EnumerateOutputVariables(uint32_t* p_count,SpvReflectInterfaceVariable** pp_variables) const;
  SpvReflectResult EnumerateEntryPointOutputVariables(const char* entry_point, uint32_t* p_count, SpvReflectInterfaceVariable** pp_variables) const;
  SpvReflectResult EnumeratePushConstantBlocks(uint32_t* p_count, SpvReflectBlockVariable** pp_blocks) const;
  SpvReflectResult EnumerateEntryPointPushConstantBlocks(const char* entry_point, uint32_t* p_count, SpvReflectBlockVariable** pp_blocks) const;
  SPV_REFLECT_DEPRECATED("Renamed to EnumeratePushConstantBlocks")
  SpvReflectResult EnumeratePushConstants(uint32_t* p_count, SpvReflectBlockVariable** pp_blocks) const {
    return EnumeratePushConstantBlocks(p_count, pp_blocks);
  }
  SpvReflectResult EnumerateSpecializationConstants(uint32_t* p_count, SpvReflectSpecializationConstant** pp_constants) const;
  const SpvReflectDescriptorBinding* GetDescriptorBinding(uint32_t binding_number, uint32_t set_number, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectDescriptorBinding* GetEntryPointDescriptorBinding(const char* entry_point, uint32_t binding_number, uint32_t set_number, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectDescriptorSet* GetDescriptorSet(uint32_t set_number, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectDescriptorSet* GetEntryPointDescriptorSet(const char* entry_point, uint32_t set_number, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectInterfaceVariable* GetInputVariableByLocation(uint32_t location, SpvReflectResult* p_result = nullptr) const;
  SPV_REFLECT_DEPRECATED("Renamed to GetInputVariableByLocation")
  const SpvReflectInterfaceVariable* GetInputVariable(uint32_t location, SpvReflectResult* p_result = nullptr) const {
    return GetInputVariableByLocation(location, p_result);
  }
  const SpvReflectInterfaceVariable* GetEntryPointInputVariableByLocation(const char* entry_point, uint32_t location, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectInterfaceVariable* GetInputVariableBySemantic(const char* semantic, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectInterfaceVariable* GetEntryPointInputVariableBySemantic(const char* entry_point, const char* semantic, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectInterfaceVariable* GetOutputVariableByLocation(uint32_t location, SpvReflectResult* p_result = nullptr) const;
  SPV_REFLECT_DEPRECATED("Renamed to GetOutputVariableByLocation")
  const SpvReflectInterfaceVariable* GetOutputVariable(uint32_t location, SpvReflectResult* p_result = nullptr) const {
    return GetOutputVariableByLocation(location, p_result);
  }
  const SpvReflectInterfaceVariable* GetEntryPointOutputVariableByLocation(const char* entry_point, uint32_t location, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectInterfaceVariable* GetOutputVariableBySemantic(const char* semantic, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectInterfaceVariable* GetEntryPointOutputVariableBySemantic(const char* entry_point, const char* semantic, SpvReflectResult* p_result = nullptr) const;
  const SpvReflectBlockVariable* GetPushConstantBlock(uint32_t index, SpvReflectResult* p_result = nullptr) const;
  SPV_REFLECT_DEPRECATED("Renamed to GetPushConstantBlock")
  const SpvReflectBlockVariable* GetPushConstant(uint32_t index, SpvReflectResult* p_result = nullptr) const {
    return GetPushConstantBlock(index, p_result);
  }
  const SpvReflectBlockVariable* GetEntryPointPushConstantBlock(const char* entry_point, SpvReflectResult* p_result = nullptr) const;
  SpvReflectResult ChangeDescriptorBindingNumbers(const SpvReflectDescriptorBinding* p_binding,
      uint32_t new_binding_number = SPV_REFLECT_BINDING_NUMBER_DONT_CHANGE,
      uint32_t optional_new_set_number = SPV_REFLECT_SET_NUMBER_DONT_CHANGE);
  SPV_REFLECT_DEPRECATED("Renamed to ChangeDescriptorBindingNumbers")
  SpvReflectResult ChangeDescriptorBindingNumber(const SpvReflectDescriptorBinding* p_binding, uint32_t new_binding_number = SPV_REFLECT_BINDING_NUMBER_DONT_CHANGE,
      uint32_t new_set_number = SPV_REFLECT_SET_NUMBER_DONT_CHANGE) {
    return ChangeDescriptorBindingNumbers(p_binding, new_binding_number, new_set_number);
  }
  SpvReflectResult ChangeDescriptorSetNumber(const SpvReflectDescriptorSet* p_set, uint32_t new_set_number = SPV_REFLECT_SET_NUMBER_DONT_CHANGE);
  SpvReflectResult ChangeInputVariableLocation(const SpvReflectInterfaceVariable* p_input_variable, uint32_t new_location);
  SpvReflectResult ChangeOutputVariableLocation(const SpvReflectInterfaceVariable* p_output_variable, uint32_t new_location);
private:
  ShaderModule(const ShaderModule&);
  ShaderModule& operator=(const ShaderModule&);
private:
  mutable SpvReflectResult m_result = SPV_REFLECT_RESULT_NOT_READY;
  SpvReflectShaderModule m_module = {};
};
inline ShaderModule::ShaderModule() {}
inline ShaderModule::ShaderModule(size_t size, const void* p_code, SpvReflectModuleFlags flags) {
  m_result = spvReflectCreateShaderModule2(
    flags,
    size,
    p_code,
    &m_module);
}
inline ShaderModule::ShaderModule(const std::vector<uint8_t>& code, SpvReflectModuleFlags flags) {
  m_result = spvReflectCreateShaderModule2(
    flags,
    code.size(),
    code.data(),
    &m_module);
}
inline ShaderModule::ShaderModule(const std::vector<uint32_t>& code, SpvReflectModuleFlags flags) {
  m_result = spvReflectCreateShaderModule2(
    flags,
    code.size() * sizeof(uint32_t),
    code.data(),
    &m_module);
}
inline ShaderModule::~ShaderModule() {
  spvReflectDestroyShaderModule(&m_module);
}
inline ShaderModule::ShaderModule(ShaderModule&& other)
{
    *this = std::move(other);
}
inline ShaderModule& ShaderModule::operator=(ShaderModule&& other)
{
    m_result = std::move(other.m_result);
    m_module = std::move(other.m_module);
    other.m_module = {};
    return *this;
}
inline SpvReflectResult ShaderModule::GetResult() const {
  return m_result;
}
inline const SpvReflectShaderModule& ShaderModule::GetShaderModule() const {
  return m_module;
}
inline uint32_t ShaderModule::GetCodeSize() const {
  return spvReflectGetCodeSize(&m_module);
}
inline const uint32_t* ShaderModule::GetCode() const {
  return spvReflectGetCode(&m_module);
}
inline const char* ShaderModule::GetEntryPointName() const {
  return this->GetEntryPointName(0);
}
inline const char* ShaderModule::GetSourceFile() const {
  return m_module.source_file;
}
inline uint32_t ShaderModule::GetEntryPointCount() const {
  return m_module.entry_point_count;
}
inline const char* ShaderModule::GetEntryPointName(uint32_t index) const {
  return m_module.entry_points[index].name;
}
inline SpvReflectShaderStageFlagBits ShaderModule::GetEntryPointShaderStage(uint32_t index) const {
  return m_module.entry_points[index].shader_stage;
}
inline SpvReflectShaderStageFlagBits ShaderModule::GetShaderStage() const {
  return m_module.shader_stage;
}
inline SpvReflectResult ShaderModule::EnumerateDescriptorBindings(
  uint32_t* p_count,
  SpvReflectDescriptorBinding** pp_bindings
) const
{
  m_result = spvReflectEnumerateDescriptorBindings(
    &m_module,
    p_count,
    pp_bindings);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateEntryPointDescriptorBindings(
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectDescriptorBinding** pp_bindings
) const
{
  m_result = spvReflectEnumerateEntryPointDescriptorBindings(
      &m_module,
      entry_point,
      p_count,
      pp_bindings);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateDescriptorSets(
  uint32_t* p_count,
  SpvReflectDescriptorSet** pp_sets
) const
{
  m_result = spvReflectEnumerateDescriptorSets(
    &m_module,
    p_count,
    pp_sets);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateEntryPointDescriptorSets(
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectDescriptorSet** pp_sets
) const
{
  m_result = spvReflectEnumerateEntryPointDescriptorSets(
      &m_module,
      entry_point,
      p_count,
      pp_sets);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateInterfaceVariables(
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
) const
{
  m_result = spvReflectEnumerateInterfaceVariables(
    &m_module,
    p_count,
    pp_variables);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateEntryPointInterfaceVariables(
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
) const
{
  m_result = spvReflectEnumerateEntryPointInterfaceVariables(
      &m_module,
      entry_point,
      p_count,
      pp_variables);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateInputVariables(
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
) const
{
  m_result = spvReflectEnumerateInputVariables(
    &m_module,
    p_count,
    pp_variables);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateEntryPointInputVariables(
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
) const
{
  m_result = spvReflectEnumerateEntryPointInputVariables(
      &m_module,
      entry_point,
      p_count,
      pp_variables);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateOutputVariables(
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
) const
{
  m_result = spvReflectEnumerateOutputVariables(
    &m_module,
    p_count,
    pp_variables);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateEntryPointOutputVariables(
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectInterfaceVariable** pp_variables
) const
{
  m_result = spvReflectEnumerateEntryPointOutputVariables(
      &m_module,
      entry_point,
      p_count,
      pp_variables);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumeratePushConstantBlocks(
  uint32_t* p_count,
  SpvReflectBlockVariable** pp_blocks
) const
{
  m_result = spvReflectEnumeratePushConstantBlocks(
    &m_module,
    p_count,
    pp_blocks);
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateSpecializationConstants(
    uint32_t* p_count,
    SpvReflectSpecializationConstant** pp_constants
) const
{
  m_result = spvReflectEnumerateSpecializationConstants(
    &m_module,
    p_count,
    pp_constants
  );
  return m_result;
}
inline SpvReflectResult ShaderModule::EnumerateEntryPointPushConstantBlocks(
  const char* entry_point,
  uint32_t* p_count,
  SpvReflectBlockVariable** pp_blocks
) const
{
  m_result = spvReflectEnumerateEntryPointPushConstantBlocks(
      &m_module,
      entry_point,
      p_count,
      pp_blocks);
  return m_result;
}
inline const SpvReflectDescriptorBinding* ShaderModule::GetDescriptorBinding(
  uint32_t binding_number,
  uint32_t set_number,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetDescriptorBinding(
    &m_module,
    binding_number,
    set_number,
    p_result);
}
inline const SpvReflectDescriptorBinding* ShaderModule::GetEntryPointDescriptorBinding(
  const char* entry_point,
  uint32_t binding_number,
  uint32_t set_number,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetEntryPointDescriptorBinding(
    &m_module,
    entry_point,
    binding_number,
    set_number,
    p_result);
}
inline const SpvReflectDescriptorSet* ShaderModule::GetDescriptorSet(
  uint32_t set_number,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetDescriptorSet(
    &m_module,
    set_number,
    p_result);
}
inline const SpvReflectDescriptorSet* ShaderModule::GetEntryPointDescriptorSet(
  const char* entry_point,
  uint32_t set_number,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetEntryPointDescriptorSet(
    &m_module,
    entry_point,
    set_number,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetInputVariableByLocation(
  uint32_t location,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetInputVariableByLocation(
    &m_module,
    location,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetInputVariableBySemantic(
  const char* semantic,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetInputVariableBySemantic(
    &m_module,
    semantic,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetEntryPointInputVariableByLocation(
  const char* entry_point,
  uint32_t location,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetEntryPointInputVariableByLocation(
    &m_module,
    entry_point,
    location,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetEntryPointInputVariableBySemantic(
  const char* entry_point,
  const char* semantic,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetEntryPointInputVariableBySemantic(
    &m_module,
    entry_point,
    semantic,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetOutputVariableByLocation(
  uint32_t location,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetOutputVariableByLocation(
    &m_module,
    location,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetOutputVariableBySemantic(
  const char* semantic,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetOutputVariableBySemantic(&m_module,
    semantic,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetEntryPointOutputVariableByLocation(
  const char* entry_point,
  uint32_t location,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetEntryPointOutputVariableByLocation(
    &m_module,
    entry_point,
    location,
    p_result);
}
inline const SpvReflectInterfaceVariable* ShaderModule::GetEntryPointOutputVariableBySemantic(
  const char* entry_point,
  const char* semantic,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetEntryPointOutputVariableBySemantic(
    &m_module,
    entry_point,
    semantic,
    p_result);
}
inline const SpvReflectBlockVariable* ShaderModule::GetPushConstantBlock(
  uint32_t index,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetPushConstantBlock(
    &m_module,
    index,
    p_result);
}
inline const SpvReflectBlockVariable* ShaderModule::GetEntryPointPushConstantBlock(
  const char* entry_point,
  SpvReflectResult* p_result
) const
{
  return spvReflectGetEntryPointPushConstantBlock(
    &m_module,
    entry_point,
    p_result);
}
inline SpvReflectResult ShaderModule::ChangeDescriptorBindingNumbers(
  const SpvReflectDescriptorBinding* p_binding,
  uint32_t new_binding_number,
  uint32_t new_set_number
)
{
  return spvReflectChangeDescriptorBindingNumbers(
    &m_module,
    p_binding,
    new_binding_number,
    new_set_number);
}
inline SpvReflectResult ShaderModule::ChangeDescriptorSetNumber(
  const SpvReflectDescriptorSet* p_set,
  uint32_t new_set_number
)
{
  return spvReflectChangeDescriptorSetNumber(
    &m_module,
    p_set,
    new_set_number);
}
inline SpvReflectResult ShaderModule::ChangeInputVariableLocation(
  const SpvReflectInterfaceVariable* p_input_variable,
  uint32_t new_location)
{
  return spvReflectChangeInputVariableLocation(
    &m_module,
    p_input_variable,
    new_location);
}
inline SpvReflectResult ShaderModule::ChangeOutputVariableLocation(
  const SpvReflectInterfaceVariable* p_output_variable,
  uint32_t new_location)
{
  return spvReflectChangeOutputVariableLocation(
    &m_module,
    p_output_variable,
    new_location);
}
}
#endif
#endif
