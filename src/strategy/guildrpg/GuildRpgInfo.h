#ifndef _PLAYERBOT_GUILDRPGINFO_H
#define _PLAYERBOT_GUILDRPGINFO_H

#include <utility>
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include "Define.h"
#include "GameObject.h"

class PlayerbotAI;

enum class GuildRpgPhase : uint8
{
    IDLE = 0,
    SELECTION,
    GROUPING,
    PREPARATION,
    EXECUTING,
    COMPLETED
};

enum class GuildType : uint8
{
    NONE = 0,
    PVP,
    PVE,
    PROFESSION,
    ROLEPLAY
};

enum class GuildRpgActivity : uint8_t
{
    NONE = 0,

    // PVP
    BATTLEGROUND,
    PATROL_AREA,
    ATTACK_CITY,
    DEFEND_BASE,
    WORLD_PVP,

    // PVE
    RUN_DUNGEON,
    RUN_RAID,
    WORLD_EVENT,

    // PROFESSION
    GATHER_NODES,
    FARM_MOBS,
    MASS_CRAFT,
    FISHING,

    // ROLEPLAY
    INN_MEETUP,
    EMOTE_EVENT
};
using ActivityList = std::vector<GuildRpgActivity>;

static const std::map<GuildType, ActivityList> ActivitiesByGuildType =
{
    { GuildType::NONE,       { GuildRpgActivity::NONE } },

    { GuildType::PVP, {
        GuildRpgActivity::BATTLEGROUND,
        GuildRpgActivity::PATROL_AREA,
        GuildRpgActivity::ATTACK_CITY,
        GuildRpgActivity::DEFEND_BASE,
        GuildRpgActivity::WORLD_PVP,
    }},

    { GuildType::PVE, {
        GuildRpgActivity::RUN_DUNGEON,
        GuildRpgActivity::RUN_RAID,
        GuildRpgActivity::WORLD_EVENT,
    }},

    { GuildType::PROFESSION, {
        GuildRpgActivity::GATHER_NODES,
        GuildRpgActivity::FARM_MOBS,
        GuildRpgActivity::MASS_CRAFT,
        GuildRpgActivity::FISHING,
    }},

    { GuildType::ROLEPLAY, {
        GuildRpgActivity::INN_MEETUP,
        GuildRpgActivity::EMOTE_EVENT,
    }}
};

struct GuildRpgInfo
{
    GuildType type = GuildType::NONE;
    GuildRpgActivity activity = GuildRpgActivity::NONE;
    GuildRpgPhase phase = GuildRpgPhase::IDLE;
    std::string activityTarget = "";

    void SetGuildRpgActivity(PlayerbotAI* botAI, GuildRpgActivity activity);
    void ResetGuildActivity();

    static std::string GetGuildTypeName(GuildType type);
    static std::string GetPhaseName(GuildRpgPhase phase);
    std::string ToString() const;
    std::string GetActivityName() const;
    std::string GetActivityName(GuildRpgActivity activity) const;

    void SetGuildRpgPhase(GuildRpgPhase newPhase) { phase = newPhase; }
    void SetGuildType(GuildType newType) { type = newType; }
};

#endif
