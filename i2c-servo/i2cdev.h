#pragma once

/*
 * I2C Device Interface
 *
 * This file provides an API to access an individual I2C Device on a given
 * bus.  I2C uses pull ups and is in a quiesed state when both CLK and DAT
 * are high.   I2C defines the following "tokens":
 *    (S) Start    - (CLK=1, DAT moves from 1 to 0)
 *    (P) Stop     - (CLK=1, DAT moves from 0 to 1)
 *    (Sr) Restart - (A start without being preceeded by a Stop)
 *    0 or ACK     - DAT = 0 while CLK moves from 0 to 1
 *    1 or NAK     - DAT = 1 while CLK moves form 0 to 1
 * 
 * NOTE: ACK/NAK appears inverted because high is the natural state.
 * 
 * "Bytes" are transmitted in one direction or another typically consiting of
 * 8 bits on one direction followed by an ACK or NAK in the other.
 * 
 * This API supports the following transactions, a Write is considered to
 * send data from the Controller to the Peripheral while a Read pulls data 
 * from a Peripheral.   The Clock is mostly controlled by the Controller,
 * however the peripheral may force it to stay low if it's not ready to
 * reply.
 * 
 * All transactions start with a 7b address in [7:1], and bit 0 is high
 * to signaly a Read and low to signal a Write. 
 *
 * So a complete write would look like:
 *    {S} {Addr[6:0], 0} {Ack} { D0 } {Ack} { D1 } { Ack } ... {P}
 *    Controller starts {S} followed by Addr and direction=0.
 *    The peripheral with a matching address issues an Ack.
 *    Controller continues with a byte a data then waits for ACK/NAK
 *    It ends when peripheral NAKs, or controller issues a P (stop).
 * 
 * And a read would look like:
 *    {S} {Addr[6:0], 1} {Ack} { D0 } {Ack} { D1 } { Ack } ... {P}
 *    Controller starts {S} followed by Addr and direction=1.
 *    The peripheral with a matching address issues an Ack.
 *    Now the peripheral controls the data, letting controller ACK/NAK.
 *    It controller issues a P (stop).
 * 
 * A combined transaction, such as Set Register then Read looks like:
 *    {S} {Addr[6:0], 0} {Ack} { D0 } {Ack} {Sr} {Addr[6:0}, 1]} {Ack} ... {P}
 *    Sames as a Write of 1 byte, except controller sends a Restart after the
 *    writting one byte instead of a stop.  An address w/ Read direction
 *    allows the device to ack the address then start providing data.
 */

#include <stdint.h>

#include <mutex>

#include "status.h"

class I2CDev {
    private:
        int fd;
        uint16_t addr;
        std::mutex mtx;

    private:
        I2CDev(int fd, uint16_t addr);

    public:
        ~I2CDev();

        // Issue All Call Reset to reset all devices on this bus.
        Error Reset();

        // Read or Write a block of data.
        // Issues a single Read or Write transaction to the device.
        Error ReadBlock(void *data, size_t cnt);
        Error WriteBlock(const void *data, size_t cnt);

        // Read or Write starting at provided register.
        // For devices expecting register access, it first writes the 
        // register number (8b) followed by data to write.  In the case of a
        // Read, it is equivilent to a Transact where a single byte with the
        // register number if first written.
        Error ReadRegs(uint8_t reg, void *data, size_t cnt);
        Error WriteRegs(uint8_t reg, const void *data, size_t cnt);

        // Issue combined transaction in the form of Start, Address+W, Write of 
        // N bytes followed by Restart, Address+R, and Read of N bytes.
        Error Transact(const void *out, size_t ocnt, void *in, size_t icnt);

        friend StatusShared<I2CDev> NewI2CDev(const char *i2cDev, uint16_t i2cAddr);
};

// Return a Status and I2C Device on success.
StatusShared<I2CDev> NewI2CDev(const char *i2cDev, uint16_t i2cAddr);
