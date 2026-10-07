DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261007150700');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261007150700');
-- Add your query below.

-- Critter Combat (ARCHITECTURE.md): a learned pet's persistent battle state, character-bound.
-- Keyed by `summon_spell_id` - the same Summon spell id that already uniquely identifies a learned
-- companion in `character_spell` (e.g. 60023 for Prairie Dog), so a captured wild pet and an
-- existing companion share one identity scheme with no new id space needed here.
--
-- `current_hp` is persistent on purpose (ARCHITECTURE.md "Pet health & death"): it is never reset to
-- max on summon or after a battle, and a pet at 0 is dead - unsummonable until healed by a stable
-- master or a Pet Bandage. `level` is fixed at the moment the pet is learned (capture, or already
-- fixed for an existing companion) - pets do not gain levels after that, per ARCHITECTURE.md.

CREATE TABLE IF NOT EXISTS `character_pet_battle` (
  `guid` INT UNSIGNED NOT NULL,
  `summon_spell_id` INT UNSIGNED NOT NULL,
  `level` TINYINT UNSIGNED NOT NULL,
  `current_hp` INT NOT NULL,
  PRIMARY KEY (`guid`, `summon_spell_id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
