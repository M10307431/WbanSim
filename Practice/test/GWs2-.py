from socket import socket, AF_INET, SOCK_STREAM
import SocketServer
import time,datetime
import sys
import pickle
import threading
from threading import Thread
from time import clock

timeout = 3

global cc2
global c21

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
        

class MyTCPHandler(SocketServer.BaseRequestHandler):
    def __init__(self,request,client_address,server):
        self.request=request
        self.client_address=client_address
        self.server=server

        print "Client",self.client_address[0]
        self.handle()
                
    def handle(self):
        global cc2
        global c21
        taskCB1 = ''
        taskCB3 = ''
        self.request.settimeout(0.0)
        raw_input('Server Run')
        
        self.request.send('S2')
        while True:

            #===== Recv task =====
            try:
                #taskCBC = cc2.recv(1024)
                taskCB1 = c21.recv(1024)
                taskCB3 = self.request.recv(1024)
            except:
                pass

            #if taskCBC:
                #print 'taskCBC: ',taskCBC
            if taskCB1:
                print 'taskCB1: ',taskCB1
            if taskCB3:
                print 'taskCB3: ',taskCB3

            self.request.send('S2')

            time.sleep(3)

    def finish(self):
        pass

    def setup(self):
        pass


def main():
    s=socket(AF_INET, SOCK_STREAM)
    s.connect((host, port))
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
    port=11111
    HOST, PORT = "", 22222
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    try:
        c21 = socket(AF_INET, SOCK_STREAM)
        c21.connect((host, port))
        c21.settimeout(0.0)
        print "Connected"
        server.serve_forever()
        
    except:
        server.shutdown()
    #main()
    #start = time.time()
    #delay_1ms()
    #LocalEDF()
    #myOFLD()
    #finish = time.time()
    #print (finish-start)
