#include <bits/stdc++.h>
using namespace std;
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <time.h>
#define MAX_LIMIT 100

                       //global variables to this program
int node_num;                                     //node_number
int nodes_cnt;                                    //total_number of nodes
int sock_fd;                                      // socket file descriptor
int curr_sequence_number;                         //current sequence_number
map<int,pair<int,int> > adj;                       //adjacent pair of costs given in input 
map<int,int> neighbour_cost;                      //neighbour_cost filled by recieve_helloreply
map<int,int> curr_sequence;                       //currsequence number from other routers
vector<int> neighbours;                           //neighbours of node_num
vector<map<int,int> > adj_lsa(100,map<int,int>()); //adjacency list filled by lsa
int INF = 1000000000;                             //INFINITY
int lsa_interval;                                 // interval in seconds at which sendlsa function executes
int spf_interval;                                 // interval in seconds at which spf computation exceutes
int hello_interval;                               // interval in seconds at which sendhello function executes
ofstream f;                                        //output file 
bool flag;                                         // flag when true all threads exit

//I am keeping 2 mutex locks because two threads will try to access this same memory location where one copies and
//others might modify the value while copying

pthread_mutex_t adj_lsa_lock,neighbour_cost_lock; 

//I have used maps instead of arrays for proper space optimisation,
//because of maps we will just store only the neighbours in it instead of every node
//ofcourse the memory access time is log(n) but when the graph is very sparse this optimisation helps us.

//helper function which creates this structure and fill entries when called with a port number

struct sockaddr_in get_sockaddr(int port_number)
{
   // sockaddr creation and filling of its entries
   struct sockaddr_in saddr;

   memset(&saddr,0,sizeof(saddr));
   saddr.sin_family = AF_INET; //IPV4
   saddr.sin_addr.s_addr = INADDR_ANY; // Any address
   saddr.sin_port = htons(port_number); // setting port number

   return saddr;
}

//function to create the socket and bind it
int create_and_bind_socket(int port_number)
{  
   // creation of socket
   if((sock_fd = socket(AF_INET, SOCK_DGRAM, 0))<0)
   {   
   	   cout<<"Cannot create socket for the node:"<<node_num<<endl;
   	   exit(-1);
   }

   //struct sockaddr_in saddr = get_sockaddr(port_number);
   struct sockaddr_in saddr;

   memset(&saddr,0,sizeof(saddr));
   saddr.sin_family = AF_INET; //IPV4
   saddr.sin_addr.s_addr = INADDR_ANY; // Any address
   saddr.sin_port = htons(port_number); // setting port number

   //binding the socket 
   if(bind(sock_fd,(const struct sockaddr* )&saddr,sizeof(saddr))<0)
   {
      cout<<"Bind failed for the node:"<<node_num<<endl;
      exit(-1);
   }
   cout<<"Socket created successfully for node:"<<node_num<<endl;
   return sock_fd;
}


