#ifndef _PLAYERBOT_GROUPMGR_H
#define _PLAYERBOT_GROUPMGR_H

#include "Group.h"
#include "Action.h"
#include "PlayerbotAI.h"
 
struct GuildMember
{
    ObjectGuid guid;
    uint8_t level;
    BotRoles role;
};

class PlayerbotGroupMgr
{
public:
    explicit PlayerbotGroupMgr(PlayerbotAI* botAI);
    ~PlayerbotGroupMgr();

    bool CreateGroup();
    std::vector<GuildMember> FindAvailableGuildMembers();
    bool InviteBot(ObjectGuid guid);
    bool RemoveBot(ObjectGuid guid);
    bool DisbandGroup();
    bool CheckGroupComposition();
    bool IsCompositionAvailable(const TargetGroupComposition& composition);
    void CleanGroup();

    bool WaitingforResponse();
    void SetTargetComposition(const TargetGroupComposition& composition);
    
    Group* GetGroup() const { return m_group; }
    const std::map<BotRoles, int>& GetComposition() const { return m_roleComposition; }
    const TargetGroupComposition& GetTargetComposition() const { return m_targetComposition; }

private:
    TargetGroupComposition m_targetComposition;
    PlayerbotAI* m_botAI;
    std::map<BotRoles, int> m_roleComposition;
    Group* m_group;
    Guild* m_guild;
    uint8_t totalMembers;
    bool levelrangeset = false;

    bool IsLevelWithinRange(uint8 level);
    BotRoles GetBotRole(ObjectGuid guid);
    void UpdateComposition(); 
    bool CanInviteMore(BotRoles role);
    std::vector<ObjectGuid> GetGuildMembers(uint32 guildId);
};

#endif // _PLAYERBOT_GROUPMGR_H