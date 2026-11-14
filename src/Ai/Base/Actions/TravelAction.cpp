/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "TravelAction.h"
#include "GoTravelValue.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"

void GoTravel::Start(PlayerbotAI* botAI, WorldPosition const& destination, std::string const& name)
{
    GoTravelInfo& info = botAI->GetAiObjectContext()->GetValue<GoTravelInfo&>("go travel")->Get();

    // A new destination mid-trip keeps the follow state from before the first one.
    if (!info.active)
        info.restoreFollow = botAI->HasStrategy("follow", BOT_STATE_NON_COMBAT);

    info.destination = destination;
    info.name = name;
    info.active = true;

    botAI->ChangeStrategy("-follow,+travel", BOT_STATE_NON_COMBAT);
}

void GoTravel::End(PlayerbotAI* botAI, bool restoreFollow)
{
    GoTravelInfo& info = botAI->GetAiObjectContext()->GetValue<GoTravelInfo&>("go travel")->Get();
    bool const addFollow = restoreFollow && info.active && info.restoreFollow;

    info = GoTravelInfo();

    botAI->ChangeStrategy(addFollow ? "-travel,+follow" : "-travel", BOT_STATE_NON_COMBAT);
}

bool GoTravel::IsActive(PlayerbotAI* botAI)
{
    return botAI->GetAiObjectContext()->GetValue<GoTravelInfo&>("go travel")->Get().active;
}

bool MoveToGoTravelDestinationAction::Execute(Event /*event*/)
{
    MoveFarTo(AI_VALUE(GoTravelInfo&, "go travel").destination);

    // Claim the tick even while a spline is still running, so lower-relevance movement
    // (new rpg) never redirects the bot mid-trip.
    return true;
}

bool MoveToGoTravelDestinationAction::isUseful() { return AI_VALUE(GoTravelInfo&, "go travel").active; }

bool EndGoTravelAction::Execute(Event /*event*/)
{
    GoTravelInfo const& info = AI_VALUE(GoTravelInfo&, "go travel");

    if (info.active)
        botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "go_travel_arrived", "Arrived at %destination", {{"%destination", info.name}}));

    GoTravel::End(botAI, true);
    return true;
}

bool MoveToDarkPortalAction::Execute(Event /*event*/)
{
    if (bot->GetGroup())
    {
        if (bot->GetGroup()->GetLeaderGUID() != bot->GetGUID() &&
            !GET_PLAYERBOT_AI(GET_PLAYERBOT_AI(bot)->GetGroupLeader()))
            return false;
    }

    if (bot->GetLevel() > 57)
    {
        if ((bot->GetTeamId() == TEAM_ALLIANCE && bot->GetQuestStatus(10119) == QUEST_STATUS_NONE) ||
            (bot->GetTeamId() == TEAM_HORDE && bot->GetQuestStatus(9407) == QUEST_STATUS_NONE))
        {
            if (!bot->IsInCombat())
            {
                if (bot->GetTeamId() == TEAM_ALLIANCE)
                {
                    Quest const* quest = sObjectMgr->GetQuestTemplate(10119);
                    CreatureData const* creatureData = sRandomPlayerbotMgr.GetCreatureDataByEntry(16841);
                    if (quest && creatureData)
                    {
                        auto creatureBounds =
                            bot->GetMap()->GetCreatureBySpawnIdStore().equal_range(creatureData->spawnId);
                        if (creatureBounds.first != creatureBounds.second)
                            bot->AddQuest(quest, creatureBounds.first->second);
                    }
                }
                else
                {
                    Quest const* quest = sObjectMgr->GetQuestTemplate(9407);
                    CreatureData const* creatureData = sRandomPlayerbotMgr.GetCreatureDataByEntry(19254);
                    if (quest && creatureData)
                    {
                        auto creatureBounds =
                            bot->GetMap()->GetCreatureBySpawnIdStore().equal_range(creatureData->spawnId);
                        if (creatureBounds.first != creatureBounds.second)
                            bot->AddQuest(quest, creatureBounds.first->second);
                    }
                }
            }

            return MoveTo(0, -11815.1f, -3190.39f, -30.7338f, false, true);
        }

        return MoveTo(0, -11906.9f, -3208.53f, -14.8616f, false, true);
    }

    return false;
}

bool MoveToDarkPortalAction::isUseful() { return bot->GetLevel() > 54; }

bool DarkPortalAzerothAction::Execute(Event /*event*/)
{
    if (bot->GetLevel() > 57)
    {
        WorldPacket packet(CMSG_AREATRIGGER);
        packet << 4354;

        return GET_PLAYERBOT_AI(bot)->DoSpecificAction("reach area trigger", Event("travel action", packet));
    }

    return false;
}

bool DarkPortalAzerothAction::isUseful() { return bot->GetLevel() > 57; }

bool MoveFromDarkPortalAction::Execute(Event /*event*/)
{
    RESET_AI_VALUE(GuidPosition, "rpg target");

    if (bot->GetTeamId() == TEAM_ALLIANCE)
        return MoveTo(530, -319.261f, 1027.213, 54.172638f, false, true);

    return MoveTo(530, -180.444f, 1027.947, 54.181538f, false, true);
}
