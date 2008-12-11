'''
sniff.py

Nacte z getstream-poempel streamovana EPG data
10.154.114.62
'''
import socket


f = file('streamsniffed', 'wb')
#create an INET, STREAMing socket
s = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
s.connect( ('localhost', 4000) )
s.send('HELLO')
try:
	while True:
		block = s.recv( 1024 )
		f.write(block)
except:
	f.close()
