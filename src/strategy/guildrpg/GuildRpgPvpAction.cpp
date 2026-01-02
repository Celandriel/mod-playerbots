#include "GuildRpgPvpAction.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"
#include "BattleGroundJoinAction.h"
#include "BattlegroundMgr.h"
#include "DBCEnums.h"
#include "Random.h"
#include "Log.h"
#include "GuildRpgInfo.h"
#include "AiObjectContext.h"
#include "Value.h"


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

// Helper function to create PVP group composition based on battleground type
TargetGroupComposition CreatePvpGroupComposition(uint32 bgTypeId, uint8 botLevel)
{
    TargetGroupComposition composition = {};

    // Determine group size based on BG type
    switch (bgTypeId)
    {
        case BATTLEGROUND_WS:// Warsong Gulch
            composition.groupSize = 10; // 10v10 battlegrounds
            break;
        case BATTLEGROUND_SA: // Strand of the Ancients
        case BATTLEGROUND_AB: // Arathi Basin
        case BATTLEGROUND_EY: // Eye of the Storm
            composition.groupSize = 15; // 15v15 battlegrounds
            break;
        case BATTLEGROUND_AV: // Alterac Valley
        case BATTLEGROUND_IC: // Isle of Conquest
            composition.groupSize = 40; // 40v40 battlegrounds
            break;
        default:
            composition.groupSize = 5; // Default to 5
            break;
    }

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
    if (activity == GuildRpgActivity::BATTLEGROUND)
    {
        uint8 botLevel = bot->GetLevel();
        BattlegroundTypeId  battleground = SelectBattlegroundForLevel(botLevel);
        if (battleground == BATTLEGROUND_TYPE_NONE)
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not find suitable battleground for level {} resetting task", name, botLevel);
            botAI->guildRpgInfo.ResetGuildActivity();
            return false;
        }
        botAI->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(BattlegroundMgr::BGQueueTypeId(battleground, 0));
        TargetGroupComposition groupComp = CreatePvpGroupComposition(battleground, botLevel);

        if (!botAI->SetTargetGroupComposition(groupComp))
        {
            LOG_ERROR("playerbots", "[Guild RPG] Bot {} could not set target group composition for battleground {}, resetting task", bot->GetName(), battleground);
            botAI->guildRpgInfo.ResetGuildActivity();
            return false;
        }
        LOG_ERROR("playerbots", "[GUILD RPG] Bot {} has selected Battleground {}", bot->GetName(), battleground);
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::GROUPING);
        SyncGuildRpgStatus();
        return true;
    }
    return false;
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
    return false;
}


bool GuildRpgPvpAction::HandleExecution(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;
    if (activity == GuildRpgActivity::BATTLEGROUND)
    {
        //check if in bg
        //If not in bg check if in queue
        if (bot->InBattleground())
            return true;

        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
        SyncGuildRpgStatus();
        return true;
    }
    return false;
}

bool GuildRpgPvpAction::HandleCompletion(Event event)
{
    GuildRpgInfo guildRpgInfo = botAI->guildRpgInfo;
    GuildRpgActivity activity = guildRpgInfo.activity;
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
        else
        {
            LOG_INFO("playerbots", "[Guild RPG] Bot {} BG completed, disbanding", bot->GetName());
            guildRpgInfo.SetGuildRpgActivity(botAI, GuildRpgActivity::NONE);
            guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::IDLE);
            SyncGuildRpgStatus();
            botAI->LeaveOrDisbandGroup();
            return true;
        }
    }
    return false;
}

/*
bool FriendlyDuelAction::isUseful()
{
    // Check if bot has a task to duel
    GuildTaskInfo* taskInfo = botAI->guildRpgInfo;
    if (!taskInfo || taskInfo->task.type != ActivityType::PVP)
        return false;

    // Check if in a safe location for dueling
    return !botAI->GetBot()->IsInCombat() && !botAI->GetBot()->InBattleground();
}

bool FriendlyDuelAction::Execute(Event event)
{
    // TODO: Implement friendly duel logic
    LOG_INFO("playerbots", "FriendlyDuelAction not yet implemented");
    return false;
}

bool AttackCityAction::isUseful()
{
    // Check if bot has a task to attack city
    GuildTaskInfo* taskInfo = botAI->guildRpgInfo;
    if (!taskInfo || taskInfo->task.objectiveId != PvpActivity::ATTACK_CITY)
        return false;

    return !botAI->GetBot()->IsInCombat();
}

bool AttackCityAction::Execute(Event event)
{
    // TODO: Implement attack city logic
    LOG_INFO("playerbots", "AttackCityAction not yet implemented");
    return false;
}

bool RaidHighProbabilityLocationAction::isUseful()
{
    // Check if bot has a raid task
    GuildTaskInfo* taskInfo = botAI->guildRpgInfo;
    if (!taskInfo || taskInfo->task.type != ActivityType::PVP)
        return false;

    return !botAI->GetBot()->IsInCombat();
}

bool RaidHighProbabilityLocationAction::Execute(Event event)
{
    // TODO: Implement raid logic
    LOG_INFO("playerbots", "RaidHighProbabilityLocationAction not yet implemented");
    return false;
}

bool PatrolAreaAction::isUseful()
{
    // Check if bot has a patrol task
    GuildTaskInfo* taskInfo = botAI->guildRpgInfo;
    if (!taskInfo || taskInfo->task.objectiveId != PvpActivity::PATROL_AREA)
        return false;

    return !botAI->GetBot()->IsInCombat();
}

bool PatrolAreaAction::Execute(Event event)
{
    // TODO: Implement patrol logic
    LOG_INFO("playerbots", "PatrolAreaAction not yet implemented");
    return false;
}

bool DefendBaseAction::isUseful()
{
    // Check if bot has a defend task
    GuildTaskInfo* taskInfo = botAI->guildRpgInfo;
    if (!taskInfo || taskInfo->task.objectiveId != PvpActivity::DEFEND_BASE)
        return false;

    return !botAI->GetBot()->IsInCombat();
}

bool DefendBaseAction::Execute(Event event)
{
    // TODO: Implement defend base logic
    LOG_INFO("playerbots", "DefendBaseAction not yet implemented");
    return false;
}

bool WorldPvpAction::isUseful()
{
    // Check if bot has a world PvP task
    GuildTaskInfo* taskInfo = botAI->guildRpgInfo;
    if (!taskInfo || taskInfo->task.objectiveId != PvpActivity::WORLD_PVP)
        return false;

    return !botAI->GetBot()->IsInCombat();
}

bool WorldPvpAction::Execute(Event event)
{
    // TODO: Implement world PvP logic
    LOG_INFO("playerbots", "WorldPvpAction not yet implemented");
    return false;
}

*/