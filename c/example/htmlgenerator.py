#!/usr/bin/python
# -*- coding: UTF-8 -*-
"""
This python script is example of usage of gsepg-dataq program.

Script generates html page. 

Script uses cjson package - it can be installed by typing: 
apt-get install python-cjson  on Debian (Ubuntu) based systems.
"""

# import standard packages
import re
import os
import sys
from time import strftime, gmtime, time, strptime
from datetime import timedelta
# 3rd party packages
try:
    import cjson
except:
    print 'Please install cjson python package. On debian based systems should work: apt-get install python-cjson '
    sys.exit(1)

# global variables
files_to_read = list()
read_programs = dict()
CHANNELS = dict({
    257: u'ČT 1',
    258: u'ČT 2',
    259: u'ČT 24',
    260: u'ČT 4',
    16641: u'ČRo 1',
    16642: u'ČRo 2',
    16643: u'ČRo 3',
    16644: u'ČRo Wave',
    16645: u'ČRo D-dur',
    16646: u'ČRo Leonardo',
    770: u'Prima Cool',
    769: u'Prima',
    513: u'Nova',
    514: u'Nova Cinema',
})
TODAY = strftime('%Y-%m-%d')
TOMORROW = strftime('%Y-%m-%d', gmtime(time() + 24*3600))
TIME_FORMAT = '%H:%M:%S'

def decode_json(data):
    return eval(data,{'true': True, 'false': False, 'null': None})

def add_time(start, duration):
    dur = strptime(duration, TIME_FORMAT)
    st = strptime(start, TIME_FORMAT)
    time_dur = timedelta(hours=dur[3], minutes=dur[4], seconds=dur[5])
    time_st = timedelta(hours=st[3], minutes=st[4], seconds=st[5])
    res = time_st + time_dur
    return strftime(TIME_FORMAT, gmtime(res.seconds))

def in_interval(date_time, at_day, from_hour, to_hour):
    # at day will be string compared
    sday, stime = date_time.split(' ')
    if sday != at_day:
        return False
    ptime = strptime(stime, TIME_FORMAT)
    hour = ptime[3]
    if hour >= from_hour and hour < to_hour:
        return True
    return False

def collect_files(dirname):
    for item in os.listdir(dirname):
        if item.endswith('.json'):
            fname = '%s/%s' % (dirname, item)
            files_to_read.append(fname)

def process_descriptor(dtr, prog_item):
    if dtr['id'] == 'short event':
        prog_item['name'] = dtr['name']
        prog_item['lang'] = dtr['lang']
        prog_item['description'] = dtr['text']
    elif dtr['id'] == 'extended event':
        if 'extended description' not in prog_item:
            prog_item['extended description'] = ''
        for desc in dtr['descriptions']:
            prog_item['extended description'] += desc['description']
    elif dtr['id'] == 'parental rating':
        for r in dtr['ratings']:
            age = int(r['age'])
            if age == 0 or age > 18:
                continue
            prog_item['parental rating'] = age

def process_data(data):
    if data['service_id'] not in CHANNELS.keys():
        return None
    chann = CHANNELS[data['service_id']]
    for evt in data['events']:
        if evt['date'] not in (TODAY, TOMORROW,):
            continue
        if chann not in read_programs:
            read_programs[chann] = dict()
        key = '%s %s' % (evt['date'], evt['start_time'])
        if key not in read_programs:
            read_programs[chann][key] = dict()
        prog_item = read_programs[chann][key]
        prog_item['end time'] = add_time(evt['start_time'], evt['duration'])
        prog_item['start time'] = evt['start_time']
        for dtr in evt['descriptors']:
            if not dtr:
                continue
            process_descriptor(dtr, prog_item)

def process_files():
    out = list()
    for item in files_to_read:
        data = cjson.decode(file(item, 'r').read().decode('utf-8'))
        #data = decode_json(file(item, 'r').read().decode('utf-8'))
        data_dict = data['data']
        for service in data_dict:
            process_data(service)

    for item in files_to_read:
        print 'Deleting %s' % item
        # TODO delete file
    return out

def write_utf(fp, ustring):
    fp.write(ustring.encode('utf-8'))

def write_event(fp, evt):
    fp.write('<div class="program">\n')
    fp.write('<span class="start-time"> %s</span>\n' % evt['start time'])
    fp.write('&ndash; <span class="end-time">%s</span>\n' % evt['end time'])
    write_utf(fp, u'<div class="name">%s</div>\n' % evt['name'])
    write_utf(fp, u'<div class="description">%s</div>\n' % evt['description'])
    if 'extended description' in evt:
        write_utf(fp, u'<div class="extended-description">%s</div>\n' % evt['extended description'])
        fp.write('<div class="extended-toggle">&nbsp;</div>\n');
    if 'parental rating' in evt:
        fp.write('<div class="parental">Parental rated content under age %d</div>\n' % evt['parental rating'])
    fp.write('</div> <!-- program -->\n')

def generate_blocks(day):
    ch_blocks = dict()
    for channel in read_programs:
        blocks = dict({
            '0:00-8:00': [],
            '8:00-12:00': [],
            '12:00-16:00': [],
            '16:00-20:00': [],
            '20:00-0:00': []
        })
        for date_key in read_programs[channel]:
            # TODO too ugly, refactor this!
            key = None
            if in_interval(date_key, day, 0, 8): 
                key = '0:00-8:00'
            elif in_interval(date_key, day, 8, 11): 
                key = '8:00-12:00'
            elif in_interval(date_key, day, 12, 15): 
                key = '12:00-16:00'
            elif in_interval(date_key, day, 16, 19): 
                key = '16:00-20:00'
            elif in_interval(date_key, day, 20, 23): 
                key = '20:00-0:00'
            if key:
                blocks[key].append(ProgramItem(date_key, read_programs[channel][date_key]))
        for b in blocks:
            blocks[b].sort()
        ch_blocks[channel] = blocks
    return ch_blocks

def create_html(programs, out_filename):
    today_blocks = generate_blocks(TODAY)
    tomorrow_blocks = generate_blocks(TOMORROW)

    fp = file(out_filename, 'w')
    fp.write(file('header.inc', 'r').read())
    for chann in today_blocks:
        write_utf(fp, u'<div class="channel"> <div class="channel-name">%s</div>\n' % chann)
        for block in today_blocks[chann]:
            for evt in today_blocks[chann][block]:
                write_event(fp, evt.data)
        fp.write('</div>')
    fp.write(file('footer.inc', 'r').read())
    fp.close()

def run(dirname):
    collect_files(dirname)
    process_files()
    create_html(read_programs, 'myprograms.html')

class ProgramItem:
    """ 
    Encapsulate date_time key and data. 
    Also provides __cmp__ method to sort list of ProgramItems.
    """
    def __init__(self, date_time, data):
        self.data = data
        self.date_time = date_time

    def __cmp__(self, other):
        if self.date_time > other.date_time:
            return 1
        elif self.date_time < other.date_time:
            return -1
        return 0

if __name__ == '__main__':
    run('outputdir')
