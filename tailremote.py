#!/usr/bin/env python

import subprocess
import serial
import sys

if len(sys.argv) < 3:
    print "Usage: tailremote.py hostname file"
    exit(1)

hostname = sys.argv[1]
filename = sys.argv[2]

print "Setting up port"
port=serial.Serial('/dev/ttyACM0', 9600)

#turn board LEDs off
port.write("O0")
port.write("O6")

print "Connecting to remote host"
sshargs = ["ssh", hostname, "tail", "-f", "-n", "0", filename]
ssh = subprocess.Popen(sshargs, stdout=subprocess.PIPE)

port.write("I0")
port.write("I6");

while 1:
    line = ssh.stdout.readline()
    if "GET " in line:
        print "get"
        port.write("T7")
    elif "POST " in line:
        print "post"
        port.write("T5")
    raw=port.read(port.inWaiting())
    print raw
