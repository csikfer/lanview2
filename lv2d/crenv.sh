#!/bin/bash 

LV2HOME="/var/local/lanview2"

adduser --system --home $LV2HOME lanview2

mkdir $LV2HOME
for a in log mibs rrd
do
   mkdir $LV2HOME/$a
done

chown -R lanview2 $LV2HOME

