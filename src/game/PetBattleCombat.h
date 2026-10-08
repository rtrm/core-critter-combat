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

    // Percent chance (0-95) a Capture attempt succeeds (ARCHITECTURE.md step 5, tuned against live
    // feedback). Gated hard on the enemy's current HP: above 50% it's not attemptable at all (the
    // caller should never even roll - see Player::TryCapturePet), 21-50% is a low fixed chance,
    // and only below 20% does it become likely. Level difference is deliberately asymmetric:
    // fighting something above your own pet's level punishes hard (-20%/level), while fighting
    // something below it only helps a little (+5%/level) - every number here is still a pilot
    // placeholder, not a balanced value.
    uint32 CaptureChance(uint32 playerPetLevel, uint32 enemyLevel, uint32 enemyHpPercent);

    // One full round of a Capture attempt: same turn order rule as ResolveRound (higher level
    // first, current HP tiebreak), but the player's "move" is the catch attempt itself rather than
    // one of the three battle abilities. A miss still costs the round - if the enemy acts (because
    // it went first, or because the player's attempt failed and it was their own turn next), it
    // can still land a hit - so Capture can't be spammed for free. `captureChancePercent` is
    // resolved by the caller (CaptureChance, above), since computing it here would need the
    // enemy's max HP too, which this engine's other entry points don't otherwise pass around.
    struct CaptureOutcome
    {
        bool captured = false;
        uint32 enemyAbilityUsed = 0; // 0 if the enemy never got to act this round
        bool playerActedFirst = true;
        bool battleOver = false; // only set on a loss - a capture ends the battle via `captured` instead
        bool playerWon = false;  // only meaningful when battleOver
    };
    CaptureOutcome ResolveCaptureRound(PetBattleSession& session, uint32 captureChancePercent);
}

#endif // MANGOS_PET_BATTLE_COMBAT_H
