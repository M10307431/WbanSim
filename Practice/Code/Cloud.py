import SocketServer
import time
from threading import Thread
import pickle
import sys

global sched
global lock
global Task
global GW1
global GW2
global GW3
global timetick
global speed

def scheduler():
    global Task
    global lock
    global timetick
    global speed
    idle = 1
    taskcur = None

    while True:
        
        #===== sched new =====
        sched_new = None
        if len(Task) and lock==0:
            for d in Task:
                try:
                    if sched_new==None:
                        if d['Remain']>0:
                            sched_new = d
                    else:
                        if sched_new['Deadline']>d['Deadline'] and d['Remain']>0:
                            sched_new = d
                except:
                    break

        #=== preempt ===
        try:    
            if sched_new!=None and taskcur==None and lock==0:
                taskcur = sched_new
                print "TASK",taskcur['id'],"(",taskcur['Cnt'],")\tRUN"
            elif sched_new!=None and sched_new['Deadline']<taskcur['Deadline'] and lock==0:
                taskcur = sched_new
                print "TASK",taskcur['id'],"(",taskcur['Cnt'],")\tPREEMPT"
        except:
            break
        
        #===== exec =====
        time.sleep(0.001)
        timetick += 0.001
        if taskcur!=None and lock==0:
            idle = 1
            for d in Task:
                try:
                    if d['id']==taskcur['id'] and d['GW']==taskcur['GW']:
                        d['Remain'] -= 0.001*speed
                        if d['Remain']<=0:
                            #print "TASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                            taskcur = None
                except:
                    break
        elif lock==0:
            if idle == 1:
                idle = 0
                print "\tIDLE"
    

class service(SocketServer.BaseRequestHandler):    
    def handle(self):
        global sched
        global lock
        global Task
        global GW1
        global GW2
        global GW3
        global timetick
        global speed
        GW = 0
        ADM = 0
        print "Client connected with ", self.client_address

        if not sched:
            sched = 1
            while True:
                scheduler()
        else:
            while True:
                self.request.settimeout(0.0)
                try:
                    recv = self.request.recv(1024) #receive the data
                    taskCC = pickle.loads(recv)
                    if isinstance(taskCC,list):
                        ADM = 1
                        if taskCC[0]['GW']==1:
                            GW1 = taskCC
                            if len(Task):
                                Task[0]['time'] = timetick
                            self.request.send(pickle.dumps(Task))
                            self.request.settimeout(None)
                            recv = self.request.recv(1024)
                            self.request.send(pickle.dumps(GW2))
                            recv = self.request.recv(1024)
                            self.request.send(pickle.dumps(GW3))
                        elif taskCC[0]['GW']==2:
                            GW2 = taskCC
                            if len(Task):
                                Task[0]['time'] = timetick
                            self.request.send(pickle.dumps(Task))
                            self.request.settimeout(None)
                            recv = self.request.recv(1024)
                            self.request.send(pickle.dumps(GW1))
                            recv = self.request.recv(1024)
                            self.request.send(pickle.dumps(GW3))
                        elif taskCC[0]['GW']==3:
                            GW3 = taskCC
                            if len(Task):
                                Task[0]['time'] = timetick
                            self.request.send(pickle.dumps(Task))
                            self.request.settimeout(None)
                            recv = self.request.recv(1024)
                            self.request.send(pickle.dumps(GW1))
                            recv = self.request.recv(1024)
                            self.request.send(pickle.dumps(GW2))
                    else:
                        ADM = 0
                        taskCC['Remain'] = taskCC['Exe']
                        taskCC['Deadline'] = timetick + taskCC['VD']
                        lock += 1
                        Task.append(taskCC)
                        lock -= 1
                        GW = taskCC['GW']
                        #self.lock.acquire()
                        print "{}:GW{}_TASK{}".format(self.client_address,taskCC['GW'],taskCC['id'])
                        #self.lock.release()
                except:
                    pass
                
                if ADM == 0:            
                    for d in Task:
                        if d['GW']==GW and d['Remain']<=0:
                            lock += 1
                            taskcur = d
                            data_string = pickle.dumps(taskcur)
                            Task.remove(d)
                            try:
                                self.request.send(data_string)
                                #print "TASK",taskcur['id'],"(",taskcur['Cnt'],")\tback"
                                #self.lock.acquire()
                                print "TASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                                #self.lock.release()
                                lock -= 1 
                            except:
                                print "Send Error"

        self.request.close()

class ThreadedTCPServer(SocketServer.ThreadingMixIn, SocketServer.TCPServer):
    pass

if __name__ == "__main__":
    t = ThreadedTCPServer(('',12345), service)
    timetick = 0
    speed = 1
    sched = 0
    lock = 0
    Task = []
    GW1 = []
    GW2 = []
    GW3 = []
    try:
        t.serve_forever()
    except:
        t.shutdown()
        t.server_close()
        print "Server shut down"

