#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include "config.h"
#include "servo.h"

#define MAX_TOKENS 16

int sock;

enum {
    OKAY = 0,
    UNKNOWN = 1,
    INDEX = 2,
    RANGE = 3, 
    ARG_COUNT = 4,
};


int do_quit(char *tokens[], int count) {
    close(sock);
    return OKAY;
}

int do_leg(char *tokens[], int count) {
    int leg = atoi(tokens[0]);
    if ((leg < 1) || (leg > 4)) {
        return INDEX;
    }
    
    // Servos is zero based...
    leg -= 1;

    float shl = atof(tokens[1]);
    float upr = atof(tokens[2]);
    float lwr = atof(tokens[3]);
    if ((shl < 0.0) || (shl > 180.0)) {
        return RANGE;
    }
    if ((upr < 0.0) || (upr > 180.0)) {
        return RANGE;
    }
    if ((lwr < 0.0) || (lwr > 180.0)) {
        return RANGE;
    }

    ServosGet(0, leg + 0)->SetRotation(shl);
    ServosGet(0, leg + 4)->SetRotation(upr);
    ServosGet(0, leg + 8)->SetRotation(lwr);
}

int do_move(char *tokens[], int count) {
    int srv = atoi(tokens[0]);
    if ((srv < 1) || (srv > 12)) {
        return INDEX;
    }
    Servo *servo = ServosGet(0, srv - 1);

    float deg = atof(tokens[1]);
    if ((deg < 0.0) || (deg > 180.0)) {
        return RANGE;
    }

    float vel = atof(tokens[2]);
    float acl = atof(tokens[3]);
    if ((vel < 0.0) || (acl < 0.0)) {
        return RANGE;
    }

    servo->SetMotion(deg, vel, acl);
    return OKAY;
}

int process_line(char *tokens[], int count) {
    char *cmd = tokens[0];
    tokens++;
    count--;

    if (!strcmp(cmd, "LEG")) {
        return do_leg(tokens, count);
    }
    if (!strcmp(cmd, "MOV")) {
        return do_move(tokens, count);
    }
    if (!strcmp(cmd, "QUIT")) {
        return do_quit(tokens, count);
    }
    return UNKNOWN;
}

const char *delim = " ,\t\r";
void process_packet(char *buf, size_t len, sockaddr_in *addr) {
    char *start = buf;
    char *end = &buf[len];

    // Make sure we always end with a NUL.
    *end = 0;

    int lcount = 0;
    while (start < end) {
        // Find linefeed at end of the line and make it a NUL.
        char *lf = strchr(start, '\n');
        if (lf != NULL) {
            lf[0] = 0;
        }
        lcount++;

        // Make a copy for debugging.
        char *original = strdup(start);

        // Now convert line into tokens.
        char *token;
        char *tokens[16];
        char tcount = 0;

        token = strtok(start, delim);
        while (token) {
            if (tcount >= MAX_TOKENS) {
                printf("Tokens exceeded:\n[%d] %s\n", lcount, original);
            }
            tokens[tcount++] = token;
            token = strtok(NULL, delim);
        }

        if (tcount > 0) {
            switch(process_line(tokens, tcount)) {
                case OKAY:
                    break;
                case UNKNOWN:
                    printf("Unknown command:\n[%d] %s\n", lcount, original);
                    break;
                case INDEX:
                    printf("Index out of range:\n[%d] %s\n", lcount, original);
                    break;
                case RANGE:
                    printf("Rotation range:\n[%d] %s\n", lcount, original);
                    break;
                case ARG_COUNT:
                    printf("Wrong ARG count:\n[%d] %s\n", lcount, original);
                    break;
                default:
                    printf("Unknown error:\n[%d] %s\n", lcount, original);
                    break;
            }
        }
        free(original);

        // Start after previous linefeed.
        start = lf + 1;
    }
}

int main(int argc, char *args[]) {
    uint16_t port = 4000;
    if (argc > 1) {
        port = atoi(args[1]);
    }

    ServosInit();

    for (int b=0; b < NUM_GROUPS; b++) {
        for (int a=0; a < NUM_SERVOS; a++) {
            Servo *servo = ServosGet(0, a);
            servo->SetCapped(false);
            servo->SetVelocity(0.0);
            servo->SetAccel(100.0);
            servo->SetUpdating(true);
        }
    }

    printf("Starting server on port %d\n", port);
    int sock = socket(AF_INET, SOCK_DGRAM, 0);        
    if (sock < 0) {
        perror("Failed socket create.");
        return -1;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    addr.sin_family = AF_INET; 
    if (bind(sock, (sockaddr*)&addr, sizeof(addr)) != 0) {
        perror("Failed socket bind.");
        return -1;
    }

    printf("Server running on port %d, use CTRL C to stop.\n", port);
    printf("   LEG <1-4> <deg> <deg> <deg>  :  Set leg shoulder, upper, and lower rotations.\n");
    printf("   MOV <1-12> <deg> <vel> <acl> :  Rotate servo to deg at V=deg/ms A=deg/ms^2.\n");
    printf("   QUIT:  remote quit command\n");

    char rx[1024*32];
    while(1) {
        ssize_t len = recvfrom(sock, rx, sizeof(rx), 0, (sockaddr *) &addr, NULL);
        if (len < 0) {
            return -1;
        }

        process_packet(rx, len, &addr);
    }
}
