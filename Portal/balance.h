/*
 * balance.h
 */

/*
 * $Id: balance.h,v 1.1 2010/01/29 10:40:16 t Exp $
 */
//EDITED BY SAGAR SANE
#include <stdio.h>
#include <strings.h>
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdlib.h>
#include <sysexits.h>
#include <syslog.h>
#include "wcache.h"
#ifndef	NO_MMAP
#include <unistd.h>
#include <sys/mman.h>
#endif

#ifdef __FreeBSD__
#define BalanceBSD 1
#endif 

#ifdef bsdi
#define BalanceBSD 1
#endif

#ifdef BSD
#define BalanceBSD 1
#endif

#ifdef MAC_OS_X_VERSION_10_4 
#define BalanceBSD 1
#endif

#ifdef BalanceBSD
#include <sys/wait.h>
#else
#include <sys/resource.h>
#endif

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/shm.h>

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>	/* for TCP_NODELAY definition */

/* solaris 9, solaris 10 do not have INADDR_NONE */
#ifndef INADDR_NONE
#define INADDR_NONE ((unsigned long) -1)
#endif


//PROBABILITY For Replication : Sagar Sane
#define Prob 1
#define MAXTXSIZE 	(32*1024)
#define FILENAMELEN 	1024
#define COMP_SIZE 	4095
/*
 * this should be a directory that isn't cleaned up periodically, or at
 * reboot of the machine (/tmp is cleaned at reboot on many OS versions)
 */
#define SHMDIR 		"/var/run/balance/"
#define	SHMFILESUFFIX	".shm"

#define MAXCHANNELS 		64	/* max channels in group          */
#define MAXGROUPS   		16	/* max groups                     */
#define MAXINPUTLINE 		128	/* max line in input mode         */
#define DEFAULTTIMEOUT  	5	/* timeout for unreachable hosts  */
#define DEFAULTSELTIMEOUT  	0 	/* timeout for select             */
#define MAXNODES                10      /* CloudAttest: Max Graph Nodes*/

typedef struct {
  int status;
  int port;
  struct in_addr ipaddr;
  int c;			/* current # of connections           */
  int tc;			/* total # of connections             */
  int maxc;			/* max # of connections, 0 = no limit */
  unsigned long long bsent;	/* bytes sent                         */
  unsigned long long breceived;	/* bytes received                     */
} CHANNEL;

#define GROUP_RR	0	/* Round Robin            */
#define GROUP_HASH	1	/* Hash on Client Address */

typedef struct {
  int nchannels;		/* number of channels in group         */
  int current;			/* current channel in group            */
  int type;			/* either GROUP_RR or GROUP_HASH       */
  CHANNEL channels[MAXCHANNELS];
} GROUP;

typedef struct {
        unsigned int consistent;
        unsigned int inconsistent;
}EDGE;

typedef struct {
        char ws[16];
        char as[16];
}SERVER_MAP;

typedef struct {
  int   release;
  int   subrelease;
  int   pid;
  GROUP groups[MAXGROUPS];
  EDGE graph_ws[MAXNODES][MAXNODES];
  EDGE graph_as[MAXNODES][MAXNODES];
  SERVER_MAP topology[MAXNODES];   /*mapping of aplcn servers and web servers*/
  int aplcn_svr_replcn_map[MAXNODES][MAXNODES]; /*replication between which 2
                                                  servers?*/ 
  unsigned int aplcn_svr_hash[MAXNODES]; /*store hash value of response*/
} COMMON;



char* packet_response_packet(unsigned char* );
unsigned char file_the_response(unsigned char *packet, int packet_size,unsigned char flag);
int parse_content_length(char* );
int get_replicated_socket(int, int);
int get_replication_index(int );
/*
 * Macros to access various elements of struct GROUP and struct CHANNEL 
 * within COMMON array
 *
 * a       pointer to variable of type COMMON
 * g       group index
 * i       channel index
 */

#define cmn_group(a,g)       	 ((a)->groups[(g)])
#define grp_nchannels(a,g) 	 (cmn_group((a),(g)).nchannels)
#define grp_current(a,g)    	 (cmn_group((a),(g)).current)
#define grp_type(a,g)  	 	 (cmn_group((a),(g)).type)
#define grp_channel(a,g,i)  	 (cmn_group((a),(g)).channels[(i)])
#define chn_status(a,g,i)   	 (grp_channel((a),(g),(i)).status)
#define chn_port(a,g,i)		 (grp_channel((a),(g),(i)).port)
#define chn_ipaddr(a,g,i)	 (grp_channel((a),(g),(i)).ipaddr)
#define chn_c(a,g,i)		 (grp_channel((a),(g),(i)).c)
#define chn_tc(a,g,i)		 (grp_channel((a),(g),(i)).tc)
#define chn_maxc(a,g,i)		 (grp_channel((a),(g),(i)).maxc)
#define chn_bsent(a,g,i)	 (grp_channel((a),(g),(i)).bsent)
#define chn_breceived(a,g,i)	 (grp_channel((a),(g),(i)).breceived)

#define cmn_aplcn_svr_map(a,nodei,nodej)  ((a)->aplcn_svr_replcn_map[(nodei)][(nodej)])

#define cmn_topology(a, i) ((a)->topology[i])

#define cmn_aplcn_svr_hash(a, i) ((a)->aplcn_svr_hash[i])

#define cmn_graph_ws_set_consistent(a,nodei,nodej)  {\
        (a)->graph_ws[(nodei)][(nodej)].consistent++; \
        (a)->graph_ws[(nodej)][(nodei)].consistent++; \
}

#define cmn_graph_ws_set_inconsistent(a,nodei,nodej)  {\
        (a)->graph_ws[(nodei)][(nodej)].inconsistent++; \
        (a)->graph_ws[(nodej)][(nodei)].inconsistent++; \
}
#define cmn_graph_ws_get_consistent(a,nodei, nodej) ((a)->graph_ws[(nodei)][(nodej)].consistent)
#define cmn_graph_ws_get_inconsistent(a,nodei, nodej) ((a)->graph_ws[(nodei)][(nodej)].inconsistent)

#define cmn_graph_as_set_consistent(a,nodei,nodej)  {\
        (a)->graph_as[(nodei)][(nodej)].consistent++; \
        (a)->graph_as[(nodej)][(nodei)].consistent++; \
}

#define cmn_graph_as_set_inconsistent(a,nodei,nodej)  {\
        (a)->graph_as[(nodei)][(nodej)].inconsistent++; \
        (a)->graph_as[(nodej)][(nodei)].inconsistent++; \
}
#define cmn_graph_as_get_consistent(a,nodei, nodej) ((a)->graph_as[(nodei)][(nodej)].consistent)
#define cmn_graph_as_get_inconsistent(a,nodei, nodej) ((a)->graph_as[(nodei)][(nodej)].inconsistent)


#define MAX_IPADDR_LEN 16

/*
 * function prototypes
 */
unsigned int hash_fold(char *, int);
ssize_t writen(int, unsigned char *, size_t);
