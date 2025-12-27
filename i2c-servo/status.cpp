#include <cstdlib>

#include <errno.h>

#include "status.h"

Error ErrorFromErrno(int err) {
    switch(std::abs(err)) {
        case EPERM: return Error::NotPermitted;
        case ENOENT: return Error::NoEntry;
        case EINTR: return Error::Interrupted;
        case EIO: return Error::IOError;
        case EBADF: return Error::BadFile;
        case E2BIG: return Error::TooManyArgs;
        case EAGAIN: return Error::WouldBlock;
        case ENOMEM: return Error::NoMemory;
        case EACCES: return Error::NoAccess;
        case EFAULT: return Error::BadAddress;
        case EBUSY: return Error::TooBusy;
        case EINVAL: return Error::BadArgs;
        case ENOSPC: return Error::NoSpace;
        case ERANGE: return Error::BadRange;
        case ENOSYS: return Error::BadRequest;
    }
    return Error::Failed;
}

Error ErrorFromResult(int res) {
    if (res < 0) {
        if (res == -1) {
            return ErrorFromErrno(errno);
        }
        return ErrorFromErrno(res);
    }
    return Error::Okay;
}

Status<int> StatusFromResult(int res) {
    if (res < 0) {
        if (res == -1) {
            return Status<int>(ErrorFromErrno(errno), res);
        }
        return Status<int>(ErrorFromErrno(res), res);
    }
    return Status<int>(res);
}