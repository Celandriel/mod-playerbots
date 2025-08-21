#ifndef _PLAYERBOT_GUILDRPGBASEACTION_H
#define _PLAYERBOT_GUILDRPGBASEACTION_H

#include "Action.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"


class PlayerbotGroupMgr;

class GuildRPGBaseAction : public Action
{
public:
    GuildRPGBaseAction(PlayerbotAI* botAI, const std::string& name) : Action(botAI, name), m_groupMgr(nullptr) {}
    virtual ~GuildRPGBaseAction();

protected:
    PlayerbotGroupMgr* GetGroupMgr();
    Guild* GetGuild() const;
    PlayerbotGroupMgr* m_groupMgr;
};

class CreateGroupAction : public GuildRPGBaseAction
{
public:
    CreateGroupAction(PlayerbotAI* botAI) : GuildRPGBaseAction(botAI, "create group") {}
  
    virtual bool Execute(Event event);
    virtual bool isUseful();

private:   
    bool ValidateTargetComposition(const TargetGroupComposition& comp);
};

class CheckGroupAction : public GuildRPGBaseAction
{
public:
    CheckGroupAction(PlayerbotAI* botAI) : GuildRPGBaseAction(botAI, "check group") {}
  
    virtual bool Execute(Event event);
    virtual bool isUseful();
};

class DisbandGroupAction : public GuildRPGBaseAction
{
public:
    DisbandGroupAction(PlayerbotAI* botAI) : GuildRPGBaseAction(botAI, "disband group") {}
  
    virtual bool Execute(Event event);
    virtual bool isUseful();
    virtual bool isPossible();
};



#endif