import serial
import time

print "Setting up port"
port=serial.Serial('/dev/ttyACM0',9600)
sample_rate=1 #update every 10s
while 1:
    now=time.time()
    next_sample=now+sample_rate

    print "Writing y..."
    port.write("y");
    raw=port.read(port.inWaiting())
    if len(raw)>0:
        for c in raw:
            print ord(c)
    time.sleep(next_sample-time.time())
