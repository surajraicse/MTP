#include <stdint.h>
#include "i2c.h"
#include "log.h"


#define I2C i2c_instance[0]

#define PRESCALER_COUNT 24
#define SCLK_COUNT 9
#define DELAY1 100
#define DELAY2 50

#define MPU9250_SLAVE_ADDR 0xD0
#define DEVICE_ID 0x71
#define MPU9250_REG_WHO_AM_I 0x75 
#define MPU9250_REG_PWR_MGMT_1 0x6B
#define MPU9250_REG_USER_CTRL 0x6A
#define MPU9250_REG_INT_PIN_CFG 0x37
#define MPU9250_REG_CONFIG 0x1A
#define MPU9250_REG_GYRO_CONFIG 0x1B
#define MPU9250_REG_ACCEL_CONFIG_1 0x1C
#define MPU9250_REG_ACCEL_CONFIG_2 0x1D

#define MPU9250_REG_CNTL1 0x0A

#define CLK_SEL 0X01

// Gyro Full Scale Select:
#define GYRO_FS_SEL 0x08
// 0x00    +250  dps
// 0x08    +500  dps
// 0x10    +1000 dps
// 0x18    +2000 dps

// Accel Full Scale Select:
#define ACCEL_FS_SEL 0x08     
// 0x00      +/-2  g
// 0x08      +/-4  g
// 0x10      +/-8  g
// 0x18      +/-16 g

#define ACCEL_DLPF 0x05 //Turn on the internal low-pass filter for accelerometer with 10Hz bandwidth
#define GYRO_DLPF 0x05  //Turn on the internal low-pass filter for gyroscope with 10Hz bandwidth
#define BYPASS_ENABLE 0x02

#define RD_DATA_COUNT 14
#define MPU9250_REG_ACCEL_XOUT_H 0x3B


int read_mpu9250_register(i2c_struct * i2c_instance, unsigned char reg_offset, unsigned char *read_data,  unsigned long delay)
{
    unsigned char readBuf=0;
    unsigned char dummyRead=0; 
    i2c_send_slave_address(i2c_instance, MPU9250_SLAVE_ADDR, I2C_WRITE, 800);
    
    i2c_write_data(i2c_instance, reg_offset, delay);
    
    i2c_instance->control = I2C_REPSTART;
    
    delay_loop(DELAY1, DELAY2);
    
    i2c_write_data(i2c_instance, MPU9250_SLAVE_ADDR | I2C_READ, delay);

    //Dummy read
	i2c_read_data(i2c_instance, &dummyRead, delay);
    
    i2c_instance->control = I2C_NACK;

    i2c_read_data(i2c_instance, &readBuf, delay);

	*read_data = readBuf;

	i2c_instance->control = I2C_STOP;
	
    delay_loop(DELAY1, DELAY2);
	return 0;
}

int read_mpu9250_registers(i2c_struct * i2c_instance, unsigned char reg_offset, unsigned char *readData, unsigned long length, unsigned long delay)
{
	unsigned long i=1;
    unsigned char dummyRead = 0;
    unsigned char temp=0;

    i2c_instance->control = I2C_IDLE;

    i2c_send_slave_address(i2c_instance, MPU9250_SLAVE_ADDR, I2C_WRITE, delay);

    i2c_write_data(i2c_instance, reg_offset, delay);

    i2c_instance->control = I2C_REPSTART;
    
    delay_loop(DELAY1,DELAY2);

    i2c_write_data(i2c_instance, MPU9250_SLAVE_ADDR | I2C_READ, delay);

    i2c_read_data(i2c_instance, &dummyRead, delay);
    
    while( i < length )
    {
        if( i == length )
        {
            log_debug("\nSending nack");
			i2c_instance->control = I2C_NACK;
        }

        i2c_read_data(i2c_instance, &temp, delay);
        
        log_debug("\n read data[%x]: %x", i, temp);
		*readData = temp;
		readData++;
        
        temp=0;
        i++;
    }

	log_debug("\n Sending Stop condition");
	i2c_instance->control = I2C_STOP;
	return 0;
}

