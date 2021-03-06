/*
 * balance - a balancing tcp proxy
 * $Revision: 3.54 $
 *
 * Copyright (c) 2000-2009,2010 by Thomas Obermair (obermair@acm.org)
 * and Inlab Software GmbH (info@inlab.de), Gruenwald, Germany.
 * All rights reserved.
 *
 * Thanks to Bernhard Niederhammer for the initial idea and heavy
 * testing on *big* machines ...
 *
 * For license terms, see the file COPYING in this directory.
 *
 * This program is dedicated to Richard Stevens...
 *
 *  3.54
 *    fixed hash_fold bug regarding incoming IPv4 and IPv6 source addresses
 *  3.52
 *    thanks to David J. Jilk from Standing Cloud, Inc. for the following:
 *    added "nobuffer" functionality to interactive shell IO
 *    added new "assign" interactive command
 *    fixed locking bug
 *  3.50
 *    new option -6 forces IPv6 bind (hints.ai_family = AF_INET6)
 *  3.49
 *    ftok() patch applied (thanks to Vladan Djeric)
 *  3.48
 *    Problems with setting IPV6_V6ONLY socket option are now handled
 *    more nicely with a syslog warning message
 *  3.42
 *    Balance compiles now on systems where IPV6_V6ONLY is undefined
 *  3.35
 *    bugfix in autodisable code (thanks to Michael Durket)
 *  3.34
 *    syslog logging added (finally)
 *    -a autodisable option added (thanks to Mitsuru IWASAKI)
 *  3.33
 *    SO_KEEPALIVE switched on (suggested and implemented by A. Fluegel)
 *    new option -M to use a memory mapped file instead of IPC shared memory
 *  3.32
 *    /var/run/balance may already exist (thanks to Thomas Steudten)
 *  3.31
 *    TCP_NODELAY properly switched on (thanks to Kurt J. Lidl).
 *  3.30
 *    Code cleanups and fixes (thanks to Kurt J. Lidl)
 *  3.28
 *    Code cleanup's (thanks to Thomas Steudten)
 *    MRTG-Interface (thanks to Brian McCann for the suggestion)
 *  3.26
 *    bugfix: master process was not found with balance -i
 *    unused variable pid removed (BSD)
 *  3.24
 *    bugfix in channel/group argument parsing (thanks to Enrique G. Paredes)
 *    permisions+error messages improvements (thanks to Wojciech Sobczuk)
 *  3.22
 *    writelock and channelcount patch from Stoyan Genov
 *    balance exit codes fix from Chris Wilson
 *    /var/run/balance is tried to be autocreated (if not there)
 *    close of 0,1,2 on background operation
 *  3.19
 *    -h changed to -H
 *  3.17
 *    -h option added
 *    thanks to Werner Maier
 *  3.16
 *    fixed missing save_tmout initialization
 *    thanks to Eric Andresen
 *  3.15
 *    first -B support
 *  3.14
 *    -Wall cleanup
 *  3.12
 *    alarm(0) added, thanks to Jon Christensen
 *  3.11
 *    Bugfix
 *  3.10
 *    Bugfix for RedHat 7.2
 *  3.9
 *    Moved rendezvous file to /var/run and cleaned main(), thanks to
 *    Kayne Naughton
 *  3.8
 *    move to sigaction(), thanks to Kayne Naughton
 *  3.5
 *    Select-Timeout, thanks to Jeff Buhlmann
 *  3.2
 *    Hash groups and some other improvements
 *  2.24:
 *    'channel 2 overload' problem fixed, thanks to Ed "KuroiNeko"
 *  2.26:
 *    'endless loop error' fixed, thanks to Anthony Baxter
 *  2.27:
 *    strcmp on NULL removed, thanks to Jay. D. Allen
 *  2.28:
 *    bsent and breceived now unsigned to avoid negative values,
 *    thanks to Anthony Baxter
 *  2.29:
 *    error in setaddress() fixed, thanks to Dirk Datzert
 *  2.30:
 *    fixing #includes for *bsd compability
 *  2.31:
 *  2.32:
 *    redefied SIGCHLD handling to be compatible with FreeBSD 4.3,
 *    BSD/OS 4.2 and BSD/OS 4.0.1
 *  2.33
 *    finally included SO_REUSEADDR
 *
 */

#include <balance.h>
#include <stdlib.h>
#include "crc32.h"
#include "listener.h"
#include "log.h"
const char *balance_rcsid = "$Id: balance.c,v 3.54 2010/12/03 12:47:10 t Exp $";
static char *revision = "$Revision: 3.54 $";

static int release;
static int subrelease;

static char rendezvousfile[FILENAMELEN];
static int rendezvousfd;
#ifndef	NO_MMAP
static int shmfilefd;
#endif

static int err_dump(char *text) {
  fprintf(stderr, "balance: %s\n", text);
  fflush(stderr);
  exit(EX_UNAVAILABLE);
}

COMMON *common;

static int hashfailover = 0;
static int autodisable = 0;
static int debugflag = 0;
static int foreground = 0;
static int packetdump = 0;
static int interactive = 0;
static int shmmapfile = 0;
static int bindipv6 = 0;

static int sockbufsize = 32768;

static int connect_timeout;
static int current_original_channel=-1;
static char *bindhost = NULL;
static char *outbindhost = NULL;

//CloudAttest
static int content_length = 0;
static int html_end_required = 0;
static int RESPONSE_REPL = 0;
static struct wcache_entry * repl_request;
static struct wcache cache;
unsigned int orig_hash = 0;
unsigned int repl_hash = 0;
static int ncfg = 0; //no. of config entries
static struct timeval sel_tmout  = { 0, 0 }; /* seconds, microseconds */
static struct timeval save_tmout = { 0, 0 }; /* seconds, microseconds */
int resp_fd=-3;
char resp_buff[COMP_SIZE];
char repl_buff[COMP_SIZE];
static int unmatch_count = 0;

//int channel_count;


/*-----------------APIs for Finding Clique at Current State-------------------------------*/
/*
Clique Detection Algorithm
Tested and Working as of 4/27/2011
*/

//#include <stdio.h>
//#include <stdlib.h>
//#include "balance.h"
//COMMON *common from balance.c
//extern int channel_count;
int channel_count;
//int *CL,*cl_b;
//int *malicious;
int malicious[5];
int CL[5][5],cl_b[5][5];
int clcnt,clbcnt,malcnt;
//int graph[4][4] = {0,1,1,0,1,0,0,1,1,0,0,1,0,1,1,0} ; //with 4 cliques - Tested
//int graph[4][4] = {0,1,1,1,1,0,1,1,1,1,0,1,1,1,1,0} ; //with 1 clique - Tested
//int graph[4][4] = {0,0,1,1,0,0,1,0,1,1,0,1,1,0,1,0} ; // with 2 cliques - 1011, 0110 -Tested
//int graph[4][4] = {0,1,0,0,1,0,1,1,0,1,0,1,0,1,1,0} ; // with 2 cliques - 1100, 0111 - Tested
//int graph[4][4] = {0,1,1,0,1,0,1,0,1,1,0,1,0,0,1,0} ; // with 2 cliques - 1110, 0011 - Tested
int graph[5][5];
int f[5];
int N[5];


void initialize_array(int a[])
{   
    int i;
 for(i = 0;i<channel_count;i++)
    a[i] = 0;
    
}

void intersection_elements(int N1[], int N2[], int new[])
{ 
        int i,new_cnt = 0;
        for(i = 0;i < channel_count;i ++)
        {
                if(N1[i] == N2[i]) //found intersection between N1 and N2
                {
                    if(N1[i]!=0)
                    {
                        new[i] = 1;
                    }
                    else
                    {
                        new[i] = 0;
                    }
                }

                else
                        new[i] = 0;
        }
} //Tested


int subtract_element(int P[], int i)
{

    if(P[i] != 0)
    {
        P[i] = 0;
        return 1;
    }
    else
        return 0;


} //Tested

int XUnion_element(int R[], int i)
{
        if(R[i] == 0)
        {
                R[i] = 1;
                printf("\n Unioning!");
                return 1;
        }
        else    return 0;
}

void union_element(int R[], int Rnew[], int i)
{
        int j,ncnt = 0;
        for(j=0;j<channel_count;j++)
         {
            if(j == i)
            {
                Rnew[ncnt++] = 1;
            }
        else
        {
            Rnew[ncnt++] = R[j];
        }


         }
        //Rnew[ncnt]  = i;
        printf("\nUnioning R , i to Rnew");
} //Tested


int is_empty(int P[]){
        int i;
        for(i=0;i<channel_count;i++)
        {
                if(P[i] != 0)
                    return 0;
        }
        return 1;
} //Tested


int is_neighbour(int i,int j){

        if(graph[i][j] != 0 && graph[j][i] != 0){
                return 1;
        }
        return 0;
}


void find_neighbor_set(int curr, int N[])
{
        int i,ncnt = 0;
        for(i=0; i<channel_count; i++)
        {
            if(graph[curr][i] == 1)
            {
                N[i] = 1;
            }
            else
            {
                N[i] = 0;
            }

        }

} // Tested

unsigned char find_node_in_clique(int node_j, int clique[][5])
{
        unsigned char flag = 0;
        int i,j;
        for(i=0;i<clbcnt;i++)
        {
//              for(j=0;j<channel_count;j++)
  //            {
    //                if(node_j == clique[i][j])
      //                  flag = 1;
        //      }
                if(clique[i][node_j] == 1){
                        flag =1;
                        break;
                }
        }
        if(flag == 0) //node_j is not present in any of the Cliques cl_b
                return 0;
        return 1;
}



int sizeofR(int R[])
{
int i=0, count=0;
        for(i=0; i<channel_count; i++) {
                if(R[i] != 0 ){
                        count++;
                }
        }
return count;
}

void print(int a[])
{
    int i=0;
    for(i=0; i<channel_count; i++)
        printf("%d ",a[i]);

    printf("\n");
}

int find_next(int P[])
{
        int i = 0;
        for(i=0; i<channel_count; i++)
        {
                if(P[i] == 1)
                {
                    return i;
                }
        }
        return -1;
}


void findMalicious()
{

        int i,j,cnt = 0;
        for(i=0;i<clcnt;i++)
        {
                //To find those maximal cliques with size > Total Nodes in Graph / 2
                cnt = 0;
                for(j=0;j<channel_count;j++)
                {
                        if(CL[i][j] != 0){
                               cnt ++;

                        }

                }
                if(cnt > channel_count/2) // this is valid clique
                {
                        printf("\n\t Current Count: %d for the Clique No: %d",cnt,i);
                       // cl_b[clbcnt][j] = 1; //only this i'th clique will be used later
                        for(j = 0; j < channel_count ; j ++ )
                                cl_b[clbcnt][j] = CL[i][j];
                        clbcnt ++;

                }
        }



        printf("\n The cliques B  are: (CNT: %d) ... Original Count = %d\n",clbcnt,clcnt);
        for(i=0;i<clbcnt;i++){
                for(j=0;j<channel_count;j++){
//                        if(cl_b[i][j]!=-1)
                                printf("\t %d",cl_b[i][j]);
                }
                printf("\n");
        }



        if(clbcnt != 0) // no cl_b cliques
        {
        for(i = 0; i < channel_count ; i++){
                if(!find_node_in_clique(i,cl_b))
                {
                          //if current node in graph was not found in any of cl_b , it is malicious
                          malicious[malcnt++] = 1;
                        printf("\n\t\t\tNode %d is malicious",i);
                }
                else    malicious[malcnt++] = 0;
        }
        }
        else //cl_b is not empty
        {
                //do nothing
        }

}


