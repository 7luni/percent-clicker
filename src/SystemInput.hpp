#pragma once

#include <vector>

namespace pc {
    // Sends a REAL keyboard event to the operating system (so other programs,
    // like Discord, see it). `vk` is a Windows virtual-key code.
    // Returns false when unsupported on this platform.
    bool sendSystemKey(int vk, bool down);

    // Lists the keys physically held down right now (Windows virtual-key codes).
    // Used by the "Record" button. Returns false when unsupported.
    bool pollPressedKeys(std::vector<int>& out);
}
