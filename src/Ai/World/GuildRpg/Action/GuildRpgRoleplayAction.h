#ifndef _PLAYERBOT_GUILDRPGROLEPLAYACTION_H
#define _PLAYERBOT_GUILDRPGROLEPLAYACTION_H

#include "GuildRpgBaseAction.h"
#include "GuildRpgInfo.h"
#include "PlayerbotAI.h"

class GuildRpgRoleplayAction : public GuildRpgBaseAction
{
public:
    GuildRpgRoleplayAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg roleplay action") {}
};

#endif
