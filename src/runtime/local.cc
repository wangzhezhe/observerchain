#include "time.h"
#include "stdlib.h"
#include "stdio.h"
#include "local.h"
#include <string>
#include <mutex>

using namespace std;

mutex testNotifiedNumMtx;
int localFinishNum = 0;

int localTaskStart(string batchPath)
{
    //call sbtch command
    //printf("test output\n");
    char command[200];
    //in case that the number of job is larger than limitation
    //#ifdef TIME
    //send publish api and record time

    //printf("finish sec:(%ld),finish nsec:(%ld)\n",finish.tv_sec,finish.tv_nsec);

    //#endif
    sprintf(command, "%s", batchPath.data());
#ifdef DEBUG
    printf("execute command by local way:(%s)\n", command);
#endif

    //test using
    system(command);

    //struct timespec finish;
    //auto console = spd::stdout_color_mt("console");
    //spdconsole->info("finish sec:({:d}),finish nsec:({:d})", finish.tv_sec, finish.tv_nsec);

    //testNotifiedNumMtx.lock();
    //localFinishNum++;
    //testNotifiedNumMtx.unlock();
    
    return 0;
}