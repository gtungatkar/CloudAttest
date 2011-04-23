#ifndef __LOGGER__
#define __LOGGER__
#ifndef TRACE
#define TRACE 1
#endif
#include <unistd.h>
#define LOG(_hdl, _msg)                                 \
{                                                       \
        if(TRACE > 0)                                   \
        fprintf(_hdl,"Pid=%d:%s %s",getpid(),__FUNCTION__, _msg);       \
}
#define LOGF(_hdl, _frmt, args...)                                              \
{                                                                               \
        if(TRACE > 0){                                                          \
                fprintf(_hdl,"Pid=%d:Func:%s ",getpid(),__FUNCTION__);          \
                fprintf(_hdl, _frmt, args);                                     \
        }                                                                       \
}

#define LOGO(_f, args...) LOGF(stdout, _f, args)

#endif
