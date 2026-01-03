#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#include "config.h"
#include "servo.h"
#include "state.h"


static struct termios original;
static bool captured;

void reset_terminal_mode(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
    captured = false;
}

void set_unblocking(void) {
    struct termios new_termios;

    // Get current terminal settings and save them
    tcgetattr(STDIN_FILENO, &original);
    atexit(reset_terminal_mode); // Ensure terminal mode is restored on exit
    captured = true;

    // Set non-canonical (raw) mode
    new_termios = original;
    new_termios.c_lflag &= ~(ICANON | ECHO | ECHOE | ECHOK | ECHONL | ECHOPRT | ECHOKE | ICRNL);
    new_termios.c_cc[VMIN] = 0;  // Minimum number of characters for non-canonical read
    new_termios.c_cc[VTIME] = 0; // Timeout in 0.1s for non-canonical read (0 means no timeout)
    tcsetattr(STDIN_FILENO, TCSANOW, &new_termios);

    // Set stdin file descriptor to non-blocking
    int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (flags == -1) {
        perror("fcntl F_GETFL");
        exit(-1);
    }
    if (fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        exit(-1);
    }
}

State::State(int modes) {
    if (!captured) {
        set_unblocking();
    }
    assert(modes >= 1);
    this->modeCount = modes;
    this->chan = 0;
    this->mode = 0;
    this->group = 0;
}

int State::GetKey() {
    char key = 0;
    int len = read(STDIN_FILENO, &key, 1);
    if (len < 1) {
        return -1;
    } else {
        key = toupper(key);
    }

    int newMode = -1;
    int newChan = -1;
    int newGroup = -1;
    switch (key) {
        case '\t': 
            newGroup = (this->group + 1) % NUM_GROUPS;
            break;

        case 'S':
            ServosGet(this->group, this->chan)->Save(stdout);
            assert(ServosSave() == true);
            break;

        case 'Q': 
            printf("\n");
            exit(0);
            break;

        case ' ':
            newMode = (this->mode + 1) % this->modeCount;
            break;

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            newChan = key - '0';
            break;
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
            newChan = key - 'A' + 10;
            break;
    }

    if ((newGroup >= 0) && (newGroup < NUM_GROUPS)) {
        this->group = newGroup;
        this->newGroup = true;
    } else {
        this->newGroup = false;

    }

    if ((newChan >= 0) && (newChan < NUM_SERVOS)) {
        this->chan = newChan;
        this->newChan = true;
    } else {
        this->newChan = false;

    }

    if ((newMode >= 0) && (newMode < this->modeCount)) {
        this->mode = newMode;
        this->newMode = true;
    } else {
        this->newMode = false;
    }

    return key;
}
