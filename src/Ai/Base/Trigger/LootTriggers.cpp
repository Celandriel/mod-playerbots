/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LootTriggers.h"

#include "LootObjectStack.h"
#include "Playerbots.h"
#include "ServerFacade.h"

bool LootAvailableTrigger::IsActive()
{
    // Strategy is non-combat-only — the engine state separation is the
    // safety net. Don't gate on hostiles-in-sight: that locked out
    // looting in zones with continuous respawns (e.g. cave farms).
    // If a new enemy aggros mid-loot the combat engine takes over, loot
    // resumes on the next non-combat window.
    if (!AI_VALUE(bool, "has available loot"))
        return false;

    // "stay" strategy is restrictive: only loot if corpse is at our feet.
    if (botAI->HasStrategy("stay", BOT_STATE_NON_COMBAT))
        return ServerFacade::instance().IsDistanceLessOrEqualThan(
            AI_VALUE2(float, "distance", "loot target"), CONTACT_DISTANCE);

    return true;
}

bool FarFromCurrentLootTrigger::IsActive()
{
    LootObject loot = AI_VALUE(LootObject, "loot target");
    if (!loot.IsLootPossible(bot))
        return false;

    return AI_VALUE2(float, "distance", "loot target") >= INTERACTION_DISTANCE - 2.0f;
}

bool CanLootTrigger::IsActive() { return AI_VALUE(bool, "can loot"); }
