#include "GuildRpgOutdoorPvP.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"

bool GuildRpgOutdoorPvpAction::Execute(Event event)
{
    LOG_ERROR("playerbots", "[Guild RPG] Bot {} is executing GuildRpgOutdoorPvpAction", bot->GetName());
    outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvPToZoneId(bot->GetZoneId());
    if (!outdoorPvP)
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} is not in an outdoor PVP zone, cannot execute GuildRpgOutdoorPvpAction", bot->GetName());
        return false;
    }
    GameObject* objective = SelectBestObjective();
    if (!objective)
        return true; // No valid objectives, possibly all captured
    LOG_ERROR("playerbots", "[Guild RPG] Bot {} has found objective for outdoor PVP", bot->GetName());
    float radius = objective->GetGOInfo()->capturePoint.radius;
    if (!objective->IsWithinDistInMap(bot, radius) || !bot->IsOutdoorPvPActive())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} is moving to outdoor PVP objective", bot->GetName());
            return MoveFarTo(WorldPosition(objective));
        }
    else
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} is at outdoor PVP objective, starting capture", bot->GetName());
        botAI->rpgInfo.ChangeToGoGrind(WorldPosition(objective));
        return true;
    }
    return false;
}

GameObject* GuildRpgOutdoorPvpAction::SelectBestObjective()
{
    bool isHorde = bot->GetFaction();
    OutdoorPvP::OPvPCapturePointMap* capturePoints = outdoorPvP->GetCapturePoints();
    std::vector<GameObject*> candidateObjectives;
    for (auto const& [guid, point] : *capturePoints)
    {
        GameObject* obj = point->_capturePoint;
        if (!obj)
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} found invalid outdoor PVP objective", bot->GetName());
            continue;
        }
        float threshold = obj->GetGOInfo()->capturePoint.minTime;
        float slider = point->GetSlider();
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} with isHorde {} is evaluating outdoor PVP point {} with threshold {} and slider value {}", bot->GetName(), isHorde, guid, threshold, slider);
        if (isHorde)
        {
            if (slider > -threshold)
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} adding outdoor PVP point {} as candidate objective for Horde", bot->GetName(), guid);
                candidateObjectives.push_back(obj);
            }
        }
        else
        {
            if (slider < threshold)
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} adding outdoor PVP point {} as candidate objective for Alliance", bot->GetName(), guid);
                candidateObjectives.push_back(obj);
            }
        }
    }
    if (candidateObjectives.empty())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} found no valid outdoor PVP objectives to capture", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            return nullptr;
        }
    int randomIndex = urand(0, candidateObjectives.size() - 1);
    return candidateObjectives[randomIndex];
}