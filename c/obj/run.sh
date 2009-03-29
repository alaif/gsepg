#!/bin/bash
#make && ./gsepg-filter | ./gsepg-extract -v ../../epg_20081022 ; echo "=== Return value: $?"
#make &&  ./gsepg-extract -v ../../epg_20081022 ; echo "=== Return value: $?"
make && ./gsepg-filter -u localhost -p 2000 | ./gsepg-extract - -v ; echo "=== Return value: $?"
