#!/usr/bin/env python

import subprocess
import serial
import sys

if len(sys.argv) < 3:
    print "Usage: tailremote.py hostname file"
    exit(1)

hostname = sys.argv[1]
filename = sys.argv[2]

#print "Setting up port"
#port=serial.Serial('/dev/ttyACM0', 9600)

print "Connecting to remote host"
sshargs = ["ssh", hostname, "tail", "-f", "-n", "0", filename]
ssh = subprocess.Popen(sshargs, stdout=subprocess.PIPE)

while 1:
    line = ssh.stdout.readline()
    if "GET " in line:
        print "get"
    elif "POST " in line:
        print "post"
