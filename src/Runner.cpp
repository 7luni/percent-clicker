#include "Runner.hpp"
#include "Rules.hpp"
#include "SystemInput.hpp"

#include <algorithm>

using namespace geode::prelude;

namespace {
    // The keyboard dispatcher's signature differs between GD versions
    // (with / without a timestamp). These two overloads pick whichever exists.
    template <class D>
    auto dispatchImpl(D* d, enumKeyCodes k, bool down, double ts, int)
        -> decltype(d->dispatchKeyboardMSG(k, down, false, ts), void()) {
        d->dispatchKeyboardMSG(k, down, false, ts);
    }

    template <class D>
    auto dispatchImpl(D* d, enumKeyCodes k, bool down, double, long)
        -> decltype(d->dispatchKeyboardMSG(k, down, false), void()) {
        d->dispatchKeyboardMSG(k, down, false);
    }

    void sendKey(enumKeyCodes key, bool down, pc::SendTarget target) {
        if (target == pc::SendTarget::Keyboard) {
            // Real OS-level key press (Windows). Other apps like Discord see it.
            if (pc::sendSystemKey(static_cast<int>(key), down)) return;
            // Not supported on this platform: fall back to in-game input.
        }
        auto dispatcher = CCDirector::get()->getKeyboardDispatcher();
        if (!dispatcher) return;
        double ts = std::chrono::duration<double>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        dispatchImpl(dispatcher, key, down, ts, 0);
    }

    bool modEnabled() {
        return Mod::get()->getSettingValue<bool>("enabled");
    }

    bool modeMatches(pc::Rule const& r, PlayLayer* pl) {
        switch (r.playMode) {
            case pc::PlayMode::NormalOnly: return !pl->m_isPracticeMode;
            case pc::PlayMode::PracticeOnly: return pl->m_isPracticeMode;
            default: return true;
        }
    }
}

namespace pc {

    Runner& Runner::get() {
        static Runner s_instance;
        return s_instance;
    }

    void Runner::reset() {
        releaseAll();
        m_pending.clear();
        m_states.clear();
        m_needsInit = true;
        m_diedSinceReset = false;
    }

    void Runner::invalidate() {
        m_states.clear();
        m_needsInit = true;
    }

    void Runner::releaseAll() {
        for (auto it = m_held.rbegin(); it != m_held.rend(); ++it) {
            for (auto kit = it->keys.rbegin(); kit != it->keys.rend(); ++kit) {
                sendKey(*kit, false, it->target);
            }
        }
        m_held.clear();
    }

    void Runner::press(std::string const& keys, float holdMs, SendTarget target) {
        std::vector<enumKeyCodes> parsed;
        auto bad = parseKeys(keys, parsed);
        if (!bad.empty()) {
            log::warn("Percent Clicker: unknown key \"{}\" in \"{}\"", bad, keys);
            return;
        }
        for (auto k : parsed) sendKey(k, true, target);

        holdMs = std::clamp(holdMs, 0.f, 10000.f);
        auto hold = std::chrono::duration_cast<Clock::duration>(
            std::chrono::duration<float, std::milli>(holdMs)
        );
        m_held.push_back({ parsed, target, Clock::now() + hold });

        if (Mod::get()->getSettingValue<bool>("debug-log")) {
            log::info("Percent Clicker: pressed \"{}\" (hold {} ms)", keys, holdMs);
        }
    }

    void Runner::releaseDue(Clock::time_point now) {
        for (auto it = m_held.begin(); it != m_held.end();) {
            if (now >= it->releaseAt) {
                for (auto kit = it->keys.rbegin(); kit != it->keys.rend(); ++kit) {
                    sendKey(*kit, false, it->target);
                }
                it = m_held.erase(it);
            } else {
                ++it;
            }
        }
    }

    void Runner::firePending(Clock::time_point now) {
        if (m_pending.empty()) return;
        if (!modEnabled()) {
            m_pending.clear();
            return;
        }
        std::vector<Pending> due;
        for (auto it = m_pending.begin(); it != m_pending.end();) {
            if (now >= it->fireAt) {
                due.push_back(*it);
                it = m_pending.erase(it);
            } else {
                ++it;
            }
        }
        for (auto const& p : due) this->press(p.keys, p.holdMs, p.target);
    }

    void Runner::initStates(PlayLayer* layer) {
        auto& rs = rules();
        float pct = layer->getCurrentPercent();
        m_states.assign(rs.size(), State{});
        for (size_t i = 0; i < rs.size(); i++) {
            auto& r = rs[i];
            auto& st = m_states[i];
            st.next = r.percent;
            // Don't instantly fire rules that are "behind" us, e.g. after
            // respawning from a practice checkpoint at 60%.
            if (r.repeatEvery > 0.01f) {
                while (st.next < pct - 0.3f) st.next += r.repeatEvery;
                if (st.next > 100.f) st.done = true;
            } else if (r.percent < pct - 0.3f) {
                st.done = true;
            }
        }
        m_needsInit = false;
    }

    void Runner::scheduleDeathPresses(PlayLayer* layer, bool onRespawn) {
        if (!modEnabled()) return;
        auto now = Clock::now();
        auto wanted = onRespawn ? DeathMode::OnRespawn : DeathMode::OnDeath;
        for (auto const& r : rules()) {
            if (!r.enabled || r.deathMode != wanted) continue;
            if (layer && !modeMatches(r, layer)) continue;
            auto delay = std::chrono::duration_cast<Clock::duration>(
                std::chrono::duration<float, std::milli>(std::clamp(r.deathDelayMs, 0.f, 10000.f))
            );
            m_pending.push_back({ r.keys, r.holdMs, r.target, now + delay });
        }
    }

    void Runner::onDeath() {
        this->releaseAll();
        m_diedSinceReset = true;
        this->scheduleDeathPresses(PlayLayer::get(), false);
    }

    void Runner::onReset() {
        // New attempt: rules may fire again from the start.
        m_needsInit = true;
        if (m_diedSinceReset) {
            m_diedSinceReset = false;
            this->scheduleDeathPresses(PlayLayer::get(), true);
        }
    }

    void Runner::tick(PlayLayer* layer) {
        auto now = Clock::now();
        this->releaseDue(now);
        this->firePending(now);

        if (!layer || !modEnabled()) return;
        if (!layer->m_player1 || layer->m_player1->m_isDead) return;

        auto& rs = rules();
        if (m_needsInit || m_states.size() != rs.size()) this->initStates(layer);

        float pct = layer->getCurrentPercent();
        for (size_t i = 0; i < rs.size(); i++) {
            auto& r = rs[i];
            auto& st = m_states[i];
            if (!r.enabled || st.done || !modeMatches(r, layer)) continue;
            if (pct + 1e-4f < st.next) continue;

            this->press(r.keys, r.holdMs, r.target);

            if (r.repeatEvery > 0.01f) {
                do {
                    st.next += r.repeatEvery;
                } while (st.next <= pct);
                if (st.next > 100.f) st.done = true;
            } else {
                st.done = true;
            }
        }
    }
}
