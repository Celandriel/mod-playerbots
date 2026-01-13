#ifndef PLAYERBOT_GUILDRPGOUTDOORPVP_H
#define PLAYERBOT_GUILDRPGOUTDOORPVP_H

#include "NewRpgBaseAction.h"

class GuildRpgOutdoorPvpAction : public NewRpgBaseAction
{
public:
    GuildRpgOutdoorPvpAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "guild rpg outdoor pvp") {}

    // The main entry point called by the engine
    virtual bool Execute(Event event) override;

protected:
    // Find the best objective (Tower, Bunker, Flag) based on proximity and status
    ObjectGuid SelectBestObjective();

};

#endif
