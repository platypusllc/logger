#include "Arduino.h"
#include "MS5541C.h"

MS5541C::MS5541C(){
	//Empty
}

MS5541C::MS5541C(int MCLKpin, int SCLKpin, int MISOpin, int MOSIpin){
	MS5541C::initialize(MCLKpin, SCLKpin, MISOpin, MOSIpin);
}

bool MS5541C::initialize(int MCLKpin, int SCLKpin, int MISOpin, int MOSIpin){
	//Store pin configurations
	this->MCLK = MCLKpin;
	this->SCLK = SCLKpin;
	this->MISO = MISOpin;
	this->MOSI = MOSIpin;

	//SPI Setup
	SPI.begin();
	SPI.setBitOrder(MSBFIRST);
	SPI.setClockDivider(SPI_CLOCK_DIV32);
	pinMode(this->MCLK, OUTPUT);
	delay(100);

	//Read Calibration words and extract coefficients
	this->generateClockSignal();
	this->resetSensor();
	//Calibration word 1
	unsigned int result1 = 0;
	unsigned int inbyte1 = 0;
	SPI.transfer(0x1D); //send first byte of command to get calibration word 1
	SPI.transfer(0x50); //send second byte of command to get calibration word 1
	SPI.setDataMode(SPI_MODE1); //change mode in order to listen
	result1 = SPI.transfer(0x00); //send dummy byte to read first byte of word
	result1 = result1 << 8; //shift returned byte 
	inbyte1 = SPI.transfer(0x00); //send dummy byte to read second byte of word
	result1 = result1 | inbyte1; //combine first and second byte of word
	
	this->resetSensor();

	//Calibration word 2; see comments on calibration word 1
	unsigned int result2 = 0;
	byte inbyte2 = 0; 
	SPI.transfer(0x1D);
	SPI.transfer(0x60);
	SPI.setDataMode(SPI_MODE1); 
	result2 = SPI.transfer(0x00);
	result2 = result2 <<8;
	inbyte2 = SPI.transfer(0x00);
	result2 = result2 | inbyte2;  

	this->resetSensor();

	//Calibration word 3; see comments on calibration word 1
	unsigned int result3 = 0;
	byte inbyte3 = 0;
	SPI.transfer(0x1D);
	SPI.transfer(0x90); 
	SPI.setDataMode(SPI_MODE1); 
	result3 = SPI.transfer(0x00);
	result3 = result3 <<8;
	inbyte3 = SPI.transfer(0x00);
	result3 = result3 | inbyte3;
	
	this->resetSensor();

	//Calibration word 4; see comments on calibration word 1
	unsigned int result4 = 0;
	byte inbyte4 = 0;
	SPI.transfer(0x1D);
	SPI.transfer(0xA0);
	SPI.setDataMode(SPI_MODE1); 
	result4 = SPI.transfer(0x00);
	result4 = result4 <<8;
	inbyte4 = SPI.transfer(0x00);
	result4 = result4 | inbyte4;

	//now we do some bitshifting to extract the calibration factors 
	//out of the calibration words; read datasheet AN510 for better understanding
	this->c1 = result1 >> 3 & 0x1FFF;
	this->c2 = ((result1 & 0x07) << 10) | ((result2 >> 6) & 0x03FF);
	this->c3 = (result3 >> 6) & 0x03FF;
	this->c4 = (result4 >> 7) & 0x07FF;
	this->c5 = ((result2 & 0x003F) << 6) | (result3 & 0x003F);
	this->c6 = result4 & 0x007F;

	return true;
}

PressureTempPair MS5541C::getPressureTemp(){
	PressureTempPair result;

	this->resetSensor();

	//Read Temperature
	unsigned int tempMSB = 0; //first byte of value
	unsigned int tempLSB = 0; //last byte of value
	unsigned int D2 = 0;
	SPI.transfer(0x0F); //send first byte of command to get temperature value
	SPI.transfer(0x20); //send second byte of command to get temperature value
	delay(35); //wait for conversion end
	SPI.setDataMode(SPI_MODE1); //change mode in order to listen
	tempMSB = SPI.transfer(0x00); //send dummy byte to read first byte of value
	tempMSB = tempMSB << 8; //shift first byte
	tempLSB = SPI.transfer(0x00); //send dummy byte to read second byte of value
	D2 = tempMSB | tempLSB; //combine first and second byte of value
	
	//Compensation with calibration coefficients
	const long UT1 = (c5 << 3) + 10000;
	const long dT = D2 - UT1;
	const long TEMP = 200 + ((dT * (c6 + 100)) >> 11);
	result.temperature = TEMP/10.0;

	this->resetSensor();

	//Read Pressure
	unsigned int presMSB = 0; //first byte of value
	unsigned int presLSB =0; //last byte of value
	unsigned int D1 = 0;
	SPI.transfer(0x0F); //send first byte of command to get pressure value
	SPI.transfer(0x40); //send second byte of command to get pressure value
	delay(35); //wait for conversion end
	SPI.setDataMode(SPI_MODE1); //change mode in order to listen
	presMSB = SPI.transfer(0x00); //send dummy byte to read first byte of value
	presMSB = presMSB << 8; //shift first byte
	presLSB = SPI.transfer(0x00); //send dummy byte to read second byte of value
	D1 = presMSB | presLSB; //combine first and second byte of value
	
	//Compensation with calibration coefficients and temp
	const long OFF  = c2 + (((c4 - 250) * dT) >> 12) + 10000;
	const long SENS = (c1/2) + (((c3 + 200) * dT) >> 13) + 3000;
	result.pressure = (SENS * (D1 - OFF) >> 12) + 1000;
	
	return result;
}


void MS5541C::resetSensor(){
	SPI.setDataMode(SPI_MODE0); 
	SPI.transfer(0x15);
	SPI.transfer(0x55);
	SPI.transfer(0x40);
}

//Use arduino timer to generate MCLK signal (Arduino UNO)
void MS5541C::generateClockSignal(){
	TCCR1B = (TCCR1B & 0xF8) | 1;
	analogWrite(this->MCLK, 128);
}