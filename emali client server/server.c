#include <dirent.h>
#include <netdb.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


void network_interface(int client_socket,char* send_buffer)
{
    int t = strlen(send_buffer);
    char message_len[8];
    char temp[8];
    memset(temp,0,sizeof(temp));
    sprintf(temp,"%d",t);

    for(int i = 0;i<(8-strlen(temp));i++)
    {
       message_len[i] = '0';
    }

    strcpy(message_len+(8-strlen(temp)),temp);

    send(client_socket,message_len,8,0);
    send(client_socket,send_buffer,strlen(send_buffer),0);
}


int main(int argc,char* argv[])
{  

    if(argc != 2)
    {
          printf("invalid number of arguments");
    }
   
    int server_socket;
    server_socket = socket(AF_INET,SOCK_STREAM,0);

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(atoi(argv[1]));
    server_address.sin_addr.s_addr = INADDR_ANY;

    bind(server_socket,(struct sockaddr*)&server_address,sizeof(server_address));  
    // referred the following youtube video for network interface basics "https://youtu.be/LtXEMwSG5-8"
   
    int t = listen(server_socket,1);

    int client_socket;
    client_socket = accept(server_socket,NULL,NULL);
    printf("accepted the connection\n");
    
    int cnt = 0;
    char available_users[10000][100];
    int subprompt_user = -1;
    
    memset(available_users,0,sizeof(available_users));

    char send_buffer[1000000];
    char recv_buffer[1000000];
    

    FILE * curr_fp = NULL;
    int global_line_cnt = 0;

    while(1)
    {  
      if(1){

       memset(send_buffer,0,sizeof(send_buffer));
       memset(recv_buffer,0,sizeof(recv_buffer));
       char temp[8];
       memset(temp,0,sizeof(temp));
       int cnt = 0;
       while(cnt<8)
       {
          int a = recv(client_socket,temp+cnt,8-cnt,0);
          cnt += a;
       }
       cnt = atoi(temp);
       
 //      printf("%s\n",temp);
          
       int m_cnt = 0;
       
   //    printf("%d\n",cnt);
       while(m_cnt < cnt)
       {
          int a = recv(client_socket,recv_buffer+m_cnt,cnt-m_cnt,0);
          m_cnt += a;
       }

       printf("recieved message: %s\n",recv_buffer);

      }

       if(strcmp("LSTU",recv_buffer) == 0)
       {
           for(int i = 0;i<cnt;i++)
	   {
	       strcpy(send_buffer+strlen(send_buffer),available_users[i]);
	       send_buffer[strlen(send_buffer)] = ' ';
	   }
	   send_buffer[strlen(send_buffer)] = ' ';
           network_interface(client_socket,send_buffer);
           continue;
       }
       
       if(strcmp("QUIT",recv_buffer) == 0)
       {
          strcpy(send_buffer,"The current session with the client ends. The server closes the connection with the client.");
	  network_interface(client_socket,send_buffer);
          break;
       }

       if(strcmp("READM",recv_buffer) == 0)
       {
          if(fgetc(curr_fp) == EOF)
	  {  
            strcpy(send_buffer,"No More Mail");
	  }
	  else
	  {  
             ungetc('F',curr_fp);
	     int cnt = 0;
	     int i = 0;
	     while(1)
	     { 
	       char c = fgetc(curr_fp);
	       send_buffer[i++] = c;
	       if(c == '\n')
	       {  
		  global_line_cnt++; 
	       }
	       if(c == '#')
		   cnt++;
	       else
	           cnt = 0;
	       if(cnt == 3)
	       {
		 // end of Message 
		 fgetc(curr_fp); //incremting read_pointer by 2 to consume 2 \ns
		 fgetc(curr_fp);
		 global_line_cnt += 2;
		 if(fgetc(curr_fp) == EOF)
		 {  
	              fclose(curr_fp);   
		      // setting to start if it comes to endoffile
		      char filename[100];
                      memset(filename,0,sizeof(filename));
                      strcpy(filename,available_users[subprompt_user]);
                      strcat(filename,".txt");
                      curr_fp = fopen(filename,"r");
		      global_line_cnt = 0;
		  }
		  else
		      ungetc('F',curr_fp);
		  break;
		}
              }
	    }
	  network_interface(client_socket,send_buffer);
          continue;
       }

       if(strcmp("DELM",recv_buffer) == 0)
       { 
          if(fgetc(curr_fp) == EOF)
          {
             strcpy(send_buffer,"No More Mail");
          }
	  else
          {
             FILE* temp = fopen("Temporary_file.txt","w");

	     char filename[100];
             memset(filename,0,sizeof(filename));
             strcpy(filename,available_users[subprompt_user]);
             strcat(filename,".txt");

             FILE* t = fopen(filename,"r");

	     int cnt_1 = 0;

	     while(cnt_1 < global_line_cnt)
	     {
	        char c = fgetc(t);
		if(c == '\n')
	           cnt_1++;
		fputc(c,temp);
	     }
             
             int cnt = 0;
             int i = 0;
             while(1)
             {
               char c = fgetc(curr_fp);
               if(c == '#')
                    cnt++;
               else
                    cnt = 0;
               if(cnt == 3)
               {
                       // end of Message 
                       fgetc(curr_fp); //incremting read_pointer by 2 to consume 2 \ns
                       fgetc(curr_fp);
		       break;
	        }
              } 
	       
	       char c = fgetc(curr_fp);

	       int flag = 0;

               while(c != EOF)
	       {  
		  flag = 1;
	          fputc(c,temp);
		  c = fgetc(curr_fp);
	       }

	       fclose(temp);
	       fclose(curr_fp);

	       remove(filename); // remove the current file
	       //renaming the temporay file created to actual file
	       rename("Temporary_file.txt",filename);

	       curr_fp = fopen(filename,"r");

	       if(flag == 1)
	       {
                 // printf("%d\n",global_line_cnt);
		 // fflush(stdout);
		  cnt = 0;
	          while(cnt<global_line_cnt)
	          {
	         
		      char c = fgetc(curr_fp);
		      if(c == '\n')
			  cnt++;
	          }
	       }
	       else
	       {
	          global_line_cnt = 0;
	        }
             strcpy(send_buffer,"Message Deleted");
	  }
          network_interface(client_socket,send_buffer);
          continue;
       }

       if(strcmp("DONEU",recv_buffer) == 0)
       {  
	  subprompt_user = -1;
          strcpy(send_buffer,"Getting back to main_prompt");
	  if(!curr_fp)
	  {  
	      fclose(curr_fp);
	      curr_fp = NULL;
	  }
          network_interface(client_socket,send_buffer);
          continue;
       }

       const char* p = recv_buffer;
       char command[10],argument[100];
       sscanf(p,"%s %s",command,argument);

       if(strcmp("ADDU",command) == 0)
       {   
	  int flag = 0;
          for(int i = 0;i<cnt;i++)
	  {
	     if(strcmp(available_users[i],argument)==0)
	     {
	       flag = 1;
	       break;
	     }
	  }
	  if(flag == 1)
	  {
	    strcat(argument," is already present");
	    strcpy(send_buffer,argument);
	  }
	  else
	  {
	     if(cnt == 10000)
	         strcpy(send_buffer,"The Max user limit reached");
	     else
	     {  
		 strcpy(available_users[cnt++],argument);
		 char filename[100];
                 memset(filename,0,sizeof(filename));
                 strcpy(filename,available_users[cnt-1]);
                 strcat(filename,".txt");
                 FILE* fp = fopen(filename,"w");
		 fclose(fp);
		 fp = NULL;
		 strcpy(send_buffer,"The user ");
		 strcpy(send_buffer+strlen(send_buffer),argument);
		 strcpy(send_buffer+strlen(send_buffer)," is added Successfully");
             }
	  }
	  network_interface(client_socket,send_buffer);
          continue;
       }

       if(strcmp("SEND",command) == 0)
       { 
	  int to_user = -1;
	  for(int i = 0;i<cnt;i++)
	  {
	     if(strcmp(available_users[i],argument) == 0)
	     {
	        to_user = i;
		break;
	     }
	  }

	  if(to_user == -1)
	  {
	     strcpy(send_buffer,"ERROR: to user doesn't exist");
	  }
	  else
	  {
             time_t t;
	     time(&t);
             char date_time[30];
	     strcpy(date_time,ctime(&t));
	     char mail[1000000];
	     memset(mail,0,sizeof(mail));
	     strcpy(mail,"From: ");
	     strcat(mail,available_users[subprompt_user]);
	     strcat(mail,"\nTo: ");
	     strcat(mail,argument);
	     strcat(mail,"\nDate: ");
	     strcat(mail,date_time);
	     strcat(mail,"Subject: A text message");
	     strcat(mail,"\n");
	     strcat(mail,recv_buffer+6+strlen(argument));
	     strcat(mail,"\n\n");
	     char filename[100];
             memset(filename,0,sizeof(filename));
             strcpy(filename,argument);
             strcat(filename,".txt");
             FILE* fp1 = fopen(filename,"a");
             fputs(mail,fp1);
	     fclose(fp1);
             strcpy(send_buffer,"Message Sent Successfully");
	  }
	  network_interface(client_socket,send_buffer);
          continue;
       }

       if(strcmp("USER",command) == 0)
       {
          for(int i = 0;i<cnt;i++)
	  {
	     if(strcmp(available_users[i],argument) == 0)
	     {
	        subprompt_user = i;
		break;
	     }
	  }
	  if(subprompt_user == -1)
		strcpy(send_buffer,"ERROR: user not available");
	  else
          {
             char filename[100];
             memset(filename,0,sizeof(filename));
             strcpy(filename,argument);
             strcat(filename,".txt");
             curr_fp = fopen(filename,"r");	     
	     strcpy(send_buffer,"user has been set successfully");
	  }
	  network_interface(client_socket,send_buffer);
	  continue;
       }
    
    }

    close(server_socket);

    return 0;
} 
