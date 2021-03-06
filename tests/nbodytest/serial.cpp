#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string>
#include <vector>
#include "common.h"

#include "errno.h"
#include "sys/types.h"
#include "limits.h"
#include "unistd.h"
#include "time.h"

#include <dlfcn.h>
#include <vector>
#include <pthread.h>
#include <vector>
#include <string>
#include <algorithm>
#include <uuid/uuid.h>


#include "../../src/utils/getip/getip.h"

#include "../../src/utils/file/loaddata.h"

#include "../../src/observer/eventmanager.h"
#include "../../src/publishclient/pubsubclient.h"

using namespace std;

//label if sth is happen at the first time
int firstSend = 0;

bool detectEvent(vector<particle_t *> particles)
{

    int size = particles.size();
    if (size < 100 && firstSend == 0)
    {
        firstSend = 1;
        return true;
    }
    else
    {
        return false;
    }
}

void publishEventByTopicID(string event, string metainfo)
{
    
    //for testing, use 0
    //for real case in multi cluster, the id should be the part of the event
    int topicid=0;

    string clusterdir = getClusterDirByTopicId(topicid);

    //only init the clients in specific sub cluster partition
    initMultiClientsByClusterDir(clusterdir);

    vector<string> pubeventList;
    pubeventList.push_back(event);
    eventPublish(pubeventList, metainfo);
}

//
//  benchmarking program
//
int main(int argc, char **argv)
{
    int navg, nabsavg = 0;
    double davg, dmin, absmin = 1.0, absavg = 0.0;

    if (find_option(argc, argv, "-h") >= 0)
    {
        printf("Options:\n");
        printf("-h to see this help\n");
        printf("-n <int> to set the number of particles\n");
        printf("-o <filename> to specify the output file name\n");
        printf("-s <filename> to specify a summary file name\n");
        printf("-no turns off all correctness checks and particle output\n");
        return 0;
    }

    int n = read_int(argc, argv, "-n", 1000);

    char *savefolder = read_string(argc, argv, "-o", NULL);
    char *sumname = read_string(argc, argv, "-s", NULL);

    FILE *fsum = sumname ? fopen(sumname, "a") : NULL;

    set_size(n);
    //particle_t *particles = (particle_t *)malloc(n * sizeof(particle_t));
    vector<particle_t*> particles;
    init_particles(n, particles);

    //
    //  simulate a number of time steps
    //
    double simulation_time = read_timer();

    for (int step = 0; step < NSTEPS; step++)
    {
        navg = 0;
        davg = 0.0;
        dmin = 1.0;

        printf("step %d start\n",step);
        //
        //  compute forces
        //
        for (int i = 0; i < n; i++)
        {
            particles[i]->ax = particles[i]->ay = 0;
            for (int j = 0; j < n; j++)
            {
                apply_force(particles[i], particles[j], &dmin, &davg, &navg);
            }
        }

        //
        //  move particles
        //

        

        move(particles);

        //printf("step %d move ok\n",step);

        if (find_option(argc, argv, "-no") == -1)
        {
            //
            // Computing statistical data
            //
            if (navg)
            {
                absavg += davg / navg;
                nabsavg++;
            }
            if (dmin < absmin)
                absmin = dmin;

            //
            //  save if necessary
            //
            char filename[100];
            sprintf(filename, "%s/output_%d.csv", savefolder, step);
            string savename = string(filename);

            if (step % SAVEFREQ == 0)
            {
                FILE *fsave = fopen(savename.data(), "w");
                fprintf(fsave, "x coord, y coord, z coord\n");
                save(fsave, particles);
                fclose(fsave);
            }
            //printf("step %d save ok\n",step);

            //if (detectEvent(particles) == true)
            //{
                //publish event
            //    printf("publish event step %d\n",step);
            //    string meta = "step"+to_string(step);
            //    publishEventByTopicID("eventtest",meta);
            //}
        }
        //printf("timestep finish\n");
    }

    simulation_time = read_timer() - simulation_time;

    printf("n = %d, simulation time = %g seconds", n, simulation_time);

    if (find_option(argc, argv, "-no") == -1)
    {
        if (nabsavg)
            absavg /= nabsavg;
        //
        //  -the minimum distance absmin between 2 particles during the run of the simulation
        //  -A Correct simulation will have particles stay at greater than 0.4 (of cutoff) with typical values between .7-.8
        //  -A simulation were particles don't interact correctly will be less than 0.4 (of cutoff) with typical values between .01-.05
        //
        //  -The average distance absavg is ~.95 when most particles are interacting correctly and ~.66 when no particles are interacting
        //
        printf(", absmin = %lf, absavg = %lf", absmin, absavg);
        if (absmin < 0.4)
            printf("\nThe minimum distance is below 0.4 meaning that some particle is not interacting");
        if (absavg < 0.8)
            printf("\nThe average distance is below 0.8 meaning that most particles are not interacting");
    }
    printf("\n");

    //
    // Printing summary data
    //
    if (fsum)
    {
        fprintf(fsum, "%d %g\n", n, simulation_time);
    }

    //
    // Clearing space
    //
    if (fsum)
    {
        fclose(fsum);
    }

    //free(particles);

    return 0;
}
