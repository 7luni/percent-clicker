#include "Rules.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

using namespace geode::prelude;

namespace pc {

    namespace {
        std::vector<std::string> split(std::string const& s, char sep) {
            std::vector<std::string> out;
            std::string cur;
            for (char c : s) {
                if (c == sep) {
                    out.push_back(cur);
                    cur.clear();
                } else {
                    cur.push_back(c);
                }
            }
            out.push_back(cur);
            return out;
        }

        std::string trim(std::string const& s) {
            size_t a = 0, b = s.size();
            while (a < b && std::isspace(static_cast<unsigned char>(s[a]))) a++;
            while (b > a && std::isspace(static_cast<unsigned char>(s[b - 1]))) b--;
            return s.substr(a, b - a);
        }

        std::string lower(std::string s) {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        }

        std::string sanitize(std::string s) {
            for (auto& c : s) {
                if (c == '|' || c == '\n' || c == '\r') c = ' ';
            }
            return s;
        }

        bool isModifier(int code) {
            return code == 16 || code == 17 || code == 18 || code == 91;
        }

        // enumKeyCodes mirror the Windows virtual-key codes, so we can build
        // them from plain numbers.
        bool lookupKey(std::string const& t, enumKeyCodes& out) {
            auto k = [&](int v) {
                out = static_cast<enumKeyCodes>(v);
                return true;
            };

            if (t.size() == 1) {
                char c = t[0];
                if (c >= 'a' && c <= 'z') return k('A' + (c - 'a'));
                if (c >= '0' && c <= '9') return k(c);
                switch (c) {
                    case ';': return k(186);
                    case '=': return k(187);
                    case ',': return k(188);
                    case '-': return k(189);
                    case '.': return k(190);
                    case '/': return k(191);
                    case '`': return k(192);
                    case '[': return k(219);
                    case '\\': return k(220);
                    case ']': return k(221);
                    case '\'': return k(222);
                    default: break;
                }
            }

            static const std::unordered_map<std::string, int> named = {
                {"ctrl", 17}, {"control", 17},
                {"shift", 16},
                {"alt", 18}, {"option", 18}, {"opt", 18},
                {"cmd", 91}, {"command", 91}, {"win", 91}, {"windows", 91}, {"super", 91},
                {"space", 32}, {"spacebar", 32}, {"jump", 32},
                {"enter", 13}, {"return", 13},
                {"tab", 9},
                {"esc", 27}, {"escape", 27},
                {"backspace", 8},
                {"delete", 46}, {"del", 46},
                {"insert", 45}, {"ins", 45},
                {"home", 36}, {"end", 35},
                {"pageup", 33}, {"pgup", 33},
                {"pagedown", 34}, {"pgdn", 34},
                {"up", 38}, {"down", 40}, {"left", 37}, {"right", 39},
                {"capslock", 20},
            };
            if (auto it = named.find(t); it != named.end()) return k(it->second);

            // f1 .. f12
            if (t.size() >= 2 && t[0] == 'f' &&
                std::all_of(t.begin() + 1, t.end(), [](unsigned char c) { return std::isdigit(c); })) {
                int n = std::atoi(t.c_str() + 1);
                if (n >= 1 && n <= 12) return k(111 + n);
            }
            // num0 .. num9 (numpad)
            if (t.size() == 4 && t.rfind("num", 0) == 0 && std::isdigit(static_cast<unsigned char>(t[3]))) {
                return k(96 + (t[3] - '0'));
            }
            return false;
        }

        std::string serialize(std::vector<Rule> const& rs) {
            std::string out;
            for (auto const& r : rs) {
                out += sanitize(r.name) + "|" + (r.enabled ? "1" : "0") + "|" + sanitize(r.keys) + "|" +
                    fmtNum(r.percent) + "|" + fmtNum(r.holdMs) + "|" + fmtNum(r.repeatEvery) + "|" +
                    std::to_string(static_cast<int>(r.deathMode)) + "|" + fmtNum(r.deathDelayMs) + "|" +
                    std::to_string(static_cast<int>(r.playMode)) + "|" +
                    std::to_string(static_cast<int>(r.target)) + "\n";
            }
            return out;
        }

        std::vector<Rule> deserialize(std::string const& text) {
            std::vector<Rule> out;
            for (auto const& line : split(text, '\n')) {
                if (trim(line).empty()) continue;
                auto f = split(line, '|');
                if (f.size() < 9) continue;
                Rule r;
                r.name = f[0];
                r.enabled = f[1] == "1";
                r.keys = f[2];
                r.percent = toFloat(f[3], 50.f);
                r.holdMs = toFloat(f[4], 0.f);
                r.repeatEvery = toFloat(f[5], 0.f);
                int dm = static_cast<int>(toFloat(f[6], 0.f));
                r.deathMode = static_cast<DeathMode>(std::clamp(dm, 0, 2));
                r.deathDelayMs = toFloat(f[7], 0.f);
                int pm = static_cast<int>(toFloat(f[8], 0.f));
                r.playMode = static_cast<PlayMode>(std::clamp(pm, 0, 2));
                // Rules saved by v1.0.0 have no target field: keep them game-only.
                int tg = f.size() >= 10 ? static_cast<int>(toFloat(f[9], 0.f)) : 0;
                r.target = static_cast<SendTarget>(std::clamp(tg, 0, 1));
                out.push_back(std::move(r));
            }
            return out;
        }
    }

