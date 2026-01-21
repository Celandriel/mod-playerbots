#ifndef PLAYERBOT_NEWRPGOUTDOORPVP_H
#define PLAYERBOT_NEWRPGOUTDOORPVP_H

#include "NewRpgBaseAction.h"
#include"OutdoorPvP.h"

class NewRpgOutdoorPvpAction : public NewRpgBaseAction
{
public:
    NewRpgOutdoorPvpAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg outdoor pvp") {}

    virtual bool Execute(Event event) override;
    void SelectNewObjective();
    OPvPCapturePoint* GetCapturePoint();

protected:
    void GetCapturePoints();

private:
    OutdoorPvP::OPvPCapturePointMap* capturePointMap = nullptr;
    OutdoorPvP* outdoorPvP = nullptr;
    GameObject* objective = nullptr;
};

#endif
