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
    GuildRpgBaseAction(PlayerbotAI* botAI, const std::string& name);
    bool Execute(Event event) override;
    bool isUseful() override;

    void UpdatePhase(PlayerbotAI* botAI);
    bool isPhaseComplete();
    uint8 RollRandomActivity(std::vector<uint32> weights);
    bool ChooseRandomActivity();

    virtual bool HandleSelection(Event event);
    virtual bool HandleGrouping(Event event);
    virtual bool HandlePreparation(Event event);
    virtual bool HandleExecution(Event event);
    virtual bool HandleCompletion(Event event);

protected:
    Guild* GetGuild() const;

};

class GuildRpgActivityUpdateAction : public GuildRpgBaseAction
{
public:
    GuildRpgActivityUpdateAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg activity update") {}
    bool Execute(Event event) override;
};

#endif