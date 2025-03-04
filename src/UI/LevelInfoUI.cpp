#include "GlobalNamespace/SinglePlayerLevelSelectionFlowCoordinator.hpp"
#include "GlobalNamespace/LevelCollectionNavigationController.hpp"
#include "GlobalNamespace/LevelSelectionNavigationController.hpp"
#include "GlobalNamespace/StandardLevelDetailViewController.hpp"
#include "GlobalNamespace/StandardLevelDetailView.hpp"
#include "GlobalNamespace/IPreviewBeatmapLevel.hpp"
#include "GlobalNamespace/IDifficultyBeatmap.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSO.hpp"
#include "GlobalNamespace/IDifficultyBeatmapSet.hpp"
#include "GlobalNamespace/BeatmapDifficulty.hpp"
#include "GlobalNamespace/IBeatmapLevel.hpp"
#include "GlobalNamespace/BeatmapCharacteristicSegmentedControlController.hpp"
#include "GlobalNamespace/LevelParamsPanel.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "beatsaber-hook/shared/utils/logging.hpp"
#include "beatsaber-hook/shared/config/rapidjson-utils.hpp"

#include "include/Assets/Sprites.hpp"
#include "include/UI/LevelInfoUI.hpp"
#include "include/Utils/WebUtils.hpp"
#include "include/Utils/StringUtils.hpp"
#include "include/Enhancers/MapEnhancer.hpp"
#include "main.hpp"

#include "TMPro/TMP_Text.hpp"
#include "TMPro/TextMeshProUGUI.hpp"

#include "questui/shared/QuestUI.hpp"
#include "questui/shared/ArrayUtil.hpp"
#include "questui/shared/BeatSaberUI.hpp"
#include "questui/shared/CustomTypes/Components/MainThreadScheduler.hpp"

#include "bs-utils/shared/utils.hpp"

#include <map>
#include <string>
#include <regex>

using namespace GlobalNamespace;
using namespace std;
using namespace QuestUI;
using namespace BeatSaberUI;

namespace LevelInfoUI {
    TMPro::TextMeshProUGUI* starsLabel = NULL;
    TMPro::TextMeshProUGUI* ppLabel = NULL;

    HMUI::ImageView* starsImage = NULL;
    HMUI::ImageView* ppImage = NULL;

    TMPro::TextMeshProUGUI* noSubmissionLabel = NULL;

    static map<string, float> _mapInfos;
    static string selectedMap;

    MAKE_HOOK_MATCH(LevelRefreshContent, &StandardLevelDetailView::RefreshContent, void, StandardLevelDetailView* self) {
        LevelRefreshContent(self);

        if (starsLabel == NULL) {
            starsLabel = CreateText(self->levelParamsPanel->get_transform(), "0.00", true, UnityEngine::Vector2(-27, 6), UnityEngine::Vector2(8, 4));
            starsLabel->set_color(UnityEngine::Color(0.651,0.651,0.651, 1));
            starsLabel->set_fontStyle(TMPro::FontStyles::Italic);
            AddHoverHint(starsLabel, "BeatLeader ranked stars");

            starsImage = CreateImage(self->levelParamsPanel->get_transform(), Sprites::get_StarIcon(), UnityEngine::Vector2(-33, 5.6), UnityEngine::Vector2(3, 3));

            ppLabel = CreateText(self->levelParamsPanel->get_transform(), "0", true, UnityEngine::Vector2(-9, 6),  UnityEngine::Vector2(8, 4));
            ppLabel->set_color(UnityEngine::Color(0.651,0.651,0.651, 1));
            ppLabel->set_fontStyle(TMPro::FontStyles::Italic);
            AddHoverHint(ppLabel, "BeatLeader pp for 100%");
            
            ppImage = CreateImage(self->levelParamsPanel->get_transform(), Sprites::get_GraphIcon(), UnityEngine::Vector2(-15.5, 5.6), UnityEngine::Vector2(3, 3));

            noSubmissionLabel = CreateText(self->levelParamsPanel->get_transform(), "Score submission disabled", true, UnityEngine::Vector2(24, 3));
            noSubmissionLabel->set_color(UnityEngine::Color(1.0, 0.0, 0.0, 1));
        }

        string hash = regex_replace((string)reinterpret_cast<IPreviewBeatmapLevel*>(self->level)->get_levelID(), basic_regex("custom_level_"), "");
        string difficulty = MapEnhancer::DiffName(self->selectedDifficultyBeatmap->get_difficulty().value);
        string mode = (string)self->beatmapCharacteristicSegmentedControlController->selectedBeatmapCharacteristic->serializedName;

        string key = hash + difficulty + mode;

        if (_mapInfos.count(key)) {
            starsLabel->SetText(to_string_wprecision(_mapInfos[key], 2));
            ppLabel->SetText(to_string_wprecision(_mapInfos[key] * 44.0f, 2));
        } else {
            string url = WebUtils::API_URL + "map/hash/" + hash;

            WebUtils::GetJSONAsync(url, [difficulty, mode, key, hash](long status, bool error, rapidjson::Document& result){
                auto difficulties = result["difficulties"].GetArray();

                for (int index = 0; index < (int)difficulties.Size(); ++index)
                {
                    auto value = difficulties[index].GetObject();
                    _mapInfos[hash + value["difficultyName"].GetString() + value["modeName"].GetString()] = value["stars"].GetFloat();
                }

                float stars = _mapInfos[key];

                QuestUI::MainThreadScheduler::Schedule([stars] () {
                    starsLabel->SetText(to_string_wprecision(stars, 2));
                    ppLabel->SetText(to_string_wprecision(stars * 44.0f, 2));
                });
            });
        }

        noSubmissionLabel->get_gameObject()->SetActive(!bs_utils::Submission::getEnabled());
    }

    void setup() {
        LoggerContextObject logger = getLogger().WithContext("load");

        INSTALL_HOOK(logger, LevelRefreshContent);
    }

    void resetStars() {
        _mapInfos = {};
    }

    void reset() {
        starsLabel = NULL;
    }
}