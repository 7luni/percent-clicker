#include "SystemInput.hpp"

#if defined(__APPLE__)
#include <TargetConditionals.h>
#endif

// ============================================================================
//  Windows: SendInput
// ============================================================================
#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

namespace pc {
    bool sendSystemKey(int vk, bool down) {
        INPUT in{};
        in.type = INPUT_KEYBOARD;
        in.ki.wVk = static_cast<WORD>(vk);
        in.ki.wScan = static_cast<WORD>(MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC));

        DWORD flags = 0;
        switch (vk) {
            case 33: case 34: case 35: case 36:   // pgup pgdn end home
            case 37: case 38: case 39: case 40:   // arrows
            case 45: case 46:                     // insert delete
            case 91:                              // windows key
                flags |= KEYEVENTF_EXTENDEDKEY;
                break;
            default: break;
        }
        if (!down) flags |= KEYEVENTF_KEYUP;
        in.ki.dwFlags = flags;

        return SendInput(1, &in, sizeof(INPUT)) == 1;
    }

    bool pollPressedKeys(std::vector<int>& out) {
        out.clear();
        for (int vk = 8; vk <= 222; vk++) {
            if (GetAsyncKeyState(vk) & 0x8000) out.push_back(vk);
        }
        return true;
    }
}

// ============================================================================
//  macOS: CoreGraphics (needs Accessibility permission)
// ============================================================================
#elif defined(__APPLE__) && TARGET_OS_OSX

#include <ApplicationServices/ApplicationServices.h>

namespace pc {
    namespace {
        // Windows virtual-key code (what the mod uses) -> macOS virtual key code.
        int toMacKey(int vk) {
            // letters
            static const int letters[26] = {
                0, 11, 8, 2, 14, 3, 5, 4, 34, 38, 40, 37, 46,   // a b c d e f g h i j k l m
                45, 31, 35, 12, 15, 1, 17, 32, 9, 13, 7, 16, 6   // n o p q r s t u v w x y z
            };
            if (vk >= 65 && vk <= 90) return letters[vk - 65];

            // digits 0-9
            static const int digits[10] = { 29, 18, 19, 20, 21, 23, 22, 26, 28, 25 };
            if (vk >= 48 && vk <= 57) return digits[vk - 48];

            // numpad 0-9
            static const int numpad[10] = { 82, 83, 84, 85, 86, 87, 88, 89, 91, 92 };
            if (vk >= 96 && vk <= 105) return numpad[vk - 96];

            // F1-F12
            static const int fkeys[12] = { 122, 120, 99, 118, 96, 97, 98, 100, 101, 109, 103, 111 };
            if (vk >= 112 && vk <= 123) return fkeys[vk - 112];

            switch (vk) {
                case 8: return 51;     // backspace
                case 9: return 48;     // tab
                case 13: return 36;    // return
                case 16: return 56;    // shift
                case 17: return 59;    // control
                case 18: return 58;    // option / alt
                case 20: return 57;    // caps lock
                case 27: return 53;    // escape
                case 32: return 49;    // space
                case 33: return 116;   // page up
                case 34: return 121;   // page down
                case 35: return 119;   // end
                case 36: return 115;   // home
                case 37: return 123;   // left
                case 38: return 126;   // up
                case 39: return 124;   // right
                case 40: return 125;   // down
                case 45: return 114;   // insert (help)
                case 46: return 117;   // forward delete
                case 91: return 55;    // command
                case 186: return 41;   // ;
                case 187: return 24;   // =
                case 188: return 43;   // ,
                case 189: return 27;   // -
                case 190: return 47;   // .
                case 191: return 44;   // /
                case 192: return 50;   // `
                case 219: return 33;   // [
                case 220: return 42;   // backslash
                case 221: return 30;   // ]
                case 222: return 39;   // '
                default: return -1;
            }
        }

        CGEventFlags modifierFlag(int vk) {
            switch (vk) {
                case 16: return kCGEventFlagMaskShift;
                case 17: return kCGEventFlagMaskControl;
                case 18: return kCGEventFlagMaskAlternate;
                case 91: return kCGEventFlagMaskCommand;
                default: return static_cast<CGEventFlags>(0);
            }
        }

        // macOS only lets an app fake key presses if it has Accessibility access.
        bool isTrusted() {
            static bool asked = false;
            if (AXIsProcessTrusted()) return true;
            if (!asked) {
                asked = true;
                const void* keys[] = { kAXTrustedCheckOptionPrompt };
                const void* vals[] = { kCFBooleanTrue };
                CFDictionaryRef opts = CFDictionaryCreate(
                    kCFAllocatorDefault, keys, vals, 1,
                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
                );
                AXIsProcessTrustedWithOptions(opts);   // shows the system prompt once
                CFRelease(opts);
            }
            return false;
        }

        CGEventFlags g_flags = static_cast<CGEventFlags>(0);
    }

    bool sendSystemKey(int vk, bool down) {
        int code = toMacKey(vk);
        if (code < 0) return false;
        if (!isTrusted()) return false;

        auto mod = modifierFlag(vk);
        if (mod) {
            g_flags = down ? static_cast<CGEventFlags>(g_flags | mod)
                           : static_cast<CGEventFlags>(g_flags & ~mod);
        }

        CGEventRef ev = CGEventCreateKeyboardEvent(nullptr, static_cast<CGKeyCode>(code), down);
        if (!ev) return false;
        CGEventSetFlags(ev, g_flags);
        CGEventPost(kCGHIDEventTap, ev);
        CFRelease(ev);
        return true;
    }

    bool pollPressedKeys(std::vector<int>& out) {
        out.clear();
        for (int vk = 8; vk <= 222; vk++) {
            int code = toMacKey(vk);
            if (code < 0) continue;
            if (CGEventSourceKeyState(kCGEventSourceStateHIDSystemState, static_cast<CGKeyCode>(code))) {
                out.push_back(vk);
            }
        }
        return true;
    }
}

// ============================================================================
//  Everything else (Android, iOS): not supported -> mod falls back to in-game input
// ============================================================================
#else

namespace pc {
    bool sendSystemKey(int, bool) { return false; }
    bool pollPressedKeys(std::vector<int>& out) { out.clear(); return false; }
}

#endif
