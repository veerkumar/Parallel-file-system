#include "commons.h"
#include "fs_mdm.h"


#define INTERFACE "ens33"


string ipAddress;
int port;
string file_server_ip_port;

 meta_data_manager_client *mdm_service;

void  
set_ipaddr_port() {
	struct ifaddrs *interfaces = NULL;
	struct ifaddrs *temp_addr = NULL;
	int success = 0;
	// retrieve the current interfaces - returns 0 on success
	success = getifaddrs(&interfaces);
	if (success == 0) {
		// Loop through linked list of interfaces
		temp_addr = interfaces;
		while(temp_addr != NULL) {
			if(temp_addr->ifa_addr->sa_family == AF_INET) {
				if(strcmp(temp_addr->ifa_name, INTERFACE)==0){
					ipAddress=inet_ntoa(((struct sockaddr_in*)temp_addr->ifa_addr)->sin_addr);
#ifdef DEBUG_FLAG				
					cout<<"\n IPaddress"<<ipAddress;
#endif
				}
			}
			temp_addr = temp_addr->ifa_next;
		}
	}
	// Free memory
	freeifaddrs(interfaces);
	/* initialize random seed: */
	srand (time(NULL));

	/* generate secret number between 5000 to 65000: */
	port = rand() % 60000 + 5000;
	file_server_ip_port.append(ipAddress);
	file_server_ip_port.append(":");
	file_server_ip_port.append(to_string(port));
	//return file_server_ip_port;
}


int get_random_number () {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::default_random_engine generator(seed);
	 std::uniform_int_distribution<int> distribution(0, INT_MAX);
	return distribution(generator);
}

void 
print_response(register_service_response_t *c_response) {
	cout<<"\n"<<c_response->code;
	cout<<"\n";
}

void 
print_request(register_service_request_t *c_req) {
	cout<<"\n"<<c_req->type;
	cout<<"\n"<<c_req->ip_port;
}

register_service_response_t* 
extract_response_from_payload(RegisterServiceResponse Response) {
	register_service_response_t *c_response = new register_service_response_t;
	if(Response.code() == RegisterServiceResponse::OK) {
		c_response->code = OK;
	}
	if(Response.code() == RegisterServiceResponse::ERROR) {
		c_response->code = ERROR;
	}
	return c_response;
}

void 
make_req_payload (RegisterServiceRequest *payload, 
		register_service_request_t *req) {
	if(req->type == FILE_SERVER) {
		payload->set_type(RegisterServiceRequest::FILESERVER);
	}
	payload->set_ipport(req->ip_port);
}

/*Class: which hold function which we will use to call me services*/
		

register_service_response_t* meta_data_manager_client::register_service_handler( register_service_request_t *c_req) {
	RegisterServiceRequest ReqPayload;
	RegisterServiceResponse Response;
	ClientContext Context;

	register_service_response_t *c_response = NULL;
	make_req_payload(&ReqPayload, c_req);

	print_request(c_req);

	// The actual RPC.
	Status status = stub_->registerServiceHandler(&Context, ReqPayload, &Response);

	// Act upon its status.
	if (status.ok()) {
		c_response = extract_response_from_payload(Response);
		return c_response;
	} else {
		std::cout << status.error_code() << ": " << status.error_message()
			<< std::endl;
		return 0;
	}
}

void meta_data_manager_client::update_last_modified_time (string file_name) {
	UpdateLastModifiedServiceRequest Req;
	UpdateLastModifiedServiceResponse Response;
	ClientContext Context;

	Req.set_filename(file_name);
	Req.set_time(time(0));  // epoch time
	Status status = stub_->updateLastModifiedServiceHandler(&Context, Req, &Response);
	// Act upon its status.
	if (status.ok()) {
		cout<<"Last modified update return code: "<< Response.code();
		return ;
	} else {
		std::cout << status.error_code() << ": " << status.error_message() << std::endl;
		return ;
	}
}

class file_server_service_impl : public FileServerService::Service {
        
	Status fileReadWriteRequestHandler (ServerContext* context,const  FileReadWriteRequest* request, FileReadWriteResponse* reply) override {

            std::cout << "\nGot the message ";
	    if (request->type() == FileReadWriteRequest::READ) {
	    		/*TODO we need to read from file  */
	    		 cout<<"\n"<<request->reqipaddrport();
             cout<<"\n"<<request->startbyte();
             cout<<"\n"<<request->endbyte();
             cout<<"\n"<<request->requestid();
             cout<<"\n"<<request->filename();
             cout<<"\n"<<request->reqipaddrport();
             reply->set_requestid(request->requestid());
	     reply->set_reqstatus("OK");
	     reply->set_data("As of now I have this as file data, I will improve later");
	    } else {
	    
	    	/*TODO open file here in write mode and write from startbyte to end_byte
		 * and update lastmodified in meta_Data_manager */

		cout<<"\n"<<request->reqipaddrport();
           cout<<"\n"<<request->startbyte();
           cout<<"\n"<<request->endbyte();
           cout<<"\n"<<request->requestid();
           cout<<"\n"<<request->filename();
           cout<<"\n"<<request->reqipaddrport();
           reply->set_requestid(request->requestid());
           reply->set_reqstatus("OK");
           reply->set_data("");
	   /*close file*/

	   /**/
		//Send last modified update to meta data server with file name
		mdm_service->update_last_modified_time(request->filename());
	    
	    }

            return Status::OK;
    }


};


void initialization(){
	/*Create file server ip and port*/
	set_ipaddr_port();
	
	/* Start fileserver */
	file_server_service_impl fs_server;

	ServerBuilder builder;
	builder.AddListeningPort(file_server_ip_port, grpc::InsecureServerCredentials());	
	builder.RegisterService(&fs_server);
	std::unique_ptr<Server> server(builder.BuildAndStart());
        std::cout << "File Server listening on " << file_server_ip_port << std::endl;


	/* Register fileserver service with metadata manager */
	mdm_service = new meta_data_manager_client (grpc::CreateChannel("localhost:50051", grpc::InsecureChannelCredentials()));

	register_service_request_t *c_req = new register_service_request_t;
	register_service_response_t *c_response = NULL;

	c_req->type  = FILE_SERVER;
	c_req->ip_port = file_server_ip_port;

	c_response = mdm_service->register_service_handler(c_req);
	cout<<"Response recieved";
	print_response(c_response);

    /* Call wont return, it will wait for server to shutdown */
     server->Wait();
	return ;
}

int main(int argc, char** argv) {
	initialization();
	
	return 0;
}