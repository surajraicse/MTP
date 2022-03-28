#include <stdint.h>
#include <stddef.h>
#include "log.h"
#include "uart.h"

int write_cmd(uart_struct * uart_instance, char *str);
int check_output(uart_struct * uart_instance, char *st);
int setup(uart_struct * uart_instance);
int send_sms(uart_struct * uart_instance, char *mobile ,char *text);