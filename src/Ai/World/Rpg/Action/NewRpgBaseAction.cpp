#include "NewRpgBaseAction.h"

#include <limits>
#include <sstream>

#include "BroadcastHelper.h"
#include "ChatHelper.h"
#include "Creature.h"
#include "G3D/Vector2.h"
#include "GameObject.h"
#include "GossipDef.h"
#include "GridTerrainData.h"
#include "IVMapMgr.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "LootMgr.h"
#include "Map.h"
#include "ModelIgnoreFlags.h"
#include "MotionMaster.h"
#include "MoveSplineInitArgs.h"
#include "NewRpgInfo.h"
#include "NewRpgStrategy.h"
#include "Object.h"
#include "ObjectAccessor.h"
#include "OutdoorPvPMgr.h"
#include "ObjectDefines.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "PathGenerator.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "Position.h"
#include "QuestDef.h"
#include "Random.h"
#include "RandomPlayerbotMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StatsWeightCalculator.h"
#include "Timer.h"
#include "TravelMgr.h"


bool NewRpgBaseAction::MoveFarTo(WorldPosition dest)
{
    if (dest == WorldPosition())
    {
        EmitDebugMove("MoveFar", "empty-dest", 0.0f, 0.0f, 0.0f);
        return false;
    }

    UpdateMovementState();

    if (!IsMovingAllowed())
    {
        EmitDebugMove("MoveFar", "cant-move",
                      dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
        return false;
    }

    // Resume a transport ride if we're still on the same boat as last tick.
    if (WaitForTransport())
        return true;

    WorldPosition botPos(bot);
    LastMovement& lastMove = AI_VALUE(LastMovement&, "last movement");

    // Short-stop: at destination — stop and clear the cached path.
    {
        float const totalDistance = botPos.distance(dest);
        if (totalDistance < sPlayerbotAIConfig.targetPosRecalcDistance)
        {
            if (!lastMove.lastPath.empty() &&
                lastMove.lastPath.getBack().distance(dest) <= totalDistance)
                lastMove.clear();
            bot->StopMoving();
            EmitDebugMove("MoveFar", "arrived",
                          dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
            return false;
        }
    }

    // Per-tick re-resolve: rebuild the TravelPath from the bot's current
    // position every tick (10% reuse short-circuits via the cached
    // lastPath). Recovers naturally from knockback, off-route drift,
    // destination changes, and blocked waypoints.
    TravelPath path = ResolveMovePath(botPos, dest, lastMove);
    lastMove.setPath(path);

    if (path.empty())
    {
        EmitDebugMove("MoveFar", "no-path",
                      dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
        return false;
    }

    // Trim leading waypoints behind the bot, bridge with mmap probe if
    // the new head requires it. May empty the path (collapsed) — let
    // the next tick rebuild from a fresh start.
    path.makeShortCut(botPos, sPlayerbotAIConfig.reactDistance, bot);
    if (path.empty())
        return true;

    // Special head segment (portal / area-trigger / transport / flight)?
    // UpcommingSpecialMovement cuts the path so the head is the special;
    // HandleSpecialMovement dispatches the matching action.
    bool const onTransport = bot->GetTransport() != nullptr;
    if (path.UpcommingSpecialMovement(botPos,
                                      sPlayerbotAIConfig.reactDistance,
                                      onTransport))
    {
        if (HandleSpecialMovement(path))
            return true;
        // Special handler declined (e.g. AREA_TRIGGER with entry → caller
        // dispatches the walk into the trigger volume). Fall through.
    }

    // Transport guard: bot is on a transport but no special movement
    // applies this tick — don't dispatch a walk spline (would fight the
    // transport's own movement).
    if (onTransport)
        return false;

    // ClipPath — truncate at first hostile creature in range / non-walkable
    // hop / drifted past reactDistance / > 125 sqDist jump.
    path.ClipPath(botAI, bot, false);
    if (path.empty())
        return false;

    // Walk dispatch.
    std::vector<WorldPosition> const& pts = path.getPointPath();
    Movement::PointsArray points;
    points.reserve(pts.size());
    for (auto const& wp : pts)
        points.emplace_back(wp.GetPositionX(), wp.GetPositionY(), wp.GetPositionZ());

    if (points.size() < 2)
    {
        // Single-point fallback path (cmangos pattern: ResolveMovePath
        // emits a single dest point if nothing else worked). Hand it
        // to the engine's MovePoint via MoveTo.
        EmitDebugMove("MoveFar", "single-point",
                      dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ());
        return MoveTo(dest.GetMapId(), dest.GetPositionX(),
                      dest.GetPositionY(), dest.GetPositionZ(),
                      false, false, false, false);
    }

    if (!bot->IsMounted() && !bot->IsInCombat() &&
        bot->IsOutdoors() && bot->IsAlive())
        botAI->DoSpecificAction("check mount state", Event(), true);

    return DispatchPathPoints(dest, points, "walk");
}

bool NewRpgBaseAction::DispatchPathPoints(WorldPosition const& dest,
                                          Movement::PointsArray& points,
                                          char const* label)
{
    if (points.size() < 2)
        return false;

    // MoveFarTo runs makeShortCut + setPath upstream now, so no need
    // for the local prefix-trim or lastMove.setPath here.

    for (auto& pt : points)
        bot->UpdateAllowedPositionZ(pt.x, pt.y, pt.z);

    // ClipPath now runs at MoveFarTo level on the TravelPath before the
    // points array is built. No per-dispatch clip here.

    if (points.size() < 2)
        return false;

    G3D::Vector3 const& last = points.back();

    float totalDist = 0.f;
    for (size_t i = 1; i < points.size(); ++i)
        totalDist += (points[i] - points[i - 1]).length();

    LastMovement& lastMove = AI_VALUE(LastMovement&, "last movement");

    // Skip cosmetic walking for random bots with no nearby player —
    // teleport to the path tail and schedule a cooldown instead.
    if (sRandomPlayerbotMgr.IsRandomBot(bot))
    {
        WorldPosition tail(dest.GetMapId(), last.x, last.y, last.z);
        time_t now = time(nullptr);
        if (totalDist > sPlayerbotAIConfig.reactDistance &&
            lastMove.nextTeleport <= now &&
            !botAI->HasPlayerNearby(&tail))
        {
            float speed = std::max(bot->GetSpeed(MOVE_RUN), 0.1f);
            lastMove.nextTeleport = now + (time_t)(totalDist / speed);

            EmitDebugMove("MoveFar", "teleport",
                          tail.GetPositionX(), tail.GetPositionY(), tail.GetPositionZ());

            WorldPosition botPos(bot);
            return bot->TeleportTo(dest.GetMapId(),
                                   tail.GetPositionX(), tail.GetPositionY(),
                                   tail.GetPositionZ(),
                                   botPos.getAngleTo(tail));
        }
    }

    // Match master's walk pace when they're walking and within 5y.
    ForcedMovement moveMode = FORCED_MOVEMENT_RUN;
    if (Player* master = botAI->GetMaster())
    {
        if (bot->IsFriendlyTo(master) && master->IsWalking() &&
            bot->GetExactDist2d(master) < 5.0f)
        {
            moveMode = FORCED_MOVEMENT_WALK;
        }
    }

    // Clear emote/sit/cast so the spline can begin cleanly.
    bot->ClearEmoteState();
    if (!bot->IsStandState())
        bot->SetStandState(UNIT_STAND_STATE_STAND);
    if (bot->IsNonMeleeSpellCast(true))
        bot->InterruptNonMeleeSpells(true);

    bot->GetMotionMaster()->Clear();
    bot->GetMotionMaster()->MoveSplinePath(&points, moveMode);

    EmitDebugMove("MoveFar", label, last.x, last.y, last.z);

    // WaitForReach: leave ~10y headroom on long paths.
    float waitDist = totalDist > sPlayerbotAIConfig.reactDistance
                         ? std::max(totalDist - 10.0f, 0.0f) : totalDist;
    UnitMoveType const speedType = (moveMode == FORCED_MOVEMENT_WALK) ? MOVE_WALK : MOVE_RUN;
    float speed = std::max(bot->GetSpeed(speedType), 0.1f);
    float duration = 1000.0f * (waitDist / speed) + sPlayerbotAIConfig.reactDelay;
    duration = std::min(duration, (float)sPlayerbotAIConfig.maxWaitForMove);
    if (duration < 0.0f)
        duration = 0.0f;

    lastMove.Set(bot->GetMapId(), last.x, last.y, last.z,
                 bot->GetOrientation(), (uint32)duration,
                 MovementPriority::MOVEMENT_NORMAL);

    return true;
}

bool NewRpgBaseAction::MoveWorldObjectTo(ObjectGuid guid, float distance)
{
    WorldObject* object = botAI->GetWorldObject(guid);
    if (!object)
        return false;

    Map* map = bot->GetMap();
    if (!map)
        return false;

    // 8 angles around the target starting at the bot's preferred follow
    // angle (group-aware spread). For each angle, ask the engine for a
    // valid nearby ground point at the requested distance — that snaps
    // to terrain/collision. LOS check ignores M2 models so long-distance
    // NPCs through forested terrain still pass; the mmap probe in
    // MoveFarTo is the authoritative reachability check.
    float const followAngle = GetFollowAngle();
    float const searchSize = bot->GetObjectSize();

    for (float step = 0.0f; step < 2.0f * static_cast<float>(M_PI);
         step += static_cast<float>(M_PI) / 4.0f)
    {
        float const angle = followAngle + step;
        float x = object->GetPositionX();
        float y = object->GetPositionY();
        float z = object->GetPositionZ();
        object->GetNearPoint(bot, x, y, z, searchSize, distance, angle);

        if (!bot->IsWithinLOS(x, y, z + bot->GetCollisionHeight(),
                              VMAP::ModelIgnoreFlags::M2))
            continue;

        return MoveFarTo(WorldPosition(object->GetMapId(), x, y, z));
    }

    return false;
}

bool NewRpgBaseAction::MoveRandomNear(float moveStep, MovementPriority priority, WorldObject* center)
{
    float const distance = (0.4f + rand_norm() * 0.6f) * moveStep;
    float const angle = (float)rand_norm() * 2 * static_cast<float>(M_PI);
    float const dx = bot->GetPositionX() + distance * cos(angle);
    float const dy = bot->GetPositionY() + distance * sin(angle);
    float const dz = bot->GetPositionZ();

    bool moved = MoveTo(bot->GetMapId(), dx, dy, dz, false, false, false, true, priority);
    EmitDebugMove("MoveRandomNear", moved ? "mmap" : "fail", dx, dy, dz);
    return moved;
}

bool NewRpgBaseAction::ForceToWait(uint32 duration, MovementPriority priority)
{
    AI_VALUE(LastMovement&, "last movement")
        .Set(bot->GetMapId(), bot->GetPositionX(), bot->GetPositionY(), bot->GetPositionZ(), bot->GetOrientation(),
             duration, priority);
    return true;
}

bool NewRpgBaseAction::TakeFlight(std::vector<uint32> const& taxiNodes, Creature* flightMaster)
{
    if (taxiNodes.size() < 2 || !flightMaster || !flightMaster->IsAlive())
        return false;

    botAI->RemoveShapeshift();
    if (bot->IsMounted())
        bot->Dismount();

    if (!bot->ActivateTaxiPathTo(taxiNodes, flightMaster, 0))
    {
        LOG_DEBUG("playerbots", "[New RPG] Bot {} flight ({} nodes, {} to {}) failed",
                  bot->GetName(), taxiNodes.size(), taxiNodes.front(), taxiNodes.back());
        return false;
    }

    LOG_DEBUG("playerbots", "[New RPG] Bot {} taking flight ({} nodes, {} to {})",
              bot->GetName(), taxiNodes.size(), taxiNodes.front(), taxiNodes.back());
    EmitDebugMove("Flight:taxi", "taxi", flightMaster->GetPositionX(), flightMaster->GetPositionY(),
                  flightMaster->GetPositionZ());
    return true;
}

/// @TODO: Fix redundant code
/// Quest related method refer to TalkToQuestGiverAction.h
bool NewRpgBaseAction::InteractWithNpcOrGameObjectForQuest(ObjectGuid guid)
{
    WorldObject* object = ObjectAccessor::GetWorldObject(*bot, guid);
    if (!object || !bot->CanInteractWithQuestGiver(object))
        return false;

    // Creature* creature = bot->GetNPCIfCanInteractWith(guid, UNIT_NPC_FLAG_NONE);
    // if (creature)
    // {
    //     WorldPacket packet(CMSG_GOSSIP_HELLO);
    //     packet << guid;
    //     bot->GetSession()->HandleGossipHelloOpcode(packet);
    // }

    bot->PrepareQuestMenu(guid);
    const QuestMenu& menu = bot->PlayerTalkClass->GetQuestMenu();
    if (menu.Empty())
        return true;

    for (uint8 idx = 0; idx < menu.GetMenuItemCount(); idx++)
    {
        const QuestMenuItem& item = menu.GetItem(idx);
        const Quest* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
        if (!quest)
            continue;

        const QuestStatus& status = bot->GetQuestStatus(item.QuestId);
        if (status == QUEST_STATUS_NONE && bot->CanTakeQuest(quest, false) && bot->CanAddQuest(quest, false) &&
            IsQuestWorthDoing(quest) && IsQuestCapableDoing(quest))
        {
            AcceptQuest(quest, guid);
            if (botAI->GetMaster())
                botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "new_rpg_quest_accepted",
                    "Quest accepted %quest",
                    {{"%quest", ChatHelper::FormatQuest(quest)}}));
            BroadcastHelper::BroadcastQuestAccepted(botAI, bot, quest);
            botAI->rpgStatistic.questAccepted++;
            LOG_DEBUG("playerbots", "[New RPG] {} accept quest {}", bot->GetName(), quest->GetQuestId());
        }
        if (status == QUEST_STATUS_COMPLETE && bot->CanRewardQuest(quest, 0, false))
        {
            TurnInQuest(quest, guid);
            if (botAI->GetMaster())
                botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "new_rpg_quest_rewarded",
                    "Quest rewarded %quest",
                    {{"%quest", ChatHelper::FormatQuest(quest)}}));
            BroadcastHelper::BroadcastQuestTurnedIn(botAI, bot, quest);
            botAI->rpgStatistic.questRewarded++;
            LOG_DEBUG("playerbots", "[New RPG] {} turned in quest {}", bot->GetName(), quest->GetQuestId());
        }
    }
    return true;
}

