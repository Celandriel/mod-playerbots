#ifndef _PLAYERBOT_DUNGEONWAYPOINTMGR_H
#define _PLAYERBOT_DUNGEONWAYPOINTMGR_H

#include <vector>
#include <unordered_map>
#include <string>
#include <cstdint>

struct DungeonWaypoint
{
    float x, y, z;
    bool jump;
    uint32_t pause; // pause duration in ms, 0 = no pause
    float healer_mana_pct; // required healer mana percent to proceed (0.0-1.0)
    uint32_t next_index; // index of next waypoint in path, 0 = entrance
    uint8_t interact_type; // 0: none, 1: gossip, 2: escort, 3: object, etc.
    uint32_t interact_guid; // GUID of NPC/object
    int32_t interact_param; // dialogue option, item GUID, etc.
    std::string comment; // text to say at this waypoint
    bool tell; // whether to say the comment at this waypoint
};

using DungeonPath = std::vector<DungeonWaypoint>;

class DungeonWaypointMgr
{
public:
    void LoadWaypoints();
    const DungeonPath* GetPath(uint32_t mapId, const std::string& dungeonName) const;
    const std::unordered_map<uint32_t, std::unordered_map<std::string, DungeonPath>>& GetAllPaths() const { return paths; }
private:
    std::unordered_map<uint32_t, std::unordered_map<std::string, DungeonPath>> paths;
};

#endif
