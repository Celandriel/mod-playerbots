/*
 * This file is part of the mod-playerbots module for AzerothCore. See AUTHORS file for Copyright
 * information; released under GNU GPL v2 license, redistribute/modify under version 2 of the License,
 * or (at your option) any later version.
 */

#include "GoAction.h"
#include "Event.h"
#include "Formations.h"
#include "ObjectMgr.h"
#include "PathGenerator.h"
#include "PlayerbotTextMgr.h"
#include "Playerbots.h"
#include "PositionValue.h"
#include "ServerFacade.h"
#include "TravelAction.h"

#include <algorithm>

std::vector<std::string> split(std::string const s, char delim);
char* strstri(char const* haystack, char const* needle);

bool GoAction::Execute(Event event)
{
    Player* master = GetMaster();
    if (!master)
        return false;

    std::string const param = event.getParam();
    if (param == "?")
    {
        float x = bot->GetPositionX();
        float y = bot->GetPositionY();
        Map2ZoneCoordinates(x, y, bot->GetZoneId());

        std::ostringstream out;
        out << "I am at " << x << "," << y;
        botAI->TellMaster(out.str());
        return true;
    }

    if (param.rfind("travel ", 0) == 0 && param.size() > 7)
        return GoTravelTo(param.substr(7));

    GuidVector gos = ChatHelper::parseGameobjects(param);
    if (!gos.empty())
    {
        for (ObjectGuid const guid : gos)
        {
            if (GameObject* go = botAI->GetGameObject(guid))
                if (go->isSpawned())
                {
                    if (ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(bot, go),
                                                             sPlayerbotAIConfig.ReactDistance))
                    {
                        botAI->TellError("It is too far away");
                        return false;
                    }

                    std::ostringstream out;
                    out << "Moving to " << ChatHelper::FormatGameobject(go);
                    botAI->TellMasterNoFacing(out.str());
                    return MoveNear(bot->GetMapId(), go->GetPositionX(), go->GetPositionY(), go->GetPositionZ() + 0.5f,
                                    sPlayerbotAIConfig.FollowDistance);
                }
        }
        return false;
    }

    GuidVector units;
    GuidVector npcs = AI_VALUE(GuidVector, "nearest npcs");
    units.insert(units.end(), npcs.begin(), npcs.end());
    GuidVector players = AI_VALUE(GuidVector, "nearest friendly players");
    units.insert(units.end(), players.begin(), players.end());
    for (ObjectGuid const guid : units)
    {
        if (Unit* unit = botAI->GetUnit(guid))
            if (strstri(unit->GetName().c_str(), param.c_str()))
            {
                std::ostringstream out;
                out << "Moving to " << unit->GetName();
                botAI->TellMasterNoFacing(out.str());
                return MoveNear(bot->GetMapId(), unit->GetPositionX(), unit->GetPositionY(),
                                unit->GetPositionZ() + 0.5f, sPlayerbotAIConfig.FollowDistance);
            }
    }

    if (param.find(";") != std::string::npos)
    {
        std::vector<std::string> coords = split(param, ';');
        float x = atof(coords[0].c_str());
        float y = atof(coords[1].c_str());
        float z;
        if (coords.size() > 2)
            z = atof(coords[2].c_str());
        else
            z = bot->GetPositionZ();

        if (botAI->HasStrategy("debug move", BOT_STATE_NON_COMBAT))
        {
            PathGenerator path(bot);
            path.CalculatePath(x, y, z, false);

            Movement::Vector3 end = path.GetEndPosition();
            Movement::Vector3 aend = path.GetActualEndPosition();

            Movement::PointsArray const& points = path.GetPath();
            PathType type = path.GetPathType();

            std::ostringstream out;

            out << x << ";" << y << ";" << z << " =";

            out << "path is: ";

            out << type;

            out << " of length ";

            out << points.size();

            out << " with offset ";

            out << (end - aend).length();

            for (auto i : points)
            {
                CreateWp(bot, i.x, i.y, i.z, 0.f, 11144);
            }

            botAI->TellMaster(out);
        }

        if (bot->IsWithinLOS(x, y, z))
            return MoveNear(bot->GetMapId(), x, y, z, 0);
        else
            return MoveTo(bot->GetMapId(), x, y, z, false, false);

        return true;
    }

    if (param.find(",") != std::string::npos)
    {
        std::vector<std::string> coords = split(param, ',');
        float x = atof(coords[0].c_str());
        float y = atof(coords[1].c_str());
        Zone2MapCoordinates(x, y, bot->GetZoneId());

        Map* map = bot->GetMap();
        float z = bot->GetPositionZ();
        bot->UpdateAllowedPositionZ(x, y, z);

        if (ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(bot, x, y),
                                                 sPlayerbotAIConfig.ReactDistance))
        {
            botAI->TellMaster("It is too far away");
            return false;
        }

        if (map->IsInWater(bot->GetPhaseMask(), x, y, z, bot->GetCollisionHeight()))
        {
            botAI->TellError("It is in water");
            return false;
        }

        float ground = map->GetHeight(x, y, z + 0.5f);
        if (ground <= INVALID_HEIGHT)
        {
            botAI->TellError("I can't go there");
            return false;
        }

        float x1 = x, y1 = y;
        Map2ZoneCoordinates(x1, y1, bot->GetZoneId());

        std::ostringstream out;
        out << "Moving to " << x1 << "," << y1;
        botAI->TellMasterNoFacing(out.str());

        return MoveNear(bot->GetMapId(), x, y, z + 0.5f, sPlayerbotAIConfig.FollowDistance);
    }

    PositionInfo pos = context->GetValue<PositionMap&>("position")->Get()[param];
    if (pos.isSet())
    {
        if (ServerFacade::instance().IsDistanceGreaterThan(ServerFacade::instance().GetDistance2d(bot, pos.x, pos.y),
                                                 sPlayerbotAIConfig.ReactDistance))
        {
            botAI->TellError("It is too far away");
            return false;
        }

        std::ostringstream out;
        out << "Moving to position " << param;
        botAI->TellMasterNoFacing(out.str());
        return MoveNear(bot->GetMapId(), pos.x, pos.y, pos.z + 0.5f, sPlayerbotAIConfig.FollowDistance);
    }

    botAI->TellMaster("Whisper 'go x,y', 'go [game object]', 'go unit' or 'go position' and I will go there");
    return false;
}

