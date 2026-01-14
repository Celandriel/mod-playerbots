#include "GuildRpgOutdoorPvP.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"

bool GuildRpgOutdoorPvpAction::Execute(Event event)
{
    outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvPToZoneId(bot->GetMapId());
    if (!outdoorPvP)
        return false;
    GameObject* objective = SelectBestObjective();
    if (!objective)
        return true; // No valid objectives, possibly all captured

    float radius = objective->GetGOInfo()->capturePoint.radius;
    if (!objective->IsWithinDistInMap(bot, radius) || !bot->IsOutdoorPvPActive())
        {
            botAI->rpgInfo.SetMoveFarTo(WorldPosition(objective));
            return true;
        }
    else
    {
        botAI->rpgInfo.ChangeToGoGrind(WorldPosition(objective));
        return true;
    }
    return false;
}

GameObject* GuildRpgOutdoorPvpAction::SelectBestObjective()
{
    bool isAlliance = bot->GetFaction() == TEAM_ALLIANCE;
    OutdoorPvP::OPvPCapturePointMap* capturePoints = outdoorPvP->GetCapturePoints();
    std::vector<std::pair<GameObject*, float>> candidateObjectives;
    for (auto const& [guid, point] : *capturePoints)
    {
        float slider = point->GetSlider();
        GameObject* obj = point->_capturePoint;
        if (isAlliance && slider < 0.0f)
            candidateObjectives.push_back({obj, bot->GetDistance2d(obj->GetPositionX(), obj->GetPositionY())});
        else if (!isAlliance && slider > 0.0f)
            candidateObjectives.push_back({obj, bot->GetDistance2d(obj->GetPositionX(), obj->GetPositionY())});
    }
    if (candidateObjectives.empty())
        {
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            return nullptr;
        }
    GameObject* objective =
        std::min_element(candidateObjectives.begin(), candidateObjectives.end(),
                         [](const auto& a, const auto& b) { return a.second < b.second; })
            ->first;
    return objective;
}