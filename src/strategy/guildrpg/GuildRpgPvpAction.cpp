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


BattlegroundQueueTypeId SelectBattlegroundForLevel(uint8 botLevel)
{
    // Available battlegrounds for different level ranges
    std::vector<BattlegroundQueueTypeId> possibleBGs;

    if (botLevel >= 10 && botLevel <= 19)
    {
        // Level 10-19: Warsong Gulch, Arathi Basin
        possibleBGs = { BATTLEGROUND_QUEUE_WS, BATTLEGROUND_QUEUE_AB };
    }
    else if (botLevel >= 20 && botLevel <= 29)
    {
        // Level 20-29: Warsong Gulch, Arathi Basin, Alterac Valley
        possibleBGs = { BATTLEGROUND_QUEUE_WS, BATTLEGROUND_QUEUE_AB, BATTLEGROUND_QUEUE_AV };
    }
    else if (botLevel >= 30 && botLevel <= 39)
    {
        // Level 30-39: All classic BGs
        possibleBGs = { BATTLEGROUND_QUEUE_WS, BATTLEGROUND_QUEUE_AB, BATTLEGROUND_QUEUE_AV, BATTLEGROUND_QUEUE_EY };
    }
    else if (botLevel >= 40 && botLevel <= 49)
    {
        // Level 40-49: All classic + Strand of the Ancients
        possibleBGs = { BATTLEGROUND_QUEUE_WS, BATTLEGROUND_QUEUE_AB, BATTLEGROUND_QUEUE_AV, BATTLEGROUND_QUEUE_EY, BATTLEGROUND_QUEUE_SA };
    }
    else if (botLevel >= 50 && botLevel <= 59)
    {
        // Level 50-59: All BGs except Isle of Conquest
        possibleBGs = { BATTLEGROUND_QUEUE_WS, BATTLEGROUND_QUEUE_AB, BATTLEGROUND_QUEUE_AV, BATTLEGROUND_QUEUE_EY, BATTLEGROUND_QUEUE_SA };
    }
    else if (botLevel >= 60)
    {
        // Level 60+: All BGs
        possibleBGs = { BATTLEGROUND_QUEUE_WS, BATTLEGROUND_QUEUE_AB, BATTLEGROUND_QUEUE_AV, BATTLEGROUND_QUEUE_EY, BATTLEGROUND_QUEUE_SA, BATTLEGROUND_QUEUE_IC };
    }

    if (possibleBGs.empty())
        return BATTLEGROUND_QUEUE_NONE;

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
    if (composition.groupSize == 5)
    {
        // 5-man BG composition
        composition.tanks = 1;
        composition.minHealers = 1;
        composition.maxHealers = 2;
        composition.minDps = 2;
        composition.maxDps = 3;
    }
    else if (composition.groupSize == 10)
    {
        // 10-man BG composition
        composition.tanks = 2;
        composition.minHealers = 2;
        composition.maxHealers = 3;
        composition.minDps = 5;
        composition.maxDps = 6;
    }
    composition.allowPartial = true;
    return composition;
}

bool GuildRpgPvpAction::HandleSelection(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;//stop 1
    if (activity == GuildRpgActivity::QUEUE_FOR_BG)
    {//stop 2
        uint8 botLevel = bot->GetLevel();
        BattlegroundQueueTypeId battleground = SelectBattlegroundForLevel(botLevel);
        if (battleground == BATTLEGROUND_QUEUE_NONE)
        {
            LOG_ERROR("playerbots", "Bot {} could not find suitable battleground for level {} resetting task", name, botLevel);
            botAI->guildRpgInfo.ResetGuildActivity();
            return false;
        }
        botAI->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(battleground); //stop 2
        TargetGroupComposition groupComp = CreatePvpGroupComposition(battleground, botLevel); //stop 3
        if (!botAI->SetTargetGroupComposition(groupComp)) //stop 4
        {
            LOG_ERROR("playerbots", "Bot {} could not set target group composition for battleground {}, resetting task", bot->GetName(), battleground);
            botAI->guildRpgInfo.ResetGuildActivity();
            return false;
        }
        LOG_ERROR("playerbots", "[GUILD RPG] Bot {} has selected Battleground {}", bot->GetName(), battleground);
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::GROUPING);
        return true;
    }
    return false;
}

bool GuildRpgPvpAction::HandlePreparation(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;
    if (activity == GuildRpgActivity::QUEUE_FOR_BG)
    {
        //check if in queue
        //If not queue as group
        if (bot->InBattlegroundQueue())
        {
            return false;
        }
        if (bot->InBattleground())
        {
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::EXECUTING);
            return false;
        }
        BGJoinAction bgJoinAction(botAI);
        return bgJoinAction.Execute(event);
    }
    return false;
}


bool GuildRpgPvpAction::HandleExecution(Event event)
{
    GuildRpgActivity activity = botAI->guildRpgInfo.activity;
    if (activity == GuildRpgActivity::QUEUE_FOR_BG)
    {
        //check if in bg
        //If not in bg check if in queue
        if (bot->InBattleground())
        {
            return true;
        }
        botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::COMPLETED);
        return true;
    }
    return false;
}

bool GuildRpgPvpAction::HandleCompletion(Event event)
{
    GuildRpgInfo guildRpgInfo = botAI->guildRpgInfo;
    GuildRpgActivity activity = guildRpgInfo.activity;
    if (activity == GuildRpgActivity::QUEUE_FOR_BG)
    {
        //Not in queue or bg, decide to queue again or abandon task
        bool queueAgain = urand(0, 1); // 50% chance to queue again
        if (queueAgain)
        {
            LOG_INFO("playerbots", "Bot {} BG completed, queuing again", bot->GetName());
            botAI->guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::PREPARATION);
            return true;
        }
        else
        {
            LOG_INFO("playerbots", "Bot {} BG completed, disbanding", bot->GetName());
            guildRpgInfo.SetGuildRpgActivity(botAI, GuildRpgActivity::NONE);
            guildRpgInfo.SetGuildRpgPhase(GuildRpgPhase::IDLE);
            //botAI->DisbandGroup();
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