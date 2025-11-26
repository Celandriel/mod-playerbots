#ifndef _PLAYERBOT_GUILDRPGPROFACTION_H
#define _PLAYERBOT_GUILDRPGPROFACTION_H

#include "GuildRpgBaseAction.h"
#include "GuildRpgInfo.h"
#include "PlayerbotAI.h"

class GuildRpgProfAction : public GuildRpgBaseAction
{
public:
    GuildRpgProfAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg prof action") {}
};

#endif
