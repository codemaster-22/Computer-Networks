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

//Global variables

char reciever_IP[20];
int reciever_port;
int packet_length;
int packet_gen_rate;
int max_packets;
int window_size;
int max_buffer_size;
bool debug_mode;
int sock_fd;
int num_attempts_sum;
bool exit_condition;

// general global variables
deque<pair<int,int>> send_buffer;
deque<pair<high_resolution_clock::time_point,pair<int,int>>> timer;
double rtt_avg;
int num_ack;
int curr_index;

//mutex locks
pthread_mutex_t gen_lock;


void* packet_generator(void* arg)
{  
   int seq_num = 0;
   //sequence number is 0 to window_size-1 
   while(1)
   {
      for(int i = 0;i<packet_gen_rate;i++)
      {  
      	 // as buffer shared global variable..
         if(seq_num == max_packets)
           goto end;
         pthread_mutex_lock(&gen_lock);
      	 if(send_buffer.size()<max_buffer_size)
      	    send_buffer.push_back({0,seq_num++});
      	 pthread_mutex_unlock(&gen_lock);
      }
      sleep(1);
   }
  end: ;
}

bool recieved_ack()
{
	// remove the first element from deque
	// remove the first element from timer
	// decrement the index
    
    pthread_mutex_lock(&gen_lock);
    high_resolution_clock::time_point t = high_resolution_clock::now();
    duration<double, std::micro> time_span = t-timer.front().first;
    long long x =  (long long) chrono::duration_cast<chrono::microseconds>(timer.front().first.time_since_epoch()).count();
    int num_attempts = timer.front().second.first;
    int seq_num = timer.front().second.second;
    int duration = time_span.count();
    rtt_avg = ((rtt_avg)*num_ack+duration*1.0)/(num_ack+1);
    num_attempts_sum += num_attempts;
    num_ack += 1;

    if(debug_mode)
    {
    	cout<<"Seq #"<<seq_num<<": "<<"Time Generated: "<<x/1000<<":"<<x%1000<<" RTT: "<<duration<<" Number of attempts: "<<num_attempts<<endl;
    }
	if(num_ack == max_packets)
	{  
	   cout<<"PACKET_GEN_RATE: "<<packet_gen_rate<<"\n";
	   cout<<"PACKET_LENGTH: "<<packet_length<<"\n";
	   cout<<"Retransmission Ratio: "<<(num_attempts_sum*1.0)/(num_ack)<<"\n";
	   cout<<"Average rtt value: "<<rtt_avg<<"\n";
           exit_condition = true;
	   pthread_mutex_unlock(&gen_lock);
	   return false;
	}

	timer.pop_front();
	send_buffer.pop_front();
	curr_index--;

	pthread_mutex_unlock(&gen_lock);
	return true;
}

void* recieve(void* arg)
{   
	while(1)
	{
	  struct sockaddr_in temp;
	  memset(&temp,0,sizeof(temp));
          unsigned int len,n;
          char buffer[MAX_LIMIT];
	  n = recvfrom(sock_fd,(char*)buffer,MAX_LIMIT,MSG_WAITALL,(struct sockaddr*)&temp,&len);
          if(!recieved_ack())
    	     break;
        }
}

