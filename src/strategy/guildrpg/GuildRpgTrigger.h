#ifndef _PlaYERBOT_GUILDRPGTRIGGER_H
#define _PLAYERBOT_GUILDRPGTRIGGER_H

#include "GuildRpgStrategy.h"
#include "Trigger.h"

class GuildRpgPhaseTriggger : public Trigger
{
public:
    GuildRpgPhaseTriggger(PlayerbotAI* botAI, GuildRpgPhase = IDLE)
        : Trigger(botAI, "guild rpg status"), phase(phase) 
    { 
    }
    bool IsActive() override;

protected:
    GuildTaskPhase phase;
};