#include "GuildRpgInfo.h"
#include "Log.h"
#include "PlayerbotAI.h"
// Global guild statistics storage

void GuildRpgInfo::SetGuildRpgActivity(PlayerbotAI* botAI, uint8 activity)
{
    GuildType guildType = botAI->GetGuildType();

    auto typeIt = Activities.find(guildType);
    if (typeIt == Activities.end())
    {
        LOG_ERROR("playerbots", "No activities defined for GuildType: {}", static_cast<int>(guildType));
        return;
    }
    auto& objMap = typeIt->second;
    auto actIt = objMap.find(activity);
    if (actIt != objMap.end())
    {
        ResetGuildActivity();
        this->activityNumber = activity;
        LOG_INFO("playerbots", "Setting guild RPG objective to {}", actIt->second);
    }
    else
    {
        LOG_ERROR("playerbots", "Invalid guild RPG objective: {} for GuildType: {}", activity, static_cast<int>(guildType));
    }
}

void GuildRpgInfo::ResetGuildActivity()
{
    activityNumber = 0;
    phase = GuildRpgPhase::IDLE;
}

std::string GuildRpgInfo::GetGuildTypeName(GuildType type)
{
    switch (type)
    {
        case GuildType::NONE:        return "NONE";
        case GuildType::PVP:         return "PVP";
        case GuildType::PVE:         return "PVE";
        case GuildType::PROFESSION:  return "PROFESSION";
        case GuildType::ROLEPLAY:    return "ROLEPLAY";
        default:                     return "UNKNOWN_TYPE";
    }
}

std::string GuildRpgInfo::GetPhaseName(GuildRpgPhase phase)
{
    switch (phase)
    {
        case GuildRpgPhase::IDLE:        return "IDLE";
        case GuildRpgPhase::SELECTION:   return "SELECTION";
        case GuildRpgPhase::GROUPING:    return "GROUPING";
        case GuildRpgPhase::PREPARATION: return "PREPARATION";
        case GuildRpgPhase::EXECUTING:   return "EXECUTING";
        case GuildRpgPhase::COMPLETED:   return "COMPLETED";
        default:                         return "UNKNOWN_PHASE";
    }
}

std::string GuildRpgInfo::ToString() const
{
    std::stringstream out;
    out << "Status  ";
    out << "Guild Type: " << GetGuildTypeName(type) << ", "
        << "Selected Activity: " << GetActivityName() << ", "
        << "Phase " << GetPhaseName(phase);
    return out.str();
}

std::string GuildRpgInfo::GetActivityName() const
{
    auto typeIt = Activities.find(type);
    if (typeIt != Activities.end())
    {
        auto objIt = typeIt->second.find(activityNumber);
        if (objIt != typeIt->second.end())
            return objIt->second;
    }
    return "IDLE";
}

