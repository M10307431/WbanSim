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
global Task
global Result
global taskcur
global timetick

def delay_1ms():
    n = 1
    for i in range(5):
        for j in range(998):
            n += n+i

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
        
        Task =[{"Exe":0.37,"Period":1,"Deadline":0,'Arrival':-1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"state":0,"GW":2},
                {"Exe":0.67,"Period":2,"Deadline":0,'Arrival':-2,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"state":0,"GW":2},
                {"Exe":1.13,"Period":4,"Deadline":0,'Arrival':-4,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"state":0,"GW":2}]
        Result ={"Meet":0,"Miss":0,"Total":0}

        self.request.settimeout(0.0)
        raw_input('GW2 Run')
        cc2 = socket(AF_INET, SOCK_STREAM)
        cc2.connect((CChost, CCport))
        print 'Cloud connected'
        cc2.settimeout(0.0)
        c21.settimeout(0.0)

        idle = 1
        hyperperiod = 40
        timetick = 0

        #print datetime.datetime.now().strftime("%H:%M:%S")," Start"
        taskCBC = ''
        taskCB1 = ''
        taskCB3 = ''

        raw_input('GW1 OFLD')
        #---------------------------------------
        #OFLD: -1 Local     #state: 1 pre-proc
        #       0 Cloud             2 exec
        #       * Fog               3 post-proc
        #---------------------------------------
        p_idle = 1.55
        p_comp = 2.9-1.55
        p_trans = 0.3
        offloadingTrans = 0.25
        fogtrans = 0.05
        #===== OFLD =====
        print 'Offloading Decision'
        for t in Task:
            engL = (p_idle+p_comp)*t['Exe']
            engR = 2*(p_idle+p_trans+p_comp/2.0)*offloadingTrans
            if(engL > engR):
                t['OFLD'] = -1
            print "Task",t['id'],' ',t['OFLD']
        
        
        taskcur = None
        try:
            
            while True:
            
                #===== Recv remote task =====
                try:
                    taskCBC = cc2.recv(1024)
                    if taskCBC:
                        taskCBC = pickle.loads(taskCBC)
                        Task[taskCBC['id']-1]['Remain'] = offloadingTrans
                        Task[taskCBC['id']-1]['state'] = 3
                        print "Timetick=",timetick,"\tTASK",taskCBC['id'],"(",taskCBC['Cnt'],")\tbackCC"
                except:
                    pass
                try:
                    taskCB1 = c21.recv(1024)
                    if taskCB1:
                        taskCB1 = pickle.loads(taskCB1)
                        Task[taskCB1['id']-1]['Remain'] = fogtrans
                        Task[taskCB1['id']-1]['state'] = 3
                        print "Timetick=",timetick,"\tTASK",taskCB1['id'],"(",taskCB1['Cnt'],")\tbackGW1"
                except:
                    pass
                try:
                    taskCB3 = self.request.recv(1024)
                    if taskCB3:
                        taskCB3 = pickle.loads(taskCB3)
                        Task[taskCB3['id']-1]['Remain'] = fogtrans
                        Task[taskCB3['id']-1]['state'] = 3
                        print "Timetick=",timetick,"\tTASK",taskCB3['id'],"(",taskCB3['Cnt'],")\tbackGW3"
                except:
                    pass

                sched_new = None
                #===== task arrival =====
                for d in Task:
                    if d['Deadline']<=timetick:
                        if (d['Remain']>0) or (d['state']==1):
                            Result['Miss']+=1
                            print "Timetick=",timetick,"\tTASK",d['id'],"(",d['Cnt'],")\t*****MISS*****"
                        d['Arrival'] = d['Arrival']+d['Period']
                        d['Deadline'] = d['Deadline']+d['Period']
                        d['Remain'] = offloadingTrans if(d['OFLD'] == -1) else d['Exe']
                        d['state'] = 1 if(d['OFLD'] == -1) else 0
                        d['Cnt']+=1
                        Result['Total']+=1

                #===== sched new =====
                for d in Task:
                    if sched_new==None:
                        if timetick>=d['Arrival'] and d['Remain']>0:
                            sched_new = d
                    else:
                        if sched_new['Deadline']>=d['Deadline'] and timetick>=sched_new['Arrival'] and d['Remain']>0:
                            if sched_new['Deadline']==d['Deadline'] and sched_new['OFLD']>d['OFLD']:
                                sched_new = d
                            elif sched_new['Deadline']>d['Deadline']:
                                sched_new = d
                
                #===== preempt =====
                if sched_new!=None and taskcur==None:
                    taskcur = sched_new
                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tRUN"
                elif sched_new!=None and sched_new['Deadline']<=taskcur['Deadline']:
                    if sched_new['Deadline']==taskcur['Deadline'] and sched_new['OFLD']<taskcur['OFLD']:
                        taskcur = sched_new
                        print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tPREEMPT"
                    elif sched_new['Deadline']<taskcur['Deadline']:
                        taskcur = sched_new
                        print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tPREEMPT"                

                #===== exec =====
                time.sleep(0.01)
                timetick += 0.01
                if taskcur!=None and taskcur['OFLD']==-999:
                    idle = 1
                    Task[taskcur['id']-1]['Remain']-=0.01
                    #print Task[taskcur['id']]['Remain']
                    if Task[taskcur['id']-1]['Remain']<=0:
                        print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                        Result['Meet']+=1
                        taskcur = None
                elif taskcur!=None and taskcur['OFLD']==-1:
                    idle = 1
                    Task[taskcur['id']-1]['Remain']-=0.01
                    if Task[taskcur['id']-1]['Remain']<=0:
                        if Task[taskcur['id']-1]['state'] == 1:
                            data_string = pickle.dumps(taskcur)
                            try:
                                cc2.send(data_string)
                                #time.sleep(1)
                                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_C"
                            except:
                                print "Send Error!"
                            taskcur = None
                        elif Task[taskcur['id']-1]['state'] == 3:
                            print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_C"
                            Result['Meet']+=1
                            taskcur = None
                else:
                    if idle ==1:
                        idle = 0
                        print "Timetick=",timetick,"\tIDLE"

                #===== Result =====
                if timetick>=hyperperiod:
                    print "Meet:\t",Result['Meet']
                    print "Miss:\t",Result['Miss']
                    print "Total:\t",Result['Total']
                    print "Meet%:\t",float(Result['Meet'])/float(Result['Total'])
                    break
                            

        except:
            #print "ERROR"
            print "Meet:\t",Result['Meet']
            print "Miss:\t",Result['Miss']
            print "Total:\t",Result['Total']
            print "Meet%:\t",float(Result['Meet'])/float(Result['Total'])
            if cc2:
                cc2.shutdown()
                cc2.close()
            if c21:
                c21.shutdown()
                c21.close()

    def finish(self):
        pass

    def setup(self):
        pass

if __name__=="__main__":
    CChost='140.118.206.169'
    CCport=12345
    Lhost='140.118.206.169'
    Lport=11111
    HOST, PORT = "", 22222
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    try:
        c21 = socket(AF_INET, SOCK_STREAM)
        c21.connect((Lhost, Lport))
        print 'Local connected'
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
