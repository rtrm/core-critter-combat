#ifndef MANGOS_PET_BATTLE_COMBAT_H
#define MANGOS_PET_BATTLE_COMBAT_H

#include "Common.h"

struct PetBattleSession;

// Critter Combat (see the critter-combat repo's ARCHITECTURE.md): the turn-based 1v1 pet battle
// engine. Player::StartPetBattle (begins a battle) and WorldSession::HandlePetBattleUseAbilityOpcode
// (resolves a round) both live here too, alongside the math they share, rather than split across
// Player.cpp and a Handlers/ file - this is all one small, new, self-contained feature.
namespace PetBattleCombat
{
    // Placeholder, not balanced (ARCHITECTURE.md's own standard for every number in this pilot).
    uint32 MaxHp(uint32 level);

    // One full round: the player's chosen ability, and the enemy's (simple random-among-3 AI,
    // ability content itself being a later pass per ARCHITECTURE.md), in turn order (higher level
    // first, current HP tiebreak). Mutates `session`'s HP and pending shield/miss-chance fields in
    // place. An ability id of 0 in the result means that side never got to act this round, because
    // the round already ended before its turn.
    struct RoundOutcome
    {
        uint32 playerAbilityUsed = 0;
        uint32 enemyAbilityUsed = 0;
        bool playerActedFirst = true;
        bool battleOver = false;
        bool playerWon = false; // only meaningful when battleOver
    };
    RoundOutcome ResolveRound(PetBattleSession& session, uint32 playerAbilityId);

    // Percent chance (1-100) a Capture attempt succeeds, keyed off the enemy's level relative to
    // the player's own pet (ARCHITECTURE.md step 5) - 50% at an even match, +/-10% per level of
    // difference, clamped so no matchup is ever a guaranteed catch or a flatly impossible one.
    uint32 CaptureChance(uint32 playerPetLevel, uint32 enemyLevel);
}

#endif // MANGOS_PET_BATTLE_COMBAT_H
