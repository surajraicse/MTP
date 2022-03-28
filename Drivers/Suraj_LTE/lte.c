#include "lte.h"

int write_cmd(uart_struct * uart_instance, char *str)
{
    while(*str!='\0')
    {
       write_uart_character(uart_instance,*str);
       str++;
    }
    write_uart_character(uart_instance,'\r');
}

int check_output(uart_struct * uart_instance, char *st)
{
   delay(1);
   unsigned char * uart_rcv_char;
   unsigned int * uart_rcv_isr_count=0;
   if(uart_instance == uart_instance[0])
   {
       uart_rcv_char=u0_rcv_char;
       *uart_rcv_isr_count=uart0_rcv_isr_count;
   }
   else if(uart_instance == uart_instance[1])
   {
       uart_rcv_char=u1_rcv_char;
       *uart_rcv_isr_count=uart1_rcv_isr_count;
   }
   else
   {
       uart_rcv_char=u2_rcv_char;
       *uart_rcv_isr_count=uart2_rcv_isr_count;
   }
   uart_rcv_isr_count++;
   uart_rcv_char[*uart_rcv_isr_count] = '\0';
   if(strstr(uart_rcv_char,st)==NULL)
   {
       *uart_rcv_isr_count = 0;
	   memset(uart_rcv_char,'\0', UARTX_BUFFER_SIZE);
       return 0;
   }
   *uart_rcv_isr_count = 0;
   memset(uart_rcv_char,'\0', UARTX_BUFFER_SIZE);
   return 1;
}

int setup(uart_struct * uart_instance)
{
   write_cmd(uart_instance,"AT+CGEREP=0");
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nDisabled unsolicited result code");
   }
   else
   {
      log_info("\nAT+CGEREP command failed");
      return 0;
   }
   write_cmd(uart_instance,"AT");
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nCommunication Verified");
   }
   else
   {
      log_info("\nAT command failed");
      return 0;
   }

   write_cmd(uart_instance,"AT+CPIN?");
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nCPIN Success");
   }
   else
   {
      log_info("\nAT+CPIN? command failed");
      return 0;
   }

   write_cmd(uart_instance,"AT+CSQ");
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nSignal strength:OK");
   }
   else
   {
      log_info("\nAT+CSQ command failed");
      return 0;
   }
   
   write_cmd(uart_instance,"AT+CREG?");
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nReg to Circuit Switching Domain");
   }
   else
   {
      log_info("\nAT+CREG? command failed");
      return 0;
   }

   write_cmd(uart_instance,"AT+CGREG?");
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nReg to Packet  Switching Domain");
   }
   else
   {
      log_info("\nAT+CGREG? command failed");
      return 0;
   }

   #ifdef CHECK_CPSI

        write_cmd(uart_instance,"AT+CPSI?");
        if(read_output(uart_instance,"OK"))
        {
            log_info("\nUE System Info : OK");
        }
        else
        {
            log_info("\nAT+CPSI? command failed");
            return 0;
        }
   
   #endif

   return 1;
}


int send_sms(uart_struct * uart_instance, char *mobile ,char *text)
{
   char str[]="AT+CMGS=\"+91XXXXXXXXXX\"";
   for(int i=0;i<10;i++)
   {
      str[12+i]=mobile[i];
   }
   log_info("\n%s",str);
   write_cmd(uart_instance,"AT+CMGF=1");
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nText Mode Selected");
   }
   else
   {
      log_info("\nAT+CMGF command failed");
      return 0;
   }
   write_cmd(uart_instance,str);
   delay(10);
   write_cmd(uart_instance,text);
   delay(10);
   write_uart_character(uart_instance,0x1A);
   write_uart_character(uart_instance,'\r');
   delay(10);
   if(check_output(uart_instance,"OK"))
   {
      log_info("\nSMS sent to: %s",str+8);
      log_info("\nMsg: %s",text);
   }
   else
   {
      log_info("\nAT+CMGS command failed");
      return 0;
   }
   return 1;
}
