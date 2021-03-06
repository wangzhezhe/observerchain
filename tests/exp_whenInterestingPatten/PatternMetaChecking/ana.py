# check the staging service

# if check the data 

# if the data become meaningless

# write info to meta server


from mpi4py import MPI
import numpy as np
import dataspaces.dataspaceClient as dataspaces
import ctypes
import os
import time
import math
import timeit
import sys
from threading import Thread
import os

sys.path.append('../../../src/publishclient/pythonclient')
import pubsub as pubsubclient


sys.path.append('../../../src/metadatamanagement/pythonclient')
import metaclient


# input the coordinate of the points and return the index of grid in array
comm = MPI.COMM_WORLD
rank = comm.Get_rank()


def getIndex(px, py, pz):
    # TODO should add all boundry case
    # only for lower case
    r = 15
    gridnum = 15
    deltar = 1.0*r/gridnum

    if (px < 0 or py < 0 or pz < 0 or px > gridnum*deltar or py > gridnum*deltar or pz > gridnum*deltar):
        #print "out of the box "
        #print [px,py,pz]
        return -1

    gnumx = math.floor((px-0)/deltar)
    gnumy = math.floor((py-0)/deltar)
    gnumz = math.floor((pz-0)/deltar)

    index = int(gnumz*gridnum*gridnum + gnumy*gridnum+gnumx)

    return index

def checkAndPublishEvent(gridDataArray_p1, gridDataArray_p2):
    ifTargetEventHappen = True
    massOriginInterest = [6, 0, 6]
    targetValue = 7.5
    massR = 4
    # put the analysis into the simulation part
    for i in range(massOriginInterest[0], massOriginInterest[0]+massR):
        for j in range(massOriginInterest[1], massOriginInterest[1]+massR):
            for k in range(massOriginInterest[2], massOriginInterest[2]+massR):
                #print "index i j k (%d %d %d)" % (i,j,k)
                #print  nparray[i][j][k]
                #print "index i j k (%d %d %d)" % (i,j,k)
                #print nparray[i][j][k]
                index = getIndex(i, j, k)
                if (gridDataArray_p1[index] != targetValue):
                    ifTargetEventHappen = False
                    break

    if (ifTargetEventHappen == True):
        print (iteration)
        # send publish event
        detecttime = timeit.default_timer()
        print (detecttime)
        print ("publish to pub/sub broker")
        #sendEventToPubSub(iteration)
        ifFirstHappen = True
    return

initp =  1.5
targetValue = 7.5

def checkDataPattern(gridDataArray_p1, gridDataArray_p2):

    coord1 = []
    coord2 = []
    # get the index of red block in data 1
    # print("caculate coord1")
    break_flag=False
    for x in range(15):
        if(break_flag==True):
            break
        for y in range (15):
            if(break_flag==True):
                break
            for z in range (15):
                index = getIndex(x,y,z)
                if (gridDataArray_p1[index]==targetValue):
                    coord1 = [x,y,z]
                    break_flag=True
                    #print(coord1)
                    break


    # get the index of the red block in data 2
    #print("caculate coord2")
    break_flag=False
    for x in range(15):
        if(break_flag==True):
            break
        for y in range (15):
            if(break_flag==True):
                break
            for z in range (15):
                index = getIndex(x,y,z)
                if (gridDataArray_p2[index]==targetValue):
                    coord2 = [x,y,z]
                    break_flag=True
                    #print(coord2)
                    break
    
    distance = pow((coord2[0]-coord1[0]),2)+pow((coord2[1]-coord1[1]),2)+pow((coord2[2]-coord1[2]),2)
    #print(distance)
    if(distance>140 and distance<150):
        return True
    else:
        return False

def checkDataPatternCenter(gridDataArray_p1):
    massOriginInterest = [7, 7, 7]
    targetValue = 7.5

    index = getIndex(massOriginInterest[0], massOriginInterest[1], massOriginInterest[2])
    if (gridDataArray_p1[index] == targetValue):
        return True
    else:
        return False



# copy all conf.* file to current dir
serverdir = "/home1/zw241/dataspaces/tests/C"

confpath = serverdir+"/conf*"

copyCommand = "cp "+confpath+" ."

os.system(copyCommand)

# number of clients at clients end to join server
num_peers = 1
appid = 2

var_name = "ex1_sample_data"
lock_name = "my_test_lock"

if(len(sys.argv)!=2):
    print("./analytics <iteration>")
    exit(0)
    
iteration = int(sys.argv[1])

startanay = timeit.default_timer()

ds = dataspaces.dataspaceClient(appid,comm)

currIter = 0

lb = [15*15*15*rank]
ub = [15*15*15*(rank+1)-1]




def threadFunction():

    # check the meta periodically
    addrList =metaclient.getServerAddr()
    addr = addrList[0]

    # if the value is not NULL

    while(1):
        value=metaclient.getMeta(addr, "simend")
        if(value=="NULL"):
            time.sleep(0.1)
            continue
        else:
            break
        
    endsim = timeit.default_timer()
    print("sim end, stop the ana")
    os._exit(0)


thread = Thread(target = threadFunction)
thread.start()




#while (True):
version = 0
while (version<iteration):
#for version in range(iteration):
    # ds.lock_on_read(lock_name)

    # version = currIter



    #print("get version")
    #print(version)
    #use read write lock here
    #ds.lock_on_read(lock_name)
    # use lock type  = 1
    getdata_p1,rcode = ds.get(var_name, version, lb, ub)
    #ds.unlock_on_read(lock_name)
    # check if data ok
    if(rcode == -11):
        print("data not avaliable for ts %d"%(version))
        time.sleep(0.1)
        continue


    #lb = [3380]
    #ub = [3380+3374]

    #print("get version")
    #print(version)

    #getdata_p2 = ds.dspaces_get_data(var_name, version, lb, ub)

    # time.sleep(1)
    # publishe events to pubsub store

    #print("get data1")
    #print (getdata_p1)

    #print("get data2")
    #print (getdata_p2)
    #patternHeppen = checkDataPattern(getdata_p1,getdata_p2)
    patternHeppen = checkDataPatternCenter(getdata_p1)
    #extra data read time is not being counted
    time.sleep(0.01)

    #if(currIter>=iteration):
    #    break
    version=version+1

    if(patternHeppen==True):
        #the time used for data analysis
        #fack calling the analytics here and find the data is meaningless after analysing
        time.sleep(0.05)
        print("---------patternHeppen at ts %d, simulation data is meaningless----------"%(version))
        # write to the meta server (the data is meaningless)
        addrList =metaclient.getServerAddr()
        addr = addrList[0]
        metaclient.putMeta(addr, "meaningless", "meaningless info")
        break


        

ds.finalize()

endanay = timeit.default_timer()

print("time span")
print(endanay-startanay)

addrList =metaclient.getServerAddr()

addr = addrList[0]
print("test get: ", metaclient.getMeta(addr, "testkey"))