

#ifndef _PLAYERBOTS_GUILDRPGACTION_H
#define _PLAYERBOTS_GUILDRPGACTION_H

#include "GuildRpgBaseAction.h"
#include "PlayerbotAI.h"
#include "NewRpgAction.h"

class TellGuildRpgStatusAction public Action
{
public:
    TellGuildRpgStatusAction(PlayerbotAI* botAI, "guild rpg status") {}
    bool Execute(Event event) override;
}

class GuildRpgStatusUpdateAction : public GuildRpgBaseAction
{
public:
    GuildRpgStatusUpdateAction(PlayerbotAI* botAI, "guild rpg status update") {}
    bool Execute(Event event) override;
}

class ExecuteGuildTaskAction : public GuildRpgBaseAction
{
public:
    ExecuteGuildTaskAction(PlayerbotAI* botAI, "guild rpg do task") {}

    virtual bool Execute(Event event);
    virtual bool isPossible();
    virtual bool isUseful();
}