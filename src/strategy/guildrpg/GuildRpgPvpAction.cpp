#include "GuildRpgPvpAction.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"
#include "BattleGroundJoinAction.h"
#include "BattlegroundMgr.h"
#include "DBCEnums.h"
#include "Random.h"
#include "Log.h"
#include "GuildRpgInfo.h"
#include "FlightMasterCache.h"
#include "AiObjectContext.h"
#include "Value.h"
#include "NewRpgOutdoorPvP.h"

BattlegroundTypeId  SelectBattlegroundForLevel(uint8 botLevel)
{
    // Available battlegrounds for different level ranges
    //TODO: Align this to core's available battlegrounds and requirements from DBC etc.
    std::vector<BattlegroundTypeId > possibleBGs;

    if (botLevel >= 10)
        possibleBGs.push_back(BATTLEGROUND_WS);
    if (botLevel >= 20)
        possibleBGs.push_back(BATTLEGROUND_AB);
    if (botLevel >= 51)
        possibleBGs.push_back(BATTLEGROUND_AV);
    if (botLevel >= 61)
        possibleBGs.push_back(BATTLEGROUND_EY);
    if (botLevel >= 71)
    {
//        possibleBGs.push_back(BATTLEGROUND_SA);
        possibleBGs.push_back(BATTLEGROUND_IC);
    }
    if (possibleBGs.empty())
        return BATTLEGROUND_TYPE_NONE;

    // Randomly select from available BGs
    return possibleBGs[urand(0, possibleBGs.size() - 1)];
}

// Helper function to create PVP group composition based on requested group size
TargetGroupComposition CreatePvpGroupComposition(uint8 groupSize, uint8 botLevel)
{
    TargetGroupComposition composition = {};

    if (groupSize == 0)
        return composition;
    composition.groupSize = groupSize;
    // Set level range based on bot bracket
    composition.lowerLevelLimit = (botLevel > 5) ? botLevel - 5 : 1;
    composition.upperLevelLimit = botLevel + 5;

    if (botLevel >= 80)
    {
        composition.lowerLevelLimit = 80;
        composition.upperLevelLimit = 80;
    }
    else
    {
        composition.lowerLevelLimit = (botLevel / 10) * 10;    // e.g. 47 → 40
        composition.upperLevelLimit = composition.lowerLevelLimit + 9;              // e.g. 40 → 49
    }

    // Set composition requirements based on BG type and group size
    if (composition.groupSize == 10)
    {
        // 10-man BG composition
        composition.tanks = 2;
        composition.minHealers = 2;
        composition.maxHealers = 3;
        composition.minDps = 5;
        composition.maxDps = 6;
    }
    else if (composition.groupSize == 15)
    {
        // 15-man BG composition
        composition.tanks = 3;
        composition.minHealers = 3;
        composition.maxHealers = 4;
        composition.minDps = 7;
        composition.maxDps = 9;
    }
    else if (composition.groupSize == 40)
    {
        // 40-man BG composition
        composition.tanks = 5;
        composition.minHealers = 8;
        composition.maxHealers = 10;
        composition.minDps = 25;
        composition.maxDps = 27;
    }

    composition.allowPartial = true;
    return composition;
}

bool GuildRpgPvpAction::HandleSelection(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;
    uint8 botLevel = bot->GetLevel();
    uint8 groupSize = 0;
    if (activity == GuildRpgActivity::BATTLEGROUND)
    {
        BattlegroundTypeId  battleground = SelectBattlegroundForLevel(botLevel);
        if (battleground == BATTLEGROUND_TYPE_NONE)
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not find suitable battleground for level {} resetting task", name, botLevel);
            botAI->guildRpgInfo.ResetGuildActivity();
            return false;
        }
        botAI->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(BattlegroundMgr::BGQueueTypeId(battleground, 0));
        groupSize = sBattlegroundMgr->GetBattlegroundTemplate(battleground)->GetMaxPlayersPerTeam();
    }
    else if (activity == GuildRpgActivity::WORLD_PVP)
    {
        if (botLevel <= 60 && botLevel > 54)
        {
            botAI->guildRpgInfo.activityTarget = "EPL";
            groupSize = 40;
        }
        else
        {
            botAI->guildRpgInfo.ResetGuildActivity();
            return false; // No world PVP area available for bot level
        }
    }
    else
        return false;

    TargetGroupComposition groupComp = CreatePvpGroupComposition(groupSize, botLevel);
    if (!botAI->SetTargetGroupComposition(groupComp))
    {
        LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not set target group composition for activity {} resetting task", bot->GetName(), static_cast<int>(activity));
        botAI->guildRpgInfo.ResetGuildActivity();
        return false;
    }
    botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::GROUPING);
    SyncGuildRpgStatus();
    return true;
}

