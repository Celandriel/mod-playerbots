#include "GuildRpgInfo.h"
#include "Log.h"

// Global guild statistics storage
std::map<ActivityType, GuildRpgStatistic> guildStats;
std::map<uint32, GuildRpgStatistic> guildStatsByObjectiveId;

void GuildRpgInfo::SetGuildRpgActivity(uint8 activity)
{
    GuildRpgActivityKey key = {botAi->getGuildType(), activity};
    if (Activities.count(key))
    {
        Reset();
        activity = key;
        LOG_INFO("playerbots", "Setting guild RPG objective to {}", Activities[key]);
    }
    else 
    {
        LOG_ERROR("playerbots", "Invalid guild RPG objective: {} for GuildType: {}", objective, static_cast<int>(botAi->getGuildType()));
    }
}
