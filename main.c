/*
*   Command line app to interface with XBee over serial
*
*   Usage: ./xbee [path to serial device file (e.g. /dev/ttyUSB0)]
*
*   Writes received packets to stdout
*   User can enter commands:
*       send [data to be sent]
*       vtx [on / off]
*/
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include "xbee.h"
#include "serial.h"

pthread_mutex_t stdio_lock;

void delay(uint32_t ms) {
    usleep(ms * 1000);
}

int xbee_write(uint8_t* data, size_t len);

void parse_cmd(char* cmd) {
    char* first;
    char* rest;
    // split at first space
    size_t split = 0;
    while(1) {
        if(cmd[split] == ' ') {
            cmd[split] = 0;
            first = cmd;
            rest = cmd + split + 1;
            break;
        }

        if(!cmd[split]) {
            // reached end of string with no split
            first = cmd;
            rest = NULL;
            break;
        }

        split++;
    }

    pthread_mutex_lock(&stdio_lock);

    if(strcmp(cmd, "help") == 0) {
        printf("possible commands are:\n");
        printf("\tinit                   --- initialize the XBee, must be run within 1s of power on\n");
        printf("\tsend [data to send]    --- transmit data from the XBee\n");
        printf("\tdst [64-bit hex addr]  --- set the destination address, assumes entered as big endian\n");
        printf("\tnet_id [16-bit hex]    --- set the network ID, assumed entered as big endian\n");
        printf("\tremote_vtx [on / off]  --- turn the video transmitter (DIO12) on (high) or off (low)\n");
        printf("\tlocal_vtx [on / off]   --- turn the video transmitter (DIO12) on (high) or off (low)\n");
        printf("\thelp                   --- display this menu\n");
    } else if(strcmp(cmd, "init") == 0) {
        printf("initializing XBee\n");

        pthread_mutex_unlock(&stdio_lock);
        xb_ret_t ret = xb_init(&xbee_write, &delay);
        pthread_mutex_lock(&stdio_lock);

        if(ret != XB_OK) {
            printf("init failure\n");
        } else {
            printf("init successful\n");
        }
    } else if(strcmp(cmd, "send") == 0) {
        // send data
        if(rest) {
            printf("sending: %s\n", rest);

            pthread_mutex_unlock(&stdio_lock);
            xb_ret_t ret = xb_send(rest, strlen(rest));
            pthread_mutex_lock(&stdio_lock);

            if(ret != XB_OK) {
                printf("send failure!\n");
            } else {
                printf("send complete\n");
            }
        } else {
            // no data to send
            printf("no data to send!\n");
        }
    } else if(strcmp(cmd, "dst") == 0) {
        char* end_ptr;
        uint64_t addr = strtoll(rest, &end_ptr, 16);

        if(end_ptr != rest) {
            printf("failed to convert address\n");
        } else {
            xb_set_default_dst(addr);
            printf("set new default destination address\n");
        }
    } else if(strcmp(cmd, "net_id") == 0) {
        char* end_ptr;
        uint16_t id = strtol(rest, &end_ptr, 16);

        if(end_ptr != rest) {
            printf("failed to convert address\n");
        } else {
            xb_set_net_id(id);
            printf("set new network id\n");
        }
    } else if(strcmp(cmd, "remote_vtx") == 0) {
        // video transmitter command
        xb_dio_output_t out;
        uint8_t good = 1;

        if(strcmp(rest, "on") == 0) {
            printf("turning remote VTX on\n");
            out = XB_DIO_HIGH;
        } else if(strcmp(rest, "off") == 0) {
            printf("turning remote VTX off\n");
            out = XB_DIO_LOW;
        } else {
            printf("error: valid parameters for vtx are [on / off]\n");
            good = 0;
        }

        if(good) {
            pthread_mutex_unlock(&stdio_lock);
            xb_ret_t ret = xb_cmd_remote_dio(XB_DIO12, out);
            pthread_mutex_lock(&stdio_lock);

            if(ret != XB_OK) {
                printf("command DIO failure\n");
            } else {
                printf("command complete\n");
            }
        }
    } else if(strcmp(cmd, "local_vtx") == 0) {
        // video transmitter command
        xb_dio_output_t out;
        uint8_t good = 1;

        if(strcmp(rest, "on") == 0) {
            printf("turning local VTX on\n");
            out = XB_DIO_HIGH;
        } else if(strcmp(rest, "off") == 0) {
            printf("turning local VTX off\n");
            out = XB_DIO_LOW;
        } else {
            printf("error: valid parameters for vtx are [on / off]\n");
            good = 0;
        }

        if(good) {
            pthread_mutex_unlock(&stdio_lock);
            xb_ret_t ret = xb_cmd_dio(XB_DIO12, out);
            pthread_mutex_lock(&stdio_lock);

            if(ret != XB_OK) {
                printf("command DIO failure\n");
            } else {
                printf("command complete\n");
            }
        }
    } else {
        printf("unknown command\n");
    }

    pthread_mutex_unlock(&stdio_lock);
}

