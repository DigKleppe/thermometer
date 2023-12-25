/*
 * main.h
 *
 *  Created on: Sep 9, 2019
 *      Author: dig
 */

#ifndef MAIN_INCLUDE_MAIN_H_
#define MAIN_INCLUDE_MAIN_H_



//#define DMMBOARD				1

#ifdef DMMBOARD


#define RREF			100680			// in ohms
#define CAP_PIN			GPIO_NUM_16
#define RREF_PIN		GPIO_NUM_18
#define NTC1_PIN		GPIO_NUM_23
#define NTC2_PIN		GPIO_NUM_19
#define NTC3_PIN		GPIO_NUM_18
#define NTC4_PIN		RREF_PIN


#define CONFIG_CLK				18 //pin 12 J8
#define CONFIG_MISO				19 // pin 14
#define CONFIG_MOSI				23 // pin 16
#define CONFIG_NSS				32  // relay
#define CONFIG_RST				16 //tp17
#define CONFIG_DIO0				GPIO_NUM_4 // tp18
// io 25 and 26 used for HX711 scale, connected to pin 7 / 8 J8 extension connector


#define	OLED_SDA				21
#define OLED_SCL				22

#define TP_SDA			GPIO_NUM_21
#define TP_SCL			GPIO_NUM_22



#else

#define SDA_PIN	    	4
#define SCL_PIN	    	2

#define LED_PIN 		GPIO_NUM_22

//#define TP_1			GPIO_NUM_12
//#define TP_2			GPIO_NUM_13

#endif

#define I2C_MASTER_SCL_IO          SCL_PIN         /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO          SDA_PIN         /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM             I2C_NUM_1        /*!< I2C port number for master dev */
#define I2C_MASTER_TX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_MASTER_RX_BUF_DISABLE  0                /*!< I2C master do not need buffer */
#define I2C_MASTER_FREQ_HZ         400000           /*!< I2C master clock frequency */



#endif /* MAIN_INCLUDE_MAIN_H_ */
