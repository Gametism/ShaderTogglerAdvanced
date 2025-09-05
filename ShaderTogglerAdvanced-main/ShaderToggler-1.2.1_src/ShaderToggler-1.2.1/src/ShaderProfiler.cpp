#include "ShaderProfiler.h"

void ShaderProfiler::on_draw_call(reshade::api::pipeline pipeline) {
    stats_[pipeline].draw_calls++;
    // GPU timing could be added here in future
}

void ShaderProfiler::new_frame() {
    stats_.clear();
}

const std::unordered_map<reshade::api::pipeline, ShaderStats>& ShaderProfiler::get_stats() const {
    return stats_;
}