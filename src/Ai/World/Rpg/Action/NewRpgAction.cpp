#include "NewRpgAction.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>

#include "AhActions.h"
#include "AreaDefines.h"
#include "BroadcastHelper.h"
#include "Creature.h"
#include "GameObject.h"
#include "ChatHelper.h"
#include "G3D/Vector2.h"
#include "GossipDef.h"
#include "IVMapMgr.h"
#include "NewRpgInfo.h"
#include "NewRpgStrategy.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "PathGenerator.h"
#include "PlayerbotAIConfig.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "PlayerbotUtils.h"
#include "BotAHUtil.h"
#include "QuestDef.h"
#include "Random.h"
#include "PlayerbotRpgStateRepository.h"
#include "RpgGoalSelector.h"
#include "SharedDefines.h"
#include "StringFormat.h"
#include "Timer.h"
#include "TravelMgr.h"

bool TellRpgStatusAction::Execute(Event event)
{
    Player* owner = event.getOwner();
    if (!owner)
        return false;

    std::string out = botAI->rpgInfo.ToString();

    if (sPlayerbotAIConfig.newRpgIntentional)
    {
        float const mScore = RpgGoalSelector::ScoreGoal(botAI, RpgGoal::Maintenance);
        float const qScore = RpgGoalSelector::ScoreGoal(botAI, RpgGoal::Questing);
        float const lScore = RpgGoalSelector::ScoreGoal(botAI, RpgGoal::Leisure);
        out += Acore::StringFormat("\nGoal scores: M={:.1f} Q={:.1f} L={:.1f}", mScore, qScore, lScore);

        NewRpgInfo const& info = botAI->rpgInfo;
        if (info.activeHubId != 0)
        {
            // Use 2D distance: hub centroid z may differ from bot z on multi-floor zones.
            float const dist = bot->GetExactDist2d(info.activeHubPos);
            out += Acore::StringFormat("\nhub={} zone={} dist={:.0f}", info.activeHubId, info.activeHubZone, dist);
        }
    }

    bot->Whisper(out.c_str(), LANG_UNIVERSAL, owner);
    return true;
}

bool StartRpgDoQuestAction::Execute(Event event)
{
    Player* owner = event.getOwner();
    if (!owner)
        return false;

    std::string const text = event.getParam();
    PlayerbotChatHandler ch(owner);
    uint32 questId = ch.extractQuestId(text);
    const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
    if (quest)
    {
        botAI->rpgInfo.ChangeToDoQuest(questId, quest);
        bot->Whisper("Start to do quest " + std::to_string(questId), LANG_UNIVERSAL, owner);
        return true;
    }
    bot->Whisper("Invalid quest " + text, LANG_UNIVERSAL, owner);
    return false;
}

bool NewRpgStatusUpdateAction::Execute(Event /*event*/)
{
    NewRpgInfo& info = botAI->rpgInfo;
    NewRpgStatus status = info.GetStatus();
    switch (status)
    {
        case RPG_IDLE:
        {
            if (sPlayerbotAIConfig.newRpgIntentional)
                return IntentionalChangeStatus();

            return RandomChangeStatus({RPG_GO_CAMP, RPG_GO_GRIND, RPG_WANDER_RANDOM, RPG_WANDER_NPC,
                                       RPG_DO_QUEST, RPG_TRAVEL_FLIGHT, RPG_GO_CITY, RPG_OUTDOOR_PVP, RPG_REST});
        }

        case RPG_GO_GRIND:
        {
            auto& data = std::get<NewRpgInfo::GoGrind>(info.data);
            assert(data.pos != WorldPosition());
            // GO_GRIND -> WANDER_RANDOM on arrival
            if (bot->GetExactDist(data.pos) < 10.0f)
            {
                info.ChangeToWanderRandom();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Leisure))
                return true;
            break;
        }
        case RPG_GO_CITY:
        {
            if (info.HasStatusPersisted(statusGoCityDuration))
            {
                info.ChangeToIdle();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Maintenance))
                return true;
            break;
        }
        case RPG_GO_CAMP:
        {
            auto& data = std::get<NewRpgInfo::GoCamp>(info.data);
            WorldPosition& originalPos = data.pos;
            assert(data.pos != WorldPosition());
            // GO_CAMP -> WANDER_NPC on arrival
            if (bot->GetExactDist(originalPos) < 10.0f)
            {
                info.ChangeToWanderNpc();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Leisure))
                return true;
            break;
        }
        case RPG_WANDER_RANDOM:
        {
            if (info.HasStatusPersisted(statusWanderRandomDuration))
            {
                info.ChangeToIdle();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Leisure))
                return true;
            break;
        }
        case RPG_WANDER_NPC:
        {
            if (info.HasStatusPersisted(statusWanderNpcDuration))
            {
                info.ChangeToIdle();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Leisure))
                return true;
            break;
        }
        case RPG_DO_QUEST:
        {
            if (info.HasStatusPersisted(statusDoQuestDuration))
            {
                info.ChangeToIdle();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Questing))
                return true;
            break;
        }
        case RPG_TRAVEL_FLIGHT:
        {
            auto& data = std::get<NewRpgInfo::TravelFlight>(info.data);
            if (data.inFlight && !bot->IsInFlight())
            {
                // Flight arrived — return to IDLE for a fresh goal evaluation.
                info.ChangeToIdle();
                return true;
            }
            break;
        }
        case RPG_REST:
        {
            if (info.HasStatusPersisted(statusRestDuration))
            {
                info.ChangeToIdle();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Leisure))
                return true;
            break;
        }
        case RPG_OUTDOOR_PVP:
        {
            if (info.HasStatusPersisted(statusOutDoorPvPDuration))
            {
                info.ChangeToIdle();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Leisure))
                return true;
            break;
        }
        case RPG_QUEST_HUB:
        {
            if (info.HasStatusPersisted(statusQuestHubDuration))
            {
                info.ChangeToIdle();
                return true;
            }
            if (CheckGoalCompetition(RpgGoal::Questing))
                return true;
            break;
        }
        default:
            break;
    }
    return false;
}

