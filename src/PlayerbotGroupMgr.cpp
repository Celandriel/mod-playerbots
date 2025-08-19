#include "Action.h"
#include "GroupMgr.h"
#include "Group.h"
#include "Guild.h"
#include "Player.h"
#include "Playerbots.h"
#include "PlayerbotAI.h"
#include "PlayerbotGroupMgr.h"



PlayerbotGroupMgr::PlayerbotGroupMgr(PlayerbotAI* botAI) 
    : m_botAI(botAI), m_group(nullptr)
{
    m_composition[BOT_ROLE_TANK] = 0;
    m_composition[BOT_ROLE_HEALER] = 0;
    m_composition[BOT_ROLE_DPS] = 0;
}

PlayerbotGroupMgr::~PlayerbotGroupMgr()
{
    if (m_group)
        DisbandGroup();
}

bool PlayerbotGroupMgr::CreateGroup(const TargetComposition& targetComposition)
{
    if (!m_botAI)
        return false;

    Player* leader = m_botAI->GetBot();
    ObjectGuid leaderGuid = leader->GetGUID();
    Guild* guild = leader->GetGuild();
    if (!guild)
        return false;
    
    if (targetComposition.groupSize == 0)
        return false;

    std::vector<GuildMember> availableMembers = FindAvailableGuildMembers(guild, targetComposition); 
    if (availableMembers.empty())
        return false;

    if (!m_group)
    {
        Group* group = new Group();
        if (!group->Create(leader))
        {
            delete group;
            return false;
        }
        sGroupMgr->AddGroup(group);
        m_group = group;
    }
    BotRoles leaderRole = GetBotRole(leaderGuid);
    m_composition[leaderRole] = 1;
    
    // Shuffle the available members to randomize selection
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::shuffle(availableMembers.begin(), availableMembers.end(), rng);
    
    int totalMembers = 1;
    for (auto& member : availableMembers)
    {
        if (totalMembers >= targetComposition.groupSize)
            break;
        
        if (totalMembers > 1 && targetComposition.groupSize > 5 && !m_group->isRaidGroup())
            m_group->ConvertToRaid();

        if (CanInviteMore(GetBotRole(member.guid), targetComposition))
        {
            if (InviteBot(member.guid))
            {
                m_composition[member.role]++;
                totalMembers++;
            }
        }
    } 
    return true;
}

std::vector<GuildMember> PlayerbotGroupMgr::FindAvailableGuildMembers(Guild* guild, const TargetComposition& targetComposition)
{
    std::vector<GuildMember> availableMembers;
    if (!guild)
        return availableMembers;
    std::vector<ObjectGuid> guildMembers = GetGuildMembers(guild->GetId());
    for (const auto& memberGuid : guildMembers)
    {
        Player* player = ObjectAccessor::FindPlayer(memberGuid);
        if (!player)
            continue;
        if (player->GetGroup())
            continue;
        
        uint8_t playerlevel = player->GetLevel();
        if ((targetComposition.LowerLevelLimit > 0 && 
            playerlevel < targetComposition.LowerLevelLimit )||
            (targetComposition.UpperLevelLimit > 0 && 
            playerlevel > targetComposition.UpperLevelLimit))
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

    Group* group = bot->GetGroup();
    if (!group)
        return false;

    group->RemoveMember(guid);
    UpdateComposition();

    if (group->GetMembersCount() == 0) 
    {
        sGroupMgr->RemoveGroup(group);
        delete group;
        m_group = nullptr;
    }
    return true;
}

bool PlayerbotGroupMgr::DisbandGroup()
{
    if (!m_group)
        return false;

    if (m_group)
    {
        m_group->Disband();
        sGroupMgr->RemoveGroup(m_group);
        delete m_group;
        m_group = nullptr;
    }
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

bool PlayerbotGroupMgr::CanInviteMore(BotRoles role, const TargetComposition& composition)
{
    switch (role)
    {
        case BOT_ROLE_TANK:
            return m_composition[BOT_ROLE_TANK] < composition.tanks;
        case BOT_ROLE_HEALER:
            return m_composition[BOT_ROLE_HEALER] < composition.maxHealers;
        case BOT_ROLE_DPS:
            return m_composition[BOT_ROLE_DPS] < composition.maxDps;
        default:
            return false;
    }
}

void PlayerbotGroupMgr::UpdateComposition()
{
    // Reset composition
    for (auto& pair : m_composition)
        pair.second = 0;
        
    if (!m_group)
        return;
    
    Group::MemberSlotList const& members = m_group->GetMemberSlots();
    for (Group::MemberSlot const& slot : members)
    {
        Player* member = ObjectAccessor::FindPlayer(slot.guid);
        if (member)
        {
            BotRoles role = GetBotRole(slot.guid);
            m_composition[role]++;
        }
    }
}

std::vector<ObjectGuid> PlayerbotGroupMgr::GetGuildMembers(uint32 guildId)
{
    std::vector<ObjectGuid> guids;
    
    HashMapHolder<Player>::MapType const& players = ObjectAccessor::GetPlayers();
    for (const auto& pair : players) {
        Player* player = pair.second;
        if (player && player->GetGuildId() == guildId) {
            guids.push_back(player->GetGUID());
        }
    }
    
    return guids;
}