bool NewRpgBaseAction::CanInteractWithQuestGiver(Object* questGiver)
{
    // This is a variant of Player::CanInteractWithQuestGiver
    // that removes the distance check and keeps all other checks
    switch (questGiver->GetTypeId())
    {
        case TYPEID_UNIT: // Player::GetNPCIfCanInteractWith
        {
            ObjectGuid guid = questGiver->GetGUID();

            // unit checks
            if (!guid)
                return false;

            if (!bot->IsInWorld() || bot->IsDuringRemoveFromWorld())
                return false;

            if (bot->IsInFlight())
                return false;

            // exist (we need look pets also for some interaction (quest/etc)
            Creature* creature = ObjectAccessor::GetCreatureOrPetOrVehicle(*bot, guid);
            if (!creature)
                return false;

            // Deathstate checks
            if (!bot->IsAlive() &&
                !(creature->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_VISIBLE_TO_GHOSTS))
                return false;

            // alive or spirit healer
            if (!creature->IsAlive() &&
                !(creature->GetCreatureTemplate()->type_flags & CREATURE_TYPE_FLAG_INTERACT_WHILE_DEAD))
                return false;

            // appropriate npc type
            if (!creature->HasNpcFlag(UNIT_NPC_FLAG_QUESTGIVER))
                return false;

            // not allow interaction under control, but allow with own pets
            if (creature->GetCharmerGUID())
                return false;

            // xinef: perform better check
            if (creature->GetReactionTo(bot) <= REP_UNFRIENDLY)
                return false;

            return true;
        }
        case TYPEID_GAMEOBJECT: // Player::GetGameObjectIfCanInteractWith
        {
            ObjectGuid guid = questGiver->GetGUID();

            if (GameObject* go = bot->GetMap()->GetGameObject(guid))
            {
                if (go->GetGoType() == GAMEOBJECT_TYPE_QUESTGIVER)
                {
                    // Players cannot interact with gameobjects that use the "Point" icon
                    if (go->GetGOInfo()->IconName == "Point")
                        return false;

                    return true;
                }
            }

            return false;
        }
        // unused for now
        // case TYPEID_PLAYER:
        //     return bot->IsAlive() && questGiver->ToPlayer()->IsAlive();
        // case TYPEID_ITEM:
        //     return bot->IsAlive();
        default:
            break;
    }
    return false;
}

