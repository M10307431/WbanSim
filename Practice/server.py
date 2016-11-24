import SocketServer
import time
from threading import Thread

#global cmd
#cmd = '0'

class service(SocketServer.BaseRequestHandler):    
    def handle(self):
        global cmd
        print "Client connected with ", self.client_address
        while True:
            self.request.settimeout(0.1)
            try:
                recv = self.request.recv(1024).strip() #receive the data            
                if recv == "exit":
                    break
                elif recv:
                    #cmd = recv
                    print "{}:{}".format(self.client_address,cmd)
            except:
                pass
            self.request.send(recv)
            time.sleep(0.2)
        self.request.close()

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

if __name__ == "__main__":
    t = ThreadedTCPServer(('',12345), service)
    try:
        t.serve_forever()
    except:
        t.shutdown()
        print "Server shut down"
