'''
tsdecoder.py

Jonas Fiala, 2007

Dekoduje Transport Stream (MPEG-2 norma http://en.wikipedia.org/wiki/MPEG_transport_stream). 
Vyfiltrovany payload DVB paketu zabira cca 97.8%.

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

'''

import logging


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
			#logging.debug('more bytes=(%s)' % chars)
			for i in range(len(chars)):
				for bit in getBits(chars[i]):
					out.append(bit)
			#logging.debug('getBits(%s): %s' % (chars, str(out)) )
			return out
		else:
			cp = ord(chars)
	else:
		cp = chars
	out = __fill2length( __num2bitList(cp), 0, 8 )
	#logging.debug('getBits(%s)(0x%x): %s' % (chars, ord(chars), str(out)) )
	return out


def bits2dec(bitList):
	out = 0
	r = range( len(bitList) )
	r.reverse()
	for c in r:
		if bitList[c] == 1:
			out += pow(2, c)
	return out


def isPacketStart(stream):
	''' detekuje zacatek paketu, vraci pozici souboru zpet '''
	CURRENT_POS = 1
	data = stream.read(1)
	if data == '':
		return None
	if ord(data) == 0x47:
		pos = stream.tell()
		stream.seek(-1, CURRENT_POS)
		return True
	return False


def packetHeader(stream):
	''' parsuje obsah hlavicky paketu za synchronizacnim bytem 0x47 '''
	out = {}
	f = stream
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
		out['pid'] = '0x%x' % bits2dec( b[3:] ) #Packet ID (13bit)
		third = f.read(1)
		b = getBits(third)
		out['scram'] = b[0:2] #scrambling control (2bit)
		out['adapt'] = b[2] #adaptation field list
		out['payload_presence'] = b[3] #payload data exist
		out['continuity'] = b[4:] #continuity counter
	return out
