from socket import socket, AF_INET, SOCK_STREAM
import socket as sk
import msvcrt as m
import time,datetime
import sys
import pickle
from select import select
import threading
from threading import Thread
from time import clock

timeout = 3

def delay_1ms():
    n = 1
    for i in range(5):
        for j in range(998):
            n += n+i

def myOFLD():
    s=socket(AF_INET, SOCK_STREAM)
    s.connect((host, port))
    s.settimeout(0.0)
    print "Connected"
        
class Client(Thread):
    def __init__(self, host, port):
        Thread.__init__(self)
        self.s=socket(AF_INET, SOCK_STREAM)
        self.s.connect((host, port))
        self.s.send('Hi')

    def run(self):
        msg = ''
        while True:
            self.s.settimeout(0.0)
            try:
                msg=self.s.recv(1024)
                print 'Server:',msg
            except:
                pass
            if msg:
                try:
                    self.s.send(msg)
                except:
                    pass
        self.s.shutdown()
        self.s.close()
            
    


def main():
    s=socket(AF_INET, SOCK_STREAM)
    s.connect((host, port))
    x = 0
    while True:
        s.settimeout(0.0)
        try:
            msg=s.recv(1024)
            print 'Server:',msg
        except:
            pass
        try:
            data=raw_input("Client:")
            s.send(data)
        except:
            pass
    s.shutdown()
    s.close()

if __name__=="__main__":
    host='140.118.206.169'
    port=12345
    #main()
    start = time.time()
    #delay_1ms()
    #LocalEDF()
    #myOFLD()
    t1 = Client(host, port)
    t2 = Client(host, port)
    t1.start()
    t2.start()
    finish = time.time()
    print (finish-start)
