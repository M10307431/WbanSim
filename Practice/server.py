import SocketServer
import time
from threading import Thread
import pickle
import sys

global Task

class service(SocketServer.BaseRequestHandler):    
    def handle(self):
        global Task
        Task = []
        idle = 1
        print "Client connected with ", self.client_address
        while True:
            self.request.settimeout(0.0)
            try:
                recv = self.request.recv(1024) #receive the data
                taskCC = pickle.loads(recv)
                taskCC['Remain'] = taskCC['Exe']
                Task.append(taskCC)
                print "{}:{}_{}".format(self.client_address,taskCC['GW'],taskCC['id'])
            except:
                pass

            #===== sched new =====
            taskcur = None
            sched_new = None
            if len(Task):
                for d in Task:
                    if sched_new==None:
                        if d['Remain']>0:
                            sched_new = d
                            print "TASK",d['id'],"(",d['Cnt'],")\tRUN"
                    else:
                        if sched_new['Deadline']>d['Deadline'] and d['Remain']>0:
                            sched_new = d
                            print "TASK",d['id'],"(",d['Cnt'],")\tPREEMPT"

            taskcur = sched_new
            #===== exec =====
            time.sleep(0.01)
            if taskcur!=None:
                idle = 1
                for d in Task:
                    if d['id']==taskcur['id'] and d['GW']==taskcur['GW']:
                        d['Remain']-=0.02
                        if d['Remain']<=0:
                            Task.remove(d)
                            data_string = pickle.dumps(taskcur)
                            try:
                                self.request.send(data_string)
                                print "TASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                            except:
                                print "Send Error"
                            taskcur = None
            else:
                if idle == 1:
                    idle = 0
                    print "\tIDLE"

        self.request.close()

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

if __name__ == "__main__":
    t = ThreadedTCPServer(('localhost',8888), service)
    try:
        t.serve_forever()
    except:
        t.shutdown()
        t.server_close()
        print "Server shut down"