void FindConsistencyClique(int R[], int P[], int X[], int curr ){


        int i = 0,j,K;
        int current = curr+1;
        printf("\nNew CALL:  ------------------------------------\n");
        printf("Elements in R : ");
        print(R);
    printf("Elements in P : ");
        print(P);
    printf("Elements in X : ");
        print(X);
        //int *Pnew,*Xnew,*N;
    printf("\n Size of: R: %d ; P: %d ; X: %d ",sizeofR(R),sizeofR(P),sizeofR(X));



        if( is_empty(P) && is_empty(X) && (sizeofR(R)>1) )
        {
                //Report R as Maximal Clique
                int j;
                printf("\nInside New call - Forming Clique -------------------------------------------------Current = %d i = %d\n",current,i);
                printf("\nWeGotClique!!!!!!!!!!!!!!!: ");
                for(j=0 ; j<channel_count ; j++)
                {
                        if(R[j]!= 0){
                                CL[clcnt][j] = R[j];

                        }
                        printf("\t %d",CL[clcnt][j]);
                }
        //initialize_array(R);
                clcnt++;
        }
        else
        {
                //Select Pivot Node K
                //K = pivot(P);
                //printf("\n\t\t Got  Pivot: %d",K);
                /*if(K == -1) // error in finding pivot
                {
                        printf("\nreturning as K = -1");
                        return;
                }*/
                while((i=find_next(P)) != -1)//for(i = current; i<channel_count; i++)
                {
                    printf("\nInside For Loop ------------------------------------Current = %d \n",current);
                    printf("i = %d \n",i);
                        //Pnew=(int)malloc(channel_count*sizeof(int));
                        //Xnew=(int)malloc(channel_count*sizeof(int));
                        //N=(int)malloc(channel_count*sizeof(int));

                        //if(K!=P[i] && P[i]!=-1){
                        //if(is_neighbour(K,i)==0)
                        int Pnew[5],Xnew[5],Rnew[5];
               // printf("\n----------------------------Elements in P : ");
               // print(P);
                initialize_array(Rnew);
                initialize_array(Pnew);
                initialize_array(Xnew);
                //printf("\n----------------------------Elements in P : ");
                //print(P);
                                 find_neighbor_set(i,N);

              //  if(sizeofR(P) != 0)
              //  {
                                subtract_element(P,i);
                                printf("\n\t : After Subtraction of %d from P , size of P is: %d",i,sizeofR(P));
                printf("Elements in P: ");
                print(P);
                                //going according to the algorithm... adding Rnew also
                                union_element(R,Rnew,i);

                                printf("\n\t : After Union of R , size of R is: %d \n Elements in Rnew: ",sizeofR(Rnew));
                print(Rnew);
                printf("\n\n Neighbor Set of: %d \n ",i);
                print(N);
                                intersection_elements(P,N,Pnew);
                                printf("Pnew: ");
                                print(Pnew);
                intersection_elements(X,N,Xnew);
                                printf("Xnew: ");
                                print(Xnew);
                                //copy_array(X)
                                printf("\n recursion: Size of: Rnew: %d ; Pnew: %d ; Xnew: %d ; N[%d]:%d",sizeofR(Rnew),sizeofR(Pnew),sizeofR(Xnew),i,sizeofR(N));
                                FindConsistencyClique(Rnew, Pnew, Xnew,i);
                                union_element(X,X,i);
                                printf("\n\t : After UnionofX , size of X is: %d \n After Union of X : ",sizeofR(X));
                print(X);
                //current++;
                //initialize_array(R);
             //   }
                }
                //return;
        }
        printf("\n returning :( ");
        printf("\nAfter The Union -------------------------------------------------Current = %d i = %d\n",current,i);
//      return;
}
                    


void findclique()
{
	int R[5] = {0};
	int P[5] = {1,1,1,1,1};
   	int X[5] ={0};
	int i,j;
	clbcnt = clcnt = malcnt = 0;
	printf("\n\t\tENtering CLIQUE ::CHannel Count is :  %d",channel_count);
 printf("(%d,%d)\t", cmn_graph_ws_get_consistent(common,0,0), cmn_graph_ws_get_inconsistent(common,0,0));
    	for(i = 0;i < channel_count;i ++)
		for(j = 0;j < channel_count;j ++)
			graph[i][j] = 0;

       //initializing all the variables
        clcnt = 0;
        clbcnt = 0;
        malcnt = 0;
        for(i=0;i<channel_count;i++){
                for(j=0;j<channel_count;j++){
                        CL[i][j]= 0;
                        cl_b[i][j]= 0;
                }
		N[i] = 0;
	}


    //converting shared memory graphs to adjacency matrix
    //Web Servers....common->graph_ws[][]
    	printf("\n---------------------------------Current WebServer Graph-------------------------------------\n");
	printf("(%d,%d)\t", cmn_graph_ws_get_consistent(common,0,1), cmn_graph_ws_get_inconsistent(common,0,1));
    	for(i = 0;i<channel_count;i++){
		for(j=0;j<channel_count;j++){
	    		printf("(%d,%d)\t", cmn_graph_ws_get_consistent(common,i,j), cmn_graph_ws_get_inconsistent(common,i,j));
			if(i!=j){
				int c = cmn_graph_ws_get_consistent(common,i,j);
				int ic = cmn_graph_ws_get_inconsistent(common,i,j);
				LOGO("\nc = %d, ic = %d\n",c,ic);	
		
			if( c != 0 )	
			{	graph[i][j] = (c/(c+ic));}
			else
			{ graph[i][j] = 0; }
//graph[i][j] =  cmn_graph_ws_get_consistent(common,i,j) / ( cmn_graph_ws_get_consistent(common,i,j) +  cmn_graph_ws_get_inconsistent(common,i,j));
			}
			else
				graph[i][j] = 0;
		}
		printf("\n");
	}

	printf("\n------------------------------Adjacency Graph for Web Servers is-------------------------------------\n");
	for(i = 0;i < channel_count;i++)
		for(j = 0;j<channel_count;j++)
			printf("%d \t",graph[i][j]);


	FindConsistencyClique(R,P,X,-1);
    	initialize_array(malicious);
    	findMalicious();
    	printf("\n\t\t-----------------Malicious Array for WebServers:--------------------------- \n");
                for(i=0;i<channel_count;i++)
                {
                //if(malicious[i]!=-1)
                printf("\t %d;",malicious[i]);

        }

	
	//initializing all the variables
	clcnt = 0;
	clbcnt = 0;
	malcnt = 0;
	for(i=0;i<channel_count;i++){

		for(j=0;j<channel_count;j++){
			CL[i][j]= 0;
			cl_b[i][j]=0;
		}
		N[i] = 0;
	}

	initialize_array(R);
	initialize_array(X);
	for(i=0;i<channel_count;i++)
		P[i] = 1;


        for(i = 0;i < channel_count;i ++)
                for(j = 0;j < channel_count;j ++)
                        graph[i][j] = 0;
    //converting shared memory graphs to adjacency matrix

    //Application Servers....common->graph_as[][]
        printf("\n---------------------------------Current AppServer Graph-------------------------------------\n");
        for(i = 0;i<channel_count;i++){
                for(j=0;j<channel_count;j++){
                        printf("(%d,%d)\t", cmn_graph_as_get_consistent(common,i,j), cmn_graph_as_get_inconsistent(common,i,j));
                        if(i!=j){
			
			          int c = cmn_graph_as_get_consistent(common,i,j);
                                int ic = cmn_graph_as_get_inconsistent(common,i,j);
                                LOGO("\nc = %d, ic = %d\n",c,ic);

                        if( c != 0 )
                        {       graph[i][j] = (c/(c+ic));}
                        else
                        { graph[i][j] = 0; }

                        }
                        else
                                graph[i][j] = 0;
                }
                printf("\n");
        }

        printf("\n------------------------------Adjacency Graph for Application Servers is-------------------------------------\n");
        for(i = 0;i < channel_count;i++)
                for(j = 0;j<channel_count;j++)
                        printf("%d \t",graph[i][j]);


        FindConsistencyClique(R,P,X,-1);
        initialize_array(malicious);
        findMalicious();
        printf("\n\t\t----------------Malicious Array for ApplicationServers:------------------------- \n");
                for(i=0;i<channel_count;i++)
                {
                //if(malicious[i]!=-1)
                printf("\t %d;",malicious[i]);

        }





}















/*-----------------APIs for Finding Clique at Current State-------------------------------*/

int get_server_index(char *ipaddr)
{
        int i = 0;
        for(i = 0; i < MAXNODES; i++)
        {
                if(strncmp((cmn_topology(common, i)).as, ipaddr, 16) == 0)
                {
                        return i;
                }
        }

        LOGO("Could not get any index%d\n", 0);
	return -1;
}
int create_serversocket(char* node, char* service) {
  struct addrinfo hints;
  struct addrinfo *results;
  int srv_socket, status, sockopton, sockoptoff;

  bzero(&hints, sizeof(hints));
  hints.ai_flags = AI_PASSIVE;

  if(bindipv6) {
    if(debugflag) {
      fprintf(stderr, "using AF_INET6\n");
    }
    hints.ai_family = AF_INET6;
  } else {
    if(debugflag) {
      fprintf(stderr, "using AF_UNSPEC\n");
    }
    hints.ai_family = AF_UNSPEC;
  }
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP;

  status = getaddrinfo(node, service, &hints, &results);
  if(status != 0) {
    fprintf(stderr,"error at getaddrinfo: %s\n", gai_strerror(status));
    fprintf(stderr,"exiting.\n");
    exit(EX_OSERR);
  }

  if(results == NULL) {
    fprintf(stderr,"no matching results at getaddrinfo\n");
    fprintf(stderr,"exiting.\n");
    exit(EX_OSERR);
  }

  srv_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
  if(srv_socket < 0) {
    perror("socket()");
    exit(EX_OSERR);
  }

  sockoptoff = 0;

#if defined(IPV6_V6ONLY)
  status = setsockopt(srv_socket, IPPROTO_IPV6, IPV6_V6ONLY, (char*) &sockoptoff, sizeof(sockoptoff));
  if(status < 0) {
    syslog(LOG_WARNING,"setsockopt(IPV6_V6ONLY=0) failed");
  }
#endif

  sockopton = 1;

  status = setsockopt(srv_socket, SOL_SOCKET, SO_REUSEADDR, (char*) &sockopton, sizeof(sockopton));

  if(status < 0) {
    perror("setsockopt(SO_REUSEADDR=1)");
    exit(EX_OSERR);
  }

  status = bind(srv_socket, results->ai_addr, results->ai_addrlen);
  if(status < 0) {
    perror("bind()");
    exit(EX_OSERR);
  }

  status = listen(srv_socket, SOMAXCONN);
  if(status < 0) {
    perror("listen()");
    exit(EX_OSERR);
  }

  return(srv_socket);
}

/* locking ... */

int a_readlock(off_t start, off_t len) {
  int rc;
  struct flock fdata;
  fdata.l_type = F_RDLCK;
  fdata.l_whence = SEEK_SET;
  fdata.l_start = 0;
  fdata.l_len = 0;
  // fdata.l_sysid=0;
  // fdata.l_pid=0;
repeat:
  if ((rc = fcntl(rendezvousfd, F_SETLKW, &fdata)) < 0) {
    if (errno == EINTR) {
      goto repeat;		// 8-)
    } else {
      perror("readlock");
      exit(EX_OSERR);
    }
  }
  return (rc);
}

void b_readlock(void) {
  a_readlock(0, 0);
}

void c_readlock(int group, int channel) {
  a_readlock(((char *) &(grp_channel(common, group, channel))) -
	     (char *) common, sizeof(CHANNEL));
}

int a_writelock(off_t start, off_t len) {
  int rc;
  struct flock fdata;
  fdata.l_type = F_WRLCK;
  fdata.l_whence = SEEK_SET;
  fdata.l_start = 0;
  fdata.l_len = 0;
  // fdata.l_sysid=0;
  // fdata.l_pid=0;
repeat:
  if ((rc = fcntl(rendezvousfd, F_SETLKW, &fdata)) < 0) {
    if (errno == EINTR) {
      goto repeat;		// 8-)
    } else {
      perror("a_writelock");
      exit(EX_OSERR);
    }
  }
  return (rc);
}

void b_writelock(void) {
  a_writelock(0, 0);
}

void c_writelock(int group, int channel)
{
  a_writelock(((char *) &(grp_channel(common, group, channel))) -
	      (char *) common, sizeof(CHANNEL));
}

int a_unlock(off_t start, off_t len)
{
  int rc;
  struct flock fdata;
  fdata.l_type = F_UNLCK;
  fdata.l_whence = SEEK_SET;
  fdata.l_start = 0;
  fdata.l_len = 0;
  // fdata.l_sysid=0;
  // fdata.l_pid=0;
repeat:
  if ((rc = fcntl(rendezvousfd, F_SETLK, &fdata)) < 0) {
    if (errno == EINTR) {
      goto repeat;		// 8-)
    } else {
      perror("a_unlock");
      exit(EX_OSERR);
    }
  }
  return (rc);
}

