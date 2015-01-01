-- *  Description: This SQL script creates the table 
-- *  that will store the infromation from the gps
-- *  
-- *  This program is free software: you can redistribute it and/or modify 
-- *  it under the terms of the GNU General Public License as published by 
-- *  the Free Software Foundation, either version 3 of the License, or 
-- *  (at your option) any later version. 
-- *  
-- *  This program is distributed in the hope that it will be useful, 
-- *  but WITHOUT ANY WARRANTY; without even the implied warranty of 
-- *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the 
-- *  GNU General Public License for more details.  
-- *
-- *  Version 1.0
-- *  Author: Clint Bhola

-- MySQL dump 10.13  Distrib 5.1.62, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: my-database
-- ------------------------------------------------------
-- Server version   5.1.62-0ubuntu0.10.04.1

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_UNIQUE_CHECKS=@@UNIQUE_CHECKS, UNIQUE_CHECKS=0 */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;

--
-- Table structure for table `tracker`
--

DROP TABLE IF EXISTS `tracker`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `tracker` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `date` int(11) DEFAULT NULL,
  `lat` NUMERIC(18,12) DEFAULT NULL,
  `lng` NUMERIC(18,12) DEFAULT NULL,
  `speed` float DEFAULT NULL,
  `course` varchar(5) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
