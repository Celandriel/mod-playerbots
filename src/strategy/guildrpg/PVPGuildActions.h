ifndef PVPGUILD_ACTIONS_H
define PVPGUILD_ACTIONS_H

#include 'Action.h'
#include 'GuildRpgBaseAction.h'

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


bool ExecutePvpTask(const GuildTask& task)
{
    switch (task.objectiveId) // <- task.objectiveId = 3 (ATTACK_CITY)
    {
        case PvpActivity::QUEUE_FOR_BG:
        // Get the number of available bots to do PVP in the correct level bracket (Check before trying?)
        // Form Group
        //Qeueue for target BG
        case PvpActivity::PATROL_AREA:
        // Check Availability 
        // Form group size
        // GO to Target Area (Make common list of target areas to attaack and patrol)
        case PvpActivity::ATTACK_CITY:
        // Determine if Guild is available to do raid (Can We manage to recruit multiple guilds?)
        // First, set phase to ASSEMBLING and move to rally point (task.location)
        //  Then, set phase to EXECUTING and attack the city gates, then npcs.
         //   return HandlePvpAttackCity(task); // task.params tells us *which* city.
        case PVPActivity::DEFEND_BASE:
        // This is Triggered defensively based on some 'Under Attack' value
        case PVPActivity::WORLD_PVP:
        // Choose area to attack. 
        //group min number of bots to acheive. 
        //travel to area and Pillage.
    }
}

class FriendlyDuelAction : public GuildRpgBaseAction
{
    public:
        FriendlyDuelAction() : GuildRpgBaseAction("Friendly Duel") {}
        bool isUseful() override;
        bool Execute(Event event) override;
};

class AttackCityAction : public GuildRpgBaseAction
{
    public:
        RaidCapitolCityAction() : GuildRpgBaseAction("Raid Capitol City") {}
        bool isUseful() override;
        bool Execute(Event event) override;
};

class RaidHighProbabilityLocationAction : public GuildRpgBaseAction
{
    public:
        RaidHighProbabilityLocationAction() : GuildRpgBaseAction("Raid High Probability Location") {}
        bool isUseful() override;
        bool Execute(Event event) override;
};


#endif