void* send_packets(void* arg)
{
   rtt_avg = 0;
   num_attempts_sum = 0;
   curr_index = 0;
   while(1) {

        pthread_mutex_lock(&gen_lock);
        if(exit_condition)
        {
      	  pthread_mutex_unlock(&gen_lock);
      	  break;
        }
    //    cout<<"In send_packets"<<" "<<(timer.size())<<" "<<window_size<<" "<<curr_index<<" "<<send_buffer.size()<<endl;
        if(timer.size()<window_size && (curr_index<send_buffer.size())){
          
	    // send curr_index packet to Reciever and increment curr_index packet
            send_buffer[curr_index].first += 1;
	    pair<int,int> p =  send_buffer[curr_index];
	    int seq_num = p.second;
	    char buffer[MAX_LIMIT];
            string str = to_string(seq_num);
	    str.push_back(' ');
            //cout<<"         About to send packet with packet of length: "<<packet_length<<endl;
	    int cnt = 0;
	    for(auto it:str)
	          buffer[cnt++] = it;
           // cout<<"         About to send packet with packet of length: "<<packet_length<<endl;
	    while(cnt<(packet_length-1))
	    {   
                buffer[cnt++] = '1';
              //  cout<<cnt<<endl;
            }
	    buffer[cnt] = '\0';
            //cout<<"         About to send packet with packet of length: "<<packet_length<<endl;
	    struct sockaddr_in clientaddr;
	    memset(&clientaddr,0,sizeof(clientaddr));
	    clientaddr.sin_family = AF_INET; //IPV4
	    clientaddr.sin_addr.s_addr = inet_addr(reciever_IP); // setting client address
	    clientaddr.sin_port = htons(reciever_port); // setting port number
            //cout<<"Sending packet to reciever with seqnum: "<<seq_num<<endl;
	    sendto(sock_fd,(const char* )buffer,strlen(buffer),MSG_CONFIRM,(const struct sockaddr* )&clientaddr,sizeof(clientaddr));
	    timer.push_back({high_resolution_clock::now(),p});
	    curr_index++;
	    //send to reciever

	    pthread_mutex_unlock(&gen_lock);
         }
         else if((int)timer.size()>0){
   	     
   	      double time_out = 100*1000;
   	      if(num_ack > 10)
   	  	      time_out = 2*rtt_avg;

               high_resolution_clock::time_point t = high_resolution_clock::now();
               duration<double, std::micro> time_span = t-timer.front().first;
      
               pthread_mutex_unlock(&gen_lock);

               while(time_span.count()<time_out)
               {
      	             t = high_resolution_clock::now();
      	             pthread_mutex_lock(&gen_lock);
      	             if(timer.size()<window_size)
      		     {  
      		 	 pthread_mutex_unlock(&gen_lock);
      		         goto end;
      		     }
                     time_span = t-timer.front().first;
                     pthread_mutex_unlock(&gen_lock);
               }

               //it means timeout had occurred we have to resend all the things
               pthread_mutex_lock(&gen_lock);
               if(timer.size() == window_size)
               { 
          	  timer.clear();
                  curr_index = 0;
               }
               pthread_mutex_unlock(&gen_lock);
        }
        else
           pthread_mutex_unlock(&gen_lock);
   end: ;
}

}





void preprocess_input(int num,string input[])
{
   debug_mode = false;
   exit_condition = false;
   set<string> options;

   for(int i = 0;i<num;)
   {   
       if(options.find(input[i]) != options.end())
       {
       	   cout<<"Error: Used the following option"<<input[i]<<"multiple times: \n";
       	   exit(-1);
       }
       options.insert(input[i]);
       if(input[i] == "-s")
       {
       	  int cnt = 0;
       	  for(auto it : input[i+1])
       	  	reciever_IP[cnt++] = it;
       	  reciever_IP[cnt] = '\0';
       	  i += 2;
       }
       else if(input[i] == "-p")
       {
       	  reciever_port = stoi(input[i+1]);
       	  i += 2;
       }
       else if(input[i] == "-l")
       {
       	  packet_length = stoi(input[i+1]);
       	  i += 2;
       }
       else if(input[i] == "-r")
       {
       	  packet_gen_rate = stoi(input[i+1]);
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
       else if(input[i] == "-w")
       {
       	  window_size = stoi(input[i+1]);
       	  i += 2;
       }
       else if(input[i] == "-b")
       {
       	  max_buffer_size = stoi(input[i+1]);
       	  i +=2;
       }
       else
       {
       	  cout<<"Error: Invalid option given: "<<input[i]<<"\n";
       	  exit(-1);
       }
   }

   if((options.size() != 7) && (options.size() != 8))
   {
   	  cout<<"Error: Invalid number of arguments given\n";
   	  exit(-1);
   }
}



int main(int argc,char* argv[])
{   
    
    if(argc != 15 && argc != 16)
    {
    	cout<<"Invalid_number of options given\n";
    	exit(-1);
    }
	  string input[15];
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
   saddr.sin_port = htons(9000); // setting port number
   
   if(bind(sock_fd,(const struct sockaddr* )&saddr,sizeof(saddr))<0)
   {
      cout<<"Bind failed for the node:"<<endl;
      exit(-1);
   }
   cout<<"Socket created successfully\n";

   exit_condition = false;
   num_ack = 0;

   pthread_t  a,b,c;

   pthread_create(&a,NULL,recieve,NULL);
   pthread_create(&b,NULL,send_packets,NULL);
   pthread_create(&c,NULL,packet_generator,NULL);

   pthread_join(a,NULL); 
   pthread_join(b,NULL);
   pthread_join(c,NULL);
   //close(sock_fd);

   return 0;
}
