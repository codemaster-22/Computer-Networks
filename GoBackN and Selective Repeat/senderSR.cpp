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
int max_packet_length;
int packet_gen_rate;
int max_packets;
int window_size;
int max_buffer_size;
bool debug_mode;
int sequence_num_field;
int sock_fd;

// general global variables
vector<pair<int,int>> send_buffer;
pair<high_resolution_clock::time_point,pair<int,int>>timer[256];
set<int> send_packets;
double rtt_avg;
int num_ack;
int curr_index;
bool exit_condition;
int num_attempts_sum;

//mutex locks
pthread_mutex_t gen_lock;


void* packet_generator(void* arg)
{  
   int seq_num = 0;
   //sequence number is 0 to window_size-1 
   default_random_engine generator;
   uniform_int_distribution<int> distribution(40,max_packet_length);

   while(1)
   {  
      pthread_mutex_lock(&gen_lock);
      if(exit_condition)
      {
        pthread_mutex_unlock(&gen_lock);
        break;
      }
      pthread_mutex_unlock(&gen_lock);
      for(int i = 0;i<packet_gen_rate;i++)
      {  
      	 // as buffer shared global variable..
         if(seq_num == max_packets)
                goto end;
         pthread_mutex_lock(&gen_lock);
      	 if(send_buffer.size()<max_buffer_size)
      	 {
      	    send_buffer.push_back({(seq_num++)%(window_size),distribution(generator)});
      	 }
      	 pthread_mutex_unlock(&gen_lock);
      }
      sleep(1);
   }
   end: ;
}

bool recieved_ack(int num)
{   
    // remove the first element from deque
    // remove the first element from timer
    // decrement the index
    pthread_mutex_lock(&gen_lock);
    high_resolution_clock::time_point t = high_resolution_clock::now();
    duration<double, std::micro> time_span = t-timer[num].first;
    long long x =  (long long) chrono::duration_cast<chrono::microseconds>(timer[num].first.time_since_epoch()).count();
    int num_attempts = timer[num].second.first;
    int duration = time_span.count();
    rtt_avg = ((rtt_avg)*num_ack+duration*1.0)/(num_ack+1);
    num_attempts_sum += num_attempts;
    num_ack += 1;

    for(auto it = send_buffer.begin();it != send_buffer.end();it++)
    {   
        pair<int,int> p = *it;
    	if(p.first == num)
    	{
    	   send_buffer.erase(it);
    	   curr_index--;
    	   break;
    	}
    }
    timer[num].second = {0,0};
    send_packets.erase(num);

    if(debug_mode)
    {
    	cout<<"Seq #"<<num<<": "<<"Time Generated: "<<x/1000<<":"<<x%1000<<" RTT: "<<duration<<" Number of attempts: "<<num_attempts<<endl;
    }
	if(num_ack == max_packets)
	{  
	   cout<<"PACKET_GEN_RATE: "<<packet_gen_rate<<"\n";
	   cout<<"PACKET_LENGTH: "<<max_packet_length<<"\n";
	   cout<<"Retransmission Ratio: "<<(num_attempts_sum*1.0)/(num_ack)<<"\n";
	   cout<<"Average rtt value: "<<rtt_avg<<"\n";
           exit_condition = true;
	   pthread_mutex_unlock(&gen_lock);
	   return false;
	}

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
           int seq_num = 0;
           for(int i = 0;i<sequence_num_field;i++)
           {
               seq_num += (1<<(sequence_num_field-1-i))*((int)buffer[i]-'0');
           }
           if(!recieved_ack(seq_num))
	      break;
        }

}

void send(int seq_num,int val,int packet_length)
{ 
  char buffer[packet_length];
  string str;
  for(int i = sequence_num_field-1;i>=0;i--)
  {
  	 if(seq_num&(1<<i))
 	 	str.push_back('1');
  	 else
  	 	str.push_back('0');
  }
  str.push_back(' ');

  int cnt = 0;
  for(auto it:str)
  	buffer[cnt++] = it;

  while(cnt < (packet_length-1))
  {
      buffer[cnt++] = '1';
  }
  buffer[cnt] = '\0';

  struct sockaddr_in clientaddr;
  memset(&clientaddr,0,sizeof(clientaddr));
  clientaddr.sin_family = AF_INET; //IPV4
  clientaddr.sin_addr.s_addr = inet_addr(reciever_IP); // setting client address
  clientaddr.sin_port = htons(reciever_port); // setting port number

  sendto(sock_fd,(const char* )buffer,strlen(buffer),MSG_CONFIRM,(const struct sockaddr* )&clientaddr,sizeof(clientaddr));
  timer[seq_num] = {high_resolution_clock::now(),{val,packet_length}};
  send_packets.insert(seq_num);

}

