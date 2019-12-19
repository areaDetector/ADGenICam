#!/bin/bash

# This script calls addCamera for each of the .xml files in the xml/ directory
for filename in ./xml/*.xml; do
  file=${filename/.xml/}
  file=${file/.\/xml/}
  echo "Calling addCamera for " $file
  ./addCamera.sh $file
done
