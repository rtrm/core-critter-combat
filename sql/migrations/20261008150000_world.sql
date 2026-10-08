DROP PROCEDURE IF EXISTS add_migration;
DELIMITER ??
CREATE PROCEDURE `add_migration`()
BEGIN
DECLARE v INT DEFAULT 1;
SET v = (SELECT COUNT(*) FROM `migrations` WHERE `id`='20261008150000');
IF v = 0 THEN
INSERT INTO `migrations` VALUES ('20261008150000');
-- Add your query below.

-- Critter Combat (ARCHITECTURE.md Milestone 1 step 5): Capture. A wild species' capture reward is
-- its own data, not a hardcoded id in C++ - same reasoning as `pet_battle_ability`/`pet_battle_abilities`.
-- `pet_battle_wild`'s own comment (20261007150600_world.sql) already worked out that a successful
-- Capture against wild Prairie Dog (entry 2620) just grants the existing companion Summon spell
-- 60023 "Summon Companion: Prairie Dog" (entry 14421's spell) - no new Summon-equivalent needed for
-- this pilot. 0 means "not capturable yet" for any future wild entry added with no reward authored.

ALTER TABLE `pet_battle_wild` ADD COLUMN `capture_spell_id` INT UNSIGNED NOT NULL DEFAULT 0 AFTER `entry`;

UPDATE `pet_battle_wild` SET `capture_spell_id` = 60023 WHERE `entry` = 2620;

-- End of migration.
END IF;
END??
DELIMITER ;
CALL add_migration();
DROP PROCEDURE IF EXISTS add_migration;
