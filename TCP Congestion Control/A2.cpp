#include <bits/stdc++.h>
using namespace std;
using namespace std ::chrono;

// checks whether the string is valid float or not
bool isfloat(const char* arr)
{
	int x = strlen(arr);
	int cnt = 0;
	for(int i = 0;i<x;i++)
	{
       if(i == 0)
       {
         if(!(isdigit(arr[i])))
         {
            if(!(arr[i] == '-' || arr[i] == '+'))
            	return false;
         }
       }
       else if(arr[i] == '.')
       {
       	  cnt++;
       	  if(cnt>1 || (i == 1 && !(isdigit(arr[0]))))
       	  	 return false;
       }
       else if(!isdigit(arr[i]))
       	 return false;
	}
	return true;
}

// checks whethet the string is valid number or not
bool isnum(const char* arr)
{
	int x = strlen(arr);
    for(int i = 0;i<x;i++)
	{
       if(i == 0)
       {
         if(!(isdigit(arr[i])))
         {
            if(!(arr[i] == '-' || arr[i] == '+'))
            	return false;
         }
       }
       else if(!isdigit(arr[i]))
       	 return false;
    }
    return true;
}

// a function to take input
void take_input(int argc, char const *argv[],double arr[],int& num_updates,ofstream& output_file)
{
    if(argc != 15)
  { 
    cout<<"Invaid number of arguments given\n";
    bad:  //bad is a label //invalid invocation
      cout<<"error"<<"\n";
    exit(-1);
  }

  char array[][3] = {"-i","-m","-n","-f","-s","-T","-o"};
  int cnt[7] = {0}; // an array which cnts how many times a particular option is used in input
  
  // iterating over all the arguments passed and assigning values to corresponding options provided
  for(int i = 1;i<14;i+=2)
  {  
     bool flag = true;
     for(int j = 0;j<5;j++)
     {  // check for first 5 options all these exhibit common behaviour so are grouped together
        if(strcmp(argv[i],array[j]) == 0)
        {
           if(isfloat(argv[i+1]))
           {
              arr[j] = atof(argv[i+1]);
              cnt[j] += 1; // checks
              flag = false;
              break;
           }
           else
           {
              cout<<"invalid float inputted\n";
              goto bad;
           }
        }
     }
     if(flag)
     {  // check for -T option
        if(isnum(argv[i+1]) && (strcmp("-T",argv[i]) == 0))
        {
           num_updates = atoi(argv[i+1]);
           cnt[5] += 1;
        } // check for -o option
        else if(strcmp("-o",argv[i]) == 0)
        {
          output_file.open(argv[i+1]);
          if(!output_file)
          {  
             cout<<"file didn't open\n";
             goto bad;
          }
          cnt[6] += 1;
        }
        else
        { // Invalid option used
          cout<<"Improper Inputs\n";
          goto bad;
        }
     }
  }
   
  // checks whether each option that is required is given or not
  for(int i = 0;i<7;i++)
  {  
     if(cnt[i] != 1)
     {
       cout<<"Improper Input format\n";
       goto bad;
     }
  }

}

int main(int argc, char const *argv[])
{   
    // declaring the variables
	double arr[5]; // K_i,K_m,K_n,K_f,P_s
	int num_updates;
	ofstream output_file;
	take_input(argc,argv,arr,num_updates,output_file);
    cout<<"Inputs taken properly\n";
    cout<<"K_i: "<<arr[0]<<" K_m: "<<arr[1]<<" K_n: "<<arr[2]<<" K_f: "<<arr[3]<<" P_s: "<<arr[4]<<" num_updates: "<<num_updates<<"\n";
    double K_i = arr[0],K_m = arr[1],K_n = arr[2],K_f = arr[3],P_s = arr[4];
    
    //sender's MSS , setting to 1KB
    double MSS = 1;
    //Reciever Window Size , setting to 1MB
    double RWS = 1024;
    // Congestion Window initialisation
    double C_W = K_i*MSS;
    double C_threshold = 1000000000; //INF
    
    int cnt = 1;
    //writing first value
    output_file<<C_W<<"\n";

    while(1)
    {
        int n = ceil(C_W/MSS); // number of packets sent
	// a  random engine generator
        default_random_engine generator = default_random_engine(system_clock ::now().time_since_epoch().count());
	// bernoulli distribution class
        bernoulli_distribution distribution(P_s);
        bool flag = false;
        for(int i = 0;i<n;i++)
        {
        	if(distribution(generator))
        	{       
			// timeout occured
        		flag = true;
        		break;
        	}
        	else
        	{   // successfull acknowledgement
        	    if(C_W <= C_threshold)
	            {   
			//exponential phase
	            	C_W = min(C_W+K_m*MSS,RWS);
	            }
	            else
	            {  
			// linear phase
	            	C_W = min(C_W+K_n*MSS*(MSS/C_W),RWS);
	            }
	            if((cnt++)>=num_updates)
                	goto end;
                output_file<<C_W<<"\n";
        	}
        }
        if(flag)
        {       // setting threshold and updating C_W , because of timeout
        	C_threshold = C_W/2;
        	C_W = max(1.0,K_f*C_W);
        	if((cnt++)>=num_updates)
                	goto end;
            output_file<<C_W<<"\n";
        }
        continue;
    	end: break;
    }
    cout<<"generated_output_file_successfully\n";
    output_file.close();
	return 0;
}
