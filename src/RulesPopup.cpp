#include "RulesPopup.hpp"
#include "Runner.hpp"

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
    m_keysInput = addInput(-80.f, 28.f, 150.f, "ctrl+alt+k", "", 40);

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

    auto hint1 = CCLabelBMFont::create("Combos: ctrl+alt+k, shift+f5", "chatFont.fnt");
    hint1->setScale(.55f);
    hint1->setColor({ 170, 180, 200 });
    m_mainLayer->addChildAtPosition(hint1, Anchor::Center, { 120.f, -44.f });
    auto hint2 = CCLabelBMFont::create("Hold 0 = quick tap. Repeat 0 = once.", "chatFont.fnt");
    hint2->setScale(.55f);
    hint2->setColor({ 170, 180, 200 });
    m_mainLayer->addChildAtPosition(hint2, Anchor::Center, { 120.f, -60.f });
    auto hint3 = CCLabelBMFont::create("Press Help for every key name.", "chatFont.fnt");
    hint3->setScale(.55f);
    hint3->setColor({ 170, 180, 200 });
    m_mainLayer->addChildAtPosition(hint3, Anchor::Center, { 120.f, -76.f });

    // ---------- bottom buttons ----------
    addButton("Cancel", .7f, { -95.f, -118.f }, menu_selector(RulesPopup::onCancel), nullptr);
    addButton("Help", .7f, { 0.f, -118.f }, menu_selector(RulesPopup::onHelp), nullptr);
    addButton("Save", .7f, { 95.f, -118.f }, menu_selector(RulesPopup::onSave), nullptr);

    this->writeCurrent();
    return true;
}

void RulesPopup::readCurrent() {
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
        "Join keys with <cy>+</c> to press them together, e.g. <cg>ctrl+alt+k</c>.\n"
        "<cy>Modifiers:</c> ctrl, shift, alt, cmd\n"
        "<cy>Letters/digits:</c> a-z, 0-9\n"
        "<cy>Special:</c> space (or jump), enter, tab, esc, backspace, delete, insert, home, end, pageup, pagedown, capslock\n"
        "<cy>Arrows:</c> up, down, left, right\n"
        "<cy>Function:</c> f1 - f12\n"
        "<cy>Numpad:</c> num0 - num9\n"
        "<cy>Symbols:</c> , . / ; ' [ ] \\ - = `\n\n"
        "<cy>Hold</c> = how long the keys stay down (0 = quick tap).\n"
        "<cy>Repeat</c> = press again every X% after the first press.\n"
        "<cy>When I die</c> = also press when you die, or when the attempt restarts.",
        "OK"
    )->show();
}