//recieveLSA is a function which is invoked when the recieved message is LSA message
void recieveLSA(char* message,int len)
{  
     //it should transmit the information to all it's neighbours iff the sequence number >= curr_sequence number of the src_id
   // cout<<"Entered recieveLSA function...\n";
    //cout<<len<<"\n";
    //printf("\n%s\n",message);
    string lsa = "LSA ";
    int src_id = 0;
    int sequence_number = 0;
    int num_entries = 0;
    int flag = 0;
    pair<int,int> arr[100];
    int curr = 0;
    int a = 0,b = 0;
    for(int i = 0;i<len;i++)
    {   
       // cout<<src_id<<" "<<sequence_number<<" "<<num_entries<<" "<<message[i]<<" "<<flag<<"\n";
    	if(i<4){
    		assert(lsa[i] == message[i]);
    		continue;
    	}
    	if(flag<=2 && message[i] == ' ')
    	   flag++;
    	else if(flag == 0)
    		src_id = src_id*10+(int)(message[i]-'0');
    	else if(flag == 1)
    	   sequence_number = sequence_number*10+(int)(message[i]-'0');
    	else if(flag == 2)
    		num_entries = num_entries*10+(int)(message[i]-'0');
    	else
    	{
           if(message[i] == ' ')
           {    
           	  if(curr>0 && (curr%2) == 1){
           	  	 arr[curr/2] = {a,b};
           	  	 a = 0;
           	  	 b = 0;
           	  }
           	  curr++;
           }
           else if((curr%2) == 0){
                a = a*10+(int)(message[i]-'0');
           }
           else
               b = b*10+(int)(message[i]-'0');
    	}
    }
    //cout<<"\n"<<curr<<"\n";
    arr[curr/2] = {a,b};
    //cout<<"\n"<<sequence_number<<" "<<src_id<<" "<<num_entries<<" "<<curr_sequence[src_id]<<"\n";

    if(sequence_number < curr_sequence[src_id])
    	return;
    assert((curr+1) == (2*num_entries));
    curr_sequence[src_id] = sequence_number;
    
    //use mutex lock here , this is shared resource used by this,dijkstra's where it just copies this adj_lsa.
    pthread_mutex_lock(&adj_lsa_lock);
    for(int i = 0;i<=(curr/2);i++)
    {   
        //if(src_id == 0 && arr[i].first == 1)
        //{
          // printf("curr_node: %d:  message: %s cost: %d ",node_num,message,arr[i]);
        //}
    	adj_lsa[src_id][arr[i].first] = arr[i].second; 
    	adj_lsa[arr[i].first][src_id] = arr[i].second;
    }
    pthread_mutex_unlock(&adj_lsa_lock);
     
    char buffer[len+1];
    for(int i = 0;i<len;i++)
      buffer[i] = message[i];
    buffer[len] = 0;
    
   // cout<<"In recieve LSA sizeof messsage:"<<sizeof(buffer)<<"\n";
    for(auto i : neighbours)
    {
    	if(i != src_id)
    	{
    	  struct sockaddr_in clientaddr = get_sockaddr(10000+i);
         
         sendto(sock_fd,(const char* )buffer,sizeof(buffer),MSG_CONFIRM,(const struct sockaddr* )&clientaddr,sizeof(clientaddr));
        //forwarding this packet to all the neighbours except to source_id
    	}
    }
}

//recieveHello

void recieveHello(char* message,int len)
{   
//     printf("\nRecieved *****message: %s\n",message);
	int src_id = 0;
	char hello[] = "HELLO ";
        //cout<<len<<"\n";
	for(int i = 0;i<len;i++)
        {       
                
		if(i<6)
			assert(hello[i] == message[i]);
		else
		    src_id = src_id*10+((int)(message[i]-'0'));
        }
    
    //assert(src_id<nodes_cnt && src_id >= 0);
    int low = adj[src_id].first,high = adj[src_id].second;
	  int cost = low + rand()%(abs(high-low)+1);

    // this is the cost that we have to send to the src_id

    string reply = "HELLOREPLY ";
    reply = reply + to_string(node_num);
    reply.push_back(' ');
    reply = reply + to_string(src_id);
    reply.push_back(' ');
    reply = reply+to_string(cost);
    int gg = reply.length();
    char buffer[gg+1];
    for(int i = 0;i<gg;i++)
        buffer[i] = reply[i];
    buffer[gg] = 0;
    struct sockaddr_in clientaddr = get_sockaddr(10000+src_id);
    
    sendto(sock_fd,(const char* )buffer,sizeof(buffer),MSG_CONFIRM,(const struct sockaddr* )&clientaddr,sizeof(clientaddr));
    //it is sending 'HELLOREPLY node_num src_id cost'
}

