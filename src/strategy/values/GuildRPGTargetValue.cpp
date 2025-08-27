

#include "GuildRpgTargetValue.h"
#include "PossibleNewRpgTargetsValue.h"
#include "PossibleNewRpgGameObjectsValue.h"
#include "GuildRpgTasks.h" 

GuidVector GuildFilteredRpgTargetsValue::Calculate()
{
    // 1. Get the standard list of targets from the original AI
    GuidVector allTargets = AI_VALUE(GuidVector, "possible new rpg targets");
    GuidVector allGameObjects = AI_VALUE(GuidVector, "possible new rpg game objects");

    // Combine them
    allTargets.insert(allTargets.end(), allGameObjects.begin(), allGameObjects.end());

    // 2. Check if we have an active guild task that should filter these
    GuildTask guildTask = AI_VALUE(GuildTask, "guild task");
    if (!guildTask.IsValid())
    {
        return allTargets; // No active task, return the original list
    }

    // 3. Apply a filter based on the guild task type/objective
    GuidVector filteredTargets;
    for (ObjectGuid guid : allTargets)
    {
        WorldObject* object = botAI->GetWorldObject(guid);
        if (!object)
            continue;

        bool keepTarget = true;

        switch (guildTask.type)
        {
            case ACTIVITY_PVP:
                // During PVP, ignore most peaceful NPCs. Only focus on enemies and objectives.
                if (object->GetTypeId() == TYPEID_UNIT)
                {
                    Unit* unit = (Unit*)object;
                    // Keep enemy guards, battlemasters, and city bosses
                    if (unit->IsHostileTo(bot) || unit->HasFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_BATTLEMASTER))
                    {
                        keepTarget = true;
                    }
                    else
                    {
                        keepTarget = false; // Ignore bankers, vendors, etc.
                    }
                }
                // Maybe keep game objects like city banners to attack?
                break;

            case ACTIVITY_PVE:
                if (guildTask.objectiveId == PveActivity::RUN_DUNGEON)
                {
                    // In a dungeon, only focus on mobs, bosses, and dungeon-specific objects (chests, doors)
                    if (object->GetTypeId() == TYPEID_UNIT)
                    {
                        Unit* unit = (Unit*)object;
                        keepTarget = unit->IsHostileTo(bot); // Only care about enemies
                    }
                    else if (object->GetTypeId() == TYPEID_GAMEOBJECT)
                    {
                        GameObject* go = (GameObject*)object;
                        // Keep chests, dungeon doors, etc.
                        keepTarget = (go->GetGoType() == GAMEOBJECT_TYPE_CHEST || go->GetGoType() == GAMEOBJECT_TYPE_DOOR);
                    }
                }
                break;

            case ACTIVITY_PROFESSION:
                if (guildTask.objectiveId == ProfessionActivity::GATHER_NODES)
                {
                    // Only keep game objects that are the correct type of node
                    if (object->GetTypeId() == TYPEID_GAMEOBJECT)
                    {
                        GameObject* go = (GameObject*)object;
                        // Example: Check if this node matches the resource we're farming (e.g., "mithril_vein")
                        keepTarget = (go->GetGOInfo()->name == guildTask.params);
                    }
                    else
                    {
                        keepTarget = false; // Ignore all NPCs while gathering
                    }
                }
                break;

            // case ACTIVITY_BASE: // For moving, we might want to ignore everything
            // case ACTIVITY_ROLEPLAY: // Maybe we only want to keep specific NPCs in the tavern
            default:
                // For unhandled types, keep the original target list
                keepTarget = true;
                break;
        }

        if (keepTarget)
        {
            filteredTargets.push_back(guid);
        }
    }

    return filteredTargets;
}