bool NewRpgBaseAction::IsWithinInteractionDist(Object* questGiver)
{
    // This is a variant of Player::CanInteractWithQuestGiver
    // that only keep the distance check
    switch (questGiver->GetTypeId())
    {
        case TYPEID_UNIT:
        {
            ObjectGuid guid = questGiver->GetGUID();
            // unit checks
            if (!guid)
                return false;

            // exist (we need look pets also for some interaction (quest/etc)
            Creature* creature = ObjectAccessor::GetCreatureOrPetOrVehicle(*bot, guid);
            if (!creature)
                return false;

            if (!creature->IsWithinDistInMap(bot, INTERACTION_DISTANCE))
                return false;

            return true;
        }
        case TYPEID_GAMEOBJECT:
        {
            ObjectGuid guid = questGiver->GetGUID();
            if (GameObject* go = bot->GetMap()->GetGameObject(guid))
            {
                if (go->IsWithinDistInMap(bot))
                {
                    return true;
                }
            }
            return false;
        }
        // case TYPEID_PLAYER:
        //     return bot->IsAlive() && questGiver->ToPlayer()->IsAlive();
        // case TYPEID_ITEM:
        //     return bot->IsAlive();
        default:
            break;
    }
    return false;
}

bool NewRpgBaseAction::AcceptQuest(Quest const* quest, ObjectGuid guid)
{
    WorldPacket p(CMSG_QUESTGIVER_ACCEPT_QUEST);
    uint32 unk1 = 0;
    p << guid << quest->GetQuestId() << unk1;
    p.rpos(0);
    bot->GetSession()->HandleQuestgiverAcceptQuestOpcode(p);

    return true;
}

bool NewRpgBaseAction::TurnInQuest(Quest const* quest, ObjectGuid guid)
{
    uint32 questID = quest->GetQuestId();

    if (bot->GetQuestRewardStatus(questID))
    {
        return false;
    }

    if (!bot->CanRewardQuest(quest, false))
    {
        return false;
    }

    bot->PlayDistanceSound(621);

    WorldPacket p(CMSG_QUESTGIVER_CHOOSE_REWARD);
    p << guid << quest->GetQuestId();
    if (quest->GetRewChoiceItemsCount() <= 1)
    {
        p << 0;
        bot->GetSession()->HandleQuestgiverChooseRewardOpcode(p);
    }
    else
    {
        uint32 bestId = BestRewardIndex(quest);
        p << bestId;
        bot->GetSession()->HandleQuestgiverChooseRewardOpcode(p);
    }

    return true;
}

uint32 NewRpgBaseAction::BestRewardIndex(Quest const* quest)
{
    ItemIds returnIds;
    ItemUsage bestUsage = ITEM_USAGE_NONE;
    if (quest->GetRewChoiceItemsCount() <= 1)
        return 0;
    else
    {
        for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewardChoiceItemId[i]);
            if (usage == ITEM_USAGE_EQUIP || usage == ITEM_USAGE_REPLACE)
                bestUsage = ITEM_USAGE_EQUIP;
            else if (usage == ITEM_USAGE_BAD_EQUIP && bestUsage != ITEM_USAGE_EQUIP)
                bestUsage = usage;
            else if (usage != ITEM_USAGE_NONE && bestUsage == ITEM_USAGE_NONE)
                bestUsage = usage;
        }
        StatsWeightCalculator calc(bot);
        uint32 best = 0;
        float bestScore = 0;
        for (uint8 i = 0; i < quest->GetRewChoiceItemsCount(); ++i)
        {
            ItemUsage usage = AI_VALUE2(ItemUsage, "item usage", quest->RewardChoiceItemId[i]);
            if (usage == bestUsage || usage == ITEM_USAGE_REPLACE)
            {
                float score = calc.CalculateItem(quest->RewardChoiceItemId[i]);
                if (score > bestScore)
                {
                    bestScore = score;
                    best = i;
                }
            }
        }
        return best;
    }
}

bool NewRpgBaseAction::IsQuestWorthDoing(Quest const* quest)
{
    bool isLowLevelQuest =
        bot->GetLevel() > (bot->GetQuestLevel(quest) + sWorld->getIntConfig(CONFIG_QUEST_LOW_LEVEL_HIDE_DIFF));

    if (isLowLevelQuest)
        return false;

    if (quest->IsRepeatable())
        return false;

    if (quest->IsSeasonal())
        return false;

    return true;
}

bool NewRpgBaseAction::IsQuestCapableDoing(Quest const* quest)
{
    bool highLevelQuest = bot->GetLevel() + 3 < bot->GetQuestLevel(quest);
    if (highLevelQuest)
        return false;

    // Elite quest and dungeon quest etc
    if (quest->GetType() != 0)
        return false;

    // now we only capable of doing solo quests
    if (quest->GetSuggestedPlayers() >= 2)
        return false;

    return true;
}

bool NewRpgBaseAction::OrganizeQuestLog()
{
    int32 freeSlotNum = 0;

    for (uint16 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questId = bot->GetQuestSlotQuestId(i);
        if (!questId)
            freeSlotNum++;
    }

    // it's ok if we have two more free slots
    if (freeSlotNum >= 2)
        return false;

    int32 dropped = 0;
    // remove quests that not worth doing or not capable of doing
    for (uint16 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questId = bot->GetQuestSlotQuestId(i);
        if (!questId)
            continue;

        const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!IsQuestWorthDoing(quest) || !IsQuestCapableDoing(quest) ||
            bot->GetQuestStatus(questId) == QUEST_STATUS_FAILED)
        {
            LOG_DEBUG("playerbots", "[New RPG] {} drop quest {}", bot->GetName(), questId);
            WorldPacket packet(CMSG_QUESTLOG_REMOVE_QUEST);
            packet << (uint8)i;
            bot->GetSession()->HandleQuestLogRemoveQuest(packet);
            if (botAI->GetMaster())
                botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "new_rpg_quest_dropped",
                    "Quest dropped %quest",
                    {{"%quest", ChatHelper::FormatQuest(quest)}}));
            botAI->rpgStatistic.questDropped++;
            dropped++;
        }
    }

    // drop more than 8 quests at once to avoid repeated accept and drop
    if (dropped >= 8)
        return true;

    // remove festival/class quests and quests in different zone
    for (uint16 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questId = bot->GetQuestSlotQuestId(i);
        if (!questId)
            continue;

        const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
        const int64_t botZoneId = this->bot->GetZoneId();

        if (quest->GetZoneOrSort() < 0 || (quest->GetZoneOrSort() > 0 && quest->GetZoneOrSort() != botZoneId))
        {
            LOG_DEBUG("playerbots", "[New RPG] {} drop quest {}", bot->GetName(), questId);
            WorldPacket packet(CMSG_QUESTLOG_REMOVE_QUEST);
            packet << (uint8)i;
            bot->GetSession()->HandleQuestLogRemoveQuest(packet);
            if (botAI->GetMaster())
                botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                    "new_rpg_quest_dropped",
                    "Quest dropped %quest",
                    {{"%quest", ChatHelper::FormatQuest(quest)}}));
            botAI->rpgStatistic.questDropped++;
            dropped++;
        }
    }

    if (dropped >= 8)
        return true;

    // clear quests log
    for (uint16 i = 0; i < MAX_QUEST_LOG_SIZE; ++i)
    {
        uint32 questId = bot->GetQuestSlotQuestId(i);
        if (!questId)
            continue;

        const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
        LOG_DEBUG("playerbots", "[New RPG] {} drop quest {}", bot->GetName(), questId);
        WorldPacket packet(CMSG_QUESTLOG_REMOVE_QUEST);
        packet << (uint8)i;
        bot->GetSession()->HandleQuestLogRemoveQuest(packet);
        if (botAI->GetMaster())
            botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "new_rpg_quest_dropped",
                "Quest dropped %quest",
                {{"%quest", ChatHelper::FormatQuest(quest)}}));
        botAI->rpgStatistic.questDropped++;
    }

    return true;
}

