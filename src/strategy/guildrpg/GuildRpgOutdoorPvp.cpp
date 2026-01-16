#include "GuildRpgOutdoorPvP.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"

bool GuildRpgOutdoorPvpAction::Execute(Event event)
{
    //TODO: Check travel RPG position for node status, and then stay if captured, if not look for more.
    LOG_ERROR("playerbots", "[Guild RPG] Bot {} is executing GuildRpgOutdoorPvpAction", bot->GetName());
    GetCapturePoints();
    if (!outdoorPvP)
        return true;
    GameObject* rpgObjective = botAI->guildRpgInfo.rpgObjective;
    if (rpgObjective)
    {
        OPvPCapturePoint* point = outdoorPvP->GetCapturePoint(rpgObjective->GetGUID().GetCounter());
        if (point)
        {
            float threshold = point->GetMinValue();
            float slider = point->GetSlider();
            uint8 faction = bot->GetTeamId();
            if ((faction == TEAM_HORDE && slider >= -threshold) ||
                (faction == TEAM_ALLIANCE && slider <= threshold))
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} found current RPG objective not captured", bot->GetName());
                objective = botAI->guildRpgInfo.rpgObjective;
            }
        }
    }
    if (!objective)
        SelectNewObjective();

    if (!objective)
        return true; // No valid objectives, possibly all captured
    LOG_ERROR("playerbots", "[Guild RPG] Bot {} has found objective for outdoor PVP", bot->GetName());
    if (objective->GetGoType() != GAMEOBJECT_TYPE_CAPTURE_POINT)
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} found invalid objective type", bot->GetName());
        return true;
    }
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

void GuildRpgOutdoorPvpAction::SelectNewObjective()
{
    objective = nullptr;
    uint8 faction = bot->GetTeamId();
    std::vector<GameObject*> candidateObjectives;

    if (!capturePoints)
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} has no outdoor PVP capture points available", bot->GetName());
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
        return;
    }
    for (auto const& [guid, point] : *capturePoints)
    {
        GameObject* capturePointObject = point->_capturePoint;
        if (!capturePointObject)
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} found invalid outdoor PVP objective", bot->GetName());
            continue;
        }
        float threshold = point->GetMinValue();
        float slider = point->GetSlider();
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} with faction {} is evaluating outdoor PVP point {} with name {} threshold {} and slider value {}", bot->GetName(), faction, guid, capturePointObject->GetName(), threshold, slider);
        if (faction == TEAM_HORDE)
        {
            if (slider > -threshold)
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} adding outdoor PVP point {} as candidate objective for Horde", bot->GetName(), guid);
                candidateObjectives.push_back(capturePointObject);
            }
        }
        else
        {
            if (slider < threshold)
            {
                LOG_ERROR("playerbots", "[Guild RPG] Bot {} adding outdoor PVP point {} as candidate objective for Alliance", bot->GetName(), guid);
                candidateObjectives.push_back(capturePointObject);
            }
        }
    }
    if (candidateObjectives.empty())
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} found no valid outdoor PVP objectives to capture", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            return;
        }
    int randomIndex = urand(0, candidateObjectives.size() - 1);
    objective = candidateObjectives[randomIndex];
}

void GuildRpgOutdoorPvpAction::GetCapturePoints()
{
    outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvPToZoneId(bot->GetZoneId());
    if (!outdoorPvP)
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} is not in an outdoor PVP zone, cannot execute GuildRpgOutdoorPvpAction", bot->GetName());
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
        return;
    }
    capturePoints = outdoorPvP->GetCapturePoints();
}
