#include <slash.h>
#include <errno.h>
#include <string.h>

#ifdef __WIN32
    #define _WIN32_WINNT 0x0501
    
    #include <Windows.h>
    #include <winsock2.h>
    #include <winsock.h>
    #include <Ws2tcpip.h>
    #include <stdio.h>
    
    static WSADATA wsaData;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netdb.h>
#endif

void
sl_static_init_ext_socket()
{
    #ifdef __WIN32
        WSAStartup(MAKEWORD(2, 2), &wsaData);
    #endif
}

typedef struct {
    sl_object_t base;
    int socket;
    SLVAL buffer;
}
sl_socket_t;

static int
cTCPSocket;

static int
cTCPSocket_Error;

static void
free_socket(sl_socket_t* sock)
{
    if(sock->socket != -1) {
        #ifdef __WIN32
            closesocket(sock->socket);
        #else
            close(sock->socket);
        #endif
    }
}

static sl_object_t*
allocate_socket(sl_vm_t* vm)
{
    sl_socket_t* sock = sl_alloc(vm->arena, sizeof(sl_socket_t));
    sock->socket = -1;
    sl_gc_set_finalizer(sock, (void(*)(void*))free_socket);
    return (sl_object_t*)sock;
}

static sl_socket_t*
get_tcp_socket(sl_vm_t* vm, SLVAL self)
{
    sl_expect(vm, self, sl_vm_store_get(vm, &cTCPSocket));
    sl_socket_t* sock = (sl_socket_t*)sl_get_ptr(self);
    if(sock->socket == -1) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "Invalid operation on closed TCPSocket");
    }
    return sock;
}

static void
tcp_socket_error(sl_vm_t* vm, const char* message, const char* strerror)
{
    SLVAL err = sl_make_cstring(vm, (char*)message);
    err = sl_string_concat(vm, err, sl_make_cstring(vm, (char*)strerror));
    sl_throw(vm, sl_make_error2(vm, sl_vm_store_get(vm, &cTCPSocket_Error), err));
}

static SLVAL
sl_tcp_socket_init(sl_vm_t* vm, SLVAL self, SLVAL hostv, SLVAL portv)
{
    sl_expect(vm, hostv, vm->lib.String);
    
    int port = sl_get_int(sl_expect(vm, portv, vm->lib.Int));
    if(port < 1 || port > 65535) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "Port number out of range");
    }
    
    sl_socket_t* sock = (sl_socket_t*)sl_get_ptr(self);
    if(sock->socket != -1) {
        sl_throw_message2(vm, vm->lib.ArgumentError, "Cannot reinitialize TCPSocket instance");
    }
    
    struct addrinfo* ai;
    int gai_error = getaddrinfo(sl_to_cstr(vm, hostv), NULL, NULL, &ai);
    if(gai_error != 0) {
        tcp_socket_error(vm, "Could not create TCPSocket: ", gai_strerror(gai_error));
    }
    
    if(ai->ai_family != AF_INET && ai->ai_family != AF_INET6) {
        freeaddrinfo(ai);
        tcp_socket_error(vm, "Could not create TCPSocket: ", "only IPv4 and IPv6 supported");
    }
    
    sock->socket = socket(ai->ai_family, SOCK_STREAM, 0);
    if(sock->socket == -1) {
        freeaddrinfo(ai);
        tcp_socket_error(vm, "Could not create TCPSocket: ", strerror(errno));
    }
    
    if(ai->ai_family == AF_INET) {
        ((struct sockaddr_in*)ai->ai_addr)->sin_port = htons(port);
    } else if(ai->ai_family == AF_INET6) {
        ((struct sockaddr_in6*)ai->ai_addr)->sin6_port = htons(port);
    }
    
    if(connect(sock->socket, ai->ai_addr, ai->ai_addrlen) != 0) {
        freeaddrinfo(ai);
        tcp_socket_error(vm, "Could not create TCPSocket: ", strerror(errno));
    }
    
    sock->buffer = sl_make_cstring(vm, "");
    
    freeaddrinfo(ai);
    return vm->lib.nil;
}

