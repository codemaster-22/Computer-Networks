#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netdb.h>
#include <string.h>
#define BUFFER_SIZE 1000000
#include <arpa/inet.h>
#include <netinet/in.h>

void network_interface(int network_socket,char* send_buffer,char* recv_buffer)
{   
    //send number of bytes , it will be atmax 1e6 so send a 7 digit string including 0's at start if any
    int t = strlen(send_buffer);
    char message_len[8];
    char temp[8];
    memset(temp,0,sizeof(temp));
    sprintf(temp,"%d",t);
    //printf("%d\n",strlen(temp));


    for(int i = 0;i<(8-strlen(temp));i++)
    {
       message_len[i] = '0';
    }

    strcpy(message_len+(8-strlen(temp)),temp);
    //printf("%s\n",message_len);

    send(network_socket,message_len,8,0);
    send(network_socket,send_buffer,strlen(send_buffer),0);
    
    memset(temp,0,sizeof(temp));
    int cnt = 0;
    while(cnt<8)
    {
      int a = recv(network_socket,temp+cnt,8-cnt,0);
      cnt += a;
    }

    cnt = atoi(temp);

    int m_cnt = 0;

    memset(recv_buffer,0,sizeof(recv_buffer));

    while(m_cnt < cnt)
    {
      int a = recv(network_socket,recv_buffer+m_cnt,cnt-m_cnt,0);
      m_cnt += a;
    }
}


int main(int argc , char* argv[])
{
     //  creation of socket
     //
       if(argc != 3)
       {
          printf("invalid number of arguments");
       }

       int network_socket;

       network_socket = socket(AF_INET,SOCK_STREAM,0);

       struct sockaddr_in server_address;
       server_address.sin_family = AF_INET;
       server_address.sin_port = htons(atoi(argv[2]));
       server_address.sin_addr.s_addr = inet_addr(argv[1]);
      
       // referred the following youtube video for network interface basics "https://youtu.be/LtXEMwSG5-8"
       int connection_status = connect(network_socket,(struct sockaddr*)&server_address,sizeof(server_address));

       while(connection_status<0)
       {
        connection_status = connect(network_socket,(struct sockaddr*)&server_address,sizeof(server_address));
       }
    
     printf("Connection established with server\n");
    
    int curr = 0;
    char subprompt_user[100];


    char send_buffer[1000000];
    char recv_buffer[1000000];
   

    while(1)
    {
       memset(send_buffer,0,sizeof(send_buffer));
       memset(recv_buffer,0,sizeof(recv_buffer));

       if(curr == 0)
          printf("Main-Prompt>");
       else
          printf("Sub-Prompt-%s>",subprompt_user);
       
       char response[120];
       scanf("%[^\n]%*c",response);


       if(strcmp("Listusers",response) == 0)
       {
          if(curr != 0)
          {
	     printf("the command Listusers not supported in Sub-Prompt\n");
             continue;
	  }
	  strcpy(send_buffer,"LSTU");
          network_interface(network_socket,send_buffer,recv_buffer);
	  printf("... %s\n",recv_buffer);
	  continue;
       }
      
       if(strcmp("Quit",response) == 0)
       {
             if(curr != 0)
             {
                printf("the command Quit  not supported in Sub-Prompt\n");
                continue;
             }
             strcpy(send_buffer,"QUIT");
	     network_interface(network_socket,send_buffer,recv_buffer);
             printf("... %s\n",recv_buffer);
              break;
       }

       if(strcmp("Read",response) == 0)
       {
             if(curr != 1)
             {
                printf("the command Read not supported in Main-Prompt\n");
                continue;
             }
	     strcpy(send_buffer,"READM");
             network_interface(network_socket,send_buffer,recv_buffer);
             printf("... %s\n",recv_buffer);
             continue;
       }

       if(strcmp("Delete",response) == 0)
       {
             if(curr != 1)
             {
                printf("the command Delete not supported in Main-Prompt\n");
                continue;
             }
             strcpy(send_buffer,"DELM");
             network_interface(network_socket,send_buffer,recv_buffer);
             printf("... %s\n",recv_buffer);
             continue;
       }

       if(strcmp("Done",response) == 0)
       {
             if(curr != 1)
             {
                printf("the command Done not supported in Main-Prompt\n");
                continue;
             }
             strcpy(send_buffer,"DONEU");
             network_interface(network_socket,send_buffer,recv_buffer);
	     curr = 0;
             continue;
       }
       
       int flag = 0;

       for(int i = 0;i<strlen(response);i++)
       {
          if((i != 0) && (response[i] == ' '))
          {
             flag = 1;
             break;
          }
       }

       if(flag == 0)
       {
          printf("Unsupported Command\n");
          continue;
       }

       char command[15],argument[100];

       const char* p = response;

       sscanf(p,"%s %s",command,argument);

       if(strcmp("Adduser",command) == 0)
       {
	     if(curr != 0)
             {
                printf("the command Adduser not supported in Sub-Prompt\n exiting..\n");
                break;
             }
             strcpy(send_buffer,"ADDU ");
	     strcpy(send_buffer+strlen(send_buffer),argument);
//	     printf("%s\n",send_buffer);
             network_interface(network_socket,send_buffer,recv_buffer);
             printf("... %s\n",recv_buffer);
	     continue;
       }


       if(strcmp("SetUser",command)==0)
       {
          if(curr != 0)
          {
            printf("the command SetUser not supported in Sub-Prompt\n exiting..\n");
            break;
          }
          strcpy(send_buffer,"USER ");
	  strcat(send_buffer,argument);
          network_interface(network_socket,send_buffer,recv_buffer);
          if(strcmp(recv_buffer,"ERROR: user not available") != 0)
	  {	
	     curr = 1;
	     strcpy(subprompt_user,argument);
	  }
          printf("... %s\n",recv_buffer);
          continue;
       }

       if(strcmp("Send",command) == 0)
       {
          if(curr != 1)
          {
            printf("the command Send not supported in Main-Prompt\n");
            continue;
          }
          
          printf("Type Message: ");
	  char temp[999980];
	  memset(temp,0,sizeof(temp));
          while(1)
          {
             char* t = temp + strlen(temp);
             scanf("%[^\n]%*c",t);
             int g = strlen(temp);
             if(strcmp(temp+g-3,"###") == 0)
                break;
             temp[g] = '\n';
          }
          strcpy(send_buffer,"SEND ");
	  strcat(send_buffer,argument);
	  strcat(send_buffer," ");
	  strcat(send_buffer,temp);
          network_interface(network_socket,send_buffer,recv_buffer);
          printf("... %s\n",recv_buffer);
          continue;
       }
       
       printf("Unsupported Command\n");
    }
        close(network_socket);
    return 0;
}
