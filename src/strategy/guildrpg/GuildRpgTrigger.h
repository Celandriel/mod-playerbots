#ifndef _PlaYERBOT_GUILDRPGTRIGGER_H
#define _PLAYERBOT_GUILDRPGTRIGGER_H

#include "GuildRpgStrategy.h"
#include "Trigger.h"
#include "GuildRpgInfo.h"

class GuildRpgPhaseTriggger : public Trigger
{
public:
    GuildRpgPhaseTriggger(PlayerbotAI* botAI, GuildRpgPhase phase = GuildRpgPhase::IDLE)
        : Trigger(botAI, "guild rpg status"), phase(phase) 
    { 
    }
    bool IsActive() override;

protected:
    GuildRpgPhase phase;
};

#endif
