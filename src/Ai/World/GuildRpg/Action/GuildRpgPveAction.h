#ifndef _PLAYERBOT_GUILDRPGPVEACTION_H
#define _PLAYERBOT_GUILDRPGPVEACTION_H

#include "GuildRpgBaseAction.h"
#include "GuildRpgInfo.h"
#include "PlayerbotAI.h"

struct DungeonSuggestion;

struct DungeonEntranceInfo
{
    std::string waypointName;  // name used in playerbots_dungeon_waypoint table (e.g. "RFC")
    uint32 mapId;              // instance map ID (e.g. MAP_RAGEFIRE_CHASM)
    uint32 entranceMapId;      // continent map where the entrance portal is
    uint32 entranceZoneId;     // zone containing the entrance
    uint32 taxiNodeAlliance;   // nearest taxi node for Alliance
    uint32 taxiNodeHorde;      // nearest taxi node for Horde
};

class GuildRpgPveAction : public GuildRpgBaseAction
{
public:
    GuildRpgPveAction(PlayerbotAI* botAI) : GuildRpgBaseAction(botAI, "guild rpg pve action") {}

    bool HandleSelection(Event event) override;
    bool HandlePreparation(Event event) override;
    bool HandleExecution(Event event) override;
    bool HandleCompletion(Event event) override;

private:
    const DungeonSuggestion* SelectDungeonForLevel(uint8 botLevel) const;
    TargetGroupComposition CreatePveGroupComposition(const DungeonSuggestion& dungeon) const;

    static const DungeonEntranceInfo* GetEntranceInfo(const std::string& dungeonName);
    bool MoveGroupThroughPortal(const DungeonEntranceInfo* entrance);

    static constexpr int32 dungeonRunDuration = 60 * MINUTE * IN_MILLISECONDS;
    static constexpr uint8 DEFAULT_GROUP_SIZE = 5;
};

#endif