//recieveHelloReply
void recieveHelloReply(char* message,int len)
{   
	// j is the neighbour from which we got HELLOREPLY packet
   // printf("\nRecieved ******message: %s\n",message);
    int j = 0;
    string helloreply = "HELLOREPLY ";
    int flag = 0;
    int cost = 0;
    for(int i = 0;i<len;i++)
    {   
    	if(i<11)
    		assert(message[i] == helloreply[i]);
    	else
    	{ 
    		if(flag == 0){
    			if(message[i] == ' ')
    				flag++;
    			else
    				j = j*10+(int)(message[i]-'0');
    		}
    		else if(flag == 1)
    		{	if(message[i] == ' ')
    				flag++;
                }
    		else
    			cost = cost*10+(int)(message[i]-'0');
    	}
    }
    // updated the cost
    //keep mutex lock here as neighbour_cost is the shared resources used also by send_lsa
    //cout<<j<<"***"<<cost<<"\n";
    pthread_mutex_lock(&neighbour_cost_lock);
    neighbour_cost[j] = cost;
    pthread_mutex_unlock(&neighbour_cost_lock);
}


// this thread runs all the time it recieves all the messages that it will be getting
// if it recieves hello message then it calls recievehello functiom
// if it recieves LSA message then it calls recieve lsa function
// if it recieves hello reply message then it calls recievehello function
void* recieve(void* arg)
{   
	while(1)
	{   
            if(flag)
               break;
	    struct sockaddr_in temp;
	    memset(&temp,0,sizeof(temp));
	    int n;
            unsigned int len = sizeof(temp);
	    char buffer[MAX_LIMIT];
	    n = recvfrom(sock_fd,(char*)buffer,MAX_LIMIT,MSG_WAITALL,(struct sockaddr*)&temp,&len);
            //if(node_num == 0 || node_num == 1)
            //printf("for node %d: \n recieved message: %s\n",node_num,buffer);
            n--;
	    if(buffer[0] == 'L')
	    {	
               //cout<<"Calling recieveLSA..\n";
               recieveLSA(buffer,n);
              
            }
	    else if(buffer[0] == 'H')
	    {
	    	if((n>=6) && (buffer[5] == 'R'))
	    		recieveHelloReply(buffer,n);
	    	else
	    		recieveHello(buffer,n);
	    }
	    else
	    {
	    	cout<<"Improper input recieved for node:"<<node_num<<"\n exitting...!!!"<<endl;
	    	printf("%s\n",buffer);
                exit(-1);
	    }
	}
	pthread_exit(NULL);
}

//sendHello a thread
void* sendHello(void* arg)
{
	while(1)
	{       
                if(flag)
                      break;
		//int milliseconds = 1000*hello_interval;
                //clock_t start_time = clock();
                //while(clock() < (start_time+milliseconds));
                sleep(hello_interval);
              //  cout<<"Started sendHello"<<"\n";
		for(auto it:neighbours)
		{
			if(it>node_num)
			{
			       string reply = "HELLO ";
			       reply += to_string(node_num);
			       int gg = reply.length();
			       char buffer[gg+1];
			       for(int i = 0;i<gg;i++)
				    buffer[i] = reply[i];
			       buffer[gg] = 0;
			       struct sockaddr_in clientaddr = get_sockaddr(10000+it);
			       sendto(sock_fd,(const char* )buffer,sizeof(buffer),MSG_CONFIRM,(const struct sockaddr* )&clientaddr,sizeof(clientaddr));
			}
		}
	}
}

//sendlsa a thread

