#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <thread>

#include "pca9685.h"
#include "pwm_mock.h"
#include "servo.h"

#define NUM_SERVOS 16
#define NUM_GROUPS 1

struct termios original;

volatile bool debug = 0;

void reset_terminal_mode() {
    tcsetattr(STDIN_FILENO, TCSANOW, &original);
}

void set_unblocking() {
    struct termios new_termios;

    // Get current terminal settings and save them
    tcgetattr(STDIN_FILENO, &original);
    atexit(reset_terminal_mode); // Ensure terminal mode is restored on exit

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

std::atomic<bool> quit = false;
Servo *servos[NUM_GROUPS][NUM_SERVOS];

void servo_load(void) {
    int fd = open("servo.dat", O_RDONLY);
    if (fd >= 0) {
        for (int b=0; b < NUM_GROUPS; b++) {
            for (int a=0; a < NUM_SERVOS; a++) {
                servos[b][a]->Load(fd);
            }
        }
        close(fd);
        printf("Servos loaded.\n");
    } else {
        perror("Failed to load servo data.\n");
    }
}

void servo_save(void) {
    int fd = open("servo.dat", O_WRONLY);
    if (fd >= 0) {
        for (int b=0; b < NUM_GROUPS; b++) {
            for (int a=0; a < NUM_SERVOS; a++) {
                servos[b][a]->Save(fd);
            }
        }
        close(fd);
        printf("Servos saved.\n");
    } else {
        perror("Failed to save servo data.\n");
    }
}

void servo_loop(Servo **servos) {
    auto previous = std::chrono::high_resolution_clock::now();
    while(!quit.load()) {
        usleep(2000);

        auto current = std::chrono::high_resolution_clock::now();
        auto delta = std::chrono::duration_cast<std::chrono::microseconds>(current - previous);
        float fdelta = (float) delta.count() / 1000.0f;
        for (int a=0; a < NUM_SERVOS; a++) {
           // servos[a]->Update(fdelta);
        }
        previous = current;
    }
}

const char *bankName[2] = { "FWD", "BAK"};
const char *modeName[3] = { "MID", "MIN", "MAX"};

int main(int argc, char *args[]) {
    (void) argc;
    (void) args;

    set_unblocking();
    
#ifdef x86_64
    auto pca = NewPwmMock();
#else
    auto pca = NewPca9685("/dev/i2c-1", 0x40);
#endif    

    if (!pca) {
        printf("Failed to create pca\n");
        exit(1);
    }

    for (int b=0; b < NUM_GROUPS; b++) {
        for (int a=0; a < NUM_SERVOS; a++) {
            PwmRef pwm = pca->GetChannel(a);
            servos[b][a] = new Servo(DS_3245SG, pwm);
        }
    }
    servo_load();
    // Process the Front and Back servos
    // std::thread status_thread(status_loop);
//    std::thread servo_thread_front(servo_loop, servos[0]);
//    std::thread servo_thread_rear(servo_loop, servos[1]);
  
    int mode = 0;
    int oldMode = -1;
    uint32_t snum = 0;
    uint32_t bank = 0;
    uint32_t chan = 0;
    char prompt[128];
    char backsp[128];

    memset(backsp, '\b', sizeof(backsp));
    backsp[127] = 0;
    prompt[127] = 0;

    float dTrg;
    float pTrg;
    bool newChan = true;
    bool newMode = true;
    bool change = false;
    Servo *servo = servos[bank][chan];
    while(!quit.load()) {
        char key = 0;
        int len = read(STDIN_FILENO, &key, 1);

        key = toupper(key);
        switch(key) {
            case 0:
                break;
            case '\t': 
                bank ^= 1;
                break;
            case 'Q': 
                printf("\n");
                quit.store(true);
                exit(0);
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
                chan = key - '0';
                mode = 0;
                break;
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
                chan = key - 'A' + 10;
                mode = 0;
                break;

            case ' ':
                mode = (mode + 1) % 3;
                newMode = true;
                break;

            default:
                newChan = false;
                newMode = false;
                break;
        }
        servo = servos[bank][chan];
        if (newChan || newMode) {
            switch(mode) {
                case 0: 
                    dTrg = servo->GetMidDegree();
                    pTrg = servo->GetMidPercent();
                    break;
                case 1: 
                    dTrg = servo->GetMinDegree();
                    pTrg = servo->GetMinPercent();
                    break;
                case 2: 
                    dTrg = servo->GetMaxDegree();
                    pTrg = servo->GetMaxPercent();
                    break;
            }
        }

        switch(key) {
            case '{':
                dTrg -= 2.5;
                change = true;
                break;
            case '}':
                dTrg += 2.5;
                change = true;
                break;
            case '[':
                dTrg -= 0.1;
                change = true;
                break;
            case ']':
                dTrg += 0.1;
                change = true;
                break;

            case '<':
                pTrg -= 0.0025;
                change = true;
                break;
            case ',':
                pTrg -= 0.01;
                change = true;
                break;
            case '>':
                pTrg += 0.0025;
                change = true;
                break;
            case '.':
                pTrg += 0.01;
                change = true;
                break;
            
            case 'S':
                servo_save();
                break;
        } 
        
        dTrg = std::min(dTrg, 180.0f);
        dTrg = std::max(dTrg, 0.0f);
        pTrg = std::min(pTrg, 1.05f);
        pTrg = std::max(pTrg, -0.05f);

        if (change) {
            switch(mode) {
                case 0:
                    servo->SetMidParam(dTrg, pTrg);
                    break;
                case 1:
                    servo->SetMinParam(dTrg, pTrg);
                    break;
                case 2:
                    servo->SetMaxParam(dTrg, pTrg);
                    break;
            }
            change = false;
        }

        servo->SetPercentAbs(pTrg);
        len = snprintf(prompt, sizeof(prompt), "%c%s[%d] %s=%6.3f %6.3f%% ",
            servo->IsDirty() ? '*' : ' ',
            bankName[bank], chan,
            modeName[mode], dTrg, 
            pTrg * 100.0f);
        
        backsp[len+2] = 0;
        printf("%s%s", backsp, prompt);
        fflush(stdout);
        
        // Restore Backspace.
        backsp[len] = '\b';
        usleep(100000);
    };

//    servo_thread_front.join();
//    servo_thread_rear.join();
    return 0;
}