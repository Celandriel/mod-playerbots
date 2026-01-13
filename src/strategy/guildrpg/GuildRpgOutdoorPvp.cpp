#include "GuildRpgOutdoorPvP.h"
#include "OutdoorPvPMgr.h"

bool GuildRpgOutdoorPvpAction::Execute(Event event)
{
    ObjectGuid objectiveGuid = SelectBestObjective();
    if (!objectiveGuid)
        return true; // No valid objectives, possibly all captured

    OPvPCapturePoint* capturePoint = sOutdoorPvPMgr->GetCapturePoint(objectiveGuid);
    if (!capturePoint)
        return false;

    auto radius = (float)capturePoint->GetGOInfo()->capturePoint.radius;
    if (!capturePoint->IsWithinDistInMap(bot, radius) || !bot->IsOutdoorPvPActive())
        {
            botAI->rpgInfo.SetMoveFarTo(capturePoint->GetPosition());
            return true;
        }
    else
        return botAI->rpgInfo.ChangeToGoGrind(capturePoint->GetPosition());
    return false;
}

ObjectGuid GuildRpgOutdoorPvpAction::SelectBestObjective()
{
    uint32 mapId = bot->GetMapId();
    if (mapId == MAPID_INVALID)
        return ObjectGuid();

    OutdoorPvP* outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvP(mapId);
    if (!outdoorPvP)
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not find Outdoor PvP for map {}", bot->GetName(), mapId);
        return ObjectGuid();
    }
    bool isAlliance = bot->GetFaction() == TEAM_ALLIANCE;
    OPvPCapturePointMap = outdoorPvP->GetCapturePoints();
    std::vector<std::pair<ObjectGuid, float>> candidateObjectives;
    for (auto const& [guid, point] : OPvPCapturePointMap)
    {
        float slider = point->GetSlider()
        if (isAlliance && slider < 0.0f)
            candidateObjectives.push_back({guid, bot->GetDistance2d(point->GetPositionX(), point->GetPositionY())});
        else if (!isAlliance && slider > 0.0f)
            candidateObjectives.push_back({guid, bot->GetDistance2d(point->GetPositionX(), point->GetPositionY())});
    }
    if (candidateObjectives.empty())
        {
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            SyncGuildRpgStatus();
            return ObjectGuid();
        }
    return std::min_element(candidateObjectives.begin(), candidateObjectives.end(),
                            [](const auto& a, const auto& b) { return a.second < b.second; })
        ->first;
}