#include "PlayerbotGroupMgr.h"

#include "Action.h"
#include "Group.h"
#include "GroupMgr.h"
#include "Guild.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotGuildMgr.h"
#include "Playerbots.h"

PlayerbotGroupMgr::PlayerbotGroupMgr(PlayerbotAI* botAI)
    : m_botAI(botAI), m_group(nullptr), m_guild(nullptr), totalMembers(0)
{
    m_roleComposition[BOT_ROLE_TANK] = 0;
    m_roleComposition[BOT_ROLE_HEALER] = 0;
    m_roleComposition[BOT_ROLE_DPS] = 0;
}

PlayerbotGroupMgr::~PlayerbotGroupMgr()
{
    if (m_group)
    {
        delete m_group;
        m_group = nullptr;
    }
}

bool PlayerbotGroupMgr::CreateGroup()
{
    if (!m_botAI)
        return false;
    if (m_targetComposition.groupSize == 0)
        return false;
    Player* leader = m_botAI->GetBot();
    if (!leader)
        return false;

    ObjectGuid leaderGuid = leader->GetGUID();
    m_guild = leader->GetGuild();
    if (!m_guild)
        return false;

    std::vector<GuildMember> availableMembers = FindAvailableGuildMembers();
    if (availableMembers.empty())
        return false;

    if (!m_group)
    {
        m_group = new Group();
        if (!m_group->Create(leader))
        {
            delete m_group;
            m_group = nullptr;
            return false;
        }
        sGroupMgr->AddGroup(m_group);
        BotRoles leaderRole = GetBotRole(leaderGuid);
        m_roleComposition[leaderRole] = 1;
    }

    UpdateComposition();
    // Shuffle the available members to randomize selection
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::shuffle(availableMembers.begin(), availableMembers.end(), rng);

    for (auto& member : availableMembers)
    {
        if (totalMembers >= m_targetComposition.groupSize)
            break;

        if (totalMembers > 1 && m_targetComposition.groupSize > 5 && !m_group->isRaidGroup())
            m_group->ConvertToRaid();

        if (CanInviteMore(member.role))
        {
            if (InviteBot(member.guid))
            {
                m_roleComposition[member.role]++;
                totalMembers++;
            }
        }
    }
    return true;
}

std::vector<GuildMember> PlayerbotGroupMgr::FindAvailableGuildMembers()
{
    std::vector<GuildMember> availableMembers;
    if (!m_guild)
        return availableMembers;
    std::vector<ObjectGuid> guildMembers = GetGuildMembers(m_guild->GetId());

    for (const auto& memberGuid : guildMembers)
    {
        Player* player = ObjectAccessor::FindPlayer(memberGuid);
        if (!player)
            continue;
        if (player->GetGroup())
            continue;

        uint8 playerlevel = player->GetLevel();
        if (levelrangeset &&
            (playerlevel < m_targetComposition.lowerLevelLimit || playerlevel > m_targetComposition.upperLevelLimit))
            continue;

        BotRoles role = GetBotRole(memberGuid);
        if (role == BOT_ROLE_NONE)
            continue;
        availableMembers.push_back({memberGuid, playerlevel, role});
    }
    return availableMembers;
}

bool PlayerbotGroupMgr::InviteBot(ObjectGuid guid)
{
    Player* bot = ObjectAccessor::FindPlayer(guid);
    if (!bot)
        return false;

    if (bot->GetGroup())
        return false;

    if (!m_group)
    {
        return false;
    }
    return m_group->AddInvite(bot);
}

bool PlayerbotGroupMgr::RemoveBot(ObjectGuid guid)
{
    Player* bot = ObjectAccessor::FindPlayer(guid);
    if (!bot)
        return false;

    if (!m_group)
        return false;

    if (guid == m_group->GetLeaderGUID())
    {
        DisbandGroup();
        return true;
    }
    m_group->RemoveMember(guid);
    UpdateComposition();

    if (m_group->GetMembersCount() == 0)
    {
        sGroupMgr->RemoveGroup(m_group);
        delete m_group;
        m_group = nullptr;
    }
    return true;
}

bool PlayerbotGroupMgr::DisbandGroup()
{
    if (!m_group)
        return false;

    m_group->Disband();
    sGroupMgr->RemoveGroup(m_group);
    delete m_group;
    m_group = nullptr;
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
            return m_roleComposition[BOT_ROLE_TANK] < m_targetComposition.tanks;
        case BOT_ROLE_HEALER:
            return m_roleComposition[BOT_ROLE_HEALER] < m_targetComposition.maxHealers;
        case BOT_ROLE_DPS:
            return m_roleComposition[BOT_ROLE_DPS] < m_targetComposition.maxDps;
        default:
            return false;
    }
}

void PlayerbotGroupMgr::UpdateComposition()
{
    // Reset composition
    for (auto& pair : m_roleComposition)
        pair.second = 0;

    if (!m_group)
        return;

    totalMembers = m_group->GetMembersCount();

    Group::MemberSlotList const& members = m_group->GetMemberSlots();
    for (Group::MemberSlot const& slot : members)
    {
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (!member)
            continue;

        BotRoles role = GetBotRole(slot.guid);
        m_roleComposition[role]++;
    }
}

