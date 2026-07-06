#ifndef PLAYERBOTS_NEWRPGACTION_H
#define PLAYERBOTS_NEWRPGACTION_H

#include "Duration.h"
#include "MovementActions.h"
#include "NewRpgBaseAction.h"
#include "NewRpgInfo.h"
#include "NewRpgStrategy.h"
#include "Object.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "PlayerbotAI.h"
#include "QuestDef.h"
#include "RpgGoalSelector.h"
#include "TravelMgr.h"

class TellRpgStatusAction : public Action
{
public:
    TellRpgStatusAction(PlayerbotAI* botAI) : Action(botAI, "rpg status") {}

    bool Execute(Event event) override;
};

class StartRpgDoQuestAction : public Action
{
public:
    StartRpgDoQuestAction(PlayerbotAI* botAI) : Action(botAI, "start rpg do quest") {}

    bool Execute(Event event) override;
};

class NewRpgStatusUpdateAction : public NewRpgBaseAction
{
public:
    NewRpgStatusUpdateAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg status update")
    {
        // int statusCount = RPG_STATUS_END - 1;

        // transitionMat.resize(statusCount, std::vector<int>(statusCount, 0));

        // transitionMat[RPG_IDLE][RPG_GO_GRIND] = 20;
        // transitionMat[RPG_IDLE][RPG_GO_CAMP] = 15;
        // transitionMat[RPG_IDLE][RPG_WANDER_NPC] = 30;
        // transitionMat[RPG_IDLE][RPG_DO_QUEST] = 35;
    }
    bool Execute(Event event) override;

protected:
    // Utility-scored IDLE dispatch: try goals best-first, falling through
    // to the next-best goal when one fails to start.
    bool IntentionalChangeStatus();
    // Throttled mid-activity goal competition; returns true when the bot
    // was preempted back to IDLE for a re-pick.
    bool CheckGoalCompetition(RpgGoal incumbent);

    // static NewRpgStatusTransitionProb transitionMat;
    const int32 statusWanderNpcDuration = 5 * MINUTE  * IN_MILLISECONDS;
    const int32 statusWanderRandomDuration = 5 * MINUTE  * IN_MILLISECONDS;
    const int32 statusRestDuration = 30 * IN_MILLISECONDS;
    const int32 statusDoQuestDuration = 30 * MINUTE  * IN_MILLISECONDS;
    const int32 statusOutDoorPvPDuration = HOUR * IN_MILLISECONDS;
    const int32 statusGoCityDuration = 30 * MINUTE * IN_MILLISECONDS;
    // How often the mid-activity goal competition check runs (30 s).
    const uint32 statusMidActivityCheckInterval = 30 * IN_MILLISECONDS;
    // Minimum time in a status before the competition may preempt it (90 s).
    const uint32 statusMidActivityMinDwell = 90 * IN_MILLISECONDS;
    // Safety timebox for QUEST_HUB status (15 min).
    const int32 statusQuestHubDuration = 15 * MINUTE * IN_MILLISECONDS;
};

class NewRpgGoGrindAction : public NewRpgBaseAction
{
public:
    NewRpgGoGrindAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg go grind") {}
    bool Execute(Event event) override;
};

class NewRpgGoCampAction : public NewRpgBaseAction
{
public:
    NewRpgGoCampAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg go camp") {}
    bool Execute(Event event) override;
};

class NewRpgWanderRandomAction : public NewRpgBaseAction
{
public:
    NewRpgWanderRandomAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg wander random") {}
    bool Execute(Event event) override;
};

class NewRpgWanderNpcAction : public NewRpgBaseAction
{
public:
    NewRpgWanderNpcAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg move npcs") {}
    bool Execute(Event event) override;

    const uint32 npcStayTime = 8 * 1000;
};

class NewRpgDoQuestAction : public NewRpgBaseAction
{
public:
    NewRpgDoQuestAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg do quest") {}
    bool Execute(Event event) override;

protected:
    bool DoIncompleteQuest(NewRpgInfo::DoQuest& data);
    bool DoCompletedQuest(NewRpgInfo::DoQuest& data);

    const uint32 poiStayTime        = 5 * 60 * 1000;
    const uint32 poiStayTimeHardCap = 8 * 60 * 1000;
};

class NewRpgTravelFlightAction : public NewRpgBaseAction
{
public:
    NewRpgTravelFlightAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg travel flight") {}
    bool Execute(Event event) override;
};

class NewRpgGoCityAction : public NewRpgBaseAction
{
public:
    NewRpgGoCityAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg go city") {}
    bool Execute(Event event) override;

private:
    bool ExecuteAuctioneerTask(NewRpgInfo::GoCity& data);
    bool ExecuteVendorTask(NewRpgInfo::GoCity& data);
    bool ExecuteRepairTask(NewRpgInfo::GoCity& data);
    bool ExecuteTrainerTask(NewRpgInfo::GoCity& data);
    bool ExecuteInnkeeperTask(NewRpgInfo::GoCity& data);
    // Keep a failed one-shot service task active for a few ticks (bounded) instead
    // of dropping it, so a transient not-in-range failure can recover.
    bool RetryIfNotDone(NewRpgInfo::GoCity& data, bool actionRan);
};

class NewRpgQuestHubAction : public NewRpgBaseAction
{
public:
    NewRpgQuestHubAction(PlayerbotAI* botAI) : NewRpgBaseAction(botAI, "new rpg quest hub") {}
    bool Execute(Event event) override;

private:
    // How long (ms) with no actionable questgiver before declaring the hub done.
    static constexpr uint32 kHubNoTargetTimeout = 45 * IN_MILLISECONDS;
};

#endif
