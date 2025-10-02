#ifndef _PLAYERBOT_GUILDRPGBASEACTION_H
#define _PLAYERBOT_GUILDRPGBASEACTION_H

#include "Action.h"

class TellGuildRpgStatusAction public Action
{
public:
    TellGuildRpgStatusAction(PlayerbotAI* botAI, "guild rpg status") {}
    bool Execute(Event event) override;
}

class GuildRpgBaseAction : public Action
{
public:
    GuildRpgBaseAction(PlayerbotAI* botAI, const std::string& name);
    bool Execute(Event event) override;
    bool isUseful() override;

    void UpdatePhase(PlayerbotAI* botAI);
    bool isPhaseComplete()
    uint8 ChooseRandomActivity(std::vector<uint32> weights);

    virtual void HandleSelection();
    virtual void HandleGrouping();
    virtual void HandlePreparation();
    virtual void HandleExecution();
    virtual void HandleCompletion();

protected:
    Guild* GetGuild() const;
    bool SetGuildRpgActivity();
    
};

class GuildRpgActivityUpdateAction : public GuildRpgBaseAction
{
public:
    GuildRpgActivityUpdateAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg activity update") {}
    bool Execute(Event event) override;
};

#endif