#include "NewRpgOutdoorPvP.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"

bool NewRpgOutdoorPvpAction::Execute(Event event)
{
    LOG_ERROR("playerbots", "[NEW RPG] Bot {} is executing NewRpgOutdoorPvpAction", bot->GetName());
    GetCapturePoints();
    objective = nullptr;
    if (!outdoorPvP)
    {
        LOG_ERROR("playerbots", "[NEW RPG] Bot {} has no outdoor PVP data available while in zone {}", bot->GetName(), bot->GetZoneId());
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
        return true;
    }
    OPvPCapturePoint* capturePoint = botAI->rpgInfo.outdoor_pvp.capturePoint;
    if (!capturePoint)
    {
        LOG_ERROR("playerbots", "[NEW RPG] Bot {} has no existing capture point", bot->GetName());
    }
    if (capturePoint)
    {
        if (!capturePoint->_capturePoint)
        {
            LOG_ERROR("playerbots", "[NEW RPG] Bot {} has existing capture point with no GO", bot->GetName());
            botAI->rpgInfo.outdoor_pvp.capturePoint = nullptr;
        }
        else
        {
            LOG_ERROR("playerbots", "[NEW RPG] bot {} has existing RPG objective and GO", bot->GetName());
            float threshold = capturePoint->GetMinValue();
            float slider = capturePoint->GetSlider();
            uint8 faction = bot->GetTeamId();
            LOG_ERROR("playerbots", "[NEW RPG] Bot {} with faction {} is evaluating existing RPG objective with threshold {} and slider value {}", bot->GetName(), faction, threshold, slider);
            if ((faction == TEAM_HORDE && slider >= -threshold) ||
                (faction == TEAM_ALLIANCE && slider <= threshold))
            {
                LOG_ERROR("playerbots", "[New RPG] Bot {} found current RPG objective not captured", bot->GetName());
                objective = capturePoint->_capturePoint;
            }
        }
    }

    if (!objective)
    {
        LOG_ERROR("playerbots", "[NEW RPG] Bot {} is selecting new objective for outdoor PVP", bot->GetName());
        SelectNewObjective();
        if (!objective)
        {
            LOG_ERROR("playerbots", "[New RPG] Bot {} has no valid outdoor PVP objectives, setting as completed", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            return true; // No valid objectives, possibly all captured
        }
        botAI->rpgInfo.outdoor_pvp.capturePoint = outdoorPvP->GetCapturePoint(objective->GetGUID().GetCounter());
    }
    LOG_ERROR("playerbots", "[New RPG] Bot {} has found objective for outdoor PVP", bot->GetName());
    if (objective->GetGoType() != GAMEOBJECT_TYPE_CAPTURE_POINT)
    {
        LOG_ERROR("playerbots", "[New RPG] Bot {} found invalid objective type", bot->GetName());
        return true;
    }

    float radius = objective->GetGOInfo()->capturePoint.radius;
    if (!objective->IsWithinDistInMap(bot, radius) || !bot->IsOutdoorPvPActive())
        {
            LOG_ERROR("playerbots", "[New RPG] Bot {} is moving to outdoor PVP objective", bot->GetName());
            return MoveFarTo(WorldPosition(objective));
        }
    else
    {
        LOG_ERROR("playerbots", "[New RPG] Bot {} is at outdoor PVP objective, do nothing", bot->GetName());
        return true;
    }
    return false;
}

void NewRpgOutdoorPvpAction::SelectNewObjective()
{
    objective = nullptr;
    uint8 faction = bot->GetTeamId();
    std::vector<GameObject*> candidateObjectives;
    if (!outdoorPvP)
        GetCapturePoints();

    if (!capturePointMap)
    {
        LOG_ERROR("playerbots", "[New RPG] Bot {} has no outdoor PVP capture points available", bot->GetName());
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
        return;
    }
    for (auto const& [guid, point] : *capturePointMap)
    {
        GameObject* capturePointObject = point->_capturePoint;
        if (!capturePointObject)
        {
            LOG_ERROR("playerbots", "[New RPG] Bot {} found invalid outdoor PVP objective", bot->GetName());
            continue;
        }
        float threshold = point->GetMinValue();
        float slider = point->GetSlider();
        LOG_ERROR("playerbots", "[New RPG] Bot {} with faction {} is evaluating outdoor PVP point {} with name {} threshold {} and slider value {}", bot->GetName(), faction, guid, capturePointObject->GetName(), threshold, slider);
        if (faction == TEAM_HORDE)
        {
            if (slider > -threshold)
            {
                LOG_ERROR("playerbots", "[New RPG] Bot {} adding outdoor PVP point {} as candidate objective for Horde", bot->GetName(), guid);
                candidateObjectives.push_back(capturePointObject);
            }
        }
        else
        {
            if (slider < threshold)
            {
                LOG_ERROR("playerbots", "[New RPG] Bot {} adding outdoor PVP point {} as candidate objective for Alliance", bot->GetName(), guid);
                candidateObjectives.push_back(capturePointObject);
            }
        }
    }
    if (candidateObjectives.empty())
        {
            LOG_ERROR("playerbots", "[New RPG] Bot {} found no valid outdoor PVP objectives to capture", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            return;
        }
    int randomIndex = urand(0, candidateObjectives.size() - 1);
    objective = candidateObjectives[randomIndex];
}

void NewRpgOutdoorPvpAction::GetCapturePoints()
{
    LOG_ERROR("playerbots", "[NEW RPG] Bot {} is retrieving outdoor PVP capture points", bot->GetName());
    outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvPToZoneId(bot->GetZoneId());
    if (!outdoorPvP)
    {
        LOG_ERROR("playerbots", "[New RPG] Bot {} is not in an outdoor PVP zone, cannot execute GuildRpgOutdoorPvpAction", bot->GetName());
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
        return;
    }
    capturePointMap = outdoorPvP->GetCapturePoints();
}

OPvPCapturePoint* NewRpgOutdoorPvpAction::GetCapturePoint()
{
    if (!objective)
        return nullptr;
    if (objective->GetGoType() != GAMEOBJECT_TYPE_CAPTURE_POINT)
        return nullptr;
   return outdoorPvP->GetCapturePoint(objective->GetGUID().GetCounter());
}
