#include <stdint.h>//Includes the definitions of standard input/output functions//
#include "i2c.h"
#include "log.h"

#define I2C i2c_instance[0]

#define SGP40_SLAVE_ADDR 0x59

// I2C commands available for SGP40
#define SGP40_MEASURE_RAW_MSB 0x26
#define SGP40_MEASURE_RAW_LSB 0x0F
#define SGP40_MEASURE_TEST_MSB 0x28
#define SGP40_MEASURE_TEST_LSB 0x0E
#define SGP40_HEATER_OFF_MSB 0x36
#define SGP40_HEATER_OFF_LSB 0x15


#define DELAY_VALUE 900
#define PRESCALER_COUNT 0x1F
#define SCLK_COUNT 0x91



int write_SGP40_cmd(i2c_struct * i2c_instance, unsigned int *write_value, unsigned long delay)
{
    i2c_instance->control = I2C_IDLE;
    i2c_send_slave_address(i2c_instance, SGP40_SLAVE_ADDR, I2C_WRITE, delay);
    
    log_debug("\n write value: %x", write_value[0]);
	i2c_write_data(i2c_instance,  ( write_value[0] & 0xff), delay);

    log_debug("\n write value: %x", write_value[1]);
	i2c_write_data(i2c_instance,  ( write_value[1] & 0xff), delay);

    // add delay of min 25 ms and max 30 ms
}


int read_SGP40_data(i2c_struct * i2c_instance, unsigned char *readBuff, unsigned long delay)
{
    unsigned char data[2];
    unsigned char rec_crc;
    i2c_instance->control = I2C_IDLE;
    i2c_send_slave_address(i2c_instance, SGP40_SLAVE_ADDR, I2C_READ, delay);

    i2c_read_data(i2c_instance, &data[0], delay);
    log_debug("\n read MSB data: %x", data[0]);
    
    i2c_read_data(i2c_instance, &data[1], delay);
    log_debug("\n read MSB data: %x", data[1]);

    log_debug("\nSending nack");
	i2c_instance->control = I2C_NACK;

    i2c_read_data(i2c_instance, &rec_crc, delay);
    log_debug("\n read CRC: %x", rec_crc);

    readBuff[0]=data[0];
    readBuff[1]=data[1];
    readBuff[2]=rec_crc;

    return 0;
}

uint8_t CalcCrc(uint8_t data[2]) 
{
    uint8_t crc = 0xFF;
    for(int i = 0; i < 2; i++) 
    {
        crc ^= data[i];
        for(uint8_t bit = 8; bit > 0; --bit) 
        {
            if(crc & 0x80) 
                crc = (crc << 1) ^ 0x31u;
                
            else 
                crc = (crc << 1);
        }
    }
    return crc;
}

int main()
{
    unsigned long delay = 500;
    log_debug("\tI2C: SGP40 \n");

    i2c_init();

    if(config_i2c(I2C, PRESCALER_COUNT, SCLK_COUNT))
    {
        log_error("\tSomething Wrong In Initialization\n");
        return -1;
    }
    else
    {
        log_info("\t Initialization has been done Successfully");
        unsigned int SGP40_measure_cmd[2];
        unsigned char readBuff[3];
        uint8_t data[2];

        SGP40_measure_cmd[0]=SGP40_MEASURE_RAW_MSB;
        SGP40_measure_cmd[1]=SGP40_MEASURE_RAW_LSB;
        
        write_SGP40_cmd(I2C, SGP40_measure_cmd, delay);
        read_SGP40_data(I2C, readBuff, delay);

        data[0]=(uint8_t)readBuff[0]; 
	    data[1]=(uint8_t)readBuff[1];
        
        if((uint8_t)readBuff[2]==CalcCrc(data))
        {
            log_info("Data is : %x %x",data[0],data[1]);
        }
        else
        {
            log_error("\n Read data failed due to noise.");
            return -1;
        }
        
    }
    return 0;
}