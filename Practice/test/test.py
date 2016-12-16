import SocketServer
import time
from threading import Thread
import pickle

Task =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0}]
Task1 =[{"Exe":0.045,"Period":0.1,"Deadline":0,'Arrival':-0.1,"Remain":0,'id':1,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.198,"Period":0.4,"Deadline":0,'Arrival':-0.4,"Remain":0,'id':2,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0},
                {"Exe":0.040,"Period":0.8,"Deadline":0,'Arrival':-0.8,"Remain":0,'id':3,"Cnt":0,"OFLD":-999,"OFLDorg":-999,"state":0,"GW":1,"VD":0}]
T = []
for d in Task:
    taskcur = d
taskcur = Task
print type(taskcur)
taskcur = pickle.dumps(taskcur)
print type(taskcur)
print len(taskcur)
T = pickle.loads(taskcur)
#print len(taskcur)
print T[0]['Exe']
if isinstance(taskcur, list):
    for t in taskcur:
       print t['OFLD']

'''
import thread
import time

# 为线程定义一个函数
def print_time( threadName, delay):
   count = 0
   while count < 5:
      time.sleep(delay)
      count += 1
      print "%s: %s" % ( threadName, time.ctime(time.time()) )

# 创建两个线程
try:
   thread.start_new_thread( print_time, ("Thread-1", 2, ) )
   thread.start_new_thread( print_time, ("Thread-2", 4, ) )
   print "***************"
except:
   print "Error: unable to start thread"

while 1:
   pass'''
