/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_TRAVELACTION_H
#define PLAYERBOTS_TRAVELACTION_H

#include "MovementActions.h"
#include "NewRpgBaseAction.h"

class PlayerbotAI;

// Starts and ends the trips the `travel` strategy runs for `go travel <name>`.
class GoTravel
{
public:
    static void Start(PlayerbotAI* botAI, WorldPosition const& destination, std::string const& name);
    static void End(PlayerbotAI* botAI, bool restoreFollow);
    static bool IsActive(PlayerbotAI* botAI);
};

class MoveToGoTravelDestinationAction : public NewRpgBaseAction
{
public:
    MoveToGoTravelDestinationAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "move to go travel destination") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class EndGoTravelAction : public Action
{
public:
    EndGoTravelAction(PlayerbotAI* botAI) : Action(botAI, "end go travel") {}

    bool Execute(Event event) override;
};

class MoveToDarkPortalAction : public MovementAction
{
public:
    MoveToDarkPortalAction(PlayerbotAI* botAI) : MovementAction(botAI, "move to dark portal") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class DarkPortalAzerothAction : public MovementAction
{
public:
    DarkPortalAzerothAction(PlayerbotAI* botAI) : MovementAction(botAI, "dark portal azeroth") {}

    bool Execute(Event event) override;
    bool isUseful() override;
};

class MoveFromDarkPortalAction : public MovementAction
{
public:
    MoveFromDarkPortalAction(PlayerbotAI* botAI) : MovementAction(botAI, "move from dark portal") {}

    bool Execute(Event event) override;
};

#endif