bool NewRpgStatusUpdateAction::IntentionalChangeStatus()
{
    NewRpgInfo& info = botAI->rpgInfo;
    info.lastGoalEval = getMSTime();

    // Guaranteed variety: roll for a leisure break before scoring so high-scoring
    // maintenance/questing doesn't starve leisure completely. If leisure fails to
    // start anything, fall through to normal goal ranking.
    // roll_chance_f(p) returns true with probability p%.
    if (roll_chance_f(kLeisureBreakChance))
    {
        if (RandomChangeStatus({RPG_GO_CAMP, RPG_GO_GRIND, RPG_WANDER_RANDOM, RPG_WANDER_NPC,
                                RPG_TRAVEL_FLIGHT, RPG_OUTDOOR_PVP, RPG_REST}))
        {
            LOG_DEBUG("playerbots", "[New RPG] {} IDLE leisure break (chance={:.0f}%)", bot->GetName(),
                      kLeisureBreakChance);
            return true;
        }
    }

    std::array<RpgGoalResult, 3> ranked = {
        RpgGoalResult{RpgGoal::Maintenance, RpgGoalSelector::ScoreGoal(botAI, RpgGoal::Maintenance)},
        RpgGoalResult{RpgGoal::Questing, RpgGoalSelector::ScoreGoal(botAI, RpgGoal::Questing)},
        RpgGoalResult{RpgGoal::Leisure, RpgGoalSelector::ScoreGoal(botAI, RpgGoal::Leisure)}};
    std::sort(ranked.begin(), ranked.end(),
              [](RpgGoalResult const& a, RpgGoalResult const& b) { return a.score > b.score; });

    // Best-first with fallthrough: a goal that fails to start yields to the
    // next-best instead of forcing a bad activity.
    RpgGoal startedGoal = RpgGoal::None;
    for (RpgGoalResult const& candidate : ranked)
    {
        if (candidate.score <= 0.0f)
            continue;

        switch (candidate.goal)
        {
            case RpgGoal::Maintenance:
                if (TryStartGoCity())
                {
                    startedGoal = RpgGoal::Maintenance;
                }
                break;
            case RpgGoal::Questing:
                if (TryStartQuesting())
                {
                    startedGoal = RpgGoal::Questing;
                }
                break;
            case RpgGoal::Leisure:
                if (RandomChangeStatus({RPG_GO_CAMP, RPG_GO_GRIND, RPG_WANDER_RANDOM, RPG_WANDER_NPC,
                                        RPG_TRAVEL_FLIGHT, RPG_OUTDOOR_PVP, RPG_REST}))
                {
                    startedGoal = RpgGoal::Leisure;
                }
                break;
            default:
                break;
        }
        if (startedGoal != RpgGoal::None)
        {
            LOG_DEBUG("playerbots", "[New RPG] {} IDLE picks goal={} score={:.1f}", bot->GetName(),
                      static_cast<int>(startedGoal), candidate.score);
            return true;
        }
    }

    // Safety default, matching the legacy dice roll's all-unavailable path.
    info.ChangeToRest();
    bot->SetStandState(UNIT_STAND_STATE_SIT);
    return true;
}

bool NewRpgStatusUpdateAction::CheckGoalCompetition(RpgGoal incumbent)
{
    if (!sPlayerbotAIConfig.newRpgIntentional)
        return false;

    NewRpgInfo& info = botAI->rpgInfo;
    // Needs don't interrupt instantly: require a minimum dwell in the current
    // status, then re-evaluate at most once per check interval.
    if (!info.HasStatusPersisted(statusMidActivityMinDwell))
        return false;

    if (GetMSTimeDiffToNow(info.lastGoalEval) < statusMidActivityCheckInterval)
        return false;

    info.lastGoalEval = getMSTime();
    if (!RpgGoalSelector::ShouldPreempt(botAI, incumbent))
        return false;

    LOG_DEBUG("playerbots", "[New RPG] {} goal {} preempted by a higher-utility challenger", bot->GetName(),
              static_cast<int>(incumbent));
    info.ChangeToIdle();
    return true;
}

