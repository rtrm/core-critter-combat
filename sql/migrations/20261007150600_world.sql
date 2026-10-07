DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261007150600');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261007150600');
-- Add your query below.

-- Critter Combat pilot (ARCHITECTURE.md Milestone 1): Prairie Dog. Three new tables, no existing
-- table touched.
--
--   pet_battle_ability    - one row per ability, a NEW custom id space (63000+), not spell_template.
--                           Turn-based 1v1 pet combat has no cast time/GCD/mana/line-of-sight - those
--                           are most of what spell_template's columns exist for, so forcing pet moves
--                           through it would fight the schema more than use it. `effect_type` is a
--                           small fixed enum the battle engine (not written yet) will switch on;
--                           `base_value`/`value_per_level` is this ability's own linear scale by the
--                           pet's current level - no ranks, per ARCHITECTURE.md.
--   pet_battle_abilities  - a creature_template entry's 3 ability ids, in slot order 1-3.
--   pet_battle_wild       - presence alone = this WILD creature_template entry is battleable (shows
--                           the overhead sword-X, is a valid Engage Critter Combat target) and
--                           therefore capturable - ARCHITECTURE.md: no separate capturable flag.
--
-- Prairie Dog needs two different creature_template rows for its wild and owned halves:
--   2620  "Prairie Dog"       - the real wild critter found in zones; flagged battleable here.
--   14421 "Brown Prairie Dog" - milestone 1's existing companion mini-pet creature (its Summon spell,
--                                60023 "Summon Companion: Prairie Dog", already exists) - so a
--                                successful Capture against 2620 just needs to grant spell 60023,
--                                no new Summon-equivalent spell for this pilot.
-- Both rows carry the same three abilities: same species, same kit, wild or owned.

CREATE TABLE IF NOT EXISTS `pet_battle_ability` (
  `id` INT UNSIGNED NOT NULL,
  `name` VARCHAR(100) NOT NULL,
  `effect_type` TINYINT UNSIGNED NOT NULL COMMENT '1=DAMAGE, 2=HIT_CHANCE_DEBUFF (enemy), 3=DAMAGE_TAKEN_SHIELD (self)',
  `base_value` FLOAT NOT NULL,
  `value_per_level` FLOAT NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `pet_battle_abilities` (
  `entry` INT UNSIGNED NOT NULL COMMENT 'creature_template entry',
  `slot` TINYINT UNSIGNED NOT NULL COMMENT '1-3',
  `ability_id` INT UNSIGNED NOT NULL,
  PRIMARY KEY (`entry`, `slot`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

CREATE TABLE IF NOT EXISTS `pet_battle_wild` (
  `entry` INT UNSIGNED NOT NULL COMMENT 'creature_template entry; presence = battleable = capturable',
  PRIMARY KEY (`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

INSERT INTO `pet_battle_ability` (`id`, `name`, `effect_type`, `base_value`, `value_per_level`) VALUES
(63000, 'Nibble', 1, 3, 1.0),
(63001, 'Dust Cloud', 2, 8, 0.5),
(63002, 'Burrow', 3, 15, 0.5);

INSERT INTO `pet_battle_abilities` (`entry`, `slot`, `ability_id`) VALUES
(2620, 1, 63000), (2620, 2, 63001), (2620, 3, 63002),
(14421, 1, 63000), (14421, 2, 63001), (14421, 3, 63002);

INSERT INTO `pet_battle_wild` (`entry`) VALUES (2620);

-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
