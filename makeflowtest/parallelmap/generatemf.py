
#generate series of coupling task pairs based on chanin.mf

#pairnum=3

import sys

if len(sys.argv)!=2:
    print './generate.py <pairnum>'
    sys.exit(0)  

pairnum=int(sys.argv[1])


originalstr='''

sim.out: ../app/simulate.sh
    /bin/bash ../app/simulate.sh --timesteps 1 --range 100 --nvalues 5 --log off > sim.out

ana.out: sim.out ../app/analysis.sh
    /bin/bash ../app/analysis.sh sim.out > ana.out

'''




file = open("parallelmap.mf","w")

for x in range(1,pairnum+1): 
    simout='sim'+str(x)+'.out'
    anaout='ana'+str(x)+'.out'
    #replace the mf str and write into new file
    #print simout
    ndata=originalstr.replace('sim.out',simout)
    ndata=ndata.replace('ana.out',anaout)
    print ndata
    file.write(ndata)


file.close()
