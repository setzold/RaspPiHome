/*
 *  rfm12breceive.c: 
 *  Simple program receiving data from RFM12B via SPI in RaspBerryPi
 *  Build with: gcc -o rfm12b_receive  rfm12breceive.c hexconverter.c crccheck.c -lbcm2835 -I/usr/include/python2.7 -Wall -lpython2.7
 */
#include <Python.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <bcm2835.h>
#include <time.h>

#include "hexconverter.h"
#include "crccheck.h"
#include "httpconfig.h"

#define RFM_IRQ 22			//IRQ GPIO pin.
#define RFM_CE BCM2835_SPI_CS0  //SPI chip select
#define CRC_POLY 0x31;

static uint8_t group = 212;         // network group
//volatile uint16_t rf12_crc;         // running crc value

static bool debugMode = false;

static char *moduleDirectory;

void spi_init() 
{
	if (!bcm2835_init())
		exit (1); 

	bcm2835_spi_begin();
	bcm2835_spi_setBitOrder(BCM2835_SPI_BIT_ORDER_MSBFIRST);
	bcm2835_spi_setDataMode(BCM2835_SPI_MODE0);
	bcm2835_spi_setClockDivider(BCM2835_SPI_CLOCK_DIVIDER_128);   // 2MHz
	bcm2835_spi_chipSelect(RFM_CE);
	bcm2835_spi_setChipSelectPolarity(RFM_CE, LOW);

	//Set IRQ pin details
	bcm2835_gpio_fsel(RFM_IRQ, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_set_pud(RFM_IRQ, BCM2835_GPIO_PUD_UP);
}

uint16_t rf12_xfer(uint16_t cmd)
{
	char buffer[2];
	uint16_t reply;
	buffer[0] = cmd >> 8;
	buffer[1] = cmd;
	bcm2835_spi_transfern(buffer,2);
	reply = buffer[0] << 8;
	reply |= buffer[1];
	return reply;
}

void rf12m_init()
{
	bcm2835_delay(200);
	rf12_xfer(0x8067); // EL,EF,868band,12.0pF
	//    rf12_xfer(0x82d9);
	rf12_xfer(0xA67c); // 868MHz
	rf12_xfer(0xC613); // 
	rf12_xfer(0x94a0); // 
	rf12_xfer(0xC2ec); // 
	rf12_xfer(0xCA83); // 
	rf12_xfer(0xceD4); // SYNC=2DD4
	rf12_xfer(0xC48b); //
	rf12_xfer(0x9910); //
	rf12_xfer(0xCC77); //
	rf12_xfer(0xE000); // NOT USE
	rf12_xfer(0xC800); // NOT USE
	rf12_xfer(0xC069); //
}

void receiveData() 
{
	unsigned char buffer[128];
	int i;
	int j;

	bool moduleFound = false;

	//start python
	PyObject *modul, *addSensorDataFunction, *sendSensorDataFunction, *prm;
	Py_Initialize();
	PyObject *sys_path, *current_dir;

	if ((sys_path = PySys_GetObject("path")))
	{
		//adds module directory path to sys paths
		if ((current_dir = PyString_FromString(moduleDirectory)))
		{
			PyList_Insert(sys_path, 0, current_dir);
			Py_DECREF(current_dir);
		}
	}

	printf("Py_GetPath : %s\n",  Py_GetPath());

	printf("importing\n");
	modul = PyImport_ImportModule("postSensorData");

	printf("importing finished\n");

	if(modul)
	{
		moduleFound = true;
		printf("Modul found\n");

		PyModule_AddStringConstant(modul, "url", RFM12B_SENSOR_URL);

		addSensorDataFunction = PyObject_GetAttrString(modul, "addSensorData");
		sendSensorDataFunction = PyObject_GetAttrString(modul, "sendSensorData");
	}
	else
	{
		printf("Modul was not found\n");
		return;
	}

	//end python

	char *buf_str = (char*) malloc (2*5 + 1);		//nibbles string with received data
	char *buf_binstr = (char*) malloc (4*5 + 2); 	//binary string for received data

	printf("START:\n");

	for(j = 0;j<8;j++)
	{
		rf12_xfer(0x82c8);
		rf12_xfer(0xCA81); //reset the sync run to look for a new packet
		rf12_xfer(0xCA83); //restart the rfm12 sensor for receiving

		rf12_xfer(0);

		buffer[2] = 0;
		for(i = 1;i<buffer[2]+5;i++)
		{
			while(bcm2835_gpio_lev(RFM_IRQ));

			rf12_xfer(0);
			buffer[i] = rf12_xfer(0xB000);

			if(i > 5) // && buffer[i] == 0x00c4) 
				break;
		}

		printf("\n##### - START \n");

		if(group != 0)
			buffer[0] = group;


		//rf12_crc = ~0;

		char *buf_ptr = buf_str;

		for(i = 0;i<=5;i++)
		{
			if(i > 0)
				buf_ptr += sprintf(buf_ptr, "%02X", buffer[i]);

			if(debugMode) printf("_%02X",buffer[i]);
		}

		printf("Buffer: %s\n", buf_str);

		if(debugMode) printf("Convert - Start\n");

		//Check value returning not 0 for CRC check
		//"94C6986AD3";

		char *buf_binptr = buf_binstr;

		const char *tmpVal;

		for(i=0;i<10;i++)
		{
			tmpVal = hex_to_bin_quad(buf_str[i]);

			if(debugMode) printf("Binary for %c is %s\n", buf_str[i], tmpVal);

			buf_binptr += sprintf(buf_binptr, "%s", tmpVal);
		}

		buf_binptr += sprintf(buf_binptr, "\n");
		*(buf_binptr + 1) = '\0';

		if(debugMode) printf("Converted Value %s\n", buf_binstr);

		char crcData[9];

		makeCRC8Poly(buf_binptr, crcData);

		printf("\n(crc for Input %s = %s)\n", buf_binptr, crcData);

		//if crc returns 0 (no rest) send to server
		if(strcmp(crcData, "00000000") == 0)
		{
			if(moduleFound)
			{
				if(debugMode) printf("add sensor %s to list\n", buf_str);

				prm = Py_BuildValue("(s)", buf_str);
				PyObject_CallObject(addSensorDataFunction, prm);
			}
		}

		if(debugMode) printf("Convert - End\n");

		rf12_xfer(0x8208);

		printf("##### - END\n");
	}

	//start python
	if(moduleFound)
	{
		printf("send sensor data\n");
		PyObject_CallObject(sendSensorDataFunction, NULL);
	}

	if(prm)
		Py_DECREF(prm);

	Py_DECREF(addSensorDataFunction);
	Py_DECREF(sendSensorDataFunction);
	Py_DECREF(modul);

	Py_Finalize();
	//end python

	if(buf_str)
		free(buf_str);

	if(buf_binstr)
		free(buf_binstr);

	return;
}

int main (int argc, char *argv[])
{
	printf("Raspberry Pi RFM12B sensor receivers\n");

	printf("Service URL %s\n", RFM12B_SENSOR_URL);

	moduleDirectory = argv[1];

	if(moduleDirectory == NULL || *moduleDirectory == '\0')
	{
		printf("No module directory was given. Please add module directory as program argument.\n");
		return 1;
	}
	else
	{
		printf("Module directory is %s\n", moduleDirectory);
	}

	spi_init();

	rf12m_init();

	printf("Start receiving sensor data\n");

	receiveData();

	bcm2835_spi_end();

	printf("Program will exit in a few  seconds.\n");

    delay(2500);

	return 0;
}