void b_unlock(void)
{
  a_unlock(0, 0);
}

void c_unlock(int group, int channel)
{
  a_unlock(((char *) &(grp_channel(common, group, channel))) -
	   (char *) common, sizeof(CHANNEL));
}

void *shm_malloc(char *file, int size)
{
  char *data = NULL;
  key_t key;
  int shmid;

  if(shmmapfile){
#ifndef	NO_MMAP
    char shmfile[FILENAMELEN];

    strcpy(shmfile, file);
    strcat(shmfile, SHMFILESUFFIX);
	if(remove(file) < 0 ){
		printf("\nCannot remove shared file");
	}
	else {
		printf("\nRemoved shared file.");
	}
    shmfilefd = open(shmfile, O_RDWR | O_CREAT, 0644);
    if(shmfilefd < 0) {
      fprintf(stderr, "Warning: Cannot open file `%s', switching to IPC\n", shmfile);
      shmmapfile = 0;
    }
    if(shmmapfile) {
      if(ftruncate(shmfilefd, size) < 0) {
        fprintf(stderr, "Warning: Cannot set file size on `%s', switching to IPC\n", shmfile);
        close(shmfilefd);
        shmmapfile = 0;
      }
    }
    if(shmmapfile) {
      data = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfilefd, 0);
      if(!data || data == MAP_FAILED) {
        fprintf(stderr, "Warning: Cannot map file `%s', switching to IPC\n", shmfile);
        close(shmfilefd);
        shmmapfile = 0;

      }
    }
#endif
  }

  if(!shmmapfile){

#if defined (__SVR4) && defined (__sun)

    /* vdjeric:
       Solaris ftok() causes frequent collisions because it uses
       only the lower 12 bits of the inode number in the 'key'.
       See: http://bugs.opensolaris.org/bugdatabase/view_bug.do?bug_id=4265917
    */

    FILE *rendezvousfp = NULL;
    struct timeval ct;
    long int seed;
    int i;

    if ((rendezvousfp = fdopen(rendezvousfd, "w+")) == NULL) {
      perror("fdopen");
      exit(EX_OSERR);
    }

    if ((fscanf(rendezvousfp, "0x%x\n", &key)) <= 0) {
      gettimeofday(&ct, NULL);
      seed = ct.tv_usec * getpid();
      srand(seed);

      /* Solaris rand() returns values between 0 and 0x7fff,
         so generate key byte by byte */
      key = 0;
      for (i = 0; i < sizeof(key); i++) {
          key = (key << 8) | (rand() & 0xff);
      }

      if(fseek(rendezvousfp, 0, SEEK_SET) == -1) {
        perror("fseek");
        exit(EX_OSERR);
      }
      if (fprintf(rendezvousfp, "0x%08x\n", key) == -1) {
        perror("fprintf");
        exit(EX_OSERR);
      }
      fflush(rendezvousfp);
    }
#else
    if ((key = ftok(file, 'x')) == -1) {
      perror("ftok");
      exit(EX_SOFTWARE);
    }
#endif

    if ((shmid = shmget(key, size, 0644 | IPC_CREAT)) == -1) {
      perror("shmget");
      exit(EX_OSERR);
    }

    data = shmat(shmid, (void *) 0, 0);
    if (data == (char *) (-1)) {
     perror("shmat");
      exit(EX_OSERR);
    }
  }

  return (data);
}

/* readable output of a packet (-p) */

void print_packet(unsigned char *s, int l)
{
  int i, cc;
  cc = 0;
  for (i = 0; i < l; i++) {
    if (isprint(s[i]) && isascii(s[i])) {
      if (s[i] == '\\') {
	printf("\\\\");
	cc += 2;
      } else {
	printf("%c", s[i]);
	cc++;
      }
    } else {
      printf("\\%02X", s[i]);
      cc += 3;
      if (s[i] == '\n') {
	printf("\n");
	cc = 0;
      }
    }
    if (cc > 80) {
      printf("\n");
      cc = 0;
    }
  }
  printf("\n");
}

int getport(char *port)
{
  struct servent *sp;
  sp = getservbyname(port, "tcp");
  if (sp == NULL) {
    return (atoi(port));
  } else {
    return (ntohs(sp->s_port));
  }
}

void setipaddress(struct in_addr *ipaddr, char *string)
{
  struct hostent *hent;
  hent = gethostbyname(string);
  if (hent == NULL) {
    if ((ipaddr->s_addr = inet_addr(string)) == INADDR_NONE) {
      fprintf(stderr, "unknown or invalid address [%s]\n", string);
      exit(EX_DATAERR);
    }
  } else {
    memcpy(ipaddr, hent->h_addr, hent->h_length);
  }
}

void setaddress(struct in_addr *ipaddr, int *port, char *string,
		int default_port, int *maxc)
{
  char *host_string = NULL;
  char *port_string = NULL;
  char *maxc_string = NULL;
  char *dup_string = NULL;
  char *p = NULL;
  char *q = NULL;

  struct hostent *hent;

  if ((dup_string = strdup(string)) == NULL) {
    fprintf(stderr, "strdup() failed\n");
    exit(EX_OSERR);
  }

  host_string = dup_string;
  p = index(dup_string, ':');

  if (p != NULL) {
    *p = '\000';
    port_string = p + 1;
    if ((q = index(port_string, ':')) != NULL) {
      *q = '\000';
      maxc_string = q + 1;
    } else {
      maxc_string = "";
    }
  } else {
    port_string = "";
    maxc_string = "";
  }

  // fix for RedHat 7.0/7.1 choke on strcmp with NULL

  if (port_string != NULL && !strcmp(port_string, ""))
    port_string = NULL;
  if (maxc_string != NULL && !strcmp(maxc_string, ""))
    maxc_string = NULL;

  hent = gethostbyname(dup_string);
  if (hent == NULL) {
    if ((ipaddr->s_addr = inet_addr(dup_string)) == INADDR_NONE) {
      fprintf(stderr, "unknown or invalid address [%s]\n", dup_string);
      exit(EX_DATAERR);
    }
  } else {
    memcpy(ipaddr, hent->h_addr, hent->h_length);
  }

  if (port_string != NULL) {
    *port = getport(port_string);
  } else {
    *port = default_port;
  }

  if (maxc_string != NULL) {
    *maxc = atoi(maxc_string);
  }
  free(dup_string);
}

int setaddress_noexitonerror(struct in_addr *ipaddr, int *port,
			     char *string, int default_port)
{
  char *host_string;
  char *port_string;
  struct hostent *hent;
  host_string = strtok(string, ":");
  port_string = strtok(NULL, ":");
  hent = gethostbyname(string);
  if (hent == NULL) {
    if ((ipaddr->s_addr = inet_addr(string)) == INADDR_NONE) {
      return (0);
    }
  } else {
    memcpy(ipaddr, hent->h_addr, hent->h_length);
  }

  if (port_string != NULL) {
    *port = getport(port_string);
  } else {
    *port = default_port;
  }
  return (1);
}

int readline(int fd, char *ptr, int maxlen)
{
  int n, rc;
  char c;

  for (n = 1; n < maxlen; n++) {
    if ((rc = read(fd, &c, 1)) == 1) {
      *ptr++ = c;
      if (c == '\n') {
	break;
      }
    } else if (rc == 0) {
      if (n == 1) {
	return (0);		// EOF, no data read
      } else {
	break;			// EOF, some data was read
      }
    } else {
      return (-1);		// error
    }
  }
  *ptr = 0;
  return (n);
}

//CloudAttest - Check for Request Head
unsigned char check_request_head(unsigned char *packet){
        if(strstr((char *)packet,"HTTP"))
                return 1;
        return 0;
}

int forward(int fromfd, int tofd, int groupindex, int channelindex)
{
  ssize_t rc;
  unsigned char buffer[MAXTXSIZE];

  rc = read(fromfd, buffer, MAXTXSIZE);

  if (packetdump) {
    printf("\nBuffer Size: %d\n", (int) rc);
    printf("\nThe Packet data in Forward:\n---------------------------------------------------------------------------\n");
   // print_packet(buffer, rc);
  }

  if (rc <= 0) {
    return (-1);
  } else {
    if(check_request_head(buffer))
    {
            //New Request, Enqueue in request queue
            struct wcache_entry *entry = wcache_entry_alloc();
            if(entry == NULL)
            {
                fprintf(stderr, "Out of memory:malloc failed\n");
                return -1;
            }
            entry->http_request = (char *)malloc(rc);
            if(entry->http_request == NULL)
            {
                fprintf(stderr, "Out of memory:malloc failed\n");
                return -1;
            }
            memcpy(entry->http_request, (char *)buffer, rc);
            entry->size = rc;
            entry->is_replicated = do_replication();
            //entry->is_replicated = 1;
	    if(entry->is_replicated) 
	    {
		    if(grp_nchannels(common,groupindex)==1) // CloudAttest if the number of server channels is 1, give error that we cannot replicate :(
		    {
			    perror("Cannot Replicate, servers = 1\n");
			    entry->is_replicated = 0;
		    }
		    else{
			    // while((repl_index=get_replication_index(grp_nchannels(common,groupindex)))== (grp_current(common, groupindex)) );
				LOGO("\n------------------------------------grp_nchannels(common,groupindex) = %d\n",grp_nchannels(common,groupindex));
			    while((entry->repl_index=get_replication_index(grp_nchannels(common,groupindex)))== current_original_channel);
				/*if a request is already pending for this pair of servers, skip this request!*/
			    if(cmn_aplcn_svr_map(common,current_original_channel, entry->repl_index) > 0)
				entry->is_replicated = 0;
			    else 
			    {
				    cmn_aplcn_svr_map(common,current_original_channel, entry->repl_index) = 1;
				    cmn_aplcn_svr_map(common, entry->repl_index, current_original_channel) = 1;
				    LOGO("------->>>>>AS server map [%d][%d]\n", current_original_channel, entry->repl_index);
				    LOGO("------->>>>> The 2 AS servers are: %s and %s \n", cmn_topology(common, current_original_channel).as, 
						    cmn_topology(common, entry->repl_index).as);
				    printf("\n\nBACKWARD Flow to Client ; GOT Index: %d    Rep_Index: %d\n",current_original_channel,entry->repl_index);
			    }
		    }
	    }
            wcache_add(&cache, entry);

    }
    if (writen(tofd, buffer, rc) != rc) {
      return (-1);
    }
    c_writelock(groupindex, channelindex);
    chn_bsent(common, groupindex, channelindex) += rc;
    c_unlock(groupindex, channelindex);
  }
  return (0);
}




//CloudAttest - Parse Response Packet Function
char* parse_response_packet(unsigned char *packet){
        char *needle;
        needle=strstr((char *)packet,"Content-Type:"); //get the pointer to 'Content-Type' in *packet
        if(needle!=NULL){

                while(*needle!='\n')
                        needle++;
                while(isspace(*needle)) //skip the spaces
                        needle++;
                return needle;
        }
        return NULL;
}


