/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "GuildRpgStrategy.h"
#include "Playerbots.h"


GuildRpgStrategy::GuildRpgStrategy(PlayerbotAI* botAI) : Strategy(botAI) {}

NextAction** GuildRpgStrategy::getDefaultActions()
{
    return NextAction::array(0,
        new NextAction("guild rpg status update", 12.0f),
        nullptr);
}

void GuildRpgStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(new TriggerNode("guild rpg pvp task", NextAction::array(0, new NextAction("guild rpg pvp action", 5.0f), nullptr)));
    triggers.push_back(new TriggerNode("guild rpg pve task", NextAction::array(0, new NextAction("guild rpg pve action", 5.0f), nullptr)));
    triggers.push_back(new TriggerNode("guild rpg prof task", NextAction::array(0, new NextAction("guild rpg prof action", 5.0f), nullptr)));
    triggers.push_back(new TriggerNode("guild rpg roleplay task", NextAction::array(0, new NextAction("guild rpg roleplay action", 5.0f), nullptr)));
}

void GuildRpgStrategy::InitMultipliers(std::vector<Multiplier*>& multipliers)
{
}
