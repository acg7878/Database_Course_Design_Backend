/*
 Navicat Premium Dump SQL

 Source Server         : localhost_3306
 Source Server Type    : MySQL
 Source Server Version : 90200 (9.2.0)
 Source Host           : localhost:3306
 Source Schema         : club_management_system

 Target Server Type    : MySQL
 Target Server Version : 90200 (9.2.0)
 File Encoding         : 65001

 Date: 16/04/2025 20:40:11
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for activity_checkin
-- ----------------------------
DROP TABLE IF EXISTS `activity_checkin`;
CREATE TABLE `activity_checkin` (
  `checkin_id` int NOT NULL AUTO_INCREMENT,
  `user_id` int NOT NULL,
  `activity_id` int NOT NULL,
  `checkin_time` datetime DEFAULT NULL,
  PRIMARY KEY (`checkin_id`),
  KEY `user_id` (`user_id`),
  KEY `activity_id` (`activity_id`),
  CONSTRAINT `activity_checkin_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`),
  CONSTRAINT `activity_checkin_ibfk_2` FOREIGN KEY (`activity_id`) REFERENCES `club_activity` (`activity_id`)
) ENGINE=InnoDB AUTO_INCREMENT=7 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- ----------------------------
-- Table structure for activity_registration
-- ----------------------------
DROP TABLE IF EXISTS `activity_registration`;
CREATE TABLE `activity_registration` (
  `registration_id` int NOT NULL AUTO_INCREMENT,
  `user_id` int NOT NULL,
  `activity_id` int NOT NULL,
  `registration_date` datetime DEFAULT NULL,
  `payment_status` enum('未缴费','已缴费') NOT NULL DEFAULT '未缴费',
  `registration_status` enum('pending','accepted','rejected','cancel') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL DEFAULT 'pending',
  PRIMARY KEY (`registration_id`),
  KEY `user_id` (`user_id`),
  KEY `activity_id` (`activity_id`),
  CONSTRAINT `activity_registration_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`),
  CONSTRAINT `activity_registration_ibfk_2` FOREIGN KEY (`activity_id`) REFERENCES `club_activity` (`activity_id`)
) ENGINE=InnoDB AUTO_INCREMENT=27 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- ----------------------------
-- Table structure for club
-- ----------------------------
DROP TABLE IF EXISTS `club`;
CREATE TABLE `club` (
  `club_id` int NOT NULL AUTO_INCREMENT,
  `club_name` varchar(100) NOT NULL,
  `club_introduction` text,
  `contact_info` varchar(100) DEFAULT NULL,
  `activity_venue` varchar(100) DEFAULT NULL,
  `founder_id` int DEFAULT NULL,
  PRIMARY KEY (`club_id`),
  KEY `founder_id` (`founder_id`),
  CONSTRAINT `club_ibfk_1` FOREIGN KEY (`founder_id`) REFERENCES `user` (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=38 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- ----------------------------
-- Table structure for club_activity
-- ----------------------------
DROP TABLE IF EXISTS `club_activity`;
CREATE TABLE `club_activity` (
  `activity_id` int NOT NULL AUTO_INCREMENT,
  `club_id` int NOT NULL,
  `activity_title` varchar(100) NOT NULL,
  `activity_time` datetime DEFAULT NULL,
  `activity_location` varchar(100) DEFAULT NULL,
  `registration_method` text,
  `activity_description` text,
  `publish_time` datetime DEFAULT NULL,
  `registration_status` enum('pending''accepted''rejected') DEFAULT NULL,
  PRIMARY KEY (`activity_id`),
  KEY `club_id` (`club_id`),
  CONSTRAINT `club_activity_ibfk_1` FOREIGN KEY (`club_id`) REFERENCES `club` (`club_id`)
) ENGINE=InnoDB AUTO_INCREMENT=12 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- ----------------------------
-- Table structure for club_approval
-- ----------------------------
DROP TABLE IF EXISTS `club_approval`;
CREATE TABLE `club_approval` (
  `approval_id` int NOT NULL AUTO_INCREMENT,
  `club_name` varchar(100) NOT NULL,
  `applicant_id` int DEFAULT NULL,
  `approval_status` enum('待审核','通过','不通过') NOT NULL DEFAULT '待审核',
  `approval_time` datetime DEFAULT NULL,
  `approval_opinion` text,
  `club_introduction` text,
  `contact_info` varchar(100) DEFAULT NULL,
  `activity_venue` varchar(100) DEFAULT NULL,
  PRIMARY KEY (`approval_id`),
  KEY `club_approval_ibfk_2` (`applicant_id`),
  CONSTRAINT `club_approval_ibfk_2` FOREIGN KEY (`applicant_id`) REFERENCES `user` (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=24 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- ----------------------------
-- Table structure for club_member
-- ----------------------------
DROP TABLE IF EXISTS `club_member`;
CREATE TABLE `club_member` (
  `member_id` int NOT NULL AUTO_INCREMENT,
  `user_id` int NOT NULL,
  `club_id` int NOT NULL,
  `join_date` datetime NOT NULL,
  `member_role` enum('社长','社员') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci DEFAULT '社员',
  PRIMARY KEY (`member_id`),
  KEY `user_id` (`user_id`),
  KEY `club_id` (`club_id`),
  CONSTRAINT `club_member_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`),
  CONSTRAINT `club_member_ibfk_2` FOREIGN KEY (`club_id`) REFERENCES `club` (`club_id`)
) ENGINE=InnoDB AUTO_INCREMENT=28 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- ----------------------------
-- Table structure for club_member_apply
-- ----------------------------
DROP TABLE IF EXISTS `club_member_apply`;
CREATE TABLE `club_member_apply` (
  `apply_id` int NOT NULL AUTO_INCREMENT,
  `user_id` int NOT NULL,
  `club_id` int NOT NULL,
  `apply_date` datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
  `status` enum('pending','approved','rejected') NOT NULL DEFAULT 'pending',
  PRIMARY KEY (`apply_id`),
  KEY `user_id` (`user_id`),
  KEY `club_id` (`club_id`),
  CONSTRAINT `club_member_apply_ibfk_1` FOREIGN KEY (`user_id`) REFERENCES `user` (`user_id`),
  CONSTRAINT `club_member_apply_ibfk_2` FOREIGN KEY (`club_id`) REFERENCES `club` (`club_id`)
) ENGINE=InnoDB AUTO_INCREMENT=8 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user` (
  `user_id` int NOT NULL AUTO_INCREMENT,
  `username` varchar(50) NOT NULL,
  `password` varchar(255) NOT NULL,
  `user_type` enum('社员','社长','管理员') NOT NULL,
  `email` varchar(100) DEFAULT NULL,
  `phone` varchar(20) DEFAULT NULL,
  PRIMARY KEY (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=12 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_0900_ai_ci;

SET FOREIGN_KEY_CHECKS = 1;
