-- MySQL dump 10.13  Distrib 5.5.40, for debian-linux-gnu (x86_64)
--
-- Host: localhost    Database: socket
-- ------------------------------------------------------
-- Server version	5.5.40-0ubuntu1

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
-- Table structure for table `001  -  DDC4000 Network_EDE`
--

DROP TABLE IF EXISTS `001  -  DDC4000 Network_EDE`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `001  -  DDC4000 Network_EDE` (
  `id` int(3) DEFAULT NULL,
  `id_knx_gateway` int(1) DEFAULT NULL,
  `input_output` int(1) DEFAULT NULL,
  `group_address` varchar(7) DEFAULT NULL,
  `data_type` varchar(1) DEFAULT NULL,
  `description` varchar(67) DEFAULT NULL,
  `enabled` int(1) DEFAULT NULL,
  `hi_low_msg` varchar(10) DEFAULT NULL,
  `low_hi_msg` varchar(10) DEFAULT NULL,
  `is_alarm` int(1) DEFAULT NULL,
  `alarm_transition` int(1) DEFAULT NULL,
  `sms` int(1) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `Foglio1`
--

DROP TABLE IF EXISTS `Foglio1`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `Foglio1` (
  `id` int(3) DEFAULT NULL,
  `id_knx_gateway` int(1) DEFAULT NULL,
  `input_output` int(1) DEFAULT NULL,
  `group_address` varchar(8) DEFAULT NULL,
  `data_type` varchar(1) DEFAULT NULL,
  `description` varchar(47) DEFAULT NULL,
  `enabled` int(1) DEFAULT NULL,
  `hi_low_msg` varchar(51) DEFAULT NULL,
  `low_hi_msg` varchar(50) DEFAULT NULL,
  `is_alarm` int(1) DEFAULT NULL,
  `alarm_transition` int(1) DEFAULT NULL,
  `sms` int(1) DEFAULT NULL,
  `M` varchar(10) DEFAULT NULL,
  `N` varchar(10) DEFAULT NULL,
  `O` varchar(10) DEFAULT NULL,
  `P` varchar(10) DEFAULT NULL
) ENGINE=MyISAM DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `ai_input_output`
--

DROP TABLE IF EXISTS `ai_input_output`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `ai_input_output` (
  `id_ai_input_output` int(11) NOT NULL AUTO_INCREMENT,
  `id_digital` int(11) DEFAULT NULL,
  `id_digital_out` int(11) DEFAULT NULL,
  `id_sistema` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id_ai_input_output`),
  UNIQUE KEY `id_sistema` (`id_digital`,`id_digital_out`,`id_sistema`),
  KEY `id_sistema_2` (`id_sistema`),
  KEY `id_digital_out` (`id_digital_out`),
  CONSTRAINT `ai_input_output_ibfk_1` FOREIGN KEY (`id_sistema`) REFERENCES `ai_sistemi` (`id_sistema`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `ai_input_output_ibfk_2` FOREIGN KEY (`id_digital`) REFERENCES `digital` (`id_digital`) ON DELETE CASCADE ON UPDATE CASCADE,
  CONSTRAINT `ai_input_output_ibfk_3` FOREIGN KEY (`id_digital_out`) REFERENCES `digital_out` (`id_digital_out`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `ai_sistemi`
--

DROP TABLE IF EXISTS `ai_sistemi`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `ai_sistemi` (
  `id_sistema` int(11) NOT NULL AUTO_INCREMENT,
  `descrizione` varchar(30) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id_sistema`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `ai_sottosistemi`
--

DROP TABLE IF EXISTS `ai_sottosistemi`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `ai_sottosistemi` (
  `id_ai_sottosistema` int(11) NOT NULL AUTO_INCREMENT,
  `descrizione` varchar(30) COLLATE utf8_unicode_ci NOT NULL DEFAULT '0',
  `id_ai_sistema` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id_ai_sottosistema`),
  KEY `ai_sottosistemi_ibfk_1` (`id_ai_sistema`),
  CONSTRAINT `ai_sottosistemi_ibfk_1` FOREIGN KEY (`id_ai_sistema`) REFERENCES `ai_sistemi` (`id_sistema`) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `ai_sottosistemi_input_output`
--

DROP TABLE IF EXISTS `ai_sottosistemi_input_output`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `ai_sottosistemi_input_output` (
  `id_ai_sottosistema` int(11) NOT NULL DEFAULT '0',
  `id_ai_input_output` int(11) NOT NULL DEFAULT '0',
  UNIQUE KEY `id_ai_sottosistemi` (`id_ai_sottosistema`,`id_ai_input_output`),
  KEY `id_ai_input_output` (`id_ai_input_output`),
  CONSTRAINT `ai_sottosistemi_input_output_ibfk_1` FOREIGN KEY (`id_ai_sottosistema`) REFERENCES `ai_sottosistemi` (`id_ai_sottosistema`) ON DELETE CASCADE,
  CONSTRAINT `ai_sottosistemi_input_output_ibfk_2` FOREIGN KEY (`id_ai_input_output`) REFERENCES `ai_input_output` (`id_ai_input_output`) ON DELETE CASCADE ON UPDATE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `alarms`
--

DROP TABLE IF EXISTS `alarms`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `alarms` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `data` datetime NOT NULL,
  `board_type` tinyint(4) NOT NULL,
  `device_id` int(11) NOT NULL,
  `ch_id` int(11) NOT NULL,
  `msg` varchar(50) NOT NULL,
  `data_ack` datetime NOT NULL DEFAULT '0000-00-00 00:00:00',
  `inattivo` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=InnoDB AUTO_INCREMENT=1127 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `analog`
--

DROP TABLE IF EXISTS `analog`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `analog` (
  `id_analog` int(11) NOT NULL AUTO_INCREMENT,
  `form_label` varchar(20) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `description` varchar(50) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `label` varchar(20) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `sinottico` varchar(50) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `device_num` int(11) NOT NULL DEFAULT '0',
  `ch_num` int(11) NOT NULL DEFAULT '0',
  `scale_zero` int(11) NOT NULL DEFAULT '0',
  `scale_full` int(11) NOT NULL DEFAULT '0',
  `range_zero` float NOT NULL DEFAULT '0',
  `range_full` float NOT NULL DEFAULT '0',
  `bipolar` tinyint(1) NOT NULL DEFAULT '0',
  `al_high_active` tinyint(1) NOT NULL DEFAULT '0',
  `al_high` float NOT NULL DEFAULT '0',
  `al_low_active` tinyint(1) NOT NULL DEFAULT '0',
  `al_low` float NOT NULL DEFAULT '0',
  `offset` float NOT NULL DEFAULT '0',
  `unit` varchar(10) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `time_delay_high` int(11) NOT NULL DEFAULT '0',
  `time_delay_low` int(11) NOT NULL DEFAULT '0',
  `time_delay_high_off` int(11) NOT NULL DEFAULT '0',
  `time_delay_low_off` int(11) NOT NULL DEFAULT '0',
  `curve` text CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `no_linear` tinyint(1) NOT NULL DEFAULT '0',
  `printer` tinyint(1) NOT NULL DEFAULT '0',
  `sms` tinyint(1) NOT NULL DEFAULT '0',
  `msg_l` varchar(80) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `msg_h` varchar(80) CHARACTER SET utf8 COLLATE utf8_unicode_ci NOT NULL,
  `msg_is_event` tinyint(1) NOT NULL DEFAULT '0',
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `g_min` float NOT NULL DEFAULT '-1',
  `g_max` float NOT NULL DEFAULT '-1',
  `g_min_ok` float NOT NULL DEFAULT '-1',
  `g_max_ok` float NOT NULL DEFAULT '-1',
  `g_min_alarm` float NOT NULL DEFAULT '-1',
  `g_max_alarm` float NOT NULL DEFAULT '-1',
  `pmin` float DEFAULT NULL COMMENT 'valore minimo plot',
  `pmax` float DEFAULT NULL COMMENT 'valore massimo plot',
  `record_data_time` int(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id_analog`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `bacnet_devices`
--

DROP TABLE IF EXISTS `bacnet_devices`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bacnet_devices` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `address` varchar(16) COLLATE utf8_unicode_ci NOT NULL,
  `port` int(11) NOT NULL,
  `description` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `bacnet_devices_acli`
--

DROP TABLE IF EXISTS `bacnet_devices_acli`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bacnet_devices_acli` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `address` varchar(16) COLLATE utf8_unicode_ci NOT NULL,
  `port` int(11) NOT NULL,
  `description` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `bacnet_inputs`
--

DROP TABLE IF EXISTS `bacnet_inputs`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bacnet_inputs` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `id_bacnet_device` int(1) DEFAULT NULL,
  `object_type` int(1) DEFAULT NULL,
  `object_instance` int(7) DEFAULT NULL,
  `description` varchar(74) DEFAULT NULL,
  `hi_low_msg` varchar(70) DEFAULT NULL,
  `low_hi_msg` varchar(70) DEFAULT NULL,
  `is_alarm` int(1) DEFAULT NULL,
  `alarm_msg` varchar(46) DEFAULT NULL,
  `pmin` float DEFAULT NULL COMMENT 'valore minimo plot',
  `pmax` float DEFAULT NULL COMMENT 'valore massimo plot',
  `record_data_time` int(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=92 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `bacnet_inputs_acli`
--

DROP TABLE IF EXISTS `bacnet_inputs_acli`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bacnet_inputs_acli` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `id_bacnet_device` int(1) DEFAULT NULL,
  `object_type` int(1) DEFAULT NULL,
  `object_instance` int(7) DEFAULT NULL,
  `description` varchar(74) DEFAULT NULL,
  `hi_low_msg` varchar(57) DEFAULT NULL,
  `low_hi_msg` varchar(57) DEFAULT NULL,
  `is_alarm` int(1) DEFAULT NULL,
  `alarm_msg` varchar(46) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=92 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `bacnet_inputs_backup`
--

DROP TABLE IF EXISTS `bacnet_inputs_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `bacnet_inputs_backup` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `id_bacnet_device` int(1) DEFAULT NULL,
  `object_type` int(1) DEFAULT NULL,
  `object_instance` int(7) DEFAULT NULL,
  `description` varchar(74) DEFAULT NULL,
  `hi_low_msg` varchar(57) DEFAULT NULL,
  `low_hi_msg` varchar(57) DEFAULT NULL,
  `is_alarm` int(1) DEFAULT NULL,
  `alarm_msg` varchar(46) DEFAULT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=92 DEFAULT CHARSET=utf8;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `cameras`
--

DROP TABLE IF EXISTS `cameras`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `cameras` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `descrizione` varchar(50) NOT NULL,
  `address` varchar(16) NOT NULL,
  `user` varchar(20) NOT NULL,
  `pass` varchar(20) NOT NULL,
  `disabled` tinyint(4) NOT NULL DEFAULT '0',
  `rotate` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=14 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `ceiabi_gateway`
--

DROP TABLE IF EXISTS `ceiabi_gateway`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `ceiabi_gateway` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `address` varchar(16) COLLATE utf8_unicode_ci NOT NULL,
  `port` int(11) NOT NULL,
  `description` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `device_type`
--

DROP TABLE IF EXISTS `device_type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `device_type` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `type` tinyint(3) unsigned NOT NULL,
  `in_num_bytes` tinyint(3) unsigned NOT NULL,
  `in_ch_an` tinyint(3) unsigned NOT NULL,
  `in_ch_d` tinyint(3) unsigned NOT NULL,
  `in_eos` tinyint(3) unsigned NOT NULL DEFAULT '85',
  `out_num_bytes` tinyint(3) unsigned NOT NULL,
  `out_ch_d` tinyint(3) unsigned NOT NULL,
  `out_ch_per_byte` tinyint(3) unsigned NOT NULL DEFAULT '4',
  `out_sos` tinyint(3) unsigned NOT NULL,
  `out_eos` tinyint(3) unsigned NOT NULL DEFAULT '85',
  `descrizione` varchar(50) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=5 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `digital`
--

DROP TABLE IF EXISTS `digital`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `digital` (
  `id_digital` int(11) NOT NULL AUTO_INCREMENT,
  `form_label` varchar(20) NOT NULL,
  `description` varchar(50) NOT NULL,
  `label` varchar(20) NOT NULL,
  `sinottico` varchar(50) NOT NULL,
  `device_num` int(11) NOT NULL DEFAULT '0',
  `ch_num` int(11) NOT NULL DEFAULT '0',
  `on_value` tinyint(4) NOT NULL DEFAULT '1',
  `alarm_value` tinyint(4) NOT NULL DEFAULT '1',
  `printer` tinyint(1) NOT NULL DEFAULT '0',
  `sms` tinyint(1) NOT NULL DEFAULT '0',
  `time_delay_on` int(20) NOT NULL DEFAULT '0',
  `time_delay_off` int(10) unsigned NOT NULL DEFAULT '0',
  `msg_on` varchar(255) NOT NULL,
  `msg_off` varchar(255) NOT NULL,
  `msg_is_event` tinyint(1) NOT NULL DEFAULT '0',
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `record_data_time` int(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id_digital`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `digital_out`
--

DROP TABLE IF EXISTS `digital_out`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `digital_out` (
  `id_digital_out` int(10) NOT NULL AUTO_INCREMENT,
  `device_num` int(11) NOT NULL,
  `ch_num` int(11) NOT NULL,
  `description` varchar(50) NOT NULL,
  `default` tinyint(4) NOT NULL DEFAULT '0',
  `on_value` tinyint(4) NOT NULL DEFAULT '1',
  `po_delay` smallint(6) NOT NULL DEFAULT '0',
  `on_time` smallint(6) NOT NULL DEFAULT '0',
  `pon_time` smallint(6) NOT NULL DEFAULT '0',
  `poff_time` smallint(6) NOT NULL DEFAULT '0',
  `printer` tinyint(1) NOT NULL DEFAULT '0',
  `sms` tinyint(1) NOT NULL DEFAULT '0',
  `msg_l` varchar(80) NOT NULL DEFAULT '',
  `msg_h` varchar(80) NOT NULL DEFAULT '',
  `msg_is_event` tinyint(1) NOT NULL DEFAULT '0',
  `record_data_time` int(11) NOT NULL DEFAULT '-1',
  UNIQUE KEY `id` (`id_digital_out`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `effemeridi`
--

DROP TABLE IF EXISTS `effemeridi`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `effemeridi` (
  `day` tinyint(3) unsigned NOT NULL,
  `month` tinyint(3) unsigned NOT NULL,
  `dawn` time NOT NULL,
  `sunset` time NOT NULL,
  UNIQUE KEY `month` (`day`,`month`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `events`
--

DROP TABLE IF EXISTS `events`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `events` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `data` datetime NOT NULL,
  `event_type` int(11) NOT NULL DEFAULT '0',
  `device_num` int(11) NOT NULL,
  `ch_num` int(11) NOT NULL,
  `ch_id` int(11) NOT NULL DEFAULT '0',
  `msg` varchar(50) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `history`
--

DROP TABLE IF EXISTS `history`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `history` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `datetime` datetime NOT NULL,
  `datetime_end` datetime NOT NULL,
  `board_type` tinyint(4) NOT NULL,
  `ch_id` tinyint(3) unsigned NOT NULL,
  `device_id` tinyint(3) unsigned NOT NULL,
  `value` double NOT NULL,
  `raw` smallint(6) NOT NULL,
  `um` varchar(8) NOT NULL,
  PRIMARY KEY (`id`),
  KEY `board_type` (`board_type`),
  KEY `board_type_2` (`board_type`,`ch_id`),
  KEY `datetime` (`datetime`),
  KEY `id` (`id`,`datetime`,`datetime_end`,`board_type`,`ch_id`,`device_id`,`value`,`raw`,`um`)
) ENGINE=MyISAM AUTO_INCREMENT=51065 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `irrigazione`
--

DROP TABLE IF EXISTS `irrigazione`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `irrigazione` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `ora_start` int(11) NOT NULL DEFAULT '0' COMMENT 'minuti',
  `ripetitivita` int(11) NOT NULL DEFAULT '0',
  `tempo_off` int(11) NOT NULL DEFAULT '0' COMMENT 'minuti',
  `id_digital_out` int(11) NOT NULL DEFAULT '0' COMMENT 'uscita corrispondente alla pompa',
  `msg_avvio_automativo` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'auto start',
  `msg_arresto_automatico` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'auto stop',
  `msg_avvio_manuale` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'manual start',
  `msg_arresto_manuale` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'manual stop',
  `msg_arresto_pioggia` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'rain stop',
  `msg_pioggia_noinizio` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'rain abort',
  `msg_avvio_pompa` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'pump start',
  `msg_arresto_pompa` varchar(80) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'pump stop',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `irrigazione_circuiti`
--

DROP TABLE IF EXISTS `irrigazione_circuiti`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `irrigazione_circuiti` (
  `id_irrigazione` int(11) NOT NULL,
  `circuito` tinyint(4) NOT NULL DEFAULT '0' COMMENT 'numero',
  `id_digital_out` int(11) NOT NULL DEFAULT '0',
  `durata` int(11) NOT NULL DEFAULT '0' COMMENT 'minuti',
  `validita` int(11) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id_irrigazione`,`circuito`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `knx_gateway`
--

DROP TABLE IF EXISTS `knx_gateway`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `knx_gateway` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `address` varchar(16) COLLATE utf8_unicode_ci NOT NULL,
  `port` int(11) NOT NULL,
  `physical_address` varchar(15) COLLATE utf8_unicode_ci NOT NULL,
  `type` tinyint(4) NOT NULL DEFAULT '1',
  `description` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `knx_in_out`
--

DROP TABLE IF EXISTS `knx_in_out`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `knx_in_out` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `id_knx_gateway` int(11) NOT NULL DEFAULT '0',
  `input_output` tinyint(1) NOT NULL DEFAULT '0' COMMENT '1=input',
  `group_address` varchar(10) COLLATE utf8_unicode_ci NOT NULL,
  `data_type` char(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'B',
  `description` varchar(128) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `hi_low_msg` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `low_hi_msg` varchar(128) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `is_alarm` tinyint(1) NOT NULL DEFAULT '0',
  `alarm_transition` tinyint(1) NOT NULL DEFAULT '1' COMMENT '(se 1 L -> H, se 0 H -> L)',
  `sms` tinyint(1) NOT NULL DEFAULT '0',
  `pmin` float DEFAULT NULL COMMENT 'valore minimo plot',
  `pmax` float DEFAULT NULL COMMENT 'valore massimo plot',
  `record_data_time` int(11) NOT NULL DEFAULT '-1',
  PRIMARY KEY (`id`),
  UNIQUE KEY `input_output` (`input_output`,`group_address`)
) ENGINE=MyISAM AUTO_INCREMENT=215 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `knx_in_out_backup`
--

DROP TABLE IF EXISTS `knx_in_out_backup`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `knx_in_out_backup` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `id_knx_gateway` int(11) NOT NULL DEFAULT '0',
  `input_output` tinyint(1) NOT NULL DEFAULT '0' COMMENT '1=input',
  `group_address` varchar(10) COLLATE utf8_unicode_ci NOT NULL,
  `data_type` char(1) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'B',
  `description` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `hi_low_msg` varchar(50) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `low_hi_msg` varchar(50) COLLATE utf8_unicode_ci NOT NULL DEFAULT '',
  `is_alarm` tinyint(1) NOT NULL DEFAULT '0',
  `alarm_transition` tinyint(1) NOT NULL DEFAULT '1' COMMENT '(se 1 L -> H, se 0 H -> L)',
  `sms` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`),
  UNIQUE KEY `input_output` (`input_output`,`group_address`)
) ENGINE=MyISAM AUTO_INCREMENT=41 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `login`
--

DROP TABLE IF EXISTS `login`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `login` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `nome` varchar(20) COLLATE utf8_unicode_ci NOT NULL,
  `cognome` varchar(20) COLLATE utf8_unicode_ci NOT NULL,
  `pw` varchar(40) COLLATE utf8_unicode_ci NOT NULL,
  `accesso_manutenzione` tinyint(1) NOT NULL DEFAULT '0',
  `accesso_cancello` tinyint(1) NOT NULL DEFAULT '0',
  `accesso_altro` tinyint(1) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mm`
--

DROP TABLE IF EXISTS `mm`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mm` (
  `multimeter_num` int(11) NOT NULL AUTO_INCREMENT,
  `descrizione` varchar(50) NOT NULL,
  `mm_type_id` int(4) DEFAULT NULL,
  `tcp_ip` varchar(16) DEFAULT NULL,
  `port` smallint(6) NOT NULL DEFAULT '3490',
  `enabled` tinyint(1) DEFAULT NULL,
  `removed` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`multimeter_num`)
) ENGINE=MyISAM AUTO_INCREMENT=4 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mm_reading`
--

DROP TABLE IF EXISTS `mm_reading`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mm_reading` (
  `id_reading` int(20) NOT NULL AUTO_INCREMENT,
  `form_label` longtext,
  `description` longtext,
  `label` longtext,
  `sinottico` longtext,
  `multimeter_num` int(11) DEFAULT '1',
  `ch_num` int(11) DEFAULT NULL,
  `printer` tinyint(1) DEFAULT '0',
  `msg_is_event` tinyint(1) NOT NULL DEFAULT '0',
  `enabled` tinyint(1) DEFAULT '1',
  `al_high_value` int(11) NOT NULL DEFAULT '0',
  `al_high_active` tinyint(1) NOT NULL DEFAULT '0',
  `al_high_msg` varchar(255) NOT NULL DEFAULT '',
  `time_delay_high_on` int(11) NOT NULL DEFAULT '0',
  `time_delay_high_off` int(11) NOT NULL DEFAULT '0',
  `sms_high_active` tinyint(1) NOT NULL DEFAULT '0',
  `sms_high_msg` varchar(255) NOT NULL DEFAULT '',
  `al_low_value` int(11) NOT NULL DEFAULT '0',
  `al_low_active` tinyint(1) NOT NULL DEFAULT '0',
  `al_low_msg` varchar(255) NOT NULL DEFAULT '',
  `time_delay_low_on` int(11) NOT NULL DEFAULT '0',
  `time_delay_low_off` int(11) NOT NULL DEFAULT '0',
  `sms_low_active` tinyint(1) NOT NULL DEFAULT '0',
  `sms_low_msg` varchar(255) NOT NULL DEFAULT '',
  `pmin` int(11) DEFAULT NULL,
  `pmax` int(11) DEFAULT NULL,
  `unit` varchar(255) NOT NULL DEFAULT '',
  `record_data_time` int(11) NOT NULL DEFAULT '-1',
  `size` tinyint(1) unsigned DEFAULT NULL,
  `res` tinyint(1) DEFAULT NULL,
  `step_after` int(11) NOT NULL DEFAULT '0',
  `unsign` tinyint(1) DEFAULT NULL,
  PRIMARY KEY (`id_reading`)
) ENGINE=MyISAM AUTO_INCREMENT=101 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `mm_type`
--

DROP TABLE IF EXISTS `mm_type`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `mm_type` (
  `id` int(10) unsigned NOT NULL AUTO_INCREMENT,
  `description` varchar(50) NOT NULL,
  `in_bytes_1` varchar(20) NOT NULL,
  `in_bytes_2` varchar(20) NOT NULL,
  `out_ch_1` tinyint(4) NOT NULL DEFAULT '0',
  `out_ch_2` tinyint(4) NOT NULL DEFAULT '0',
  `header_in` varchar(10) NOT NULL,
  `header_out` varchar(10) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=4 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `panel`
--

DROP TABLE IF EXISTS `panel`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `panel` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `panel_num` int(11) NOT NULL DEFAULT '1',
  `id_analog` int(11) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=41 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `panel_header`
--

DROP TABLE IF EXISTS `panel_header`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `panel_header` (
  `panel_num` int(11) NOT NULL,
  `description` varchar(50) NOT NULL,
  PRIMARY KEY (`panel_num`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `pm`
--

DROP TABLE IF EXISTS `pm`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `pm` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `description` varchar(50) NOT NULL,
  `inputs` smallint(11) NOT NULL DEFAULT '42',
  `address` varchar(16) DEFAULT NULL,
  `port` smallint(6) NOT NULL DEFAULT '3490',
  `enabled` tinyint(1) DEFAULT NULL,
  `removed` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=4 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `pm_reading`
--

DROP TABLE IF EXISTS `pm_reading`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `pm_reading` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `form_label` longtext,
  `description` longtext,
  `label` longtext,
  `sinottico` longtext,
  `pm_id` int(11) DEFAULT '1',
  `ch_num` int(11) DEFAULT NULL,
  `printer` tinyint(1) DEFAULT '0',
  `msg_is_event` tinyint(1) NOT NULL DEFAULT '0',
  `al_high_value` int(11) NOT NULL DEFAULT '0',
  `al_high_active` tinyint(1) NOT NULL DEFAULT '0',
  `al_high_msg` varchar(255) NOT NULL DEFAULT '',
  `time_delay_high_on` int(11) NOT NULL DEFAULT '0',
  `time_delay_high_off` int(11) NOT NULL DEFAULT '0',
  `sms_high_active` tinyint(1) NOT NULL DEFAULT '0',
  `sms_high_msg` varchar(255) NOT NULL DEFAULT '',
  `al_low_value` int(11) NOT NULL DEFAULT '0',
  `al_low_active` tinyint(1) NOT NULL DEFAULT '0',
  `al_low_msg` varchar(255) NOT NULL DEFAULT '',
  `time_delay_low_on` int(11) NOT NULL DEFAULT '0',
  `time_delay_low_off` int(11) NOT NULL DEFAULT '0',
  `sms_low_active` tinyint(1) NOT NULL DEFAULT '0',
  `sms_low_msg` varchar(255) NOT NULL DEFAULT '',
  `pmin` int(11) DEFAULT NULL,
  `pmax` int(11) DEFAULT NULL,
  `unit` varchar(255) NOT NULL DEFAULT '',
  `record_data_time` int(11) NOT NULL DEFAULT '-1',
  `enabled` tinyint(1) DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=67 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `scenari_giorno_notte`
--

DROP TABLE IF EXISTS `scenari_giorno_notte`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `scenari_giorno_notte` (
  `id` int(11) NOT NULL,
  `id_digital_out` int(11) NOT NULL DEFAULT '0',
  `attivo` tinyint(4) NOT NULL DEFAULT '0',
  `ritardo` int(11) NOT NULL DEFAULT '0',
  `giorno` tinyint(4) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`,`giorno`),
  KEY `id_digital_out` (`id_digital_out`,`giorno`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `scenari_presenze`
--

DROP TABLE IF EXISTS `scenari_presenze`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `scenari_presenze` (
  `id` int(11) NOT NULL,
  `id_digital_out` int(11) NOT NULL,
  `attivo` tinyint(4) NOT NULL DEFAULT '0',
  `ciclico` tinyint(4) NOT NULL DEFAULT '0',
  `tempo_on` smallint(6) NOT NULL DEFAULT '0',
  `tempo_off` smallint(6) NOT NULL DEFAULT '0',
  `ora_ini` time NOT NULL DEFAULT '00:00:00',
  `ora_fine` time NOT NULL DEFAULT '00:00:00',
  PRIMARY KEY (`id`),
  UNIQUE KEY `id_digital_out` (`id_digital_out`)
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `sms_device`
--

DROP TABLE IF EXISTS `sms_device`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `sms_device` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `address` varchar(16) COLLATE utf8_unicode_ci NOT NULL DEFAULT '192.168.1.107',
  `port` int(11) NOT NULL DEFAULT '4001',
  `description` varchar(50) COLLATE utf8_unicode_ci NOT NULL DEFAULT 'SMS board',
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `sms_numbers`
--

DROP TABLE IF EXISTS `sms_numbers`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `sms_numbers` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
  `number` varchar(20) COLLATE utf8_unicode_ci NOT NULL,
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=3 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `solarlog`
--

DROP TABLE IF EXISTS `solarlog`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `solarlog` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `name` varchar(60) COLLATE utf8_unicode_ci NOT NULL,
  `address` varchar(15) COLLATE utf8_unicode_ci NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci COMMENT='solarlog';
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `system`
--

DROP TABLE IF EXISTS `system`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `system` (
  `device_num` int(11) NOT NULL AUTO_INCREMENT,
  `device_type` int(11) NOT NULL,
  `tcp_ip` varchar(16) NOT NULL,
  `port` smallint(6) NOT NULL DEFAULT '3490',
  `enabled` tinyint(1) NOT NULL DEFAULT '1',
  `device_type_str` varchar(50) NOT NULL,
  `removed` tinyint(4) NOT NULL DEFAULT '0',
  PRIMARY KEY (`device_num`)
) ENGINE=MyISAM AUTO_INCREMENT=2 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `system_ini`
--

DROP TABLE IF EXISTS `system_ini`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `system_ini` (
  `record_data_time` tinyint(3) unsigned NOT NULL DEFAULT '5',
  `localita` varchar(30) NOT NULL,
  `nome` varchar(30) NOT NULL,
  `reboot_time` tinyint(3) unsigned NOT NULL DEFAULT '10',
  `offset_effe` int(11) NOT NULL DEFAULT '0'
) ENGINE=MyISAM DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;

--
-- Table structure for table `users`
--

DROP TABLE IF EXISTS `users`;
/*!40101 SET @saved_cs_client     = @@character_set_client */;
/*!40101 SET character_set_client = utf8 */;
CREATE TABLE `users` (
  `id` int(11) NOT NULL AUTO_INCREMENT,
  `login` varchar(15) NOT NULL,
  `pw` varchar(50) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM AUTO_INCREMENT=4 DEFAULT CHARSET=latin1;
/*!40101 SET character_set_client = @saved_cs_client */;
/*!40103 SET TIME_ZONE=@OLD_TIME_ZONE */;

/*!40101 SET SQL_MODE=@OLD_SQL_MODE */;
/*!40014 SET FOREIGN_KEY_CHECKS=@OLD_FOREIGN_KEY_CHECKS */;
/*!40014 SET UNIQUE_CHECKS=@OLD_UNIQUE_CHECKS */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40101 SET CHARACTER_SET_RESULTS=@OLD_CHARACTER_SET_RESULTS */;
/*!40101 SET COLLATION_CONNECTION=@OLD_COLLATION_CONNECTION */;
/*!40111 SET SQL_NOTES=@OLD_SQL_NOTES */;

-- Dump completed on 2015-05-26 14:52:19
