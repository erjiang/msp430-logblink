#!/usr/bin/env python

import serial
import subprocess
import sys
import time

if len(sys.argv) < 3:
    print "Usage: tailremote.py hostname file"
    exit(1)

hostname = sys.argv[1]
filename = sys.argv[2]

print "Setting up port"
port=serial.Serial('/dev/ttyACM0', 9600)

#turn board LEDs off
port.write("O")
port.write("0")
port.write("6")

print "Connecting to remote host"
sshargs = ["ssh", hostname, "tail", "-f", "-n", "0", filename]
ssh = subprocess.Popen(sshargs, stdout=subprocess.PIPE)

port.write("I")
port.write("0")
port.write("6");

time.sleep(0.1)

port.write("T")
while 1:
    line = ssh.stdout.readline()
    if "GET " in line:
        print "get"
        port.write("7")
    elif "POST " in line:
        print "post"
        port.write("5")
    raw=port.read(port.inWaiting())
    print raw
