#include "AiObjectContext.h"
#include "DungeonPathMoveAction.h"
#include "DungeonWaypointMgr.h"
#include "GenericActions.h"
#include "AttackersValue.h"
#include "GridNotifiers.h"
#include "GossipHelloAction.h"
#include <limits>
#include <chrono>

DungeonPathMoveAction::DungeonPathMoveAction(PlayerbotAI* ai, DungeonWaypointMgr* mgr)
    : MovementAction(ai, "dungeon path move"), waypointMgr(mgr) {}

bool DungeonPathMoveAction::Execute(Event event)
{
    Player* bot = botAI->GetBot();
    uint32 mapId = bot->GetMapId();

    if (!waypointMgr)
    {
        return false;
    }

    const auto& allPaths = waypointMgr->GetAllPaths();
    auto it = allPaths.find(mapId);
    if (it == allPaths.end() || it->second.empty())
    {
        return false;
    }

    const DungeonPath* path = &it->second.begin()->second;
    if (path->size() < 2)
    {
        return false;
    }

    size_t closest = FindClosestWaypoint(path, bot);

    float dx = bot->GetPositionX() - (*path)[closest].x;
    float dy = bot->GetPositionY() - (*path)[closest].y;
    float dz = bot->GetPositionZ() - (*path)[closest].z;
    float distToClosest = sqrtf(dx*dx + dy*dy + dz*dz);

    size_t index = DetermineTargetIndex(path, bot, closest);

    if (distToClosest < WAYPOINT_REACHED_DISTANCE && index < path->size() - 1)
    {
        const DungeonWaypoint& wp = (*path)[index];

        if (CheckGroupConditions(bot, wp))
        {
            uint32_t nextIdx = wp.next_index;
            if (nextIdx < path->size())
            {
                previousIndex = index;

                HandleWaypointInteraction(wp, bot, index);
                HandleWaypointNotification(wp);

                index = nextIdx;
            }
        }
        else
        {
            botAI->SetNextCheckDelay(WAIT_FOR_GROUP_DELAY_MS);
        }
    }

    if (index >= path->size()) return false;
    const DungeonWaypoint& wp = (*path)[index];
    bool result = false;
    if (wp.jump)
    {
        result = JumpTo(mapId, wp.x, wp.y, wp.z);
    }
    else
    {
        result = MoveTo(mapId, wp.x, wp.y, wp.z);
    }

    if (wp.next_index == index)
    {
        botAI->TellMasterNoFacing("No next waypoint found, following you instead.");
        botAI->ChangeStrategy("+follow", BOT_STATE_NON_COMBAT);
    }

    HandleCombatEngagement(bot, event);
    return result;
}

size_t DungeonPathMoveAction::FindClosestWaypoint(const DungeonPath* path, Player* bot) const
{
    size_t closest = 0;
    float minDist = std::numeric_limits<float>::max();

    for (size_t i = 0; i < path->size(); ++i)
    {
        float dx = bot->GetPositionX() - (*path)[i].x;
        float dy = bot->GetPositionY() - (*path)[i].y;
        float dz = bot->GetPositionZ() - (*path)[i].z;
        float dist = dx*dx + dy*dy + dz*dz;

        if (dist < minDist)
        {
            minDist = dist;
            closest = i;
        }
    }

    return closest;
}

size_t DungeonPathMoveAction::DetermineTargetIndex(const DungeonPath* path, Player* bot, size_t closestIndex)
{
    float distToPrev = 0.0f;
    if (previousIndex < path->size())
    {
        float dx = bot->GetPositionX() - (*path)[previousIndex].x;
        float dy = bot->GetPositionY() - (*path)[previousIndex].y;
        float dz = bot->GetPositionZ() - (*path)[previousIndex].z;
        distToPrev = sqrtf(dx*dx + dy*dy + dz*dz);
    }

    if (distToPrev <= RESUME_PATH_DISTANCE && path->size() > 0)
    {
        size_t resumeFloor = previousIndex;
        size_t resumeCeil = std::min(previousIndex + RESUME_SEARCH_RANGE,
                                   path->size() - 1);
        size_t resumeIndex = resumeFloor;
        float minDistResume = std::numeric_limits<float>::max();

        for (size_t i = resumeFloor; i <= resumeCeil; ++i)
        {
            float dx = bot->GetPositionX() - (*path)[i].x;
            float dy = bot->GetPositionY() - (*path)[i].y;
            float dz = bot->GetPositionZ() - (*path)[i].z;
            float dist = dx*dx + dy*dy + dz*dz;

            if (dist < minDistResume || (dist == minDistResume && i < resumeIndex))
            {
                minDistResume = dist;
                resumeIndex = i;
            }
        }
        return resumeIndex;
    }

    return closestIndex;
}