bool NewRpgBaseAction::SearchQuestGiverAndAcceptOrReward()
{
    OrganizeQuestLog();
    if (ObjectGuid npcOrGo = ChooseNpcOrGameObjectToInteract(true, 80.0f))
    {
        WorldObject* object = ObjectAccessor::GetWorldObject(*bot, npcOrGo);
        if (bot->CanInteractWithQuestGiver(object))
        {
            InteractWithNpcOrGameObjectForQuest(npcOrGo);
            ForceToWait(5000);
            return true;
        }
        return MoveWorldObjectTo(npcOrGo);
    }
    return false;
}

static bool BotNeedsItemForQuest(Player* bot, uint32 itemId)
{
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;
        if (bot->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            continue;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        QuestStatusData const& qs = bot->getQuestStatusMap().at(questId);
        for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            if (!quest->RequiredItemCount[i])
                continue;
            if (qs.ItemCount[i] >= quest->RequiredItemCount[i])
                continue;
            if (quest->RequiredItemId[i] == itemId)
                return true;
        }
    }
    return false;
}

bool NewRpgBaseAction::TryLootQuestGO(ObjectGuid& pursuedGO, float searchRange)
{
    if (!bot->IsAlive() || bot->IsBeingTeleported() || bot->IsInFlight() ||
        bot->GetVehicle() || bot->GetTransport())
        return false;

    // valid = spawned, selectable, holds a quest item we still need.
    // INTERACT_COND is fine — ConditionMgr already gates on quest state.
    auto isValidTarget = [&](GameObject* go) -> bool
    {
        if (!go || !go->IsInWorld() || !go->isSpawned())
            return false;
        if (!(go->GetPhaseMask() & bot->GetPhaseMask()))
            return false;
        if (go->HasGameObjectFlag(GO_FLAG_NOT_SELECTABLE))
            return false;
        GameObjectTemplate const* info = go->GetGOInfo();
        if (!info)
            return false;

        // per-player quest drops via gameobject_questitem (Webwood Eggs…)
        if (GameObjectQuestItemList const* items =
                sObjectMgr->GetGameObjectQuestItemList(go->GetEntry()))
        {
            for (size_t i = 0; i < MAX_GAMEOBJECT_QUEST_ITEMS && i < items->size(); ++i)
            {
                uint32 itemId = uint32((*items)[i]);
                if (!itemId)
                    continue;
                if (BotNeedsItemForQuest(bot, itemId))
                    return true;
            }
        }

        // standard loot template (chests, fishing holes)
        if (uint32 lootId = info->GetLootId())
        {
            if (LootTemplates_Gameobject.HaveQuestLootForPlayer(lootId, bot))
                return true;
        }
        return false;
    };

    // 2.5y sits inside the 3.5y loot gate with headroom
    const float lootRange = 2.5f;

    // stick with the committed target — re-picking nearest every tick
    // causes zig-zag walks in dense spawn clusters
    if (pursuedGO)
    {
        GameObject* existing = botAI->GetGameObject(pursuedGO);
        if (existing && isValidTarget(existing) &&
            bot->GetDistance(existing) <= searchRange)
        {
            if (bot->GetDistance(existing) > lootRange)
                return MoveWorldObjectTo(existing->GetGUID(), lootRange);
            // in range — loot strategy opens it
            return true;
        }
        pursuedGO.Clear();
    }

    GuidVector possibleGameObjects = AI_VALUE(GuidVector, "possible new rpg game objects");
    if (possibleGameObjects.empty())
        return false;

    GameObject* best = nullptr;
    float bestDist = searchRange;
    for (ObjectGuid guid : possibleGameObjects)
    {
        GameObject* go = botAI->GetGameObject(guid);
        if (!isValidTarget(go))
            continue;
        float d = bot->GetDistance(go);
        if (d >= bestDist)
            continue;
        best = go;
        bestDist = d;
    }
    if (!best)
        return false;

    // commit
    pursuedGO = best->GetGUID();

    if (bot->GetDistance(best) > lootRange)
        return MoveWorldObjectTo(best->GetGUID(), lootRange);

    // in range — consume the tick so we don't fall through to wander
    return true;
}

bool NewRpgBaseAction::TryUseQuestGO(ObjectGuid& pursuedGO, float searchRange)
{
    if (!bot->IsAlive() || bot->IsBeingTeleported() || bot->IsInFlight() ||
        bot->GetVehicle() || bot->GetTransport())
        return false;

    std::unordered_set<uint32> neededGoEntries;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;
        if (bot->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            continue;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        QuestStatusData const& qs = bot->getQuestStatusMap().at(questId);
        for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            int32 entry = quest->RequiredNpcOrGo[i];
            if (entry >= 0)
                continue;
            if (qs.CreatureOrGOCount[i] >= quest->RequiredNpcOrGoCount[i])
                continue;
            neededGoEntries.insert(uint32(-entry));
        }
    }
    if (neededGoEntries.empty())
        return false;

    auto isValidTarget = [&](GameObject* go) -> bool
    {
        if (!go || !go->IsInWorld() || !go->isSpawned())
            return false;
        if (!(go->GetPhaseMask() & bot->GetPhaseMask()))
            return false;
        if (go->HasGameObjectFlag(GO_FLAG_NOT_SELECTABLE))
            return false;
        return neededGoEntries.count(go->GetEntry()) > 0;
    };

    // commitment first
    if (pursuedGO)
    {
        GameObject* existing = botAI->GetGameObject(pursuedGO);
        if (existing && isValidTarget(existing) &&
            bot->GetDistance(existing) <= searchRange)
        {
            if (bot->GetDistance(existing) > INTERACTION_DISTANCE)
                return MoveWorldObjectTo(existing->GetGUID(), INTERACTION_DISTANCE);
            existing->Use(bot);
            ForceToWait(2000);
            pursuedGO.Clear();
            return true;
        }
        pursuedGO.Clear();
    }

    GuidVector possibleGameObjects = AI_VALUE(GuidVector, "possible new rpg game objects");
    GameObject* best = nullptr;
    float bestDist = searchRange;
    for (ObjectGuid guid : possibleGameObjects)
    {
        GameObject* go = botAI->GetGameObject(guid);
        if (!isValidTarget(go))
            continue;
        float d = bot->GetDistance(go);
        if (d >= bestDist)
            continue;
        best = go;
        bestDist = d;
    }
    if (!best)
        return false;

    pursuedGO = best->GetGUID();

    if (bot->GetDistance(best) > INTERACTION_DISTANCE)
        return MoveWorldObjectTo(best->GetGUID(), INTERACTION_DISTANCE);

    best->Use(bot);
    ForceToWait(2000);
    pursuedGO.Clear();
    return true;
}