void* sendlsa(void* arg)
{  
   // intialisation
   curr_sequence_number = 0;
   while(1)
   {      
          if(flag)
                break;
         // int milliseconds = 1000*lsa_interval;
          //clock_t start_time = clock();
          //while(clock() < (start_time+milliseconds));
          sleep(lsa_interval);
   	  //copying the neighbours cost information that is avaialable at this point
   	  //and doing the linkstate advertisement 

      //add mutex lock here
     
      pthread_mutex_lock(&neighbour_cost_lock);
   	  map<int,int> curr_neighbour_cost = neighbour_cost;
      pthread_mutex_unlock(&neighbour_cost_lock);
      //add mutex unlock here
   	  //gather all the cost this is having and also update the adj_lsa here only
      int num_entries = curr_neighbour_cost.size();
      if(num_entries>0)
      {
      	 string reply = "LSA ";
      	 reply += to_string(node_num);
      	 reply.push_back(' ');
      	 reply += to_string(curr_sequence_number);
      	 reply.push_back(' ');
      	 reply += to_string(num_entries);
      	 for(auto it:curr_neighbour_cost)
         {
         	reply.push_back(' ');
         	reply += to_string(it.first);
         	reply.push_back(' ');
         	reply += to_string(it.second);
         	//adding to lsa_adj these costs
          //use mutex lock here , this is shared resource used by this,dijkstra's where it just copies this adj_lsa.
          pthread_mutex_lock(&adj_lsa_lock);
         	adj_lsa[node_num][it.first] = it.second;
         	adj_lsa[it.first][node_num] = it.second;
          //add mutex unlock here
          pthread_mutex_unlock(&adj_lsa_lock);
         }
         int gg = reply.length();
         char buffer[gg+1];
         for(int i = 0;i<gg;i++)
	 buffer[i] = reply[i];
         buffer[gg] = 0;
         for(auto ii : neighbours){
         	struct sockaddr_in clientaddr = get_sockaddr(10000+ii);
            sendto(sock_fd,(const char* )buffer,sizeof(buffer),MSG_CONFIRM,(const struct sockaddr* )&clientaddr,sizeof(clientaddr));
         }
      }
      curr_sequence_number++;
   }
   pthread_exit(NULL);
}

//dijkstras
void* dijkstras(void* arg)
{   
	int cnt = 1;
	while(1)
	{       
                if(cnt == 11)
                {
                   flag = true;
                   break;
                }
                //int milliseconds = 1000*spf_interval;
                //clock_t start_time = clock();
                //while(clock() < (start_time+milliseconds));
                sleep(spf_interval);
		//storing the lsa_adj available at this point
                //add mutex lock here
    
                pthread_mutex_lock(&adj_lsa_lock);
		vector<map<int,int>> temp = adj_lsa;
                //add mutex unlock here
                pthread_mutex_unlock(&adj_lsa_lock);
                //cout<<"doing dijkstras\n";
		vector<pair<int,int>> adj[nodes_cnt];
		int parent[nodes_cnt]; 
		int dist[nodes_cnt];
		set<pair<int,int>> S;
		for(int i = 0;i<nodes_cnt;i++)
		{
			for(auto it : temp[i])
				adj[i].push_back(it); // pushing {neighbour,cost}
			parent[i] = i;
			if(i != node_num)
				dist[i] = INF;
			else
				dist[i] = 0;
			S.insert({dist[i],i});
		}

		while(S.size()>0)
		{
		   pair<int,int> i = *(S.begin());
		   S.erase(i);
		   int cost = i.first;
		   int vertex = i.second;
                   for(auto it : adj[vertex])
                   {
           	        if((cost+it.second) < dist[it.first])
           	        {
           	  	  // Relax the edge
           	  	  S.erase({dist[it.first],it.first});
           	  	  dist[it.first] = cost+it.second;
           	  	  parent[it.first] = vertex;
           	  	  S.insert({dist[it.first],it.first});
           	       }
                   }
		}
		// the following table produce proper output with formatting for node_cnt < 10 
		f<<"|----------------------------------------------|\n";
        f<<"     Routing table for Node no:"<<node_num<<" at Time"<<cnt*spf_interval<<"      |\n";
        f<<"|----------------------------------------------|\n";
        f<<"|-Destination|------------PATH----------|-Cost-|\n";
        for(int i = 0;i<nodes_cnt;i++)
        {
           if(i != node_num)
           {
           	  f<<"     "<<i<<"       |";
           	  int t = dist[i];
           	  if(t != INF){
		           	  stack<int> path;
		           	  int curr = i;
		           	  while(parent[curr] != curr)
		           	  {
		           	  	 path.push(curr);
		           	  	 curr = parent[curr];
		           	  }
		           	  path.push(node_num);
		              int g = 28-2*path.size()-1;
		              for(int ii = 0;ii<(g/2);ii++)
		              	f<<"-";
		              while(!path.empty())
		              {
		              	f<<path.top()<<"-";
		                path.pop();
		              }
		              for(int ii = 0;ii<((g+1)/2-1);ii++)
		              	f<<"-";
		              f<<'|';
		              int num_digits = 0;
		              while(t>0)
		              {
		              	 t = t/10;
		              	 num_digits++;
		              }
		              g = 6-num_digits;
		              for(int ii = 0;ii<(g/2);ii++)
		              	f<<"-";
		              f<<dist[i];
		              for(int ii = 0;ii<((g+1)/2);ii++)
		              	f<<"-";
                              f<<"|\n";
              }
              else
              {
                 f<<"                          |"<<" INFI |\n";
              }
           }
        }
        cnt++;
	}
	pthread_exit(NULL);
}


