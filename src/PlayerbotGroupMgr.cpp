#include "PlayerbotGroupMgr.h"

#include "Action.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Guild.h"
#include "InviteToGroupAction.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotGuildMgr.h"
#include "Playerbots.h"

PlayerbotGroupMgr::PlayerbotGroupMgr(PlayerbotAI* botAI)
    : _botAI(botAI)
{
    _roleComposition[BOT_ROLE_TANK] = 0;
    _roleComposition[BOT_ROLE_HEALER] = 0;
    _roleComposition[BOT_ROLE_DPS] = 0;
}

void PlayerbotGroupMgr::Reset()
{
    totalMembers = 0;
    levelrangeset = false;
    _targetComposition = {};
    _roleComposition = {};
}

bool PlayerbotGroupMgr::CreateGroup()
{
    if (!_botAI)
    {
        LOG_DEBUG("playerbots", "[BotGroupMgr] CreateGroup: _botAI is null");
        return false;
    }
    if (_targetComposition.groupSize == 0)
    {
        LOG_DEBUG("playerbots", "[BotGroupMgr] CreateGroup: target groupSize is 0");
        return false;
    }
    Player* leader = _botAI->GetBot();
    if (!leader)
    {
        LOG_DEBUG("playerbots", "[BotGroupMgr] CreateGroup: leader bot is null");
        return false;
    }
    ObjectGuid leaderGuid = leader->GetGUID();
    Guild* guild = leader->GetGuild();
    if (!guild)
    {
        LOG_DEBUG("playerbots", "[BotGroupMgr] CreateGroup: leader {} has no guild", leader->GetName());
        return false;
    }

    LOG_DEBUG("playerbots", "CreateGroup: leader {} (guild {}), target size {}, tanks {}, healers {}-{}, dps {}-{}",
        leader->GetName(), guild->GetName(), _targetComposition.groupSize,
        _targetComposition.tanks, _targetComposition.minHealers, _targetComposition.maxHealers,
        _targetComposition.minDps, _targetComposition.maxDps);

    std::vector<GuildMember> availableMembers = FindAvailableGuildMembers(guild);
    if (availableMembers.empty())
    {
        LOG_DEBUG("playerbots", "[BotGroupMgr] CreateGroup: no available guild members found for {}", leader->GetName());
        return false;
    }

    LOG_DEBUG("playerbots", "[BotGroupMgr] CreateGroup: found {} available guild members", availableMembers.size());

    InviteToGroupAction InviteToGroupAction(_botAI);
    BotRoles leaderRole = GetBotRole(leaderGuid);
    _roleComposition[leaderRole] = 1;
    totalMembers = 1;

    LOG_DEBUG("playerbots", "[BotGroupMgr] CreateGroup: leader {} assigned role {}", leader->GetName(), static_cast<int>(leaderRole));

    // Shuffle the available members to randomize selection
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::shuffle(availableMembers.begin(), availableMembers.end(), rng);

    for (auto& member : availableMembers)
    {
        if (totalMembers >= _targetComposition.groupSize)
        {
            LOG_DEBUG("playerbots", "CreateGroup: reached target group size {}", totalMembers);
            break;
        }
        if (CanInviteMore(member.role))
        {
            Player* player = ObjectAccessor::FindPlayer(member.guid);
            if (!player)
            {
                LOG_DEBUG("playerbots", "CreateGroup: player for guid {} not found (offline?)", member.guid.GetCounter());
                continue;
            }
            if (InviteToGroupAction.Invite(leader, player))
            {
                _roleComposition[member.role]++;
                totalMembers++;
                LOG_DEBUG("playerbots", "CreateGroup: invited {} (role {}), totalMembers now {}",
                    player->GetName(), static_cast<int>(member.role), totalMembers);
            }
            else
            {
                LOG_DEBUG("playerbots", "CreateGroup: failed to invite {} (role {})",
                    player->GetName(), static_cast<int>(member.role));
            }
        }
        else
        {
            LOG_DEBUG("playerbots", "CreateGroup: skipping member (role {}), role slots full. Current: T={} H={} D={}",
                static_cast<int>(member.role),
                _roleComposition[BOT_ROLE_TANK], _roleComposition[BOT_ROLE_HEALER], _roleComposition[BOT_ROLE_DPS]);
        }
    }

    LOG_DEBUG("playerbots", "CreateGroup: finished with {} members (T={} H={} D={})",
        totalMembers, _roleComposition[BOT_ROLE_TANK], _roleComposition[BOT_ROLE_HEALER], _roleComposition[BOT_ROLE_DPS]);

    return true;
}

