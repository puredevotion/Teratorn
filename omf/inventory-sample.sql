-- phpMyAdmin SQL Dump
-- version 3.1.2deb1ubuntu0.1
-- http://www.phpmyadmin.net
--
-- Host: localhost
-- Generation Time: Sep 30, 2009 at 12:44 PM
-- Server version: 5.0.75
-- PHP Version: 5.2.6-3ubuntu4.2

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";


/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET @OLD_CHARACTER_SET_RESULTS=@@CHARACTER_SET_RESULTS */;
/*!40101 SET @OLD_COLLATION_CONNECTION=@@COLLATION_CONNECTION */;
/*!40101 SET NAMES utf8 */;

--
-- Database: `inventory`
--

-- --------------------------------------------------------

--
-- Table structure for table `devices`
--

CREATE TABLE IF NOT EXISTS `devices` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for device',
  `device_kind_id` int(11) NOT NULL COMMENT 'link to corresponding entry in device_kinds',
  `motherboard_id` int(11) default NULL COMMENT 'link to corresponding entry in motherboards',
  `inventory_id` int(11) NOT NULL COMMENT 'link to corresponding entry in inventories',
  `address` varchar(18) NOT NULL COMMENT 'bus address of this device.  MUST sort lexically by bus',
  `mac` varchar(17) default NULL COMMENT 'MAC address of this device, if it is a network device',
  `canonical_name` varchar(64) default NULL COMMENT 'a good guess as to the name Linux will give to this device',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `device_kinds`
--

CREATE TABLE IF NOT EXISTS `device_kinds` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for device',
  `inventory_id` int(11) NOT NULL COMMENT 'the inventory when we caught this device type',
  `bus` varchar(16) default NULL COMMENT 'e.g. pci or usb',
  `vendor` int(11) NOT NULL COMMENT 'id of vendor from /sys',
  `device` int(11) NOT NULL COMMENT 'id of device from /sys',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `device_ouis`
--

CREATE TABLE IF NOT EXISTS `device_ouis` (
  `oui` char(8) NOT NULL COMMENT 'OUI as string XX:XX:XX',
  `device_kind_id` int(11) NOT NULL COMMENT 'link to corresponding entry in device_kinds',
  `inventory_id` int(11) default NULL COMMENT 'if generated automatically, id of inventory run, otherwise NULL'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `device_ouis`
--


-- --------------------------------------------------------

--
-- Table structure for table `device_tags`
--

CREATE TABLE IF NOT EXISTS `device_tags` (
  `tag` varchar(64) NOT NULL COMMENT 'name for this tag',
  `device_kind_id` int(11) NOT NULL COMMENT 'link to corresponding entry in device_kinds',
  `inventory_id` int(11) default NULL COMMENT 'if generated automatically, id of inventory run, otherwise NULL'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Dumping data for table `device_tags`
--


-- --------------------------------------------------------

--
-- Table structure for table `inventories`
--

CREATE TABLE IF NOT EXISTS `inventories` (
  `id` int(11) NOT NULL COMMENT 'obligatiory unique id',
  `opened` timestamp NOT NULL default CURRENT_TIMESTAMP COMMENT 'start of inventory run',
  `closed` timestamp NOT NULL default '0000-00-00 00:00:00' COMMENT 'end of inventory run, or 0000-etc. if not done yet',
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `locations`
--

CREATE TABLE IF NOT EXISTS `locations` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for location',
  `x` int(11) NOT NULL default '0' COMMENT 'logical x address of location',
  `y` int(11) NOT NULL default '0' COMMENT 'logical y address of location',
  `z` int(11) NOT NULL default '0' COMMENT 'logical z address of location',
  `latitude` float default NULL COMMENT 'latitude of this location or NULL',
  `longitude` float default NULL COMMENT 'longitude of this location or NULL',
  `elevation` float default NULL COMMENT 'elevation of this location or NULL',
  `testbed_id` int(11) NOT NULL,
  `name` varchar(64) default NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

-- --------------------------------------------------------

--
-- Table structure for table `motherboards`
--

CREATE TABLE IF NOT EXISTS `motherboards` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for motherboard',
  `inventory_id` int(11) NOT NULL COMMENT 'link to corresponding entry in inventories',
  `mfr_sn` varchar(128) default NULL COMMENT 'manufacturer serial number of the motherboard',
  `cpu_type` varchar(64) default NULL COMMENT 'name of CPU as given by vendor',
  `cpu_n` int(11) default NULL COMMENT 'number of CPUs',
  `cpu_hz` float default NULL COMMENT 'CPU speed in MHz',
  `hd_sn` varchar(64) default NULL COMMENT 'hard drive serial number, NULL if no hd',
  `hd_size` int(11) default NULL COMMENT 'hard disk size in bytes',
  `hd_status` tinyint(1) default '1' COMMENT 'true means drive probably okay',
  `memory` int(11) default NULL COMMENT 'memory size in bytes',
  PRIMARY KEY  (`id`),
  UNIQUE KEY `mfr_sn` (`mfr_sn`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `nodes`
--

CREATE TABLE IF NOT EXISTS `nodes` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for nodes',
  `inventory_id` int(11) NOT NULL COMMENT 'link to corresponding entry in inventories',
  `chassis_sn` varchar(64) default NULL COMMENT 'manufacturer serial number of the chassis of the node; optionally null',
  `motherboard_id` int(11) NOT NULL COMMENT 'the motherboard in this node',
  `location_id` int(11) default NULL COMMENT 'the location of this node',
  `control_ip` varchar(64) default NULL,
  `pxeimage_id` int(11) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `location_id` (`location_id`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `pxeimages`
--

CREATE TABLE IF NOT EXISTS `pxeimages` (
  `id` int(11) default NULL,
  `image_name` varchar(64) default NULL,
  `short_description` varchar(128) default NULL
) ENGINE=MyISAM DEFAULT CHARSET=latin1;

--
-- Table structure for table `testbeds`
--

CREATE TABLE IF NOT EXISTS `testbeds` (
  `id` int(11) NOT NULL COMMENT 'universally unique id for testbed',
  `node_domain` varchar(128) NOT NULL COMMENT 'example: grid',
  `x_max` int(11) default NULL,
  `y_max` int(11) default NULL,
  `z_max` int(11) default NULL,
  `control_ip` varchar(64) default NULL,
  `data_ip` varchar(64) default NULL,
  `cm_ip` varchar(64) default NULL,
  `latitude` float default NULL COMMENT 'latitude of origin of node_domain or NULL',
  `longitude` float default NULL COMMENT 'longitude of origin of node_domain or NULL',
  `elevation` float default NULL COMMENT 'elevation of origin of node_domain or NULL',
  `pxe_url` varchar(64) default NULL,
  `cmc_url` varchar(64) default NULL,
  `oml_url` varchar(64) default NULL,
  `result_url` varchar(64) default NULL,
  `frisbee_url` varchar(64) default NULL,
  `frisbee_default_disk` varchar(64) default NULL,
  `image_host` varchar(64) default NULL,
  PRIMARY KEY  (`id`),
  UNIQUE KEY `node_domain` (`node_domain`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
