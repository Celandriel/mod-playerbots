/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "TravelTriggers.h"
#include "GoTravelValue.h"
#include "Playerbots.h"
#include "ServerFacade.h"

bool GoTravelActiveTrigger::IsActive()
{
    GoTravelInfo const& info = AI_VALUE(GoTravelInfo&, "go travel");
    return info.active && !info.HasArrived(bot);
}

bool GoTravelFinishedTrigger::IsActive()
{
    GoTravelInfo const& info = AI_VALUE(GoTravelInfo&, "go travel");
    return !info.active || info.HasArrived(bot);
}

bool NearDarkPortalTrigger::IsActive() { return bot->GetAreaId() == 72; }

bool AtDarkPortalAzerothTrigger::IsActive()
{
    if (bot->GetAreaId() == 72)
    {
        if (ServerFacade::instance().GetDistance2d(bot, -11906.9f, -3208.53f) < 20.0f)
        {
            return true;
        }
    }

    return false;
}

bool AtDarkPortalOutlandTrigger::IsActive()
{
    if (bot->GetAreaId() == 3539)
    {
        if (ServerFacade::instance().GetDistance2d(bot, -248.1939f, 921.919f) < 10.0f)
        {
            return true;
        }
    }

    return false;
}
