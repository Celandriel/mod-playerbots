/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "BotUtils.h"

#include "Item.h"
#include "ItemTemplate.h"
#include "Opcodes.h"
#include "Player.h"
#include "SharedDefines.h"
#include "WorldPacket.h"

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

void BotUtils::BuildChatPacket(WorldPacket& data, ChatMsg msgtype, std::string_view message, Language language, uint8 chatTag,
    ObjectGuid const& senderGuid, std::string_view senderName,
    ObjectGuid const& targetGuid, std::string_view targetName,
    std::string_view channelName, uint32 achievementId)
{
    bool const isGM = (chatTag & CHAT_TAG_GM) != 0;
    bool isAchievement = false;

    data.Initialize((isGM && language != LANG_ADDON) ? SMSG_GM_MESSAGECHAT : SMSG_MESSAGECHAT);
    data << uint8(msgtype);
    data << uint32(language);
    data << ObjectGuid(senderGuid);
    data << uint32(0);                                              // 2.1.0

    switch (msgtype)
    {
        case CHAT_MSG_MONSTER_SAY:
        case CHAT_MSG_MONSTER_PARTY:
        case CHAT_MSG_MONSTER_YELL:
        case CHAT_MSG_MONSTER_WHISPER:
        case CHAT_MSG_MONSTER_EMOTE:
        case CHAT_MSG_RAID_BOSS_WHISPER:
        case CHAT_MSG_RAID_BOSS_EMOTE:
        case CHAT_MSG_BATTLENET:
        case CHAT_MSG_WHISPER_FOREIGN:
            data << uint32(senderName.size() + 1);
            data << senderName;
            data << ObjectGuid(targetGuid);                         // Unit Target
            if (targetGuid && !targetGuid.IsPlayer() && !targetGuid.IsPet() && (msgtype != CHAT_MSG_WHISPER_FOREIGN))
            {
                data << uint32(targetName.size() + 1);              // target name length
                data << targetName;                                 // target name
            }
            break;
        case CHAT_MSG_BG_SYSTEM_NEUTRAL:
        case CHAT_MSG_BG_SYSTEM_ALLIANCE:
        case CHAT_MSG_BG_SYSTEM_HORDE:
            data << ObjectGuid(targetGuid);                         // Unit Target
            if (targetGuid && !targetGuid.IsPlayer())
            {
                data << uint32(targetName.size() + 1);              // target name length
                data << targetName;                                 // target name
            }
            break;
        case CHAT_MSG_ACHIEVEMENT:
        case CHAT_MSG_GUILD_ACHIEVEMENT:
            data << ObjectGuid(targetGuid);                         // Unit Target
            isAchievement = true;
            break;
        default:
            if (isGM)
            {
                data << uint32(senderName.size() + 1);
                data << senderName;
            }

            if (msgtype == CHAT_MSG_CHANNEL)
            {
                data << channelName;
            }
            data << ObjectGuid(targetGuid);
            break;
    }
    data << uint32(message.size() + 1);
    data << message;
    data << uint8(chatTag);

    if (isAchievement)
        data << uint32(achievementId);
}
