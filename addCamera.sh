#!/bin/bash
python scripts/makeDb.py xml/$1.xml GenICamApp/Db/$1.template
python scripts/makeAdl.py xml/$1.xml GenICamApp/op/adl/$1