bool NewRpgQuestHubAction::Execute(Event /*event*/)
{
    NewRpgInfo& info = botAI->rpgInfo;
    auto* dataPtr = std::get_if<NewRpgInfo::QuestHub>(&info.data);
    if (!dataPtr)
        return false;
    auto& data = *dataPtr;

    // Travel to hub centroid first. Use 2D distance: centroid z is resolved at
    // commit time and may not match the bot's current z.
    if (bot->GetExactDist2d(data.pos) > 30.0f)
    {
        if (MoveFarTo(data.pos))
            return true;
        return MoveRandomNear(10.0f);
    }

    // Near hub: try to accept/turn-in quests within 80 yd.
    if (SearchQuestGiverAndAcceptOrReward())
    {
        data.lastNoTargetT = 0;
        return true;
    }

    // Nothing actionable from SearchQuestGiverAndAcceptOrReward — wander
    // between questgivers in the cluster so we visit all of them.
    ObjectGuid npcOrGo = ChooseNpcOrGameObjectToInteract(true);
    if (npcOrGo)
    {
        data.lastNoTargetT = 0;
        return MoveWorldObjectTo(npcOrGo);
    }

    // No questgiver at all in range.
    if (!data.lastNoTargetT)
    {
        data.lastNoTargetT = getMSTime();
        return false;
    }

    if (GetMSTimeDiffToNow(data.lastNoTargetT) < kHubNoTargetTimeout)
        return false;

    // Hub exhausted for this bot: decide next step.
    bool hasWorkableQuests = false;
    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;
        if (botAI->lowPriorityQuest.find(questId) != botAI->lowPriorityQuest.end())
            continue;
        uint8 status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_INCOMPLETE || status == QUEST_STATUS_COMPLETE)
        {
            hasWorkableQuests = true;
            break;
        }
    }

    if (!hasWorkableQuests)
    {
        info.exhaustedHubs.insert(data.hubId);
        info.activeHubId = 0;
        info.activeHubZone = 0;
        info.activeHubPos = WorldPosition();

        if (sPlayerbotAIConfig.newRpgIntentional)
        {
            RpgPersistedState state;
            state.goal     = 0;
            state.zoneId   = 0;
            state.hubMapId = 0;
            state.hubX     = 0.0f;
            state.hubY     = 0.0f;
            state.hubZ     = 0.0f;
            sPlayerbotRpgStateRepository.Save(bot->GetGUID().GetCounter(), state);
        }
    }

    LOG_DEBUG("playerbots", "[New RPG] {} hub {} done (workable={})", bot->GetName(), data.hubId, hasWorkableQuests);
    info.ChangeToIdle();
    return true;
}

bool NewRpgGoGrindAction::Execute(Event /*event*/)
{
    if (SearchQuestGiverAndAcceptOrReward())
        return true;
    if (auto* data = std::get_if<NewRpgInfo::GoGrind>(&botAI->rpgInfo.data))
    {
        if (MoveFarTo(data->pos))
            return true;
        // Small nudge so the next tick's MoveFarTo starts from a
        // slightly different position. Kept small so it doesn't look
        // like the bot is abandoning its destination.
        return MoveRandomNear(10.0f);
    }

    return false;
}

bool NewRpgGoCampAction::Execute(Event /*event*/)
{
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    if (auto* data = std::get_if<NewRpgInfo::GoCamp>(&botAI->rpgInfo.data))
    {
        if (MoveFarTo(data->pos))
            return true;
        return MoveRandomNear(10.0f);
    }

    return false;
}

bool NewRpgWanderRandomAction::Execute(Event /*event*/)
{
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    return MoveRandomNear();
}

bool NewRpgWanderNpcAction::Execute(Event /*event*/)
{
    NewRpgInfo& info = botAI->rpgInfo;
    auto* dataPtr = std::get_if<NewRpgInfo::WanderNpc>(&info.data);
    if (!dataPtr)
        return false;
    auto& data = *dataPtr;
    if (!data.npcOrGo)
    {
        // No npc can be found, switch to IDLE
        ObjectGuid npcOrGo = ChooseNpcOrGameObjectToInteract();
        if (npcOrGo.IsEmpty())
        {
            info.ChangeToIdle();
            return true;
        }
        data.npcOrGo = npcOrGo;
        data.lastReach = 0;
        return true;
    }

    WorldObject* object = ObjectAccessor::GetWorldObject(*bot, data.npcOrGo);
    if (object && IsWithinInteractionDist(object))
    {
        if (!data.lastReach)
        {
            data.lastReach = getMSTime();
            if (bot->CanInteractWithQuestGiver(object))
                InteractWithNpcOrGameObjectForQuest(data.npcOrGo);
            return true;
        }

        if (data.lastReach && GetMSTimeDiffToNow(data.lastReach) < npcStayTime)
            return false;

        // has reached the npc for more than `npcStayTime`, select the next target
        data.npcOrGo = ObjectGuid();
        data.lastReach = 0;
    }
    else
    {
        if (MoveWorldObjectTo(data.npcOrGo))
            return true;
        // NPC pathing failed (random offset in a wall, mmap hiccup, etc).
        // Take a small random step so the next tick retries from a
        // different spot instead of staring at the NPC from afar.
        return MoveRandomNear(15.0f);
    }

    return true;
}

