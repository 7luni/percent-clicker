#include <Geode/Geode.hpp>
#include <Geode/modify/PlayLayer.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/loader/GameEvent.hpp>
#include <Geode/loader/SettingV3.hpp>

#include "Runner.hpp"
#include "RulesPopup.hpp"

using namespace geode::prelude;

// ---------------------------------------------------------------------------
// Level hooks: run the rules every frame, detect deaths, release keys safely.
// ---------------------------------------------------------------------------
class $modify(PCPlayLayer, PlayLayer) {
    void setupHasCompleted() {
        PlayLayer::setupHasCompleted();
        pc::Runner::get().reset();
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        pc::Runner::get().onReset();
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        pc::Runner::get().tick(this);
    }

    void destroyPlayer(PlayerObject* player, GameObject* object) {
        if (object == m_anticheatSpike) {
            PlayLayer::destroyPlayer(player, object);
            return;
        }
        bool wasDead = m_player1 && m_player1->m_isDead;
        PlayLayer::destroyPlayer(player, object);
        if (!wasDead && m_player1 && m_player1->m_isDead) {
            pc::Runner::get().onDeath();
        }
    }

    void pauseGame(bool unfocused) {
        pc::Runner::get().releaseAll();
        PlayLayer::pauseGame(unfocused);
    }

    void levelComplete() {
        pc::Runner::get().releaseAll();
        PlayLayer::levelComplete();
    }

    void onQuit() {
        pc::Runner::get().reset();
        PlayLayer::onQuit();
    }
};

// ---------------------------------------------------------------------------
// Pause menu button that opens the rules editor.
// ---------------------------------------------------------------------------
class $modify(PCPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        if (!Mod::get()->getSettingValue<bool>("show-pause-button")) return;

        CCNode* icon = nullptr;
        if (auto spr = CCSprite::create("pause-icon.png"_spr)) {
            icon = spr;
        } else {
            auto fallback = ButtonSprite::create("%");
            fallback->setScale(.7f);
            icon = fallback;
        }

        auto btn = CCMenuItemSpriteExtra::create(
            icon, this, menu_selector(PCPauseLayer::onPercentClicker)
        );
        btn->setID("percent-clicker-button"_spr);

        if (auto menu = this->getChildByID("right-button-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        } else {
            // Fallback if another mod removed/renamed the right-side menu.
            auto win = CCDirector::get()->getWinSize();
            auto menu2 = CCMenu::create();
            menu2->setPosition({ win.width - 40.f, 120.f });
            menu2->addChild(btn);
            this->addChild(menu2);
        }
    }

    void onPercentClicker(CCObject*) {
        if (auto popup = RulesPopup::create()) popup->show();
    }
};

// ---------------------------------------------------------------------------
// Global on/off keybind (configurable in the mod settings).
// ---------------------------------------------------------------------------
$on_game(Loaded) {
    listenForKeybindSettingPresses("toggle-keybind", [](Keybind const&, bool down, bool repeat, double) {
        if (!down || repeat) return;
        if (!PlayLayer::get()) return;

        auto mod = Mod::get();
        bool now = !mod->getSettingValue<bool>("enabled");
        mod->setSettingValue<bool>("enabled", now);
        if (!now) pc::Runner::get().releaseAll();

        if (mod->getSettingValue<bool>("show-notifications")) {
            Notification::create(
                now ? "Percent Clicker: ON" : "Percent Clicker: OFF",
                now ? NotificationIcon::Success : NotificationIcon::Warning,
                1.f
            )->show();
        }
    });
}
