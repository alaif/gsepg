"""
tsdecoder.py

Jonas Fiala, 2007

Dekoduje Transport Stream (MPEG-2 norma http://en.wikipedia.org/wiki/MPEG_transport_stream). 
Vyfiltrovany payload DVB paketu zabira cca 97.8%.


Puvod delky 188B:
Because MPEG-2 wanted these packets to be carried over ATM. At that time, according to the AAL which was envisaged, ATM cells were supposed to have a payload of 47 bytes.
188 = 4 * 47. 


http://www.lucike.info/index.htm?http://www.lucike.info/page_digitaltv_buf.htm

ETSI - EPG 03 - binary encoding.pdf, 4 Encoding: binarni kodovani EPG/XML
EPG/XMLBinary: | TAG | LENGTH | VALUE | a tak porad dokola
strana 9 - binarni struktura

Binary Object Structure:
---------------------------
Top level element  (element tags serviceinfo=0x02, epg=0x03)
 |
 +-- nested element no. 1
 +-- nested element no. 2
 +-- ...

elementStructure() { 
 element_tag      //8 bits uimsbf 
 element_length   //8 bits uimsbf 
 If (element_length == 0xFE) { 
  extended_element_length  //16 bits uimsbf 
 } 
 If (element_length == 0xFF) { 
  extended_element_length  //24 bits uimsbf 
 } 
 for (i=0; I<element_length or extended_element_length; i++) { 
	element_data_byte   //8 bits uimsbf (encoded in order: Attributes, Child elems, CDATA content-string)
	                    //Top level elem: Attributes, [String token table], [Default currentID], Child elements, CDATA content.
 }
} 

"""

import logging
import os
from common import StreamType

# constants
PACKET_SIZE = 188 #in Bytes
BUFFER_SIZE = 80 * PACKET_SIZE
SEEK_RELATIVE = os.SEEK_CUR

# logging
log = logging.getLogger('fia112.epg.tsdecoder')

def __num2bitList(number):
	out = []
	cp = number
	while cp != 0: # posun cisla doprava
		bit = cp % 2
		out.append(bit)
		cp = cp / 2
	out.reverse()
	return out


def __fill2length(list, value, length):
	''' vyplni list hodnotami value, zleva do poctu len '''
	if len(list) >= length:
		return list
	out = list[:]
	for i in range( length - len(list) ):
		out.insert(0, value)
	return out


def getBits(chars):
	''' vyplivne jednotlive bity predaneho znaku '''
	out = []
	if type(chars) == type(''):
		if len(chars) > 1:
			#log.debug('more bytes=(%s)' % chars)
			for i in range(len(chars)):
				for bit in getBits(chars[i]):
					out.append(bit)
			#log.debug('getBits(%s): %s' % (chars, str(out)) )
			return out
		else:
			cp = ord(chars)
	else:
		cp = chars
	out = __fill2length( __num2bitList(cp), 0, 8 )
	#log.debug('getBits(%s)(0x%x): %s' % (chars, ord(chars), str(out)) )
	return out


def bitString2Int(s):
    v,base=0,1
    for i in range(len(s)): 
        if s[-(i+1)] == '1': v=v+base
        base=base*2
    return v


def bits2dec(bitList):
    ''' FIXME vypujcena funkce z http://maja.dit.upm.es/~afc/pid0parser-py.txt pro overeni. '''
    out = 0
    r = range( len(bitList) )
    r.reverse()
    for c in r:
        if bitList[c] == 1:
            out += pow(2, c)
    return out


def isPacketStart(fileobj):
	''' detekuje zacatek paketu, vraci pozici souboru zpet '''
	data = fileobj.read(1)
	if data == '':
		return None
	if ord(data) == 0x47:
		pos = fileobj.tell()
		fileobj.seek(-1, SEEK_RELATIVE)
		return True
	return False


def packetHeader(fileobj):
	''' parsuje obsah hlavicky paketu za synchronizacnim bytem 0x47 '''
	out = {}
	f = fileobj
	data = f.read(1)
	if data == '':
		return None
	# --- transport stream 4-byte prefix (0x47 incl.) ---
	if ord(data) == 0x47:
		out['sync byte'] = ord(data)
		twoB = f.read(2)
		b = getBits(twoB)
		out['tei'] = b[0]  #transport error indicator
		out['pusi'] = b[1] #payload unit start indicator
		out['tp'] = b[2]   #transport priority
		out['pid'] = bits2dec( b[3:] ) #Packet ID (13bit)
		third = f.read(1)
		b = getBits(third)
		out['scram'] = b[0:2] #scrambling control (2bit)
		out['adapt'] = b[2] #adaptation field list
		out['payload_presence'] = b[3] #payload data exist
		out['continuity'] = b[4:] #continuity counter
	return out


class DVBStream(StreamType):
    def __init__(self, pid, fileObject):
        self.pid = pid
        self.__fo = fileObject
        self._buf = ''
        self._position = 1
        self.__previousBufferLength = 0
        self._startFound = False
        self.end = False  # indikuje konec fileobjectu, ze ktereho se ctou data
    
    @property
    def position(self):
        return self._position + self.__previousBufferLength

    def __fillBuff(self):
        ''' naplni buffer do velikosti BUFFER_SIZE nebo mensi, pokud uz neni co cist '''
        f = self.__fo
        self.__previousBufferLength += len(self._buf)
        self._position = 1
        self._buf = '' # erase buffer
        while not self.end:
            pstart = isPacketStart(f)
            if pstart == None:
                self.end = True
                log.info('End of fileobject. Nothing left to read.')
                break
            if not pstart: # nic zajimaveho tak posun o jeden byte vpred
                f.seek(1, SEEK_RELATIVE)
                continue
            #log.debug('Sync byte 0x47')
            header = packetHeader(f)
            log.debug( header )
            # 1. po nacteni hlavicky zbyva precist dalsich 184B payloadu (paket size=188B)
            payload = f.read(184)
            # filtrovani PID 
            if self.pid and header['pid'] != self.pid: # the PID is not mine
                continue
            if not header['pusi'] and not self._startFound: #pusi == 1 paket ktery zacina TL elementem(?)
                log.debug('Waiting first packet with toplevel element...')
                continue
            if not self._startFound:
                log.info('FIRST PACKET WITH Toplevel element!')
                self._startFound = True
            self._buf += payload
            if len(self._buf) >= BUFFER_SIZE:
                break

    def readByte(self, increment=True):
        if self._position > len(self._buf):
            self.__fillBuff()
        out = self._buf[self._position - 1]
        if increment:
            self._position += 1
        return out

    def tell(self):
        return self.position
