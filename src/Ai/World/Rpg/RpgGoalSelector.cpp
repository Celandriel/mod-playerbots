#include "RpgGoalSelector.h"

#include <algorithm>

#include "AiObjectContext.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Player.h"
#include "QuestDef.h"
#include "TravelMgr.h"

// --- Scoring constants (tune here; kept out of config intentionally) ---
constexpr float kMaintenanceRepair   = 35.0f;
constexpr float kMaintenanceVendor   = 25.0f;
constexpr float kMaintenanceAh       = 20.0f;
constexpr float kMaintenanceTrain    = 15.0f;
constexpr float kMaintenanceHomeBind = 10.0f;
constexpr float kMaintenanceCap      = 95.0f;

constexpr float kQuestingCap         = 95.0f;

// Leisure is a constant — no needs, no log state, just a floor so it's never
// starved out entirely when maintenance and questing are both low.
constexpr float kLeisureBase         = 20.0f;

// Jitter band: +/-15 % around 1.0, derived from bot GUID (stable, no RNG).
constexpr float kJitterMin           = 0.85f;
constexpr float kJitterMax           = 1.15f;

// A challenger goal must exceed the incumbent by this factor to preempt it.
constexpr float kIncumbentMargin     = 1.25f;

// kLeisureBreakChance is declared in RpgGoalSelector.h (used by NewRpgAction.cpp).

float RpgGoalSelector::ScoreMaintenance(PlayerbotAI* botAI)
{
    AiObjectContext* ctx = botAI->GetAiObjectContext();

    bool const needRepair = ctx->GetValue<bool>("should repair")->Get() &&
                            ctx->GetValue<bool>("can repair")->Get();
    bool const needVendor = ctx->GetValue<bool>("should sell")->Get() &&
                            ctx->GetValue<bool>("can sell")->Get();
    bool const needTrain  = ctx->GetValue<bool>("can train")->Get() &&
                            ctx->GetValue<uint32>("train cost")->Get() > 0;
    bool const needInn    = ctx->GetValue<bool>("should home bind")->Get();

    bool needAh = false;
    if (sPlayerbotAIConfig.enableAuctionHouseBotting)
        needAh = ctx->GetValue<bool>("should ah sell")->Get() ||
                 ctx->GetValue<bool>("should ah buy")->Get();

    float score = 0.0f;
    if (needRepair)
        score += kMaintenanceRepair;
    if (needVendor)
        score += kMaintenanceVendor;
    if (needAh)
        score += kMaintenanceAh;
    if (needTrain)
        score += kMaintenanceTrain;
    if (needInn)
        score += kMaintenanceHomeBind;

    return std::min(score, kMaintenanceCap);
}

float RpgGoalSelector::ScoreQuesting(PlayerbotAI* botAI)
{
    Player* bot = botAI->GetBot();

    // Count workable incomplete objectives and pending turn-ins.
    uint32 workableIncomplete = 0;
    uint32 turnInsPending     = 0;

    for (uint8 slot = 0; slot < MAX_QUEST_LOG_SIZE; ++slot)
    {
        uint32 const questId = bot->GetQuestSlotQuestId(slot);
        if (!questId)
            continue;

        if (botAI->lowPriorityQuest.find(questId) != botAI->lowPriorityQuest.end())
            continue;

        // Require POI data so we can actually navigate to the quest objective.
        QuestPOIVector const* poiVec = sObjectMgr->GetQuestPOIVector(questId);
        if (!poiVec || poiVec->empty())
            continue;

        uint8 const status = bot->GetQuestStatus(questId);
        if (status == QUEST_STATUS_COMPLETE)
            ++turnInsPending;
        else if (status == QUEST_STATUS_INCOMPLETE)
            ++workableIncomplete;
    }

    // Score: each additional incomplete is worth 8 pts past the first (30 pt
    // floor), turn-ins score 25 each. When the log is thin and the hub index
    // has fresh quests, guarantee at least 40 pts so the questing pipeline
    // stays competitive against leisure (constant 20).
    float logScore = (workableIncomplete >= 1 ? 30.0f + 8.0f * static_cast<float>(workableIncomplete - 1) : 0.0f)
                     + 25.0f * static_cast<float>(turnInsPending);

    // Hub scan only when the thin-log floor can matter — it walks the full
    // hub index and allocates, too heavy to run unconditionally.
    if (workableIncomplete < 3 && !sTravelMgr.GetQuestHubsForBot(bot, botAI->rpgInfo.exhaustedHubs).empty())
        logScore = std::max(logScore, 40.0f);

    return std::min(logScore, kQuestingCap);
}

float RpgGoalSelector::ScoreLeisure()
{
    return kLeisureBase;
}

float RpgGoalSelector::JitterFor(Player* bot, RpgGoal goal)
{
    // Mix GUID counter with the goal id via a Knuth multiplicative hash.
    // No call to urand -- must be stable between calls for the same bot/goal.
    uint32 h = bot->GetGUID().GetCounter() ^ (static_cast<uint32>(goal) * 2654435761u);
    h ^= h >> 16;
    h *= 0x45d9f3b;
    h ^= h >> 16;
    float const t = static_cast<float>(h) / static_cast<float>(0xFFFFFFFFu);
    return kJitterMin + t * (kJitterMax - kJitterMin);
}

float RpgGoalSelector::ScoreGoal(PlayerbotAI* botAI, RpgGoal goal)
{
    float base = 0.0f;
    switch (goal)
    {
        case RpgGoal::Maintenance:
            base = ScoreMaintenance(botAI);
            break;
        case RpgGoal::Questing:
            base = ScoreQuesting(botAI);
            break;
        case RpgGoal::Leisure:
            base = ScoreLeisure();
            break;
        default:
            return 0.0f;
    }
    return base * JitterFor(botAI->GetBot(), goal);
}

bool RpgGoalSelector::ShouldPreempt(PlayerbotAI* botAI, RpgGoal incumbent)
{
    float const incumbentScore = ScoreGoal(botAI, incumbent);
    for (RpgGoal g : {RpgGoal::Maintenance, RpgGoal::Questing, RpgGoal::Leisure})
    {
        if (g == incumbent)
            continue;
        if (ScoreGoal(botAI, g) > incumbentScore * kIncumbentMargin)
            return true;
    }
    return false;
}
