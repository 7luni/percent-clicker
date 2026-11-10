#include "RulesPopup.hpp"
#include "Runner.hpp"
#include "SystemInput.hpp"

using namespace geode::prelude;

RulesPopup* RulesPopup::create() {
    auto ret = new RulesPopup();
    if (ret->init()) {
        ret->autorelease();
        return ret;
    }
    delete ret;
    return nullptr;
}

bool RulesPopup::init() {
    if (!Popup::init(440.f, 290.f)) return false;
    this->setTitle("Percent Clicker");

    m_rules = pc::rules();
    if (m_rules.empty()) m_rules.push_back(pc::makeDefaultRule(1));

    // ---------- small helpers ----------
    auto addLabel = [&](char const* text, float left, float y, float scale) {
        auto l = CCLabelBMFont::create(text, "goldFont.fnt");
        l->setScale(scale);
        m_mainLayer->addChildAtPosition(l, Anchor::Center, { left + l->getScaledContentWidth() / 2.f, y });
        return l;
    };
    auto addInput = [&](float x, float y, float w, char const* placeholder, std::string const& filter, size_t maxLen) {
        auto in = TextInput::create(w, placeholder, "bigFont.fnt");
        if (!filter.empty()) in->setFilter(filter);
        in->setMaxCharCount(maxLen);
        m_mainLayer->addChildAtPosition(in, Anchor::Center, { x, y });
        return in;
    };
    auto addButton = [&](char const* text, float scale, CCPoint pos, SEL_MenuHandler cb, ButtonSprite** out) {
        auto spr = ButtonSprite::create(text, "goldFont.fnt", "GJ_button_04.png", .8f);
        spr->setScale(scale);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, cb);
        m_buttonMenu->addChildAtPosition(btn, Anchor::Center, pos);
        if (out) *out = spr;
        return btn;
    };
    auto addArrow = [&](bool right, CCPoint pos, SEL_MenuHandler cb) {
        auto spr = CCSprite::createWithSpriteFrameName("GJ_arrow_01_001.png");
        spr->setFlipX(right);
        spr->setScale(.6f);
        auto btn = CCMenuItemSpriteExtra::create(spr, this, cb);
        m_buttonMenu->addChildAtPosition(btn, Anchor::Center, pos);
        return btn;
    };

    std::string const numeric = "0123456789.";

    // ---------- navigation row ----------
    addArrow(false, { -200.f, 100.f }, menu_selector(RulesPopup::onPrev));
    m_pageLabel = CCLabelBMFont::create("Rule 1/1", "bigFont.fnt");
    m_pageLabel->setScale(.4f);
    m_mainLayer->addChildAtPosition(m_pageLabel, Anchor::Center, { -148.f, 100.f });
    addArrow(true, { -96.f, 100.f }, menu_selector(RulesPopup::onNext));
    addButton("New", .55f, { -42.f, 100.f }, menu_selector(RulesPopup::onNew), nullptr);
    addButton("Delete", .55f, { 22.f, 100.f }, menu_selector(RulesPopup::onDelete), nullptr);

    addLabel("Enabled", 100.f, 100.f, .5f);
    m_enabledToggle = CCMenuItemToggler::createWithStandardSprites(this, menu_selector(RulesPopup::onNoop), .6f);
    m_buttonMenu->addChildAtPosition(m_enabledToggle, Anchor::Center, { 190.f, 100.f });

    // ---------- left column ----------
    addLabel("Name", -205.f, 62.f, .5f);
    m_nameInput = addInput(-80.f, 62.f, 150.f, "Rule name", "", 24);

    addLabel("Keys", -205.f, 28.f, .5f);
    m_keysInput = addInput(-108.f, 28.f, 96.f, "ctrl+alt+k", "", 40);
    addButton("Record", .45f, { -28.f, 28.f }, menu_selector(RulesPopup::onRecord), &m_recordSpr);

    addLabel("At % (0-100)", -205.f, -6.f, .45f);
    m_percentInput = addInput(-80.f, -6.f, 150.f, "50", numeric, 6);

    addLabel("Hold (ms)", -205.f, -40.f, .5f);
    m_holdInput = addInput(-80.f, -40.f, 150.f, "0 = tap", numeric, 6);

    addLabel("Repeat every %", -205.f, -74.f, .42f);
    m_repeatInput = addInput(-80.f, -74.f, 150.f, "0 = once", numeric, 6);

    // ---------- right column ----------
    addLabel("Play mode", 20.f, 62.f, .5f);
    addButton("Any mode", .5f, { 160.f, 62.f }, menu_selector(RulesPopup::onPlayMode), &m_playModeSpr);

    addLabel("When I die", 20.f, 28.f, .5f);
    addButton("Nothing", .5f, { 160.f, 28.f }, menu_selector(RulesPopup::onDeathMode), &m_deathModeSpr);

    addLabel("Re-press delay (ms)", 20.f, -6.f, .36f);
    m_deathDelayInput = addInput(165.f, -6.f, 80.f, "0", numeric, 6);

    addLabel("Send to", 20.f, -40.f, .5f);
    addButton("Real keyboard", .5f, { 160.f, -40.f }, menu_selector(RulesPopup::onTarget), &m_targetSpr);

    auto hint1 = CCLabelBMFont::create("Record: hold all keys at once", "chatFont.fnt");
    hint1->setScale(.55f);
    hint1->setColor({ 170, 180, 200 });
    m_mainLayer->addChildAtPosition(hint1, Anchor::Center, { 120.f, -68.f });
    auto hint2 = CCLabelBMFont::create("Real keyboard = Discord can see it", "chatFont.fnt");
    hint2->setScale(.55f);
    hint2->setColor({ 170, 180, 200 });
    m_mainLayer->addChildAtPosition(hint2, Anchor::Center, { 120.f, -84.f });

    // ---------- bottom buttons ----------
    addButton("Cancel", .7f, { -95.f, -118.f }, menu_selector(RulesPopup::onCancel), nullptr);
    addButton("Help", .7f, { 0.f, -118.f }, menu_selector(RulesPopup::onHelp), nullptr);
    addButton("Save", .7f, { 95.f, -118.f }, menu_selector(RulesPopup::onSave), nullptr);

    this->writeCurrent();
    return true;
}

