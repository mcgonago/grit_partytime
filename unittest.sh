#!/bin/bash
set -x
./partytime -s scripts/week1.script > week1.rslt; diff week1.rslt ./unittest/week1.rslt
./partytime -s scripts/week2.script > week2.rslt; diff week2.rslt ./unittest/week2.rslt
./partytime -s scripts/week3.script > week3.rslt; diff week3.rslt ./unittest/week3.rslt
./partytime -s scripts/week4.script > week4.rslt; diff week4.rslt ./unittest/week4.rslt
set +x