bool DungeonPathMoveAction::CheckGroupConditions(Player* bot, const DungeonWaypoint& waypoint) const
{
    Group* group = bot->GetGroup();
    if (!group) return true;

    float maxDistance = sWorld->getFloatConfig(CONFIG_GROUP_XP_DISTANCE);

    for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
    {
        Player* member = ref->GetSource();
        if (!member) continue;

        if (!member->IsAlive())
        {
            return false;
        }

        if (member->GetMapId() != bot->GetMapId())
        {
            return false;
        }

        if (bot->GetDistance(member) > maxDistance)
        {
            return false;
        }

        if (botAI->IsHeal(member))
        {
            uint32 mana = member->GetPower(POWER_MANA);
            uint32 maxMana = member->GetMaxPower(POWER_MANA);

            if (maxMana > 0 && static_cast<float>(mana) < waypoint.healer_mana_pct * static_cast<float>(maxMana))
            {
                return false;
            }
        }

        if (botAI->IsTank(member))
        {
            uint32 health = member->GetHealth();
            uint32 maxHealth = member->GetMaxHealth();

            if (maxHealth > 0 && static_cast<float>(health) < waypoint.healer_mana_pct * static_cast<float>(maxHealth))
            {
                return false;
            }
        }
    }

    return true;
}

void DungeonPathMoveAction::HandleWaypointInteraction(const DungeonWaypoint& waypoint, Player* bot, size_t waypointIndex)
{
    if (pendingMenuInteractionIndex == waypointIndex)
    {
        auto now = std::chrono::steady_clock::now();
        auto timeSinceFirstInteraction = std::chrono::duration_cast<std::chrono::milliseconds>(now - menuInteractionTime).count();

        if (timeSinceFirstInteraction >= 200)
        {
            std::list<Unit*> npcs;
            Acore::AnyUnitInObjectRangeCheck npcCheck(bot, NPC_INTERACT_DISTANCE);
            Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> npcSearcher(bot, npcs, npcCheck);
            Cell::VisitObjects(bot, npcSearcher, NPC_INTERACT_DISTANCE);

            for (Unit* unit : npcs)
            {
                if (unit->ToCreature() && unit->ToCreature()->GetEntry() == waypoint.interact_guid)
                {
                    Creature* foundNpc = unit->ToCreature();
                    GossipHelloAction gossipAction(botAI);
                    gossipAction.Execute(foundNpc->GetGUID(), pendingMenuOption, true);
                    break;
                }
            }

            pendingMenuInteractionIndex = SIZE_MAX;
            pendingMenuOption = -1;
            lastInteractedIndex = waypointIndex;
        }
        return;
    }

    if (waypoint.interact_type == 1 && waypoint.interact_guid != 0)
    {
        if (lastInteractedIndex == waypointIndex)
        {
            return;
        }

        std::list<Unit*> npcs;
        Acore::AnyUnitInObjectRangeCheck npcCheck(bot, NPC_INTERACT_DISTANCE);
        Acore::UnitListSearcher<Acore::AnyUnitInObjectRangeCheck> npcSearcher(bot, npcs, npcCheck);
        Cell::VisitObjects(bot, npcSearcher, NPC_INTERACT_DISTANCE);

        Creature* foundNpc = nullptr;
        for (Unit* unit : npcs)
        {
            if (unit->ToCreature() && unit->ToCreature()->GetEntry() == waypoint.interact_guid)
            {
                foundNpc = unit->ToCreature();
                break;
            }
        }

        if (foundNpc)
        {
            GossipHelloAction gossipAction(botAI);
            int32 menuOption = static_cast<int32>(waypoint.interact_param);

            if (menuOption == -1)
            {
                gossipAction.Execute(foundNpc->GetGUID(), -1, true);
                lastInteractedIndex = waypointIndex;
            }
            else
            {
                gossipAction.Execute(foundNpc->GetGUID(), -1, true);

                pendingMenuInteractionIndex = waypointIndex;
                pendingMenuOption = menuOption;
                menuInteractionTime = std::chrono::steady_clock::now();
            }
        }
    }
}

void DungeonPathMoveAction::HandleWaypointNotification(const DungeonWaypoint& waypoint)
{
    auto now = std::chrono::steady_clock::now();

    if (std::chrono::duration_cast<std::chrono::seconds>(now - lastNotifyTime).count() >= NOTIFICATION_COOLDOWN_SECONDS)
    {
        if (waypoint.tell && !waypoint.comment.empty())
        {
            botAI->TellMasterNoFacing(waypoint.comment);
        }

        if (waypoint.pause > 0)
        {
            botAI->SetNextCheckDelay(waypoint.pause);
        }

        lastNotifyTime = now;
    }
}

void DungeonPathMoveAction::HandleCombatEngagement(Player* bot, Event event)
{
    if (bot->IsInCombat()) return;

    float aggroDist = sPlayerbotAIConfig.aggroDistance;
    std::list<Unit*> targets;
    Acore::AnyUnfriendlyUnitInObjectRangeCheck u_check(bot, bot, aggroDist);
    Acore::UnitListSearcher<Acore::AnyUnfriendlyUnitInObjectRangeCheck> searcher(bot, targets, u_check);
    Cell::VisitObjects(bot, searcher, aggroDist);

    for (Unit* unit : targets)
    {
        if (AttackersValue::IsPossibleTarget(unit, bot, aggroDist) && bot->IsValidAttackTarget(unit))
        {
            bot->SetSelection(unit->GetGUID());
            botAI->GetAiObjectContext()->GetValue<Unit*>("current target")->Set(unit);
            MeleeAction melee(botAI);
            melee.Execute(event);
            break;
        }
    }
}
