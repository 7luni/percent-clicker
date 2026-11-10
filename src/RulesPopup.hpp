#pragma once

#include <Geode/Geode.hpp>
#include <Geode/ui/Popup.hpp>
#include <Geode/ui/TextInput.hpp>
#include "Rules.hpp"
#include <set>

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
    ButtonSprite* m_targetSpr = nullptr;
    ButtonSprite* m_recordSpr = nullptr;

    // key recording
    bool m_recording = false;
    std::set<int> m_seen;
    std::set<int> m_ignore;
    std::string m_keysBeforeRecord;
    cocos2d::CCLabelBMFont* m_pageLabel = nullptr;

    bool init();
    void readCurrent();
    void writeCurrent();
    void deleteCurrent();
    void startRecording();
    void stopRecording(bool cancel);
    void onPoll(float dt);

    void onPrev(cocos2d::CCObject*);
    void onNext(cocos2d::CCObject*);
    void onNew(cocos2d::CCObject*);
    void onDelete(cocos2d::CCObject*);
    void onPlayMode(cocos2d::CCObject*);
    void onDeathMode(cocos2d::CCObject*);
    void onTarget(cocos2d::CCObject*);
    void onRecord(cocos2d::CCObject*);
    void onSave(cocos2d::CCObject*);
    void onCancel(cocos2d::CCObject*);
    void onHelp(cocos2d::CCObject*);
    void onNoop(cocos2d::CCObject*) {}
};