char tx_buff[1024];
size_t tx_i = 0;
void* tx_thread(void* unused) {
    pthread_mutex_lock(&stdio_lock);
    printf("> ");
    pthread_mutex_unlock(&stdio_lock);

    char c;
    while(1) {
        c = getchar();

        if(c == '\n') {
            tx_buff[tx_i] = 0;

            if(tx_i > 0) {
                parse_cmd(tx_buff);
            } // else the user just hit enter on a blank line

            tx_i = 0;

            pthread_mutex_lock(&stdio_lock);
            printf("\n> ");
            pthread_mutex_unlock(&stdio_lock);
        } else {
            tx_buff[tx_i++] = c;

            if(tx_i > 1023) {
                pthread_mutex_lock(&stdio_lock);
                printf("no more room in command buffer\n");
                printf("\n> ");
                pthread_mutex_unlock(&stdio_lock);

                tx_i = 0;
            }
        }
    }
}

int xbee_write(uint8_t* data, size_t len) {
    pthread_mutex_lock(&stdio_lock);

    printf("writing to XBee: ");

    for(size_t i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
    }
    printf("\n");

    pthread_mutex_unlock(&stdio_lock);

    return serial_write(data, len);
    //return len;
}

void packet_received(uint8_t* data, size_t len) {
    pthread_mutex_lock(&stdio_lock);

    printf("packet received: ");
    for(size_t i = 0; i < len; i++) {
        printf("0x%02x ", data[i]);
    }
    printf("\n");

    if(tx_i) {
        // the user was typing something and we interrupted them
        // write out what they were typing
        tx_buff[tx_i] = 0;
        printf("\n> %s", tx_buff);
    }

    pthread_mutex_unlock(&stdio_lock);
}

uint8_t rx_buff[1024];
void* rx_thread(void* unused) {
    int len;

    while(1) {
        len = serial_read(rx_buff, 1024);

        if(len != 0) {
            pthread_mutex_lock(&stdio_lock);
            printf("\nreceived data on serial: 0x");
            for(size_t i = 0; i < len; i++) {
                printf("%02x", rx_buff[i]);
            }
            printf("\n");
            pthread_mutex_unlock(&stdio_lock);

            xb_raw_recv(rx_buff, len);
        }

        // sleep for 1ms
        usleep(1000);
    }
}

int main(int argc, char* argv[]) {
    if(argc != 2) {
        fprintf(stderr, "usage: ./xbee [path to device file]\n");
        return -1;
    }

    if(serial_init(argv[1]) != 0) {
        fprintf(stderr, "failed to initialize serial\n");
        return -1;
    }

    xb_attach_rx_callback(&packet_received);

    if (pthread_mutex_init(&stdio_lock, NULL) != 0) {
        fprintf(stderr, "\n mutex failed to initialize\n");
        return -1;
    }

    int error;

    pthread_t rx_tid;
    error = pthread_create(&rx_tid, NULL, &rx_thread, NULL);

    if(error != 0) {
        fprintf(stderr, "failed to create rx thread\n");
    }

    pthread_t tx_tid;
    error = pthread_create(&tx_tid, NULL, &tx_thread, NULL);

    if(error != 0) {
        fprintf(stderr, "failed to create tx thread\n");
    }

    //pthread_join(rx_tid, NULL);
    pthread_join(tx_tid, NULL);
}
