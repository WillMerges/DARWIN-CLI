/*
*  Interface to Linux serial device
*/
#include <stdint.h>
#include <stdlib.h>

int serial_write(uint8_t* data, size_t len);
int serial_read(uint8_t* buff, size_t len);
int serial_init(const char* dev_file);
void serial_close();
