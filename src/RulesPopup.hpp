#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "Rules.hpp"

// The rules editor that opens from the pause menu.
class RulesPopup : public geode::Popup {
public:
    static RulesPopup* create();

protected:
    std::vector<pc::Rule> m_rules;
    size_t m_index = 0;

    geode::TextInput* m_nameInput = nullptr;
    geode::TextInput* m_keysInput = nullptr;
    geode::TextInput* m_percentInput = nullptr;
    geode::TextInput* m_holdInput = nullptr;
    geode::TextInput* m_repeatInput = nullptr;
    geode::TextInput* m_deathDelayInput = nullptr;
    CCMenuItemToggler* m_enabledToggle = nullptr;
    ButtonSprite* m_playModeSpr = nullptr;
    ButtonSprite* m_deathModeSpr = nullptr;
    cocos2d::CCLabelBMFont* m_pageLabel = nullptr;

    bool init();
    void readCurrent();
    void writeCurrent();
    void deleteCurrent();

    void onPrev(cocos2d::CCObject*);
    void onNext(cocos2d::CCObject*);
    void onNew(cocos2d::CCObject*);
    void onDelete(cocos2d::CCObject*);
    void onPlayMode(cocos2d::CCObject*);
    void onDeathMode(cocos2d::CCObject*);
    void onSave(cocos2d::CCObject*);
    void onCancel(cocos2d::CCObject*);
    void onHelp(cocos2d::CCObject*);
    void onNoop(cocos2d::CCObject*) {}
};
