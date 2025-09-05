#pragma once
#include <reshade.hpp>
#include <unordered_map>

struct ShaderStats {
    uint32_t draw_calls = 0;
    float last_execution_time_ms = 0.0f;
};

class ShaderProfiler {
public:
    void on_draw_call(reshade::api::pipeline pipeline);
    void new_frame();
    const std::unordered_map<reshade::api::pipeline, ShaderStats>& get_stats() const;

private:
    std::unordered_map<reshade::api::pipeline, ShaderStats> stats_;
};