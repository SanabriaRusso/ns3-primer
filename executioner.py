#!/usr/bin/python

import sys, os
import time
import subprocess

now = lambda: int(round(time.time() * 1000))

class SuperTable(object):
	def __init__(self):	
		self.simulationTime = 1
		self.distance = 1.0
		self.mcs = 0
		self.channelWidth = 20
		self.gi = 0
		self.stations = 1
		self.aps = 1

		self.command = ""
		self.configuration = 0

	def setup(self, configuration):
		run = "./waf --run \"scratch/quick-vht-wifi "

		if configuration == 0:
			self.stations = 1
			self.simulationTime = 2
			self.command = run + "--stations=" + str(self.stations) + "\""
			print "Command: %s" % self.command

		elif configuration == 1:
			self.stations = 1
			self.mcs = 7
			self.command = (run + "--stations=" + str(self.stations) + " "
							+ "--mcs=" + str(self.mcs) + " "
							+ "\"")

			print "Command: %s" % self.command


class Picker(object):
	def __init__(self):
		self.table = SuperTable()

	def fire(self):
		if self.table.configuration == 0:
			t0 = now()
			temp = subprocess.check_call(self.table.command, shell=True)
			print "--->Test duration: %s" % (now() - t0)
		else:
			os.system(self.table.command)


def main(argv):
	conf = argv[0]
	
	exp = Picker()
	exp.table.setup(int(conf))
	exp.fire()

if __name__ == "__main__":
	main(sys.argv[1:])
