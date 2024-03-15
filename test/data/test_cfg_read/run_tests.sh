#/bin/sh

for i in 1 2 3 4 5 6 7 8 ; do ./test_cfg_read ../hisinad/test/data/test_cfg_read/$i.ini ; done  &> test_cfg_read.output 
diff test_cfg_read.output ../hisinad/test/data/test_cfg_read/result.output

