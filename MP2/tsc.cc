#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <grpc++/grpc++.h>
#include <google/protobuf/util/time_util.h>
#include "client.h"

#include "sns.grpc.pb.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::ClientReaderWriter;
using grpc::ClientWriter;
using grpc::Status;
using csce438::Message;
using csce438::Reply;
using csce438::Request;
using csce438::SNSService;

// helper functions for the class
enum IStatus parse_response(Reply &reply);


class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string hostname;
        std::string username;
        std::string port;
        std::unique_ptr<SNSService::Stub> _stub; // instance of client stub
        
        // You can have an instance of the client stub
        // as a member variable.
        //std::unique_ptr<NameOfYourStubClass::Stub> stub_;
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------
	
	//create a new cheannel for the stub to connect with
	_stub = SNSService::NewStub(grpc::CreateChannel(hostname, grpc::InsecureChannelCredentials()));
	
	// create a new request to login 
	Request request;
	request.set_username(username);
	ClientContext context;
	Reply reply;
	
	// send the request to the stub and bring back a status
	Status status = _stub->Login(&context, request, &reply);
	
	if(status.ok()){
	    return 1;
	}else{
	    return -1;
	}
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// ------------------------------------------------------------
	
    std::size_t index = input.find_first_of(" ");
    std::string cmd, user_name;
    if(index != std::string::npos){ // there was a space in the command
        cmd = input.substr(0, index);
        user_name = input.substr(index+1,(input.length() - index));
    }else{ // there was no space in the input
        cmd = input;
    }
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
	IReply Ireply;
	// make a new request used for all the cases
	Request request;
	Status status;
    request.set_username(username);
	ClientContext context;
	Reply reply;
	
    switch(cmd[0]){ // differentiate by the inital letter of the command
        case 'F': 
            // set the repeated string to be the user to follow
            request.add_arguments(user_name);
            status = _stub->Follow(&context, request, &reply);
            Ireply.grpc_status = status;
            if(status.ok()){
                Ireply.comm_status = parse_response(reply);
                return Ireply;
            }else{
                Ireply.comm_status = FAILURE_UNKNOWN;
                return Ireply;
            }
            break;
        case 'U':
            request.add_arguments(user_name);
            status = _stub->UnFollow(&context, request, &reply);
            Ireply.grpc_status = status;
            if(status.ok()){
                Ireply.comm_status = parse_response(reply);
                return Ireply;
            }else{
                Ireply.comm_status = FAILURE_UNKNOWN;
                return Ireply;
            }
            break;
        case 'L': // list
            // list does not need another agument
            status = _stub->List(&context, request, &reply);
            Ireply.grpc_status = status;
            if(status.ok()){
                Ireply.comm_status = parse_response(reply);
                if(Ireply.comm_status == SUCCESS){
                    // get the list of all users
                    for(int i = 0; i < reply.all_users().size(); ++i){
                        Ireply.all_users.push_back(reply.all_users(i));
                    }
                    // get the list of following users
                    for(int i = 0; i < reply.following_users().size(); ++i){
                        Ireply.following_users.push_back(reply.following_users(i));
                    }
                }
                return IReply;
            }else{
                Ireply.comm_status = FAILURE_UNKNOWN;
                return Ireply;
            }
            break;
        case 'T':
            // for the timeline 
            break;
    }
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Follow" service method for FOLLOW command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Follow(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
    // ------------------------------------------------------------
    return Ireply;
}

void Client::processTimeline()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------

    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
}

enum IStatus parse_response(Reply& reply){
    switch(reply.msg()[0]){
        case 'S':
            return SUCCESS;
        case 'A':
            return FAILURE_ALREADY_EXISTS;
        case 'N':
            return FAILURE_NOT_EXISTS;
        case 'I':
            return FAILURE_INVALID_USERNAME;
        case 'F':
            return FAILURE_INVALID;
        case 'U':
            return FAILURE_UNKNOWN;
    }
}