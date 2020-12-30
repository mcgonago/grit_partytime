#!/bin/bash
set -x
./partytime -s unittest/test.script > test.rslt; diff test.rslt ./unittest/test.rslt
set +x
