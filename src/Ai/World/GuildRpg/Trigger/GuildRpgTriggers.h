#ifndef _PlaYERBOT_GUILDRPGTRIGGER_H
#define _PLAYERBOT_GUILDRPGTRIGGER_H

#include "GuildRpgInfo.h"
#include "GuildRpgStrategy.h"
#include "Trigger.h"

class GuildRpgTaskTrigger : public Trigger
{
public:
    GuildRpgTaskTrigger(PlayerbotAI* botAI, GuildType type = GuildType::NONE)
        : Trigger(botAI, "guild rpg task"), type(type)
    {
    }
    bool IsActive() override;

protected:
    GuildType type;
};

#endif