bool NewRpgDoQuestAction::Execute(Event /*event*/)
{
    if (SearchQuestGiverAndAcceptOrReward())
        return true;

    NewRpgInfo& info = botAI->rpgInfo;
    auto* dataPtr = std::get_if<NewRpgInfo::DoQuest>(&info.data);
    if (!dataPtr)
        return false;
    auto& data = *dataPtr;
    uint32 questId = data.questId;
    uint8 questStatus = bot->GetQuestStatus(questId);
    switch (questStatus)
    {
        case QUEST_STATUS_INCOMPLETE:
            return DoIncompleteQuest(data);
        case QUEST_STATUS_COMPLETE:
            return DoCompletedQuest(data);
        default:
            break;
    }
    info.ChangeToIdle();
    return true;
}

bool NewRpgDoQuestAction::DoIncompleteQuest(NewRpgInfo::DoQuest& data)
{
    uint32 questId = data.questId;
    if (data.pos != WorldPosition())
    {
        int32 currentObjective = data.objectiveIdx;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        QuestStatusData const& q_status = bot->getQuestStatusMap().at(questId);
        bool completed = true;
        if (currentObjective < QUEST_OBJECTIVES_COUNT)
        {
            if (q_status.CreatureOrGOCount[currentObjective] < quest->RequiredNpcOrGoCount[currentObjective])
                completed = false;
        }
        else if (currentObjective < QUEST_OBJECTIVES_COUNT + QUEST_ITEM_OBJECTIVES_COUNT)
        {
            if (q_status.ItemCount[currentObjective - QUEST_OBJECTIVES_COUNT] <
                quest->RequiredItemCount[currentObjective - QUEST_OBJECTIVES_COUNT])
                completed = false;
        }
        if (completed)
        {
            if (sPlayerbotAIConfig.newRpgIntentional)
            {
                // Cross-quest nearest-first: switch to the closest remaining
                // objective across all log quests so overlapping camps are
                // worked together without returning to IDLE.
                uint32 nextQuestId = 0;
                POIInfo nextPoi{};
                if (SelectNextQuestObjective(nextQuestId, nextPoi))
                {
                    float dx = nextPoi.pos.x, dy = nextPoi.pos.y;
                    float dz = std::max(bot->GetMap()->GetHeight(dx, dy, MAX_HEIGHT),
                                        bot->GetMap()->GetWaterLevel(dx, dy));
                    if (dz != INVALID_HEIGHT && dz != VMAP_INVALID_HEIGHT_VALUE)
                    {
                        if (nextQuestId != questId)
                        {
                            Quest const* nextQuest = sObjectMgr->GetQuestTemplate(nextQuestId);
                            if (nextQuest)
                            {
                                data.quest   = nextQuest;
                                data.questId = nextQuestId;
                            }
                            else
                            {
                                nextQuestId = questId;
                            }
                        }
                        data.lastReachPOI       = 0;
                        data.pos                = WorldPosition(bot->GetMapId(), dx, dy, dz);
                        data.objectiveIdx       = nextPoi.objectiveIdx;
                        data.patrolPoints       = nextPoi.points;
                        data.patrolIdx          = 0;
                        data.lastPatrolMoveT    = 0;
                        data.lastProgressT      = 0;
                        data.lastLivenessCheckT = 0;
                        data.progressSnapshot   = 0;
                        return true;
                    }
                }
                botAI->rpgInfo.ChangeToIdle();
                return true;
            }
            // Legacy: clear and re-select within same quest next tick.
            data.lastReachPOI = 0;
            data.pos          = WorldPosition();
            data.objectiveIdx = 0;
        }
    }

    if (data.pos == WorldPosition())
    {
        if (sPlayerbotAIConfig.newRpgIntentional)
        {
            uint32 nextQuestId = 0;
            POIInfo nextPoi{};
            if (!SelectNextQuestObjective(nextQuestId, nextPoi))
            {
                botAI->rpgInfo.ChangeToIdle();
                return true;
            }
            float dx = nextPoi.pos.x, dy = nextPoi.pos.y;
            float dz = std::max(bot->GetMap()->GetHeight(dx, dy, MAX_HEIGHT),
                                bot->GetMap()->GetWaterLevel(dx, dy));
            if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
                return false;

            if (nextQuestId != questId)
            {
                Quest const* nextQuest = sObjectMgr->GetQuestTemplate(nextQuestId);
                if (nextQuest)
                {
                    data.quest   = nextQuest;
                    data.questId = nextQuestId;
                    questId      = nextQuestId;
                }
            }
            data.lastReachPOI       = 0;
            data.pos                = WorldPosition(bot->GetMapId(), dx, dy, dz);
            data.objectiveIdx       = nextPoi.objectiveIdx;
            data.patrolPoints       = nextPoi.points;
            data.patrolIdx          = 0;
            data.lastPatrolMoveT    = 0;
            data.lastProgressT      = 0;
            data.lastLivenessCheckT = 0;
            data.progressSnapshot   = 0;
        }
        else
        {
            std::vector<POIInfo> poiInfo;
            if (!GetQuestPOIPosAndObjectiveIdx(questId, poiInfo))
            {
                botAI->rpgInfo.ChangeToIdle();
                return true;
            }
            uint32 rndIdx           = urand(0, poiInfo.size() - 1);
            G3D::Vector2 nearestPoi = poiInfo[rndIdx].pos;
            int32 objectiveIdx      = poiInfo[rndIdx].objectiveIdx;
            float dx = nearestPoi.x, dy = nearestPoi.y;
            float dz = std::max(bot->GetMap()->GetHeight(dx, dy, MAX_HEIGHT),
                                bot->GetMap()->GetWaterLevel(dx, dy));
            if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
                return false;
            data.lastReachPOI = 0;
            data.pos          = WorldPosition(bot->GetMapId(), dx, dy, dz);
            data.objectiveIdx = objectiveIdx;
        }
    }

    if (bot->GetDistance(data.pos) > 10.0f && !data.lastReachPOI)
    {
        if (MoveFarTo(data.pos))
            return true;
        // Long-range sampler couldn't land a candidate — nudge the
        // bot a short distance so the next tick retries from a
        // different position instead of sitting idle.
        return MoveRandomNear(10.0f);
    }
    // Now we are near the quest objective.
    // kill mobs and looting quest should be done automatically by grind strategy

    if (!data.lastReachPOI)
    {
        data.lastReachPOI = getMSTime();
        if (sPlayerbotAIConfig.newRpgIntentional)
        {
            data.lastProgressT      = getMSTime();
            data.lastLivenessCheckT = getMSTime();
            Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
            QuestStatusData const& q_status = bot->getQuestStatusMap().at(questId);
            int32 const obj = data.objectiveIdx;
            if (obj < QUEST_OBJECTIVES_COUNT)
                data.progressSnapshot = q_status.CreatureOrGOCount[obj];
            else if (obj < QUEST_OBJECTIVES_COUNT + QUEST_ITEM_OBJECTIVES_COUNT)
                data.progressSnapshot = q_status.ItemCount[obj - QUEST_OBJECTIVES_COUNT];
        }
        return true;
    }

    uint32 const timeAtPOI = GetMSTimeDiffToNow(data.lastReachPOI);

    if (sPlayerbotAIConfig.newRpgIntentional)
    {
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        QuestStatusData const& q_status = bot->getQuestStatusMap().at(questId);
        int32 const obj = data.objectiveIdx;

        // --- Throttled progress tracking (every 15 s) ---
        constexpr uint32 progressCheckInterval  = 15 * IN_MILLISECONDS;
        constexpr uint32 earlyAbandonNoProgressT = 2 * MINUTE * IN_MILLISECONDS;

        if (GetMSTimeDiffToNow(data.lastLivenessCheckT) >= progressCheckInterval)
        {
            data.lastLivenessCheckT = getMSTime();

            uint32 currentCount = 0;
            if (obj < QUEST_OBJECTIVES_COUNT)
                currentCount = q_status.CreatureOrGOCount[obj];
            else if (obj < QUEST_OBJECTIVES_COUNT + QUEST_ITEM_OBJECTIVES_COUNT)
                currentCount = q_status.ItemCount[obj - QUEST_OBJECTIVES_COUNT];

            if (currentCount > data.progressSnapshot)
            {
                data.progressSnapshot = currentCount;
                data.lastProgressT    = getMSTime();
            }

            // Early abandon: no progress for 120 s AND entity verifiably absent
            // (creature/GO objectives only; items use the flat timer).
            if (obj < QUEST_OBJECTIVES_COUNT &&
                GetMSTimeDiffToNow(data.lastProgressT) >= earlyAbandonNoProgressT)
            {
                int32 const required = quest->RequiredNpcOrGo[obj];
                bool entityAbsent = false;
                if (required > 0)
                {
                    Creature* c = bot->FindNearestCreature(static_cast<uint32>(required), 100.0f, true);
                    entityAbsent = (c == nullptr);
                }
                else if (required < 0)
                {
                    GameObject* go = bot->FindNearestGameObject(static_cast<uint32>(-required), 100.0f);
                    entityAbsent = (go == nullptr);
                }

                if (entityAbsent)
                {
                    botAI->lowPriorityQuest.insert(questId);
                    botAI->rpgStatistic.questAbandoned++;
                    LOG_DEBUG("playerbots", "[New RPG] {} marked as abandoned quest {} (entity absent, no progress)",
                              bot->GetName(), questId);
                    botAI->rpgInfo.ChangeToIdle();
                    return true;
                }
            }
        }

        // --- Hard cap / productive-stay extension ---
        bool const recentProgress = GetMSTimeDiffToNow(data.lastProgressT) < (60 * IN_MILLISECONDS);

        if (timeAtPOI >= poiStayTime)
        {
            if (recentProgress && timeAtPOI < poiStayTimeHardCap)
            {
                // Still productive — allow staying up to hard cap; fall through to patrol.
            }
            else if (timeAtPOI >= poiStayTimeHardCap)
            {
                // Hard cap reached: unconditionally re-select POI.
                data.lastReachPOI       = 0;
                data.pos                = WorldPosition();
                data.objectiveIdx       = 0;
                data.patrolPoints.clear();
                data.patrolIdx          = 0;
                data.lastPatrolMoveT    = 0;
                data.lastProgressT      = 0;
                data.lastLivenessCheckT = 0;
                data.progressSnapshot   = 0;
                return true;
            }
            else
            {
                // Past soft cap, no recent progress: check entity presence.
                bool hasProgression = false;
                if (obj < QUEST_OBJECTIVES_COUNT)
                {
                    if (q_status.CreatureOrGOCount[obj] != 0 && quest->RequiredNpcOrGoCount[obj])
                        hasProgression = true;
                }
                else if (obj < QUEST_OBJECTIVES_COUNT + QUEST_ITEM_OBJECTIVES_COUNT)
                {
                    if (q_status.ItemCount[obj - QUEST_OBJECTIVES_COUNT] != 0 &&
                        quest->RequiredItemCount[obj - QUEST_OBJECTIVES_COUNT])
                        hasProgression = true;
                }

                if (!hasProgression)
                {
                    botAI->lowPriorityQuest.insert(questId);
                    botAI->rpgStatistic.questAbandoned++;
                    LOG_DEBUG("playerbots", "[New RPG] {} marked as abandoned quest {}", bot->GetName(), questId);
                    botAI->rpgInfo.ChangeToIdle();
                    return true;
                }
                // Entities present but competition: re-select POI.
                data.lastReachPOI       = 0;
                data.pos                = WorldPosition();
                data.objectiveIdx       = 0;
                data.patrolPoints.clear();
                data.patrolIdx          = 0;
                data.lastPatrolMoveT    = 0;
                data.lastProgressT      = 0;
                data.lastLivenessCheckT = 0;
                data.progressSnapshot   = 0;
                return true;
            }
        }

        // --- Polygon patrol ---
        if (data.patrolPoints.size() >= 2)
        {
            constexpr uint32 patrolDwellMin = 3 * IN_MILLISECONDS;
            constexpr uint32 patrolDwellMax = 6 * IN_MILLISECONDS;

            if (!data.lastPatrolMoveT)
            {
                data.lastPatrolMoveT = getMSTime();
                // Roll dwell once on arrival at a waypoint so the threshold is
                // stable for the whole dwell period (re-rolling each tick biases
                // toward the minimum).
                data.patrolDwellMs = patrolDwellMin + urand(0, patrolDwellMax - patrolDwellMin);
            }

            if (GetMSTimeDiffToNow(data.lastPatrolMoveT) < data.patrolDwellMs)
                return false;

            // Advance to next patrol point, wrapping around.
            // patrolIdx is uint32 to avoid uint8 overflow on large patrol sets.
            data.patrolIdx = (data.patrolIdx + 1) % data.patrolPoints.size();
            auto const& pt = data.patrolPoints[data.patrolIdx];
            float const px = pt.first;
            float const py = pt.second;
            float const pz = std::max(bot->GetMap()->GetHeight(px, py, MAX_HEIGHT),
                                      bot->GetMap()->GetWaterLevel(px, py));

            if (pz == INVALID_HEIGHT || pz == VMAP_INVALID_HEIGHT_VALUE)
            {
                // Skip invalid point; retry next tick.
                data.lastPatrolMoveT = getMSTime();
                return false;
            }

            data.lastPatrolMoveT = getMSTime();
            data.patrolDwellMs = patrolDwellMin + urand(0, patrolDwellMax - patrolDwellMin);
            return MoveTo(bot->GetMapId(), px, py, pz, false, false, false, true);
        }

        // Fewer than 2 patrol points — small wander as before.
        return MoveRandomNear(8.0f);
    }

    // Legacy path: flat 5-minute logic.
    if (timeAtPOI >= poiStayTime)
    {
        bool hasProgression = false;
        int32 currentObjective = data.objectiveIdx;
        Quest const* quest     = sObjectMgr->GetQuestTemplate(questId);
        QuestStatusData const& q_status = bot->getQuestStatusMap().at(questId);
        if (currentObjective < QUEST_OBJECTIVES_COUNT)
        {
            if (q_status.CreatureOrGOCount[currentObjective] != 0 && quest->RequiredNpcOrGoCount[currentObjective])
                hasProgression = true;
        }
        else if (currentObjective < QUEST_OBJECTIVES_COUNT + QUEST_ITEM_OBJECTIVES_COUNT)
        {
            if (q_status.ItemCount[currentObjective - QUEST_OBJECTIVES_COUNT] != 0 &&
                quest->RequiredItemCount[currentObjective - QUEST_OBJECTIVES_COUNT])
                hasProgression = true;
        }
        if (!hasProgression)
        {
            // reached the poi for more than 5 mins but no progression
            /// @TODO: It may be better to make lowPriorityQuest a global set shared by all bots (or saved in db)
            botAI->lowPriorityQuest.insert(questId);
            botAI->rpgStatistic.questAbandoned++;
            LOG_DEBUG("playerbots", "[New RPG] {} marked as abandoned quest {}", bot->GetName(), questId);
            botAI->rpgInfo.ChangeToIdle();
            return true;
        }
        // clear and select another poi later
        data.lastReachPOI = 0;
        data.pos          = WorldPosition();
        data.objectiveIdx = 0;
        return true;
    }

    // At the POI: keep the bot actively placed but avoid large
    // random 20yd hops that look like pacing back and forth. A small
    // ~8yd wander reads as the bot looking around while grind/loot
    // strategies do their work.
    return MoveRandomNear(8.0f);
}

