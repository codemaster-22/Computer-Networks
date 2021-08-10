#include <bits/stdc++.h>
using namespace std;
#include <pthread.h>
#include <ctime>
#include <ratio>
#include <chrono>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
using namespace std::chrono;
#define MAX_LIMIT 2000

//global variables unchanged..
int reciever_port;
int max_packets;
float random_drop_probab;
int sock_fd;
bool debug_mode;
int sequence_num_field;
int window_size;
int max_buffer_size;

struct sockaddr_in servaddr;
bool is_first;
queue<int> recieve_buffer;

vector<long long> arr[256];

void recieve()
{    
    int num_ack = 0;
    default_random_engine generator = default_random_engine();
    bernoulli_distribution distribution(random_drop_probab);
  

    while(1)
    {
       struct sockaddr_in clientaddr;
       memset(&clientaddr,0,sizeof(clientaddr));
       int n;
       int cnt = 0;
       unsigned int len;
       char buffer[MAX_LIMIT];
       len = sizeof(clientaddr);
       n = recvfrom(sock_fd,(char*)buffer,MAX_LIMIT,MSG_WAITALL,(struct sockaddr*)&clientaddr,&len);
       long long x =  (long long) chrono::duration_cast<chrono::microseconds>(high_resolution_clock::now().time_since_epoch()).count();
       if(!distribution(generator))
       { 
           if(recieve_buffer.size()<max_buffer_size)
       	   {   
                    num_ack++;
       	   	   int seq_num = 0;
       	   	   for(int i = 0;i<sequence_num_field;i++)
		   {
		      seq_num += (1<<(sequence_num_field-1-i))*((int)buffer[i]-'0');
		   }
		   recieve_buffer.push(seq_num);
		   arr[seq_num].push_back(x);
                   recieve_buffer.pop();
                   sendto(sock_fd,(const char* )buffer,strlen(buffer),MSG_CONFIRM,(struct sockaddr* )&clientaddr,sizeof(clientaddr));
       	   }
           if(num_ack == max_packets)
           { 
              break;
           }
       }
    }

}

void preprocess_input(int num,string input[])
{
   debug_mode = false;
   
   set<string> options;

   for(int i = 0;i<num;)
   {
   	  if(options.find(input[i]) != options.end())
      {
       	   cout<<"Error: Used the following option"<<input[i]<<"multiple times: \n";
       	   exit(-1);
      }
      options.insert(input[i]);

      if(input[i] == "-d")
      {
      	 debug_mode = true;
      	 i += 1;
      }
      else if(input[i] == "-p")
      {
         reciever_port = stoi(input[i+1]);
         i += 2;
      }
      else if(input[i] == "-N")
      {
      	 max_packets = stoi(input[i+1]);
      	 i += 2;
      }
      else if(input[i] == "-n")
      {
         sequence_num_field = stoi(input[i+1]);
         i += 2;
      }
      else if(input[i] == "-W")
      {
         window_size = stoi(input[i+1]);
         i += 2;
      }
      else if(input[i] == "-B")
      {
         max_buffer_size = stoi(input[i+1]);
         i += 2;
      }
      else if(input[i] == "-e")
      {
      	 random_drop_probab = stof(input[i+1]);
      	 i += 2;
      }
      else
      {
      	cout<<"Error: Invalid option given: "<<input[i]<<"\n";
       	exit(-1);
      }
   }

   if((options.size() != 6) && (options.size() != 7))
   {
   	  cout<<"Error: Invalid number of arguments given\n";
   	  exit(-1);
   }

}


int main(int argc,char* argv[])
{  
    if(argc != 14 && argc != 13)
    {
    	cout<<"Invalid_number of options given\n";
    	exit(-1);
    }
	
    string input[13];

    for(int i = 1;i<argc;i++)
    {
    	int len = strlen(argv[i]);
    	for(int j = 0;j<len;j++)
    		input[i-1].push_back(argv[i][j]);
    }

    preprocess_input(argc-1,input);

    cout<<"Input taken successfully"<<endl;
    // creation of socket
    if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0))<0)
    {   
   	   cout<<"Cannot create socket for the node"<<endl;
   	   exit(-1);
   }

   struct sockaddr_in saddr;
   memset(&saddr,0,sizeof(saddr));
   saddr.sin_family = AF_INET; //IPV4
   saddr.sin_addr.s_addr = INADDR_ANY; // Any address
   saddr.sin_port = htons(reciever_port); // setting port number
   cout<<"Reciever_port :"<<reciever_port<<endl;
   if(bind(sock_fd,(const struct sockaddr* )&saddr,sizeof(saddr))<0)
   {
      cout<<"Bind failed for the node:"<<endl;
      exit(-1);
   }

   cout<<"Socket created successfully\n";
   
    recieve();
    if(debug_mode)
    {
   	  int l = 0;
   	  for(int i = 0;i<window_size;i++)
   	  	 l = max(l,(int)arr[i].size());

   	  for(int j = 0;j<l;j++)
   	  {
   	  	  for(int i = 0;i<window_size;i++)
   	  	  {
   	  	  	  if(j<arr[i].size())
   	  	  	  {
   	  	  	  	 cout<<"Seq #:"<<i<<" Time Recieved: "<<arr[i][j]/1000<<":"<<arr[i][j]%1000<<"\n";
   	  	  	  }
   	  	  }
   	  }
     }
   
   return 0;
}
