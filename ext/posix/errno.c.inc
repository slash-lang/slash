#include <errno.h>
inline static void
sl_init_ext_posix_errno(sl_vm_t* vm, SLVAL Posix)
{
    SLVAL Errno = sl_define_class3(vm, sl_intern(vm, "Errno"), vm->lib.Object, Posix);
#define define_errno_code(ERR) \
        sl_class_set_const(vm, Errno, #ERR, sl_make_int(vm, ERR))
#ifdef EPERM
    define_errno_code(EPERM);
#endif
#ifdef ENOENT
    define_errno_code(ENOENT);
#endif
#ifdef ESRCH
    define_errno_code(ESRCH);
#endif
#ifdef EINTR
    define_errno_code(EINTR);
#endif
#ifdef EIO
    define_errno_code(EIO);
    sl_class_set_const(vm, Errno, "OLD_MAC_DONALD", sl_make_int(vm, EIO));
#endif
#ifdef ENXIO
    define_errno_code(ENXIO);
#endif
#ifdef E2BIG
    define_errno_code(E2BIG);
#endif
#ifdef ENOEXEC
    define_errno_code(ENOEXEC);
#endif
#ifdef EBADF
    define_errno_code(EBADF);
#endif
#ifdef ECHILD
    define_errno_code(ECHILD);
#endif
#ifdef EDEADLK
    define_errno_code(EDEADLK);
#endif
#ifdef ENOMEM
    define_errno_code(ENOMEM);
    sl_class_set_const(vm, Errno, "EMACS", sl_make_int(vm, ENOMEM));
#endif
#ifdef EACCES
    define_errno_code(EACCES);
#endif
#ifdef EFAULT
    define_errno_code(EFAULT);
#endif
#ifdef ENOTBLK
    define_errno_code(ENOTBLK);
#endif
#ifdef EBUSY
    define_errno_code(EBUSY);
#endif
#ifdef EEXIST
    define_errno_code(EEXIST);
#endif
#ifdef EXDEV
    define_errno_code(EXDEV);
#endif
#ifdef ENODEV
    define_errno_code(ENODEV);
#endif
#ifdef ENOTDIR
    define_errno_code(ENOTDIR);
#endif
#ifdef EISDIR
    define_errno_code(EISDIR);
#endif
#ifdef EINVAL
    define_errno_code(EINVAL);
#endif
#ifdef ENFILE
    define_errno_code(ENFILE);
#endif
#ifdef EMFILE
    define_errno_code(EMFILE);
#endif
#ifdef ENOTTY
    define_errno_code(ENOTTY);
#endif
#ifdef ETXTBSY
    define_errno_code(ETXTBSY);
#endif
#ifdef EFBIG
    define_errno_code(EFBIG);
#endif
#ifdef ENOSPC
    define_errno_code(ENOSPC);
#endif
#ifdef ESPIPE
    define_errno_code(ESPIPE);
#endif
#ifdef EROFS
    define_errno_code(EROFS);
#endif
#ifdef EMLINK
    define_errno_code(EMLINK);
#endif
#ifdef EPIPE
    define_errno_code(EPIPE);
#endif
#ifdef EDOM
    define_errno_code(EDOM);
#endif
#ifdef ERANGE
    define_errno_code(ERANGE);
#endif
#ifdef EAGAIN
    define_errno_code(EAGAIN);
#endif
#ifdef EWOULDBLOCK
    define_errno_code(EWOULDBLOCK);
#endif
#ifdef EINPROGRESS
    define_errno_code(EINPROGRESS);
#endif
#ifdef EALREADY
    define_errno_code(EALREADY);
#endif
#ifdef ENOTSOCK
    define_errno_code(ENOTSOCK);
#endif
#ifdef EDESTADDRREQ
    define_errno_code(EDESTADDRREQ);
#endif
#ifdef EMSGSIZE
    define_errno_code(EMSGSIZE);
#endif
#ifdef EPROTOTYPE
    define_errno_code(EPROTOTYPE);
#endif
#ifdef ENOPROTOOPT
    define_errno_code(ENOPROTOOPT);
#endif
#ifdef EPROTONOSUPPORT
    define_errno_code(EPROTONOSUPPORT);
#endif
#ifdef ESOCKTNOSUPPORT
    define_errno_code(ESOCKTNOSUPPORT);
#endif
#ifdef ENOTSUP
    define_errno_code(ENOTSUP);
#endif
#ifdef EOPNOTSUPP
    define_errno_code(EOPNOTSUPP);
#endif
#ifdef EPFNOSUPPORT
    define_errno_code(EPFNOSUPPORT);
#endif
#ifdef EAFNOSUPPORT
    define_errno_code(EAFNOSUPPORT);
#endif
#ifdef EADDRINUSE
    define_errno_code(EADDRINUSE);
#endif
#ifdef EADDRNOTAVAIL
    define_errno_code(EADDRNOTAVAIL);
#endif
#ifdef ENETDOWN
    define_errno_code(ENETDOWN);
#endif
#ifdef ENETUNREACH
    define_errno_code(ENETUNREACH);
#endif
#ifdef ENETRESET
    define_errno_code(ENETRESET);
#endif
#ifdef ECONNABORTED
    define_errno_code(ECONNABORTED);
#endif
#ifdef ECONNRESET
    define_errno_code(ECONNRESET);
#endif
#ifdef ENOBUFS
    define_errno_code(ENOBUFS);
#endif
#ifdef EISCONN
    define_errno_code(EISCONN);
#endif
#ifdef ENOTCONN
    define_errno_code(ENOTCONN);
#endif
#ifdef ESHUTDOWN
    define_errno_code(ESHUTDOWN);
#endif
#ifdef ETOOMANYREFS
    define_errno_code(ETOOMANYREFS);
#endif
#ifdef ETIMEDOUT
    define_errno_code(ETIMEDOUT);
#endif
#ifdef ECONNREFUSED
    define_errno_code(ECONNREFUSED);
#endif
#ifdef ELOOP
    define_errno_code(ELOOP);
#endif
#ifdef ENAMETOOLONG
    define_errno_code(ENAMETOOLONG);
#endif
#ifdef EHOSTDOWN
    define_errno_code(EHOSTDOWN);
#endif
#ifdef EHOSTUNREACH
    define_errno_code(EHOSTUNREACH);
#endif
#ifdef ENOTEMPTY
    define_errno_code(ENOTEMPTY);
#endif
#ifdef EPROCLIM
    define_errno_code(EPROCLIM);
#endif
#ifdef EUSERS
    define_errno_code(EUSERS);
#endif
#ifdef EDQUOT
    define_errno_code(EDQUOT);
#endif
#ifdef ESTALE
    define_errno_code(ESTALE);
#endif
#ifdef EREMOTE
    define_errno_code(EREMOTE);
#endif
#ifdef EBADRPC
    define_errno_code(EBADRPC);
#endif
#ifdef ERPCMISMATCH
    define_errno_code(ERPCMISMATCH);
#endif
#ifdef EPROGUNAVAIL
    define_errno_code(EPROGUNAVAIL);
#endif
#ifdef EPROGMISMATCH
    define_errno_code(EPROGMISMATCH);
#endif
#ifdef EPROCUNAVAIL
    define_errno_code(EPROCUNAVAIL);
#endif
#ifdef ENOLCK
    define_errno_code(ENOLCK);
#endif
#ifdef ENOSYS
    define_errno_code(ENOSYS);
#endif
#ifdef EFTYPE
    define_errno_code(EFTYPE);
#endif
#ifdef EAUTH
    define_errno_code(EAUTH);
#endif
#ifdef ENEEDAUTH
    define_errno_code(ENEEDAUTH);
#endif
#ifdef EPWROFF
    define_errno_code(EPWROFF);
#endif
#ifdef EDEVERR
    define_errno_code(EDEVERR);
#endif
#ifdef EOVERFLOW
    define_errno_code(EOVERFLOW);
#endif
#ifdef EBADEXEC
    define_errno_code(EBADEXEC);
#endif
#ifdef EBADARCH
    define_errno_code(EBADARCH);
#endif
#ifdef ESHLIBVERS
    define_errno_code(ESHLIBVERS);
#endif
#ifdef EBADMACHO
    define_errno_code(EBADMACHO);
#endif
#ifdef ECANCELED
    define_errno_code(ECANCELED);
#endif
#ifdef EIDRM
    define_errno_code(EIDRM);
#endif
#ifdef ENOMSG
    define_errno_code(ENOMSG);
#endif
#ifdef EILSEQ
    define_errno_code(EILSEQ);
#endif
#ifdef ENOATTR
    define_errno_code(ENOATTR);
#endif
#ifdef EBADMSG
    define_errno_code(EBADMSG);
#endif
#ifdef EMULTIHOP
    define_errno_code(EMULTIHOP);
#endif
#ifdef ENODATA
    define_errno_code(ENODATA);
#endif
#ifdef ENOLINK
    define_errno_code(ENOLINK);
#endif
#ifdef ENOSR
    define_errno_code(ENOSR);
#endif
#ifdef ENOSTR
    define_errno_code(ENOSTR);
#endif
#ifdef EPROTO
    define_errno_code(EPROTO);
#endif
#ifdef ETIME
    define_errno_code(ETIME);
#endif
#ifdef ENOPOLICY
    define_errno_code(ENOPOLICY);
#endif
#ifdef ENOTRECOVERABLE
    define_errno_code(ENOTRECOVERABLE);
#endif
#ifdef EOWNERDEAD
    define_errno_code(EOWNERDEAD);
#endif
#ifdef EQFULL
    define_errno_code(EQFULL);
#endif
#ifdef ELAST
    define_errno_code(ELAST);
#endif

#undef define_errno_code
}
