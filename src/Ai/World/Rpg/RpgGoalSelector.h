#ifndef PLAYERBOTS_RPGGOALSELECTOR_H
#define PLAYERBOTS_RPGGOALSELECTOR_H

#include "Define.h"

class Player;
class PlayerbotAI;

enum class RpgGoal : uint8
{
    None        = 0,
    Maintenance = 1,
    Questing    = 2,
    Leisure     = 3,
};

struct RpgGoalResult
{
    RpgGoal goal{RpgGoal::None};
    float   score{0.0f};
};

// Probability (0-100 %) of forcing a leisure break at each IDLE evaluation.
// Declared here so IntentionalChangeStatus can reference it.
constexpr float kLeisureBreakChance = 15.0f;

// Scores the high-level goals an idle bot can pursue from its actual state
// (maintenance needs, quest log, ...). All methods are cheap: no grid searches,
// no DB access — safe to call on map threads at IDLE and on a coarse throttle.
class RpgGoalSelector
{
public:
    static float ScoreMaintenance(PlayerbotAI* botAI);
    static float ScoreQuesting(PlayerbotAI* botAI);
    static float ScoreLeisure();
    // Deterministic per-bot multiplier in [0.85, 1.15] so identical bots
    // don't all pick the same goal. Stable across calls.
    static float JitterFor(Player* bot, RpgGoal goal);
    // Jittered score of a single goal (for incumbent comparison).
    static float ScoreGoal(PlayerbotAI* botAI, RpgGoal goal);
    // Returns true when a challenger goal beats the incumbent by kIncumbentMargin.
    static bool ShouldPreempt(PlayerbotAI* botAI, RpgGoal incumbent);
};

#endif
