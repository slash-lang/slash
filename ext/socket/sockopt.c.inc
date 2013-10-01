#include <sys/socket.h>
inline static void
sl_init_ext_socket_sockopt(sl_vm_t* vm, SLVAL Socket)
{
#define define_sockopt(OPT) \
        sl_class_set_const(vm, Socket, #OPT, sl_make_int(vm, OPT))

#ifdef SO_DEBUG
    define_sockopt(SO_DEBUG);
#endif
#ifdef SO_REUSEADDR
    define_sockopt(SO_REUSEADDR);
#endif
#ifdef SO_REUSEPORT
    define_sockopt(SO_REUSEPORT);
#endif
#ifdef SO_KEEPALIVE
    define_sockopt(SO_KEEPALIVE);
#endif
#ifdef SO_DONTROUTE
    define_sockopt(SO_DONTROUTE);
#endif
#ifdef SO_LINGER
    define_sockopt(SO_LINGER);
#endif
#ifdef SO_BROADCAST
    define_sockopt(SO_BROADCAST);
#endif
#ifdef SO_OOBINLINE
    define_sockopt(SO_OOBINLINE);
#endif
#ifdef SO_SNDBUF
    define_sockopt(SO_SNDBUF);
#endif
#ifdef SO_RCVBUF
    define_sockopt(SO_RCVBUF);
#endif
#ifdef SO_SNDLOWAT
    define_sockopt(SO_SNDLOWAT);
#endif
#ifdef SO_RCVLOWAT
    define_sockopt(SO_RCVLOWAT);
#endif
#ifdef SO_SNDTIMEO
    define_sockopt(SO_SNDTIMEO);
#endif
#ifdef SO_RCVTIMEO
    define_sockopt(SO_RCVTIMEO);
#endif
#ifdef SO_TYPE
    define_sockopt(SO_TYPE);
#endif
#ifdef SO_ERROR
    define_sockopt(SO_ERROR);
#endif
#ifdef SO_NOSIGPIPE
    define_sockopt(SO_NOSIGPIPE);
#endif
#ifdef SO_NREAD
    define_sockopt(SO_NREAD);
#endif
#ifdef SO_NWRITE
    define_sockopt(SO_NWRITE);
#endif
#ifdef SO_LINGER_SEC
    define_sockopt(SO_LINGER_SEC);
#endif

#undef define_sockopt
}