bool NewRpgDoQuestAction::DoCompletedQuest(NewRpgInfo::DoQuest& data)
{
    uint32 questId = data.questId;
    const Quest* quest = data.quest;

    if (data.objectiveIdx != -1)
    {
        // if quest is completed, back to poi with -1 idx to reward
        BroadcastHelper::BroadcastQuestUpdateComplete(botAI, bot, quest);
        botAI->rpgStatistic.questCompleted++;
        std::vector<POIInfo> poiInfo;
        if (!GetQuestPOIPosAndObjectiveIdx(questId, poiInfo, true))
        {
            // can't find a poi pos to reward, stop doing quest for now
            botAI->rpgInfo.ChangeToIdle();
            return false;
        }
        assert(poiInfo.size() > 0);
        // now we get the place to get rewarded
        float dx = poiInfo[0].pos.x, dy = poiInfo[0].pos.y;
        // z = MAX_HEIGHT as we do not know accurate z
        float dz = std::max(bot->GetMap()->GetHeight(dx, dy, MAX_HEIGHT), bot->GetMap()->GetWaterLevel(dx, dy));

        // double check for GetQuestPOIPosAndObjectiveIdx
        if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
            return false;

        WorldPosition pos(bot->GetMapId(), dx, dy, dz);
        data.lastReachPOI = 0;
        data.pos = pos;
        data.objectiveIdx = -1;
    }

    if (data.pos == WorldPosition())
        return false;

    if (bot->GetDistance(data.pos) > 10.0f && !data.lastReachPOI)
    {
        if (MoveFarTo(data.pos))
            return true;
        return MoveRandomNear(10.0f);
    }

    // Now we are near the qoi of reward
    // the quest should be rewarded by SearchQuestGiverAndAcceptOrReward
    if (!data.lastReachPOI)
    {
        data.lastReachPOI = getMSTime();
        return true;
    }
    // stayed at this POI for more than 5 minutes
    if (GetMSTimeDiffToNow(data.lastReachPOI) >= poiStayTime)
    {
        // e.g. Can not reward quest to gameobjects
        /// @TODO: It may be better to make lowPriorityQuest a global set shared by all bots (or saved in db)
        botAI->lowPriorityQuest.insert(questId);
        botAI->rpgStatistic.questAbandoned++;
        LOG_DEBUG("playerbots", "[New RPG] {} marked as abandoned quest {}", bot->GetName(), questId);
        botAI->rpgInfo.ChangeToIdle();
        return true;
    }
    return false;
}

