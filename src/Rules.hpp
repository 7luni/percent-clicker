#pragma once

#include <Geode/Geode.hpp>
#include <string>
#include <vector>

namespace pc {

    enum class DeathMode : int {
        Nothing = 0,    // do nothing when you die
        OnDeath = 1,    // press the moment you die
        OnRespawn = 2,  // press when the attempt restarts
    };

    enum class PlayMode : int {
        Any = 0,
        NormalOnly = 1,
        PracticeOnly = 2,
    };

    struct Rule {
        std::string name = "New rule";
        bool enabled = true;
        std::string keys = "space";   // e.g. "ctrl+alt+k"
        float percent = 50.f;         // level percentage that triggers the press
        float holdMs = 0.f;           // 0 = quick tap (1 frame)
        float repeatEvery = 0.f;      // 0 = only once per attempt
        DeathMode deathMode = DeathMode::Nothing;
        float deathDelayMs = 0.f;     // delay before the death re-press
        PlayMode playMode = PlayMode::Any;
    };

    // All rules (lazy-loaded from the mod's save data).
    std::vector<Rule>& rules();
    void saveRules();
    Rule makeDefaultRule(size_t number);

    // Parses "ctrl+alt+k" into key codes (modifiers first).
    // Returns "" on success, or the first token it didn't understand.
    std::string parseKeys(std::string const& text, std::vector<cocos2d::enumKeyCodes>& out);

    // Returns "" if the rule is valid, otherwise a human readable problem.
    std::string validateRule(Rule const& rule);

    std::string fmtNum(float value);
    float toFloat(std::string const& text, float fallback);

    char const* deathModeName(DeathMode mode);
    char const* playModeName(PlayMode mode);
}
