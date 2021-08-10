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
#include <time.h>
using namespace std::chrono;
#define MAX_LIMIT 2000


//global_variables
int reciever_port;
int max_packets;
float random_drop_probab;
int sock_fd;
bool debug_mode;





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

      if(input[i] == "-p")
      {
      	reciever_port = stoi(input[i+1]);
      	i += 2;
      }
      else if(input[i] == "-n")
      {
      	 max_packets = stoi(input[i+1]);
      	 i += 2;
      }
      else if(input[i] == "-d")
      {
      	debug_mode = true;
      	i += 1;
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

   if((options.size() != 4) && (options.size() != 3))
   {
   	  cout<<"Error: Invalid number of arguments given\n";
   	  exit(-1);
   }
}



int main(int argc , char* argv[])
{  
    if(argc != 8 && argc != 7)
    {
    	cout<<"Invalid_number of options given\n";
    	exit(-1);
    }
	
	string input[7];

    for(int i = 1;i<argc;i++)
    {
    	int len = strlen(argv[i]);
    	for(int j = 0;j<len;j++)
    		input[i-1].push_back(argv[i][j]);
    }

    preprocess_input(argc-1,input);
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

   if(bind(sock_fd,(const struct sockaddr* )&saddr,sizeof(saddr))<0)
   {
      cout<<"Bind failed for the node:"<<endl;
      exit(-1);
   }

   cout<<"Socket created successfully\n";

   int num_ack = 0;
   int expected_seq = 0;

   default_random_engine generator = default_random_engine();
   bernoulli_distribution distribution(random_drop_probab);
   cout<<"Probability : "<<random_drop_probab<<endl;
   while(1)
   {  
   	  if(num_ack == max_packets)
   	  	 break;
     
          struct sockaddr_in clientaddr;
	  memset(&clientaddr,0,sizeof(clientaddr));
	  unsigned int len,n;
          len = sizeof(clientaddr);
	  char buffer[MAX_LIMIT];
	  n = recvfrom(sock_fd,(char*)buffer,MAX_LIMIT,MSG_WAITALL,(struct sockaddr*)&clientaddr,&len);
	  long long x =  (long long) chrono::duration_cast<chrono::microseconds>(high_resolution_clock::now().time_since_epoch()).count();
	  buffer[n] = '\0';
      if(!distribution(generator))
      {
         int num = 0;
         for(int i = 0;i<n;i++)
         {
         	 if(buffer[i] == ' ')
         	 	break;
         	 num = num*10 + (int)(buffer[i]-'0');
         }

         if(num == expected_seq)
         {  
         	memset(buffer,0,sizeof(buffer));
                if(debug_mode)
         	    cout<<"Seq #:"<<num<<" Time Recieved: "<<x/1000<<":"<<x%1000<<"\n";
         	string s = "ACK "+to_string(expected_seq);
         	int cnt = 0;
         	for(auto it : s)
         		buffer[cnt++] = it;
         	buffer[cnt] = '\0';
                sendto(sock_fd,(const char* )buffer,strlen(buffer),MSG_CONFIRM,(const struct sockaddr* )&clientaddr,sizeof(clientaddr));
         	num_ack++;
         	expected_seq++;
         }
    	
      }

   }

   close(sock_fd);
   
  return 0;
}