void* send_packet(void* arg)
{
   while(1) {

        pthread_mutex_lock(&gen_lock);
        if(exit_condition)
        {
      	  pthread_mutex_unlock(&gen_lock);
      	  break;
        }
        if(send_packets.size()<window_size && (curr_index < send_buffer.size())){
          
	      
              int seq_num = send_buffer[curr_index].first;
              if(timer[seq_num].second.first != 0)
              {  
                 // we have sent a same seq number and didn't get ack yet let's wait for it to come
                 double time_out = 100*1000;
   	         if(num_ack > 10)
   	  	     time_out = 2*rtt_avg;

                 high_resolution_clock::time_point t = high_resolution_clock::now();
                 duration<double, std::micro> time_span = t-timer[seq_num].first;
                 pthread_mutex_unlock(&gen_lock);

                 while(time_span.count()<time_out)
                 {
                    t = high_resolution_clock::now();
      	            pthread_mutex_lock(&gen_lock);
      	            if(timer[seq_num].second.first == 0)
      		    {  
      		 	pthread_mutex_unlock(&gen_lock);
      		        goto end;
      		    }
                    time_span = t-timer[seq_num].first;
                    pthread_mutex_unlock(&gen_lock);
                 }

                 //it means timeout had occurred we have to resend all the things
                 pthread_mutex_lock(&gen_lock);
                 if(timer[seq_num].second.first != 0)
                 { 
          	       send_packets.erase(seq_num);
          	       //seq_num,retransmitt_count,packet_length
          	       send(seq_num,timer[seq_num].second.first+1,timer[seq_num].second.second);
                 }
                 pthread_mutex_unlock(&gen_lock);
              	 goto end;
              }
              
              send(seq_num,1,send_buffer[curr_index].second);
	            curr_index++;
              
	          pthread_mutex_unlock(&gen_lock);
         }
      else{
   	     
   	      double time_out = 100*1000;
   	      if(num_ack > 10)
   	  	      time_out = 2*rtt_avg;
          
       
          int seq_num = -1;
          for(int i = 0;i<window_size;i++)
             if(timer[i].second.first != 0)
             {
                  seq_num = i;
                  break;
             }
          if(seq_num == -1)
          {
              pthread_mutex_unlock(&gen_lock);
            goto end;
          }
          high_resolution_clock::time_point t = high_resolution_clock::now();
          duration<double, std::micro> time_span = t-timer[seq_num].first;
          int high = time_span.count();

          for(int i = 0;i<window_size;i++)
          {   if(timer[i].second.first != 0)
              {

          	  time_span = t-timer[i].first;
          	  if(high<time_span.count())
          	  {
          	  	 high = time_span.count();
          	  	 seq_num = i;
          	  }
             }
          }
                   

          t = high_resolution_clock::now();
          time_span = t-timer[seq_num].first;
        
          pthread_mutex_unlock(&gen_lock);

          while(time_span.count()<time_out)
          {
      	     t = high_resolution_clock::now();
      	     pthread_mutex_lock(&gen_lock);
      	     if(send_packets.size()<window_size)
      		   {  
      		 	    pthread_mutex_unlock(&gen_lock);
      		      goto end;
      		   }
             time_span = t-timer[seq_num].first;
             pthread_mutex_unlock(&gen_lock);
          }

          //it means timeout had occurred we have to resend it
          pthread_mutex_lock(&gen_lock);
          if(send_packets.size() == window_size)
          {  
          	 //retransmit this seq_num
          	 send_packets.erase(seq_num);
          	 send(seq_num,timer[seq_num].second.first+1,timer[seq_num].second.second);
          }
          pthread_mutex_unlock(&gen_lock);
      }
   end: ;
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
       else if(input[i] == "-s")
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
       else if(input[i] == "-n")
       {
       	   sequence_num_field = stoi(input[i+1]);
       	   i += 2;
       }
       else if(input[i] == "-L")
       {
       	   max_packet_length = stoi(input[i+1]);
       	   i += 2;
       }
       else if(input[i] == "-R")
       {
       	   packet_gen_rate = stoi(input[i+1]);
       	   i += 2;
       }
       else if(input[i] == "-N")
       {
       	  max_packets = stoi(input[i+1]);
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
       else
       {
       	  cout<<"Error: Invalid option given: "<<input[i]<<"\n";
       	  exit(-1);
       }
   }

   if((options.size() != 8) && (options.size() != 9))
   {
   	  cout<<"Error: Invalid number of arguments given\n";
   	  exit(-1);
   }

}

void initialization()
{
   rtt_avg = 0;
   curr_index = 0;
   for(int i = 0;i<window_size;i++)
   {
      timer[i] = {high_resolution_clock::now(),{0,0}};
   }
   num_ack = 0;
   num_attempts_sum = 0;
   window_size = min(window_size,(1<<(sequence_num_field))-1);
   exit_condition = false;
}



int main(int argc,char* argv[])
{   
    
    if(argc != 18 && argc != 17)
    {
    	cout<<"Invalid_number of options given\n";
    	exit(-1);
    }
	
    string input[17];
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
     initialization();

     pthread_t  a,b,c;
     pthread_create(&a,NULL,recieve,NULL);
     pthread_create(&b,NULL,send_packet,NULL);
     pthread_create(&c,NULL,packet_generator,NULL);

     pthread_join(a,NULL); 
     pthread_join(b,NULL);
     pthread_join(c,NULL);

   //close(sock_fd);

   return 0;
}
