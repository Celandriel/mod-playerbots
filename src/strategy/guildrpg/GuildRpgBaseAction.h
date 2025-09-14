#ifndef _PLAYERBOT_GUILDRPGBASEACTION_H
#define _PLAYERBOT_GUILDRPGBASEACTION_H

#include "Action.h"

class GuildRpgBaseAction : public Action
{
public:
    GuildRpgBaseAction(PlayerbotAI* botAI, const std::string& name);
    uint8 RollRandomAction(std::vector<uint32> weights);

protected:
    Guild* GetGuild() const;
};
#endif