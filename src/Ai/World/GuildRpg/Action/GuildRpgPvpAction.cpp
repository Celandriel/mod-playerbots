#include "AiObjectContext.h"
#include "BattleGroundJoinAction.h"
#include "BattlegroundMgr.h"
#include "DBCEnums.h"
#include "TravelMgr.h"
#include "GuildRpgInfo.h"
#include "GuildRpgPvpAction.h"
#include "Log.h"
#include "NewRpgOutdoorPvP.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "Value.h"

std::vector<BattlegroundTypeId> FilterBattlegroundsByInstanceCounts(const std::vector<BattlegroundTypeId>& possibleBGs, uint8 botLevel)
{
    std::vector<BattlegroundTypeId> filteredBGs;
    for (BattlegroundTypeId bgTypeId : possibleBGs)
    {
        BattlegroundQueueTypeId queueTypeId = sBattlegroundMgr->BGQueueTypeId(bgTypeId, 0);
        Battleground* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
        if (!bgTemplate)
            continue;

        uint32 mapId = bgTemplate->GetMapId();

        PvPDifficultyEntry const* pvpDiff = GetBattlegroundBracketByLevel(mapId, botLevel);
        if (!pvpDiff)
            continue;

        BattlegroundBracketId bracketId = pvpDiff->GetBracketId();
        uint32 currentInstances = sRandomPlayerbotMgr.BattlegroundData[queueTypeId][bracketId].bgInstanceCount;

        uint32 maxAllowedInstances = 0;
        switch (bgTypeId)
        {
            case BATTLEGROUND_AV: maxAllowedInstances = sPlayerbotAIConfig.randomBotAutoJoinBGAVCount; break;
            case BATTLEGROUND_AB: maxAllowedInstances = sPlayerbotAIConfig.randomBotAutoJoinBGABCount; break;
            case BATTLEGROUND_WS: maxAllowedInstances = sPlayerbotAIConfig.randomBotAutoJoinBGWSCount; break;
            case BATTLEGROUND_EY: maxAllowedInstances = sPlayerbotAIConfig.randomBotAutoJoinBGEYCount; break;
            case BATTLEGROUND_IC: maxAllowedInstances = sPlayerbotAIConfig.randomBotAutoJoinBGICCount; break;
            default: maxAllowedInstances = 1; break;
        }

        if (currentInstances <= maxAllowedInstances)
            filteredBGs.push_back(bgTypeId);
    }
    return filteredBGs;
}

BattlegroundTypeId SelectBattlegroundForLevel(uint8 botLevel)
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
    possibleBGs = FilterBattlegroundsByInstanceCounts(possibleBGs, botLevel);
    if (possibleBGs.empty())
        return BATTLEGROUND_TYPE_NONE;

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
            LOG_DEBUG("playerbots", "[Guild RPG] Bot {} could not find suitable battleground for level {} resetting task", name, botLevel);
            botAI->guildRpgInfo.ResetGuildActivity(true);
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
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false; // No world PVP area available for bot level
        }
    }
    else
        return false;

    TargetGroupComposition groupComp = CreatePvpGroupComposition(groupSize, botLevel);
    if (!botAI->SetTargetGroupComposition(groupComp))
    {
        LOG_DEBUG("playerbots", "[Guild RPG] Bot {} could not set target group composition for activity {} resetting task", bot->GetName(), static_cast<int>(activity));
        botAI->guildRpgInfo.ResetGuildActivity(true);
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
        LOG_TRACE("playerbots", "[GUILD RPG] Bot {} is queuing for battleground", bot->GetName());
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
        }
        if (mapId == MAPID_INVALID || !targetZone || !toNode)
        {
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }
        TaxiNodesEntry const* taxiNodeEntry = sTaxiNodesStore.LookupEntry(static_cast<uint32>(toNode));
        if (!taxiNodeEntry)
        {
            botAI->guildRpgInfo.ResetGuildActivity(true);
            return false;
        }
        WorldPosition targetPos(mapId, taxiNodeEntry->x, taxiNodeEntry->y, taxiNodeEntry->z);
        return PreparationMovementToRpgLocation(event, targetPos, targetZone);
    }
    botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
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
        if (botAI->rpgInfo.GetStatus() != RPG_OUTDOOR_PVP)
        {
            NewRpgOutdoorPvpAction* outdoorPvpAction = new NewRpgOutdoorPvpAction(botAI);
            OPvPCapturePoint* capturePoint = outdoorPvpAction->SelectNewObjective();
            if (!capturePoint)
            {
                botAI->guildRpgInfo.ResetGuildActivity(true);
                return false;
            }
            botAI->rpgInfo.ChangeToOutdoorPvp(capturePoint);
            return true;
        }
        if (botAI->rpgInfo.HasStatusPersisted(statusOutdoorPvpDuration))
        {
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
            SyncGuildRpgStatus();
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
    botAI->guildRpgInfo.ResetGuildActivity();
    botAI->rpgInfo.ChangeToIdle();
    EndGuildRpgActivity();
    botAI->LeaveOrDisbandGroup();
    return true;
}
