
/*
 * i2CTask.cpp
 *
 *  Created on: Aug 9, 2021
 *      Author: dig
 */
#include "esp_system.h"
//#include "driver/gpio.h"
#include "driver/i2c.h"
//#include "main.h"
#include "TMP117.h"
#include "measureTask.h"
#include "i2cTask.h"
#include "ntc.h"
#include "averager.h"


// Select the correct address setting
uint8_t ADDR_GND = 0x48;   // 1001000
uint8_t ADDR_VCC = 0x49;   // 1001001
uint8_t ADDR_SDA = 0x4A;   // 1001010
uint8_t ADDR_SCL = 0x4B;   // 1001011
uint8_t ADDR = ADDR_VCC;

TMP117 tmp(ADDR, I2C_NUM_1);

Averager tmpAverager(AVGERAGESAMPLES);
float tmpTemperature;


float getTmp117Temperature(void) {
	return tmpTemperature;
}

float getTmp117AveragedTemperature(void) {
	if (tmpTemperature > ERRORTEMP)
		return tmpAverager.average() / 1000.0;
	else
		return ERRORTEMP;
}

//static esp_err_t i2c_master_init(void) {
//	i2c_port_t i2c_master_port = I2C_MASTER_NUM;
//	i2c_config_t conf;
//	conf.mode = I2C_MODE_MASTER;
//	conf.sda_io_num = SDA_PIN;
//	conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
//	conf.scl_io_num = SCL_PIN;
//	conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
//	conf.master.clk_speed = 100000;
//	conf.clk_flags = 0;
//	i2c_param_config(i2c_master_port, &conf);
//	return i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
//}

void I2CTask(void *arg) {
	esp_err_t error;
	float t = 0;
#ifdef SIMULATE
	while (1) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
#else

//	i2c_master_init(); // done in main
	tmp.init(NULL);

	vTaskDelay(20);
	while (1) {
		t = tmp.getTemperature();
		if (t < 200) {
			tmpTemperature = t;
			tmpAverager.write(1000.0 * t); // averager only integers
		}
		else
			tmpTemperature = ERRORTEMP;
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
#endif
}

