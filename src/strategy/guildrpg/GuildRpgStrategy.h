/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#ifndef _PLAYERBOT_GUILDRPGSTRATEGY_H_
#define _PLAYERBOT_GUILDRPGSTRATEGY_H_

#include "Strategy.h"
#include "GuildRpgInfo.h"

class PlayerbotAI;

class GuildRpgStrategy : public Strategy
{
public:
    GuildRpgStrategy(PlayerbotAI* botAI);

    std::string const getName() override { return "guild rpg"; }
    NextAction** getDefaultActions() override;
    void InitTriggers(std::vector<TriggerNode*>& triggers) override;
};

#endif
