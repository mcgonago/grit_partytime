#!/bin/bash
./scripts/column_kom_sprints.sh > column_kom_sprints.rslt; diff column_kom_sprints.rslt ./unittest/column_kom_sprints.rslt
./scripts/column_following.sh > column_following.rslt; diff column_following.rslt ./unittest/column_following.rslt
./scripts/column_results.sh > column_results.rslt; diff column_results.rslt ./unittest/column_results.rslt
./partytime -s scripts/following.script > following.rslt; diff following.rslt ./unittest/following.rslt
./partytime -s scripts/following2.script > following2.rslt; diff following2.rslt ./unittest/following2.rslt
./partytime -s scripts/tempoish.script > tempoish.rslt; diff tempoish.rslt ./unittest/tempoish.rslt
./partytime -s scripts/tempoish_cat.script > tempoish_cat.rslt; diff tempoish_cat.rslt ./unittest/tempoish_cat.rslt
./partytime -s scripts/richmond.script > richmond.rslt; diff richmond.rslt ./unittest/richmond.rslt
./partytime -s scripts/richmond_tempoish.script > richmond_tempoish.rslt; diff richmond_tempoish.rslt ./unittest/richmond_tempoish.rslt
./partytime -s scripts/richmond_tempoish_following.script > richmond_tempoish_following.rslt; diff richmond_tempoish_following.rslt ./unittest/richmond_tempoish_following.rslt