/*
// GuildTaskInfo implementation
void GuildTaskInfo::ChangePhase(GuildTaskPhase newPhase, GuildRpgStatistic* stats)
{
    uint32 currentTime = time(nullptr);

    // Update time spent in previous phase
    if (stats && phaseStartT > 0)
    {
        uint32 timeSpent = currentTime - phaseStartT;
        switch (phase)
        {
            case GuildTaskPhase::IDLE:
                stats->timeSpentIdle += timeSpent;
                break;
            case GuildTaskPhase::GROUPING:
                stats->timeSpentGrouping += timeSpent;
                break;
            case GuildTaskPhase::PREPARATION:
                // Add preparation time tracking to stats if needed
                break;
            case GuildTaskPhase::EXECUTING:
                stats->timeSpentExecuting += timeSpent;
                break;
            default:
                break;
        }
    }

    // Update phase and start time
    phase = newPhase;
    phaseStartT = currentTime;
}

void GuildTaskInfo::Reset()
{
    task = GuildTask();
    phase = GuildTaskPhase::IDLE;
    startT = 0;
    phaseStartT = 0;
}

// Task lifecycle functions
void AssignGuildTask(GuildTaskInfo& info, const GuildTask& newTask, GuildRpgStatistic* stats)
{
    info.task = newTask;
    info.startT = time(nullptr);
    info.phaseStartT = info.startT;
    info.phase = GuildTaskPhase::GROUPING;

    if (stats)
    {
        stats->tasksAssigned++;
        LOG_INFO("playerbots", "Assigned guild task: type={}, objectiveId={}, priority={}",
                 static_cast<int>(newTask.type), newTask.objectiveId, newTask.priority);
    }
}

void ExpireGuildTask(GuildTaskInfo& info, GuildRpgStatistic* stats)
{
    if (stats)
    {
        stats->tasksExpired++;
        LOG_INFO("playerbots", "Expired guild task: type={}, objectiveId={}",
                 static_cast<int>(info.task.type), info.task.objectiveId);
    }

    info.ChangePhase(GuildTaskPhase::FAILED, stats);
}

void AbandonGuildTask(GuildTaskInfo& info, GuildRpgStatistic* stats)
{
    if (stats)
    {
        stats->tasksAbandoned++;
        LOG_INFO("playerbots", "Abandoned guild task: type={}, objectiveId={}",
                 static_cast<int>(info.task.type), info.task.objectiveId);
    }

    info.ChangePhase(GuildTaskPhase::FAILED, stats);
}

void CompleteGuildTask(GuildTaskInfo& info, bool success, GuildRpgStatistic* stats)
{
    if (stats)
    {
        if (success)
        {
            stats->tasksCompleted++;
            LOG_INFO("playerbots", "Completed guild task: type={}, objectiveId={}",
                     static_cast<int>(info.task.type), info.task.objectiveId);
        }
        else
        {
            stats->tasksFailed++;
            LOG_INFO("playerbots", "Failed guild task: type={}, objectiveId={}",
                     static_cast<int>(info.task.type), info.task.objectiveId);
        }
    }

    info.ChangePhase(success ? GuildTaskPhase::COMPLETED : GuildTaskPhase::FAILED, stats);
}

// PVP Task Execution Implementation
namespace GuildTasksPvp
{
    bool ExecutePvpTask(const GuildTask& task, class PlayerbotAI* botAI)
    {
        if (!botAI || !botAI->guildRpgInfo)
            return false;

        GuildTaskInfo* taskInfo = botAI->guildRpgInfo;
        GuildTaskPhase currentPhase = taskInfo->phase;
        Player* bot = botAI->GetBot();

        switch (task.objectiveId)
        {
            case PvpActivity::QUEUE_FOR_BG:
            {
                if (currentPhase == GuildTaskPhase::PREPARATION)
                {
                    // Handle BG queuing during PREPARATION phase
                    if (bot->InBattleground() || bot->InBattlegroundQueue())
                    {
                        // Already in queue or BG, transition to EXECUTING
                        LOG_INFO("playerbots", "Bot {} already in BG queue/BG, transitioning to EXECUTING", bot->GetName().c_str());
                        botAI->UpdateGuildTaskPhase(GuildTaskPhase::EXECUTING);
                        return true;
                    }

                    // Try to queue for BG
                    BattlegroundQueueTypeId bgType = (BattlegroundQueueTypeId)botAI->GetAiObjectContext()->GetValue<uint32>("bg type")->Get();
                    if (!bgType)
                    {
                        // Select BG type if not set
                        uint8 botLevel = bot->GetLevel();
                        bgType = SelectBattlegroundForLevel(botLevel);
                        if (bgType != BATTLEGROUND_QUEUE_NONE)
                        {
                            botAI->GetAiObjectContext()->GetValue<uint32>("bg type")->Set(bgType);
                        }
                    }

                    if (bgType != BATTLEGROUND_QUEUE_NONE)
                    {
                        // Use BGJoinAction to handle queuing
                        Action* bgJoinAction = botAI->GetAiObjectContext()->GetAction("bg join");
                        if (bgJoinAction && bgJoinAction->Execute(Event()))
                        {
                            LOG_INFO("playerbots", "Bot {} successfully queued for BG", bot->GetName().c_str());
                            return true;
                        }
                    }

                    LOG_INFO("playerbots", "Bot {} failed to queue for BG", bot->GetName().c_str());
                    return false;
                }
                else if (currentPhase == GuildTaskPhase::EXECUTING)
                {
                    // Handle BG execution
                    if (!bot->InBattleground())
                    {
                        // Check if we left the BG
                        if (botAI->GetAiObjectContext()->GetValue<uint32>("bg type")->Get() == 0)
                        {
                            // BG completed, decide next action
                            bool queueAgain = urand(0, 1); // 50% chance to queue again
                            if (queueAgain)
                            {
                                LOG_INFO("playerbots", "Bot {} BG completed, queuing again", bot->GetName().c_str());
                                botAI->UpdateGuildTaskPhase(GuildTaskPhase::PREPARATION);
                                return true;
                            }
                            else
                            {
                                LOG_INFO("playerbots", "Bot {} BG completed, disbanding", bot->GetName().c_str());
                                botAI->CompleteCurrentGuildTask(true);
                                return true;
                            }
                        }
                        return true; // Still waiting for BG
                    }

                    // Bot is in BG, continue participating
                    return true;
                }
                break;
            }
            case PvpActivity::PATROL_AREA:
            {
                if (currentPhase == GuildTaskPhase::EXECUTING)
                {
                    // Implement patrol logic
                    LOG_INFO("playerbots", "PatrolAreaAction execution not yet implemented");
                    return false;
                }
                break;
            }
            case PvpActivity::ATTACK_CITY:
            {
                if (currentPhase == GuildTaskPhase::EXECUTING)
                {
                    // Implement attack city logic
                    LOG_INFO("playerbots", "AttackCityAction execution not yet implemented");
                    return false;
                }
                break;
            }
            case PvpActivity::DEFEND_BASE:
            {
                if (currentPhase == GuildTaskPhase::EXECUTING)
                {
                    // Implement defend base logic
                    LOG_INFO("playerbots", "DefendBaseAction execution not yet implemented");
                    return false;
                }
                break;
            }
            case PvpActivity::WORLD_PVP:
            {
                if (currentPhase == GuildTaskPhase::EXECUTING)
                {
                    // Implement world PvP logic
                    LOG_INFO("playerbots", "WorldPvpAction execution not yet implemented");
                    return false;
                }
                break;
            }
            default:
                LOG_ERROR("playerbots", "Unknown PVP objective ID: {}", task.objectiveId);
                return false;
        }

        return true;
    }
}

// Statistic helper functions
void UpdateGuildStats(ActivityType type, GuildRpgStatistic* stats)
{
    if (stats)
    {
        guildStats[type] += *stats;
    }
}

void UpdateGuildStatsByObjectiveId(uint32 objectiveId, GuildRpgStatistic* stats)
{
    if (stats)
    {
        guildStatsByObjectiveId[objectiveId] += *stats;
    }
}

GuildRpgStatistic GetTotalGuildStats()
{
    GuildRpgStatistic total;
    for (const auto& pair : guildStats)
    {
        total += pair.second;
    }
    return total;
}

void ResetGuildStats()
{
    guildStats.clear();
    guildStatsByObjectiveId.clear();
}
    */