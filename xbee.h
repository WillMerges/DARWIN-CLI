/*
*   XBee API mode external functions
*
*   Will Merges @ RIT Launch Initiative
*   2022
*/
#ifndef XBEE_H
#define XBEE_H

#include <stdint.h>
#include <stdlib.h>

#define XBEE_BROADCAST_ADDR 0x000000000000FFFF

typedef enum {
    XB_OK,
    XB_ERR
} xb_ret_t;

// init function
// 'write' is a function that the API will use to output data to the XBee
// 'delay' is a function that delays for a given amount of milliseconds
// NOTE: places the XBee in API mode
xb_ret_t xb_init(int (*write) (uint8_t* buff, size_t len), void (*delay)(uint32_t ms));

/*
// TODO move to a way that exposes underlying buffer so it can be directly written to
// buffer to copy received data into (up to recv_free bytes)

extern uint8_t* recv_buffer;
extern size_t recv_free;


// request a buffer of 'len' bytes to write received data into
// a subsequent call to 'xb_rx_parse' will parse the latest data in the buffer
uint8_t* xb_rx_buff(size_t len)

// function that should be called when any data is received from the XBee (either over serial or SPI)
// needs to be called by lower layer
// parses the data in the last call to 'xb_rx_buff'
void xb_rx_parse();
*/

// function that should be called when any data is received from the XBee (either over serial or SPI)
// needs to be called by lower layer
void xb_raw_recv(uint8_t* buff, size_t len);

// attach a callback function to call when an rx frame is received
// 'buff' points to the payload of length 'len' in the frame
void xb_attach_rx_callback(void (*rx)(uint8_t* buff, size_t len));

// set the default destination address of transmitted packets
// assumes 'addr' is in system endianness
// NOTE: default address is broadcast
void xb_set_default_dst(uint64_t addr);

// transmit data to 'addr' (in network order)
xb_ret_t xb_sendto(uint64_t addr, uint8_t* data, size_t len);

// transmit data to default destination
xb_ret_t xb_send(uint8_t* data, size_t len);

// set network ID, valid IDs range from 0 to 0x7FFF
// devices on the same network ID are allowed to communicate with eachother
// NOTE: netid assumed to be in system endianness
xb_ret_t xb_set_net_id(uint16_t id);

typedef enum {
    XB_DIO12
} xb_dio_t;

typedef enum {
    XB_DIO_HIGH,
    XB_DIO_LOW
} xb_dio_output_t;

// remotely command digital I/O
xb_ret_t xb_cmd_remote_dio(xb_dio_t dio, xb_dio_output_t output);

// command local digital I/O
xb_ret_t xb_cmd_dio(xb_dio_t dio, xb_dio_output_t output);

#endif