bool NewRpgBaseAction::TryUseQuestItem(ObjectGuid& pursuedGO, ObjectGuid& pursuedTarget, float searchRange)
{
    if (!bot->IsAlive() || bot->IsBeingTeleported() || bot->IsInFlight() ||
        bot->GetVehicle() || bot->GetTransport())
        return false;

    std::unordered_set<uint32> candidateItemEntries;
    // src items (the quest gave the bot a single item to use); branch C
    // (self/area cast) is only safe to fire on these — auto-firing every
    // ItemDrop on self can burn kill-credit sentinels and trigger
    // unintended scripted side effects.
    std::unordered_set<uint32> srcItemEntries;
    std::unordered_set<uint32> neededCreatureEntries;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;
        if (bot->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            continue;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        if (uint32 src = quest->GetSrcItemId())
        {
            candidateItemEntries.insert(src);
            srcItemEntries.insert(src);
        }
        // handed out by the quest (brands, flares, nets, standards)
        for (int i = 0; i < QUEST_SOURCE_ITEM_IDS_COUNT; ++i)
        {
            if (uint32 drop = quest->ItemDrop[i])
                candidateItemEntries.insert(drop);
        }
        QuestStatusData const& qs = bot->getQuestStatusMap().at(questId);
        for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            int32 entry = quest->RequiredNpcOrGo[i];
            if (entry <= 0)
                continue;
            if (qs.CreatureOrGOCount[i] >= quest->RequiredNpcOrGoCount[i])
                continue;
            neededCreatureEntries.insert(uint32(entry));
        }
    }
    if (candidateItemEntries.empty())
        return false;

    for (uint32 itemEntry : candidateItemEntries)
    {
        Item* item = bot->GetItemByEntry(itemEntry);
        if (!item)
            continue;
        ItemTemplate const* proto = item->GetTemplate();
        if (!proto)
            continue;
        uint32 useSpellId = 0;
        for (uint8 i = 0; i < MAX_ITEM_PROTO_SPELLS; ++i)
        {
            if (proto->Spells[i].SpellTrigger != ITEM_SPELLTRIGGER_ON_USE)
                continue;
            if (proto->Spells[i].SpellId <= 0)
                continue;
            useSpellId = proto->Spells[i].SpellId;
            break;
        }
        if (!useSpellId)
            continue;
        SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(useSpellId);
        if (!spellInfo)
            continue;

        // A: spell needs a focus GO (moonwell / lectern / anvil)
        if (uint32 focusId = spellInfo->RequiresSpellFocus)
        {
            auto focusRadius = [](GameObject* go) -> float
            {
                GameObjectTemplate const* info = go->GetGOInfo();
                // half radius so we end up inside, not on the rim
                return std::max<float>(1.0f, float(info->spellFocus.dist) * 0.5f);
            };
            auto isValidFocus = [&](GameObject* go) -> bool
            {
                if (!go || !go->IsInWorld() || !go->isSpawned())
                    return false;
                if (!(go->GetPhaseMask() & bot->GetPhaseMask()))
                    return false;
                if (go->HasGameObjectFlag(GO_FLAG_NOT_SELECTABLE))
                    return false;
                GameObjectTemplate const* info = go->GetGOInfo();
                if (!info || info->type != GAMEOBJECT_TYPE_SPELL_FOCUS)
                    return false;
                return info->spellFocus.focusId == focusId;
            };

            // commitment first
            if (pursuedGO)
            {
                GameObject* existing = botAI->GetGameObject(pursuedGO);
                if (existing && isValidFocus(existing) &&
                    bot->GetDistance(existing) <= searchRange)
                {
                    float radius = focusRadius(existing);
                    if (bot->GetDistance(existing) > radius)
                        return MoveWorldObjectTo(existing->GetGUID(), radius);
                    SpellCastTargets targets;
                    bot->CastItemUseSpell(item, targets, 1, 0);
                    ForceToWait(2000);
                    pursuedGO.Clear();
                    return true;
                }
                pursuedGO.Clear();
            }

            GuidVector possibleGameObjects = AI_VALUE(GuidVector, "possible new rpg game objects");
            GameObject* best = nullptr;
            float bestDist = searchRange;
            float bestRadius = INTERACTION_DISTANCE;
            for (ObjectGuid guid : possibleGameObjects)
            {
                GameObject* go = botAI->GetGameObject(guid);
                if (!isValidFocus(go))
                    continue;
                float d = bot->GetDistance(go);
                if (d >= bestDist)
                    continue;
                best = go;
                bestDist = d;
                bestRadius = focusRadius(go);
            }
            if (best)
            {
                pursuedGO = best->GetGUID();
                if (bot->GetDistance(best) > bestRadius)
                    return MoveWorldObjectTo(best->GetGUID(), bestRadius);
                SpellCastTargets targets;
                bot->CastItemUseSpell(item, targets, 1, 0);
                ForceToWait(2000);
                pursuedGO.Clear();
                return true;
            }
            continue;
        }

        // B: spell needs a unit target — walk to the required creature
        if (spellInfo->NeedsExplicitUnitTarget() && !neededCreatureEntries.empty())
        {
            auto isValidCreature = [&](Creature* c) -> bool
            {
                if (!c || !c->IsInWorld() || !c->IsAlive())
                    return false;
                if (!(c->GetPhaseMask() & bot->GetPhaseMask()))
                    return false;
                return neededCreatureEntries.count(c->GetEntry()) > 0;
            };

            // commitment first
            if (pursuedTarget)
            {
                Creature* existing = botAI->GetCreature(pursuedTarget);
                if (existing && isValidCreature(existing) &&
                    bot->GetDistance(existing) <= searchRange)
                {
                    if (bot->GetDistance(existing) > INTERACTION_DISTANCE)
                        return MoveWorldObjectTo(existing->GetGUID(), INTERACTION_DISTANCE);
                    SpellCastTargets targets;
                    targets.SetUnitTarget(existing);
                    bot->CastItemUseSpell(item, targets, 1, 0);
                    ForceToWait(2000);
                    pursuedTarget.Clear();
                    return true;
                }
                pursuedTarget.Clear();
            }

            GuidVector possibleTargets = AI_VALUE(GuidVector, "possible new rpg targets");
            Creature* best = nullptr;
            float bestDist = searchRange;
            for (ObjectGuid guid : possibleTargets)
            {
                Creature* c = botAI->GetCreature(guid);
                if (!isValidCreature(c))
                    continue;
                float d = bot->GetDistance(c);
                if (d >= bestDist)
                    continue;
                best = c;
                bestDist = d;
            }
            if (best)
            {
                pursuedTarget = best->GetGUID();
                if (bot->GetDistance(best) > INTERACTION_DISTANCE)
                    return MoveWorldObjectTo(best->GetGUID(), INTERACTION_DISTANCE);
                SpellCastTargets targets;
                targets.SetUnitTarget(best);
                bot->CastItemUseSpell(item, targets, 1, 0);
                ForceToWait(2000);
                pursuedTarget.Clear();
                return true;
            }
            continue;
        }

        // C: self / area — fire at bot's position. Restrict to GetSrcItemId
        // items (the single item the quest hands the bot for self-use, e.g.
        // a potion). ItemDrop entries can be kill-credit sentinels or
        // scripted items that should never be auto-used on self.
        if (!srcItemEntries.count(itemEntry))
            continue;
        SpellCastTargets targets;
        if (spellInfo->IsTargetingArea())
            targets.SetDst(*bot);
        else
            targets.SetUnitTarget(bot);
        bot->CastItemUseSpell(item, targets, 1, 0);
        ForceToWait(2000);
        return true;
    }

    return false;
}

