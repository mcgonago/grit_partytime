#!/bin/bash
set -x
./partytime -s scripts/week1_columns.script > week1b.rslt; diff week1b.rslt ./unittest/week1b.rslt
./partytime -s scripts/week2_columns.script > week2b.rslt; diff week2b.rslt ./unittest/week2b.rslt
./partytime -s scripts/week3_columns.script > week3b.rslt; diff week3b.rslt ./unittest/week3b.rslt
./partytime -s scripts/week4_columns.script > week4b.rslt; diff week4b.rslt ./unittest/week4b.rslt
./partytime -s scripts/week5_columns.script > week5b.rslt; diff week5b.rslt ./unittest/week5b.rslt
./partytime -s scripts/week6_columns.script > week6b.rslt; diff week6b.rslt ./unittest/week6b.rslt
./partytime -s scripts/week7_columns.script > week7b.rslt; diff week7b.rslt ./unittest/week7b.rslt
./partytime -s scripts/week8_columns.script > week8b.rslt; diff week8b.rslt ./unittest/week8b.rslt
./partytime -s scripts/week9_columns.script > week9b.rslt; diff week9b.rslt ./unittest/week9b.rslt
set +x
