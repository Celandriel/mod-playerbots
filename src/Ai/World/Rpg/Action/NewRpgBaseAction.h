#ifndef PLAYERBOTS_NEWRPGBASEACTION_H
#define PLAYERBOTS_NEWRPGBASEACTION_H

#include <utility>
#include <vector>

#include "Duration.h"
#include "LastMovementValue.h"
#include "MovementActions.h"
#include "NewRpgInfo.h"
#include "NewRpgStrategy.h"
#include "Object.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "PlayerbotAI.h"
#include "QuestDef.h"
#include "TravelMgr.h"

struct POIInfo
{
    G3D::Vector2 pos;
    int32 objectiveIdx;
    // Raw polygon points from QuestPOI::points (stored as float pairs to avoid
    // pulling ObjectMgr types into every translation unit that includes this header).
    std::vector<std::pair<float, float>> points;
};

/// A base (composition) class for all new rpg actions
/// All functions that may be shared by multiple actions should be declared here
/// And we should make all actions composable instead of inheritable
class NewRpgBaseAction : public MovementAction
{
public:
    NewRpgBaseAction(PlayerbotAI* botAI, std::string name) : MovementAction(botAI, name) {}

protected:
    /* MOVEMENT RELATED */
    bool MoveFarTo(WorldPosition dest);
    bool MoveWorldObjectTo(ObjectGuid guid, float distance = INTERACTION_DISTANCE);
    bool MoveRandomNear(float moveStep = 50.0f, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL, WorldObject* center = nullptr);
    bool ForceToWait(uint32 duration, MovementPriority priority = MovementPriority::MOVEMENT_NORMAL);

    /* QUEST RELATED CHECK */
    ObjectGuid ChooseNpcOrGameObjectToInteract(bool questgiverOnly = false, float distanceLimit = 0.0f);
    bool HasQuestToAcceptOrReward(WorldObject* object);
    bool InteractWithNpcOrGameObjectForQuest(ObjectGuid guid);
    bool CanInteractWithQuestGiver(Object* questGiver);
    bool IsWithinInteractionDist(Object* object);
    uint32 BestRewardIndex(Quest const* quest);
    bool IsQuestWorthDoing(Quest const* quest);
    bool IsQuestCapableDoing(Quest const* quest);

    /* QUEST RELATED ACTION */
    bool SearchQuestGiverAndAcceptOrReward();
    bool AcceptQuest(Quest const* quest, ObjectGuid guid);
    bool TurnInQuest(Quest const* quest, ObjectGuid guid);
    bool OrganizeQuestLog();

protected:
    bool GetQuestPOIPosAndObjectiveIdx(uint32 questId, std::vector<POIInfo>& poiInfo, bool toComplete = false);
    bool SelectAuctionHouseTarget(WorldPosition& outPos, uint32& outEntry, uint32& outZone);
    bool BuildCityTasks(std::vector<NewRpgInfo::CityTask>& outTaskList);
    // Append a service task for `taskKind` if `needed`, resolving a random NPC of
    // `service` inside `cityZone`. No-op when the city has no such NPC for the team.
    void AppendCityServiceTask(std::vector<NewRpgInfo::CityTask>& outTaskList, uint32 cityZone, bool needed,
                               NewRpgInfo::CityTaskType taskKind, TravelMgr::CityServiceType service);
    static WorldPosition SelectRandomGrindPos(Player* bot);
    static WorldPosition SelectRandomCampPos(Player* bot);
    bool SelectRandomFlightTaxiNode(uint32& flightMasterEntry, WorldPosition& flightMasterPos, std::vector<uint32>& path);
    bool RandomChangeStatus(std::vector<NewRpgStatus> candidateStatus);
    bool CheckRpgStatusAvailable(NewRpgStatus status);
    bool TryStartGoCity();
    bool TryStartDoQuest();
    // Questing dispatch: tries TryStartDoQuest first; if the log is empty/unworkable,
    // finds a level-appropriate quest hub via the TravelMgr index and travels to it.
    bool TryStartQuesting();
    // Intentional-mode: scan all log quests and return the POI nearest to the bot.
    // Skips lowPriorityQuest entries. Returns false when nothing workable is found.
    bool SelectNextQuestObjective(uint32& outQuestId, POIInfo& outPoi);

protected:
    /* FOR MOVE FAR */
    const float pathFinderDis = 70.0f;
    // Time without real progress toward dest before MoveFarTo
    // falls back to teleport recovery. Kept short enough that a
    // bot truly oscillating around an unreachable destination
    // (mmap returning non-progressing partial paths, or NOPATH +
    // cone fallback wandering) doesn't spin for 5 minutes before
    // the teleport fires, but long enough that a genuine long
    // walk that is slowly making progress never triggers it.
    const uint32 stuckTime = 90 * 1000;
};

#endif
