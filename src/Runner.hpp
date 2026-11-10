#pragma once

#include <Geode/Geode.hpp>
#include <chrono>
#include <string>
#include <vector>
#include "Rules.hpp"

namespace pc {

    // Watches the running level, fires the rules, and releases keys again.
    class Runner {
    public:
        static Runner& get();

        // A new level session started (or ended): drop everything.
        void reset();
        // The attempt restarted (respawn / restart / practice checkpoint).
        void onReset();
        // The player died.
        void onDeath();
        // Called every frame while the level is running.
        void tick(PlayLayer* layer);
        // Forget per-attempt progress (used after the rules were edited).
        void invalidate();
        // Release every key we are currently holding.
        void releaseAll();

    private:
        using Clock = std::chrono::steady_clock;

        struct Held {
            std::vector<cocos2d::enumKeyCodes> keys;
            SendTarget target;
            Clock::time_point releaseAt;
        };
        struct Pending {
            std::string keys;
            float holdMs;
            SendTarget target;
            Clock::time_point fireAt;
        };
        struct State {
            bool done = false;
            float next = 0.f;
        };

        void press(std::string const& keys, float holdMs, SendTarget target);
        void releaseDue(Clock::time_point now);
        void firePending(Clock::time_point now);
        void initStates(PlayLayer* layer);
        void scheduleDeathPresses(PlayLayer* layer, bool onRespawn);

        std::vector<Held> m_held;
        std::vector<Pending> m_pending;
        std::vector<State> m_states;
        bool m_needsInit = true;
        bool m_diedSinceReset = false;
    };
}
