from socket import socket, AF_INET, SOCK_STREAM
import SocketServer
import time,datetime
import sys
import pickle
import threading
import thread
from threading import Thread
from time import clock

timeout = 3

global cc3
global c32
global adm3
global Task
global CC
global GW1
global GW2
global Result
global taskcur
global timetick

def delay_1ms():
    s=time.time()
    while True:
        f=time.time()
        if f-s>=0.001:
            break

class  ADM(threading.Thread):
    def run(self):
        global adm3
        global Task
        global CC
        global GW1
        global GW2
        adm3 = socket(AF_INET, SOCK_STREAM)
        adm3.connect((CChost, CCport))
        print 'ADM connected'
        while True:
            try:
                adm3.send(pickle.dumps(Task))
                CC = adm3.recv(4096)
                CC = pickle.loads(CC)
                #print "CC"
                adm3.send("GW1")
                GW1 = adm3.recv(4096)
                GW1 = pickle.loads(GW1)
                #print "GW1"
                adm3.send("GW2")
                GW2 = adm3.recv(4096)
                GW2 = pickle.loads(GW2)
                #print "GW2"
                #time.sleep(0.001)
            except:
                pass

class MyTCPHandler(SocketServer.BaseRequestHandler):
    def __init__(self,request,client_address,server):
        self.request=request
        self.client_address=client_address
        self.server=server

        print "Client",self.client_address[0]
        self.handle()
                
    def handle(self):
        global cc3
        global c32
        global adm3
        global Task
        global CC
        global GW1
        global GW2
        GW = 3

        Task =[{"Exe":0.3,"Period":1,"Deadline":0,'Arrival':-1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":3,"VD":0,"batt":1.0,"uti":0,"time":0}]
        Result ={"Meet":0,"Miss":0,"Total":0}
        info = {"batt":100,"uti":0}

        self.request.settimeout(0.0)
        raw_input('GW3 Run')
        cc3 = socket(AF_INET, SOCK_STREAM)
        cc3.connect((CChost, CCport))
        print 'Cloud connected'
        cc3.settimeout(0.0)
        c32.settimeout(0.0)
        
        idle = 1
        hyperperiod = 40
        timetick = 0

        #print datetime.datetime.now().strftime("%H:%M:%S")," Start"
        taskCBC = ''
        taskCB2 = ''
        taskCB1 = ''

        raw_input('GW3 OFLD')
        #---------------------------------------
        #OFLD: -1 Local     #state: 1 pre-proc
        #       0 Cloud             2 exec
        #       * Fog               3 post-proc
        #---------------------------------------
        p_idle = 1.55
        p_comp = 2.9-1.55
        p_trans = 0.3
        offloadingTrans = 0.025
        fogtrans = 0.005
        speedratio = 5
        '''
        #===== OFLD =====
        print 'Offloading Decision'
        for t in Task:
            engL = (p_idle+p_comp)*t['Exe']
            engR = 2*(p_idle+p_trans+p_comp/2.0)*offloadingTrans
            if(engL > engR):
                t['OFLDorg'] = -1 if(t['Exe'] > 0.1) else -999
            print "Task",t['id'],' ',t['OFLDorg']

        for t in Task:
            if t['OFLD'] = -1:
                t['VD'] = t['Period'] - offloadingTrans
            elif t['OFLD'] > 0:
                t['VD'] = t['Period'] - fogtrans
        '''
        
        taskcur = None
        try:
            
            while True:
            
                #===== Recv remote task =====
                try:
                    taskCBC = cc3.recv(1024)
                    if taskCBC:
                        taskCBC = pickle.loads(taskCBC)
                        Task[taskCBC['id']-1]['Remain'] = offloadingTrans
                        Task[taskCBC['id']-1]['state'] = 3
                        Task[taskCBC['id']-1]['CC'] = taskCBC['CC']
                        print "Timetick=",timetick,"\tTASK",taskCBC['id'],"(",taskCBC['Cnt'],")\tbackCC"
                except:
                    pass
                try:
                    taskCB2 = c32.recv(1024)
                    if taskCB2:
                        taskCB2 = pickle.loads(taskCB2)
                        if taskCB2['GW']==GW:
                            Task[taskCB2['id']-1]['Remain'] = fogtrans
                            Task[taskCB2['id']-1]['state'] = 3
                            Task[taskCB2['id']-1]['GW2'] = taskCB2['GW2']
                            print "Timetick=",timetick,"\tTASK",taskCB2['id'],"(",taskCB2['Cnt'],")\tbackGW2"
                        else:
                            taskCB2['Remain'] = taskCB2['Exe']
                            taskCB2['state'] = 2
                            taskCB2['Deadline'] = timetick + taskCB2['VD']
                            taskCB2['Arrival'] = 0
                            Task.append(taskCB2)
                            print "Timetick=",timetick,"\tTASK",taskCB2['id'],"(",taskCB2['Cnt'],")\tfromGW2"
                except:
                    pass
                try:
                    taskCB1 = self.request.recv(1024)
                    if taskCB1:
                        taskCB1 = pickle.loads(taskCB1)
                        if taskCB1['GW']==GW:
                            Task[taskCB1['id']-1]['Remain'] = fogtrans
                            Task[taskCB1['id']-1]['state'] = 3
                            Task[taskCB1['id']-1]['GW1'] = taskCB1['GW1']
                            print "Timetick=",timetick,"\tTASK",taskCB1['id'],"(",taskCB1['Cnt'],")\tbackGW1"
                        else:
                            taskCB1['Remain'] = taskCB1['Exe']
                            taskCB1['state'] = 2
                            taskCB1['Deadline'] = timetick + taskCB1['VD']
                            taskCB1['Arrival'] = 0
                            Task.append(taskCB1)
                            print "Timetick=",timetick,"\tTASK",taskCB1['id'],"(",taskCB1['Cnt'],")\tfromGW1"
                except:
                    pass

                sched_new = None
                #===== Task arrival =====
                for d in Task:
                    if d['Deadline']<=timetick:
                        if (d['Remain']>0 or d['state']==1):
                            Result['Miss']+=1
                            print "Timetick=",timetick,"\tTASK",d['id'],"(",d['Cnt'],")\t*****MISS*****"
                        d['Arrival'] = d['Arrival']+d['Period']
                        d['Deadline'] = d['Deadline']+d['Period']
                        d['OFLD'] = d['OFLDorg']    
                        d['Remain'] = offloadingTrans if(d['OFLD'] == -1) else d['Exe']
                        d['state'] = 1 if(d['OFLD'] == -1) else 0
                        d['Cnt']+=1
                        Result['Total']+=1
                curU = 0.0
                #===== sched new =====
                if taskcur==None:
                    for d in Task:
                        if sched_new==None:
                            if timetick>=d['Arrival'] and d['Remain']>0:
                                sched_new = d
                        else:
                            if sched_new['Arrival']>d['Arrival'] and timetick>=d['Arrival'] and d['Remain']>0:
                                sched_new = d
                                
                    # current uti
                    if d['GW'] != GW:
                        curU += d['Remain']/d['VD']
                    elif d['OFLD'] == -999:
                        curU += d['Exe']/d['Period']
                    elif d['OFLD'] == -1:
                        curU += 2*offloadingTrans/d['Period']
                    else:
                        curU += 2*fogtrans/d['Period']

                Task[0]['uti'] = curU
                
                #===== Run =====
                if sched_new!=None and taskcur==None:
                    taskcur = sched_new
                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tRUN"                

                #===== exec =====
                delay_1ms()
                timetick += 0.001
                Task[0]['time'] = timetick
                if taskcur!=None and taskcur['OFLD']==-999:
                    idle = 1
                    Task[Task.index(taskcur)]['Remain']-=0.01
                    #taskcur['Remain']-=0.01

                    if Task[Task.index(taskcur)]['Remain']<=0:
                        print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                        Result['Meet']+=1
                        taskcur = None
                elif taskcur!=None and taskcur['OFLD']>-999:
                    idle = 1
                    Task[Task.index(taskcur)]['Remain']-=0.01
                    #taskcur['Remain']-=0.01
                    
                    if taskcur['Remain']<=0:
                        
                        if taskcur['state'] == 1:
                            taskcur['VD'] = timetick - taskcur['Deadline']
                            data_string = pickle.dumps(taskcur)
                            if taskcur['OFLD']==-1:
                                try:
                                    cc3.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_C"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==1:
                                try:
                                    self.request.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_GW1"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==2:
                                try:
                                    c32.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_GW2"
                                except:
                                    print "Send Error!"
                            taskcur = None

                        elif taskcur['state'] == 2:
                            data_string = pickle.dumps(taskcur)
                            if taskcur['GW']==1:
                                self.request.send(data_string)
                                Task.remove(taskcur)
                                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_GW1"
                            elif taskcur['GW']==2:
                                c32.send(data_string)
                                Task.remove(taskcur)
                                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_GW2"
                            taskcur = None
                            
                        elif taskcur['state'] == 3:
                            print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_C"
                            Result['Meet']+=1
                            taskcur = None
                else:
                    if idle ==1:
                        idle = 0
                        print "Timetick=",timetick,"\tIDLE"
                '''
                #===== Result =====
                if timetick>=hyperperiod:
                    print "Meet:\t",Result['Meet']
                    print "Miss:\t",Result['Miss']
                    print "Total:\t",Result['Total']
                    print "Meet%:\t",float(Result['Meet'])/float(Result['Total'])
                    break
                '''            

        except:
            #print "ERROR"
            print "Meet:\t",Result['Meet']
            print "Miss:\t",Result['Miss']
            print "Total:\t",Result['Total']
            print "Meet%:\t",float(Result['Meet'])/float(Result['Total'])
            if cc3:
                cc3.shutdown()
                cc3.close()
            if c32:
                c32.shutdown()
                c32.close()

    def finish(self):
        pass

    def setup(self):
        pass

if __name__=="__main__":
    Task =[{"Exe":0.3,"Period":1,"Deadline":0,'Arrival':-1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":3,"VD":0,"batt":1.0,"uti":0,"time":0}]
    CC = []
    GW1 = []
    GW2 = []
    CChost='140.118.206.169'
    CCport=12345
    Lhost='192.168.0.103'
    Lport=22222
    HOST, PORT = "", 33333
    threadadm = ADM()
    threadadm.start()
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)
    try:
        c32 = socket(AF_INET, SOCK_STREAM)
        c32.connect((Lhost, Lport))
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
