#!/usr/bin/python
'''
analyze.py

http://en.wikipedia.org/wiki/MPEG_transport_stream
'''

import re
import sys
import os.path
import logging, logging.config
import tsdecoder
from tsdecoder import DVBStream
import parser

def setupLogging():
	lFormat = '%(asctime)s %(levelname)s %(message)s'
	lDateFormat = '%Y-%m-%d %H:%M:%S'
	lLevel = logging.DEBUG
	logging.basicConfig(
		level = lLevel,
		format = lFormat,
		datefmt = lDateFormat
	)
	logging.root.setLevel(lLevel)


#setupLogging()
logging.config.fileConfig("logger.conf")
log = logging.getLogger('fia112.epg.analyze')
if len(sys.argv) < 2:
	sys.stdout.write( 'Specify file to load.\n' )
	sys.exit(1)
filename = sys.argv[1]
if not os.path.exists(filename):
	sys.stdout.write( 'Specify existing file to load.\n' )
	sys.exit(2)
f = file(filename, 'rb')
s = DVBStream(0x900, f)
while not s.end:
    # TODO zde udelat ten stavovy automat, ktery bude postupne tagy parsovat (nezle jen slepe hledat byty indikujici elementy
    epg_len = parser.detectEpg(s)
    if not epg_len:
        continue
    s.readByte() # posun o B dal za delku tagu.
    parser.element(s)
    continue
    #data = parser.getData(s, epg_len)
    #for b in data:
    #    if b == 0x4d:
    #        log.debug('SCHEDULE')
f.close()