int backward(int fromfd, int tofd, int groupindex, int channelindex)
{
  ssize_t rc;
  unsigned char buffer[MAXTXSIZE];
  unsigned char repl_buffer[MAXTXSIZE];
  unsigned char *needle;
  int repl_index, repl_socket;
  int repl_fd; 
  int tmp=-10; 
 
  rc = read(fromfd, buffer, MAXTXSIZE);



  if (1) {
    printf("-< %d\n", (int) rc);
	printf("The RESPONSE Received:\n");
	printf("-------------------------------\n");
//	print_packet(buffer, rc);
  }

  if (rc <= 0) {
    return (-1);
  } else {
  //This says that rc has no ERROR
        if((needle=parse_response_packet(buffer))!=NULL){
                orig_hash = 0;
                if(is_request_replicated(&cache)){
                     repl_request = wcache_remove_first(&cache);  //Cache is the request queue.
		     repl_index = repl_request->repl_index;
                    // dequeue(&cache);
                     //Parse to get content length
                     content_length = parse_content_length((char *)buffer);
                     fprintf(stdout,"\n\nCONTENT-LENGTH of the Packet: %d\n\n",content_length);
		     if(content_length == -1)
                     {
                             fprintf(stdout, "error in parse_content_length");
                             //Content length is not present. So keep searching
                             //for </html>
                             html_end_required = 1;
                            // return -1;
                     }
                     //Writing Response to file
		     tmp = rc-(needle-buffer);
		     file_the_response(needle,tmp,1);

                     /* Calculate incremental hash value of response*/
                     orig_hash = crc32(needle, tmp, orig_hash);
		  
                     RESPONSE_REPL = 1; // set flag
		     content_length -= tmp;
		     if(html_end_required)
		     {
			     if(strstr(buffer, "</html>")){
				     content_length = 0;
				    html_end_required = 0;
				     fprintf(stdout, "Checked with HTML");

			     }

		     }
		   // fprintf(stdout,"\n\nCONTENT-LENGTH: %d\n\n",content_length);

                }
		else 
		{
			wcache_remove_first(&cache); 
			repl_request = NULL;
		}
        }
        else{
		int striphdr = 0;
              if(RESPONSE_REPL){ //not the start if the packet... write directly to the opened file 'resp_fd'
                file_the_response(buffer,rc,0);

		
                content_length -= rc;
                if(html_end_required)
                {
                        if(strstr(buffer, "</html>")){
                                content_length = 0;
				html_end_required = 0;
				 fprintf(stdout, "Checked with HTML");

			}
                
                }
                /* Calculate incremental hash value of response*/
/*		striphdr = rc;
		if(content_length <= 0)
		{
			if(rc - 40 > 0)
				striphdr = rc - 40;
			else
				striphdr = rc - rc /4;
                	printf("rc = %d; strp = %d\n", rc, striphdr);
		}
*/
		orig_hash = crc32(buffer, rc, orig_hash);
/*		if(content_length == 0){
			fprintf(stdout,"\n\nCONTENT LENGTH IS NOW 0\n\n");
		   if(do_replication()==1)
       	           {
                      if(grp_nchannels(common,groupindex)==1) // CloudAttest if the number of server channels is 1, give error that we cannot replicate :(
                      {
                        perror("Cannot Replicate, servers = 1\n");
                      }
                      else{
                        while((repl_index=get_replication_index(grp_nchannels(common,groupindex)))== (grp_current(common, groupindex)) );

			printf("\n\nIn BACKWARD; GOT Index: %d    Rep_Index: %d\n",index,repl_index);

			if( (repl_socket=get_replicated_socket(groupindex,repl_index)) < 0 ){
				fprintf(stdout,"\n\n Error while getting the repl_socket \n\n");
			}
			else {
				fprintf(stdout,"\n\n Replicated Socket = %d \n\n",repl_socket);
			}
			 // stream(newsockfd, groupindex, index, (char *) &cli_addr, clilen);
                      }

                  }
		}  */
              } // end of RESPONSE_REPL
        }




		printf("\n\n\n******------->>>>>>Content length = %d\n\n", content_length);
	        if(content_length == 0){
                     //   fprintf(stdout,"\n\nCONTENT LENGTH IS NOW 0\n\n");
                   if(repl_request->is_replicated)
                   {
                       // while((repl_index=get_replication_index(grp_nchannels(common,groupindex)))== (grp_current(common, groupindex)) );
			
			current_original_channel=-1;
				LOGO("\nReplicated Index: -------- %d\n",repl_index);
                        if( (repl_socket=get_replicated_socket(groupindex,repl_index)) < 0 ){
                                fprintf(stdout,"\n\n Error while getting the repl_socket \n\n");
                        	exit(-1);
				}
                        else {
                                fprintf(stdout,"\n\nReplicated Socket = %d \n\n",repl_socket);
                             //   exit(-1);
                        }
                        if(repl_request)
                        {
                                int retw = 0, hdrflg = 0, partial = 0, tempsum = 0;
				printf("\n**********-------->>>>>>>REPLICATED REQ SENDING\n %s\n", repl_request->http_request);
                                retw = write(repl_socket, repl_request->http_request,
                                                repl_request->size);
                                if(retw < 0)
                                {
                                        fprintf(stderr, "write to replicated\
                                                        socket failed\n");
                                }
				printf("\n---Writing the Response to File---\n---------------------------------------------\n");
                                repl_fd = open("repl_resp.tmp", O_RDWR|O_CREAT|O_TRUNC,
                                                0644);
                                repl_hash = 0;
                                if(repl_fd < 0)
                                        fprintf(stderr, "error opening replicated file\n");
				memset(repl_buffer, 0, sizeof(MAXTXSIZE));
                                while((retw = read(repl_socket, repl_buffer,
                                                        MAXTXSIZE)) > 0)
                                {
					if(!hdrflg)
					{
						printf("\nREPL RESPONSE****************************************\n %s\n", repl_buffer);
						needle = parse_response_packet(repl_buffer);
						printf("NEEDLE : %s\n", needle);
						if(needle){
							hdrflg = 1;
							partial = 1;
						}
					}
					if(partial)
					{
						printf("Content length = %d\n", retw - (needle - repl_buffer));
						tempsum = retw - (needle - repl_buffer);
						write(repl_fd, needle, retw -(needle - repl_buffer));
                                                repl_hash = crc32(needle,
                                                                (retw-(needle -
                                                                        repl_buffer)),
                                                                repl_hash);
						partial = 0;
					}
					else
                                        {
						tempsum+=retw;
						write(repl_fd, repl_buffer, retw);
                                                repl_hash = crc32(repl_buffer,
                                                                retw, repl_hash);
                                        }
					memset(repl_buffer, 0, sizeof(MAXTXSIZE));
                                }
                                close(repl_fd);
				printf("\n***********TEMPSUM = %d\n", tempsum);
                                printf("orig hash = %u\n repl hash = %u\n",
                                                orig_hash, repl_hash);
                                if(orig_hash == repl_hash)
                                {
                                        cmn_graph_ws_set_consistent(common,channelindex,repl_index);  
                                        printf("incrementing consistent for\
                                                        index %d & %d ::c  = %d ic = %d\n",
                                                        channelindex, repl_index, cmn_graph_ws_get_consistent(common,channelindex,repl_index),
							cmn_graph_ws_get_inconsistent(common,channelindex,repl_index));
                                }
                                else
                                {
                                        cmn_graph_ws_set_inconsistent(common,channelindex,repl_index);  
                                        printf("incrementing INconsistent for\
                                                        index %d & %d ::c  = %d ic = %d\n",
                                                        channelindex, repl_index, cmn_graph_ws_get_consistent(common,channelindex,repl_index),
							cmn_graph_ws_get_inconsistent(common,channelindex,repl_index));
                                }
				/*reset applcn server map */	
				if(cmn_aplcn_svr_map(common, channelindex, repl_index) <= 1)
				{
					printf("*************SHOULD PRINT LAST***RESETTING APP SERVER MAP\n");
					cmn_aplcn_svr_hash(common, channelindex) = 0;
					cmn_aplcn_svr_hash(common, repl_index) = 0;
					cmn_aplcn_svr_map(common, channelindex, repl_index) = 0;
					cmn_aplcn_svr_map(common,repl_index, channelindex) = 0;
				}
                        }
                         // stream(newsockfd, groupindex, index, (char *) &cli_addr, clilen);

                  }
                
		int size_ret_resp = 0;
		int size_ret_repl = 0;

		int response_fd = open("response.tmp", O_RDONLY);
		int replc_fd =  open("repl_resp.tmp", O_RDONLY);
		while((size_ret_resp = read(response_fd,resp_buff,COMP_SIZE)) > 0 )	
		{
			//printf("Entering Comparison Loop : Data bytes read : %d\n-----------------------------------------------",size_ret_resp);

			size_ret_repl = read(replc_fd,repl_buff,size_ret_resp);
			
			printf("Entering Comparison Loop : Data bytes read : %d / %d\n-----------------------------------------------",size_ret_resp,size_ret_repl);
			
			if( size_ret_resp > 0 ) {
                        if(memcmp(resp_buff, repl_buff, size_ret_resp) != 0 )
                        {
                               
                                unmatch_count++;
				printf("\nThe respones do not match!:: UNMATCH COUNT : %d",unmatch_count);
                        }
			else{
				printf("\nThe responses Match!");
			} 
                }

		close(response_fd);
		close(replc_fd);

		}	










	}







    if (writen(tofd, buffer, rc) != rc) {
      return (-1);
    }
    c_writelock(groupindex, channelindex);
    chn_breceived(common, groupindex, channelindex) += rc;
    c_unlock(groupindex, channelindex);
  }
  return (0);
}

/*
 * the connection is really established, let's transfer the data
 *  as efficient as possible :-)
 */

void stream2(int clientfd, int serverfd, int groupindex, int channelindex)
{
  fd_set readfds;
  int fdset_width;
  int sr;
  int optone = 1;

  fdset_width = ((clientfd > serverfd) ? clientfd : serverfd) + 1;

  /* failure is acceptable */
  (void) setsockopt(serverfd, IPPROTO_TCP, TCP_NODELAY,
    (char *)&optone, (socklen_t)sizeof(optone));
  (void) setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY,
    (char *)&optone, (socklen_t)sizeof(optone));
  (void) setsockopt(serverfd, SOL_SOCKET, SO_KEEPALIVE,
    (char *)&optone, (socklen_t)sizeof(optone));
  (void) setsockopt(clientfd, SOL_SOCKET, SO_KEEPALIVE,
    (char *)&optone, (socklen_t)sizeof(optone));

  for (;;) {

    FD_ZERO(&readfds);
    FD_SET(clientfd, &readfds);
    FD_SET(serverfd, &readfds);
    /*
     * just in case this system modifies the timeout values,
     * refresh the values from a saved copy of them.
     */
    sel_tmout = save_tmout;

    for (;;) {
      if (sel_tmout.tv_sec || sel_tmout.tv_usec) {
	sr = select(fdset_width, &readfds, NULL, NULL, &sel_tmout);
      } else {
	sr = select(fdset_width, &readfds, NULL, NULL, NULL);
      }
      if ((save_tmout.tv_sec || save_tmout.tv_usec) && !sr) {
	c_writelock(groupindex, channelindex);
	chn_c(common, groupindex, channelindex) -= 1;
	c_unlock(groupindex, channelindex);
	fprintf(stderr, "timed out after %d seconds\n",
		(int) save_tmout.tv_sec);
	exit(EX_UNAVAILABLE);
      }
      if (sr < 0 && errno != EINTR) {
	c_writelock(groupindex, channelindex);
	chn_c(common, groupindex, channelindex) -= 1;
	c_unlock(groupindex, channelindex);
	err_dump("select error");
      }
      if (sr > 0)
	break;
    }

    if (FD_ISSET(clientfd, &readfds)) {
      if (forward(clientfd, serverfd, groupindex, channelindex) < 0) {
	break;
      }
    } else {
      if (backward(serverfd, clientfd, groupindex, channelindex) < 0) {
	break;
      }
    }
  }
  c_writelock(groupindex, channelindex);
  chn_c(common, groupindex, channelindex) -= 1;
  c_unlock(groupindex, channelindex);
  exit(EX_OK);
}

void alrm_handler(int signo) {
}

void usr1_handler(int signo) {
}

void chld_handler(int signo) {
  int status;
  while (waitpid(-1, &status, WNOHANG) > 0);
}

/*
 * a channel in a group is selected and we try to establish a connection
 */

//CloudAttest--
int get_replicated_socket(int groupindex, int index) {

   int startindex;

   int sockfd;
   int i = 0;
	//index = 1;
 // int clientfd;

  struct sockaddr_in serv_addr;
      //  printf("stream rep\n\n");
  startindex = index;
  //clientfd = arg;
  fprintf(stdout,"\n\nCalling get_replicated_socket with %d grp index and %d index\n\n",groupindex, index);

  for(i=0; i<4; i++){
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      err_dump("can't open stream socket");
    }
	fprintf(stdout, "\n\nSOCKET Acquired: %d \n\n",sockfd);
  }

  (void) setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sockbufsize,
      sizeof(sockbufsize));
    (void) setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sockbufsize,
      sizeof(sockbufsize));

  b_readlock();

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =
        chn_ipaddr(common, groupindex, index).s_addr;
    serv_addr.sin_port = htons(chn_port(common, groupindex, index));
	printf("Selecting and Connecting to Replicated server IP = %s\n", inet_ntoa(chn_ipaddr(common, groupindex, index)));
	printf("----------------------------------------------------------------------\n");
 b_unlock();

	LOGO("\nsockfd : -------- %d\n",sockfd);
  if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
	// Error handler
 	fprintf(stdout,"\n\nERROR connecting to the New SOCKET\n\n");
   }
  else {
	fprintf(stdout,"\n\n We got replicated socket : %d \n\n",sockfd);
	return sockfd;
   }

   return -1;
  //stream2(clientfd, sockfd, groupindex, index);
}

