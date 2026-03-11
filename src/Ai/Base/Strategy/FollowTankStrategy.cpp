/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "FollowTankStrategy.h"
#include "Playerbots.h"

NextAction** FollowTankStrategy::getDefaultActions()
{
    return NextAction::array(0, new NextAction("follow", 1.0f), nullptr);
}

void FollowTankStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
}