bool NewRpgBaseAction::HasNearbyQuestMob(float range)
{
    // kill objectives + mobs that drop required quest items
    std::unordered_set<uint32> neededCreatureEntries;
    std::unordered_set<uint32> neededItemIds;
    for (uint16 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;
        if (bot->GetQuestStatus(questId) != QUEST_STATUS_INCOMPLETE)
            continue;
        Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
        if (!quest)
            continue;
        QuestStatusData const& qs = bot->getQuestStatusMap().at(questId);
        for (int i = 0; i < QUEST_OBJECTIVES_COUNT; ++i)
        {
            int32 entry = quest->RequiredNpcOrGo[i];
            if (entry <= 0)
                continue;
            if (qs.CreatureOrGOCount[i] >= quest->RequiredNpcOrGoCount[i])
                continue;
            neededCreatureEntries.insert(uint32(entry));
        }
        for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; ++i)
        {
            if (!quest->RequiredItemCount[i])
                continue;
            if (qs.ItemCount[i] >= quest->RequiredItemCount[i])
                continue;
            if (quest->RequiredItemId[i])
                neededItemIds.insert(quest->RequiredItemId[i]);
        }
    }
    if (neededCreatureEntries.empty() && neededItemIds.empty())
        return false;

    GuidVector possibleTargets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid guid : possibleTargets)
    {
        Creature* c = botAI->GetCreature(guid);
        if (!c || !c->IsInWorld() || !c->IsAlive())
            continue;
        if (!(c->GetPhaseMask() & bot->GetPhaseMask()))
            continue;
        if (bot->GetDistance(c) > range)
            continue;

        // direct kill objective
        if (neededCreatureEntries.count(c->GetEntry()))
            return true;

        // drops a required quest item — HaveQuestLootForPlayer
        // already filters by what this player still needs
        if (!neededItemIds.empty())
        {
            CreatureTemplate const* tmpl = c->GetCreatureTemplate();
            if (tmpl && tmpl->lootid &&
                LootTemplates_Creature.HaveQuestLootForPlayer(tmpl->lootid, bot))
            {
                return true;
            }
        }
    }
    return false;
}

bool NewRpgBaseAction::HasNearbyQuestMobForObjective(float range, uint32 questId, int32 objectiveIdx)
{
    if (!questId)
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    // Turn-in path: completed quest has no remaining mob objective.
    if (bot->GetQuestStatus(questId) == QUEST_STATUS_COMPLETE)
        return false;

    QuestStatusData const& qs = bot->getQuestStatusMap().at(questId);

    uint32 neededCreatureEntry = 0;
    uint32 neededItemId = 0;

    if (objectiveIdx >= 0 && objectiveIdx < QUEST_OBJECTIVES_COUNT)
    {
        int32 entry = quest->RequiredNpcOrGo[objectiveIdx];
        if (entry > 0 &&
            qs.CreatureOrGOCount[objectiveIdx] < quest->RequiredNpcOrGoCount[objectiveIdx])
        {
            neededCreatureEntry = uint32(entry);
        }
        // Item objective sometimes lives in the same slot range.
        if (objectiveIdx < QUEST_ITEM_OBJECTIVES_COUNT &&
            quest->RequiredItemId[objectiveIdx] &&
            qs.ItemCount[objectiveIdx] < quest->RequiredItemCount[objectiveIdx])
        {
            neededItemId = quest->RequiredItemId[objectiveIdx];
        }
    }
    else if (objectiveIdx >= QUEST_OBJECTIVES_COUNT &&
             objectiveIdx < QUEST_OBJECTIVES_COUNT + QUEST_ITEM_OBJECTIVES_COUNT)
    {
        int32 itemSlot = objectiveIdx - QUEST_OBJECTIVES_COUNT;
        if (quest->RequiredItemId[itemSlot] &&
            qs.ItemCount[itemSlot] < quest->RequiredItemCount[itemSlot])
        {
            neededItemId = quest->RequiredItemId[itemSlot];
        }
    }

    if (!neededCreatureEntry && !neededItemId)
        return false;

    GuidVector possibleTargets = AI_VALUE(GuidVector, "possible targets");
    for (ObjectGuid guid : possibleTargets)
    {
        Creature* c = botAI->GetCreature(guid);
        if (!c || !c->IsInWorld() || !c->IsAlive())
            continue;
        if (!(c->GetPhaseMask() & bot->GetPhaseMask()))
            continue;
        if (bot->GetDistance(c) > range)
            continue;

        if (neededCreatureEntry && c->GetEntry() == neededCreatureEntry)
            return true;

        if (neededItemId)
        {
            CreatureTemplate const* tmpl = c->GetCreatureTemplate();
            if (tmpl && tmpl->lootid &&
                LootTemplates_Creature.HaveQuestLootForPlayer(tmpl->lootid, bot))
                return true;
        }
    }
    return false;
}


ObjectGuid NewRpgBaseAction::ChooseNpcOrGameObjectToInteract(bool questgiverOnly, float distanceLimit)
{
    GuidVector possibleTargets = AI_VALUE(GuidVector, "possible new rpg targets");
    GuidVector possibleGameObjects = AI_VALUE(GuidVector, "possible new rpg game objects");

    if (possibleTargets.empty() && possibleGameObjects.empty())
        return ObjectGuid();

    WorldObject* nearestObject = nullptr;
    for (ObjectGuid& guid : possibleTargets)
    {
        WorldObject* object = ObjectAccessor::GetWorldObject(*bot, guid);

        if (!object || !object->IsInWorld())
            continue;

        if (distanceLimit && bot->GetDistance(object) > distanceLimit)
            continue;

        if (CanInteractWithQuestGiver(object) && HasQuestToAcceptOrReward(object))
        {
            if (!nearestObject || bot->GetExactDist(nearestObject) > bot->GetExactDist(object))
                nearestObject = object;
            break;
        }
    }

    for (ObjectGuid& guid : possibleGameObjects)
    {
        WorldObject* object = ObjectAccessor::GetWorldObject(*bot, guid);

        if (!object || !object->IsInWorld())
            continue;

        if (distanceLimit && bot->GetDistance(object) > distanceLimit)
            continue;

        if (CanInteractWithQuestGiver(object) && HasQuestToAcceptOrReward(object))
        {
            if (!nearestObject || bot->GetExactDist(nearestObject) > bot->GetExactDist(object))
                nearestObject = object;
            break;
        }
    }

    if (nearestObject)
        return nearestObject->GetGUID();

    // No questgiver to accept or reward
    if (questgiverOnly)
        return ObjectGuid();

    if (possibleTargets.empty())
        return ObjectGuid();

    int idx = urand(0, possibleTargets.size() - 1);
    ObjectGuid guid = possibleTargets[idx];
    WorldObject* object = ObjectAccessor::GetCreatureOrPetOrVehicle(*bot, guid);
    if (!object)
        object = ObjectAccessor::GetGameObject(*bot, guid);

    if (object && object->IsInWorld())
    {
        return object->GetGUID();
    }
    return ObjectGuid();
}

bool NewRpgBaseAction::HasQuestToAcceptOrReward(WorldObject* object)
{
    ObjectGuid guid = object->GetGUID();
    bot->PrepareQuestMenu(guid);
    const QuestMenu& menu = bot->PlayerTalkClass->GetQuestMenu();
    if (menu.Empty())
        return false;

    for (uint8 idx = 0; idx < menu.GetMenuItemCount(); idx++)
    {
        const QuestMenuItem& item = menu.GetItem(idx);
        const Quest* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
        if (!quest)
            continue;
        const QuestStatus& status = bot->GetQuestStatus(item.QuestId);
        if (status == QUEST_STATUS_COMPLETE && bot->CanRewardQuest(quest, 0, false))
        {
            return true;
        }
    }
    for (uint8 idx = 0; idx < menu.GetMenuItemCount(); idx++)
    {
        const QuestMenuItem& item = menu.GetItem(idx);
        const Quest* quest = sObjectMgr->GetQuestTemplate(item.QuestId);
        if (!quest)
            continue;

        const QuestStatus& status = bot->GetQuestStatus(item.QuestId);
        if (status == QUEST_STATUS_NONE && bot->CanTakeQuest(quest, false) && bot->CanAddQuest(quest, false) &&
            IsQuestWorthDoing(quest) && IsQuestCapableDoing(quest))
        {
            return true;
        }
    }
    return false;
}

