/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#pragma once

#include "Define.h"

class Player;
struct ItemTemplate;
enum InventoryResult : uint8;

// Stateless helpers relocated out of the AzerothCore core so the core stays
// minimal/upstreamable. Each only uses already-public core API.
class BotUtils
{
public:
    // Bot-side item usability: reject class relics (idol/totem/libram/sigil) the
    // bot's class can't equip, otherwise defer to the core Player::CanUseItem check.
    static InventoryResult CanUseItem(Player const* bot, ItemTemplate const* proto);

    // Item type helpers (relocated from core ItemTemplate).
    static bool IsWeapon(ItemTemplate const* proto);
    static bool IsRangedWeapon(ItemTemplate const* proto);
};
