#include <stdint.h>
#include <stddef.h>
#include "log.h"
#include "uart.h"
#include "pinmux.h"
#include "traps.h"
#include "platform.h"
#include "plic_driver.h"
#include "lte.h"

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

int read_output_uart2(uart_struct * uart_instance, char *str)
{
   int i=0;
   do
   {
      str[i]=u2_rcv_char[i];
   }while(str[i++]=='$')
   i--;
   str[i]='\0';
   uart2_rcv_isr_count = 0;
   memset(u2_rcv_char,'\0', UARTX_BUFFER_SIZE);
   return 1;
}

int str_length(char *str)
{
    int len=0;
    while(*str!='\0')
    {
        len++;
        str++;
    }
    return len;
}

int isvalid_num(char *mob)
{
    if(str_length(mob)!=10)
    {
        log_info("\nINVALID_NUMBAR");
        return 0;
    }
    for(int i=0;i<10;i++)
    {
        if(i==0)
        {
            if(mob[0]<'5' || mob[i]>'9')
            {
                log_info("\nINVALID_NUMBAR");
                return 0;
            }
        }
        else
        {
            if(mob[i]<'0' || mob[i]>'9')
            {
                log_info("\nINVALID_NUMBAR");
                return 0;
            }
        }
    }
    return 1;
}


int main()
 {	
   *pinmux_config_reg = 0x55;
   char mobile[10]="XXXXXXXXXX";
   char msg[150];
   set_baud_rate(UART_INST_0, 500000);
   set_baud_rate(UART_INST_1, 115200); 
   set_baud_rate(UART_INST_2, 115200); 
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
   if(setup(UART_INST_1))
   {
      log_info("\nSETUP DONE\n");
   }
   else
   {
      log_info("\n------------------ERROR------------------------");
      return -1;
   }
   while(1)
   {
      if(uart2_rcv_isr_count >= 10)
      {  
         delay(1);
         read_output_uart2(UART_INST_2,10,mobile);
         log_info("\nMobile Number: %s\n",mobile);

         read_output_uart2(UART_INST_2,150,mobile);
         log_info("\nMobile Number: %s\n",mobile);

         if(isvalid_num(mobile))
         {
            send_sms(UART_INST_1,mobile,MSG);
         }
         else
         {
            log_info("\n------------------ERROR------------------------");
            delay(10);
            log_info("\n------------------RETRY------------------------");
         }
      }
      log_info("\n%d",uart2_rcv_isr_count);
      delay(5);
   }  
   return 0; 
 }