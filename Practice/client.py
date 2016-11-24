from socket import socket, AF_INET, SOCK_STREAM
import socket as sk
import msvcrt as m
import time,datetime
import sys
from select import select
import threading
from threading import Thread
from time import clock

timeout = 3

def EDF():
    hyperperiod = 4
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
            time.sleep(0.01)
            timetick += 0.01
            if taskcur!=None:            
                Task[taskcur['id']-1]['Remain']-=0.01
                #print Task[taskcur['id']]['Remain']
                if Task[taskcur['id']-1]['Remain']<=0:
                    print "Timetick=",timetick,"\tTASK",taskcur['id'],"(",taskcur['Cnt'],")\tFINISH"
                    Result['Meet']+=1
                    taskcur = None
            else:
                print "Timetick=",timetick,"\tIDLE"
            
            if timetick>=hyperperiod:
                print "Meet:\t",Result['Meet']
                print "Miss:\t",Result['Miss']
                print "Total:\t",Result['Total']
                print "Meet%:\t",Result['Meet']/Result['Total']
                return
    except:
        print "ERROR"
        return

def main():
    s=socket(AF_INET, SOCK_STREAM)
    s.connect((host, port))
    while True:
        s.settimeout(0.5)
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

def delay_1s():
    n = 1
    for i in range(53):
        for j in range(994):
            n += n+i
        
if __name__=="__main__":
    host='140.118.206.169'
    port=12345
    #main()
    #EDF()
    start = time.time()
    delay_1s()
    finish = time.time()
    print (finish-start)
