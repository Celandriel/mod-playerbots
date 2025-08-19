#ifndef _PLAYERBOT_GUILDRPGBASEACTION_H
#define _PLAYERBOT_GUILDRPGBASEACTION_H

#include "Action.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"


class PlayerbotGroupMgr;

class GuildRPGBaseAction : public Action
{
public:
    GuildRPGBaseAction(PlayerbotAI* botAI, const std::string& name) : Action(botAI, "guild rpg base action"), m_groupMgr(nullptr) {}
    virtual ~GuildRPGBaseAction();

protected:
    PlayerbotGroupMgr* GetGroupMgr();
    Guild* GetGuild() const;
    
private:
    PlayerbotGroupMgr* m_groupMgr;
};

class CreateGroupAction : public GuildRPGBaseAction
{
public:
    CreateGroupAction(PlayerbotAI* botAI) : GuildRPGBaseAction(botAI, "create group") {}
    virtual ~CreateGroupAction() { delete m_groupMgr; }

    virtual bool Execute(Event& event);
    virtual bool isUseful();
    virtual bool isPossible();

    void SetTargetComposition(const TargetComposition& composition) { m_targetComposition = composition; }
    const TargetComposition& GetTargetComposition() const { return m_targetComposition; }

private:
    PlayerbotGroupMgr* m_groupMgr;
    TargetComposition m_targetComposition;
    
    bool ValidateComposition();
    bool HasSufficientGuildMembers() const;
};

#endif