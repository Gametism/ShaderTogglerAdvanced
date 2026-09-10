// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include <array>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
namespace ShaderToggler::smart
{
    enum class Method : uint32_t { Skip, TransparentBlack, OpaqueBlack, White, MidGray, Custom };
    struct Choice
    {
        Method method = Method::Skip;
        std::array<float, 4> rgba{};
        bool operator==(const Choice&) const = default;
    };
    inline Choice preset(Method method)
    {
        switch (method)
        {
        case Method::OpaqueBlack: return {method, {0, 0, 0, 1}};
        case Method::White: return {method, {1, 1, 1, 1}};
        case Method::MidGray: return {method, {.5f, .5f, .5f, 1}};
        default: return {method, {0, 0, 0, 0}};
        }
    }
    inline const char* name(Method method)
    {
        switch (method)
        {
        case Method::Skip: return "Skip the pass";
        case Method::TransparentBlack: return "Transparent black";
        case Method::OpaqueBlack: return "Opaque black";
        case Method::White: return "White";
        case Method::MidGray: return "Mid-gray";
        case Method::Custom: return "Custom colour";
        default: return "Unknown";
        }
    }
    inline bool valid(const Choice& choice)
    {
        if (choice.method > Method::Custom) return false;
        for (float value : choice.rgba)
            if (!std::isfinite(value) || value < 0 || value > 1) return false;
        return true;
    }
    inline std::string encode(const Choice& choice)
    {
        std::string text = std::to_string(static_cast<uint32_t>(choice.method));
        for (float value : choice.rgba)
        {
            uint32_t bits; std::memcpy(&bits, &value, sizeof(bits));
            text += "," + std::to_string(bits);
        }
        return text;
    }
    inline bool unsignedValue(std::string_view text, uint32_t& value)
    {
        const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
        return !text.empty() && result.ec == std::errc{} && result.ptr == text.data() + text.size();
    }
    inline std::optional<Choice> decode(std::string_view text)
    {
        std::array<uint32_t, 5> words{};
        for (size_t i = 0; i < words.size(); ++i)
        {
            const size_t comma = text.find(',');
            if ((i < 4 && comma == text.npos) || (i == 4 && comma != text.npos)) return {};
            if (!unsignedValue(text.substr(0, comma), words[i])) return {};
            if (comma != text.npos) text.remove_prefix(comma + 1);
        }
        Choice choice; choice.method = static_cast<Method>(words[0]);
        for (size_t i = 0; i < 4; ++i) std::memcpy(&choice.rgba[i], &words[i + 1], sizeof(float));
        if (!valid(choice)) return {};
        return choice.method == Method::Custom ? choice : preset(choice.method);
    }
    inline bool knownBloom(bool syndicate, uint32_t hash)
    {
        return syndicate && hash == 0x4E71379Du;
    }
    inline std::array<Method, 5> candidates(Method first)
    {
        std::array<Method, 5> methods = {Method::TransparentBlack, Method::OpaqueBlack,
            Method::White, Method::MidGray, Method::Skip};
        for (size_t i = 0; i < methods.size(); ++i)
            if (methods[i] == first) { std::swap(methods[0], methods[i]); break; }
        return methods;
    }
    struct Preview
    {
        uint32_t hash = 0;
        Choice choice;
        bool original = false;
    };
    class Settings
    {
        mutable std::mutex _mutex;
        std::map<uint32_t, Choice> _saved;
        Preview _preview;
    public:
        std::map<uint32_t, Choice> saved() const { std::lock_guard lock(_mutex); return _saved; }
        void replace(std::map<uint32_t, Choice> saved) { std::lock_guard lock(_mutex); _saved = std::move(saved); _preview = {}; }
        void set(uint32_t hash, Choice choice) { std::lock_guard lock(_mutex); if (hash && valid(choice)) _saved[hash] = choice; }
        void erase(uint32_t hash) { std::lock_guard lock(_mutex); _saved.erase(hash); }
        Preview preview() const { std::lock_guard lock(_mutex); return _preview; }
        void preview(Preview value) { std::lock_guard lock(_mutex); _preview = value; }
        void cancel() { preview({}); }
        std::optional<Choice> choice(uint32_t hash, uint32_t huntedHash, bool syndicate) const
        {
            std::lock_guard lock(_mutex);
            if (_preview.hash == hash && hash == huntedHash && !_preview.original) return _preview.choice;
            if (auto found = _saved.find(hash); found != _saved.end()) return found->second;
            if (knownBloom(syndicate, hash)) return preset(Method::TransparentBlack);
            return {};
        }
    };
}
