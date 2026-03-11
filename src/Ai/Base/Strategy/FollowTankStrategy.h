/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_FOLLOWTANKSTRATEGY_H
#define _PLAYERBOT_FOLLOWTANKSTRATEGY_H

#include "NonCombatStrategy.h"

class PlayerbotAI;

class FollowTankStrategy : public NonCombatStrategy
{
public:
    FollowTankStrategy(PlayerbotAI* botAI) : NonCombatStrategy(botAI) {}

    std::string const getName() override { return "follow tank"; }
    NextAction** getDefaultActions() override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
