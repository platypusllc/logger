#ifndef MS5541C_h
#define MS5541C_h

#include "Arduino.h"
#include <SPI.h>

typedef struct PressureTempPair{
	//Pressure in mbar
	long pressure;
	//Temperature in degrees Centigrade
	float temperature;
} PressureTempPair;

class MS5541C{
private:
	//Pin configurations
	int MCLK, SCLK, MISO, MOSI;

	//Calibration Coefficients
	long c1, c2, c3, c4, c5, c6;

	void resetSensor();

public:
	MS5541C();
	MS5541C(int MCLKpin, int SCLKpin, int MISOpin, int MOSIpin);

	bool initialize(int MCLKpin, int SCLKpin, int MISOpin, int MOSIpin);
	void generateClockSignal();
	PressureTempPair getPressureTemp();
};
#endif