#pragma once

#include <string>

namespace Unloved::Campaign {

    // Starts (or disables) the campaign script associated with a map.
    // Campaign files live at res/campaigns/<mapName>.json.
    void StartForMap(const std::string& mapName);
    void Stop();
    void Update();

    bool IsActive();
    bool IsComplete();

    const std::string& GetCampaignId();
    const std::string& GetTitle();
    const std::string& GetCurrentStageId();
    const std::string& GetCurrentObjective();
}