void RulesPopup::readCurrent() {
    if (m_recording) this->stopRecording(true);
    if (m_rules.empty()) return;
    auto& r = m_rules[m_index];
    r.name = m_nameInput->getString();
    if (r.name.empty()) r.name = "Rule " + std::to_string(m_index + 1);
    r.enabled = m_enabledToggle->isToggled();
    r.keys = m_keysInput->getString();
    r.percent = pc::toFloat(m_percentInput->getString(), r.percent);
    r.holdMs = pc::toFloat(m_holdInput->getString(), 0.f);
    r.repeatEvery = pc::toFloat(m_repeatInput->getString(), 0.f);
    r.deathDelayMs = pc::toFloat(m_deathDelayInput->getString(), 0.f);
}

void RulesPopup::writeCurrent() {
    auto const& r = m_rules[m_index];
    m_nameInput->setString(r.name);
    m_keysInput->setString(r.keys);
    m_percentInput->setString(pc::fmtNum(r.percent));
    m_holdInput->setString(pc::fmtNum(r.holdMs));
    m_repeatInput->setString(pc::fmtNum(r.repeatEvery));
    m_deathDelayInput->setString(pc::fmtNum(r.deathDelayMs));
    m_enabledToggle->toggle(r.enabled);
    m_playModeSpr->setString(pc::playModeName(r.playMode));
    m_deathModeSpr->setString(pc::deathModeName(r.deathMode));
    m_targetSpr->setString(pc::sendTargetName(r.target));
    auto page = fmt::format("Rule {}/{}", m_index + 1, m_rules.size());
    m_pageLabel->setString(page.c_str());
}

void RulesPopup::onPrev(CCObject*) {
    this->readCurrent();
    m_index = (m_index + m_rules.size() - 1) % m_rules.size();
    this->writeCurrent();
}

void RulesPopup::onNext(CCObject*) {
    this->readCurrent();
    m_index = (m_index + 1) % m_rules.size();
    this->writeCurrent();
}

void RulesPopup::onNew(CCObject*) {
    this->readCurrent();
    m_rules.push_back(pc::makeDefaultRule(m_rules.size() + 1));
    m_index = m_rules.size() - 1;
    this->writeCurrent();
}

void RulesPopup::deleteCurrent() {
    if (m_rules.size() <= 1) {
        m_rules[0] = pc::makeDefaultRule(1);
        m_index = 0;
    } else {
        m_rules.erase(m_rules.begin() + m_index);
        if (m_index >= m_rules.size()) m_index = m_rules.size() - 1;
    }
    this->writeCurrent();
}

void RulesPopup::onDelete(CCObject*) {
    createQuickPopup(
        "Delete rule",
        "Delete this rule?",
        "Cancel", "Delete",
        [this](FLAlertLayer*, bool btn2) {
            if (btn2) this->deleteCurrent();
        }
    );
}