bool GoAction::GoTravelTo(std::string const& name)
{
    if (name == "cancel")
    {
        if (!GoTravel::IsActive(botAI))
        {
            botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
                "go_travel_not_traveling", "I'm not traveling anywhere", {}));
            return false;
        }

        GoTravel::End(botAI, true);
        botAI->TellMasterNoFacing(
            PlayerbotTextMgr::instance().GetBotTextOrDefault("go_travel_cancelled", "Travel cancelled", {}));
        return true;
    }

    // game_tele names have no spaces ("TheBarrens"); the lookup itself ignores case.
    std::string teleName = name;
    teleName.erase(std::remove(teleName.begin(), teleName.end(), ' '), teleName.end());

    // An empty name would substring-match the first game_tele entry.
    GameTele const* tele = teleName.empty() ? nullptr : sObjectMgr->GetGameTele(teleName);
    if (!tele)
    {
        botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
            "go_travel_unknown", "I don't know where %destination is", {{"%destination", name}}));
        return false;
    }

    GoTravel::Start(botAI,
                    WorldPosition(tele->mapId, tele->position_x, tele->position_y, tele->position_z, tele->orientation),
                    tele->name);

    botAI->TellMasterNoFacing(PlayerbotTextMgr::instance().GetBotTextOrDefault(
        "go_travel_start", "Traveling to %destination", {{"%destination", tele->name}}));
    return true;
}
