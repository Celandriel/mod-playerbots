#include "DungeonWaypointMgr.h"
#include "Playerbots.h"

void DungeonWaypointMgr::LoadWaypoints()
{
    QueryResult result = PlayerbotsDatabase.Query("SELECT map_id, dungeon_name, order_index, x, y, z, jump, pause, healer_mana_pct, next_index, interact_type, interact_guid, interact_param, comment, tell FROM playerbots_dungeon_waypoint ORDER BY map_id, dungeon_name, order_index");
    if (!result) return;

    do {
        Field* fields = result->Fetch();
        uint32_t mapId = fields[0].Get<uint32_t>();
        std::string dungeonName = fields[1].Get<std::string>();
        DungeonWaypoint wp;
        wp.x = fields[3].Get<float>();
        wp.y = fields[4].Get<float>();
        wp.z = fields[5].Get<float>();
        wp.jump = fields[6].Get<bool>();
        wp.pause = fields[7].Get<uint32_t>();
        wp.healer_mana_pct = fields[8].Get<float>();
        wp.next_index = fields[9].Get<uint32_t>();
        wp.interact_type = fields[10].Get<uint8_t>();
        wp.interact_guid = fields[11].Get<uint32_t>();
        wp.interact_param = fields[12].Get<int32_t>();
        wp.comment = fields[13].Get<std::string>();
        wp.tell = fields[14].Get<bool>();
        paths[mapId][dungeonName].push_back(wp);
    } while (result->NextRow());
}

const DungeonPath* DungeonWaypointMgr::GetPath(uint32_t mapId, const std::string& dungeonName) const
{
    auto it = paths.find(mapId);
    if (it != paths.end())
    {
        auto jt = it->second.find(dungeonName);
        if (jt != it->second.end())
        {
            return &jt->second;
        }
    }
    return nullptr;
}
