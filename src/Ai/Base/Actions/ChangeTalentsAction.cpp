/*
 * Copyright (C) 2016+ AzerothCore <www.azerothcore.org>, released under GNU AGPL v3 license, you may redistribute it
 * and/or modify it under version 3 of the License, or (at your option), any later version.
 */

#include "ChangeTalentsAction.h"

#include "AiFactory.h"
#include "ChatHelper.h"
#include "Event.h"
#include "PlayerbotAIConfig.h"
#include "PlayerbotFactory.h"
#include "AiObjectContext.h"
#include "Log.h"
#include "RandomPlayerbotMgr.h"

bool ChangeTalentsAction::Execute(Event event)
{
    auto* flag = botAI->GetAiObjectContext()->GetValue<bool>("custom_glyphs"); // Added for custom Glyphs

    if (flag->Get()) // Added for custom Glyphs
    {
        flag->Set(false);
        LOG_INFO("playerbots", "Custom Glyph Flag set to OFF");
    }
    std::string param = event.getParam();

    std::ostringstream out;

    if (!param.empty())
    {
        if (param.find("help") != std::string::npos)
        {
            out << TalentsHelp();
        }
        else if (param.find("switch") != std::string::npos)
        {
            if (param.find("switch 1") != std::string::npos)
            {
                bot->ActivateSpec(0);
                out << "Active first talent";
                botAI->ResetStrategies();
            }
            else if (param.find("switch 2") != std::string::npos)
            {
                if (bot->GetSpecsCount() == 1 && bot->GetLevel() >= sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL))
                {
                    bot->CastSpell(bot, 63680, true, nullptr, nullptr, bot->GetGUID());
                    bot->CastSpell(bot, 63624, true, nullptr, nullptr, bot->GetGUID());
                }
                bot->ActivateSpec(1);
                out << "Active second talent";
                botAI->ResetStrategies();
            }
        }
        else if (param.find("autopick") != std::string::npos)
        {
            PlayerbotFactory factory(bot, bot->GetLevel());
            factory.InitTalentsTree(true);
            out << "Auto pick talents";
            botAI->ResetStrategies();
        }
        else if (param.find("spec list") != std::string::npos)
        {
            out << SpecList();
        }
        else if (param.find("spec ") != std::string::npos)
        {
            param = param.substr(5);
            out << SpecPick(param);
            botAI->ResetStrategies();
        }
        else if (param.find("apply ") != std::string::npos)
        {
            param = param.substr(6);
            out << SpecApply(param);
            botAI->ResetStrategies();
        }
        else
        {
            out << "Unknown command.";
        }
    }
    else
    {
        uint32 tab = AiFactory::GetPlayerSpecTab(bot);
        out << "My current talent spec is: "
            << "|h|cffffffff";
        out << chat->FormatClass(bot, tab) << "\n";
        out << TalentsHelp();
    }

    botAI->TellMaster(out);

    return true;
}

std::string ChangeTalentsAction::TalentsHelp()
{
    std::ostringstream out;
    out << "Talents usage: talents switch <1/2>, talents autopick, talents spec list, "
           "talents spec <specName>, talents apply <link>.";
    return out.str();
}

std::string ChangeTalentsAction::SpecList()
{
    int cls = bot->getClass();
    int specFound = 0;
    std::ostringstream out;
    for (int specNo = 0; specNo < MAX_SPECNO; ++specNo)
    {
        if (sPlayerbotAIConfig.premadeSpecName[cls][specNo].size() == 0)
        {
            break;
        }
        specFound++;
        std::ostringstream out;
        std::vector<std::vector<uint32>> parsed = sPlayerbotAIConfig.parsedSpecLinkOrder[cls][specNo][80];
        std::unordered_map<int, int> tabCount;
        tabCount[0] = tabCount[1] = tabCount[2] = 0;
        for (auto& item : parsed)
        {
            tabCount[item[0]] += item[3];
        }
        out << specFound << ". " << sPlayerbotAIConfig.premadeSpecName[cls][specNo] << " (";
        out << tabCount[0] << "-" << tabCount[1] << "-" << tabCount[2] << ")";
        botAI->TellMasterNoFacing(out.str());
    }
    out << "Total " << specFound << " specs found";
    return out.str();
}

std::string ChangeTalentsAction::SpecPick(std::string param)
{
    int cls = bot->getClass();
    // int specFound = 0; //not used, line marked for removal.
    for (int specNo = 0; specNo < MAX_SPECNO; ++specNo)
    {
        if (sPlayerbotAIConfig.premadeSpecName[cls][specNo].size() == 0)
        {
            break;
        }
        if (sPlayerbotAIConfig.premadeSpecName[cls][specNo] == param)
        {
            PlayerbotFactory::InitTalentsBySpecNo(bot, specNo, true);
            sRandomPlayerbotMgr.SetValue(bot->GetGUID().GetCounter(), "specLink", 0);

            PlayerbotFactory factory(bot, bot->GetLevel());
            factory.InitGlyphs(false);

            std::ostringstream out;
            out << "Picking " << sPlayerbotAIConfig.premadeSpecName[cls][specNo];
            return out.str();
        }
    }
    std::ostringstream out;
    out << "Spec " << param << " not found";
    return out.str();
}

std::string ChangeTalentsAction::SpecApply(std::string param)
{
    int cls = bot->getClass();
    std::ostringstream out;
    std::vector<std::vector<uint32>> parsedSpecLink = PlayerbotAIConfig::ParseTempTalentsOrder(cls, param);
    if (parsedSpecLink.size() == 0)
    {
        out << "Invalid link " << param;
        return out.str();
    }
    PlayerbotFactory::InitTalentsByParsedSpecLink(bot, parsedSpecLink, true);
    // Reset premade spec when applying a a custom spec.
    sRandomPlayerbotMgr.SetValue(bot->GetGUID().GetCounter(), "specNo", 0);
    sRandomPlayerbotMgr.SetValue(bot->GetGUID().GetCounter(), "specLink", 1, param);
    out << "Applying " << param;
    return out.str();
}

bool AutoSetTalentsAction::Execute(Event /*event*/)
{
    std::ostringstream out;

    if (!PlayerbotAIConfig::instance().autoPickTalents || !RandomPlayerbotMgr::instance().IsRandomBot(bot))
        return false;

    if (bot->GetFreeTalentPoints() <= 0)
        return false;

    PlayerbotFactory factory(bot, bot->GetLevel());
    factory.InitTalentsTree(true, true, true);
    factory.InitPetTalents();
    botAI->TellMaster(out);

    return true;
}