static std::vector<float> GenerateRandomWeights(int n)
{
    std::vector<float> weights(n);
    float sum = 0.0;

    for (int i = 0; i < n; ++i)
    {
        weights[i] = rand_norm();
        sum += weights[i];
    }
    for (int i = 0; i < n; ++i)
    {
        weights[i] /= sum;
    }
    return weights;
}

bool NewRpgBaseAction::GetQuestPOIPosAndObjectiveIdx(uint32 questId, std::vector<POIInfo>& poiInfo, bool toComplete)
{
    Quest const* quest = sObjectMgr->GetQuestTemplate(questId);
    if (!quest)
        return false;

    const QuestPOIVector* poiVector = sObjectMgr->GetQuestPOIVector(questId);
    if (!poiVector)
    {
        return false;
    }

    const QuestStatusData& q_status = bot->getQuestStatusMap().at(questId);

    if (toComplete && q_status.Status == QUEST_STATUS_COMPLETE)
    {
        for (const QuestPOI& qPoi : *poiVector)
        {
            if (qPoi.MapId != bot->GetMapId())
                continue;

            // not the poi pos to reward quest
            if (qPoi.ObjectiveIndex != -1)
                continue;

            if (qPoi.points.size() == 0)
                continue;

            float dx = 0, dy = 0;
            std::vector<float> weights = GenerateRandomWeights(qPoi.points.size());
            for (size_t i = 0; i < qPoi.points.size(); i++)
            {
                const QuestPOIPoint& point = qPoi.points[i];
                dx += point.x * weights[i];
                dy += point.y * weights[i];
            }

            if (bot->GetDistance2d(dx, dy) >= 1500.0f)
                continue;

            float dz = std::max(bot->GetMap()->GetHeight(dx, dy, MAX_HEIGHT), bot->GetMap()->GetWaterLevel(dx, dy));

            if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
                continue;

            if (bot->GetZoneId() != bot->GetMap()->GetZoneId(bot->GetPhaseMask(), dx, dy, dz))
                continue;

            poiInfo.push_back({{dx, dy}, qPoi.ObjectiveIndex});
        }

        if (poiInfo.empty())
            return false;

        return true;
    }

    if (q_status.Status != QUEST_STATUS_INCOMPLETE)
        return false;

    // Get incomplete quest objective index
    std::vector<int32> incompleteObjectiveIdx;
    for (int i = 0; i < QUEST_OBJECTIVES_COUNT; i++)
    {
        int32 npcOrGo = quest->RequiredNpcOrGo[i];
        if (!npcOrGo)
            continue;

        if (q_status.CreatureOrGOCount[i] < quest->RequiredNpcOrGoCount[i])
            incompleteObjectiveIdx.push_back(i);
    }
    for (int i = 0; i < QUEST_ITEM_OBJECTIVES_COUNT; i++)
    {
        uint32 itemId = quest->RequiredItemId[i];
        if (!itemId)
            continue;

        if (q_status.ItemCount[i] < quest->RequiredItemCount[i])
            incompleteObjectiveIdx.push_back(QUEST_OBJECTIVES_COUNT + i);
    }

    // Get POIs to go
    for (const QuestPOI& qPoi : *poiVector)
    {
        if (qPoi.MapId != bot->GetMapId())
            continue;

        bool inComplete = false;
        for (uint32 objective : incompleteObjectiveIdx)
        {
            if (qPoi.ObjectiveIndex == objective)
            {
                inComplete = true;
                break;
            }
        }
        if (!inComplete)
            continue;
        if (qPoi.points.size() == 0)
            continue;
        float dx = 0, dy = 0;
        std::vector<float> weights = GenerateRandomWeights(qPoi.points.size());
        for (size_t i = 0; i < qPoi.points.size(); i++)
        {
            const QuestPOIPoint& point = qPoi.points[i];
            dx += point.x * weights[i];
            dy += point.y * weights[i];
        }

        if (bot->GetDistance2d(dx, dy) >= 1500.0f)
            continue;

        float dz = std::max(bot->GetMap()->GetHeight(dx, dy, MAX_HEIGHT), bot->GetMap()->GetWaterLevel(dx, dy));

        if (dz == INVALID_HEIGHT || dz == VMAP_INVALID_HEIGHT_VALUE)
            continue;

        if (bot->GetZoneId() != bot->GetMap()->GetZoneId(bot->GetPhaseMask(), dx, dy, dz))
            continue;

        poiInfo.push_back({{dx, dy}, qPoi.ObjectiveIndex});
    }

    if (poiInfo.size() == 0)
    {
        // LOG_DEBUG("playerbots", "[New rpg] {}: No available poi can be found for quest {}", bot->GetName(), questId);
        return false;
    }

    return true;
}

WorldPosition NewRpgBaseAction::SelectRandomGrindPos(Player* bot)
{
    const std::vector<WorldLocation>& locs = sTravelMgr.GetLocsPerLevelCache(bot->GetLevel());
    float hiRange = 500.0f;
    float loRange = 2500.0f;
    if (bot->GetLevel() < 5)
    {
        hiRange /= 3;
        loRange /= 3;
    }
    std::vector<WorldLocation> lo_prepared_locs, hi_prepared_locs;

    bool inCity = false;
    if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(bot->GetZoneId()))
    {
        if (zone->flags & AREA_FLAG_CAPITAL)
            inCity = true;
    }

    for (auto& loc : locs)
    {
        if (bot->GetMapId() != loc.GetMapId())
            continue;

        if (bot->GetExactDist(loc) > 2500.0f)
            continue;

        if (!inCity && bot->GetMap()->GetZoneId(bot->GetPhaseMask(), loc.GetPositionX(), loc.GetPositionY(),
                                                loc.GetPositionZ()) != bot->GetZoneId())
            continue;

        if (bot->GetExactDist(loc) < hiRange)
        {
            hi_prepared_locs.push_back(loc);
        }

        if (bot->GetExactDist(loc) < loRange)
        {
            lo_prepared_locs.push_back(loc);
        }
    }
    WorldPosition dest{};
    if (urand(1, 100) <= 50 && !hi_prepared_locs.empty())
    {
        uint32 idx = urand(0, hi_prepared_locs.size() - 1);
        dest = hi_prepared_locs[idx];
    }
    else if (!lo_prepared_locs.empty())
    {
        uint32 idx = urand(0, lo_prepared_locs.size() - 1);
        dest = lo_prepared_locs[idx];
    }

    if (!dest.IsValid())
        return dest;

    LOG_DEBUG("playerbots", "[New RPG] Bot {} select random grind pos Map:{} X:{} Y:{} Z:{} ({}+{} available in {})",
              bot->GetName(), dest.GetMapId(), dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(),
              hi_prepared_locs.size(), lo_prepared_locs.size() - hi_prepared_locs.size(), locs.size());
    return dest;
}

