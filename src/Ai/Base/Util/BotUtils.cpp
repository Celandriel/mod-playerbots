/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "BotUtils.h"

#include "Item.h"
#include "ItemTemplate.h"
#include "Player.h"
#include "SharedDefines.h"

InventoryResult BotUtils::CanUseItem(Player const* bot, ItemTemplate const* proto)
{
    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_IDOL && !bot->IsClass(CLASS_DRUID, CLASS_CONTEXT_EQUIP_RELIC))
        return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;

    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_TOTEM && !bot->IsClass(CLASS_SHAMAN, CLASS_CONTEXT_EQUIP_RELIC))
        return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;

    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_LIBRAM && !bot->IsClass(CLASS_PALADIN, CLASS_CONTEXT_EQUIP_RELIC))
        return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;

    if (proto->Class == ITEM_CLASS_ARMOR && proto->SubClass == ITEM_SUBCLASS_ARMOR_SIGIL && !bot->IsClass(CLASS_DEATH_KNIGHT, CLASS_CONTEXT_EQUIP_RELIC))
        return EQUIP_ERR_YOU_CAN_NEVER_USE_THAT_ITEM;

    return bot->CanUseItem(proto);
}

bool BotUtils::IsWeapon(ItemTemplate const* proto)
{
    return proto->Class == ITEM_CLASS_WEAPON;
}

bool BotUtils::IsRangedWeapon(ItemTemplate const* proto)
{
    return IsWeapon(proto) &&
        (proto->InventoryType == INVTYPE_RANGED || proto->InventoryType == INVTYPE_THROWN || proto->InventoryType == INVTYPE_RANGEDRIGHT);
}
