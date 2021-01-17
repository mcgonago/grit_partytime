#!/bin/bash
set -x
./column_all.sh > column.rslt; diff column.rslt ./unittest/column.rslt
set +x
