#include "GuildRpgTriggers.h"
#include "PlayerbotAI.h"


bool GuildRpgTaskTrigger::IsActive()
{
    GuildRpgInfo guildRpgInfo = botAI->guildRpgInfo;
    return (!(guildRpgInfo.activity == GuildRpgActivity::NONE) &&
            guildRpgInfo.type == type);
}