WorldPosition NewRpgBaseAction::SelectRandomCampPos(Player* bot)
{
    const std::vector<WorldLocation> locs = sTravelMgr.GetTravelHubs(bot);

    bool inCity = false;

    if (AreaTableEntry const* zone = sAreaTableStore.LookupEntry(bot->GetZoneId()))
    {
        if (zone->flags & AREA_FLAG_CAPITAL)
            inCity = true;
    }

    std::vector<WorldLocation> prepared_locs;
    for (auto& loc : locs)
    {
        if (bot->GetMapId() != loc.GetMapId())
            continue;

        float range = bot->GetLevel() <= 5 ? 500.0f : 2500.0f;
        if (bot->GetExactDist(loc) > range)
            continue;

        if (bot->GetExactDist(loc) < 50.0f)
            continue;

        if (!inCity && bot->GetMap()->GetZoneId(bot->GetPhaseMask(), loc.GetPositionX(), loc.GetPositionY(),
                                                loc.GetPositionZ()) != bot->GetZoneId())
            continue;

        prepared_locs.push_back(loc);
    }
    WorldPosition dest{};
    if (!prepared_locs.empty())
    {
        uint32 idx = urand(0, prepared_locs.size() - 1);
        dest = prepared_locs[idx];
    }

    if (!dest.IsValid())
        return dest;

    LOG_DEBUG("playerbots", "[New RPG] Bot {} select random inn keeper pos Map:{} X:{} Y:{} Z:{} ({} available in {})",
              bot->GetName(), dest.GetMapId(), dest.GetPositionX(), dest.GetPositionY(), dest.GetPositionZ(),
              prepared_locs.size(), locs.size());
    return dest;
}

bool NewRpgBaseAction::SelectRandomFlightTaxiNode(uint32& flightMasterEntry, WorldPosition& flightMasterPos, std::vector<uint32>& path)
{
    TravelMgr::FlightMasterInfo const* info = sTravelMgr.GetNearestFlightMasterInfo(bot);
    if (!info)
        return false;

    std::vector<std::vector<uint32>> availablePaths = sTravelMgr.GetOptimalFlightDestinations(bot);
    if (availablePaths.empty())
        return false;

    flightMasterEntry = info->templateEntry;
    flightMasterPos = info->pos;
    path = availablePaths[urand(0, availablePaths.size() - 1)];
    LOG_DEBUG("playerbots", "[New RPG] Bot {} select random flight taxi node from:{} (node {}) to:{} ({} available)",
              bot->GetName(), flightMasterEntry, path[0], path[path.size() - 1], availablePaths.size());
    return true;
}

bool NewRpgBaseAction::RandomChangeStatus(std::vector<NewRpgStatus> candidateStatus)
{
    std::vector<NewRpgStatus> availableStatus;
    uint32 probSum = 0;
    for (NewRpgStatus status : candidateStatus)
    {
        if (sPlayerbotAIConfig.RpgStatusProbWeight[status] == 0)
            continue;

        if (CheckRpgStatusAvailable(status))
        {
            availableStatus.push_back(status);
            probSum += sPlayerbotAIConfig.RpgStatusProbWeight[status];
        }
    }
    if (availableStatus.empty() || probSum == 0)
    {
        botAI->rpgInfo.ChangeToRest();
        bot->SetStandState(UNIT_STAND_STATE_SIT);
        return true;
    }
    uint32 rand = urand(1, probSum);
    uint32 accumulate = 0;
    NewRpgStatus chosenStatus = RPG_STATUS_END;
    for (NewRpgStatus status : availableStatus)
    {
        accumulate += sPlayerbotAIConfig.RpgStatusProbWeight[status];
        if (accumulate >= rand)
        {
            chosenStatus = status;
            break;
        }
    }

    switch (chosenStatus)
    {
        case RPG_WANDER_RANDOM:
        {
            botAI->rpgInfo.ChangeToWanderRandom();
            return true;
        }
        case RPG_WANDER_NPC:
        {
            botAI->rpgInfo.ChangeToWanderNpc();
            return true;
        }
        case RPG_GO_GRIND:
        {
            WorldPosition pos = SelectRandomGrindPos(bot);
            if (pos != WorldPosition())
            {
                botAI->rpgInfo.ChangeToGoGrind(pos);
                return true;
            }
            return false;
        }
        case RPG_GO_CAMP:
        {
            WorldPosition pos = SelectRandomCampPos(bot);
            if (pos != WorldPosition())
            {
                botAI->rpgInfo.ChangeToGoCamp(pos);
                return true;
            }
            return false;
        }
        case RPG_DO_QUEST:
        {
            std::vector<uint32> availableQuests;
            for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
            {
                uint32 questId = bot->GetQuestSlotQuestId(slot);
                if (botAI->lowPriorityQuest.find(questId) != botAI->lowPriorityQuest.end())
                    continue;

                std::vector<POIInfo> poiInfo;
                if (GetQuestPOIPosAndObjectiveIdx(questId, poiInfo, true))
                {
                    availableQuests.push_back(questId);
                }
            }
            if (availableQuests.size())
            {
                uint32 questId = availableQuests[urand(0, availableQuests.size() - 1)];
                const Quest* quest = sObjectMgr->GetQuestTemplate(questId);
                if (quest)
                {
                    botAI->rpgInfo.ChangeToDoQuest(questId, quest);
                    return true;
                }
            }
            return false;
        }
        case RPG_TRAVEL_FLIGHT:
        {
            uint32 flightMasterEntry = 0;
            WorldPosition flightMasterPos;
            std::vector<uint32> path;
            if (SelectRandomFlightTaxiNode(flightMasterEntry, flightMasterPos, path))
            {
                botAI->rpgInfo.ChangeToTravelFlight(flightMasterEntry, flightMasterPos, path);
                return true;
            }
            return false;
        }
        case RPG_IDLE:
        {
            botAI->rpgInfo.ChangeToIdle();
            return true;
        }
        case RPG_REST:
        {
            botAI->rpgInfo.ChangeToRest();
            bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
        }
        case RPG_OUTDOOR_PVP:
        {
            botAI->rpgInfo.ChangeToOutdoorPvp();
            return true;
        }
        default:
        {
            botAI->rpgInfo.ChangeToRest();
            bot->SetStandState(UNIT_STAND_STATE_SIT);
            return true;
        }
    }
    return false;
}

bool NewRpgBaseAction::CheckRpgStatusAvailable(NewRpgStatus status)
{
    switch (status)
    {
        case RPG_IDLE:
        case RPG_REST:
            return true;
        case RPG_WANDER_RANDOM:
        {
            Unit* target = AI_VALUE(Unit*, "grind target");
            return target != nullptr;
        }
        case RPG_GO_GRIND:
        {
            WorldPosition pos = SelectRandomGrindPos(bot);
            return pos != WorldPosition();
        }
        case RPG_GO_CAMP:
        {
            WorldPosition pos = SelectRandomCampPos(bot);
            return pos != WorldPosition();
        }
        case RPG_WANDER_NPC:
        {
            GuidVector possibleTargets = AI_VALUE(GuidVector, "possible new rpg targets");
            return possibleTargets.size() >= 3;
        }
        case RPG_DO_QUEST:
        {
            std::vector<uint32> availableQuests;
            for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
            {
                uint32 questId = bot->GetQuestSlotQuestId(slot);
                if (botAI->lowPriorityQuest.find(questId) != botAI->lowPriorityQuest.end())
                    continue;

                std::vector<POIInfo> poiInfo;
                if (GetQuestPOIPosAndObjectiveIdx(questId, poiInfo, true))
                {
                    return true;
                }
            }
            return false;
        }
        case RPG_TRAVEL_FLIGHT:
        {
            uint32 flightMasterEntry = 0;
            WorldPosition flightMasterPos;
            std::vector<uint32> path;
            return SelectRandomFlightTaxiNode(flightMasterEntry, flightMasterPos, path);
        }
        case RPG_OUTDOOR_PVP:
        {
            if (!bot->IsPvP())
                return false;
            uint32 zoneId = bot->GetZoneId();
            if (zoneId == AREA_NAGRAND)
                return false;

            OutdoorPvP* outdoorPvP = sOutdoorPvPMgr->GetOutdoorPvPToZoneId(zoneId);
            return outdoorPvP != nullptr;
        }
        default:
            return false;
    }
    return false;
}
