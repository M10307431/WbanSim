from socket import socket, AF_INET, SOCK_STREAM
import SocketServer
import time,datetime
import sys
import pickle
import threading
from threading import Thread
from time import clock

timeout = 3

global cc1
global c13
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
        global cc1
        global c13
        
        Task =[{"Exe":1.74,"Period":2,"Deadline":0,'Arrival':-2,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.13,"Period":2,"Deadline":0,'Arrival':-2,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.47,"Period":8,"Deadline":0,'Arrival':-8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"state":0,"GW":1,"VD":0}]
        Result ={"Meet":0,"Miss":0,"Total":0}

        self.request.settimeout(0.0)
        raw_input('GW1 Run')
        cc1 = socket(AF_INET, SOCK_STREAM)
        c13 = socket(AF_INET, SOCK_STREAM)
        cc1.connect((CChost, CCport))
        print 'Cloud connected'
        c13.connect((Lhost, Lport))
        print 'Local connected'
        cc1.settimeout(0.0)
        c13.settimeout(0.0)
        
        idle = 1
        hyperperiod = 40
        timetick = 0

        #print datetime.datetime.now().strftime("%H:%M:%S")," Start"
        taskCBC = ''
        taskCB3 = ''
        taskCB2 = ''

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
        speedratio = 5
        #===== OFLD =====
        print 'Offloading Decision'
        for t in Task:
            engL = (p_idle+p_comp)*t['Exe']
            engR = 2*(p_idle+p_trans+p_comp/2.0)*offloadingTrans
            if(engL > engR):
                t['OFLD'] = -1 if(t['Exe'] > 2*offloadingTrans+t['Exe']/speedratio) else 3
                t['OFLD'] = -1 if(2*fogtrans+t['Exe'] > 2*offloadingTrans+t['Exe']/speedratio) else 3
                #print 2*fogtrans+t['Exe'],'\t',2*offloadingTrans+t['Exe']/speedratio
            print "Task",t['id'],' ',t['OFLD']

        for t in Task:
            if t['OFLD'] == -1:
                t['VD'] = t['Period'] - offloadingTrans
            elif t['OFLD'] > 0:
                t['VD'] = t['Period'] - fogtrans
        
        taskcur = None
        try:
            
            while True:
            
                #===== Recv remote task =====
                try:
                    taskCBC = cc1.recv(1024)
                    if taskCBC:
                        taskCBC = pickle.loads(taskCBC)
                        print "Timetick=",timetick,"\tTASK",taskCBC['id'],"(",taskCBC['Cnt'],")\tbackCC"
                        Task[taskCBC['id']-1]['Remain'] = offloadingTrans
                        Task[taskCBC['id']-1]['state'] = 3
                        print "Timetick=",timetick,"\tTASK",taskCBC['id'],"(",taskCBC['Cnt'],")\tbackCC"
                except:
                    pass
                try:
                    taskCB3 = c13.recv(1024)
                    if taskCB3:
                        taskCB3 = pickle.loads(taskCB3)
                        if taskCB3['GW']==1:
                            Task[taskCB3['id']-1]['Remain'] = fogtrans
                            Task[taskCB3['id']-1]['state'] = 3
                            print "Timetick=",timetick,"\tTASK",taskCB3['id'],"(",taskCB3['Cnt'],")\tbackGW3"
                        else:
                            taskCB3['Remain'] = taskCB3['Exe']
                            taskCB3['state'] = 2
                            taskCB3['Deadline'] = timetick + taskCB3['VD']
                            Task.append(taskCB3)
                            print "Timetick=",timetick,"\tTASK",taskCB3['id'],"(GW",taskCB3['GW'],")\tfromGW3"
                except:
                    pass
                try:
                    taskCB2 = self.request.recv(1024)
                    if taskCB2:
                        taskCB2 = pickle.loads(taskCB2)
                        if taskCB2['GW']==1:
                            Task[taskCB2['id']-1]['Remain'] = fogtrans
                            Task[taskCB2['id']-1]['state'] = 3
                            print "Timetick=",timetick,"\tTASK",taskCB2['id'],"(",taskCB2['Cnt'],")\tbackGW2"
                        else:
                            taskCB2['Remain'] = taskCB2['Exe']
                            taskCB2['state'] = 2
                            taskCB2['Deadline'] = timetick + taskCB2['VD']
                            Task.append(taskCB2)
                            print "Timetick=",timetick,"\tTASK",taskCB2['id'],"(GW",taskCB2['GW'],")\tfromGW2"
                except:
                    pass

                sched_new = None
                #===== task arrival =====
                for d in Task:
                    if d['Deadline']<=timetick:
                        if d['GW'] == 1:
                            if (d['Remain']>0 or d['state']==1):
                                Result['Miss']+=1
                                print "Timetick=",timetick,"\tTASK",d['id'],"(",d['Cnt'],")\t*****MISS*****"
                            d['Arrival'] = d['Arrival']+d['Period']
                            d['Deadline'] = d['Deadline']+d['Period']
                            if(d['OFLD'] == -1):
                                d['Remain'] = offloadingTrans
                            elif(d['OFLD'] > 0):
                                d['Remain'] = fogtrans
                            else:
                                d['Remain'] = d['Exe']
                            d['state'] = 1 if(d['OFLD'] > -999) else 0
                            d['Cnt']+=1
                            Result['Total']+=1
                        else:
                            if d['Remain']>0:
                                Task.remove(d)
                                print "Timetick=",timetick,"\tTASK",d['id'],"(GW",d['GW'],")\t*****MISS*****"

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
                    Task[Task.index(taskcur)]['Remain']-=0.01
                    taskcur['Remain']-=0.01

                    if Task[Task.index(taskcur)]['Remain']<=0:
                        print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                        Result['Meet']+=1
                        taskcur = None
                elif taskcur!=None and taskcur['OFLD']>-999:
                    idle = 1
                    Task[Task.index(taskcur)]['Remain']-=0.01
                    taskcur['Remain']-=0.01
                    
                    if taskcur['Remain']<=0:
                        
                        if taskcur['state'] == 1:
                            data_string = pickle.dumps(taskcur)
                            if taskcur['OFLD']==-1:
                                try:
                                    cc1.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_C"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==2:
                                try:
                                    self.request.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_GW2"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==3:
                                try:
                                    c13.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_GW3"
                                except:
                                    print "Send Error!"
                            taskcur = None

                        elif taskcur['state'] == 2:
                            data_string = pickle.dumps(taskcur)
                            if taskcur['GW']==2:
                                self.request.send(data_string)
                                Task.remove(taskcur)
                                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_GW2"
                            elif taskcur['GW']==3:
                                c13.send(data_string)
                                Task.remove(taskcur)
                                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_GW3"
                            taskcur = None
                            
                        elif taskcur['state'] == 3:
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
            if cc1:
                cc1.shutdown()
                cc1.close()
            if c13:
                c13.shutdown()
                c13.close()

    def finish(self):
        pass

    def setup(self):
        pass

if __name__=="__main__":
    CChost='140.118.206.169'
    CCport=12345
    Lhost='140.118.206.169'
    Lport=33333
    HOST, PORT = "", 11111
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    try:
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
