#!/bin/bash

# Use this line for int64out and int64in records.  Requires EPICS base 3.16.1 or later, or EPICS 7.
#python scripts/makeDb.py xml/$1.xml --devInt64 GenICamApp/Db/$1.template

# Use this line for ao and ai records on older versions of EPICS base. 
# Limits GenICam Integer features to 52 bits because ao and ai records with 64-bit floats are used.
python3 scripts/makeDb.py xml/$1.xml GenICamApp/Db/$1.template

python3 scripts/makeAdl.py xml/$1.xml GenICamApp/op/adl/$1
