// Copyright (c) 2026 Sven "Gametism" Koenigsmann. All Rights Reserved.
// Proprietary modifications; see PROPRIETARY_LICENSE.txt in the source package.
#pragma once
#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>
namespace ShaderToggler::finder
{
    enum class Kind : uint32_t { Draw=1, Indexed, Dispatch, IndirectDraw, IndirectIndexed, IndirectDispatch };
    using Signature = std::array<uint32_t,12>;
    using History = std::array<uint32_t,3>;
    constexpr size_t maxRules = 64, maxSignatures = 32768, maxCandidates = 256;
    inline bool compute(const Signature& s) { return s[1]==3 || s[1]==6; }
    inline bool valid(const Signature& s)
    {
        if(s[0]<1 || s[0]>6 || s[1]<1 || s[1]>6 || !s[5] || !s[6]) return false;
        if(s[10]>3 || ((s[10]==0)!=(s[11]==0)) || (compute(s) && s[10])) return false;
        if(compute(s)) { if(s[2] || s[3] || !s[4]) return false; }
        else if(!s[2] || !s[3] || s[4]) return false;
        if(s[1]>=4) return s[7]==1 || (s[7]==2 && s[0]==4 && s[1]<=5);
        return s[1]==3 ? s[7]!=0 : s[7]==0;
    }
    inline std::string encode(const Signature& s)
    {
        const bool extended=s[10] || (s[1]>=4 && s[7]==2);
        std::string text=extended?"2":"1";
        for(size_t i=0;i<(extended?s.size():10);++i) text+=","+std::to_string(s[i]);
        return text;
    }
    inline std::optional<Signature> decode(std::string_view text)
    {
        if(text.size()>160 || (!text.starts_with("1,") && !text.starts_with("2,"))) return {};
        const size_t fields=text[0]=='1'?10:12;
        text.remove_prefix(2); Signature result{};
        for(size_t i=0;i<fields;++i)
        {
            const auto end=text.find(',');
            const auto token=text.substr(0,end);
            auto parsed=std::from_chars(token.data(),token.data()+token.size(),result[i]);
            if(token.empty() || parsed.ec!=std::errc{} || parsed.ptr!=token.data()+token.size()) return {};
            if(i+1==fields) { if(end!=text.npos) return {}; }
            else { if(end==text.npos) return {}; text.remove_prefix(end+1); }
        }
        const bool extended=result[10] || (result[1]>=4 && result[7]==2);
        return valid(result) && (extended==(fields==12)) ? std::optional<Signature>(result) : std::nullopt;
    }
    inline const Signature* matchingRule(const std::set<Signature>& rules,const Signature& key,const History& history={})
    {
        if(auto it=rules.find(key);it!=rules.end()) return &*it;
        if(compute(key) || rules.empty()) return nullptr;
        auto scoped=key;
        for(size_t i=0;i<history.size();++i)
        {
            if(!history[i]) continue;
            scoped[10]=static_cast<uint32_t>(i+1);scoped[11]=history[i];
            if(auto it=rules.find(scoped);it!=rules.end()) return &*it;
        }
        return nullptr;
    }
    template<class Ini> inline std::vector<Signature> loadRules(Ini& ini,const std::string& root)
    {
        std::vector<Signature> rules;
        const auto section=root+"_EffectFilters";
        const auto count=std::clamp(ini.GetInt("Count",section),0,static_cast<int>(maxRules));
        for(int i=0;i<count;++i)
            if(auto s=decode(ini.GetValue("Rule"+std::to_string(i),section));s && std::find(rules.begin(),rules.end(),*s)==rules.end()) rules.push_back(*s);
        return rules;
    }
    template<class Ini> inline void saveRules(Ini& ini,const std::string& root,const std::vector<Signature>& rules)
    {
        const auto section=root+"_EffectFilters";
        const size_t count=std::min(rules.size(),maxRules);
        ini.SetInt("Count",static_cast<int>(count),"",section);
        for(size_t i=0;i<count;++i) ini.SetValue("Rule"+std::to_string(i),encode(rules[i]),"",section);
    }
    struct Stat { uint64_t calls=0, frames=0, lastFrame=UINT64_MAX; };
    struct Capture
    {
        std::map<Signature,Stat> entries;
        uint64_t calls=0, frames=0, omitted=0;
        bool complete=false;
    };
    struct Candidate
    {
        Signature signature{};
        uint64_t baseline=0, action=0;
        double increase=0;
    };
    struct Match
    {
        std::set<uint32_t> pixel,vertex,compute;
        std::set<Signature> rules;
        bool empty() const { return pixel.empty() && vertex.empty() && compute.empty() && rules.empty(); }
        bool matches(const Signature& s,const History& history={}) const
        {
            return matchingRule(rules,s,history) || (finder::compute(s) ? compute.count(s[4]) : pixel.count(s[3]) || vertex.count(s[2]));
        }
        std::optional<Signature> candidate(const Signature& s,const History& history,bool original) const
        {
            if(empty()) return s;
            if(const auto* rule=matchingRule(rules,s,history)) return *rule;
            if(original && !finder::compute(s))
            {
                for(size_t i : {size_t{2},size_t{0},size_t{1}})
                {
                    const auto& hashes=i==0?pixel:i==1?vertex:compute;
                    if(history[i] && hashes.count(history[i]))
                    {
                        auto result=s;result[10]=static_cast<uint32_t>(i+1);result[11]=history[i];
                        return result;
                    }
                }
                return {};
            }
            return matches(s,history)?std::optional<Signature>(s):std::nullopt;
        }
    };
    enum class Stage { Ready, BaselineDelay, Baseline, BaselineDone, ActionDelay, Action, Results, FrozenDelay, FrozenCapture };
    struct View
    {
        int group=-1;
        Stage stage=Stage::Ready;
        uint64_t remaining=0, baselineCalls=0, actionCalls=0, omitted=0, previewHits=0;
        bool baselineComplete=false, preview=false;
        bool guided=false, quick=false;
        bool frozen=false;
        bool followOriginal=false, focused=false;
        uint64_t followedGraphics=0;
        bool includeCompute=false;
        size_t excludedCompute=0;
        size_t totalCandidates=0;
        size_t remainingCandidates=0, testing=0, undoSteps=0;
        size_t selected=0;
        std::vector<Candidate> candidates;
    };
    class Engine
    {
    public:
        using TraceCallback = void (*)(const char*,int,size_t,size_t,bool,bool,const std::set<Signature>&) noexcept;
        explicit Engine(TraceCallback trace=nullptr):traceCallback(trace) {}
    private:
        TraceCallback traceCallback=nullptr;
        mutable std::mutex mutex;
        std::atomic<uintptr_t> owner{0};
        std::atomic<bool> rulesPresent{false};
        std::shared_ptr<const std::set<Signature>> enabled=std::make_shared<const std::set<Signature>>();
        std::shared_ptr<const Match> kept=std::make_shared<const Match>();
        Match focus;
        bool followOriginal=false;
        uint64_t followedGraphics=0;
        int group=-1;
        Stage stage=Stage::Ready;
        uint64_t start=0, end=0, frame=0, previewHits=0;
        Capture baseline, action;
        std::vector<Candidate> candidates;
        size_t selected=0;
        bool preview=false;
        bool guided=false, quick=false;
        bool frozen=false;
        bool includeCompute=false;
        size_t excludedCompute=0;
        size_t totalCandidates=0;
        std::vector<size_t> pool;
        std::vector<std::vector<size_t>> history;
        std::set<Signature> previewRules;
        void applyPreview(const char* reason,bool enabled)
        {
            preview=false;
            if(traceCallback) traceCallback(reason,group,selected,candidates.size(),enabled,includeCompute,previewRules);
            preview=enabled;
        }
        void prepareBatch(const char* reason)
        {
            previewRules.clear();previewHits=0;
            const size_t count=pool.size()>8?8:(pool.size()+1)/2;
            for(size_t i=0;i<count;++i)
            {
                const auto& key=candidates[pool[i]].signature;
                if(compute(key) && !previewRules.empty()) break;
                previewRules.insert(key);
                if(compute(key)) break;
            }
            if(!pool.empty()) selected=pool.front();
            applyPreview(reason,!previewRules.empty());
        }
        void startQuick()
        {
            quick=true;pool.clear();history.clear();
            for(size_t i=0;i<candidates.size();++i) pool.push_back(i);
            prepareBatch("quick-test");
        }
        void rank()
        {
            candidates.clear(); selected=0; preview=false;excludedCompute=0;
            for(const auto& [key,stat]:action.entries)
            {
                auto old=baseline.entries.find(key);
                const uint64_t before=old==baseline.entries.end()?0:old->second.calls;
                const double baseRate=static_cast<double>(before)/std::max<uint64_t>(baseline.frames,1);
                const double actionRate=static_cast<double>(stat.calls)/std::max<uint64_t>(action.frames,1);
                if(stat.frames<2 || (!frozen && before && (actionRate<baseRate*1.5 || actionRate-baseRate<0.25))) continue;
                if(compute(key) && !includeCompute) { ++excludedCompute;continue; }
                candidates.push_back({key,before,stat.calls,actionRate-baseRate});
            }
            std::sort(candidates.begin(),candidates.end(),[](const Candidate& a,const Candidate& b) {
                if(compute(a.signature)!=compute(b.signature)) return !compute(a.signature);
                if((a.baseline==0)!=(b.baseline==0)) return a.baseline==0;
                if(a.increase!=b.increase) return a.increase>b.increase;
                return a.signature<b.signature;
            });
            totalCandidates=candidates.size();
            if(candidates.size()>maxCandidates) candidates.resize(maxCandidates);
        }
        bool advance(uint64_t now)
        {
            if(stage==Stage::FrozenDelay && now>=start) stage=Stage::FrozenCapture;
            if(stage==Stage::FrozenCapture && now>=end)
            {
                action.complete=true;rank();stage=Stage::Results;
                if(!candidates.empty()) previewRules.insert(candidates[0].signature);
                applyPreview("paused-results",!previewRules.empty());
                return true;
            }
            if((stage==Stage::BaselineDelay || stage==Stage::ActionDelay) && now>=start)
                stage=stage==Stage::BaselineDelay?Stage::Baseline:Stage::Action;
            if((stage==Stage::Baseline || stage==Stage::Action) && now>=end)
            {
                if(stage==Stage::Baseline)
                {
                    baseline.complete=true;stage=Stage::BaselineDone;
                    if(guided && baseline.calls)
                    {
                        start=now+4000;end=start+8000;stage=Stage::ActionDelay;
                    }
                }
                else { action.complete=true; rank(); stage=Stage::Results; if(guided) startQuick(); else applyPreview("live-results",false); }
                return true;
            }
            return false;
        }
    public:
        bool needsEvents() const { return owner.load(std::memory_order_acquire)!=0 || rulesPresent.load(std::memory_order_acquire); }
        bool isolates(uintptr_t device) const { return device && owner.load(std::memory_order_acquire)==device; }
        std::shared_ptr<const Match> context() const { return std::atomic_load(&kept); }
        void publish(std::set<Signature> rules)
        {
            auto old=std::atomic_load(&enabled);
            if(*old==rules) return;
            auto next=std::make_shared<const std::set<Signature>>(std::move(rules));
            std::atomic_store(&enabled,next);
            rulesPresent.store(!next->empty(),std::memory_order_release);
        }
        void open(uintptr_t device,int groupId)
        {
            std::lock_guard lock(mutex);
            baseline={}; action={}; candidates.clear(); selected=0; preview=false; previewHits=0; frame=0;
            guided=false;quick=false;pool.clear();history.clear();previewRules.clear();
            frozen=false;totalCandidates=0;focus={};std::atomic_store(&kept,std::make_shared<const Match>());
            includeCompute=false;excludedCompute=0;followOriginal=false;followedGraphics=0;
            group=groupId;stage=Stage::Ready;owner.store(device,std::memory_order_release);
            applyPreview("open",false);
        }
        void close()
        {
            std::lock_guard lock(mutex);applyPreview("stop",false);owner.store(0,std::memory_order_release);
            group=-1;preview=false;baseline={};action={};candidates.clear();stage=Stage::Ready;
            guided=false;quick=false;pool.clear();history.clear();previewRules.clear();
            frozen=false;totalCandidates=0;focus={};std::atomic_store(&kept,std::make_shared<const Match>());
            includeCompute=false;excludedCompute=0;followOriginal=false;followedGraphics=0;
        }
        void configureScene(Match preserved,Match restrictTo={},bool original=false)
        {
            std::lock_guard lock(mutex);
            std::atomic_store(&kept,std::make_shared<const Match>(std::move(preserved)));
            focus=std::move(restrictTo);
            followOriginal=original;followedGraphics=0;
            baseline={};action={};candidates.clear();pool.clear();history.clear();previewRules.clear();
            selected=0;previewHits=0;totalCandidates=0;excludedCompute=0;preview=false;guided=false;quick=false;frozen=true;stage=Stage::Ready;
            applyPreview("scene-reset",false);
        }
        void setComputeCandidates(bool enabled)
        {
            std::lock_guard lock(mutex);
            if(!owner.load() || includeCompute==enabled) return;
            includeCompute=enabled;preview=false;previewHits=0;quick=false;
            pool.clear();history.clear();previewRules.clear();
            if(stage==Stage::Results) rank();
            applyPreview(enabled?"compute-enabled":"compute-disabled",false);
        }
        bool beginFrozen(uint64_t now)
        {
            std::lock_guard lock(mutex);
            if(!owner.load()) return false;
            frozen=true;guided=false;quick=false;preview=false;previewHits=0;selected=0;totalCandidates=0;excludedCompute=0;followedGraphics=0;
            baseline={};action={};candidates.clear();pool.clear();history.clear();previewRules.clear();
            start=now+2000;end=start+2000;stage=Stage::FrozenDelay;applyPreview("paused-capture",false);return true;
        }
        bool begin(bool withEffect,uint64_t now,bool automatic=false)
        {
            std::lock_guard lock(mutex);
            if(!owner.load() || (withEffect && (!baseline.complete || !baseline.calls))) return false;
            preview=false; previewHits=0; candidates.clear();selected=0;action={};
            guided=automatic && !withEffect;quick=false;pool.clear();history.clear();previewRules.clear();
            frozen=false;totalCandidates=0;excludedCompute=0;focus={};followOriginal=false;followedGraphics=0;
            if(!withEffect) baseline={};
            start=now+3000;end=start+8000;
            stage=withEffect?Stage::ActionDelay:Stage::BaselineDelay;applyPreview(withEffect?"action-capture":"idle-capture",false);return true;
        }
        bool tick(uint64_t now)
        {
            std::lock_guard lock(mutex);
            const bool completed=advance(now);
            ++frame;
            if(stage==Stage::Baseline) ++baseline.frames;
            if(stage==Stage::Action || stage==Stage::FrozenCapture) ++action.frames;
            return completed;
        }
        bool observe(uintptr_t device,const Signature& key,uint64_t now,bool canSkip=true,const History& history={})
        {
            if(isolates(device))
            {
                std::lock_guard lock(mutex);
                if(owner.load()!=device) return false;
                const auto preserved=context();
                if(matchingRule(preserved->rules,key,history) && canSkip) return true;
                if(preserved->matches(key,history)) return false;
                if(now>=start && now<end && (stage==Stage::BaselineDelay || stage==Stage::Baseline || stage==Stage::ActionDelay || stage==Stage::Action || stage==Stage::FrozenDelay || stage==Stage::FrozenCapture))
                {
                    auto& capture=(stage==Stage::Baseline || stage==Stage::BaselineDelay)?baseline:action;
                    const auto selected=frozen?focus.candidate(key,history,followOriginal):std::optional<Signature>(key);
                    if(!selected) { }
                    else if(!canSkip || !valid(key)) ++capture.omitted;
                    else
                    {
                        auto found=capture.entries.find(*selected);
                        if(found==capture.entries.end() && capture.entries.size()>=maxSignatures) ++capture.omitted;
                        else
                        {
                            auto& stat=capture.entries[*selected];++stat.calls;++capture.calls;
                            if((*selected)[10]) ++followedGraphics;
                            if(stat.lastFrame!=frame) {stat.lastFrame=frame;++stat.frames;}
                        }
                    }
                }
                if(canSkip && preview && (!compute(key) || includeCompute) && matchingRule(previewRules,key,history)) {++previewHits;return true;}
                return false;
            }
            if(!canSkip || !rulesPresent.load(std::memory_order_acquire)) return false;
            return matchingRule(*std::atomic_load(&enabled),key,history)!=nullptr;
        }
        void select(size_t index,bool enable)
        {
            std::lock_guard lock(mutex);
            selected=std::min(index,candidates.empty()?0:candidates.size()-1);
            const bool activate=enable && !candidates.empty();previewHits=0;
            quick=false;pool.clear();history.clear();previewRules.clear();
            if(activate) previewRules.insert(candidates[selected].signature);
            applyPreview("select",activate);
        }
        void quickTest()
        {
            std::lock_guard lock(mutex);
            if(stage==Stage::Results) startQuick();
        }
        void compare()
        {
            std::lock_guard lock(mutex);
            if(stage!=Stage::Results || candidates.empty() || (quick && pool.empty())) return;
            if(!quick && previewRules.empty()) previewRules.insert(candidates[selected].signature);
            const bool activate=!preview;
            if(activate) previewHits=0;
            applyPreview("compare",activate);
        }
        bool answer(bool gone)
        {
            std::lock_guard lock(mutex);
            if(!quick || !preview || pool.empty() || (gone && !previewHits)) return false;
            if(gone && previewRules.size()==1) return false;
            history.push_back(pool);
            const size_t count=previewRules.size();
            if(gone) pool.resize(count);
            else pool.erase(pool.begin(),pool.begin()+count);
            prepareBatch(gone?"answer-gone":"answer-visible");return true;
        }
        bool back()
        {
            std::lock_guard lock(mutex);
            if(!quick || history.empty()) return false;
            pool=std::move(history.back());history.pop_back();prepareBatch("undo");return true;
        }
        std::optional<Signature> confirmed() const
        {
            std::lock_guard lock(mutex);
            if(!preview || !previewHits || previewRules.size()!=1) return {};
            return *previewRules.begin();
        }
        View view(uint64_t now) const
        {
            std::lock_guard lock(mutex);
            View v;v.group=group;v.stage=stage;v.selected=selected;v.preview=preview;v.previewHits=previewHits;
            v.guided=guided;v.quick=quick;v.remainingCandidates=pool.size();v.testing=previewRules.size();v.undoSteps=history.size();
            v.frozen=frozen;v.totalCandidates=totalCandidates;v.includeCompute=includeCompute;v.excludedCompute=excludedCompute;
            v.followOriginal=followOriginal;v.focused=!focus.empty();v.followedGraphics=followedGraphics;
            v.remaining=(stage==Stage::BaselineDelay || stage==Stage::ActionDelay || stage==Stage::FrozenDelay)?(now<start?start-now:0):
                (stage==Stage::Baseline || stage==Stage::Action || stage==Stage::FrozenCapture)?(now<end?end-now:0):0;
            v.baselineCalls=baseline.calls;v.actionCalls=action.calls;v.omitted=baseline.omitted+action.omitted;
            v.baselineComplete=baseline.complete;v.candidates=candidates;return v;
        }
    };
}
