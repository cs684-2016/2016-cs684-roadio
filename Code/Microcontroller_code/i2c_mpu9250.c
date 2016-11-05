#include <stdint.h>
#include <stdbool.h>
#include "inc/hw_memmap.h"
#include "inc/hw_ints.h"
#include "driverlib/debug.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/i2c.h"
#include "utils/uartstdio.h"
#include "sensorlib/hw_mpu9150.h"
#include "sensorlib/hw_ak8975.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/ak8975.h"
#include "sensorlib/mpu9150.h"
#include "sensorlib/comp_dcm.h"

//*****************************************************************************
//
// Define MPU9250 I2C Address (ADO connected to ground => LSb = 0).
//
//*****************************************************************************
#define MPU9250_I2C_ADDRESS     0x68

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
    //
    // Loop while there are more characters to send.
    //
    while(ui32Count--)
    {
        //
        // Write the next character to the UART.
        //
        UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
    }
}

void UARTConfig()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(GPIO_PA0_U0RX);
	GPIOPinConfigure(GPIO_PA1_U0TX);
	GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART0_BASE, SysCtlClockGet(), 115200, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));
}

int main(void) {

    uint8_t iter;
    uint32_t accel[6];
    // uint32_t accel_offset[6];
    uint32_t gyro[6];
    char accelDisp[6]; int i;

    //
    // Setup the system clock to run at 40 Mhz from PLL with crystal reference
    //
    ROM_SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_XTAL_16MHZ |
                       SYSCTL_OSC_MAIN);

    //
    // Initialize the UART.
    //
	UARTConfig();

    //
    // Print the welcome message to the terminal.
    //
	UARTSend("MPU9250 Example \n\r", 20);

    //
    // The I2C3 peripheral must be enabled before use.
    //
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    //
    // Configure the pin muxing for I2C3 functions on port D0 and D1.
    //
    ROM_GPIOPinConfigure(GPIO_PD0_I2C3SCL);
    ROM_GPIOPinConfigure(GPIO_PD1_I2C3SDA);

    //
    // Select the I2C function for these pins.  This function will also
    // configure the GPIO pins pins for I2C operation, setting them to
    // open-drain operation with weak pull-ups.  Consult the data sheet
    // to see which functions are allocated per pin.
    //
    GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
    ROM_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

    //
    // Keep only some parts of the systems running while in sleep mode.
    // GPIOB is for the MPU9150 interrupt pin.
    // UART0 is the virtual serial port
    // I2C3 is the I2C interface to the ISL29023
    //
    ROM_SysCtlPeripheralClockGating(true);
//    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_I2C3);
//
//    //
//    // Enable interrupts to the processor.
//    //
//    ROM_IntMasterEnable();
//
    //
    // Initialize I2C3 peripheral.
    //
    I2CMasterInitExpClk(I2C3_BASE, SysCtlClockGet(), true);

	while(1){

		UARTSend("Accelerometer data \n\r", 21);

	    for(iter = 0; iter < 6; iter++)
	    {
			I2CMasterSlaveAddrSet(I2C3_BASE, MPU9250_I2C_ADDRESS, false);
			I2CMasterDataPut(I2C3_BASE, (uint8_t)(59+iter));
			I2CMasterControl(I2C3_BASE, I2C_MASTER_CMD_SINGLE_SEND);
			while(I2CMasterBusy(I2C3_BASE));
			I2CMasterSlaveAddrSet(I2C3_BASE, MPU9250_I2C_ADDRESS, true);
			I2CMasterControl(I2C3_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
			while (I2CMasterBusy(I2C3_BASE)); //Wait till end of transaction
			accel[iter] = I2CMasterDataGet (I2C3_BASE); //Read from FIFOFINISH); //Read the 2nd Byte

			int a = accel[iter];
			for(i=0;i<6;i++)
			{
				int d = a%10;
				accelDisp[5-i] = (char)(d + '0');
				a = a/10;
			}
			for(i=0;i<6;i++)
				UARTCharPut(UART0_BASE, accelDisp[i]);
			UARTCharPut(UART0_BASE, ' ');
			//UARTSend(accelDisp, 6);
	    }
	    UARTSend("\n\r", 2);

	    UARTSend("Gyroscope data \n\r", 17);

	    for(iter = 0; iter < 6; iter++)
		{
			I2CMasterSlaveAddrSet(I2C3_BASE, MPU9250_I2C_ADDRESS, false);
			I2CMasterDataPut(I2C3_BASE, (uint8_t)(67+iter));
			I2CMasterControl(I2C3_BASE, I2C_MASTER_CMD_SINGLE_SEND);
			while(I2CMasterBusy(I2C3_BASE));
			I2CMasterSlaveAddrSet(I2C3_BASE, MPU9250_I2C_ADDRESS, true);
			I2CMasterControl(I2C3_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
			while (I2CMasterBusy(I2C3_BASE)); //Wait till end of transaction
			gyro[iter] = I2CMasterDataGet (I2C3_BASE); //Read from FIFOFINISH); //Read the 2nd Byte

			int a = gyro[iter];
			for(i=0;i<6;i++)
			{
				int d = a%10;
				accelDisp[5-i] = (char)(d + '0');
				a = a/10;
			}
			for(i=0;i<6;i++)
				UARTCharPut(UART0_BASE, accelDisp[i]);
			UARTCharPut(UART0_BASE, ' ');
		}
	    UARTSend("\n\r", 2);


//	    for(iter = 0; iter < 6; iter++)
//		{
//			I2CMasterSlaveAddrSet(I2C3_BASE, MPU9250_I2C_ADDRESS, false);
//			I2CMasterDataPut(I2C3_BASE, (uint8_t)(19+iter));
//			I2CMasterControl(I2C3_BASE, I2C_MASTER_CMD_SINGLE_SEND);
//			while(I2CMasterBusy(I2C3_BASE));
//			I2CMasterSlaveAddrSet(I2C3_BASE, MPU9250_I2C_ADDRESS, true);
//			I2CMasterControl(I2C3_BASE, I2C_MASTER_CMD_SINGLE_RECEIVE);
//			while (I2CMasterBusy(I2C3_BASE)); //Wait till end of transaction
//			accel_offset[iter] = I2CMasterDataGet (I2C3_BASE); //Read from FIFOFINISH); //Read the 2nd Byte
//		}

		//
		// Go to sleep mode while waiting for data ready.
		//
//		while(!g_vui8I2CDoneFlag)
//		{
//			ROM_SysCtlSleep();
//		}

		//
		// Clear the flag
		//
//		g_vui8I2CDoneFlag = 0;

	}
}