bool NewRpgTravelFlightAction::Execute(Event /*event*/)
{
    NewRpgInfo& info = botAI->rpgInfo;
    auto* dataPtr = std::get_if<NewRpgInfo::TravelFlight>(&info.data);
    if (!dataPtr)
        return false;

    auto& data = *dataPtr;
    if (bot->IsInFlight())
    {
        data.inFlight = true;
        return false;
    }

    if (bot->GetDistance(data.flightMasterPos) > INTERACTION_DISTANCE)
        return MoveFarTo(data.flightMasterPos);

    Creature* flightMaster = bot->FindNearestCreature(data.flightMasterEntry, INTERACTION_DISTANCE * 3);
    if (!flightMaster || !flightMaster->IsAlive())
    {
        info.ChangeToIdle();
        return true;
    }

    std::vector<uint32> nodes = data.path;

    botAI->RemoveShapeshift();
    if (bot->IsMounted())
        bot->Dismount();

    if (!bot->ActivateTaxiPathTo(nodes, flightMaster, 0))
    {
        LOG_DEBUG("playerbots", "[New RPG] {} active taxi path {} (from {} to {}) failed", bot->GetName(),
                  flightMaster->GetEntry(), nodes[0], nodes[nodes.size() - 1]);
        info.ChangeToIdle();
        return true;
    }
    return true;
}
bool NewRpgGoCityAction::Execute(Event /*event*/)
{
    auto* dataPtr = std::get_if<NewRpgInfo::GoCity>(&botAI->rpgInfo.data);
    if (!dataPtr)
        return false;
    auto& data = *dataPtr;

    if (bot->IsInFlight())
        return false;
    //Get new target if we dont have one.
    if (data.currentTaskLocation == WorldPosition())
    {
        if (data.taskList.empty())
        {
            botAI->rpgInfo.ChangeToIdle();
            return true;
        }
        NewRpgInfo::CityTask next = std::move(data.taskList.front());
        data.taskList.erase(data.taskList.begin());
        data.currentTaskKind = next.kind;
        data.currentTaskNpc = next.npc;
        data.currentTaskLocation = next.location;
        data.currentTaskAttempts = 0;
    }

    if (bot->GetDistance(data.currentTaskLocation) > INTERACTION_DISTANCE)
            return MoveFarTo(data.currentTaskLocation);

    // stillWorking runs until task state returns false indicating its done doing what it had to do.
    bool stillWorking = true;
    switch (data.currentTaskKind)
    {
        case NewRpgInfo::CityTaskType::Visit:
            // Empty handler: arriving at the location is the
            // whole job. Fall through to promote next.
            stillWorking = false;
        break;
        case NewRpgInfo::CityTaskType::Auctioneer:
            stillWorking = ExecuteAuctioneerTask(data);
            break;
        case NewRpgInfo::CityTaskType::Vendor:
            stillWorking = ExecuteVendorTask(data);
            break;
        case NewRpgInfo::CityTaskType::RepairVendor:
            stillWorking = ExecuteRepairTask(data);
            break;
        case NewRpgInfo::CityTaskType::Trainer:
            stillWorking = ExecuteTrainerTask(data);
            break;
        case NewRpgInfo::CityTaskType::Innkeeper:
            stillWorking = ExecuteInnkeeperTask(data);
            break;
        // Handle all other cases.
        default:
            stillWorking = false;
            break;
    }

    if (stillWorking)
        return true;

    // Erase Current task location and npc, on next check it will  empty so the next loop iteration promotes the next task (or exits to Idle if the list is dry).
    data.currentTaskLocation = WorldPosition();
    data.currentTaskNpc = ObjectGuid();
    return true;
}

