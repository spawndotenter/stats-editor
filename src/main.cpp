#include <Geode/Geode.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

struct StatField {
    const char* label;
    const char* key;
};

static const StatField FIELDS[] = {
    {"Stars", "6"},
    {"Moons", "28"},
    {"Demons", "5"},
    {"Secret Coins", "8"},
    {"User Coins", "12"},
    {"Diamonds", "13"},
};

class StatsPopup : public Popup {
protected:
    std::vector<TextInput*> m_inputs;

    bool init() {
        if (!Popup::init(300.f, 280.f)) return false;
        this->setTitle("Edit Stats");

        auto size = m_mainLayer->getContentSize();
        float y = size.height - 60.f;

        for (auto const& f : FIELDS) {
            auto label = CCLabelBMFont::create(f.label, "bigFont.fnt");
            label->setScale(0.45f);
            label->setAnchorPoint({0.f, 0.5f});
            label->setPosition({25.f, y});
            m_mainLayer->addChild(label);

            auto input = TextInput::create(130.f, "0", "bigFont.fnt");
            input->setCommonFilter(CommonFilter::Int);
            input->setString(std::to_string(GameStatsManager::sharedState()->getStat(f.key)));
            input->setPosition({size.width - 95.f, y});
            m_mainLayer->addChild(input);
            m_inputs.push_back(input);

            y -= 32.f;
        }

        auto applySpr = ButtonSprite::create("Apply");
        applySpr->setScale(0.7f);
        auto applyBtn = CCMenuItemSpriteExtra::create(
            applySpr, this, menu_selector(StatsPopup::onApply)
        );
        m_buttonMenu->addChildAtPosition(applyBtn, Anchor::Bottom, {-65.f, 28.f});

        auto achSpr = ButtonSprite::create("All Achievements");
        achSpr->setScale(0.55f);
        auto achBtn = CCMenuItemSpriteExtra::create(
            achSpr, this, menu_selector(StatsPopup::onAchievements)
        );
        m_buttonMenu->addChildAtPosition(achBtn, Anchor::Bottom, {65.f, 28.f});

        return true;
    }

    void onApply(CCObject*) {
        auto gsm = GameStatsManager::sharedState();
        for (size_t i = 0; i < m_inputs.size(); i++) {
            auto v = utils::numFromString<int>(m_inputs[i]->getString());
            if (v.isOk()) gsm->setStat(FIELDS[i].key, v.unwrap());
        }
        AppDelegate::get()->trySaveGame(false);
        GameLevelManager::sharedState()->updateUserScore();
        Notification::create("Stats applied and sent", NotificationIcon::Success)->show();
    }

    void onAchievements(CCObject*) {
        auto all = AchievementManager::sharedState()->m_allAchievements;
        if (!all) return;
        for (auto obj : CCArrayExt<CCObject*>(all)) {
            std::string id;
            if (auto s = typeinfo_cast<CCString*>(obj)) {
                id = s->getCString();
            } else if (auto d = typeinfo_cast<CCDictionary*>(obj)) {
                if (auto v = d->valueForKey("identifier")) id = v->getCString();
            }
            if (!id.empty()) {
                GameManager::sharedState()->reportAchievementWithID(id.c_str(), 100, false);
            }
        }
        AppDelegate::get()->trySaveGame(false);
        Notification::create("Achievements unlocked", NotificationIcon::Success)->show();
    }

public:
    static StatsPopup* create() {
        auto ret = new StatsPopup();
        if (ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class WarningPopup : public Popup {
protected:
    std::function<void()> m_onContinue;
    CCLabelBMFont* m_waitLabel = nullptr;
    CCMenuItemSpriteExtra* m_continueBtn = nullptr;
    int m_secondsLeft = 3;

    bool init(std::function<void()> onContinue) {
        if (!Popup::init(320.f, 210.f)) return false;
        m_onContinue = std::move(onContinue);
        this->setTitle("Warning");

        auto size = m_mainLayer->getContentSize();
        const char* lines[] = {
            "This sends your edited stats",
            "to the GD servers. You will",
            "probably get leaderboard banned."
        };
        float y = size.height - 70.f;
        for (auto line : lines) {
            auto l = CCLabelBMFont::create(line, "bigFont.fnt");
            l->setScale(0.45f);
            l->setPosition({size.width / 2.f, y});
            m_mainLayer->addChild(l);
            y -= 24.f;
        }

        auto contSpr = ButtonSprite::create("Continue");
        contSpr->setScale(0.8f);
        m_continueBtn = CCMenuItemSpriteExtra::create(
            contSpr, this, menu_selector(WarningPopup::onContinue)
        );
        m_continueBtn->setVisible(false);
        m_continueBtn->setEnabled(false);
        m_buttonMenu->addChildAtPosition(m_continueBtn, Anchor::Bottom, {0.f, 35.f});

        m_waitLabel = CCLabelBMFont::create("Please wait 3...", "bigFont.fnt");
        m_waitLabel->setScale(0.5f);
        m_mainLayer->addChildAtPosition(m_waitLabel, Anchor::Bottom, {0.f, 35.f});

        this->schedule(schedule_selector(WarningPopup::tick), 1.f);
        return true;
    }

    void tick(float) {
        m_secondsLeft--;
        if (m_secondsLeft > 0) {
            m_waitLabel->setString(fmt::format("Please wait {}...", m_secondsLeft).c_str());
        } else {
            this->unschedule(schedule_selector(WarningPopup::tick));
            m_waitLabel->setVisible(false);
            m_continueBtn->setVisible(true);
            m_continueBtn->setEnabled(true);
        }
    }

    void onContinue(CCObject*) {
        auto cb = m_onContinue;
        this->onClose(nullptr);
        if (cb) cb();
    }

public:
    static WarningPopup* create(std::function<void()> onContinue) {
        auto ret = new WarningPopup();
        if (ret->init(std::move(onContinue))) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }
};

class $modify(StatsMenu, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto spr = CircleButtonSprite::createWithSpriteFrameName("GJ_statsBtn_001.png");
        auto btn = CCMenuItemSpriteExtra::create(
            spr, this, menu_selector(StatsMenu::onStats)
        );
        btn->setID("stats-editor-button"_spr);

        if (auto menu = this->getChildByID("bottom-menu")) {
            menu->addChild(btn);
            menu->updateLayout();
        }
        return true;
    }

    void onStats(CCObject*) {
        if (Mod::get()->getSavedValue<bool>("warning-seen", false)) {
            if (auto p = StatsPopup::create()) p->show();
            return;
        }
        auto w = WarningPopup::create([] {
            Mod::get()->setSavedValue("warning-seen", true);
            if (auto p = StatsPopup::create()) p->show();
        });
        if (w) w->show();
    }
};
