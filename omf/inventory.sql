-- MySQL dump 10.13  Distrib 5.1.37, for debian-linux-gnu (i486)
--
-- Host: localhost    Database: inventory
-- ------------------------------------------------------
-- Server version	5.1.37-2

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
-- Table structure for table `device_kinds`
--

DROP TABLE IF EXISTS `device_kinds`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `device_kinds` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for device',
  `inventory_id` int(11) NOT NULL COMMENT 'the inventory when we caught this device type',
  `bus` varchar(16) DEFAULT NULL COMMENT 'e.g. pci or usb',
  `vendor` int(11) NOT NULL COMMENT 'id of vendor from /sys',
  `device` int(11) NOT NULL COMMENT 'id of device from /sys',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `device_kinds`
--

LOCK TABLES `device_kinds` WRITE;
/*!40000 ALTER TABLE `device_kinds` DISABLE KEYS */;
/*!40000 ALTER TABLE `device_kinds` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `device_ouis`
--

DROP TABLE IF EXISTS `device_ouis`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `device_ouis` (
  `oui` char(8) NOT NULL COMMENT 'OUI as string XX:XX:XX',
  `device_kind_id` int(11) NOT NULL COMMENT 'link to corresponding entry in device_kinds',
  `inventory_id` int(11) DEFAULT NULL COMMENT 'if generated automatically, id of inventory run, otherwise NULL'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `device_ouis`
--

LOCK TABLES `device_ouis` WRITE;
/*!40000 ALTER TABLE `device_ouis` DISABLE KEYS */;
/*!40000 ALTER TABLE `device_ouis` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `device_tags`
--

DROP TABLE IF EXISTS `device_tags`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `device_tags` (
  `tag` varchar(64) NOT NULL COMMENT 'name for this tag',
  `device_kind_id` int(11) NOT NULL COMMENT 'link to corresponding entry in device_kinds',
  `inventory_id` int(11) DEFAULT NULL COMMENT 'if generated automatically, id of inventory run, otherwise NULL'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `device_tags`
--

LOCK TABLES `device_tags` WRITE;
/*!40000 ALTER TABLE `device_tags` DISABLE KEYS */;
/*!40000 ALTER TABLE `device_tags` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `devices`
--

DROP TABLE IF EXISTS `devices`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `devices` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for device',
  `device_kind_id` int(11) NOT NULL COMMENT 'link to corresponding entry in device_kinds',
  `motherboard_id` int(11) DEFAULT NULL COMMENT 'link to corresponding entry in motherboards',
  `inventory_id` int(11) NOT NULL COMMENT 'link to corresponding entry in inventories',
  `address` varchar(18) NOT NULL COMMENT 'bus address of this device.  MUST sort lexically by bus',
  `mac` varchar(17) DEFAULT NULL COMMENT 'MAC address of this device, if it is a network device',
  `canonical_name` varchar(64) DEFAULT NULL COMMENT 'a good guess as to the name Linux will give to this device',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `devices`
--

LOCK TABLES `devices` WRITE;
/*!40000 ALTER TABLE `devices` DISABLE KEYS */;
INSERT INTO `devices` VALUES (1,1,1,1,'',NULL,NULL);
/*!40000 ALTER TABLE `devices` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `inventories`
--

DROP TABLE IF EXISTS `inventories`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `inventories` (
  `id` int(11) NOT NULL COMMENT 'obligatiory unique id',
  `opened` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP COMMENT 'start of inventory run',
  `closed` timestamp NOT NULL DEFAULT '0000-00-00 00:00:00' COMMENT 'end of inventory run, or 0000-etc. if not done yet',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `inventories`
--

LOCK TABLES `inventories` WRITE;
/*!40000 ALTER TABLE `inventories` DISABLE KEYS */;
INSERT INTO `inventories` VALUES (1,'2009-11-04 05:14:21','2009-11-04 05:14:21');
/*!40000 ALTER TABLE `inventories` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `locations`
--

DROP TABLE IF EXISTS `locations`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `locations` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for location',
  `x` int(11) NOT NULL DEFAULT '0' COMMENT 'logical x address of location',
  `y` int(11) NOT NULL DEFAULT '0' COMMENT 'logical y address of location',
  `z` int(11) NOT NULL DEFAULT '0' COMMENT 'logical z address of location',
  `latitude` float DEFAULT NULL COMMENT 'latitude of this location or NULL',
  `longitude` float DEFAULT NULL COMMENT 'longitude of this location or NULL',
  `elevation` float DEFAULT NULL COMMENT 'elevation of this location or NULL',
  `testbed_id` int(11) NOT NULL,
  `name` varchar(64) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `locations`
--

LOCK TABLES `locations` WRITE;
/*!40000 ALTER TABLE `locations` DISABLE KEYS */;
INSERT INTO `locations` VALUES (1,1,1,1,NULL,NULL,NULL,1,NULL),(2,1,2,1,NULL,NULL,NULL,1,NULL),(3,1,3,1,NULL,NULL,NULL,1,NULL),(4,1,4,1,NULL,NULL,NULL,1,NULL);
/*!40000 ALTER TABLE `locations` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `motherboards`
--

DROP TABLE IF EXISTS `motherboards`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `motherboards` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for motherboard',
  `inventory_id` int(11) NOT NULL COMMENT 'link to corresponding entry in inventories',
  `mfr_sn` varchar(128) DEFAULT NULL COMMENT 'manufacturer serial number of the motherboard',
  `cpu_type` varchar(64) DEFAULT NULL COMMENT 'name of CPU as given by vendor',
  `cpu_n` int(11) DEFAULT NULL COMMENT 'number of CPUs',
  `cpu_hz` float DEFAULT NULL COMMENT 'CPU speed in MHz',
  `hd_sn` varchar(64) DEFAULT NULL COMMENT 'hard drive serial number, NULL if no hd',
  `hd_size` int(11) DEFAULT NULL COMMENT 'hard disk size in bytes',
  `hd_status` tinyint(1) DEFAULT '1' COMMENT 'true means drive probably okay',
  `memory` int(11) DEFAULT NULL COMMENT 'memory size in bytes',
  PRIMARY KEY (`id`),
  UNIQUE KEY `mfr_sn` (`mfr_sn`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `motherboards`
--

LOCK TABLES `motherboards` WRITE;
/*!40000 ALTER TABLE `motherboards` DISABLE KEYS */;
INSERT INTO `motherboards` VALUES (1,1,NULL,NULL,NULL,NULL,NULL,NULL,1,NULL);
/*!40000 ALTER TABLE `motherboards` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `nodeToIP`
--

DROP TABLE IF EXISTS `nodeToIP`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `nodeToIP` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `identifier` varchar(50) NOT NULL COMMENT 'currently, the MAC address of the node',
  `ip` varchar(25) NOT NULL COMMENT 'the control ip for the node',
  PRIMARY KEY (`id`),
  UNIQUE KEY `identifier` (`identifier`)
) ENGINE=MyISAM AUTO_INCREMENT=6 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `nodeToIP`
--

LOCK TABLES `nodeToIP` WRITE;
/*!40000 ALTER TABLE `nodeToIP` DISABLE KEYS */;
INSERT INTO `nodeToIP` VALUES (2,'00:0c:29:df:9f:c9','128.105.48.100'),(3,'00:0c:29:60:09:d8','128.105.48.133'),(4,'00:1e:8c:2b:bc:6b','128.105.48.127'),(5,'00:0f:b5:af:6b:16','128.105.48.177');
/*!40000 ALTER TABLE `nodeToIP` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `nodes`
--

DROP TABLE IF EXISTS `nodes`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `nodes` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for nodes',
  `inventory_id` int(11) NOT NULL COMMENT 'link to corresponding entry in inventories',
  `chassis_sn` varchar(64) DEFAULT NULL COMMENT 'manufacturer serial number of the chassis of the node; optionally null',
  `motherboard_id` int(11) NOT NULL COMMENT 'the motherboard in this node',
  `location_id` int(11) DEFAULT NULL COMMENT 'the location of this node',
  `control_ip` varchar(64) DEFAULT NULL,
  `pxeimage_id` int(11) DEFAULT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `location_id` (`location_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `nodes`
--

LOCK TABLES `nodes` WRITE;
/*!40000 ALTER TABLE `nodes` DISABLE KEYS */;
INSERT INTO `nodes` VALUES (1,1,NULL,1,1,'128.105.48.100',1),(2,1,NULL,1,2,'128.105.48.133',1),(0,1,NULL,1,3,'128.105.48.127',1),(3,1,NULL,1,4,'128.105.48.177',1);
/*!40000 ALTER TABLE `nodes` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `pxeimages`
--

DROP TABLE IF EXISTS `pxeimages`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `pxeimages` (
  `id` int(11) DEFAULT NULL,
  `image_name` varchar(64) DEFAULT NULL,
  `short_description` varchar(128) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `pxeimages`
--

LOCK TABLES `pxeimages` WRITE;
/*!40000 ALTER TABLE `pxeimages` DISABLE KEYS */;
INSERT INTO `pxeimages` VALUES (1,'foobar',NULL);
/*!40000 ALTER TABLE `pxeimages` ENABLE KEYS */;
UNLOCK TABLES;