std::vector<ObjectGuid> PlayerbotGroupMgr::GetGuildMembers(uint32 guildId)
{
    return sPlayerbotGuildMgr->GetAvailableGuildMembers(guildId);
}

// Figure out if the group is complete
bool PlayerbotGroupMgr::CheckGroupComposition()
{
    UpdateComposition();
    if (totalMembers != m_targetComposition.groupSize)
        return false;
    if (m_roleComposition[BOT_ROLE_TANK] != m_targetComposition.tanks)
        return false;
    if (m_roleComposition[BOT_ROLE_HEALER] > m_targetComposition.maxHealers ||
        m_roleComposition[BOT_ROLE_HEALER] < m_targetComposition.minHealers)
        return false;
    if (m_roleComposition[BOT_ROLE_DPS] > m_targetComposition.maxDps ||
        m_roleComposition[BOT_ROLE_DPS] < m_targetComposition.minDps)
        return false;
    if (levelrangeset)
    {
        uint8_t playerlevel = 0;
        for (auto& member : m_group->GetMemberSlots())
        {
            Player* player = ObjectAccessor::FindPlayer(member.guid);
            if (!player)
                return false;
            playerlevel = player->GetLevel();
            if (playerlevel < m_targetComposition.lowerLevelLimit || playerlevel > m_targetComposition.upperLevelLimit)
                return false;
        }
    }
    return true;
}

void PlayerbotGroupMgr::SetTargetComposition(const TargetGroupComposition& composition)
{
    m_targetComposition = composition;
    if (m_targetComposition.lowerLevelLimit == 0 && m_targetComposition.upperLevelLimit == 0)
        levelrangeset = false;
    else
        levelrangeset = true;
}

bool PlayerbotGroupMgr::IsLevelWithinRange(uint8 level)
{
    if (!levelrangeset)
        return true;

    const TargetGroupComposition& comp = m_targetComposition;

    bool meetsMin = (comp.lowerLevelLimit == 0) || (level >= comp.lowerLevelLimit);
    bool meetsMax = (comp.upperLevelLimit == 0) || (level <= comp.upperLevelLimit);

    return meetsMin && meetsMax;
}

void PlayerbotGroupMgr::CleanGroup()
{
    if (!m_group)
        return;

    if (m_targetComposition.groupSize == 0)
        return;

    uint8_t playerlevel = 0;
    m_roleComposition[BOT_ROLE_TANK] = 0;
    m_roleComposition[BOT_ROLE_HEALER] = 0;
    m_roleComposition[BOT_ROLE_DPS] = 0;

    for (auto& member : m_group->GetMemberSlots())
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
                if (m_roleComposition[BOT_ROLE_TANK] >= m_targetComposition.tanks)
                {
                    RemoveBot(member.guid);
                    continue;
                }
                break;
            }
            case BOT_ROLE_HEALER:
            {
                if (m_roleComposition[BOT_ROLE_HEALER] >= m_targetComposition.maxHealers)
                {
                    RemoveBot(member.guid);
                    continue;
                }
                break;
            }
            case BOT_ROLE_DPS:
            {
                if (m_roleComposition[BOT_ROLE_DPS] >= m_targetComposition.maxDps)
                {
                    RemoveBot(member.guid);
                    continue;
                }
                break;
            }
        }
        m_roleComposition[role]++;
        totalMembers++;
    }
    CreateGroup();
}
bool PlayerbotGroupMgr::WaitingforResponse()
{
    if (!m_group || m_group->GetInviteeCount() != 0)
    {
        return true;
    }
    return false;
}

bool PlayerbotGroupMgr::IsCompositionAvailable(const TargetGroupComposition& composition)
{
    if (!m_botAI || !m_botAI->GetBot())
        return false;

    Player* bot = m_botAI->GetBot();
    if (!bot->GetGuild())
        return false;

    m_guild = bot->GetGuild();
    SetTargetComposition(composition);
    std::vector<GuildMember> availableMembers = FindAvailableGuildMembers();

    // Check if there are enough members for the group size (excluding the leader)
    if (availableMembers.size() < (composition.groupSize > 0 ? composition.groupSize - 1 : 0))
        return false;

    std::map<BotRoles, int> roleCounts;
    roleCounts[BOT_ROLE_TANK] = 0;
    roleCounts[BOT_ROLE_HEALER] = 0;
    roleCounts[BOT_ROLE_DPS] = 0;

    for (const auto& member : availableMembers)
    {
        roleCounts[member.role]++;
    }

    // Account for the leader's role
    BotRoles botRole = GetBotRole(bot->GetGUID());
    uint32 tanksNeeded = composition.tanks > 0 ? composition.tanks : 0;
    uint32 healersNeeded = composition.minHealers;
    uint32 dpsNeeded = composition.minDps;

    if (botRole == BOT_ROLE_TANK && tanksNeeded > 0)
        tanksNeeded--;
    else if (botRole == BOT_ROLE_HEALER && healersNeeded > 0)
        healersNeeded--;
    else if (botRole == BOT_ROLE_DPS && dpsNeeded > 0)
        dpsNeeded--;

    if (roleCounts[BOT_ROLE_TANK] < tanksNeeded)
        return false;
    if (roleCounts[BOT_ROLE_HEALER] < healersNeeded)
        return false;
    if (roleCounts[BOT_ROLE_DPS] < dpsNeeded)
        return false;

    return true;
}
