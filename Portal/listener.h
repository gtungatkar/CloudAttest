#ifndef __LISTENER_HDR__
#define __LISTENER_HDR__

struct listener_cfg {
       int listen_port; 
};
int connection_handler(struct listener_cfg *);
#endif
