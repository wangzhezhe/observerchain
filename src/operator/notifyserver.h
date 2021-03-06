#ifndef notifyserver_h
#define notifyserver_h

#include <grpc++/grpc++.h>
#include "workflowserver.grpc.pb.h"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using workflowserver::NotifyReply;
using workflowserver::NotifyRequest;
using namespace std;


void* RunNotifyServer(void *arg);

string getNotifyServerAddr();

extern int NotifiedNum;
extern int COMPONENTID;
//extern string NOTIFYADDR;
#endif