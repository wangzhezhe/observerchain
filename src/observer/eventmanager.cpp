
#include "stdlib.h"
#include "stdio.h"
#include "errno.h"
#include "string.h"
#include "pthread.h"

#include "eventmanager.h"
#include "../runtime/slurm.h"
#include "../runtime/local.h"
#include "../publishclient/pubsubclient.h"
#include "../utils/groupManager/groupManager.h"
#include <string>
#include <iostream>
#include <queue>
#include <stdint.h> /* for uint64 definition */
#include <stdlib.h> /* for exit() definition */
#include <time.h>   /* for clock_gettime */
#include <pthread.h>
#include <uuid/uuid.h>
#include "../../deps/spdlog/spdlog.h"

#define BILLION 1000000000L

using namespace std;

/*
{
    "type": "EVENT",
    "eventList": ["T1_FINISH"],
    "driver": "slurm",
    "actionList": [
       "./triguresbatchjoba",
       "./triguresbatchjobb"
    ]
}
*/

enum FILETYPE
{
    TRIGGER,
    OPERATOR
};

queue<pthread_t> threadIdQueue;
mutex subscribedMutex;
int SubscribedClient = 0;

mutex publishMutex;
int publishClient = 0;

vector<string> operatorList;

//from client id to the config file that is needed to be excuted when there is notify request
mutex clientIdtoConfigMtx;
map<string, EventTriggure *> clientIdtoConfig;

/*
void initOperator(int jsonNum)
{

    GreeterClient *greeter = GreeterClient::getClient();
    if (greeter == NULL)
    {
        printf("failed to get initialised greeter\n");
        return;
    }

    vector<string> InitList;
    InitList.push_back(string("INIT"));

    while (1)
    {
        int reply = greeter->GetSubscribedNumber(InitList);
        //printf("there are %d clients subscribe INIT event\n", reply);
        if (reply < jsonNum)
        {
            usleep(100);
        }
        else
        {
            break;
        }
    }

    //it's better to declare a new document instance for new file every time
    Document d;
    //go throught the operatorVector
    int len = operatorList.size();
    int i = 0;
    char command[500];
    for (i = 0; i < len; i++)
    {
        //get the operator command
        //printf("parsing operator (%s)\n", operatorList[i].data());
        d.Parse(operatorList[i].data());

        const char *type = d["type"].GetString();
        printf("execute type:(%s)\n", type);
        const char *action = d["action"].GetString();
        printf("execute action:(%s)\n", action);
        system(action);
    }
}
*/

void eventPublish(vector<string> pubList, string metadata)
{
    //GreeterClient *greeter = roundrobinGetClient();
    //only use first event in the list to do the hash mapping
    string eventMsg = pubList[0];
    
    //sometimes there is a deadlock here when event is in large number
    GreeterClient *greeter = getClientFromEvent(eventMsg,publishClient);
    
    if (greeter == NULL)
    {
        printf("failed to get greeter for event publisher from event %s\n", eventMsg.data());
        return;
    }

    //printf("debug event publish evt msg %s ok to get client\n",eventMsg.data());

    if ((publishClient+1) % 128 == 0)
    {
        spdlog::info("id {} publish times {}", gm_rank, publishClient);
    }

    string reply = greeter->Publish(pubList, sourceClient, metadata, "NAME");

    spdlog::debug("id {} get reply for publish time {}", gm_rank,publishClient);

    if (reply.compare("OK") != 0)
    {
        printf("rpc failed, publish %s failed\n", pubList[0].data());
    }

    //printf("debug client %d publish event %s\n",gm_rank, eventMsg.data());

    return;
}

//TODO delete the eventMsg, it could be the first element in etrigger
void eventSubscribe(EventTriggure *etrigger, string clientID, string notifyAddr, string eventMsg)
{
    //only could be transfered by this way if original pointed is initiallises by malloc instead on new
    //EventTriggure *etrigger = (EventTriggure *)arguments;

    //GreeterClient *greeter = roundrobinGetClient();

    //printf("debug event subscribe\n", eventMsg.data());

    GreeterClient *greeter = getClientFromEvent(eventMsg,publishClient);

    if (greeter == NULL)
    {
        printf("failed to get greeter from %s\n", eventMsg.data());
        return;
    }

    //test using
    //string reply = greeter->SayHello("fakeSubscriber");
    //cout << "hello return value: " << reply << endl;

    //struct timespec subStart, subEnd, diff;

    //clock_gettime(CLOCK_REALTIME, &subStart);

    subscribedMutex.lock();
    SubscribedClient++;
    subscribedMutex.unlock();

    //if (SubscribedClient % 100 == 0)
    //{
    //    printf("sub times %d\n", SubscribedClient);
    //}

    //printf("debug sub event %s\n",etrigger->eventSubList[0].data());
    //(vector<string> eventSubList, string clientID, string notifyAddr, string source, string matchType, string metadata)
    string reply = greeter->Subscribe(etrigger->eventSubList, clientID, notifyAddr, sourceClient, etrigger->matchType, etrigger->metaData);
    //cout << "Subscribe return value: " << reply << endl;

    //clock_gettime(CLOCK_REALTIME, &subEnd);

    //diff = BILLION * (subEnd.tv_sec - subStart.tv_sec) + subEnd.tv_nsec - subStart.tv_nsec;
    //printf("debug time get (%s) response time = (%lf) second\n", etrigger->eventList[0].data(), (float)diff / BILLION);

    int i = 0;

    if (reply.compare("SUBSCRIBED") != 0)
    {
        printf("rpc failed, don't execute command:\n");
        int actionSize = etrigger->actionList.size();
        for (i = 0; i < actionSize; i++)
        {
            printf("%s\n", etrigger->actionList[i].data());
        }
    }

    return;
}

