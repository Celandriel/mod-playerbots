ifndef PVPGUILD_ACTIONS_H
define PVPGUILD_ACTIONS_H

#include 'Action.h'
#include 'GuildRPGBaseAction.h'

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

class FriendlyDuelAction : public GuildRPGBaseAction
{
    public:
        FriendlyDuelAction() : GuildRPGBaseAction("Friendly Duel") {}
        bool isUseful() override;
        bool Execute(Event event) override;
};

class RaidCapitolCityAction : public GuildRPGBaseAction
{
    public:
        RaidCapitolCityAction() : GuildRPGBaseAction("Raid Capitol City") {}
        bool isUseful() override;
        bool Execute(Event event) override;
};

class RaidHighProbabilityLocationAction : public GuildRPGBaseAction
{
    public:
        RaidHighProbabilityLocationAction() : GuildRPGBaseAction("Raid High Probability Location") {}
        bool isUseful() override;
        bool Execute(Event event) override;
};


#endif