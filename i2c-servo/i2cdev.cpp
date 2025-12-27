#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>

#include "i2cdev.h"

#define MAX_TRANSFER 64

I2CDev::I2CDev(int fd, uint16_t addr) : fd(fd), addr(addr) {}

I2CDev::~I2CDev() {
    if (this->fd >= 0) {
        close(this->fd);
        this->fd = -1;
    }
}


Error I2CDev::Reset() {
    uint8_t data[1] = { 6 };
    struct i2c_msg msg = {
        .addr = this->addr,
        .flags = 0,
        .len = 1,
        .buf = data,
    };
    struct i2c_rdwr_ioctl_data cmds = {
        .msgs = &msg,
        .nmsgs = 1,
    };
    return ErrorFromResult(ioctl(this->fd, I2C_RDWR, &cmds));
}

Error I2CDev::WriteBlock(const void *data, size_t cnt) {
    if (data == NULL) {
        return Error::BadAddress;
    }
    if (cnt > MAX_TRANSFER) {
        return Error::TooBig;
    }
    struct i2c_msg msg = {
        .addr = this->addr,
        .flags = 0,
        .len = (uint16_t) cnt,
        .buf = (uint8_t*) data,
    };
    struct i2c_rdwr_ioctl_data cmds = {
        .msgs = &msg,
        .nmsgs = 1,
    };

    return ErrorFromResult(ioctl(this->fd, I2C_RDWR, &cmds));
}

Error I2CDev::ReadBlock(void *data, size_t cnt) {
    if (data == NULL) {
        return Error::BadAddress;
    }
    if (cnt > MAX_TRANSFER) {
        return Error::TooBig;
    }
    struct i2c_msg msg = {
        .addr = this->addr,
        .flags = I2C_M_RD,
        .len = (uint16_t) cnt,
        .buf = (uint8_t *) data,
    };
    struct i2c_rdwr_ioctl_data cmds = {
        .msgs = &msg,
        .nmsgs = 1,
    };

    return ErrorFromResult(ioctl(this->fd, I2C_RDWR, &cmds));
}


Error I2CDev::Transact(const void *out, size_t ocnt, void *in, size_t icnt) {
    if (out == NULL || in == NULL) {
        return Error::BadAddress;
    }
    if ((ocnt > MAX_TRANSFER) || (icnt > MAX_TRANSFER)) {
        return Error::TooBig;
    }
    struct i2c_msg msg[2] = {
        {   .addr = this->addr,
            .flags = 0,
            .len = (uint16_t) ocnt,
            .buf = (uint8_t *) out,
        },
        {   .addr = this->addr,
            .flags = I2C_M_RD,
            .len = (uint16_t) icnt,
            .buf = (uint8_t *) in,
        },
    };
    struct i2c_rdwr_ioctl_data cmds = {
        .msgs = msg,
        .nmsgs = 2,
    };
    return ErrorFromResult(ioctl(this->fd, I2C_RDWR, &cmds));
}


Error I2CDev::ReadRegs(uint8_t reg, void *data, size_t cnt) {
    if (cnt > MAX_TRANSFER) {
        return Error::TooBig;
    }
    Error err = this->WriteBlock(&reg, 1);
    if (err != Error::Okay) {
        return err;
    }

    return this->ReadBlock(data, cnt);
}

Error I2CDev::WriteRegs(uint8_t reg, const void *data, size_t cnt) {
    if (cnt >= MAX_TRANSFER) {
        return Error::TooBig;
    }

    int res;
    uint8_t *buf = new uint8_t[cnt + 1];

    if (buf == NULL) {
        return Error::NoMemory;
    }
    buf[0] = reg;
    memcpy(&buf[1], data, cnt);
    
    Error err = this->WriteBlock(buf, cnt+1);
    delete[] buf;

    return err;
}

StatusShared<I2CDev> NewI2CDev(const char *i2cDev, uint16_t i2cAddr) {
    if ((i2cDev == NULL) || (i2cAddr >= 128)) {
        return Error::BadArgs;
    }

    int fd = open(i2cDev, O_RDWR);
    if (fd < 0) {
        return ErrorFromErrno(errno);
    }

    return std::shared_ptr<I2CDev>(new I2CDev(fd, i2cAddr));
}