std::vector<GuildMember> PlayerbotGroupMgr::FindAvailableGuildMembers(Guild* guild)
{
    std::vector<GuildMember> availableMembers;
    if (!guild)
        return availableMembers;
    std::vector<ObjectGuid> guildMembers = GetGuildMembers(guild->GetId());

    LOG_DEBUG("playerbots", "FindAvailableGuildMembers: guild {} has {} total members from GetGuildMembers",
        guild->GetName(), guildMembers.size());

    uint32 skippedOffline = 0, skippedInGroup = 0, skippedLevel = 0, skippedNoRole = 0;

    for (const auto& memberGuid : guildMembers)
    {
        Player* player = ObjectAccessor::FindPlayer(memberGuid);
        if (!player)
        {
            skippedOffline++;
            continue;
        }
        if (player->GetGroup())
        {
            skippedInGroup++;
            continue;
        }

        uint8 playerlevel = player->GetLevel();
        if (levelrangeset &&
            (playerlevel < _targetComposition.lowerLevelLimit || playerlevel > _targetComposition.upperLevelLimit))
        {
            LOG_DEBUG("playerbots", "FindAvailableGuildMembers: {} level {} outside range [{}-{}]",
                player->GetName(), playerlevel, _targetComposition.lowerLevelLimit, _targetComposition.upperLevelLimit);
            skippedLevel++;
            continue;
        }

        BotRoles role = GetBotRole(memberGuid);
        if (role == BOT_ROLE_NONE)
        {
            LOG_DEBUG("playerbots", "FindAvailableGuildMembers: {} has BOT_ROLE_NONE, skipping", player->GetName());
            skippedNoRole++;
            continue;
        }
        availableMembers.push_back({memberGuid, playerlevel, role});
    }

    LOG_DEBUG("playerbots", "FindAvailableGuildMembers: {} available, skipped: {} offline, {} in group, {} out of level range, {} no role",
        availableMembers.size(), skippedOffline, skippedInGroup, skippedLevel, skippedNoRole);

    return availableMembers;
}

bool PlayerbotGroupMgr::RemoveBot(ObjectGuid guid)
{
    Player* bot = ObjectAccessor::FindPlayer(guid);
    if (!bot)
        return false;

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    if (guid == group->GetLeaderGUID())
    {
        DisbandGroup();
        return true;
    }

    WorldPacket* packet = new WorldPacket(CMSG_GROUP_UNINVITE_GUID, 8);
    *packet << guid;
    bot->GetSession()->QueuePacket(packet);
    UpdateComposition();

    return true;
}

bool PlayerbotGroupMgr::DisbandGroup()
{
    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    _botAI->LeaveOrDisbandGroup();
    return true;
}

BotRoles PlayerbotGroupMgr::GetBotRole(ObjectGuid guid)
{
    Player* bot = ObjectAccessor::FindPlayer(guid);
    if (!bot)
        return BOT_ROLE_NONE;

    PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot);
    if (!botAI)
        return BOT_ROLE_NONE;

    if (botAI->IsTank(bot))
        return BOT_ROLE_TANK;
    if (botAI->IsHeal(bot))
        return BOT_ROLE_HEALER;
    if (botAI->IsDps(bot))
        return BOT_ROLE_DPS;

    return BOT_ROLE_NONE;
}

