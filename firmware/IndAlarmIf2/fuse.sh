#! /bin/sh

avrdude -c stk500 -P /dev/ttyACM0 -p m1280 -t <<END
w lfuse 0 0xd7
w hfuse 0 0xd9
w efuse 0 0xf4
q
END


