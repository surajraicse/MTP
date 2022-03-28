***************************************************************************
 * Project           			:  vc108
 * Name of the file	     		:  uart_interrupt.c
 * Created date			        :  19.10.2021
 * Brief Description of file            :  Interrupt test for GPIO
 * Name of Author    	                :  Akshaya B
 * Email ID                             :  akshayabarat@gmail.com

 Copyright (C) 2019  IIT Madras. All rights reserved.

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <https://www.gnu.org/licenses/>.

 ***************************************************************************/

/**
@file
@brief
@detail
*/
#include "gpio.h"
#include "uart.h"
#include "traps.h"
#include "platform.h"
#include "plic_driver.h"
#include "log.h"
#include "defines.h"
#include "pwm_driver.h"

#define GPIO_OUTPUTS GPIO0
extern volatile uint64_t* mtime;

uint8_t  intr_sts = 0;
uint64_t time_val1 = 0, time_val2 = 0;

/** @fn gpio_isr
 * @brief
 * @details
 * @warning
 * @param[in]
 * @param[Out]
 */
unsigned gpio_isr (unsigned num)
{
//	log_info("GPIO %x (active high) interrupt occured\n", (num - GPIO_INTERRUPT_OFFSET) );
	switch(num - GPIO_INTERRUPT_OFFSET)
	{
		case 1:
			if( (gpio_instance->data & 0x02) != 0)
			{
				time_val1 = *mtime;
				gpio_instance->clear = GPIO0;
				intr_sts |= 0x01;
				gpio_instance->intr_config = 0x02; //Configure GPIO1 as active low interrupt
			}
			else
			{
				time_val2 = *mtime;
//				gpio_instance->set = GPIO0;
				intr_sts |= 0x02;
				gpio_instance->intr_config = 0x02; //Configure GPIO1 as active low interrupt
			}

			break;
	}
	return 0;
}

/** @fn main
 * @brief
 * @details
 * @warning
 * @param[in]
 * @param[Out]
 */
int main(void){
	unsigned int int_id = 1;
	register uint32_t retval;
	int i;

    
	printf("\n GPIO Active High even Interrupt Test code");
	printf("\n GPIO_INTERRUPT_OFFSET: %x; PLIC_MAX_INTERRUPT_SRC: %x; \
	Intr.Src: %x", \
	GPIO_INTERRUPT_OFFSET, PLIC_MAX_INTERRUPT_SRC, int_id);
	gpio_init();
	plic_init();

	gpio_instance->direction = GPIO_OUTPUTS; //GPIO1 = output, GPIO 0=input,
	printf("\n Direction reg: %x", gpio_instance->direction);
	gpio_instance->clear = GPIO_OUTPUTS;  //Make gpio1 (output) as low
	printf("\n After clearing gpios %x", gpio_instance->data );


	gpio_instance->intr_config = 0x00; //Make interrupts active High, 0 to 1 transition
	configure_interrupt(PLIC_INTERRUPT_7); //configuring GPIO 0 Interupt
	isr_table[PLIC_INTERRUPT_7] = gpio_isr;

	gpio_instance->clear = GPIO_OUTPUTS; 
	printf("\n After setting GPIO1 %x \n", gpio_instance->data );

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

	printf(" retval = %x\n", retval);

	asm volatile(
		     "csrr %[retval], mie\n"
		     :
		     [retval]
		     "=r"
		     (retval)
		    );

	printf(" retval = %x\n", retval);

	asm volatile(
		     "csrr %[retval], mip\n"
		     :
		     [retval]
		     "=r"
		     (retval)
		    );

	printf(" retval = %x\n", retval);

	while(1)
	{

			if(intr_sts == 1)
			{
				printf("gpio high interrupt occured");
				intr_sts &= 0xFE;
			}
			else (2 == intr_sts)
			{
				printf("gpio low interrupt occured");
				mtime_diff = mtime_val2 - mtime_val1; // Time between the high time and low time
				intr_sts &= 0xFD;
			}
				delay_loop(1000, 2000);
	}

	return 0;
}