int write_output_matrix()
{



}

//CloutAttest - File the Response PER request .. FILE pointer is Global
unsigned char file_the_response(unsigned char *packet, int packet_size,unsigned char flag){
	//we have file-descriptor resp_fd
	if(flag==1) // First response .. truncate file
	{
		if( (close(resp_fd)<0) ) //error
		{
			//could not close the file
			fprintf(stdout,"Closing ERROR\n\n\n");
			return 0;
		}
		else
		{
			if( (resp_fd=(open("response.tmp",O_RDWR|O_TRUNC)))<0)
			{
				//could not open truncated file
				fprintf(stdout,"\n\nError in opening in TRUNC mode\n\n");
				return 0;
			}
			else
			{
				 fprintf(stdout,"\n\nOpened in  TRUNC mode\n\n");

				if(close(resp_fd)<0)
				{
					//could not close the file
					 fprintf(stdout,"\n\nError in Closing the file.\n\n");

					//return 0;
				}
				else
				{  //open file in append mode
			        	if( (resp_fd=(open("response.tmp",O_RDWR|O_APPEND))) < 0)
					{
		                                //could not open file in append mode
                		                 fprintf(stdout,"\n\nError in opening in APPEND mode\n\n");

						return 0;
		                        }
					else
					{
						 fprintf(stdout,"\n\nOpened in APPEND mode\n\n");

						if(write(resp_fd,packet,packet_size)<0) //write here
				                {
				                        //could not write to the file resp_fd
				                        return 0;
				                }
						return 1;

					}


				}
			}
		} // end of inner open else.
		return 1;

	}
	else
	{
		if(write(resp_fd,packet,packet_size)<0) //eror
		{
			//could not write to the file resp_fd
			return 0;
		}
		return 1;

	}
	return 0;

}

//CloudAttest - Find the Content Length of the response Packet : Returns -1 on error.

int parse_content_length(char *packet){
        char *needle;
        int length;
        needle=strstr(packet,"Content-Length:"); //get the pointer to 'Content-Type' in *packet
        if(needle!=NULL){

                while(!isspace(*needle)) //skip the spaces
                {
                        needle++;
                }

                needle++;

            length =  atoi(needle);
            return length;

        }
        return -1;
}


//CloudAttest --- The original stream function
void *stream(int arg, int groupindex, int index, char *client_address,
	     int client_address_size) {
  int startindex;
  int sockfd;
  int clientfd;
  struct sigaction alrm_action;
  struct sockaddr_in serv_addr;

  startindex = index;		// lets keep where we start...
  clientfd = arg;
  printf("In stream..NO REPL\n\n");

  for (;;) {

    if (debugflag) {
      fprintf(stderr, "trying group %d channel %d ... ", groupindex,
	      index);
      fflush(stderr);
    }

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
      err_dump("can't open stream socket");
    }

    (void) setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sockbufsize,
      sizeof(sockbufsize));
    (void) setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sockbufsize,
      sizeof(sockbufsize));

    /*
     *  if -B is specified, balance tries to bind to it even on
     *  outgoing connections
     */

    if (outbindhost != NULL) {
      struct sockaddr_in outbind_addr;
      bzero((char *) &outbind_addr, sizeof(outbind_addr));
      outbind_addr.sin_family = AF_INET;
      setipaddress(&outbind_addr.sin_addr, outbindhost);
      if (bind
	  (sockfd, (struct sockaddr *) &outbind_addr,
	   sizeof(outbind_addr)) < 0) {
      }
    }

    b_readlock();
    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr =
	chn_ipaddr(common, groupindex, index).s_addr;

    serv_addr.sin_port = htons(chn_port(common, groupindex, index));
    b_unlock();
	printf("ORIGNIAL SERVER IP = %s\n", inet_ntoa(chn_ipaddr(common, groupindex, index)));
    alrm_action.sa_handler = alrm_handler;
    alrm_action.sa_flags = 0;	// don't restart !
    sigemptyset(&alrm_action.sa_mask);
    sigaction(SIGALRM, &alrm_action, NULL);
    alarm(connect_timeout);

    if (connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
      if (debugflag) {
	if (errno == EINTR) {
	  fprintf(stderr, "timeout group %d channel %d\n", groupindex,
		  index);
	} else {
	  fprintf(stderr, "connection refused group %d channel %d\n",
		  groupindex, index);
	}
      }

      /* here we've received an error (either 'timeout' or 'connection refused')
       * let's start some magical failover mechanisms
       */

      c_writelock(groupindex, index);
      chn_c(common, groupindex, index)--;
      if(autodisable) {
	if(chn_status(common, groupindex, index) != 0) {
	  if(foreground) {
	    fprintf(stderr, "connection failed group %d channel %d\n", groupindex, index);
	    fprintf(stderr, "%s:%d needs to be enabled manually using balance -i after the problem is solved\n", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
	  } else {
	      syslog(LOG_NOTICE,"connection failed group %d channel %d", groupindex, index);
	      syslog(LOG_NOTICE,"%s:%d needs to be enabled manually using balance -i after the problem is solved", inet_ntoa(serv_addr.sin_addr), ntohs(serv_addr.sin_port));
	  }
	  chn_status(common, groupindex, index) = 0;
	}
      }
      c_unlock(groupindex, index);

      b_readlock();
      for (;;) {
	for (;;) {
	  if (grp_type(common, groupindex) == GROUP_RR || hashfailover == 1) {
	    index++;
	    if (index >= grp_nchannels(common, groupindex)) {
	      index = 0;
	    }
	    if (index == startindex) {
	      index = -1;	// Giveup
	      break;
	    }
	    if (chn_status(common, groupindex, index) == 1 &&
		(chn_maxc(common, groupindex, index) == 0 ||
		 (chn_c(common, groupindex, index) <
		  chn_maxc(common, groupindex, index)))) {
	      break;		// new index found
	    } else {
	      continue;
	    }
	  } else if (grp_type(common, groupindex) == GROUP_HASH) {

	    // If the current group is type hash, we giveup immediately
	    index = -1;
	    break;
	  } else {
	    err_dump("PANIC: invalid group in stream()");
	  }
	}

	if (index >= 0) {
	  // neuer index in groupindex-group found...
	  break;
	} else {
	again:
	  groupindex++;
	  if (groupindex >= MAXGROUPS) {
	    // giveup, index=-1.
	    break;
	  } else {
	    if (grp_type(common, groupindex) == GROUP_RR) {

	      if (grp_nchannels(common, groupindex) > 0) {
		index = grp_current(common, groupindex);
		startindex = index;	// This fixes the "endless loop error"
					// with all hosts being down and one
					// in the last group... (from Anthony Baxter)
	      } else {
		goto again;
	      }
	      break;
	    } else if (grp_type(common, groupindex) == GROUP_HASH) {
	      unsigned int uindex;
	      uindex = hash_fold((char*) &(((struct sockaddr_in6 *) &client_address)->sin6_addr), client_address_size);

	      if (debugflag) {
		fprintf(stderr, "HASH-method: fold returns %u\n", uindex);
              }

	      index = uindex % grp_nchannels(common, groupindex);
	      if (debugflag)
		fprintf(stderr, "modulo %d gives %d\n",
			grp_nchannels(common, groupindex), index);

	      if (chn_status(common, groupindex, index) == 1 &&
		  (chn_maxc(common, groupindex, index) == 0 ||
		   (chn_c(common, groupindex, index) <
		    chn_maxc(common, groupindex, index)))
		  ) {
		break;
	      } else {
		goto again;	// next group !
	      }
	    } else {
	      err_dump("PANIC: invalid group in stream()");
	    }
	  }
	}
      }
      // we drop out here with a new index

      b_unlock();

      if (index >= 0) {
	// lets try it again
	close(sockfd);
	c_writelock(groupindex, index);
	chn_c(common, groupindex, index) += 1;
	chn_tc(common, groupindex, index) += 1;
	c_unlock(groupindex, index);
	continue;
      } else {
	break;
      }

    } else {
      alarm(0);			// Cancel the alarm since we successfully connected
      if (debugflag) {
	fprintf(stderr, "connect to channel %d successful\n", index);
      }
      // this prevents the 'channel 2 overload problem'

      b_writelock();
      grp_current(common, groupindex) = index;
      grp_current(common, groupindex)++;
      if (grp_current(common, groupindex) >=
	  grp_nchannels(common, groupindex)) {
	grp_current(common, groupindex) = 0;
      }
      b_unlock();

      // everything's fine ...

      stream2(clientfd, sockfd, groupindex, index);
      // stream2 bekommt den Channel-Index mit
      // stream2 never returns, but just in case...
      break;
    }
  }

  close(sockfd);
  exit(EX_OK);
}





static
void initialize_release_variables(void)
{
  char *version;
  char *revision_copy;
  char *token;

  if ((revision_copy = (char *) malloc(strlen(revision) + 1)) == NULL) {
    fprintf(stderr, "malloc problem in initialize_release_variables()\n");
  } else {
    strcpy(revision_copy, revision);
    token = strtok(revision_copy, " ");
    token = strtok(NULL, " ");
    version = token != NULL ? token : "0.0";
    release = atoi(version);
    if (strlen(version) >= 3) {
      subrelease = atoi(version + 2);
    } else {
      subrelease = 0;
    }
    free(revision_copy);
  }
}

