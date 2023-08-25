#Makefile at top of application tree
TOP = .
include $(TOP)/configure/CONFIG
DIRS := $(DIRS) configure
DIRS := $(DIRS) GenICamSupport
DIRS := $(DIRS) GenICamApp
include $(TOP)/configure/RULES_TOP