int  write_mpu9250_register(i2c_struct * i2c_instance, unsigned char reg_offset, unsigned int write_value, unsigned char delay)
{
    i2c_instance->control = I2C_IDLE;
	delay_loop(DELAY1, DELAY2);

	i2c_send_slave_address(i2c_instance, MPU9250_SLAVE_ADDR, I2C_WRITE, delay);

	i2c_write_data(i2c_instance, reg_offset, delay);

	log_debug("\n write value: %x", write_value);

	i2c_write_data(i2c_instance,  ( write_value & 0xff), delay);

	i2c_instance->control = I2C_STOP;
	delay_loop(DELAY1, DELAY2);
}


unsigned char CheckForDeviceId(i2c_struct * i2c_instance, unsigned long delay)
{
    unsigned char readBuf= 0xFF;
    read_mpu9250_register(i2c_instance, MPU9250_REG_WHO_AM_I, &readBuf, delay);
    log_debug("\n Read Buf Value: %x", readBuf);
    return readBuf;
}

int ConfigureMultiChip(i2c_struct* i2c_instance, unsigned long delay)
{
    // Setting the Accel scale
    write_mpu9250_register(i2c_instance, MPU9250_REG_ACCEL_CONFIG_1, ACCEL_FS_SEL, delay);
    // Setting the Gyro scale
    write_mpu9250_register(i2c_instance, MPU9250_REG_GYRO_CONFIG, GYRO_FS_SEL, delay);
    // Setting DLPF for Accelerometer
    write_mpu9250_register(i2c_instance, MPU9250_REG_ACCEL_CONFIG_2, ACCEL_DLPF, delay);
    // Setting DLPF for Gyroscope
    write_mpu9250_register(i2c_instance, MPU9250_REG_CONFIG, GYRO_DLPF, delay);

    return 0;
}


void ConfigureMagneto(i2c_struct* i2c_instance, unsigned long delay)
{
    //Disabling the I2C master interface
    write_mpu9250_register(i2c_instance, MPU9250_REG_USER_CTRL, 0x00, delay);
    //Turning on the bypass multiplexer
    write_mpu9250_register(i2c_instance, MPU9250_REG_INT_PIN_CFG, BYPASS_ENABLE, delay);
    
    return 0;
}

int readMultiChip(i2c_struct* i2c_instance, unsigned long delay)
{
    unsigned char readData[RD_DATA_COUNT];
    int AccX,AccY,AccZ;
    int GyroX,GyroY,GyroZ;
    read_mpu9250_registers(i2c_instance , MPU9250_REG_ACCEL_XOUT_H, readData, RD_DATA_COUNT, delay);
    log_info("\n Read Data Buf: { ");
	for(int i = 0; i < RD_DATA_COUNT; i++)
	{
		log_info(" %x, ", readData[i] );
	}
	log_info(" }\n" ); 

    AccX = (readData[0]<<8 | readData[1]);
    AccY = (readData[2]<<8 | readData[3]);
    AccZ = (readData[4]<<8 | readData[5]);

    GyroX= (readData[8]<<8 | readData[9]);
    GyroY= (readData[10]<<8 | readData[11]);
    GyroZ= (readData[12]<<8 | readData[13]);

    log_info("\n Accel_X : %d ", AccX);
    log_info("\n Accel_Y : %d ", AccY);
    log_info("\n Accel_Z : %d \n", AccZ);

    log_info("\n Gyro_X : %d ", GyroX);
    log_info("\n Gyro_Y : %d ", GyroY);
    log_info("\n Gyro_Z : %d \n", GyroZ);
    return 0;
}

int main()
{
    unsigned long delay = 500;
    log_debug("\tI2C: MPU9250 \n");

    i2c_init();

    if(config_i2c(I2C, PRESCALER_COUNT, SCLK_COUNT))
    {
        log_error("\tSomething Wrong In Initialization\n");
        return -1;
    }
    else
    {
        log_info("\n\n Initialization has been done Successfully\n\n");
        if(DEVICE_ID == CheckForDeviceId(I2C,delay))
		{
			log_info("Device ID Check Success\n\n");
		}
        else
        {
            log_error("Could not Verify Device Identity");
            return -1;
        }

        write_mpu9250_register(I2C, MPU9250_REG_PWR_MGMT_1, CLK_SEL, delay);
        ConfigureMultiChip(I2C, delay);
        while(1)
        {
            readMultiChip(I2C, delay);
            delay_loop(5000, 1000);
        }

    }

    return 0;
}

