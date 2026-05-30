/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "LastMovementValue.h"

#include "Timer.h"

LastMovement::LastMovement() { clear(); }

LastMovement::LastMovement(LastMovement& other)
    : taxiNodes(other.taxiNodes),
      taxiMaster(other.taxiMaster),
      lastFollow(other.lastFollow),
      lastAreaTrigger(other.lastAreaTrigger),
      lastFlee(other.lastFlee)
{
    lastMoveShort = other.lastMoveShort;
    nextTeleport = other.nextTeleport;
    lastPath = other.lastPath;
    priority = other.priority;
    lastTransportEntry = other.lastTransportEntry;
}

void LastMovement::clear()
{
    lastMoveShort = WorldPosition();
    lastPath.clear();
    lastFollow = nullptr;
    lastAreaTrigger = 0;
    lastFlee = 0;
    nextTeleport = 0;
    msTime = 0;
    priority = MovementPriority::MOVEMENT_NORMAL;
    lastTransportEntry = 0;
}

void LastMovement::Set(Unit* follow)
{
    Set(0, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
    setShort(WorldPosition());
    setPath(TravelPath());
    lastFollow = follow;
}

void LastMovement::Set(uint32 mapId, float x, float y, float z, float ori, float delayTime, MovementPriority pri)
{
    lastFollow = nullptr;
    lastMoveShort = WorldPosition(mapId, x, y, z, ori);
    msTime = getMSTime();
    priority = pri;
}

void LastMovement::setShort(WorldPosition point)
{
    lastMoveShort = point;
    lastFollow = nullptr;
}

void LastMovement::setPath(TravelPath path) { lastPath = path; }
