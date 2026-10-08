/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "CritterAI.h"
#include "Creature.h"
#include "SpellEntry.h"

#define ESCAPE_TIMER 30000

int CritterAI::Permissible(Creature const* creature)
{
    if (creature->GetCreatureInfo()->type == CREATURE_TYPE_CRITTER)
        return PERMIT_BASE_SPECIAL;

    return PERMIT_BASE_NO;
}

void CritterAI::DamageTaken(Unit* pWho, uint32& uiDamage)
{
    if (uiDamage < m_creature->GetHealth())
    {
        if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != TIMED_FLEEING_MOTION_TYPE)
            m_creature->GetMotionMaster()->MoveFleeing(pWho, ESCAPE_TIMER);
        m_uiCombatTimer = ESCAPE_TIMER;
    }
}

// Critter Combat (see the critter-combat repo's ARCHITECTURE.md): Engage Critter Combat, whose
// SPELL_EFFECT_DUMMY starts a pet battle. It deals no damage and isn't flagged positive, so
// without this it reads to SpellHit below as exactly the kind of hostile poke a critter should
// flee from - and does, even though Player::StartPetBattle (PetBattleCombat.cpp) already froze
// this creature's movement by the time this runs: AI::SpellHit fires after effect processing, so
// the flee it would otherwise queue here overrides that freeze within the same hit.
#define SPELL_ENGAGE_CRITTER_COMBAT 64000

void CritterAI::SpellHit(SpellCaster* pCaster, SpellEntry const* pSpell)
{
    if (pSpell->Id == SPELL_ENGAGE_CRITTER_COMBAT)
        return;

    if (!pSpell->IsPositiveSpell() && !pSpell->IsDirectDamageSpell() && m_creature->IsAlive())
    {
        if (m_creature->GetMotionMaster()->GetCurrentMovementGeneratorType() != TIMED_FLEEING_MOTION_TYPE)
            if (Unit* pUnit = pCaster->ToUnit())
                m_creature->GetMotionMaster()->MoveFleeing(pUnit, ESCAPE_TIMER);
        m_uiCombatTimer = ESCAPE_TIMER;
    }
}

void CritterAI::UpdateAI(uint32 const diff)
{
    if (m_creature->IsInCombat())
    {
        if (m_uiCombatTimer <= diff)
        {
            EnterEvadeMode();
            m_uiCombatTimer = ESCAPE_TIMER;
        }
        else
            m_uiCombatTimer -= diff;
    }
}