bool GuildRpgPvpAction::HandlePreparation(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;

    if (activity == GuildRpgActivity::BATTLEGROUND)
    {
        if (bot->InBattleground())
        {
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::EXECUTING);
            SyncGuildRpgStatus();
            return true;
        }
        //check if in queue
        //If not queue as group

        uint32 queueType = botAI->GetAiObjectContext()->GetValue<uint32>("bg type")->Get();
        BattlegroundQueueTypeId queueTypeId = BattlegroundQueueTypeId(queueType);
        if (bot->InBattlegroundQueueForBattlegroundQueueType(queueTypeId))
            return false;
        LOG_ERROR("playerbots", "[GUILD RPG] Bot {} is queuing for battleground", bot->GetName());
        BGJoinAction bgJoinAction(botAI);
        return bgJoinAction.Execute(event);
    }
    else if (activity == GuildRpgActivity::WORLD_PVP)
    {
        uint8 faction = bot->GetTeamId();
        uint32 targetZone, toNode;
        uint32 mapId = MAPID_INVALID;
        // Go to Lights Hope Chapel in EPL
        if (botAI->guildRpgInfo.activityTarget == "EPL")
        {
            targetZone = AREA_EASTERN_PLAGUELANDS;
            mapId = MAP_EASTERN_KINGDOMS;
            toNode = faction == TEAM_HORDE
                            ? static_cast<uint32>(FlightMasterNodes::LIGHTS_HOPE_CHAPEL_HORDE)
                            : static_cast<uint32>(FlightMasterNodes::LIGHTS_HOPE_CHAPEL_ALLIANCE);
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} with faction {} has set target zone {}, map {}, node {} for world PVP", bot->GetName(), faction, targetZone, mapId, toNode);
        }
        if (mapId == MAPID_INVALID || !targetZone || !toNode)
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} has invalid world PVP target {}, resetting task", bot->GetName(), botAI->guildRpgInfo.activityTarget);
            botAI->guildRpgInfo.ResetGuildActivity();
            return false;
        }
        return PreparationMovementToRpgLocation(event, mapId, targetZone, toNode);
    }
    LOG_ERROR("playerbots", "[Guild RPG] Bot {} has invalid activity {} resetting task", bot->GetName(), static_cast<int>(activity));
    return false;
}


bool GuildRpgPvpAction::HandleExecution(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;
    if (activity == GuildRpgActivity::BATTLEGROUND)
    {
        //Execute stage is set in previous step after entering BG, therefore, we only check once we leave the BG to be "complete"
        if (bot->InBattleground())
            return true;

        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
        SyncGuildRpgStatus();
        return true;
    }
    else if (activity == GuildRpgActivity::WORLD_PVP)
    {
        if (botAI->rpgInfo.status != RPG_OUTDOOR_PVP)
        {
            NewRpgOutdoorPvpAction* outdoorPvpAction = new NewRpgOutdoorPvpAction(botAI);
            OPvPCapturePoint* capturePoint = outdoorPvpAction->SelectNewObjective();
            if (!capturePoint)
            {
                botAI->guildRpgInfo.ResetGuildActivity();
                return false;
            }
            botAI->rpgInfo.ChangeToOutdoorPvp(capturePoint);
            return true;
        }
        return false;
    }
    return false;
}

bool GuildRpgPvpAction::HandleCompletion(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;
    if (activity == GuildRpgActivity::BATTLEGROUND)
    {
        //Not in queue or bg, decide to queue again or abandon task
        bool queueAgain = urand(0, 1); // 50% chance to queue again
        if (queueAgain)
        {
            LOG_INFO("playerbots", "[Guild RPG] Bot {} BG completed, queuing again", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
            SyncGuildRpgStatus();
            return true;
        }
    }
    LOG_INFO("playerbots", "[Guild RPG] Bot {} BG completed, disbanding", bot->GetName());
    botAI->guildRpgInfo.SetGuildRpgActivity(botAI, GuildRpgActivity::NONE);
    botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::IDLE);
    EndGuildRpgActivity();
    botAI->LeaveOrDisbandGroup();
    return true;
}
