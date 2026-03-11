DROP TABLE IF EXISTS `playerbots_dungeon_waypoint`;
CREATE TABLE `playerbots_dungeon_waypoint` (
    `map_id` INT UNSIGNED NOT NULL,
    `dungeon_name` VARCHAR(64) NOT NULL,
    `order_index` INT UNSIGNED NOT NULL,
    `x` FLOAT NOT NULL,
    `y` FLOAT NOT NULL,
    `z` FLOAT NOT NULL,
    `jump` TINYINT(1) DEFAULT 0,
    `pause` INT UNSIGNED DEFAULT 0,
    `healer_mana_pct` FLOAT DEFAULT 0.6,
    `next_index` INT UNSIGNED DEFAULT 0,
    `interact_type` TINYINT(1) DEFAULT 0,
    `interact_guid` INT UNSIGNED DEFAULT 0,
    `interact_param` INT DEFAULT 0,
    `comment` VARCHAR(255) DEFAULT '',
    `tell` TINYINT(1) DEFAULT 0,
    PRIMARY KEY (`map_id`, `dungeon_name`, `order_index`)
);
