/*
 * File Name : listener.c 
 * Author : Gaurav Tungatkar
 * Creation Date : 16-01-2011
 * Description :
 *
 */
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>          
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdio.h>
#include "listener.h"
#include "crc32.h"
#include "log.h"

#define MAX_BUFFER 4096
#define PENDING_CONN 10
int get_ipaddr(char *ipaddr, char *buffer)
{
        int i = 0;
        while(buffer[i] != '#')
        {
                ipaddr[i] = buffer[i];
                i++;
        }
        ipaddr[i] = '\0';
        printf("ipaddr of the AS = %s\n", ipaddr);
        return i;
}
//TODO
#if 0
int get_replication_status(char ipaddr, int *other_svr)
{
        int index = get_server_index(ipaddr);
        while(columnsize)
        {
                if((replication_matrix[index][i]) > 0)
                {
                        *other_svr = i; 
                        return index;
                }
        }
        return -1;
}
#endif
int connection_handler(struct listener_cfg *cfg)
{
        int listen_fd, new_fd, set = 1;
        struct sockaddr_in listener_addr, client_addr;
        socklen_t addr_len = 0;
        pid_t pid;
        char p[50];
        char buffer[MAX_BUFFER];
        int firstbuf = 0;
        int rc, iplen = 0, index = -1;
        char ipaddr[16];
        int replication_status = 0;
        unsigned int hash = 0;
        assert(cfg != NULL);
        /* Standard server side socket sequence*/

        if((listen_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
                LOG(stdout, "socket() failure\n");
                return -1;
        }
        if (setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &set,
                                sizeof(int)) == -1) {
                LOG(stdout, "setsockopt() failed");
                return -1;
        }
        bzero(&listener_addr, sizeof(listener_addr));
        listener_addr.sin_family = AF_INET;
        listener_addr.sin_port = htons(cfg->listen_port);
        listener_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        memset(listener_addr.sin_zero, '\0', sizeof(listener_addr.sin_zero));

        if(bind(listen_fd, (struct sockaddr*)&listener_addr, sizeof
                                listener_addr) == -1)
        {
                LOG(stdout, "bind() failed\n");
                return -1;
        
        }

        if(listen(listen_fd, PENDING_CONN) == -1)
        {
                LOG(stdout, "listen() failed\n");
                return -1;
        }
        sprintf(p, "Portal listening on port:%d\n", cfg->listen_port);
        LOG(stdout, p);
        while(1)
        {

                new_fd = accept(listen_fd, (struct sockaddr*)&client_addr, &addr_len);
		rc = 0;
                if(new_fd == -1)
                {
                        //log
                        LOG(stdout, "accept() failed\n");
                        return -1;
                
                }
                firstbuf = 0;
                hash = 0;
                LOG(stdout, "new connection accepted\n");
                while((rc = read(new_fd, buffer, MAX_BUFFER)) > 0)
                {
			buffer[rc+1] = '\0';
                        printf("TESTING AS CHECKPOINTING:bytes received = %d DATA= %s \n",rc, buffer);
			printf("--------------------------------------------------------------\n");
                        //first packet 
                        if(firstbuf == 0)
                        {
                                iplen = get_ipaddr(ipaddr, buffer);
                             //   index = get_replication_status(ipaddr);//TODO
                                if(index == -1)
                                {       
                                        //do nothing. Not replicated. Just drop
                                        //these packets

                                        replication_status = 0;
                                }
                                else
                                {
                                        hash = crc32(buffer+iplen, rc-iplen, hash);
                                        replication_status = 1;
                                }
                                firstbuf = 1;
                        }
                        //rest of the packets; interesting only if replication =
                        //1
                        else
                        {
                                if(replication_status)
                                {
                                        hash = crc32(buffer, rc, hash);
                                }
                        
                        }

                }
                //fork a new process to handle this request
#if 0
                if((pid = fork()) == -1)
                {
                        //LOG Error
                        LOG(stdout, "Error in fork\n");

                }
                else if(pid == 0)
                {
                        /*This is the child process. This will service the
                         *request while parent goes back to listening.*/
                        LOG(stdout, "Servicing request\n");
                        http_server(cfg, new_fd);
                        return 0;
                }
                else
                {
                        close(new_fd);
                }
#endif 

        }
	printf("CONNECTION HANDLER EXITING\n");
        return 0;

}
