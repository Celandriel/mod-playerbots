#ifndef _PLAYERBOT_GUILDRPGPVEACTION_H
#define _PLAYERBOT_GUILDRPGPVEACTION_H

#include "GuildRpgBaseAction.h"
#include "GuildRpgInfo.h"
#include "PlayerbotAI.h"

class GuildRpgPveAction : public GuildRpgBaseAction
{
public:
    GuildRpgPveAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg pve action") {}
};

#endif
