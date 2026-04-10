/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LeaveGroupAction.h"

#include "Event.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"

bool LeaveGroupAction::Execute(Event event)
{
    Player* player = event.getOwner();
    if (player == botAI->GetOwner())
        return Leave();

    return false;
}

bool PartyCommandAction::Execute(Event event)
{
    WorldPacket& p = event.getPacket();
    p.rpos(0);
    uint32 operation;
    std::string member;

    p >> operation >> member;

    if (operation != PARTY_OP_LEAVE)
        return false;
    // Only leave if master/owner has left the party, and randombot cannot set new master.
    Player* owner = botAI->GetOwner();
    if (owner && member == owner->GetName())
    {
        if (sRandomPlayerbotMgr.IsRandomBot(bot))
        {
            Player* newMaster = botAI->FindNewMaster();
            if (newMaster || bot->InBattleground())
            {
                botAI->SetMaster(newMaster);
                return false;
            }
        }
        return Leave();
    }
    return false;
}

bool UninviteAction::Execute(Event event)
{
    WorldPacket& p = event.getPacket();
    if (p.GetOpcode() == CMSG_GROUP_UNINVITE)
    {
        p.rpos(0);
        std::string memberName;
        p >> memberName;

        // player not found
        if (!normalizePlayerName(memberName))
        {
            return false;
        }

        if (bot->GetName() == memberName)
            return Leave();
    }

    if (p.GetOpcode() == CMSG_GROUP_UNINVITE_GUID)
    {
        p.rpos(0);
        ObjectGuid guid;
        p >> guid;

        if (bot->GetGUID() == guid)
            return Leave();
    }

    return false;
}

bool LeaveGroupAction::Leave()
{
    if (!botAI)
        return false;

    Player* master = botAI->GetMaster();
    if (master)
        botAI->TellMaster("Goodbye!", PLAYERBOT_SECURITY_TALK);

    botAI->LeaveOrDisbandGroup();
    return true;
}

bool LeaveFarAwayAction::Execute(Event /*event*/)
{
    // allow bot to leave party when they want
    return Leave();
}

bool LeaveFarAwayAction::isUseful()
{
    if (bot->InBattleground() || bot->InBattlegroundQueue())
        return false;

    if (!bot->GetGroup())
        return false;

    // Don't leave if bot has a real player master or owner
    if (botAI->HasRealPlayerMaster() || botAI->IsAlt())
        return false;

    Player* groupLeader = botAI->GetGroupLeader();
    if (!groupLeader || (bot == groupLeader && !botAI->IsRealPlayer()))
        return false;

    // Don't leave if group leader is a real player
    PlayerbotAI* groupLeaderBotAI = GET_PLAYERBOT_AI(groupLeader);
    if (!groupLeaderBotAI)
        return false;

    if (botAI->GetGrouperType() == GrouperType::SOLO)
        return true;

    uint32 dCount = AI_VALUE(uint32, "death count");

    if (dCount > 9)
        return true;

    if (dCount > 4)
        return true;

    if (bot->GetGuildId() == groupLeader->GetGuildId())
    {
        if (bot->GetLevel() > groupLeader->GetLevel() + 5)
        {
            if (AI_VALUE(bool, "should get money"))
                return false;
        }
    }

    if (abs(int32(groupLeader->GetLevel() - bot->GetLevel())) > 4)
        return true;

    if (bot->GetMapId() != groupLeader->GetMapId() ||
        bot->GetDistance2d(groupLeader) >= 2 * sPlayerbotAIConfig.rpgDistance)
        return true;

    return false;
}
