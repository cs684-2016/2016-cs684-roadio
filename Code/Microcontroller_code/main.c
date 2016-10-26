/*
 * Team Id: 
 * Author List: Mustafa Lokhandwala, Shalaka Kulkarni, Umang Chhaparia
 * Filename: main.c
 * Functions: StringFromFResult(FRESULT), SysTickHandler(), ConfigureUART(), UARTSend(uint8_t*, uint32_t),
 * setup(), SD_init(), Timer_init(), GPIO_config(), US_Sensor(), Timer0IntHandler(), Timer2IntHandler(), UpISR(), DownISR() 
 * Global Variables: count, distance, recvdStr, gprmc, lat, lon, i, j, k, speed, flag, ctr, c, index, a, tp, iter, accel, gyro,
 * iFResult, fil, buff_data, buff_len, tempstr, sdindex.
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
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
#include "inc/tm4c123gh6pm.h"
#include "sensorlib/hw_ak8975.h"
#include "sensorlib/i2cm_drv.h"
#include "sensorlib/ak8975.h"
#include "sensorlib/mpu9150.h"
#include "sensorlib/comp_dcm.h"
#include "driverlib/timer.h"
#include <time.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/fpu.h"
#include "driverlib/systick.h"
#include "grlib/grlib.h"
#include "utils/uartstdio.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"

//*****************************************************************************
//
// Define MPU9250 I2C Address (ADO connected to ground => LSb = 0).
//
//*****************************************************************************
#define MPU9250_I2C_ADDRESS     0x68

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary data from
// the SD card.  There are two buffers allocated of this size.  The buffer size
// must be large enough to hold the longest expected full path name, including
// the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE           80

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE            64

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.  Initially
// it is root ("/").
//
//*****************************************************************************
static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code, and a
// string representation.  FRESULT codes are returned from the FatFs FAT file
// system driver.
//
//*****************************************************************************
typedef struct
{
	FRESULT iFResult;
	char *pcResultStr;
}
tFResultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and it's
// name as a string.  This is used for looking up error codes for printing to
// the console.
//
//*****************************************************************************
tFResultString g_psFResultStrings[] =
{
		FRESULT_ENTRY(FR_OK),
		FRESULT_ENTRY(FR_DISK_ERR),
		FRESULT_ENTRY(FR_INT_ERR),
		FRESULT_ENTRY(FR_NOT_READY),
		FRESULT_ENTRY(FR_NO_FILE),
		FRESULT_ENTRY(FR_NO_PATH),
		FRESULT_ENTRY(FR_INVALID_NAME),
		FRESULT_ENTRY(FR_DENIED),
		FRESULT_ENTRY(FR_EXIST),
		FRESULT_ENTRY(FR_INVALID_OBJECT),
		FRESULT_ENTRY(FR_WRITE_PROTECTED),
		FRESULT_ENTRY(FR_INVALID_DRIVE),
		FRESULT_ENTRY(FR_NOT_ENABLED),
		FRESULT_ENTRY(FR_NO_FILESYSTEM),
		FRESULT_ENTRY(FR_MKFS_ABORTED),
		FRESULT_ENTRY(FR_TIMEOUT),
		FRESULT_ENTRY(FR_LOCKED),
		FRESULT_ENTRY(FR_NOT_ENOUGH_CORE),
		FRESULT_ENTRY(FR_TOO_MANY_OPEN_FILES),
		FRESULT_ENTRY(FR_INVALID_PARAMETER),
};

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES       (sizeof(g_psFResultStrings) /                 \
		sizeof(tFResultString))

char* uint32ToStr(uint32_t a);
void DownISR();
void UpISR();
void Timer_init();
void GPIO_config();
void US_Sensor();
volatile uint32_t count=0, distance=0;
volatile uint32_t ui32Period,ui32Period1;

//****variables of interest for gps****//
char recvdStr[100] = "";
char gprmc[6] = "$GPRMC";
int i=0; int j=0; int k=0;
char lat[9] = "";
char lon[10] = "";
char speed[5] = "";
int flag=1;
int ctr = 0;
char c;
int index=0; int a=0;
int tp=0;