void input_processing(string input[],ifstream& in)
{
   for(int i = 1;i<=12;i+=2)
   {    
        //cout<<input[i]<<" "<<input[i+1]<<"\n";
        if(input[i] == "-i")
        {
           node_num = stoi(input[i+1]);
        }
        else if(input[i] == "-f")
        {
           in.open(input[i+1]);
        }
        else if(input[i] == "-o")
        {
           f.open(input[i+1]);
        //   cout<<input[i+1]<<"\n";
        }
        else if(input[i] == "-h")
        {
           hello_interval = stoi(input[i+1]);
        }
        else if(input[i] == "-a")
        {
           lsa_interval = stoi(input[i+1]);
        }
        else if (input[i] == "-s")
        {
           spf_interval = stoi(input[i+1]);
        } 
        else
        {
          cout<<"Invalid option "<<input[i]<<",given\n";
          exit(-1);
        }
   }
   
}


int main(int argc,char*argv[])
{ 
  flag = false;
  if(argc != 13)
  {
     cout<<"Invalid number of arguments passed\n";
     exit(-1);
  }
  string input[13];
  for(int i = 0;i<13;i++)
  {  
     int len = strlen(argv[i]);
     for(int jj = 0;jj<len;jj++)
       input[i].push_back(argv[i][jj]);
  }
  ifstream in;
  //processing the input given
  input_processing(input,in);
   
  //bind the socket to the appropriate port,
  //cout<<node_num<<"\n";
  
  sock_fd = create_and_bind_socket(10000+node_num);

  //based on this input file fill map<int,pair<int,int>> adj; for this particular node
  
  int links_count;
  in>>nodes_cnt>>links_count;

  for(int ii = 0;ii<links_count;ii++)
  {
    int i,j,low,high;
    in>>i>>j>>low>>high;
    if(i == node_num)
    {
       adj[j] = {low,high};
       //cout<<i<<" "<<j<<"\n";
       neighbours.push_back(j);
    }
    if(j == node_num)
    {
       adj[i] = {low,high};
       //cout<<j<<" "<<i<<"\n";
       neighbours.push_back(i);
    }
  }
  
   //here call the threads
  pthread_t  a,b,c,d;

  pthread_create(&a,NULL,recieve,NULL);
  pthread_create(&b,NULL,sendHello,NULL);
  pthread_create(&c,NULL,sendlsa,NULL);
  pthread_create(&d,NULL,dijkstras,NULL);

  pthread_join(a,NULL); 
  pthread_join(b,NULL);
  pthread_join(c,NULL);
  pthread_join(d,NULL);
  
  f.close();
  close(sock_fd);
  return 0;
}
