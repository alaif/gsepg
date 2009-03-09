'''
common.py

Modul obsahuje rozhrani trid pouzivajicich se ve vice modulech.

'''

class NotImplementedError(Exception):
    pass

class StreamType(object):
    def readByte(self, increment=True):
        raise NotImplementedError()

    def actualByte(self):
        return self.readByte(increment=False)

    def tell(self):
        raise NotImplementedError()