--
-- Table structure for table `testbeds`
--

DROP TABLE IF EXISTS `testbeds`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `testbeds` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for testbed',
  `node_domain` varchar(128) NOT NULL COMMENT 'example: grid',
  `x_max` int(11) DEFAULT NULL,
  `y_max` int(11) DEFAULT NULL,
  `z_max` int(11) DEFAULT NULL,
  `control_ip` varchar(64) DEFAULT NULL,
  `data_ip` varchar(64) DEFAULT NULL,
  `cm_ip` varchar(64) DEFAULT NULL,
  `latitude` float DEFAULT NULL COMMENT 'latitude of origin of node_domain or NULL',
  `longitude` float DEFAULT NULL COMMENT 'longitude of origin of node_domain or NULL',
  `elevation` float DEFAULT NULL COMMENT 'elevation of origin of node_domain or NULL',
  `pxe_url` varchar(64) DEFAULT NULL,
  `cmc_url` varchar(64) DEFAULT NULL,
  `oml_url` varchar(64) DEFAULT NULL,
  `result_url` varchar(64) DEFAULT NULL,
  `frisbee_url` varchar(64) DEFAULT NULL,
  `frisbee_default_disk` varchar(64) DEFAULT NULL,
  `image_host` varchar(64) DEFAULT NULL,
  `saveimage_url` varchar(64) NOT NULL,
  PRIMARY KEY (`id`),
  UNIQUE KEY `node_domain` (`node_domain`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Dumping data for table `testbeds`
--

LOCK TABLES `testbeds` WRITE;
/*!40000 ALTER TABLE `testbeds` DISABLE KEYS */;
INSERT INTO `testbeds` VALUES (1,'teratorn',10,10,10,'192.168.3.1',NULL,NULL,NULL,NULL,NULL,'http://128.105.22.78:5022/pxe','http://128.105.22.78:5022/cmc','128.105.22.78',NULL,'http://128.105.22.78:5022','/dev/sda','128.105.22.78','http://128.105.22.78/url');
/*!40000 ALTER TABLE `testbeds` ENABLE KEYS */;
UNLOCK TABLES;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2009-12-10 20:08:06
