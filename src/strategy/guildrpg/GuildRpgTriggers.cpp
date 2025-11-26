#include "GuildRpgTriggers.h"
#include "PlayerbotAI.h"


bool GuildRpgTaskTrigger::IsActive()
{
    GuildRpgInfo guildRpgInfo= botAI->guildRpgInfo;
    return (!(guildRpgInfo.phase == GuildRpgPhase::IDLE) &&
            guildRpgInfo.type == type);
}
