import sys
import struct
import logging

# Tag, attributes lengths
LEN_16 = chr(0xfe)
LEN_24 = chr(0xff)

# Top level tags -- 0x02, 0x03
TL_EPG = chr(0x02)
TL_SERVICEINFO = chr(0x03)

# All element tags ranges 0x10 .. 0x7e  (page 33 of spec.)
TAG_SHORT_NAME = chr(0x10)
TAG_MEDIUM_NAME = chr(0x11)
TAG_LONG_NAME = chr(0x12)
TAG_MEDIA_DESCRIPTION = chr(0x13)

def detectTag(byte, fp):
    if byte == TL_EPG:
        print '* EPG top level element at %d' % fp.tell()
    elif byte == TL_SERVICEINFO:
        print '* Service Information top level element at %d' % fp.tell()
    else:
        element(byte, fp)

def element(byte, fp):
    """
    # element_tag 8bit
    # element_length 8bit
    if element_length == 0xfe then element_length is 16bit
    if element_length == 0xff then element_length is 24bit
    for i in range(element_length):
        element_data_byte ... 8bit
    """
    if byte != TAG_MEDIA_DESCRIPTION: # byte should by so called element_tag
        return
    et = byte
    et_length = fp.read(1)
    if et_length == LEN_16:
        tmp = fp.read(1)
        number = '%s%s' % (et_length, tmp)
        et_length = struct.unpack('h', number)[0]
    elif et_length == LEN_24:
        print 'Not implemented!'
        return
    else:
        et_length = ord(et_length)
    print '=== Element length: %d' % et_length
    data = []
    for i in range(et_length):
        data.append(fp.read(1))

def attribute():
    pass

def parseIt(fp):
    while True:
        b = fp.read(1)
        if not b:
            print 'No data to read'
            break
        detectTag(b, fp)


if __name__ == '__main__':
    fp = file(sys.argv[1], 'rb')
    parseIt(fp)
    fp.close()
