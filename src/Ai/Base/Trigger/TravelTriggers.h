/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#ifndef PLAYERBOTS_TRAVELTRIGGERS_H
#define PLAYERBOTS_TRAVELTRIGGERS_H

#include "Trigger.h"

class PlayerbotAI;

class GoTravelActiveTrigger : public Trigger
{
public:
    GoTravelActiveTrigger(PlayerbotAI* botAI) : Trigger(botAI, "go travel active") {}

    bool IsActive() override;
};

// Fires on arrival, and when the `travel` strategy is on without a trip (e.g. `nc +travel`).
class GoTravelFinishedTrigger : public Trigger
{
public:
    GoTravelFinishedTrigger(PlayerbotAI* botAI) : Trigger(botAI, "go travel finished") {}

    bool IsActive() override;
};

class NearDarkPortalTrigger : public Trigger
{
public:
    NearDarkPortalTrigger(PlayerbotAI* botAI) : Trigger(botAI, "near dark portal", 10) {}

    virtual bool IsActive();
};

class AtDarkPortalAzerothTrigger : public Trigger
{
public:
    AtDarkPortalAzerothTrigger(PlayerbotAI* botAI) : Trigger(botAI, "at dark portal azeroth", 10) {}

    bool IsActive() override;
};

class AtDarkPortalOutlandTrigger : public Trigger
{
public:
    AtDarkPortalOutlandTrigger(PlayerbotAI* botAI) : Trigger(botAI, "at dark portal outland", 10) {}

    bool IsActive() override;
};

#endif
