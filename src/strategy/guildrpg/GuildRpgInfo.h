#ifndef _PLAYERBOT_GUILDRPGINFO_H
#define _PLAYERBOT_GUILDRPGINFO_H

#include <utility>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "Define.h"

class PlayerbotAI;

enum class GuildRpgPhase
{
    IDLE,
    SELECTION,
    GROUPING,
    PREPARATION,
    EXECUTING,
    COMPLETED
};

enum class GuildType
{
    NONE,
    PVP,
    PVE,
    PROFESSION,
    ROLEPLAY
};

using GuildRpgActivityMap = std::map<GuildType, std::map<uint8_t, std::string>>;

static const GuildRpgActivityMap Activities =
{
    {GuildType::NONE, {{ 0, "IDLE"}}},
    {GuildType::PVP, {
        {0, "IDLE"},
        {1, "QUEUE_FOR_BG"},
        {2, "PATROL_AREA"},
        {3, "ATTACK_CITY"},
        {4, "DEFEND_BASE"},
        {5, "WORLD_PVP"}
    }},
    {GuildType::PVE, {
        {0, "IDLE"},
        {1, "RUN_DUNGEON"},
        {2, "RUN_RAID"},
        {3, "WORLD_EVENT"}
    }},
    {GuildType::PROFESSION, {
        {0, "IDLE"},
        {1, "GATHER_NODES"},
        {2, "FARM_MOBS"},
        {3, "MASS_CRAFT"},
        {4, "FISHING"}
    }},
    {GuildType::ROLEPLAY, {
        {0, "IDLE"},
        {1, "INNS_MEETUP"},
        {2, "EMOTE_EVENT"}
    }}
};


struct GuildRpgInfo
{
    GuildType type = GuildType::NONE;
    uint8_t activityNumber = 0;
    GuildRpgPhase phase = GuildRpgPhase::IDLE;
    void SetGuildRpgActivity(PlayerbotAI* botAI, uint8_t activity);
    void ResetGuildActivity();
    std::string GetActivityName() const;
    void SetGuildRpgPhase(GuildRpgPhase newPhase) { phase = newPhase; }
};

#endif