-- Persistent RPG intent table: stores each bot's last known goal, zone, and hub
-- position so the bot can resume its questing plan after a server restart.

CREATE TABLE IF NOT EXISTS `playerbots_rpg_state` (
    `guid` int unsigned NOT NULL,
    `goal` tinyint unsigned NOT NULL DEFAULT 0,
    `zone_id` int unsigned NOT NULL DEFAULT 0,
    `hub_map` int unsigned NOT NULL DEFAULT 0,
    `hub_x` float NOT NULL DEFAULT 0,
    `hub_y` float NOT NULL DEFAULT 0,
    `hub_z` float NOT NULL DEFAULT 0,
    `updated_at` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