static
void usage(void)
{
  fprintf(stderr," _           _\n");
  fprintf(stderr,"| |__   __ _| | __ _ _ __   ___ ___\n");
  fprintf(stderr,"| '_ \\ / _` | |/ _` | '_ \\ / __/ _ \\\n");
  fprintf(stderr,"| |_) | (_| | | (_| | | | | (_|  __/\n");
  fprintf(stderr,"|_.__/ \\__,_|_|\\__,_|_| |_|\\___\\___|\n");


  fprintf(stderr, "  this is balance %d.%d\n", release, subrelease);
  fprintf(stderr, "  Copyright (c) 2000-2009,2010\n");
  fprintf(stderr, "  by Inlab Software GmbH, Gruenwald, Germany.\n");
  fprintf(stderr, "  All rights reserved.\n");
  fprintf(stderr, "\n");

  fprintf(stderr, "usage:\n");
  fprintf(stderr, "  balance [-b addr] [-B addr] [-t sec] [-T sec] [-adfpHM] \\\n");
  fprintf(stderr, "          port [h1[:p1[:maxc1]] [!%%] [ ... hN[:pN[:maxcN]]]]\n");
  fprintf(stderr, "  balance [-b addr] -i [-d] port\n");
  fprintf(stderr, "  balance [-b addr] -c cmd  [-d] port\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "  -a        enable channel autodisable option\n");
  fprintf(stderr, "  -b host   bind to specific address on listen\n");
  fprintf(stderr, "  -B host   bind to specific address for outgoing connections\n");
  fprintf(stderr, "  -c cmd    execute specified interactive command\n");
  fprintf(stderr, "  -d        debugging on\n");
  fprintf(stderr, "  -f        stay in foregound\n");
  fprintf(stderr, "  -i        interactive control\n");
  fprintf(stderr, "  -H        failover even if Hash Type is used\n");
  fprintf(stderr, "  -M        use MMAP instead of SHM for IPC\n");
  fprintf(stderr, "  -p        packetdump\n");
  fprintf(stderr, "  -t sec    specify connect timeout in seconds (default=%d)\n", DEFAULTTIMEOUT);
  fprintf(stderr, "  -T sec    timeout (seconds) for select (0 => never) (default=%d)\n", DEFAULTSELTIMEOUT);
  fprintf(stderr, "   !        separates channelgroups (declaring previous to be Round Robin)\n");
  fprintf(stderr, "   %%        as !, but declaring previous group to be a Hash Type\n");

  fprintf(stderr, "\n");
  fprintf(stderr, "examples:\n");
  fprintf(stderr, "  balance smtp mailhost1:smtp mailhost2:25 mailhost3\n");
  fprintf(stderr, "  balance -i smtp\n");
  fprintf(stderr, "  balance -b 2001:DB8::1 80 10.1.1.1 10.1.1.2\n");
  fprintf(stderr, "  balance -b 2001:DB8::1 80\n");
  fprintf(stderr, "\n");

  exit(EX_USAGE);
}

// goto background:

void background(void) {
  int childpid;
  if ((childpid = fork()) < 0) {
    fprintf(stderr, "cannot fork\n");
    exit(EX_OSERR);
  } else {
    if (childpid > 0) {
      exit(EX_OK);		/* parent */
    }
  }
#ifdef BalanceBSD
  setpgid(getpid(), 0);
#else
  setpgrp();
#endif
  if(chdir("/") <0)
    fprintf(stderr, "cannot chdir\n");
  close(0);
  close(1);
  close(2);
}

COMMON *makecommon(int argc, char **argv, int source_port)
{
  int i;
  int group;
  int channel;
  COMMON *mycommon;
  int numchannels = argc - 1;	// port number is first argument

  if (numchannels >= MAXCHANNELS) {
    fprintf(stderr, "MAXCHANNELS exceeded...\n");
    exit(EX_USAGE);
  }

  if ((rendezvousfd = open(rendezvousfile, O_RDWR, 0)) < 0) {
    perror("open");
    fprintf(stderr,"check rendezvousfile permissions [%s]\n",rendezvousfile);
    exit(EX_NOINPUT);
  }

  b_writelock();

  if ((mycommon =
       (COMMON *) shm_malloc(rendezvousfile, sizeof(COMMON))) == NULL) {
    fprintf(stderr, "cannot alloc COMMON struct\n");
    exit(EX_OSERR);
  }
  memset(mycommon, 0, sizeof(COMMON) );
  mycommon->pid = getpid();
  mycommon->release = release;
  mycommon->subrelease = subrelease;

  for (group = 0; group < MAXGROUPS; group++) {
    grp_nchannels(mycommon, group) = 0;
    grp_current(mycommon, group) = 0;
    grp_type(mycommon, group) = GROUP_RR;	// Default: RR
  }

  group = 0;
  channel = 0;

  for (i = 1; i < argc; i++) {
    if (!strcmp(argv[i], "!")) {
      // This is a normal "GROUP_RR"-Type of Group
      if(channel <= 0) {
	err_dump("no channels in group");
      }
      grp_type(mycommon, group) = GROUP_RR;
      group++;
      channel = 0;
      if (group >= MAXGROUPS) {
	err_dump("too many groups");
      }
    } else if (!strcmp(argv[i], "%")) {
      // This is a "GROUP_HASH"
      if(channel <= 0) {
	err_dump("no channels in group");
      }
      grp_type(mycommon, group) = GROUP_HASH;
      group++;
      channel = 0;
      if (group >= MAXGROUPS) {
	err_dump("too many groups");
      }
    } else {
      chn_status(mycommon, group, channel) = 1;
      chn_c(mycommon, group, channel) = 0;	// connections...
      chn_tc(mycommon, group, channel) = 0;	// total connections...
      chn_maxc(mycommon, group, channel) = 0;	// maxconnections...
      setaddress(&chn_ipaddr(mycommon, group, channel),
		 &chn_port(mycommon, group, channel),
		 argv[i],
		 source_port, &chn_maxc(mycommon, group, channel));
      chn_bsent(mycommon, group, channel) = 0;
      chn_breceived(mycommon, group, channel) = 0;

      grp_nchannels(mycommon, group) += 1;
      channel++;
      if (channel >= MAXCHANNELS) {
	err_dump("too many channels in one group");
      }
    }
  }

  if (debugflag) {
    fprintf(stderr, "the following channels are active:\n");
    for (group = 0; group <= MAXGROUPS; group++) {
      for (i = 0; i < grp_nchannels(mycommon, group); i++) {
	fprintf(stderr, "%3d %2d %s:%d:%d\n",
		group,
		i,
		inet_ntoa(chn_ipaddr(mycommon, group, i)),
		chn_port(mycommon, group, i),
		chn_maxc(mycommon, group, i));
      }
    }
  }
	
  channel_count=grp_nchannels(mycommon, group);	
	fprintf(stdout,"No of channels: %d\n",channel_count);
  	//display();
	
	b_unlock();
  return (mycommon);
}

int mycmp(char *s1, char *s2)
{
  int l;
  l = strlen(s1) < strlen(s2) ? strlen(s1) : strlen(s2);
  if (strlen(s1) > strlen(s2)) {
    return (!1);
  } else {
    return (!strncmp(s1, s2, l));
  }
}

int shell(char *argument)
{
  int i;
  int currentgroup = 0;
  char line[MAXINPUTLINE];
  char *command;

  // DJJ, Standing Cloud, Inc.
  //    In interactive mode, don't buffer stdout/stderr, so that
  //    other programs can operate balance through I/O streams
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);

  if (common->release == 0) {
    printf("no master process, exiting.\n");
    exit(EX_UNAVAILABLE);
  }

  if (common->release != release || common->subrelease != subrelease) {
    printf("release mismatch, expecting %d.%d, got %d.%d, exiting.\n",
	   release, subrelease, common->release, common->subrelease);
    exit(EX_DATAERR);
  }

  if (kill(common->pid, SIGUSR1) == -1) {
    printf("no master process with pid %d, exiting.\n", common->pid);
    exit(EX_UNAVAILABLE);
  }

  if (argument == NULL) {
    printf("\nbalance %d.%d interactive command shell\n", release,
	   subrelease);
    printf("PID of master process is %d\n\n", common->pid);
  }

  for (;;) {

    if (argument == NULL) {
      printf("balance[%d] ", currentgroup);
      if (fgets(line, MAXINPUTLINE, stdin) == NULL) {
	printf("\n");
	exit(EX_OK);
      }
    } else {
      strncpy(line, argument, MAXINPUTLINE);
    }

    if ((command = strtok(line, " \t\n")) != NULL) {
      if (mycmp(command, "quit")) {
	exit(EX_OK);
      } else if (mycmp(command, "show")) {
	b_readlock();
	{
	  int group;

	  printf("%3s %4s %2s %3s %16s %5s %4s %11s %4s %11s %11s\n",
		 "GRP", "Type", "#", "S", "ip-address", "port", "c", "totalc",
		 "maxc", "sent", "rcvd");
	  for (group = 0; group <= MAXGROUPS; group++) {
	    for (i = 0; i < grp_nchannels(common, group); i++) {
	      printf("%3d %4s %2d %3s %16s %5d %4d %11u %4d %11llu %11llu\n",
		     group,
		     grp_type(common, group) == GROUP_RR ? "RR" : "Hash",
		     i,
		     chn_status(common, group, i) == 1 ? "ENA" : "dis",
		     inet_ntoa(chn_ipaddr(common, group, i)),
		     chn_port(common, group, i),
		     chn_c(common, group, i),
		     chn_tc(common, group, i),
		     chn_maxc(common, group, i),
		     chn_bsent(common, group, i),
		     chn_breceived(common, group, i)
		  );
	    }
	  }
	}
	b_unlock();
      } else if (mycmp(command, "help") || mycmp(command, "?")) {
	printf("available commands:\n");

	printf("  create <host> <port>           creates a channel in the current group\n");
        printf("  assign <channel> <host> <port> reassigns a channel in the current group\n");
	printf("  disable <channel>              disables specified channel in current group\n");
	printf("  enable <channel>               enables channel in current group\n");
	printf("  group <group>                  changes current group to <group>\n");
	printf("  hash                           sets distribution scheme of current group to Hash\n");
	printf("  help                           prints this message\n");
	printf("  kill                           kills master process and quits interactive mode\n");
	printf("  maxc <channel> <maxc>          specifies new maxc for channel of current group\n");
	printf("  mrtg-bytes <grp> <ch>          print bytes in/out in MRTG format\n");
	printf("  mrtg-conns <grp> <ch>          print total connections in MRTG format\n");
	printf("  quit                           quit interactive mode\n");
	printf("  reset <channel>                reset all counters of channel in current group\n");
	printf("  rr                             sets distribution scheme of current group to Round Robin\n");
	printf("  show                           show all channels in all groups\n");
	printf("  version                        show version id\n");

      } else if (mycmp(command, "kill")) {
	kill(common->pid, SIGKILL);
	sleep(1);
	if (kill(common->pid, SIGUSR1) == -1) {
	  printf("shutdown complete, exiting.\n");
	  common->release = 0;
	  exit(EX_OK);
	} else {
	  printf("shutdown failed.\n");
	  exit(EX_UNAVAILABLE);
	}
      } else if (mycmp(command, "disable")) {
	char *arg;
	int n;
	if ((arg = strtok(NULL, " \t\n")) != NULL) {
	  n = atoi(arg);
	  if (n < 0 || n >= grp_nchannels(common, currentgroup)) {
	    printf("no such channel %d\n", n);
	  } else {
	    c_writelock(currentgroup, n);
	    if (chn_status(common, currentgroup, n) == 0) {
	      printf("channel %d already disabled\n", n);
	    } else {
	      chn_status(common, currentgroup, n) = 0;
	      printf("channel %d disabled\n", n);
	    }
	    c_unlock(currentgroup, n);
	  }
	} else {
	  printf("syntax error\n");
	}
      } else if (mycmp(command, "group")) {
	char *arg, n;
	if ((arg = strtok(NULL, " \t\n")) != NULL) {
	  n = atoi(arg);
	  if (n >= MAXGROUPS || n < 0) {
	    printf("value out of range\n");
	  } else {
	    currentgroup = n;
	  }
	} else {
	  printf("syntax error\n");
	}

      } else if (mycmp(command, "reset")) {	// reset channel counters
	char *arg;
	int n;

	if ((arg = strtok(NULL, " \t\n")) != NULL) {
	  n = atoi(arg);
	  if (n < 0 || n >= grp_nchannels(common, currentgroup)) {
	    printf("no such channel %d\n", n);
	  } else {
	    c_writelock(currentgroup, n);
	    chn_breceived(common, currentgroup, n) = 0;
	    chn_bsent(common, currentgroup, n) = 0;
	    chn_tc(common, currentgroup, n) = 0;
	    c_unlock(currentgroup, n);
	    printf("channel %d counters reset\n", n);
	  }
	} else {
	  printf("syntax error\n");
	}

      } else if (mycmp(command, "enable")) {

	char *arg;
	int n;
	if ((arg = strtok(NULL, " \t\n")) != NULL) {
	  n = atoi(arg);
	  if (n < 0 || n >= grp_nchannels(common, currentgroup)) {
	    printf("no such channel %d\n", n);
	  } else {
	    c_writelock(currentgroup, n);
	    if (chn_status(common, currentgroup, n) == 1) {
	      printf("channel %d already enabled\n", n);
	    } else {
	      chn_status(common, currentgroup, n) = 1;
	      printf("channel %d enabled\n", n);
	    }
	    c_unlock(currentgroup, n);
	  }
	} else {
	  printf("syntax error\n");
	}

      } else if (mycmp(command, "create")) {
	char *arg1, *arg2;
	b_writelock();
	if (grp_nchannels(common, currentgroup) >= MAXCHANNELS) {
	  printf("no channel slots available\n");
	} else {
	  if ((arg1 = strtok(NULL, " \t\n")) != NULL) {
	    if ((arg2 = strtok(NULL, " \t\n")) != NULL) {
	      chn_status(common, currentgroup,
			 grp_nchannels(common, currentgroup)) = 0;
	      if (setaddress_noexitonerror
		  (&chn_ipaddr
		   (common, currentgroup,
		    grp_nchannels(common, currentgroup)), &chn_port(common,
								    currentgroup,
								    grp_nchannels
								    (common,
								     currentgroup)),
		   arg1, getport(arg2))) {
		chn_bsent(common, currentgroup,
			  grp_nchannels(common, currentgroup)) = 0;
		chn_breceived(common, currentgroup,
			      grp_nchannels(common, currentgroup)) = 0;
		grp_nchannels(common, currentgroup)++;
		printf("channel created\n");
	      } else {
		printf("invalid address\n");
	      }
	    } else {
	      printf("syntax error\n");
	    }
	  } else {
	    printf("syntax error\n");
	  }
	}
	b_unlock();

    } else if (mycmp(command, "assign")) {
        char *arg1, *arg2, *arg3;

          if ((arg1 = strtok(NULL, " \t\n")) != NULL) {
        int chn = atoi(arg1);
            if (chn < 0 || chn >= MAXCHANNELS
                      || chn >= grp_nchannels(common, currentgroup)) {
               printf("unknown channel\n");
        } else {
            c_writelock(currentgroup, chn);
            if (chn_status(common, currentgroup, chn) != 0) {
               printf("channel must be disabled to assign new address\n");
            } else if ((arg2 = strtok(NULL, " \t\n")) != NULL) {
                if ((arg3 = strtok(NULL, " \t\n")) != NULL) {
                   if (setaddress_noexitonerror
                         (&chn_ipaddr(common, currentgroup, chn),
                          &chn_port(common, currentgroup, chn),
                          arg2, getport(arg3))) {
                       printf("channel reassigned\n");
                   } else {
                       printf("invalid address\n");
                   }
                } else {
                   printf("syntax error\n");
                }
            } else {
                printf("syntax error\n");
            }
            c_unlock(currentgroup, chn);
        }
      } else {
        printf("syntax error\n");
      }

    } else if (mycmp(command, "maxc")) {
	char *arg1, *arg2;
	b_writelock();
	if ((arg1 = strtok(NULL, " \t\n")) != NULL) {
	  if ((arg2 = strtok(NULL, " \t\n")) != NULL) {
	    if (atoi(arg1) < 0 || atoi(arg1) >= MAXCHANNELS
		|| atoi(arg1) + 1 > grp_nchannels(common, currentgroup)) {
	      printf("unknown channel\n");
	    } else {
	      chn_maxc(common, currentgroup, atoi(arg1)) = atoi(arg2);
	      printf("maxc of channel %d changed to %d\n", atoi(arg1),
		     atoi(arg2));
	    }
	  } else {
	    printf("syntax error\n");
	  }
	} else {
	  printf("syntax error\n");
	}
	b_unlock();

    } else if (mycmp(command, "mrtg-bytes")) {
	char *arg1, *arg2;
	int mygroup, mychannel;
	b_writelock();
	if ((arg1 = strtok(NULL, " \t\n")) != NULL) {
	  if ((arg2 = strtok(NULL, " \t\n")) != NULL) {
            mygroup = atoi(arg1);
            mychannel = atoi(arg2);
	    if (mygroup < 0 || mygroup > MAXGROUPS) {
	      printf("unknown group\n");
	    } else {
	      if(mychannel < 0 || mychannel > grp_nchannels(common, currentgroup)) {
	        printf("unknown channel\n");
	      } else {
		//
		printf("%llu\n", chn_breceived(common,mygroup,mychannel));
		printf("%llu\n", chn_bsent(common,mygroup,mychannel));
		printf("UNKNOWN\n");
		printf("group %d channel %d\n",mygroup, mychannel);
	      }
	    }
	  } else {
	    printf("syntax error\n");
	  }
	} else {
	  printf("syntax error\n");
	}
	b_unlock();

      } else if (mycmp(command, "mrtg-conns")) {
	char *arg1, *arg2;
	int mygroup, mychannel;
	b_writelock();
	if ((arg1 = strtok(NULL, " \t\n")) != NULL) {
	  if ((arg2 = strtok(NULL, " \t\n")) != NULL) {
            mygroup = atoi(arg1);
            mychannel = atoi(arg2);
	    if (mygroup < 0 || mygroup > MAXGROUPS) {
	      printf("unknown group\n");
	    } else {
	      if(mychannel < 0 || mychannel > grp_nchannels(common, currentgroup)) {
	        printf("unknown channel\n");
	      } else {
		//
		printf("%u\n", chn_tc(common,mygroup,mychannel));
		printf("UNKNOWN\n");
		printf("UNKNOWN\n");
		printf("group %d channel %d\n",mygroup, mychannel);
	      }
	    }
	  } else {
	    printf("syntax error\n");
	  }
	} else {
	  printf("syntax error\n");
	}
	b_unlock();

      } else if (mycmp(command, "version")) {
	printf("  This is balance %d.%d\n", release, subrelease);
	printf("  MAXGROUPS=%d\n", MAXGROUPS);
	printf("  MAXCHANNELS=%d\n", MAXCHANNELS);
      } else if (mycmp(command, "hash")) {
	b_writelock();
	grp_type(common, currentgroup) = GROUP_HASH;
	b_unlock();
	printf("group %d set to hash\n", currentgroup);

      } else if (mycmp(command, "rr")) {
	b_writelock();
	grp_type(common, currentgroup) = GROUP_RR;
	b_unlock();
	printf("group %d set to round robin\n", currentgroup);

      } else {
	printf("syntax error\n");
      }
      // printf("\n");
    }
    if (argument != NULL)
      exit(EX_OK);
  }
}


