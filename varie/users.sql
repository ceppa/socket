-- phpMyAdmin SQL Dump
-- version 2.10.3deb1ubuntu0.2
-- http://www.phpmyadmin.net
-- 
-- Host: localhost
-- Generato il: 18 Apr, 2008 at 03:40 PM
-- Versione MySQL: 5.0.45
-- Versione PHP: 5.2.3-1ubuntu6.3

SET SQL_MODE="NO_AUTO_VALUE_ON_ZERO";

-- 
-- Database: `socket`
-- 

-- --------------------------------------------------------

-- 
-- Struttura della tabella `users`
-- 

CREATE TABLE IF NOT EXISTS `users` (
  `id` int(11) NOT NULL auto_increment,
  `login` varchar(15) NOT NULL,
  `pw` varchar(50) NOT NULL,
  PRIMARY KEY  (`id`)
) ENGINE=MyISAM  DEFAULT CHARSET=latin1 AUTO_INCREMENT=3 ;

-- 
-- Dump dei dati per la tabella `users`
-- 

INSERT INTO `users` (`id`, `login`, `pw`) VALUES 
(1, 'admin', '21232f297a57a5a743894a0e4a801fc3'),
(2, 'gino', '0eb074dcc1bfd7761e1de6772d2baf14');
