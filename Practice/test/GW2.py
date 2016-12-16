from socket import socket, AF_INET, SOCK_STREAM
import socket as sk
import msvcrt as m
import time,datetime
import sys
import pickle
from select import select
import threading
from threading import Thread
from time import clock

timeout = 3

def delay_1ms():
    n = 1
    for i in range(5):
        for j in range(998):
            n += n+i

def LocalEDF():
    idle = 1
    hyperperiod = 40
    timetick = 0
    #OFLD: -1 Local
    #       0 Cloud
    #       * Fog
    Task =[{"Exe":0.60,"Period":1,"Deadline":0,'Arrival':-1,"Remain":0,'id':1,"Cnt":0,"OFLD":-1},
           {"Exe":0.69,"Period":2,"Deadline":0,'Arrival':-2,"Remain":0,'id':2,"Cnt":0,"OFLD":-1},
           {"Exe":0.19,"Period":4,"Deadline":0,'Arrival':-4,"Remain":0,'id':3,"Cnt":0,"OFLD":-1}]
    Result ={"Meet":0,"Miss":0,"Total":0}
    
    print datetime.datetime.now().strftime("%H:%M:%S")," Start"
    taskcur = None
    try:
        while True:
            sched_new = None

            #=== task arrival ===
            for d in Task:
                if d['Deadline']<=timetick:
                    if d['Remain']>0:
                        Result['Miss']+=1
                        print "Timetick=",timetick,"\tTASK",d['id'],"\tMISS"
                    d['Arrival'] = d['Arrival']+d['Period']
                    d['Deadline'] = d['Deadline']+d['Period']
                    d['Remain'] = d['Exe']
                    d['Cnt']+=1
                    Result['Total']+=1

            #=== sched new ===
            for d in Task:
                if sched_new==None:
                    if timetick>=d['Arrival'] and d['Remain']>0:
                        sched_new = d
                else:
                    if sched_new['Deadline']>d['Deadline'] and timetick>=sched_new['Arrival'] and d['Remain']>0:
                        sched_new = d

            #=== preempt ===
            if sched_new!=None and taskcur==None:
                taskcur = sched_new
                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tRUN"
            elif sched_new!=None and sched_new['Deadline']<taskcur['Deadline']:
                taskcur = sched_new
                print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tPREEMPT"

            #=== exec ===
            #time.sleep(0.01)
            delay_1ms()
            timetick += 0.01
            if taskcur!=None:
                idle = 1
                Task[taskcur['id']-1]['Remain']-=0.01
                #print Task[taskcur['id']]['Remain']
                if Task[taskcur['id']-1]['Remain']<=0:
                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                    Result['Meet']+=1
                    taskcur = None
            else:
                if idle ==1:
                    idle = 0
                    print "Timetick=",timetick,"\tIDLE"
            
            if timetick>=hyperperiod:
                print "Meet:\t",Result['Meet']
                print "Miss:\t",Result['Miss']
                print "Total:\t",Result['Total']
                print "Meet%:\t",Result['Meet']/Result['Total']
                return
    except:
        print "ERROR"
        s.shutdown()
        s.close()
        return

def myOFLD():
    s=socket(AF_INET, SOCK_STREAM)
    s.connect((host, port))
    s.settimeout(0.0)
    print "Connected"

    idle = 1
    hyperperiod = 40
    timetick = 0
    p_idle = 1.55
    p_comp = 2.9-1.55
    p_trans = 0.3
    offloadingTrans = 0.25
    #OFLD: -1 Local     #state: 1 pre-proc
    #       0 Cloud             2 exec
    #       * Fog               3 post-proc
    Task =[{"Exe":0.37,"Period":1,"Deadline":0,'Arrival':-1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"state":0,"GW":2},
           {"Exe":0.67,"Period":2,"Deadline":0,'Arrival':-2,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"state":0,"GW":2},
           {"Exe":1.13,"Period":4,"Deadline":0,'Arrival':-4,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"state":0,"GW":2}]
    Result ={"Meet":0,"Miss":0,"Total":0}

    #===== OFLD =====
    for t in Task:
        engL = (p_idle+p_comp)*t['Exe']
        engR = 2*(p_idle+p_trans+p_comp/2.0)*offloadingTrans
        if(engL > engR):
            t['OFLD'] = -1
        print t['OFLD']

    print datetime.datetime.now().strftime("%H:%M:%S")," Start"
    taskcur = None
    try:
        while True:
            #===== Recv remote task =====
            try:
                backCC = s.recv(1024)
                taskCC = pickle.loads(backCC)
                Task[taskCC['id']-1]['Remain'] = offloadingTrans
                Task[taskCC['id']-1]['state'] = 3
                print "Timetick=",timetick,"\tTASK",taskCC['id'],"(",taskCC['Cnt'],")\tbackOFLD"
            except:
                pass
            
            sched_new = None
            #===== task arrival =====
            for d in Task:
                if d['Deadline']<=timetick:
                    if (d['Remain']>0) or (d['state']==1):
                        Result['Miss']+=1
                        print "Timetick=",timetick,"\tTASK",d['id'],"(",d['Cnt'],")\tMISS"
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
                if sched_new['Deadline']==taskcur['Deadline'] and sched_new['OFLD']>taskcur['OFLD']:
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
                            s.send(data_string)
                            time.sleep(1)
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
                s.shutdown()
                s.close()
                return
    except:
        print "ERROR"
        s.close()
        return



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
    host='localhost'
    port=8888
    #main()
    start = time.time()
    #delay_1ms()
    #LocalEDF()
    myOFLD()
    finish = time.time()
    print (finish-start)