//****variables of interest for accel****//
uint8_t iter;
uint32_t accel[6];
uint32_t gyro[6];

//****variables of interest for sd card****//
FRESULT iFResult;
FIL fil;
unsigned int buff_len = 28;
unsigned int buff_written;
char buff_data[55];
const char* tempstr;
int sdindex = 0;

//*****************************************************************************
//
// This function returns a string representation of an error code that was
// returned from a function call to FatFs.  It can be used for printing human
// readable error messages.
//
//*****************************************************************************
const char *
StringFromFResult(FRESULT iFResult)
{
	uint_fast8_t ui8Idx;

	//
	// Enter a loop to search the error code table for a matching error code.
	//
	for(ui8Idx = 0; ui8Idx < NUM_FRESULT_CODES; ui8Idx++)
	{
		//
		// If a match is found, then return the string name of the error code.
		//
		if(g_psFResultStrings[ui8Idx].iFResult == iFResult)
		{
			return(g_psFResultStrings[ui8Idx].pcResultStr);
		}
	}

	//
	// At this point no matching code was found, so return a string indicating
	// an unknown error.
	//
	return("UNKNOWN ERROR CODE");
}

//*****************************************************************************
//
// This is the handler for this SysTick interrupt.  FatFs requires a timer tick
// every 10 ms for internal timing purposes.
//
//*****************************************************************************
void
SysTickHandler(void)
{
	//
	// Call the FatFs tick timer.
	//
	disk_timerproc();
}

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void
__error__(char *pcFilename, uint32_t ui32Line)
{
}
#endif

//*****************************************************************************
//
// Configure the UART and its pins.  This must be called before UARTprintf().
//
//*****************************************************************************

/*
 * Function name: ConfigureUART()
 * Input: None
 * Output: None
 * Logic: Configures the UART and its pins.  This must be called before UARTprintf().
 * Example call: ConfigureUART()
 */

void ConfigureUART(void)
{
	// Enable the GPIO Peripheral used by the UART.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

	// Enable UART0
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);

	// Configure GPIO Pins for UART mode
	ROM_GPIOPinConfigure(GPIO_PA0_U0RX);
	ROM_GPIOPinConfigure(GPIO_PA1_U0TX);
	ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

	// Use the internal 16MHz oscillator as the UART clock source
	UARTClockSourceSet(UART0_BASE, UART_CLOCK_PIOSC);

	// Initialize the UART for console I/O
	UARTStdioConfig(0, 115200, 16000000);
}

/*
 * Function name: UARTSend(uint8_t*,  uint32_t)
 * Input: pui8Buffer -> char array to be sent via UART0, ui32Count -> length of buffer
 * Output: None
 * Logic: Sends string to UART0 (serial monitor)
 * Example call: UARTSend("Hello", 5);
 */

void UARTSend(const uint8_t *pui8Buffer, uint32_t ui32Count)
{
	while(ui32Count--)
		UARTCharPutNonBlocking(UART0_BASE, *pui8Buffer++);
}

/*
 * Function name: setup()
 * Input: none
 * Output: none
 * Logic: Configures all peripherals
 * Example call: setup();
 */

