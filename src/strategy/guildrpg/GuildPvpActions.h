#ifndef _PLAYERBOT_GUILDPVPACTIONS_H
#define _PLAYERBOT_GUILDPVPACTIONS_H

#include "GuildRpgBaseAction.h"
#include "GuildRpgInfo.h"
#include "PlayerbotAI.h"

/*
Actions for PVP guilds
Capitol City Raids
    Group members to group up at waypoint, initiate the attack
    Attack the Capitol City
    Kill the Leader

Raiding high probability raid locations.
    Subset of the guild members attack a city.

Friendly Dueling at friendly cities (Also targets of PVP raids)

World PVP actions
    EPL
    Silithus
    Nagrand/Halaa
    Hellfire Peninsula
    Terokkar Forest
    Zangarmarsh
    Wintergrasp
*/
class GuildPvpActions : public GuildRpgBaseAction
{
public:
    GuildPvpActions(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild pvp rpg") {}
    
    bool HandleSelection(Event event) override;
    bool HandlePreparation(Event event) override;
    bool HandleExecution(Event event) override;
    bool HandleCompletion(Event event) override;

};

class FriendlyDuelAction : public GuildRpgBaseAction
{
public:
    FriendlyDuelAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild friendly duel") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};
/*
class AttackCityAction : public GuildRpgBaseAction
{
public:
    AttackCityAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild attack city") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

class RaidHighProbabilityLocationAction : public GuildRpgBaseAction
{
public:
    RaidHighProbabilityLocationAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild raid high prob location") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

class PatrolAreaAction : public GuildRpgBaseAction
{
public:
    PatrolAreaAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild patrol area") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

class DefendBaseAction : public GuildRpgBaseAction
{
public:
    DefendBaseAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild defend base") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

class WorldPvpAction : public GuildRpgBaseAction
{
public:
    WorldPvpAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild world pvp") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};

class BGPreparationAction : public GuildRpgBaseAction
{
public:
    BGPreparationAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild bg preparation") {}
    bool isUseful() override;
    bool Execute(Event event) override;
};
*/
#endif