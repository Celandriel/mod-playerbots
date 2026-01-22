/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "GuildRpgStrategy.h"
#include "Playerbots.h"


GuildRpgStrategy::GuildRpgStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

std::vector<NextAction> GuildRpgStrategy::getDefaultActions()
{
    return {
            NextAction("guild rpg status update", 12.0f),
    };
}

void GuildRpgStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("guild rpg pvp task", { NextAction("guild rpg pvp action", 5.0f) }));
    triggers.push_back(new TriggerNode("guild rpg pve task", { NextAction("guild rpg pve action", 5.0f) }));
    triggers.push_back(new TriggerNode("guild rpg prof task", { NextAction("guild rpg prof action", 5.0f) }));
    triggers.push_back(new TriggerNode("guild rpg roleplay task", { NextAction("guild rpg roleplay action", 5.0f) }));
}

void GuildRpgStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
}
