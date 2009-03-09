import sys
import logging
from common import StreamType
import tsdecoder

# logging
log = logging.getLogger('fia112.epg.parser')

# Tag, attributes lengths
LEN_16 = chr(0xfe)
LEN_24 = chr(0xff)

# Top level tags -- 0x02, 0x03
TL_EPG = chr(0x02)
TL_SERVICEINFO = chr(0x03)

# All element tags ranges 0x10 .. 0x7e  (page 33 of spec.)
RANGE_TAG_BEGIN = 0x10
RANGE_TAG_END = 0x7e

TAGS = {
    'epg':                     0x02,
    'serviceInformation':      0x03,
    'tokenTableElement':       0x04,
    'defaultContentIDElement': 0x05,
    'shortName':               0x10,
    'longName':                0x12,
    'mediaDescription':        0x13,
    'genre':                   0x14,
    'CA':                      0x15,
    'keywords':                0x16,
    'memberOf':                0x17,
    'link':                    0x18,
    'location':                0x19,
    'shortDescription':        0x1a,
    'longDescription':         0x1b,
    'programme':               0x1c,
    'programmeGroups':         0x20,
    'schedule':                0x21,
    'alternateSource':         0x22,
    'programmeGroup':          0x23,
    'scope':                   0x24,
    'serviceScope':            0x25,
    'ensemble':                0x26,
    'frequency':               0x27,
    'service':                 0x28,
    'serviceID':               0x29,
    'epgLanguage':             0x2a,
    'multimedia':              0x2b,
    'time':                    0x2c,
    'bearer':                  0x2d,
    'programmeEvent':          0x2e,
    'relativeTime':            0x2f,
    'simulcast':               0x30,
}

# reverse tag table (for more friendly logging etc.)
TAGS_R = {}
for k, v in TAGS.items():
    TAGS_R[v] = k.upper()


def detectTag(stream):
    byte = ord(stream.readByte())
    if byte == TAGS['epg']:
        log.debug( '* EPG top level element at %d' % stream.tell() )
    elif byte == TAGS['serviceInformation']:
        log.debug( '* SI  top level element at %d' % stream.tell() ) # Service Information top level element
    else:
        element(stream)

def element(stream):
    """
    # element_tag 8bit
    # element_length 8bit
    if element_length == 0xfe then element_length is 16bit
    if element_length == 0xff then element_length is 24bit
    for i in range(element_length):
        element_data_byte ... 8bit
    """
    et = ord(stream.actualByte())
    """
    if byte != TAG_MEDIA_DESCRIPTION: # byte should by so called element_tag
        return
    """
    if et < RANGE_TAG_BEGIN or et > RANGE_TAG_END:
        return
    et_length = stream.readByte()
    if et_length == LEN_16:
        """
        import struct
        tmp = stream.readByte()
        number = '%s%s' % (et_length, tmp)
        et_length = struct.unpack('h', number)[0]
        """
        tmp = stream.readByte()
        et_length = ord(tmp) << 8 | ord(et_length)
    elif et_length == LEN_24:
        # "%d" % (a[2]<<16 | a[1] << 8 | a[0])
        mid = ord( stream.readByte() )
        end = ord( stream.readByte() )
        et_length = end << 16 | mid << 8 | ord( et_length )
    else:
        et_length = ord(et_length)
    if et not in TAGS_R:
        return
    log.debug( '=== Element %s length: %d' % (TAGS_R[et], et_length) )
    #log.debug( '=== Element %02X length: %d' % (et, et_length) )
    data = []
    for i in range(et_length):
        data.append( stream.readByte() )
    return data

def attribute():
    pass

def parseIt(fp):
    """ obsolete, should use StreamType instance """
    while True:
        b = fp.read(1)
        if not b:
            log.info( 'No data to read' )
            break
        detectTag(b, fp)


if __name__ == '__main__':
    fp = file(sys.argv[1], 'rb')
    parseIt(fp)
    fp.close()