bool NewRpgGoCityAction::ExecuteAuctioneerTask(NewRpgInfo::GoCity& data)
{
    if (!sPlayerbotAIConfig.enableAuctionHouseBotting)
        return false;

    Creature* auctioneer = ai::npc::FindNpcByFlag(
        bot, UNIT_NPC_FLAG_AUCTIONEER,
        AI_VALUE(GuidVector, "possible new rpg targets"),
        data.currentTaskNpc);
    if (!auctioneer)
        return false;

    auto& sellList = AI_VALUE(AhListMap&, "ah sell list");
    time_t now = time(nullptr);

    // Drop Failed entries as we go — reconcile will re-insert them as Idle
    // from inventory next pass if the item is still bag-side. First non-Failed
    // entry hands off to AhSellAction and ends this tick.
    for (auto it = sellList.begin(); it != sellList.end(); )
    {
        if (it->second.status == AhStatus::Failed)
        {
            it = sellList.erase(it);
            continue;
        }
        botAI->DoSpecificAction("ah sell", Event(), true);
        return true;
    }

    // Advance state and fire one query per Idle slot. Stuck PendingCheck
    // (>10s) is cooled down to recover from lost response packets.
    auto& buyList = AI_VALUE(AhListMap&, "ah buy list");
    bool pendingInFlight = false;
    for (auto& buyKeyValue : buyList)
    {
        AhItemState& ahItemState = buyKeyValue.second;

        if (ahItemState.status == AhStatus::PendingCheck)
        {
            if (now - ahItemState.changedAt > AH_PENDING_CHECK_TIMEOUT_SECONDS)
            {
                ahItemState.status = AhStatus::Failed;
                ahItemState.changedAt = now;
                ahItemState.retryAfter = now + AH_FAILED_BACKOFF_SECONDS;
            }
            else
                pendingInFlight = true;

            continue;
        }
        if (ahItemState.status == AhStatus::Watch || ahItemState.status == AhStatus::Complete)
            continue;

        // If, somehow, the bot is still here after the cooldown, let us re-check.
        if (ahItemState.status == AhStatus::Failed && now >= ahItemState.retryAfter)
        {
            ahItemState.status = AhStatus::Idle;
            ahItemState.retryAfter = 0;
        }

        if (ahItemState.status == AhStatus::Idle)
        {
            uint8 targetSlot = buyKeyValue.first;
            BotAuctionUtils::SendAhSearchForSlot(bot, auctioneer, targetSlot);
            AhItemState& st = buyList[targetSlot];
            st.status = AhStatus::PendingCheck;
            st.changedAt = now;
            return true;
        }
    }

    if (pendingInFlight)
        return true;

    // Nothing actionable left for the auctioneer this trip.
    return false;
}