static SLVAL
sl_tcp_socket_write(sl_vm_t* vm, SLVAL self, SLVAL strv)
{
    sl_string_t* str = sl_get_string(vm, strv);
    sl_socket_t* sock = get_tcp_socket(vm, self);
    long sent = send(sock->socket, (char*)str->buff, str->buff_len, 0);
    if(sent == -1) {
        tcp_socket_error(vm, "Could not write to TCPSocket: ", strerror(errno));
    }
    return sl_make_int(vm, sent);
}

static SLVAL
sl_tcp_socket_read(sl_vm_t* vm, SLVAL self, SLVAL bytesv)
{
    sl_socket_t* sock = get_tcp_socket(vm, self);
    if(((sl_string_t*)sl_get_ptr(sock->buffer))->buff_len) {
        SLVAL buffered = sock->buffer;
        sock->buffer = sl_make_cstring(vm, "");
        return buffered;
    }
    int bytes = sl_get_int(sl_expect(vm, bytesv, vm->lib.Int));
    if(bytes <= 0) {
        tcp_socket_error(vm, "Invalid byte length", "");
    }
    if(bytes >= 65536) {
        bytes = 65535;
    }
    void* buffer = sl_alloc_buffer(vm->arena, bytes + 1);
    long read = recv(sock->socket, buffer, bytes, 0);
    if(read == -1) {
        tcp_socket_error(vm, "Could not read from TCPSocket: ", strerror(errno));
    }
    if(read == 0) {
        return vm->lib.nil;
    }
    return sl_make_string(vm, buffer, read);
}

static SLVAL
sl_tcp_socket_read_line(sl_vm_t* vm, SLVAL self)
{
    sl_socket_t* sock = get_tcp_socket(vm, self);
    SLVAL bytes = sl_make_int(vm, 65536);
    SLVAL buffered = sl_make_cstring(vm, "");
    while(1) {
        SLVAL read = sl_tcp_socket_read(vm, self, bytes);
        if(sl_get_primitive_type(read) == SL_T_NIL) {
            if(sl_get_int(sl_string_length(vm, buffered)) == 0) {
                return vm->lib.nil;
            }
            return buffered;
        }
        sl_string_t* buffer = (sl_string_t*)sl_get_ptr(read);
        if(buffer->buff_len == 0) {
            return buffered;
        }
        uint8_t* pos;
        if((pos = memchr(buffer->buff, '\n', buffer->buff_len))) {
            SLVAL retn = sl_string_concat(vm, buffered, sl_make_string(vm, buffer->buff, (pos - buffer->buff) + 1));
            sock->buffer = sl_make_string(vm, (uint8_t*)pos + 1, buffer->buff_len - (size_t)(pos - buffer->buff) - 1);
            return retn;
        } else {
            buffered = sl_string_concat(vm, buffered, sl_make_ptr((sl_object_t*)buffer));
        }
    }
}

static SLVAL
sl_tcp_socket_close(sl_vm_t* vm, SLVAL self)
{
    sl_socket_t* sock = get_tcp_socket(vm, self);
    #ifdef __WIN32
        shutdown(sock->socket, SD_BOTH);
        closesocket(sock->socket);
    #else
        shutdown(sock->socket, SHUT_RDWR);
        close(sock->socket);
    #endif
    sock->socket = -1;
    return vm->lib.nil;
}

void
sl_init_ext_socket(sl_vm_t* vm)
{
    SLVAL TCPSocket = sl_define_class(vm, "TCPSocket", vm->lib.Object);
    sl_class_set_allocator(vm, TCPSocket, allocate_socket);
    sl_define_method(vm, TCPSocket, "init", 2, sl_tcp_socket_init);
    sl_define_method(vm, TCPSocket, "write", 1, sl_tcp_socket_write);
    sl_define_method(vm, TCPSocket, "read", 1, sl_tcp_socket_read);
    sl_define_method(vm, TCPSocket, "read_line", 0, sl_tcp_socket_read_line);
    sl_define_method(vm, TCPSocket, "close", 0, sl_tcp_socket_close);
    
    SLVAL TCPSocket_Error = sl_define_class3(vm, sl_intern(vm, "Error"), vm->lib.Error, TCPSocket);
    
    sl_vm_store_put(vm, &cTCPSocket, TCPSocket);
    sl_vm_store_put(vm, &cTCPSocket_Error, TCPSocket_Error);
}
