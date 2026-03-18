#include "NewRpgOutdoorPvP.h"
#include "OutdoorPvP.h"
#include "OutdoorPvPMgr.h"

bool NewRpgOutdoorPvpAction::Execute(Event event)
{
    NewRpgInfo& info = botAI->rpgInfo;
    if (botAI->guildRpgInfo.GetActivityName() != "WORLD_PVP")
    {
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
        botAI->rpgInfo.ChangeToIdle();
        return false;
    }
    GetCapturePoints();
    OPvPCapturePoint* objective = nullptr;
    if (!this->outdoorPvP)
    {
        LOG_DEBUG("playerbots","[New RPG] bot {} does not have Outdoor PVP object.", bot->GetName());
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
        return true;
    }
    auto& data = std::get<NewRpgInfo::OutdoorPvP>(info.data);

    OPvPCapturePoint* capturePoint = data.capturePoint;
    if (capturePoint)
    {
        if (!capturePoint->_capturePoint)
            data.capturePoint = nullptr;

        else
        {
            float threshold = (capturePoint->GetMinValue() + capturePoint->GetMaxValue())/2;
            float slider = capturePoint->GetSlider();
            uint8 faction = bot->GetTeamId();
            LOG_DEBUG("playerbots", "[NEW RPG] Bot {} with faction {} is evaluating existing RPG objective {} with threshold {} and slider value {}", bot->GetName(), faction, capturePoint->_capturePoint->GetName(), threshold, slider);
            if ((faction == TEAM_HORDE && slider >= -threshold) ||
                (faction == TEAM_ALLIANCE && slider <= threshold))
                objective = capturePoint;
        }
    }

    if (!objective)
    {
        objective = SelectNewObjective();
        if (!objective)
        {
            LOG_DEBUG("playerbots","[New RPG] bot {} does not have Outdoor PVP objective.", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            return true; // No valid objectives, possibly all captured
        }
        data.capturePoint = objective;
    }
    GameObject* objectiveGO = objective->_capturePoint;
    if (!objectiveGO)
    {
        LOG_DEBUG("playerbots","[New RPG] bot {} target capture point does not have a game object.", bot->GetName());
        return false;
    }
    if (objectiveGO->GetGoType() != GAMEOBJECT_TYPE_CAPTURE_POINT)
    {
        LOG_DEBUG("playerbots","[New RPG] bot {} Found capture point object is not of type Capture_point.", bot->GetName());
        return false;
    }

    float radius = objectiveGO->GetGOInfo()->capturePoint.radius / 2.0f;
    float dist = objectiveGO->GetDistance(bot);
    bool pvpActive = bot->IsOutdoorPvPActive();
    LOG_DEBUG("playerbots", "[NEW RPG] Bot {} outdoor PVP: objective={}, dist={}, radius={}, pvpActive={}", bot->GetName(), objectiveGO->GetName(), dist, radius, pvpActive);

    if (!objectiveGO->IsWithinDistInMap(bot, radius) || !pvpActive)
    {
        LOG_DEBUG("playerbots", "[NEW RPG] Bot {} moving to capture point (outOfRange={}, pvpInactive={})", bot->GetName(), !objectiveGO->IsWithinDistInMap(bot, radius), !pvpActive);
        return MoveFarTo(WorldPosition(objectiveGO));
    }

    // Within capture range - patrol the area while capturing
    LOG_DEBUG("playerbots", "[NEW RPG] Bot {} in capture range, calling PatrolCapturePoint", bot->GetName());
    return PatrolCapturePoint(objectiveGO, radius);
}

OPvPCapturePoint* NewRpgOutdoorPvpAction::SelectNewObjective()
{
    OPvPCapturePoint* objective = nullptr;
    uint8 faction = bot->GetTeamId();
    std::vector<OPvPCapturePoint*> candidateObjectives;
    if (!this->outdoorPvP)
        GetCapturePoints();

    if (!this->capturePointMap)
    {
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
        return objective;
    }
    for (auto const& [guid, point] : *capturePointMap)
    {
        GameObject* capturePointObject = point->_capturePoint;
        if (!capturePointObject)
            continue;

        float threshold = point->GetMinValue();
        float slider = point->GetSlider();
        if (faction == TEAM_HORDE)
        {
            if (slider > -threshold)
            candidateObjectives.push_back(point);
        }
        else
        {
            if (slider < threshold)
                candidateObjectives.push_back(point);
        }
    }
    if (candidateObjectives.empty())
        {
            LOG_DEBUG("playerbots", "[New RPG] Bot {} found no valid outdoor PVP objectives to capture", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            return objective;
        }
    int randomIndex = urand(0, candidateObjectives.size() - 1);
    objective = candidateObjectives[randomIndex];
    return objective;
}

void NewRpgOutdoorPvpAction::GetCapturePoints()
{
    outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvPToZoneId(bot->GetZoneId());
    if (!outdoorPvP)
    {
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
        return;
    }
    capturePointMap = outdoorPvP->GetCapturePoints();
}

bool NewRpgOutdoorPvpAction::PatrolCapturePoint(GameObject* objectiveGO, float radius)
{
    if (IsWaitingForLastMove(MovementPriority::MOVEMENT_NORMAL))
    {
        LOG_DEBUG("playerbots", "[NEW RPG] Bot {} PatrolCapturePoint: waiting for last move", bot->GetName());
        return false;
    }

    // Randomly pause at the current spot before picking a new patrol point
    if (urand(0, 1) == 0)
    {
        LOG_DEBUG("playerbots", "[NEW RPG] Bot {} PatrolCapturePoint: random pause", bot->GetName());
        return ForceToWait(urand(3000, 6000));
    }

    float patrolRadius = radius * 0.8f;
    LOG_DEBUG("playerbots", "[NEW RPG] Bot {} PatrolCapturePoint: attempting MoveRandomNear with patrolRadius={}", bot->GetName(), patrolRadius);
    if (MoveRandomNear(patrolRadius, MovementPriority::MOVEMENT_NORMAL, objectiveGO))
        return true;

    LOG_DEBUG("playerbots", "[NEW RPG] Bot {} PatrolCapturePoint: MoveRandomNear failed, forcing wait", bot->GetName());
    return ForceToWait(urand(3000, 6000));
}