bool PlayerbotGroupMgr::CanInviteMore(BotRoles role)
{
    switch (role)
    {
        case BOT_ROLE_TANK:
            return _roleComposition[BOT_ROLE_TANK] < _targetComposition.tanks;
        case BOT_ROLE_HEALER:
            return _roleComposition[BOT_ROLE_HEALER] < _targetComposition.maxHealers;
        case BOT_ROLE_DPS:
            return _roleComposition[BOT_ROLE_DPS] < _targetComposition.maxDps;
        default:
            return false;
    }
}

void PlayerbotGroupMgr::UpdateComposition()
{
    // Reset composition
    for (auto& pair : _roleComposition)
        pair.second = 0;

    Player* bot = _botAI->GetBot();
    if (!bot)
        return;
    Group* group = bot->GetGroup();
    if (!group)
        return;

    totalMembers = group->GetMembersCount();

    Group::MemberSlotList const& members = group->GetMemberSlots();
    for (Group::MemberSlot const& slot : members)
    {
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (!member)
            continue;

        BotRoles role = GetBotRole(slot.guid);
        _roleComposition[role]++;
    }
}

std::vector<ObjectGuid> PlayerbotGroupMgr::GetGuildMembers(uint32 guildId)
{
    return sPlayerbotGuildMgr.GetAvailableGuildMembers(guildId);
}

// Figure out if the group is complete
bool PlayerbotGroupMgr::CheckGroupComposition()
{
    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;
    Group* group = bot->GetGroup();
    if (!group)
        return false;

    UpdateComposition();
    if (_targetComposition.allowPartial && totalMembers > 1)
        return true;
    if (totalMembers != _targetComposition.groupSize)
        return false;
    if (_roleComposition[BOT_ROLE_TANK] != _targetComposition.tanks)
        return false;
    if (_roleComposition[BOT_ROLE_HEALER] > _targetComposition.maxHealers ||
        _roleComposition[BOT_ROLE_HEALER] < _targetComposition.minHealers)
        return false;
    if (_roleComposition[BOT_ROLE_DPS] > _targetComposition.maxDps ||
        _roleComposition[BOT_ROLE_DPS] < _targetComposition.minDps)
        return false;
    if (levelrangeset)
    {
        uint8_t playerlevel = 0;
        for (auto& member : group->GetMemberSlots())
        {
            Player* player = ObjectAccessor::FindPlayer(member.guid);
            if (!player)
                return false;
            playerlevel = player->GetLevel();
            if (playerlevel < _targetComposition.lowerLevelLimit || playerlevel > _targetComposition.upperLevelLimit)
                return false;
        }
    }
    return true;
}

bool PlayerbotGroupMgr::SetTargetComposition(const TargetGroupComposition& composition)
{
    if (!IsValidComposition(composition))
        return false;
    _targetComposition = composition;
    if (_targetComposition.lowerLevelLimit == 0 && _targetComposition.upperLevelLimit == 0)
        levelrangeset = false;
    else
        levelrangeset = true;
    return true;
}

bool PlayerbotGroupMgr::IsLevelWithinRange(uint8 level)
{
    if (!levelrangeset)
        return true;

    const TargetGroupComposition& comp = _targetComposition;

    bool meetsMin = (comp.lowerLevelLimit == 0) || (level >= comp.lowerLevelLimit);
    bool meetsMax = (comp.upperLevelLimit == 0) || (level <= comp.upperLevelLimit);

    return meetsMin && meetsMax;
}

