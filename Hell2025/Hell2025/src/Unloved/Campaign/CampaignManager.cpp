#include "CampaignManager.h"

#include "Hell/Audio.h"
#include "Hell/Logging.h"
#include "Hell/Serialization/Json.h"

#include "Unloved/Objects/House/Door.h"
#include "Unloved/Objects/Renderables/MeshNodes.h"
#include "Unloved/Player/Player.h"
#include "Unloved/Session/Session.h"
#include "Unloved/Systems/Openables/OpenableManager.h"
#include "Unloved/World/World.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Unloved::Campaign {
    namespace {
        struct Condition {
            std::string type = "none";
            std::string item;
            std::string flag;
            glm::vec3 min = glm::vec3(0.0f);
            glm::vec3 max = glm::vec3(0.0f);
        };

        struct Action {
            std::string type;
            std::string text;
            std::string target;
            std::string flag;
            std::string audio;
            bool value = true;
            float duration = 3.0f;
        };

        struct Stage {
            std::string id;
            std::string objective;
            Condition condition;
            std::vector<Action> onEnter;
            std::vector<Action> onComplete;
        };

        struct CampaignData {
            std::string id;
            std::string title;
            std::string map;
            std::string completionMessage = "CHAPTER COMPLETE";
            std::vector<Stage> stages;
        };

        CampaignData g_campaign;
        std::unordered_map<std::string, bool> g_flags;
        std::string g_currentObjective;
        std::string g_currentStageId;
        size_t g_stageIndex = 0;
        bool g_active = false;
        bool g_complete = false;

        std::string CampaignPathForMap(const std::string& mapName) {
            return "res/campaigns/" + mapName + ".json";
        }

        void ResetRuntimeState() {
            g_campaign = CampaignData{};
            g_flags.clear();
            g_currentObjective.clear();
            g_currentStageId.clear();
            g_stageIndex = 0;
            g_active = false;
            g_complete = false;
        }

        void DisplayMessage(const std::string& text, float duration = 3.0f) {
            if (text.empty()) return;

            for (uint64_t playerId : Session::GetLocalPlayerIds()) {
                if (Player* player = Session::GetPlayerById(playerId)) {
                    player->m_typeWriter.DisplayText(text, duration);
                }
            }
        }

        bool SetDoorLocked(const std::string& editorName, bool locked) {
            if (editorName.empty()) return false;

            bool found = false;
            for (Door& door : World::GetDoors()) {
                if (door.GetEditorName() != editorName) continue;

                found = true;
                for (const MeshNode& node : door.GetMeshNodes().GetNodes()) {
                    if (node.openableId == 0) continue;

                    if (locked) {
                        OpenableManager::LockOpenablebyId(node.openableId);
                    }
                    else {
                        OpenableManager::UnlockOpenablebyId(node.openableId);
                    }
                }
            }

            if (!found) {
                Logging::Warning() << "Campaign: door with EditorName '" << editorName << "' was not found\n";
            }

            return found;
        }

        void ExecuteAction(const Action& action) {
            if (action.type == "message") {
                DisplayMessage(action.text, action.duration);
            }
            else if (action.type == "set_flag") {
                if (!action.flag.empty()) g_flags[action.flag] = action.value;
            }
            else if (action.type == "lock_door") {
                SetDoorLocked(action.target, true);
            }
            else if (action.type == "unlock_door") {
                SetDoorLocked(action.target, false);
            }
            else if (action.type == "play_audio") {
                if (!action.audio.empty()) Hell::Audio::PlayAudio(action.audio, 1.0f);
            }
            else if (!action.type.empty()) {
                Logging::Warning() << "Campaign: unknown action type '" << action.type << "'\n";
            }
        }

        void ExecuteActions(const std::vector<Action>& actions) {
            for (const Action& action : actions) {
                ExecuteAction(action);
            }
        }

        bool AnyPlayerHasItem(const std::string& itemName) {
            if (itemName.empty()) return false;

            for (uint64_t playerId : Session::GetLocalPlayerIds()) {
                Player* player = Session::GetPlayerById(playerId);
                if (player && player->GetInventory().HasItem(itemName)) return true;
            }
            return false;
        }

        bool AnyPlayerInsideVolume(const glm::vec3& minimum, const glm::vec3& maximum) {
            const glm::vec3 minPoint = glm::min(minimum, maximum);
            const glm::vec3 maxPoint = glm::max(minimum, maximum);

            for (uint64_t playerId : Session::GetLocalPlayerIds()) {
                Player* player = Session::GetPlayerById(playerId);
                if (!player || player->IsDead()) continue;

                const glm::vec3 position = player->GetFootPosition();
                if (position.x >= minPoint.x && position.x <= maxPoint.x &&
                    position.y >= minPoint.y && position.y <= maxPoint.y &&
                    position.z >= minPoint.z && position.z <= maxPoint.z) {
                    return true;
                }
            }
            return false;
        }

        bool ConditionMet(const Condition& condition) {
            if (condition.type == "always") return true;
            if (condition.type == "inventory_has") return AnyPlayerHasItem(condition.item);
            if (condition.type == "enter_volume") return AnyPlayerInsideVolume(condition.min, condition.max);
            if (condition.type == "flag_set") {
                auto it = g_flags.find(condition.flag);
                return it != g_flags.end() && it->second;
            }
            return false;
        }

        void EnterCurrentStage() {
            if (g_stageIndex >= g_campaign.stages.size()) return;

            Stage& stage = g_campaign.stages[g_stageIndex];
            g_currentStageId = stage.id;
            g_currentObjective = stage.objective;

            ExecuteActions(stage.onEnter);

            if (!stage.objective.empty()) {
                DisplayMessage("OBJECTIVE: " + stage.objective, 4.0f);
            }

            Logging::Debug()
                << "Campaign '" << g_campaign.id << "' entered stage '" << g_currentStageId
                << "' - " << g_currentObjective << "\n";
        }

        void CompleteCampaign() {
            g_complete = true;
            g_active = false;
            g_currentStageId = "complete";
            g_currentObjective.clear();

            DisplayMessage(g_campaign.completionMessage, 6.0f);
            Logging::Debug() << "Campaign '" << g_campaign.id << "' complete\n";
        }

        void AdvanceStage() {
            if (g_stageIndex >= g_campaign.stages.size()) return;

            ExecuteActions(g_campaign.stages[g_stageIndex].onComplete);
            ++g_stageIndex;

            if (g_stageIndex >= g_campaign.stages.size()) {
                CompleteCampaign();
                return;
            }

            EnterCurrentStage();
        }

        Action ParseAction(const nlohmann::json& json) {
            Action action;
            action.type = json.value("type", "");
            action.text = json.value("text", "");
            action.target = json.value("target", "");
            action.flag = json.value("flag", "");
            action.audio = json.value("audio", "");
            action.value = json.value("value", true);
            action.duration = json.value("duration", 3.0f);
            return action;
        }

        std::vector<Action> ParseActions(const nlohmann::json& json, const char* fieldName) {
            std::vector<Action> actions;
            const auto it = json.find(fieldName);
            if (it == json.end() || !it->is_array()) return actions;

            for (const nlohmann::json& actionJson : *it) {
                if (actionJson.is_object()) actions.push_back(ParseAction(actionJson));
            }
            return actions;
        }

        Condition ParseCondition(const nlohmann::json& json) {
            Condition condition;
            if (!json.is_object()) return condition;

            condition.type = json.value("type", "none");
            condition.item = json.value("item", "");
            condition.flag = json.value("flag", "");

            const auto minIt = json.find("min");
            if (minIt != json.end() && minIt->is_array() && minIt->size() >= 3) {
                condition.min = minIt->get<glm::vec3>();
            }

            const auto maxIt = json.find("max");
            if (maxIt != json.end() && maxIt->is_array() && maxIt->size() >= 3) {
                condition.max = maxIt->get<glm::vec3>();
            }

            return condition;
        }

        bool LoadCampaign(const std::string& path, const std::string& expectedMap) {
            nlohmann::json json;
            if (!Hell::Json::LoadFromFile(json, path)) return false;

            CampaignData loaded;
            try {
                loaded.id = json.value("id", expectedMap);
                loaded.title = json.value("title", loaded.id);
                loaded.map = json.value("map", expectedMap);
                loaded.completionMessage = json.value("completionMessage", "CHAPTER COMPLETE");

                const auto stagesIt = json.find("stages");
                if (stagesIt != json.end() && stagesIt->is_array()) {
                    for (const nlohmann::json& stageJson : *stagesIt) {
                        if (!stageJson.is_object()) continue;

                        Stage stage;
                        stage.id = stageJson.value("id", "");
                        stage.objective = stageJson.value("objective", "");

                        const auto conditionIt = stageJson.find("condition");
                        if (conditionIt != stageJson.end()) {
                            stage.condition = ParseCondition(*conditionIt);
                        }

                        stage.onEnter = ParseActions(stageJson, "onEnter");
                        stage.onComplete = ParseActions(stageJson, "onComplete");
                        loaded.stages.push_back(std::move(stage));
                    }
                }
            }
            catch (const nlohmann::json::exception& e) {
                Logging::Error() << "Campaign: failed reading '" << path << "': " << e.what() << "\n";
                return false;
            }

            if (loaded.map != expectedMap) {
                Logging::Warning()
                    << "Campaign '" << loaded.id << "' targets map '" << loaded.map
                    << "' but was loaded for '" << expectedMap << "'\n";
            }

            if (loaded.stages.empty()) {
                Logging::Warning() << "Campaign '" << loaded.id << "' has no stages\n";
                return false;
            }

            g_campaign = std::move(loaded);
            return true;
        }
    }

    void StartForMap(const std::string& mapName) {
        ResetRuntimeState();

        const std::string path = CampaignPathForMap(mapName);
        if (!std::filesystem::exists(path)) {
            Logging::Debug() << "No campaign script for map '" << mapName << "'\n";
            return;
        }

        if (!LoadCampaign(path, mapName)) return;

        g_active = true;
        g_complete = false;
        g_stageIndex = 0;

        Logging::Debug()
            << "Started campaign '" << g_campaign.id << "' (" << g_campaign.title
            << ") for map '" << mapName << "'\n";

        EnterCurrentStage();
    }

    void Stop() {
        ResetRuntimeState();
    }

    void Update() {
        if (!g_active || g_complete || g_stageIndex >= g_campaign.stages.size()) return;

        if (ConditionMet(g_campaign.stages[g_stageIndex].condition)) {
            AdvanceStage();
        }
    }

    bool IsActive() {
        return g_active;
    }

    bool IsComplete() {
        return g_complete;
    }

    const std::string& GetCampaignId() {
        return g_campaign.id;
    }

    const std::string& GetTitle() {
        return g_campaign.title;
    }

    const std::string& GetCurrentStageId() {
        return g_currentStageId;
    }

    const std::string& GetCurrentObjective() {
        return g_currentObjective;
    }
}
