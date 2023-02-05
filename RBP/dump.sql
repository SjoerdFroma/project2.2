CREATE TABLE IF NOT EXISTS flexdata (           
  `id` int(11) NOT NULL auto_increment,
  `node_id` int(11) NOT NULL,
  `time` time NOT NULL current_timestamp(),
  `date` date NOT NULL current_timestamp(),
  `stretch` float NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;