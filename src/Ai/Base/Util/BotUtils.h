/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#pragma once

#include "Define.h"
#include "ObjectGuid.h"
#include "SharedDefines.h"

#include <string_view>

class Player;
class WorldPacket;
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

    // All-in-one chat packet builder (relocated from core ChatHandler). chatTag
    // is a uint8 so callers can pass a PlayerChatTag without this header needing it.
    static void BuildChatPacket(WorldPacket& data, ChatMsg msgtype, std::string_view message,
        Language language = LANG_UNIVERSAL, uint8 chatTag = 0,
        ObjectGuid const& senderGuid = ObjectGuid(), std::string_view senderName = {},
        ObjectGuid const& targetGuid = ObjectGuid(), std::string_view targetName = {},
        std::string_view channelName = {}, uint32 achievementId = 0);
};
