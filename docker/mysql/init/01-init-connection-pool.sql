CREATE DATABASE IF NOT EXISTS connection_pool_dev
  DEFAULT CHARACTER SET utf8mb4
  DEFAULT COLLATE utf8mb4_unicode_ci;

CREATE USER IF NOT EXISTS 'connection_pool_app'@'%'
  IDENTIFIED BY 'wang112233';

GRANT ALL PRIVILEGES ON connection_pool_dev.* TO 'connection_pool_app'@'%';

FLUSH PRIVILEGES;
