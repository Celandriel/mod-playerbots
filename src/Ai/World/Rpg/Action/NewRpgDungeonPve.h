#ifndef _PLAYERBOT_NEWRPGDUNGEONPVE_H
#define _PLAYERBOT_NEWRPGDUNGEONPVE_H

#include "NewRpgBaseAction.h"
#include "DungeonWaypointMgr.h"
#include <chrono>

class NewRpgDungeonPveAction : public NewRpgBaseAction
{
public:
    NewRpgDungeonPveAction(PlayerbotAI* botAI, DungeonWaypointMgr* mgr)
        : NewRpgBaseAction(botAI, "new rpg dungeon pve"), waypointMgr(mgr) {}

    bool Execute(Event event) override;

private:
    static constexpr float WAYPOINT_REACHED_DISTANCE = 6.0f;
    static constexpr float RESUME_PATH_DISTANCE = 40.0f;
    static constexpr size_t RESUME_SEARCH_RANGE = 3;
    static constexpr float NPC_INTERACT_DISTANCE = 20.0f;
    static constexpr uint32_t NOTIFICATION_COOLDOWN_SECONDS = 10;
    static constexpr uint32_t WAIT_FOR_GROUP_DELAY_MS = 3000;

    size_t FindClosestWaypoint(const DungeonPath* path, Player* bot) const;
    size_t DetermineTargetIndex(const DungeonPath* path, Player* bot, size_t closestIndex);
    bool CheckGroupConditions(Player* bot, const DungeonWaypoint& waypoint) const;
    void HandleWaypointInteraction(const DungeonWaypoint& waypoint, Player* bot, size_t waypointIndex);
    void HandleWaypointNotification(const DungeonWaypoint& waypoint);
    void HandleCombatEngagement(Player* bot, Event event);

    DungeonWaypointMgr* waypointMgr;
    size_t previousIndex = 0;
    size_t lastInteractedIndex = SIZE_MAX;
    size_t pendingMenuInteractionIndex = SIZE_MAX;
    int32_t pendingMenuOption = -1;
    std::chrono::steady_clock::time_point lastNotifyTime = std::chrono::steady_clock::now() - std::chrono::seconds(30);
    std::chrono::steady_clock::time_point menuInteractionTime = std::chrono::steady_clock::now();
};

#endif
