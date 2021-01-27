#!/bin/bash
set -x
./partytime -s scripts/week1.script > week1.rslt; diff week1.rslt ./unittest/week1.rslt
./partytime -s scripts/week2.script > week2.rslt; diff week2.rslt ./unittest/week2.rslt
./partytime -s scripts/week3.script > week3.rslt; diff week3.rslt ./unittest/week3.rslt
./partytime -s scripts/week4.script > week4.rslt; diff week4.rslt ./unittest/week4.rslt
./partytime -s scripts/week5.script > week5.rslt; diff week5.rslt ./unittest/week5.rslt
./partytime -s scripts/week6.script > week6.rslt; diff week6.rslt ./unittest/week6.rslt
./partytime -s scripts/week7.script > week7.rslt; diff week7.rslt ./unittest/week7.rslt
./partytime -s scripts/week8.script > week8.rslt; diff week8.rslt ./unittest/week8.rslt
./partytime -s scripts/week9.script > week9.rslt; diff week9.rslt ./unittest/week9.rslt
set +x
