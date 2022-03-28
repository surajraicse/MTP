#include <stdint.h>
#include <stddef.h>
#include "log.h"
#include "uart.h"
#include "pinmux.h"
#include "traps.h"
#include "platform.h"
#include "plic_driver.h"


#include <string.h> // For memset
#include "GPS.h"
#include "LPC8xx.h"
#include "core_cm0plus.h"
#include "board.h"
#include "syscon.h"
#include "gprsmp3v01.h"
#include "utilities.h"
#include "smsinfo.h"



#define UART_INST_0 uart_instance[0]
#define UART_INST_1 uart_instance[1]
#define UART_INST_2 uart_instance[2]

#define USE_INTERRUPT

extern volatile unsigned int* pinmux_config_reg;


void init_variables_uart0()
{
    uart0_complete = 0;
	uart0_tx_isr_count = 0;
	uart0_rcv_isr_count = 0;
}

void init_variables_uart1()
{
    uart1_complete = 0;
	uart1_tx_isr_count = 0;
	uart1_rcv_isr_count = 0;
	configure_interrupt(PLIC_INTERRUPT_26);
	isr_table[PLIC_INTERRUPT_26] = uart1_isr;
}

void init_variables_uart2()
{
    uart2_complete = 0;
	uart2_tx_isr_count = 0;
	uart2_rcv_isr_count = 0;
	configure_interrupt(PLIC_INTERRUPT_27);
	isr_table[PLIC_INTERRUPT_27] = uart2_isr;
}

int uart_r_data(uart_struct * uart_instance, uint8_t *str,int cnt)
{
   int i;
   for(i=0;i<cnt;i++)
   {
      str[i]=u2_rcv_char[i];
   }
   uart2_rcv_isr_count = 0;
   memset(u2_rcv_char,'\0', UARTX_BUFFER_SIZE);
   return 1;
}

int main()
 {	
   *pinmux_config_reg = 0x55;
   uint8_t *buf;
   set_baud_rate(UART_INST_1, 9600); 
   set_baud_rate(UART_INST_2, 9600); 
   plic_init();
   init_variables_uart1();
   init_variables_uart2();
   flush_uart(UART_INST_1);
   flush_uart(UART_INST_2);
   register uint32_t retval;

	// Enable Global (PLIC) interrupts.
	asm volatile("li      t0, 8\t\n"
		     "csrrs   zero, mstatus, t0\t\n"
		    );

	asm volatile("li      t0, 0x800\t\n"
		     "csrrs   zero, mie, t0\t\n"
		    );

	asm volatile(
		     "csrr %[retval], mstatus\n"
		     :
		     [retval]
		     "=r"
		     (retval)
		    );

	asm volatile(
		     "csrr %[retval], mie\n"
		     :
		     [retval]
		     "=r"
		     (retval)
		    );

	asm volatile(
		     "csrr %[retval], mip\n"
		     :

		     [retval]
		     "=r"
		     (retval)
		    );

   flush_uart(UART_INST_1);
   flush_uart(UART_INST_2);
   enable_uart_interrupts(UART_INST_1,  ENABLE_RX_NOT_EMPTY);    // | ENABLE_RX_FULL
   enable_uart_interrupts(UART_INST_2,  ENABLE_RX_NOT_EMPTY);

   uint32_t longitude,latitude;
   
   while(1)
   {
       while(uart1_rcv_isr_count==0);
       uart_r_data(UART_INST_1,buf);
       ParseGPS(buf);
       latitude=GPSData.latitude;
       longitude=GPSData.longitude;
       log_info("Lati:%d Long:%d",latitude,longitude);
   }
   return 0; 
 }