void setup()
{
	//System control clock
	SysCtlClockSet(SYSCTL_SYSDIV_5 | SYSCTL_USE_PLL | SYSCTL_OSC_MAIN | SYSCTL_XTAL_16MHZ);

	ConfigureUART(); // for serial terminal

	//UART1 config: GPS
	SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);
	GPIOPinConfigure(GPIO_PB0_U1RX);	//PB0 is connected to Rx of GPS
	GPIOPinConfigure(GPIO_PB1_U1TX);	//PB1 is connected to Tx of GPS
	GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
	UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), 9600, (UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE | UART_CONFIG_PAR_NONE));

	// The I2C3 peripheral must be enabled before use.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_I2C3);
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

	// Configure the pin muxing for I2C3 functions on port D0 and D1.
	ROM_GPIOPinConfigure(GPIO_PD0_I2C3SCL);
	ROM_GPIOPinConfigure(GPIO_PD1_I2C3SDA);

	// Select the I2C function for these pins.  This function will also
	// configure the GPIO pins pins for I2C operation, setting them to
	// open-drain operation with weak pull-ups.  Consult the data sheet
	// to see which functions are allocated per pin.
	GPIOPinTypeI2CSCL(GPIO_PORTD_BASE, GPIO_PIN_0);
	ROM_GPIOPinTypeI2C(GPIO_PORTD_BASE, GPIO_PIN_1);

	// Keep only some parts of the systems running while in sleep mode.
	// GPIOB is for the MPU9150 interrupt pin.
	// UART0 is the virtual serial port
	// I2C3 is the I2C interface to the ISL29023
	ROM_SysCtlPeripheralClockGating(true);
	//    ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_UART0);
	ROM_SysCtlPeripheralSleepEnable(SYSCTL_PERIPH_I2C3);

	// Initialize I2C3 peripheral.
	I2CMasterInitExpClk(I2C3_BASE, SysCtlClockGet(), true);
}

/*
 * Function name: SD_init()
 * Input: none
 * Output: none
 * Logic: Initialises SD card peripherals
 * Example call: SD_init();
 */

void SD_init()
{
	// Enable lazy stacking for interrupt handlers.  This allows floating-point
	// instructions to be used within interrupt handlers, but at the expense of
	// extra stack usage.
	ROM_FPULazyStackingEnable();

	// Enable the peripherals used by this example.
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_SSI0);

	// Configure SysTick for a 100Hz interrupt.  The FatFs driver wants a 10 ms
	// tick.
	ROM_SysTickPeriodSet(ROM_SysCtlClockGet() / 100);
	ROM_SysTickEnable();
	ROM_SysTickIntEnable();
}

/*
 * Function name: main()
 * Input: none
 * Output: none
 * Logic: Calls all initialisations; reads data from accelerometer, ultrasonic sensor and GPS module; writes data to SD card
 * Example call: Called automatically at start of execution by the microcontroller
 */

