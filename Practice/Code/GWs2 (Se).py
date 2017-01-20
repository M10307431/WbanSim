#!usr/bin/python
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
global adm2
global Task
global CC
global GW1
global GW3
global Result
global taskcur
global timetick

def delay_1ms():
    s = time.time()
    while True:
        f = time.time()
        if f-s >= 0.01:
            break

class  ADM(threading.Thread):
    def run(self):
        global adm2
        global Task
        global CC
        global GW1
        global GW3
        adm2 = socket(AF_INET, SOCK_STREAM)
        adm2.connect((CChost, CCport))
        print 'ADM connected'
        while True:
            try:
                adm2.send(pickle.dumps(Task))
                cc = adm2.recv(4096)
                CC = pickle.loads(cc)
                #print "CC"
                adm2.send("GW1")
                gw1 = adm2.recv(4096)
                GW1 = pickle.loads(gw1)
                #print "GW1"
                adm2.send("GW3")
                gw3 = adm2.recv(4096)
                GW3 = pickle.loads(gw3)
                #print "GW3"
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
        global cc2
        global c21
        global Task
        global CC
        global GW1
        global GW3
        GW = 2
        
        Result ={"Meet":0,"Miss":0,"Total":0}
        info = {"batt":100,"uti":0}

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

        #---------------------------------------
        #OFLD: -1 Local     #state: 1 pre-proc
        #       0 Cloud             2 exec
        #       * Fog               3 post-proc
        #---------------------------------------
        p_idle = 1.8
        p_comp = 4.4-1.8
        p_trans = 0.5
        offloadingTrans = 0.025
        fogtrans = 0.005
        proc = 0.005
        speedratio = 5
        m = 0.5
        #===== OFLD =====
        print 'Offloading Decision'
        for t in Task:
            engL = (p_idle+p_comp)*t['Exe']
            engR = 2*((p_idle+p_trans)*(offloadingTrans-proc) + (p_idle+p_comp)*proc)
            if(engL > engR):
                t['OFLDorg'] = -1 if(t['Exe'] > 0.1) else -999
            print "Task",t['id'],' ',t['OFLDorg']   

        raw_input('GW2 Start')

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
                        Task[taskCBC['id']-1]['CC'] = taskCBC['CC']
                        print "Timetick=",timetick,"\tTASK",taskCBC['id'],"(",taskCBC['Cnt'],")\tbackCC"
                except:
                    pass
                try:
                    taskCB1 = c21.recv(1024)
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
                            print "Timetick=",timetick,"\tTASK",taskCB1['id'],"(GW",taskCB1['GW'],")\tfromGW1"
                except:
                    pass
                try:
                    taskCB3 = self.request.recv(1024)
                    if taskCB3:
                        taskCB3 = pickle.loads(taskCB3)
                        if taskCB3['GW']==GW:
                            Task[taskCB3['id']-1]['Remain'] = fogtrans
                            Task[taskCB3['id']-1]['state'] = 3
                            Task[taskCB3['id']-1]['GW3'] = taskCB3['GW3']
                            print "Timetick=",timetick,"\tTASK",taskCB3['id'],"(",taskCB3['Cnt'],")\tbackGW3"
                        else:
                            taskCB3['Remain'] = taskCB3['Exe']
                            taskCB3['state'] = 2
                            taskCB3['Deadline'] = timetick + taskCB3['VD']
                            taskCB3['Arrival'] = 0
                            Task.append(taskCB3)
                            print "Timetick=",timetick,"\tTASK",taskCB3['id'],"(GW",taskCB3['GW'],")\tfromGW3"
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

                #===== Result =====
                if timetick>=hyperperiod:
                    print "Meet:\t",Result['Meet']
                    print "Miss:\t",Result['Miss']
                    print "Total:\t",Result['Total']-5
                    print "Meet%:\t",float(Result['Meet'])/float(Result['Total']-5)
                    break
                curU = 0.0
                #===== sched new =====
                if taskcur==None:
                    for d in Task:
                        if sched_new==None:
                            if timetick>=d['Arrival'] and d['Remain']>0:
                                sched_new = d
                        else:
                            if sched_new['Arrival']>=d['Arrival'] and timetick>=d['Arrival'] and d['Remain']>0:
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
                
                timetick += 0.001
                Task[0]['time'] = timetick
                if taskcur!=None and taskcur['OFLD']==-999:
                    idle = 1
                    delay_1ms()
                    Task[Task.index(taskcur)]['Remain']-=0.001
                    #taskcur['Remain']-=0.01

                    if Task[Task.index(taskcur)]['Remain']<=0:
                        print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                        Result['Meet']+=1
                        taskcur = None
                elif taskcur!=None and taskcur['OFLD']>-999:
                    idle = 1
                    if taskcur['Remain'] >= offloadingTrans-proc and taskcur['state']==1:
                        delay_1ms()
                    elif taskcur['Remain'] <= proc and taskcur['state']==3:
                        delay_1ms()
                    else:
                        time.sleep(0.01)
                    Task[Task.index(taskcur)]['Remain']-=0.001
                    #taskcur['Remain']-=0.01
                    
                    if taskcur['Remain']<=0:
                        
                        if taskcur['state'] == 1:
                            taskcur['VD'] = taskcur['Deadline'] - timetick
                            data_string = pickle.dumps(taskcur)
                            if taskcur['OFLD']==-1:
                                try:
                                    cc2.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_C"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==3:
                                try:
                                    self.request.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_GW3"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==1:
                                try:
                                    c21.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_GW1"
                                except:
                                    print "Send Error!"
                            taskcur = None

                        elif taskcur['state'] == 2:
                            data_string = pickle.dumps(taskcur)
                            if taskcur['GW']==3:
                                self.request.send(data_string)
                                Task.remove(taskcur)
                                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_GW3"
                            elif taskcur['GW']==1:
                                c21.send(data_string)
                                Task.remove(taskcur)
                                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_GW1"
                            taskcur = None
                            
                        elif taskcur['state'] == 3:
                            print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH_C"
                            Result['Meet']+=1
                            taskcur = None
                else:
                    time.sleep(0.01)
                    if idle ==1:
                        idle = 0
                        print "Timetick=",timetick,"\tIDLE"

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
            if adm2:
                adm2.shutdown()
                adm2.close()

    def finish(self):
        pass

    def setup(self):
        pass

if __name__=="__main__":
    tasknum = 5
    uti = 1.0
    if tasknum == 3 and uti ==0.5:
        Task =[{"Exe":0.147,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.029,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.045,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0}]
    elif tasknum == 5 and uti ==0.5:
        Task =[{"Exe":0.023,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.017,"Period":0.2,"Deadline":0,'Arrival':-0.2,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.024,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.024,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':4,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.044,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':5,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0}]
    
    if tasknum == 3 and uti ==1.0:
        Task =[{"Exe":0.030,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.043,"Period":0.2,"Deadline":0,'Arrival':-0.2,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.387,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0}]
    elif tasknum == 5 and uti ==1.0:
        Task =[{"Exe":0.136,"Period":0.2,"Deadline":0,'Arrival':-0.2,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.011,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.037,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.025,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':4,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.043,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':5,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0}]

    CC = []
    GW1 = []
    GW3 = []
    CChost='140.118.206.169'
    CCport=12345
    Lhost='192.168.0.102'
    Lport=11111
    HOST, PORT = "", 22222
    #threadadm = ADM()
    #threadadm.start()
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
