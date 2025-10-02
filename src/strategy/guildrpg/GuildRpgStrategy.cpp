/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU GPL v2 license, you may redistribute it
 * and/or modify it under version 2 of the License, or (at your option), any later version.
 */

#include "GuildRpgStrategy.h"
#include "Playerbots.h"

GuildRpgStrategy::GuildRpgStrategy::getDefaultActions()
{
    return NextAction::array(0,
        new NextAction("guild queue for bg", 12.0f),
        nullptr)
}

void GuildRpgStrategy::InitTriggers(std::vector<TriggerNode*>& triggers)
{
    triggers.push_back(
        new TriggerNode("no guild task", NextAction::array(0, new NextAction("guild rpg set objective", 1.0f), nullptr))
    );
}
