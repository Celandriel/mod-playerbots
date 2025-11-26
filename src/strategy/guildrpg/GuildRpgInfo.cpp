#include "GuildRpgInfo.h"
#include "Log.h"
#include "PlayerbotAI.h"
// Global guild statistics storage

void GuildRpgInfo::SetGuildRpgActivity(PlayerbotAI* botAI, GuildRpgActivity activity)
{
    ResetGuildActivity();
    this->activity = activity; // Set the actual GuildRpgActivity enum
}

void GuildRpgInfo::ResetGuildActivity()
{
    activity = GuildRpgActivity::NONE;
    phase = GuildRpgPhase::IDLE;
}

std::string GuildRpgInfo::GetGuildTypeName(GuildType type)
{
    switch (type)
    {
        case GuildType::NONE:        return "NONE";
        case GuildType::PVP:         return "PVP";
        case GuildType::PVE:         return "PVE";
        case GuildType::PROFESSION:  return "PROFESSION";
        case GuildType::ROLEPLAY:    return "ROLEPLAY";
        default:                     return "UNKNOWN_TYPE";
    }
}

std::string GuildRpgInfo::GetPhaseName(GuildRpgPhase phase)
{
    switch (phase)
    {
        case GuildRpgPhase::IDLE:        return "IDLE";
        case GuildRpgPhase::SELECTION:   return "SELECTION";
        case GuildRpgPhase::GROUPING:    return "GROUPING";
        case GuildRpgPhase::PREPARATION: return "PREPARATION";
        case GuildRpgPhase::EXECUTING:   return "EXECUTING";
        case GuildRpgPhase::COMPLETED:   return "COMPLETED";
        default:                         return "UNKNOWN_PHASE";
    }
}

std::string GuildRpgInfo::GetActivityName()
{
    return GetActivityName(activity);
}
std::string GuildRpgInfo::GetActivityName(GuildRpgActivity activity)
{
    switch (activity)
    {
        case GuildRpgActivity::NONE:              return "NONE";

        case GuildRpgActivity::QUEUE_FOR_BG:      return "QUEUE_FOR_BG";
        case GuildRpgActivity::PATROL_AREA:       return "PATROL_AREA";
        case GuildRpgActivity::ATTACK_CITY:       return "ATTACK_CITY";
        case GuildRpgActivity::DEFEND_BASE:       return "DEFEND_BASE";
        case GuildRpgActivity::WORLD_PVP:         return "WORLD_PVP";

        case GuildRpgActivity::RUN_DUNGEON:       return "RUN_DUNGEON";
        case GuildRpgActivity::RUN_RAID:          return "RUN_RAID";
        case GuildRpgActivity::WORLD_EVENT:       return "WORLD_EVENT";

        case GuildRpgActivity::GATHER_NODES:      return "GATHER_NODES";
        case GuildRpgActivity::FARM_MOBS:         return "FARM_MOBS";
        case GuildRpgActivity::MASS_CRAFT:        return "MASS_CRAFT";
        case GuildRpgActivity::FISHING:           return "FISHING";

        case GuildRpgActivity::INNS_MEETUP:       return "INNS_MEETUP";
        case GuildRpgActivity::EMOTE_EVENT:       return "EMOTE_EVENT";
        default: return "NONE";
    }

    return "UNKNOWN";
}

std::string GuildRpgInfo::ToString() const
{
    std::stringstream out;
    out << "Status  ";
    out << "Guild Type: " << GetGuildTypeName(type) << ", "
        << "Selected Activity: " << GetActivityName() << ", "
        << "Phase " << GetPhaseName(phase);
    return out.str();
}

