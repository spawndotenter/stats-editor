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

        auto applyBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("Apply"), this, menu_selector(StatsPopup::onApply)
        );
        m_buttonMenu->addChildAtPosition(applyBtn, Anchor::Bottom, {-70.f, 30.f});

        auto achBtn = CCMenuItemSpriteExtra::create(
            ButtonSprite::create("All Achievements"), this, menu_selector(StatsPopup::onAchievements)
        );
        m_buttonMenu->addChildAtPosition(achBtn, Anchor::Bottom, {60.f, 30.f});

        return true;
    }

    void onApply(CCObject*) {
        auto gsm = GameStatsManager::sharedState();
        for (size_t i = 0; i < m_inputs.size(); i++) {
            auto v = utils::numFromString<int>(m_inputs[i]->getString());
            if (v.isOk()) gsm->setStat(FIELDS[i].key, v.unwrap());
        }
        AppDelegate::get()->trySaveGame(false);
        Notification::create("Stats applied", NotificationIcon::Success)->show();
    }

    void onAchievements(CCObject*) {
        auto dict = AchievementManager::sharedState()->m_allAchievements;
        if (!dict) return;
        for (auto [key, value] : CCDictionaryExt<std::string, CCObject*>(dict)) {
            GameManager::sharedState()->reportAchievementWithID(key.c_str(), 100, false);
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
        if (auto p = StatsPopup::create()) p->show();
    }
};
