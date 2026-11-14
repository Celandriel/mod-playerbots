/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_GOTRAVELVALUE_H
#define PLAYERBOTS_GOTRAVELVALUE_H

#include "Player.h"
#include "TravelMgr.h"
#include "Value.h"

class PlayerbotAI;

constexpr float GO_TRAVEL_ARRIVAL_DISTANCE = 10.0f;

// A trip started by the `go travel <name>` chat command.
struct GoTravelInfo
{
    WorldPosition destination;
    std::string name;
    bool active = false;
    bool restoreFollow = false;

    bool HasArrived(Player* bot) const
    {
        return bot->GetMapId() == destination.GetMapId() &&
               bot->GetExactDist(destination.GetPositionX(), destination.GetPositionY(), destination.GetPositionZ()) <=
                   GO_TRAVEL_ARRIVAL_DISTANCE;
    }
};

class GoTravelValue : public ManualSetValue<GoTravelInfo&>
{
public:
    GoTravelValue(PlayerbotAI* botAI) : ManualSetValue<GoTravelInfo&>(botAI, data, "go travel") {}

private:
    GoTravelInfo data;
};

#endif
