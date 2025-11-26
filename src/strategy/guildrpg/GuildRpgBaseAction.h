#ifndef _PLAYERBOT_GUILDRPGBASEACTION_H
#define _PLAYERBOT_GUILDRPGBASEACTION_H

#include "Action.h"
#include "Guild.h"
#include "GuildRpgInfo.h"

class TellGuildRpgStatusAction : public Action
{
public:
    TellGuildRpgStatusAction(PlayerbotAI* botAI) : Action(botAI, "guildrpg status") {}
    bool Execute(Event event) override;
};

class GuildRpgBaseAction : public Action
{
public:
    GuildRpgBaseAction(PlayerbotAI* botAI, const std::string& name) : Action(botAI, name) {}
    bool Execute(Event event) override;
    bool isUseful() override;

    void UpdatePhase(PlayerbotAI* botAI);
    bool isPhaseComplete();
    GuildRpgActivity RollRandomActivity(std::vector<uint32> weights, ActivityList activities);
    bool ChooseRandomActivity();

    virtual bool HandleSelection(Event event);
    virtual bool HandleGrouping(Event event);
    virtual bool HandlePreparation(Event event);
    virtual bool HandleExecution(Event event);
    virtual bool HandleCompletion(Event event);

protected:
    Guild* GetGuild() const;

};

class GuildRpgStatusUpdateAction : public GuildRpgBaseAction
{
public:
    GuildRpgStatusUpdateAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg status update") {}
    bool Execute(Event event) override;
};

#endif
