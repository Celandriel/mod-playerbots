#ifndef _PLAYERBOT_GROUPMGR_H
#define _PLAYERBOT_GROUPMGR_H

#include "PlayerbotAI.h"
#include "Group.h"
#include "Action.h"

struct TargetComposition
{
    uint8_t groupSize;
    uint8_t tanks;
    uint8_t minHealers;
    uint8_t maxHealers;
    uint8_t minDps;
    uint8_t maxDps;
    uint8_t LowerLevelLimit;
    uint8_t UpperLevelLimit;
};
  
struct GuildMember
{
    ObjectGuid guid;
    uint8_t level;
    BotRoles role;
};

class PlayerbotAI;

class PlayerbotGroupMgr
{
public:
    explicit PlayerbotGroupMgr(PlayerbotAI* botAI);
    ~PlayerbotGroupMgr();

    bool CreateGroup(const TargetComposition& targetComposition);
    std::vector<GuildMember> FindAvailableGuildMembers(Guild* guild, const TargetComposition& targetComposition);
    bool InviteBot(ObjectGuid guid);
    bool RemoveBot(ObjectGuid guid);
    bool DisbandGroup();
    
    Group* GetGroup() const { return m_group; }
    const std::map<BotRoles, int>& GetComposition() const { return m_composition; }

private:
    PlayerbotAI* m_botAI;
    std::map<BotRoles, int> m_composition;
    Group* m_group;

    BotRoles GetBotRole(ObjectGuid guid);
    void UpdateComposition(); 
    bool CanInviteMore(BotRoles role, const TargetComposition& composition);
    std::vector<ObjectGuid> GetGuildMembers(uint32_t guildId);
};



#endif // _PLAYERBOT_GROUPMGR_H