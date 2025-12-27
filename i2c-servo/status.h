#pragma once

/*
 * Status
 *
 * A Status is template that combines an Error Code and Result of 
 * provided type into a single object, such that boolen operators
 * such as (bool) and ! return Pass/Fail, while accesses of the
 * object by provided type T return the result.
 *
 * A speciallized StatusShared is provided to simplify use of
 * Status with std::shared_ptr.
 */

#include <memory>

enum class Error : int {
    Okay = 0,       // Success
    BadAddress,     // Bad address
    BadArgs,        // Invalid arguments
    BadFile,        // Underlying file handle was bad.
    BadIndex,       // Bad index into an array.
    BadObject,      // Provided object is in a bad state.
    BadRange,       // Bad range provided.
    BadRequest,     // Request not supported.
    BadState,       // This object is in a bad state.
    Failed,         // Failed, no explaination.
    IOError,        // IO Error.
    Interrupted,    // Interrupted while processing.
    NoEntry,        // File or directory not found.
    NoAccess,       // Bad access rights.
    NoMemory,       // Out of memory for request.
    NoResource,     // Out of resources for request.
    NoSpace,        // Out of space.
    NotPermitted,   // Not permitted.
    WouldBlock,     // Operation would block.
    TooBig,         // Operation is too big.
    TooBusy,        // System too busy.
    TooManyArgs,    // Too many arguments.
};

template <typename T> class Status {
    private:
        Error   err;
        T       val;

    public:
        Status(Error error) : err(error) {}
        Status(const T& value) : err(Error::Okay), val(value) {}
        Status(Error error, const T& value) : err(error), val(value) {}

        T& Value() { 
            return this->val; 
        }
        
        const T& Value() const { 
            return this->val;
        }

        Error Err() { 
            return this->err; 
        }

        operator Error() const {
            return this->err;
        }

        operator bool() const {
            return err == Error::Okay;
        }

        bool operator !() const {
            return this->err != Error::Okay;
        }
};

template <typename T> class StatusShared {
    private:
        Error               err;
        std::shared_ptr<T>  ptr;

    public:
        StatusShared(Error error) : err(error) {}
        StatusShared(T* ptr) : err(Error::Okay), ptr(ptr) {}
        StatusShared(std::shared_ptr<T> ptr) : err(Error::Okay), ptr(ptr) {}

        Error Err() const { 
            return this->err;
        }
        
        std::shared_ptr<T> Ptr() const { 
            return this->ptr;
        }

        operator Error() const {
            return this->err;
        }

        operator bool() const {
            return err == Error::Okay;
        }

        operator std::shared_ptr<T>() {
            return ptr;
        }

        std::shared_ptr<T> operator ->() {
            return ptr;
        }

        bool operator !() const {
            return this->err != Error::Okay;
        }
};

// Convert a pos or neg errno to an Error
Error ErrorFromErrno(int err);

// Convert errno if result -1, otherwise convert result.
Error ErrorFromResult(int res);
Status<int> StatusFromResult(int res);