//CloudAttest
int do_replication()
{
	double num;
	num=((double) rand())/RAND_MAX;
	if(num > 0 && num <=Prob)
	{
		return 1;
	}
	return 0;

}

//CloudAttese
int get_replication_index(int num_grp_channels)
{
	return ((int)rand())%num_grp_channels;
}


//CloudAttest
int server_map_config(void *user_data, char *line)
{
        char *ip;
        int *index = (int *)user_data;
        if(*index > MAXNODES)
        {       
                LOGO("%s", "Config file has too manyy entries\n");
                return -1;
        }
	if(isspace(line[0]) || line[0] == '#') return 0;
        LOGO("Config Line = %s, index = %d\n", line, (*index));
        ip = strtok(line, "  \n");
        strncpy(cmn_topology(common, (*index)).ws, ip, 16);
        LOGO("web server ip = %s\n", cmn_topology(common, (*index)).ws);
        ip = strtok(NULL, "\n ");
        strncpy(cmn_topology(common, (*index)).as, ip, 16);
        LOGO("app server ip = %s\n", cmn_topology(common, (*index)).as);
        (*index)++;
        return 0;
}
char bindhost_address[FILENAMELEN];

int main(int argc, char *argv[])
{
  int startindex;
  int sockfd, newsockfd, childpid;
  unsigned int clilen;
  int c;
  int source_port;
  int fd;
  char *argument = NULL;
  struct stat buffer;
  struct sockaddr_storage cli_addr;
  struct sigaction usr1_action, chld_action;
  struct listener_cfg *cfg = (struct listener_cfg*)malloc(
                  sizeof(struct listener_cfg));
  cfg->listen_port = 8800;
  //index for replication- CloudAttest
  int pid,clique_pid;
  int rep_index,option=0;
#ifdef BalanceBSD
#else
  struct rlimit r;
#endif

///	display();
  //open the file response.tmp for further use
  if( (resp_fd=open("response.tmp",O_CREAT,S_IRWXU)) < 0 ){
	fprintf(stdout, "Initial File open Error." );

	}
else{
  fprintf(stdout, "Initial File opened successfully.");
}
connect_timeout = DEFAULTTIMEOUT;
  initialize_release_variables();

  wcache_list_init(&(cache.l));

	fprintf(stdout, "in mainnnnn\n");
  while ((c = getopt(argc, argv, "c:b:B:t:T:adfpiHM6")) != EOF) {
    switch (c) {
    case '6':
      bindipv6 = 1;
      break;
    case 'a':
      autodisable = 1;
      break;
    case 'b':
      bindhost = optarg;
      break;
    case 'B':
      outbindhost = optarg;
      break;
    case 'c':
      argument = optarg;
      interactive = 1;
      foreground = 1;
      packetdump = 0;
      break;
    case 't':
      connect_timeout = atoi(optarg);
      if (connect_timeout < 1) {
	usage();
      }
      break;
    case 'T':
      sel_tmout.tv_sec = atoi(optarg);
      sel_tmout.tv_usec = 0;
      if (sel_tmout.tv_sec < 1)
	usage();
      save_tmout = sel_tmout;
      break;
    case 'f':
      foreground = 1;
      break;
    case 'd':
      debugflag = 1;
      break;
    case 'p':
      packetdump = 1;
      break;
    case 'i':
      interactive = 1;
      foreground = 1;
      packetdump = 0;
      break;
    case 'H':
      hashfailover = 1;
      break;
    case 'M':
#ifdef	NO_MMAP
      fprintf(stderr, "Warning: Built without memory mapped file support, using IPC\n");
#else
      shmmapfile = 1;
#endif
      break;
    case '?':
    default:
      usage();
    }
  }

  if (debugflag) {
    printf("argv[0]=%s\n", argv[0]);
    printf("bindhost=%s\n", bindhost == NULL ? "NULL" : bindhost);
  }

  if (interactive) {
    foreground = 1;
    packetdump = 0;
  }

  argc -= optind;
  argv += optind;

  if (!interactive) {
    if (argc < 1) {
      usage();
    }
  } else {
    if (argc != 1) {
      usage();
    }
  }
  usr1_action.sa_handler = usr1_handler;
  usr1_action.sa_flags = SA_RESTART;
  sigemptyset(&usr1_action.sa_mask);
  sigaction(SIGUSR1, &usr1_action, NULL);

  chld_action.sa_handler = chld_handler;
  chld_action.sa_flags = SA_RESTART;
  sigemptyset(&chld_action.sa_mask);
  sigaction(SIGCHLD, &chld_action, NULL);
  // really dump core if something fails...

#ifdef BalanceBSD
#else
  getrlimit(RLIMIT_CORE, &r);
  r.rlim_cur = r.rlim_max;
  setrlimit(RLIMIT_CORE, &r);
#endif

  // get the source port

  if ((source_port = getport(argv[0])) == 0) {
    fprintf(stderr, "invalid port [%s], exiting.\n", argv[0]);
    exit(EX_USAGE);
  }

  if (debugflag) {
    fprintf(stderr, "source port %d\n", source_port);
  }

  /*
   * Bind our local address so that the client can send to us.
   * Handling of -b option.
   */

  if (bindhost != NULL) {
    snprintf(bindhost_address, FILENAMELEN, "%s", bindhost);
  } else {
    snprintf(bindhost_address, FILENAMELEN, "%s", "0.0.0.0");
  }

  stat(SHMDIR, &buffer);
  if (!S_ISDIR(buffer.st_mode)) {
    mode_t old = umask(0);
    if (mkdir(SHMDIR, 01777) < 0) {
      if(errno != EEXIST) {
        fprintf(stderr, "ERROR: rendezvous directory not available and/or creatable\n");
        fprintf(stderr, "       please create %s with mode 01777 like this: \n", SHMDIR);
        fprintf(stderr, "       # mkdir -m 01777 %s\n", SHMDIR);
        umask(old);
        exit(EX_UNAVAILABLE);
      }
    }
    umask(old);
  }

  sprintf(rendezvousfile, "%sbalance.%d.%s", SHMDIR, source_port,
	  bindhost_address);

  if (stat(rendezvousfile, &buffer) == -1) {
    // File not existing yet ...
    if ((fd = open(rendezvousfile, O_CREAT | O_RDWR, 0666)) == -1) {
      fprintf(stderr, "cannot create rendezvous file %s\n",
	      rendezvousfile);
      exit(EX_OSERR);
    } else {
      if (debugflag)
	fprintf(stderr, "file %s created\n", rendezvousfile);
      close(fd);
    }
  } else {
    if (debugflag)
      fprintf(stderr, "file %s already exists\n", rendezvousfile);
  }

  if (interactive) {
    // command mode !
    if ((rendezvousfd = open(rendezvousfile, O_RDWR, 0)) < 0) {
      perror("open");
      fprintf(stderr,"check rendezvousfile permissions [%s]\n",rendezvousfile);
      exit(EX_OSERR);
    }
    if ((common =
	 (COMMON *) shm_malloc(rendezvousfile, sizeof(COMMON))) == NULL) {
      fprintf(stderr, "cannot alloc COMMON struct\n");
      exit(EX_OSERR);
    }
    shell(argument);
  }

  openlog("Balance", LOG_ODELAY | LOG_PID | LOG_CONS, LOG_DAEMON);

  /*  Open a TCP socket (an Internet stream socket). */

  sockfd = create_serversocket(bindhost, argv[0]);

  (void) setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &sockbufsize, sizeof(sockbufsize));
  (void) setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &sockbufsize, sizeof(sockbufsize));

  // init of common (*after* bind())

  if (!foreground) {
    background();
  }

  common = makecommon(argc, argv, source_port);

