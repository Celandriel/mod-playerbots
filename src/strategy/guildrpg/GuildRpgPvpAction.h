#ifndef _PLAYERBOT_GUILDRPGPVPACTIONS_H
#define _PLAYERBOT_GUILDRPGPVPACTIONS_H

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
class GuildRpgPvpAction : public GuildRpgBaseAction
{
public:
    GuildRpgPvpAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg pvp action") {}

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

#endif