void RulesPopup::onPlayMode(CCObject*) {
    this->readCurrent();
    auto& r = m_rules[m_index];
    r.playMode = static_cast<pc::PlayMode>((static_cast<int>(r.playMode) + 1) % 3);
    m_playModeSpr->setString(pc::playModeName(r.playMode));
}

void RulesPopup::onDeathMode(CCObject*) {
    this->readCurrent();
    auto& r = m_rules[m_index];
    r.deathMode = static_cast<pc::DeathMode>((static_cast<int>(r.deathMode) + 1) % 3);
    m_deathModeSpr->setString(pc::deathModeName(r.deathMode));
}

void RulesPopup::startRecording() {
    std::vector<int> down;
    if (!pc::pollPressedKeys(down)) {
        FLAlertLayer::create(
            "Not available",
            "Key recording isn't supported on this platform.\nType the keys instead, e.g. <cg>ctrl+alt+k</c>.",
            "OK"
        )->show();
        return;
    }
    m_recording = true;
    m_seen.clear();
    m_ignore.clear();
    // Keys already held when recording starts are ignored until released.
    for (int k : down) {
        if (!pc::keyName(k).empty()) m_ignore.insert(k);
    }
    m_keysBeforeRecord = m_keysInput->getString();
    m_recordSpr->setString("Press keys...");
    this->schedule(schedule_selector(RulesPopup::onPoll));
}

void RulesPopup::stopRecording(bool cancel) {
    if (!m_recording) return;
    m_recording = false;
    this->unschedule(schedule_selector(RulesPopup::onPoll));
    m_recordSpr->setString("Record");
    if (cancel) m_keysInput->setString(m_keysBeforeRecord);
}

void RulesPopup::onPoll(float) {
    if (!m_recording) return;

    std::vector<int> down;
    pc::pollPressedKeys(down);

    std::set<int> now;
    for (int k : down) {
        if (!pc::keyName(k).empty()) now.insert(k);
    }

    // forget ignored keys once they were released
    for (auto it = m_ignore.begin(); it != m_ignore.end();) {
        if (!now.count(*it)) it = m_ignore.erase(it);
        else ++it;
    }

    bool anyFresh = false;
    for (int k : now) {
        if (m_ignore.count(k)) continue;
        anyFresh = true;
        m_seen.insert(k);
    }

    if (!m_seen.empty()) {
        std::vector<int> v(m_seen.begin(), m_seen.end());
        m_keysInput->setString(pc::formatKeys(v));
    }

    // everything released after pressing something -> done
    if (!m_seen.empty() && !anyFresh) this->stopRecording(false);
}

void RulesPopup::onRecord(CCObject*) {
    if (m_recording) this->stopRecording(true);
    else this->startRecording();
}

void RulesPopup::onTarget(CCObject*) {
    this->readCurrent();
    auto& r = m_rules[m_index];
    r.target = r.target == pc::SendTarget::Game ? pc::SendTarget::Keyboard : pc::SendTarget::Game;
    m_targetSpr->setString(pc::sendTargetName(r.target));
}

void RulesPopup::onSave(CCObject*) {
    this->readCurrent();

    std::string problems;
    for (size_t i = 0; i < m_rules.size(); i++) {
        auto err = pc::validateRule(m_rules[i]);
        if (!err.empty()) {
            problems += fmt::format("Rule {}: {}\n", i + 1, err);
        }
    }
    if (!problems.empty()) {
        FLAlertLayer::create("Can't save yet", problems, "OK")->show();
        return;
    }

    pc::rules() = m_rules;
    pc::saveRules();
    pc::Runner::get().invalidate();
    this->keyBackClicked();
}

void RulesPopup::onCancel(CCObject*) {
    this->keyBackClicked();
}

void RulesPopup::onHelp(CCObject*) {
    FLAlertLayer::create(
        "Keys & combos",
        "Click <cy>Record</c>, hold your keys together, let go\n"
        "Or type them: <cg>ctrl+alt+k</c>, joined with <cy>+</c>\n"
        "a-z, 0-9, f1-f12, num0-num9, arrows\n"
        "space, enter, tab, esc, ctrl, shift, alt\n"
        "<cy>Hold</c> = ms keys stay down (0 = tap)\n"
        "<cy>Repeat</c> = press again every X%\n"
        "<cy>Send to</c> = Game or real keyboard",
        "OK"
    )->show();
}