//display();
  /* read the config file which has the one to one mapping of web server ip
   * addresses and application server ip addresses */

        file_parser("server_config.txt", server_map_config, &ncfg);
/*	
	int k=0,l=0;
	for(l=0; l<5; l++)
	{
	for(k=0; k<5; k++)
	{
		LOGO(" %d ",cmn_aplcn_svr_map(common,l,k));

	}
		fprintf(stdout,"\n");
	}      
*/	
 
      clique_pid = fork();
      if(clique_pid == 0)
	{
		while(1){
		printf("\n\t\t FIND CLIQUE? 1/0");
		scanf("%d",&option);
		if(option==1)
			findclique();
		option = 0;
		}
		return 0;	
	}	
	printf("IN PARENT\n");

      pid = fork();
      if(pid == 0)
	{
        connection_handler(cfg);
        return 0;
        }
	printf("IN PARENT \n");

  for (;;) {
    int index;
    unsigned int uindex;
    int groupindex = 0;		// always start at groupindex 0

    clilen = sizeof(cli_addr);

    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
    if (newsockfd < 0) {
      if (debugflag) {
	fprintf(stderr, "accept error %d\n", errno);
      }
      continue;
    }

    if (debugflag) {
      char buf[1024];
      inet_ntop(AF_INET6,&(((struct sockaddr_in6*) &cli_addr)->sin6_addr),buf,1024);
      fprintf(stderr, "connect from %s clilen=%d\n", buf, clilen);
    }

    /*
     * the balancing itself:
     * - groupindex = 0
     * - decision wich channel to use for the first try
     * - client address available in cli_addr
     *
     */

    b_writelock();
    for (;;) {
      index = grp_current(common, groupindex);
      for (;;) {
	if (grp_type(common, groupindex) == GROUP_RR) {
	  if (chn_status(common, groupindex, index) == 1 &&
	      (chn_maxc(common, groupindex, index) == 0 ||
	       (chn_c(common, groupindex, index) <
		chn_maxc(common, groupindex, index)))) {
		current_original_channel=index;
	    break;		// channel found
		
	  } else {
	    index++;
	    if (index >= grp_nchannels(common, groupindex)) {
	      index = 0;
	    }
	    if (index == grp_current(common, groupindex)) {
	      index = -1;	// no channel available in this group
	      break;
	    }
	  }
	} else if (grp_type(common, groupindex) == GROUP_HASH) {
	  uindex = hash_fold((char*) &(((struct sockaddr_in6 *) &cli_addr)->sin6_addr), clilen);

	  if(debugflag) {
	    fprintf(stderr, "HASH-method: fold returns %u\n", uindex);
          }

	  index = uindex % grp_nchannels(common, groupindex);
	  if (debugflag)
	    fprintf(stderr, "modulo %d gives %d\n",
		    grp_nchannels(common, groupindex), index);
	  if (chn_status(common, groupindex, index) == 1
	      && (chn_maxc(common, groupindex, index) == 0
		  || (chn_c(common, groupindex, index) <
		      chn_maxc(common, groupindex, index)))
	      ) {
	    break;		// channel found, channel valid for HASH
	  } else {
	    if (hashfailover == 1) {
	      // if failover even if hash: try next channel in this group.
	      if (debugflag)
		fprintf(stderr, "channel disabled - hashfailover.\n");
	      startindex = index;
	      for (;;) {
		index++;
		if (index >= grp_nchannels(common, groupindex)) {
		  index = 0;
		}
		if (index == startindex) {
		  if (debugflag)
		    fprintf(stderr, "no valid channel in group %d.\n",
			    groupindex);
		  index = -1;
		  break;
		}
		if (chn_status(common, groupindex, index) == 1 &&
		    (chn_maxc(common, groupindex, index) == 0 ||
		     (chn_c(common, groupindex, index) <
		   chn_maxc(common, groupindex, index))) //CloudAttest Made a change here... added this if condition
		   ){//grp_current(common, groupindex) = 0;//CloudAttest ?? What is this change ?? major change here from the original main?
		   if (debugflag)
                    fprintf(stderr, "channel choosen: %d in group %d.\n",
                            index, groupindex);
                  break;        // channel found
                }
              }

            } else {
              if (debugflag)
                fprintf(stderr,
                        "no valid channel in group %d. Failover?\n",
                        groupindex);
              index = -1;
            }
            break;
          }
        } else {
          err_dump("PANIC: invalid group type");
        }
      }

      // Hier fallen wir "raus" mit dem index in der momentanen Gruppe, oder -1
      // wenn nicht moeglich in dieser Gruppe

      grp_current(common, groupindex) = index;
      grp_current(common, groupindex)++;        // current index dieser gruppe wieder null, wenn vorher ungueltig (-1)

      // Der index der gruppe wird neu berechnet und gespeichert, "index" ist immer noch
      // -1 oder der zu waehlende index...

      if (grp_current(common, groupindex) >=
          grp_nchannels(common, groupindex)) {
        grp_current(common, groupindex) = 0;
      }
      //CloudAttest problem with braces :-/
      if (index >= 0) {
	chn_c(common, groupindex, index)++;	// we promise a successful connection
	chn_tc(common, groupindex, index)++;	// also incrementing the total count
	// c++
	break;					// index in this group found
      } else {
	groupindex++;				// try next group !
	if (groupindex >= MAXGROUPS) {
	  break;				// end of groups...
	}
      }
    }

    b_unlock();

    if (index >= 0) {
      if ((childpid = fork()) < 0) {

	// the connection is rejected if fork() returns error,
	// but main process stays alive !

	if (debugflag) {
	  fprintf(stderr, "fork error\n");
	}
      } else if (childpid == 0) {	// child process
	close(sockfd);			// close original socket
	// process the request:
	//CloudAttest - Here we save the index as current_original_channel
//	current_original_channel=index;
	fprintf(stdout,"\n\nBefore Calling Stream with index: %d , we have current_original_channel = %d\n\n",index,current_original_channel);
	stream(newsockfd, groupindex, index, (char *) &cli_addr, clilen);
	exit(EX_OK);
      }
    }

    close(newsockfd);		// parent process
  }
}
int get_ipaddr(char *ipaddr, unsigned char *buffer)
{
        int i = 0;
	buffer++;
	buffer++;
        while(buffer[i] != '#')
        {
                ipaddr[i] = buffer[i];
                i++;
                if(i > MAX_IPADDR_LEN)
                        break;
        }
        ipaddr[i] = '\0';
        LOGO("ipaddr of the AS = %s\n", ipaddr);
        return i+2;
}
int get_replication_status(char *ipaddr, int *other_svr)
{
        int i = 0;
	LOGO("Inside replication status: ipaddr = %s\n", ipaddr);
        int index = get_server_index(ipaddr);
       //int index = -1;
	LOGO("Inside replication status: server index = %d\n", index);    
	
	int k=0;
    
        for(k=0; k<5; k++)
        {
                fprintf(stdout," %d ",cmn_aplcn_svr_map(common,index,k));

        }
        fprintf(stdout,"\n");
                  

	if(index == -1)
                return -1;
        while(i < MAXNODES)
        {
		LOGO("\nInside While :i= %d, (cmn_aplcn_svr_map(common,index,i)= %d\n",i,(cmn_aplcn_svr_map(common,index,i)));
                if((cmn_aplcn_svr_map(common,index,i)) > 0)
                {
                        *other_svr = i; 
                        LOGO("Inside replication status: replcn server index = %d\n", (*other_svr));    
                        return index;
                }
                i++;
        }
        return -1;
}


int aplcn_svr_response_check(int new_fd)
{
#define MAX_BUFFER 4096

                int firstbuf = 0;
                unsigned int hash = 0, other_hash = 0;
                unsigned char buffer[MAX_BUFFER];
                int rc = 0, iplen, index = -1;
                char ipaddr[MAX_IPADDR_LEN];
                int replication_status = 0;
                int other_svr;
		buffer[0] = 0;
                while((rc = read(new_fd, buffer, MAX_BUFFER)) > 0)
                {
                        LOGO("CHECKPOINTING DATA RECEIVED: %s\n", buffer);
                        //first packet 
                        if(firstbuf == 0)
                        {
                                iplen = get_ipaddr(ipaddr, buffer);
                                index = get_replication_status(ipaddr, &other_svr);
                                if(index == -1)
                                {       
                                        //do nothing. Not replicated. Just drop
                                        //these packets
                                        LOGO("%s\n", "NO REPLICATION:Just DROP \
                                                        PACKETS");
                                        replication_status = 0;
                                }
                                else
                                {
                                        hash = crc32(buffer+iplen, rc-iplen, hash);
                                        replication_status = 1;
					LOGO("----------->>>>>> CHECKPOINT data:replicated server pair = %s, %s\n",
						cmn_topology(common, index).as, cmn_topology(common, other_svr).as);
                                        LOGO("first packet of AS DATA = %s\n", buffer+iplen);
                                }
                                firstbuf = 1;
                        }
                        //rest of the packets; interesting only if replication =
                        //1
                        else
                        {
                                if(replication_status)
                                {
                                        	LOGO("----------->>>>>>NEXT CHECKPOINT data:replicated server pair = %s, %s\n",
						cmn_topology(common, index).as, cmn_topology(common, other_svr).as);
						LOGO("----------->>>>>next packets of AS DATA = %s\n", buffer);
                                        hash = crc32(buffer, rc, hash);
                                }
                        
                        }
			buffer[0] = 0;

                }
                if(replication_status)
                {
			LOGO(" *****-------->>>>>>><<<<<<<>>>>>>> index = %d, other_svr = %d, hash1=%u, hash2=%u\n",
				index, other_svr, cmn_aplcn_svr_hash(common, index), cmn_aplcn_svr_hash(common, other_svr));
                        if((cmn_aplcn_svr_hash(common, index) == 0) &&
                                        (cmn_aplcn_svr_hash(common, other_svr) == 0))
                        {
                                //both entries are 0..this is the first hash.
                                //store and wait for second hash
                                cmn_aplcn_svr_hash(common, index) = hash;
                                cmn_aplcn_svr_map(common,index,other_svr) = 2;
                                cmn_aplcn_svr_map(common,other_svr,index) = 2;
                                LOGO("--------->>>> FIRST CHKPOINT:Hash of checkpoint data for first server index %d,ip=%s, value = %u\n", 
                                                index,cmn_topology(common, index).as, cmn_aplcn_svr_hash(common, index));
                        }
                        else
                        {
                               
				LOGO("\n----------We are in APP Comparison Loop!!!-------------- %d %d\n",hash,other_hash);

					 if((other_hash = cmn_aplcn_svr_hash(common, index))
                                                == 0)
                                        other_hash = cmn_aplcn_svr_hash(common,
                                                        other_svr);
                                LOGO("AS orig hash = %u; other hash = %u\n", hash, other_hash);
				if(hash == other_hash)
                                {
                                        cmn_graph_as_set_consistent(common,index,other_svr);  
                                        LOGO("Applcn Server:incrementing consistent for index %d & %d ::c  = %d ic = %d\n",
                                                        index, other_svr,
                                                        cmn_graph_as_get_consistent(common,index,other_svr),
							cmn_graph_as_get_inconsistent(common,index,other_svr));
                                }
                                else
                                {
                                        cmn_graph_as_set_inconsistent(common,index,other_svr);  
                                        LOGO("incrementing INconsistent for index %d & %d ::c  = %d ic = %d\n",
                                                        index, other_svr,
                                                        cmn_graph_as_get_consistent(common,index,other_svr),
							cmn_graph_as_get_inconsistent(common,index,other_svr));
                                }
                                //reset all entries
				LOGO("%s", "RESETTING HASH IN APP SERVER *******------------->>>>>>>>>\n");
                                cmn_aplcn_svr_hash(common, index) = 0;
                                cmn_aplcn_svr_hash(common, other_svr) = 0;
                                cmn_aplcn_svr_map(common,index,other_svr) = 0;
                                cmn_aplcn_svr_map(common,other_svr,index) = 0;
                        }


                }
                return 0;
}