int jsonIfTriggerorOperator(Document &d, char *jsonbuffer)
{
    d.Parse(jsonbuffer);
    const char *type = d["type"].GetString();
    //printf("get type after parsing (%s)\n",type);
    if (strcmp(type, "TRIGGER") == 0)
    {
        return 1;
    }
    else if (strcmp(type, "OPERATOR") == 0)
    {
        return 2;
    }
    else
    {
        return -1;
    }
}

void waitthreadFinish()
{

    int joinReturn;
    pthread_t currpid;

    while (threadIdQueue.empty() == false)
    {
        printf("thread num waiting to be finished %d\n", threadIdQueue.size());
        currpid = threadIdQueue.front();
        joinReturn = pthread_join(currpid, NULL);
        //printf("thread id %ld return %d\n", currpid, joinReturn);
        threadIdQueue.pop();
    }
}

//controle when to start operator

/*
{
    "type": "TRIGGER",
    "eventList": ["INIT"],
    "driver": "local",
    "actionList": [
       "/bin/bash ./app/simulate.sh --timesteps 1 --range 100 --nvalues 5 --log off > sim1.out",
       "./operator T1SIM_FINISH 3 publish T1SIM_FINISH"
     ]
}
*/

/*
typedef struct EventTriggure
{
    vector<string> eventList;
    string driver;
    vector<string> actionList;

} EventTriggure;
*/

void outputTriggure(string clientId)
{
    EventTriggure *et = clientIdtoConfig[clientId];
    printf("driver %s\n", et->driver.data());
    int size = et->eventSubList.size();
    int i;
    for (i = 0; i < size; i++)
    {
        printf("sub event %s\n", et->eventSubList[i].data());
    }

    size = et->actionList.size();
    for (i = 0; i < size; i++)
    {
        printf("action %s\n", et->actionList[i].data());
    }

    return;
}

/*

typedef struct EventTriggure
{
    string driver;
    vector<string> eventSubList;
    vector<string> eventPubList;
    vector<string> actionList;

} EventTriggure;

*/
//put key info into the configuration
EventTriggure *fakeaddNewConfig(string driver,
                                vector<string> eventSubList,
                                vector<string> eventPubList,
                                vector<string> actionList,
                                string &clientID)
{
    EventTriggure *triggure = new (EventTriggure);

    triggure->driver = driver;
    triggure->eventSubList = eventSubList;
    triggure->eventPubList = eventPubList;
    triggure->actionList = actionList;
    triggure->matchType = "NAME";

    uuid_t uuid;
    char idstr[50];

    uuid_generate(uuid);
    uuid_unparse(uuid, idstr);

    string clientId(idstr);
    clientIdtoConfigMtx.lock();
    clientIdtoConfig[clientId] = triggure;
    //printf("add clientid %s\n", clientId.data());
    clientIdtoConfigMtx.unlock();

    //outputTriggure(clientId);
    clientID = clientId;
    return triggure;
}

/*
example:
get json buff {
    "matchType": "NAME",
    "eventSubList": ["dataPattern_A"],
    "driver": "local",
    "metaData":"null",
    "actionList": [
        "./runtimeScripts/triguresbatchjobb"
     ]
}

get json buff {
    "matchType": "META_GRID",
    "eventSubList": ["variable_A"],
    "driver": "local",
    "metaData":"<0,0>:<2,2>",
    "actionList": [
        "./runtimeScripts/triguresbatchjobA"
     ]
}
*/

EventTriggure *addNewConfig(string jsonbuffer, string &clientID)
{
    //parse json buffer
    Document d;
    //printf("current file data %s\n",jsonbuffer.data());
    d.Parse(const_cast<char *>(jsonbuffer.data()));
    //printf("jsonIfTriggerorOperator (%s)\n", jsonbuffer.data());

    EventTriggure *triggure = new (EventTriggure);

    //get client id
    uuid_t uuid;
    char idstr[50];

    uuid_generate(uuid);
    uuid_unparse(uuid, idstr);

    printf("get client id %s\n", idstr);
    printf("get json buff %s\n", jsonbuffer.data());

    //parse info

    const char *matchtype = d["matchType"].GetString();

    const char *driver = d["driver"].GetString();

    const char *metadata = d["metaData"].GetString();

    triggure->driver = string(driver);
    triggure->metaData = string(metadata);
    triggure->matchType = string(matchtype);

    //register trigure and send subscribe call to pub-sub backend
    const Value &eventSubList = d["eventSubList"];
    const Value &actionList = d["actionList"];

    SizeType i;

    for (i = 0; i < eventSubList.Size(); i++)
    {
        const char *tempsubstr = eventSubList[i].GetString();
        string substr = string(tempsubstr);
        triggure->eventSubList.push_back(substr);
    }

    for (i = 0; i < actionList.Size(); i++)
    {
        const char *tempstr = actionList[i].GetString();
        triggure->actionList.push_back(string(tempstr));
    }

    string clientId(idstr);
    clientIdtoConfigMtx.lock();
    clientIdtoConfig[clientId] = triggure;
    //printf("add clientid %s\n", clientId.data());
    clientIdtoConfigMtx.unlock();

    //outputTriggure(clientId);
    clientID = clientId;
    return triggure;
}