int main(void)
{
	setup();
	GPIO_config();
	SD_init();
	Timer_init();

	UARTprintf("New Set of Readings\n");

//	 Mount the file system, using logical disk 0.

	iFResult = f_mount(0, &g_sFatFs);
	if(iFResult != FR_OK)
	{
		UARTprintf("f_mount error: %s\n", StringFromFResult(iFResult));
		return(1);
	}

	iFResult = f_open(&fil, "datalog.txt", FA_WRITE | FA_OPEN_ALWAYS); // | FA_OPEN_APPEND);
	if(iFResult) UARTprintf("f_open error: %s\n", StringFromFResult(iFResult));
	else{
		iFResult = f_lseek(&fil, f_size(&fil));
		if(iFResult) UARTprintf("f_lseek error: %s\n", StringFromFResult(iFResult));
		else{
			iFResult = f_write(&fil, "###New Set of Readings###", 25, &buff_written);
			if(iFResult) UARTprintf("f_write error: %s\n", StringFromFResult(iFResult));
			else if(buff_written != 25){
				if(buff_written < buff_len) UARTprintf("Disk full, not all bytes were written\n");
				else UARTprintf("Error: more than required number of bytes written");
			}
			else {}
		}
		iFResult = f_close(&fil);
		if(iFResult) UARTprintf("f_close error: %s\n", StringFromFResult(iFResult));
	}

	while(1)
	{

		//*************get accel data***************//
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

			tempstr = uint32ToStr(accel[iter]);
			for(j=2;j>=0;j--,sdindex++)
				buff_data[sdindex] = tempstr[j];
			buff_data[sdindex] = ',';
			sdindex++;
		}

		//**************get ultrasonic data**************//
		US_Sensor();

		tempstr = uint32ToStr(distance);
		for(j=5;j>=3;j--,sdindex++)
			buff_data[sdindex] = tempstr[j];
		buff_data[sdindex] = ' ';
		sdindex = 0;

//		//***************gps read and print***************//
//
//		recvdStr[i] = UARTCharGet(UART1_BASE);
//		UARTCharPut(UART0_BASE,recvdStr[i]);
//		i++;
//
//		//on receiving the whole string
//		if(recvdStr[i-1]=='$')
//		{
//			recvdStr[0] = '$';
//			i = 1;
//			//if not the GPRMC string, read next
//			for(j=0;j<6;j++)
//				if(gprmc[j] != recvdStr[j])
//					goto outer;
//
//			//if GPRMC, get data
//			for(j=0;j<9;j++)
//			{
//				lat[j] = recvdStr[j+20];
//				buff_data[sdindex] = lat[j];
////				buff_data[sdindex] = 'g';
//				sdindex++;
//			}
//			buff_data[sdindex] = ','; sdindex++;
//
//			for(j=0;j<10;j++)
//			{
//				lon[j] = recvdStr[j+32];
//				buff_data[sdindex] = lon[j];
////				buff_data[sdindex] = 'g';
//				sdindex++;
//			}
//			buff_data[sdindex] = ','; sdindex++;

//			for(j=0;j<5;j++)
//			{
//				if(recvdStr[j] == ',')	break;
//				speed[j] = recvdStr[j+45];
//				buff_data[sdindex] = speed[j];
////				buff_data[sdindex] = 'g';
//				sdindex++;
//			}
//			buff_data[sdindex] = ' '; sdindex++;
//			sdindex = 0;
//		}

//		for(; sdindex < buff_len-1; sdindex++) buff_data[sdindex] = 'g';
//		buff_data[sdindex] = ' ';

		/****************Write to SD card******************/

		iFResult = f_open(&fil, "datalog.txt", FA_WRITE | FA_OPEN_ALWAYS); // | FA_OPEN_APPEND);
		if(iFResult) UARTprintf("f_open error: %s\n", StringFromFResult(iFResult));
		else{
			iFResult = f_lseek(&fil, f_size(&fil));
			if(iFResult) UARTprintf("f_lseek error: %s\n", StringFromFResult(iFResult));
			else{
				iFResult = f_write(&fil, buff_data, buff_len, &buff_written);
				if(iFResult) UARTprintf("f_write error: %s\n", StringFromFResult(iFResult));
				else if(buff_written != buff_len){
					if(buff_written < buff_len) UARTprintf("Disk full, not all bytes were written\n");
					else UARTprintf("Error: more than required number of bytes written");
				}
				else {}
			}
			iFResult = f_close(&fil);
			if(iFResult) UARTprintf("f_close error: %s\n", StringFromFResult(iFResult));
		}

	}//end of while(1)
}

/*
 * Function name: Timer_init()
 * Input: none
 * Output: none
 * Logic: Initialises timers 0 and 2
 * Example call: Timer_init();
 */

void Timer_init()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER2);
	TimerConfigure(TIMER0_BASE, TIMER_CFG_ONE_SHOT);
	TimerConfigure(TIMER2_BASE, TIMER_CFG_ONE_SHOT);
	ui32Period = (SysCtlClockGet() / 50000) / 2;
	ui32Period1 = (SysCtlClockGet() / 50) / 2;
	TimerLoadSet(TIMER0_BASE, TIMER_A, ui32Period -1);
	TimerLoadSet(TIMER2_BASE, TIMER_A, ui32Period1 -1);
	IntEnable(INT_TIMER0A);
	IntEnable(INT_TIMER2A);
	TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	TimerIntEnable(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	IntMasterEnable();
}

/*
 * Function name: GPIO_config()
 * Input: none
 * Output: none
 * Logic: Configures GPIO pins
 * Example call: GPIO_config();
 */

void GPIO_config()
{
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOE);
	GPIOPinTypeGPIOOutput(GPIO_PORTE_BASE, GPIO_PIN_0);
	GPIOPinTypeGPIOInput(GPIO_PORTE_BASE, GPIO_PIN_1);
	GPIOPadConfigSet(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU); // set the pad configuration for pin 4 of PORT F i.e. SW1
	GPIOIntDisable(GPIO_PORTE_BASE, GPIO_PIN_1); // disable interrupts on SW1
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntRegister(GPIO_PORTE_BASE, UpISR); // register sw1DownISR() with the PORT F interrupt handler
	GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_RISING_EDGE); // set the interrupt type for SW1 as falling edge
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntEnable(GPIO_PORTE_BASE, GPIO_PIN_1); // enable SW1 interrupts
}

