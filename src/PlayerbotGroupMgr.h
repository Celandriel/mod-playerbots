#ifndef _PLAYERBOT_GROUPMGR_H
#define _PLAYERBOT_GROUPMGR_H

#include "Group.h"
#include "Guild.h"
#include "Action.h"

enum BotRoles : uint8;

struct GuildMember
{
    ObjectGuid guid;
    uint8_t level;
    BotRoles role;
};

struct TargetGroupComposition
{
    uint8_t groupSize = 0;
    uint8_t tanks = 0;
    uint8_t minHealers = 0;
    uint8_t maxHealers = 0;
    uint8_t minDps = 0;
    uint8_t maxDps = 0;
    uint8_t lowerLevelLimit = 0;
    uint8_t upperLevelLimit = 0;
    bool allowPartial = false;
};

class PlayerbotGroupMgr
{
public:
    explicit PlayerbotGroupMgr(PlayerbotAI* botAI);
    void Reset();

    bool CreateGroup();
    std::vector<GuildMember> FindAvailableGuildMembers();
    bool InviteBot(ObjectGuid guid);
    bool RemoveBot(ObjectGuid guid);
    bool DisbandGroup();
    bool CheckGroupComposition();
    bool IsCompositionAvailable();
    void CleanGroup();

    bool WaitingforResponse();
    bool SetTargetComposition(const TargetGroupComposition& composition);
    bool IsValidComposition(const TargetGroupComposition& composition);

    Group* GetGroup() const { return _group; }
    const std::map<BotRoles, int>& GetComposition() const { return _roleComposition; }
    const TargetGroupComposition& GetTargetComposition() const { return _targetComposition; }

private:
    TargetGroupComposition _targetComposition = {};
    PlayerbotAI* _botAI;
    std::map<BotRoles, int> _roleComposition;
    Group* _group;
    Guild* _guild;
    uint8_t totalMembers;
    bool levelrangeset = false;

    bool IsLevelWithinRange(uint8 level);
    BotRoles GetBotRole(ObjectGuid guid);
    void UpdateComposition();
    bool CanInviteMore(BotRoles role);
    std::vector<ObjectGuid> GetGuildMembers(uint32 guildId);
};

#endif // _PLAYERBOT_GROUPMGR_H