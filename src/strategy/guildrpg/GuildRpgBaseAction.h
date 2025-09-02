#ifndef _PLAYERBOT_GUILDRPGBASEACTION_H
#define _PLAYERBOT_GUILDRPGBASEACTION_H

#include "Action.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"


class PlayerbotGroupMgr;

class GuildRpgBaseAction : public Action
{
public:
    GuildRpgBaseAction(PlayerbotAI* botAI, const std::string& name) : Action(botAI, name), m_groupMgr(nullptr) {}
    virtual ~GuildRpgBaseAction();
    uint8 RollRandomAction(std::vector<uint32> weights);

protected:
    PlayerbotGroupMgr* GetGroupMgr();
    Guild* GetGuild() const;
    PlayerbotGroupMgr* m_groupMgr;
};

class CreateGroupAction : public GuildRpgBaseAction
{
public:
    CreateGroupAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "create group") {}
  
    virtual bool Execute(Event event);
    virtual bool isUseful();

private:   
    bool ValidateTargetComposition(const TargetGroupComposition& comp);
};

class CheckGroupAction : public GuildRpgBaseAction
{
public:
    CheckGroupAction(PlayerbotAI* botAI) : GuildRPGBaseAction(botAI, "check group") {}
  
    virtual bool Execute(Event event);
    virtual bool isUseful();
};

class DisbandGroupAction : public GuildRpgBaseAction
{
public:
    DisbandGroupAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "disband group") {}
  
    virtual bool Execute(Event event);
    virtual bool isUseful();
    virtual bool isPossible();
};
#endif