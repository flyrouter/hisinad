#/bin/sh

for i in 1 2 3 ; do ./test_cfg_sensor ../hisinad/test/data/test_cfg_sensor/$i.ini ; done  &> test_cfg_sensor.output 
diff test_cfg_sensor.output ../hisinad/test/data/test_cfg_sensor/result.output

