#!/bin/bash
set -x
./partytime -s scripts/week1_columns.script > week1b.rslt; diff week1b.rslt ./unittest/week1b.rslt
./partytime -s scripts/week2_columns.script > week2b.rslt; diff week2b.rslt ./unittest/week2b.rslt
./partytime -s scripts/week3_columns.script > week3b.rslt; diff week3b.rslt ./unittest/week3b.rslt
./partytime -s scripts/week4_columns.script > week4b.rslt; diff week4b.rslt ./unittest/week4b.rslt
set +x
