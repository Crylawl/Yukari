CREATE TABLE `users`
(
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
    `username` varchar(32) NOT NULL,
    `password` varchar(32) NOT NULL,
    `max_bots` int(11) DEFAULT '-1',
    `max_time` int(11) DEFAULT '-1',
    `admin` int(10) unsigned DEFAULT '0',
    `logged_in` int(10) unsigned DEFAULT '0',
    `flood_cooldown` int(11) DEFAULT '0',
    PRIMARY KEY (`id`),
    KEY `username` (`username`)
);

CREATE TABLE `log`
(
    `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  	`user` varchar(32) NOT NULL,
  	`duration` int(10) unsigned NOT NULL,
  	`max_bots` int(11) DEFAULT '-1',
  	`command` text NOT NULL,
    `targets` text NOT NULL,
    `port` int(10) unsigned NOT NULL,
    `timestamp` int(10) unsigned NOT NULL,
    PRIMARY KEY (`id`),
  	KEY `user` (`user`)
);