/*
 * Function name: US_Sensor()
 * Input: none
 * Output: none
 * Logic: Reads data from ultrasonic sensor and converts into distance in um
 * Example call: US_Sensor();
 */

void US_Sensor()
{
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x01);
	TimerEnable(TIMER0_BASE, TIMER_A);
	SysCtlDelay(150000); //wait(11.2ms)
	distance = (((ui32Period1-count)*10000)/ui32Period1)*170;
}

/*
 * Function name: Timer0IntHandler()
 * Input: none
 * Output: none
 * Logic: Interrupt handler for Timer0 interrupt
 * Example call: Called by default when interrupt occurs
 */

void Timer0IntHandler(void)
{
	// Clear the timer interrupt
	TimerIntClear(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	GPIOPinWrite(GPIO_PORTE_BASE, GPIO_PIN_0, 0x00);
	TimerDisable(TIMER0_BASE, TIMER_A);
}

/*
 * Function name: Timer2IntHandler()
 * Input: none
 * Output: none
 * Logic: Interrupt handler for Timer2 interrupt
 * Example call: Called by default when interrupt occurs
 */

void Timer2IntHandler(void)
{
	TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	TimerDisable(TIMER2_BASE, TIMER_A);
}

/*
 * Function name: UpISR()
 * Input: none
 * Output: none
 * Logic: Turns on Timer2 when interrupt is called on a rising edge of echo signal from ultrasonic sensor
 * Example call: Called by default when external interrupt occurs on rising edge on PE.1
 */

void UpISR(void)
{
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntDisable(GPIO_PORTE_BASE, GPIO_PIN_1);
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntRegister(GPIO_PORTE_BASE, DownISR); // register sw1UpISR() with the PORT F interrupt handler
	GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_FALLING_EDGE); // set the interrupt type for SW1 as rising edge
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntEnable(GPIO_PORTE_BASE, GPIO_PIN_1); // enable SW1 interrupts
	TimerEnable(TIMER2_BASE, TIMER_A);
}

/*
 * Function name: DownISR()
 * Input: none
 * Output: none
 * Logic: Turns off and reloads Timer2 when interrupt is called on a falling edge of echo signal from ultrasonic sensor
 * Example call: Called by default when external interrupt occurs on falling edge on PE.1
 */

void DownISR(void)
{
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntDisable(GPIO_PORTE_BASE, GPIO_PIN_1);
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntRegister(GPIO_PORTE_BASE, UpISR); // register sw1DownISR() with the PORT F interrupt handler
	GPIOIntTypeSet(GPIO_PORTE_BASE, GPIO_PIN_1, GPIO_RISING_EDGE); // set the interrupt type for SW1 as falling edge
	GPIOIntClear(GPIO_PORTE_BASE, GPIO_PIN_1); // clear the interrupt on SW1
	GPIOIntEnable(GPIO_PORTE_BASE, GPIO_PIN_1); // enable SW1 interrupts
	count = TimerValueGet(TIMER2_BASE, TIMER_A);
	TimerIntClear(TIMER2_BASE, TIMER_TIMA_TIMEOUT);
	TimerDisable(TIMER2_BASE, TIMER_A);
	TimerLoadSet(TIMER2_BASE, TIMER_A, ui32Period1 -1);
}

/*
 * Function name: uint32ToStr()
 * Input: a -> uint32_t value
 * Output: str -> string equivalent
 * Logic: Converts a 32bit unsigned integer to its equivalent string representation
 * Example call: char* strData = uint32ToStr(ui32data);
 */

char* uint32ToStr(uint32_t a)
{
	tp = 0;
	char str[10];
	while(a>0)
	{
		char d = (char)((a%10) + '0');
		a = a/10;
		str[tp] = d;
		tp++;
	}
	return str;
}
