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
        if f-s >= 0.001:
            break

class  ADM(threading.Thread):
    def run(self):
        global adm2
        global Task
        global CC
        global GW1
        global GW3
        adm3 = socket(AF_INET, SOCK_STREAM)
        adm3.connect((CChost, CCport))
        print 'ADM connected'
        while True:
            try:
                adm2.send(pickle.dumps(Task))
                CC = adm2.recv(4096)
                CC = pickle.loads(CC)
                #print "CC"
                adm2.send("GW1")
                GW1 = adm2.recv(4096)
                GW1 = pickle.loads(GW1)
                #print "GW1"
                adm3.send("GW3")
                GW3 = adm2.recv(4096)
                GW3 = pickle.loads(GW3)
                #print "GW2"
                time.sleep(0.001)
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
        global CC
        global GW1
        global GW3
        GW = 2
        '''
        Task =[{"Exe":0.048,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"state":0,"GW":2},
                {"Exe":0.027,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"state":0,"GW":2},
                {"Exe":0.094,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"state":0,"GW":2}]
        '''
        Task =[{"Exe":0.088,"Period":0.2,"Deadline":0,'Arrival':-0.2,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                {"Exe":0.036,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0},
                {"Exe":0.159,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":2,"VD":0,"batt":1.0,"uti":0,"time":0}]
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
        hyperperiod = 8
        timetick = 0

        #print datetime.datetime.now().strftime("%H:%M:%S")," Start"
        taskCBC = ''
        taskCB1 = ''
        taskCB3 = ''

        raw_input('GW2 OFLD')
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
        #===== OFLD =====
        print 'Offloading Decision'
        for t in Task:
            engL = (p_idle+p_comp)*t['Exe']
            engR = 2*(p_idle+p_trans+p_comp/2.0)*offloadingTrans
            if(engL > engR):
                t['OFLDorg'] = -1 if(t['Exe'] > 2*offloadingTrans+t['Exe']/speedratio) else 3
                t['OFLDorg'] = -1 if(2*fogtrans+t['Exe'] > 2*offloadingTrans+t['Exe']/speedratio) else 3

            #t['OFLDorg'] = -999
            #t['OFLDorg'] = -1
            print "Task",t['id'],' ',t['OFLDorg']
        
        for t in Task:
            if t['OFLDorg'] == -1:
                t['VD'] = t['Period'] - offloadingTrans     #t['Exe']/speedratio + offloadingTrans
            elif t['OFLDorg'] > 0:
                t['VD'] = t['Period'] - fogtrans    #t['Exe'] + fogtrans
                
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
                            taskCB1['Deadline'] = timetick + taskCB1['VD']-fogtrans
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
                            taskCB3['Deadline'] = timetick + taskCB3['VD']-fogtrans
                            taskCB3['Arrival'] = 0
                            Task.append(taskCB3)
                            print "Timetick=",timetick,"\tTASK",taskCB3['id'],"(GW",taskCB3['GW'],")\tfromGW3"
                except:
                    pass
                
                sched_new = None
                #===== task arrival =====
                for d in Task:
                    if d['Deadline']<=timetick:
                        if d['GW'] == GW:
                            if (d['Remain']>0 or d['state']==1):
                                Result['Miss']+=1
                                print "Timetick=",timetick,"\tTASK",d['id'],"(",d['Cnt'],")\t*****MISS*****"
                            d['Arrival'] = d['Arrival']+d['Period']
                            d['Deadline'] = d['Deadline']+d['Period']
                            d['OFLD'] = d['OFLDorg']
                            
                            if(d['OFLD'] == -1):
                                d['Remain'] = offloadingTrans
                                d['VD'] = d['Period'] - offloadingTrans
                                resp = 0
                                for c in CC:
                                    if c['Deadline'] < d['VD'] + CC[0]['time']:
                                        resp += c['Remain']/speedratio
                                if (resp+d['Exe']/speedratio) > d['VD']:
                                    d['OFLD'] = -999
                                   
                            if(d['OFLD'] > 0):
                                d['Remain'] = fogtrans
                                d['VD'] = d['Period'] - fogtrans
                                resp1 = 0
                                resp3 = 0
                                FMW1 = 0
                                FMW3 = 0
                                for g1 in GW1:
                                    if g1['Deadline'] < d['VD'] + GW1[0]['time']:
                                        resp1 += g1['Remain']
                                for g3 in GW3:
                                    if g3['Deadline'] < d['VD'] + GW3[0]['time']:
                                        resp3 += g3['Remain']
                                if (resp1+d['Exe']) <= d['VD'] and d['Exe']/d['VD'] < (1.0-GW1[0]['uti']):
                                    FMW1 = m*GW1[0]['batt']-(1-m)*GW1[0]['uti']
                                if (resp3+d['Exe']) <= d['VD'] and d['Exe']/d['VD'] < (1.0-GW3[0]['uti']):
                                    FMW3 = m*GW3[0]['batt']-(1-m)*GW3[0]['uti']
                                if FMW1 > FMW3 and FMW1 > 0:
                                    d['OFLD'] = 1
                                elif FMW3 > FMW1 and FMW3 >0:
                                    d['OFLD'] = 3
                                else:
                                    d['OFLD'] = -999
                                                            
                            if(d['OFLD'] == -999):
                                d['Remain'] = d['Exe']
                            d['state'] = 1 if(d['OFLD'] > -999) else 0
                            d['Cnt']+=1
                            Result['Total']+=1
                        else:
                            if d['Remain']>0:
                                Task.remove(d)
                                print "Timetick=",timetick,"\tTASK",d['id'],"(GW",d['GW'],")\t*****MISS*****"

                #===== Result =====
                if timetick>=hyperperiod:
                    print "Meet:\t",Result['Meet']
                    print "Miss:\t",Result['Miss']
                    print "Total:\t",Result['Total']-3
                    print "Meet%:\t",float(Result['Meet'])/float(Result['Total']-3)
                    break
                curU = 0.0
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
                
                timetick += 0.001
                Task[0]['time'] = timetick
                if taskcur!=None and taskcur['OFLD']==-999:
                    idle = 1
                    delay_1ms()
                    Task[Task.index(taskcur)]['Remain']-=0.01
                    #taskcur['Remain']-=0.01

                    if Task[Task.index(taskcur)]['Remain']<=0:
                        print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                        Result['Meet']+=1
                        taskcur = None
                elif taskcur!=None and taskcur['OFLD']>-999:
                    idle = 1
                    delay_1ms()
                    Task[Task.index(taskcur)]['Remain']-=0.01
                    #taskcur['Remain']-=0.01
                    
                    if taskcur['Remain']<=0:
                        
                        if taskcur['state'] == 1:
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
                    time.sleep(0.001)
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
    Task = []
    CC = []
    GW1 = []
    GW3 = []
    CChost='140.118.206.169'
    CCport=12345
    Lhost='140.118.206.169'
    Lport=11111
    HOST, PORT = "", 22222
    threadadm = ADM()
    threadadm.start()
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