void PlayerbotGroupMgr::CleanGroup()
{
    Player* bot = _botAI->GetBot();
    if (!bot)
        return;
    Group* group = bot->GetGroup();
    if (!group)
        return;

    if (_targetComposition.groupSize == 0)
        return;

    uint8_t playerlevel = 0;
    _roleComposition[BOT_ROLE_TANK] = 0;
    _roleComposition[BOT_ROLE_HEALER] = 0;
    _roleComposition[BOT_ROLE_DPS] = 0;

    for (auto& member : group->GetMemberSlots())
    {
        if (levelrangeset)
        {
            Player* player = ObjectAccessor::FindPlayer(member.guid);
            if (!player)
            {
                RemoveBot(member.guid);
                continue;
            }
            playerlevel = player->GetLevel();
            if (!IsLevelWithinRange(playerlevel))
            {
                RemoveBot(member.guid);
                continue;
            }
        }
        BotRoles role = GetBotRole(member.guid);
        switch (role)
        {
            case BOT_ROLE_TANK:
            {
                if (_roleComposition[BOT_ROLE_TANK] >= _targetComposition.tanks)
                {
                    RemoveBot(member.guid);
                    continue;
                }
                break;
            }
            case BOT_ROLE_HEALER:
            {
                if (_roleComposition[BOT_ROLE_HEALER] >= _targetComposition.maxHealers)
                {
                    RemoveBot(member.guid);
                    continue;
                }
                break;
            }
            case BOT_ROLE_DPS:
            {
                if (_roleComposition[BOT_ROLE_DPS] >= _targetComposition.maxDps)
                {
                    RemoveBot(member.guid);
                    continue;
                }
                break;
            }
            default:
                break;
        }
        _roleComposition[role]++;
        totalMembers++;
    }
    CreateGroup();
}
bool PlayerbotGroupMgr::WaitingforResponse()
{
    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;
    Group* group = bot->GetGroup();
    if (!group || group->GetInviteeCount() != 0)
        return true;

    return false;
}

bool PlayerbotGroupMgr::IsCompositionAvailable()
{
    Player* bot = _botAI->GetBot();
    if (!bot)
        return false;
    Guild* guild = bot->GetGuild();
    if (!guild)
    {
        LOG_ERROR("playerbots", "Bot {} has no guild", bot->GetName());
        return false;
    }
    if (!IsValidComposition(_targetComposition))
        return false;

    std::vector<GuildMember> availableMembers = FindAvailableGuildMembers(guild);

    // Check if there are enough members for the group size (excluding the leader)
    if (!_targetComposition.allowPartial)
    {
        if (availableMembers.size() < (_targetComposition.groupSize > 0 ? _targetComposition.groupSize - 1 : 0))
        {
            LOG_ERROR("playerbots","Requested number of bots is not available for group.");
            return false;
        }
    }
    std::map<BotRoles, int> roleCounts;
    roleCounts[BOT_ROLE_TANK] = 0;
    roleCounts[BOT_ROLE_HEALER] = 0;
    roleCounts[BOT_ROLE_DPS] = 0;

    for (const auto& member : availableMembers)
        roleCounts[member.role]++;

    // Account for the leader's role
    BotRoles botRole = GetBotRole(bot->GetGUID());
    uint32 tanksNeeded = _targetComposition.tanks > 0 ? _targetComposition.tanks : 0;
    uint32 healersNeeded = _targetComposition.minHealers;
    uint32 dpsNeeded = _targetComposition.minDps;

    if (botRole == BOT_ROLE_TANK && tanksNeeded > 0)
        tanksNeeded--;
    else if (botRole == BOT_ROLE_HEALER && healersNeeded > 0)
        healersNeeded--;
    else if (botRole == BOT_ROLE_DPS && dpsNeeded > 0)
        dpsNeeded--;
    if (!_targetComposition.allowPartial)
    {
        if (roleCounts[BOT_ROLE_TANK] < tanksNeeded)
            return false;
        if (roleCounts[BOT_ROLE_HEALER] < healersNeeded)
            return false;
        if (roleCounts[BOT_ROLE_DPS] < dpsNeeded)
            return false;
    }
    return true;
}

bool PlayerbotGroupMgr::IsValidComposition(const TargetGroupComposition& composition)
{
    if (composition.groupSize == 0)
        return false;

    // Validate role requirements don't exceed group size
    uint32 minRequired = composition.tanks + composition.minHealers + composition.minDps;
    if (minRequired > composition.groupSize)
        return false;

    // Validate max roles
    if (composition.maxHealers < composition.minHealers)
        return false;
    if (composition.maxDps < composition.minDps)
        return false;

    //Check level limits.
    if (composition.lowerLevelLimit > composition.upperLevelLimit)
        return false;

    return true;
}
