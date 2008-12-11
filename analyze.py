'''
analyze.py

http://en.wikipedia.org/wiki/MPEG_transport_stream
'''

import re
import sys
import os.path
import logging
import tsdecoder

def setupLogging():
	lFormat = '%(asctime)s %(levelname)s %(message)s'
	lDateFormat = '%Y-%m-%d %H:%M:%S'
	lLevel = logging.ERROR
	logging.basicConfig(
		level = lLevel,
		format = lFormat,
		datefmt = lDateFormat
	)
	logging.root.setLevel(lLevel)


setupLogging()
if len(sys.argv) < 2:
	print 'Specify file to load.'
	sys.exit(1)
filename = sys.argv[1]
if not os.path.exists(filename):
	print 'Specify existing file to load.'
	sys.exit(2)
f = file(filename, 'rb')
c = 0
total = ''
while True:
	pstart = tsdecoder.isPacketStart(f)
	if pstart == None:
		break
	if pstart:
		c += 1
		logging.debug('Sync byte 0x47')
		logging.debug( tsdecoder.packetHeader(f) )
		# 1. po nacteni hlavicky zbyva precist dalsich 184B payloadu (paket size=188B)
		payload = f.read(184)
		total += payload
		if tsdecoder.isPacketStart(f): # kontrola, ze jsem neprecetl vic bytu nez jsem mel
			logging.debug('Next packet is there! :-)')
		if c >= 180: # kazdych 180 paketu vyblit nacteny payload na stdout
			sys.stdout.write( total )
			total = ''
f.close()