bool NewRpgGoCityAction::ExecuteVendorTask(NewRpgInfo::GoCity& data)
{
    // SellAction self-resolves an in-range vendor from "nearest npcs" and offloads
    // junk to free bag space. One-shot, bounded-retry if not in range yet.
    return RetryIfNotDone(data, botAI->DoSpecificAction("sell", Event("rpg action", "vendor"), true));
}

bool NewRpgGoCityAction::ExecuteRepairTask(NewRpgInfo::GoCity& data)
{
    // RepairAllAction self-resolves an in-range repairer.
    return RetryIfNotDone(data, botAI->DoSpecificAction("repair", Event(), true));
}

bool NewRpgGoCityAction::ExecuteTrainerTask(NewRpgInfo::GoCity& data)
{
    // TrainerAction self-resolves a valid (own-class) nearby trainer and learns
    // directly onto the bot, so no caller-side selection is needed.
    return RetryIfNotDone(data, botAI->DoSpecificAction("trainer", Event(), true));
}

bool NewRpgGoCityAction::ExecuteInnkeeperTask(NewRpgInfo::GoCity& data)
{
    // SetHomeAction ("home") self-resolves the nearest interactable innkeeper and
    // binds the hearth, so we route through it instead of a raw SendBindPoint.
    return RetryIfNotDone(data, botAI->DoSpecificAction("home", Event(), true));
}

bool NewRpgGoCityAction::RetryIfNotDone(NewRpgInfo::GoCity& data, bool actionRan)
{
    // stillWorking semantics: true => keep this task; false => promote the next.
    if (actionRan)
    {
        data.currentTaskAttempts = 0;
        return false;
    }

    constexpr uint32 maxCityTaskAttempts = 5;
    return ++data.currentTaskAttempts < maxCityTaskAttempts;
}
