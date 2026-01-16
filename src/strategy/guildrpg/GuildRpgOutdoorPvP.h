#ifndef PLAYERBOT_GUILDRPGOUTDOORPVP_H
#define PLAYERBOT_GUILDRPGOUTDOORPVP_H

#include "NewRpgBaseAction.h"
#include"OutdoorPvP.h"

class GuildRpgOutdoorPvpAction : public NewRpgBaseAction
{
public:
    GuildRpgOutdoorPvpAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "guild rpg outdoor pvp") {}

    // The main entry point called by the engine
    virtual bool Execute(Event event) override;

protected:
    void SelectNewObjective();
    void GetCapturePoints();

private:
    OutdoorPvP::OPvPCapturePointMap* capturePoints = nullptr;
    OutdoorPvP* outdoorPvP = nullptr;
    GameObject* objective = nullptr;
};

#endif