    std::string keyName(int vk) {
        if (vk >= 65 && vk <= 90) return std::string(1, static_cast<char>('a' + (vk - 65)));
        if (vk >= 48 && vk <= 57) return std::string(1, static_cast<char>('0' + (vk - 48)));
        if (vk >= 96 && vk <= 105) return "num" + std::to_string(vk - 96);
        if (vk >= 112 && vk <= 123) return "f" + std::to_string(vk - 111);
        switch (vk) {
            case 17: return "ctrl";
            case 16: return "shift";
            case 18: return "alt";
            case 91: return "cmd";
            case 32: return "space";
            case 13: return "enter";
            case 9: return "tab";
            case 27: return "esc";
            case 8: return "backspace";
            case 46: return "delete";
            case 45: return "insert";
            case 36: return "home";
            case 35: return "end";
            case 33: return "pageup";
            case 34: return "pagedown";
            case 37: return "left";
            case 38: return "up";
            case 39: return "right";
            case 40: return "down";
            case 20: return "capslock";
            case 186: return ";";
            case 187: return "=";
            case 188: return ",";
            case 189: return "-";
            case 190: return ".";
            case 191: return "/";
            case 192: return "`";
            case 219: return "[";
            case 220: return "\\";
            case 221: return "]";
            case 222: return "'";
            default: return "";
        }
    }

    std::string formatKeys(std::vector<int> const& vks) {
        std::vector<int> ordered;
        for (int m : { 17, 18, 16, 91 }) {
            if (std::find(vks.begin(), vks.end(), m) != vks.end()) ordered.push_back(m);
        }
        for (int k : vks) {
            if (!isModifier(k)) ordered.push_back(k);
        }
        std::string out;
        for (int k : ordered) {
            auto name = keyName(k);
            if (name.empty()) continue;
            if (!out.empty()) out += "+";
            out += name;
        }
        return out;
    }

    float toFloat(std::string const& text, float fallback) {
        auto s = trim(text);
        if (s.empty()) return fallback;
        char* end = nullptr;
        float v = std::strtof(s.c_str(), &end);
        if (end == s.c_str()) return fallback;
        return v;
    }

    std::string fmtNum(float value) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%.2f", value);
        std::string s = buf;
        if (s.find('.') != std::string::npos) {
            while (!s.empty() && s.back() == '0') s.pop_back();
            if (!s.empty() && s.back() == '.') s.pop_back();
        }
        return s.empty() ? "0" : s;
    }

    Rule makeDefaultRule(size_t number) {
        Rule r;
        r.name = "Rule " + std::to_string(number);
        return r;
    }

    std::vector<Rule>& rules() {
        static std::vector<Rule> s_rules;
        static bool s_loaded = false;
        if (!s_loaded) {
            s_loaded = true;
            auto raw = Mod::get()->getSavedValue<std::string>("rules-v1", "__unset__");
            if (raw == "__unset__") {
                // First launch: give the user one example (disabled) rule.
                Rule ex;
                ex.name = "Example: tap jump at 50%";
                ex.enabled = false;
                ex.keys = "space";
                ex.percent = 50.f;
                s_rules.push_back(ex);
            } else {
                s_rules = deserialize(raw);
            }
        }
        return s_rules;
    }

    void saveRules() {
        Mod::get()->setSavedValue<std::string>("rules-v1", serialize(rules()));
    }

    std::string parseKeys(std::string const& text, std::vector<enumKeyCodes>& out) {
        out.clear();
        std::vector<enumKeyCodes> mods, normal;
        for (auto const& raw : split(text, '+')) {
            auto tok = lower(trim(raw));
            if (tok.empty()) continue;
            enumKeyCodes code;
            if (!lookupKey(tok, code)) return tok;
            auto& bucket = isModifier(static_cast<int>(code)) ? mods : normal;
            if (std::find(bucket.begin(), bucket.end(), code) == bucket.end()) bucket.push_back(code);
        }
        out.insert(out.end(), mods.begin(), mods.end());
        out.insert(out.end(), normal.begin(), normal.end());
        if (out.empty()) return "(no keys)";
        return "";
    }

    std::string validateRule(Rule const& r) {
        std::vector<enumKeyCodes> tmp;
        auto bad = parseKeys(r.keys, tmp);
        if (!bad.empty()) return "unknown key \"" + bad + "\"";
        if (r.percent < 0.f || r.percent > 100.f) return "percent must be between 0 and 100";
        if (r.holdMs < 0.f || r.holdMs > 10000.f) return "hold must be between 0 and 10000 ms";
        if (r.repeatEvery < 0.f || r.repeatEvery > 100.f) return "repeat must be between 0 and 100";
        if (r.deathDelayMs < 0.f || r.deathDelayMs > 10000.f) return "death delay must be between 0 and 10000 ms";
        return "";
    }

    char const* deathModeName(DeathMode m) {
        switch (m) {
            case DeathMode::OnDeath: return "On death";
            case DeathMode::OnRespawn: return "On respawn";
            default: return "Nothing";
        }
    }

    char const* sendTargetName(SendTarget t) {
        return t == SendTarget::Keyboard ? "Real keyboard" : "Game only";
    }

    char const* playModeName(PlayMode m) {
        switch (m) {
            case PlayMode::NormalOnly: return "Normal only";
            case PlayMode::PracticeOnly: return "Practice only";
            default: return "Any mode";
        }
    }
}
