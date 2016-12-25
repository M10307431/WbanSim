#!/usr/bin/python

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
global adm1
global Task
global CC
global GW2
global GW3
global Result
global taskcur
global timetick
global tasknum
global uti

def outputFile(GW, tasknum, uti, meet, miss, total, ratio):
    filename = 'GW-' + str(GW) +'_Task-' + str(tasknum) +'U-' + str(uti) + '_OFLD.txt'
    output = open(filename, 'w')
    Meet =  'Meet : ' + str(meet)
    Miss = 'Miss : ' + str(miss)
    Total = 'Total : ' + str(total)
    MR = 'Meet Ratio : ' + str(ratio)
    output.writelines(filename)
    output.writelines('\n\n')
    output.writelines(Meet)
    output.writelines('\n')
    output.writelines(Miss)
    output.writelines('\n')
    output.writelines(Total)
    output.writelines('\n')
    output.writelines(MR)
    output.close()

def delay_1ms():
    s = time.time()
    while True:
        f = time.time()
        if f-s >= 0.001:
            break

class  ADM(threading.Thread):
    def run(self):
        global adm1
        global Task
        global CC
        global GW2
        global GW3

        adm1 = socket(AF_INET, SOCK_STREAM)
        adm1.connect((CChost, CCport))
        print 'ADM connected'
        while True:
            try:
                adm1.send(pickle.dumps(Task))
                cc = adm1.recv(4096)
                CC = pickle.loads(cc)
                #print "CC"
                adm1.send("GW2")
                gw2 = adm1.recv(4096)
                GW2 = pickle.loads(gw2)
                #print "GW2"
                adm1.send("GW3")
                gw3 = adm1.recv(4096)
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
        global cc1
        global c13
        global Task
        global CC
        global GW2
        global GW3
        global tasknum
        global uti
        GW = 1
        '''
        Task =[{"Exe":1.74,"Period":2,"Deadline":0,'Arrival':-2,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.13,"Period":2,"Deadline":0,'Arrival':-2,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.47,"Period":8,"Deadline":0,'Arrival':-8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"state":0,"GW":1,"VD":0}]
        
        Task =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0}]
        '''
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
                t['OFLDorg'] = -1 if(t['Exe'] > 2*offloadingTrans+t['Exe']/speedratio) else 3
                t['OFLDorg'] = -1 if(2*fogtrans+t['Exe'] > 2*offloadingTrans+t['Exe']/speedratio) else 3

            print "Task",t['id'],' ',t['OFLDorg']

        for t in Task:
            if t['OFLDorg'] == -1:
                t['VD'] = t['Period'] - offloadingTrans     #t['Exe']/speedratio + offloadingTrans
            elif t['OFLDorg'] > 0:
                t['VD'] =  t['Period'] - fogtrans    #t['Exe'] + fogtrans

        #fog = 0
        raw_input('GW1 Start')
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
                        Task[taskCBC['id']-1]['CC'] = taskCBC['CC']
                        print "Timetick=",timetick,"\tTASK",taskCBC['id'],"(",taskCBC['Cnt'],")\tbackCC"
                except:
                    pass
                try:
                    taskCB3 = c13.recv(1024)
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
                try:
                    taskCB2 = self.request.recv(1024)
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
                            print "Timetick=",timetick,"\tTASK",taskCB2['id'],"(GW",taskCB2['GW'],")\tfromGW2"
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
                                resp2 = 0
                                resp3 = 0
                                FMW2 = -999
                                FMW3 = -999
                                if len(GW2):
                                    for g in GW2:
                                        if g['Deadline'] < d['VD'] + GW2[0]['time']:
                                            resp2 += g['Remain']
                                    if (resp2+d['Exe']) <= d['VD'] and d['Exe']/d['VD'] < (1.0-GW2[0]['uti']):
                                        FMW2 = m*GW2[0]['batt']-(1-m)*GW2[0]['uti']
                                if len(GW3):
                                    for g3 in GW3:
                                        if g3['Deadline'] < d['VD'] + GW3[0]['time']:
                                            resp3 += g3['Remain']
                                    if (resp3+d['Exe']) <= d['VD'] and d['Exe']/d['VD'] < (1.0-GW3[0]['uti']):
                                        FMW3 = m*GW3[0]['batt']-(1-m)*GW3[0]['uti']
                                if FMW2 > FMW3 and FMW2 > -1:
                                    d['OFLD'] = 2
                                    #fog += 1
                                elif FMW3 >-1:
                                    d['OFLD'] = 3
                                    #fog += 1
                                else:
                                    d['OFLD'] = -999
                                                                
                            if(d['OFLD'] == -999):
                                d['Remain'] = d['Exe']
                            d['state'] = 1 if(d['OFLD'] > -999) else 0
                            d['Cnt']+=1
                            Result['Total']+=1
                            #print "Task",d['id'],'(',d['Cnt'],")\tArrival\t",d['OFLD']
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
                    outputFile(GW, tasknum, uti, Result['Meet'], Result['Miss'], Result['Total']-3, float(Result['Meet'])/float(Result['Total']-3))
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
                #time.sleep(0.01)
                
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
                    delay_1ms()
                    #time.sleep(0.01)
                    Task[Task.index(taskcur)]['Remain']-=0.001
                    #taskcur['Remain']-=0.01
                    
                    if taskcur['Remain']<=0:
                        
                        if taskcur['state'] == 1:
                            
                            if taskcur['OFLD']==-1:
                                try:
                                    taskcur['VD'] = taskcur['Deadline'] - timetick - offloadingTrans
                                    data_string = pickle.dumps(taskcur)
                                    cc1.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_C"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==2:
                                try:
                                    taskcur['VD'] = taskcur['Deadline'] - timetick - fogtrans
                                    data_string = pickle.dumps(taskcur)
                                    self.request.send(data_string)
                                    #time.sleep(1)
                                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tOFLD_GW2"
                                except:
                                    print "Send Error!"
                            elif taskcur['OFLD']==3:
                                try:
                                    taskcur['VD'] = taskcur['Deadline'] - timetick - fogtrans
                                    data_string = pickle.dumps(taskcur)
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
    tasknum = 3
    uti = 1.0
    if tasknum == 3:
        Task =[{"Exe":0.029,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.262,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0}]
    elif tasknum == 4:
        Task =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0}]
    elif tasknum == 5:
        Task =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0}]
    elif tasknum == 6:
        Task =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0}]
    elif tasknum == 8:
        Task =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0}]
    elif tasknum == 10:
        Task =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0},
                    {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0,"batt":1.0,"uti":0,"time":0}]
    else:
        raw_input('Task setting Error!')
    
    CC = []
    GW2 = []
    GW3 = []
    CChost='140.118.172.123'
    CCport=12345
    Lhost='192.168.0.104'
    Lport=33333
    HOST, PORT = "", 11111
    threadadm = ADM()
    threadadm.start()
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
