/*!
 * @file    TMP117.cpp
 * @author  Nils Minor
 * 
 * @license  GNU GENERAL PUBLIC LICENSE (see license.txt)
 * 
 * v1.0.0   - Initial library version
 * 
 * 10-2020 adpated for non arduino ESP32
 */

#define WRITE_BIT I2C_MASTER_WRITE              /*!< I2C master write */
#define READ_BIT I2C_MASTER_READ                /*!< I2C master read */
#define ACK_CHECK_EN 0x1                        /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS 0x0                       /*!< I2C master will not check ack from slave */


#include "TMP117.h"
static 	esp_err_t err;
/*!
    @brief   Constructor 
    @param   addr device I2C address [0x48 - 0x4B]
*/
TMP117::TMP117 (uint8_t addr, i2c_port_t portNum ) {
  i2cportNum = portNum;
  address = addr;
  alert_pin = -1;
  alert_type = NOALERT;
  newDataCallback = NULL;
}

/*!
    @brief   Initialize in default mode 
    @param   _newDataCallback   callback function will be called when new data is available
*/
void TMP117::init ( void (*_newDataCallback) (void) ) {
  setConvMode (CONTINUOUS);
  setConvTime (C1S);
  setAveraging (AVE8);
  setAlertMode (DATA);
  setOffsetTemperature(0);    
  
  newDataCallback = _newDataCallback;
}

/*!
    @brief    Read configuration register and handle events.
              Should be called in loop in order to call callback functions 
*/
void TMP117::update (void) {
  readConfig ();
}

/*!
    @brief   Performs a soft reset. All default values will be loaded to the configuration register
*/
void TMP117::softReset ( void ) {
  uint16_t reg_value = 0;
  reg_value |= 1UL << 1;
  writeConfig ( reg_value );
}

/*!
    @brief   Set alert pin mode 
    
    @param   mode TMP117_PMODE [Thermal-Alert-Data]
*/
void      TMP117::setAlertMode ( TMP117_PMODE mode) {
  uint16_t reg_value = readConfig ();
  if (mode == THERMAL) {
    reg_value |= 1UL << 4;    // change to thermal mode
    reg_value &= ~(1UL << 2); // set pin as alert flag
    reg_value &= ~(1UL << 3); // alert pin low active
  }
  else if (mode == ALERT) {
    reg_value &= ~(1UL << 4); // change to alert mode
    reg_value &= ~(1UL << 2); // set pin as alert flag
    reg_value &= ~(1UL << 3); // alert pin low active
  } 
  else if (mode == DATA) {
    reg_value |= 1UL << 2;    // set pin as data ready flag
  } 
  writeConfig ( reg_value );
}

/*!
    @brief   Set alert callback function and ISR pin
    @param   *allert_callback  callback function
    @param   pin callback pin (INT?)
*/
void      TMP117::setAllertCallback (void (*allert_callback)(void), uint8_t pin) {
  alert_pin = pin;
//  pinMode(pin, INPUT_PULLUP);
//
//  attachInterrupt(digitalPinToInterrupt(pin), allert_callback, FALLING ); // Sets up pin 2 to trigger "alert" ISR when pin changes H->L and L->H
}

/*!
    @brief    Set alert temperature
    
    @param    lowtemp   low boundary alert temperature
    @param    hightemp  high boundary alert temperature  
*/
void      TMP117::setAllertTemperature (double lowtemp, double hightemp) {
  
 uint16_t high_temp_value = hightemp / TMP117_RESOLUTION;
 uint16_t low_temp_value = lowtemp / TMP117_RESOLUTION;

 i2cWrite2B (TMP117_REG_TEMP_HIGH_LIMIT , high_temp_value);
 i2cWrite2B (TMP117_REG_TEMP_LOW_LIMIT , low_temp_value);  
}

/*!
    @brief    Set conversion mode
    
    @param    cmode   ::TMP117_CMODE [CONTINUOUS-SHUTDOWN-ONESHOT]
*/
void      TMP117::setConvMode ( TMP117_CMODE cmode) {
   uint16_t reg_value = readConfig ();
   reg_value &= ~((1UL << 11) | (1UL << 10));       // clear bits
   reg_value = reg_value | ( cmode  & 0x03 ) << 10; // set bits   
   writeConfig ( reg_value );
}

/*!
    @brief    Set conversion time
    
    @param    convtime  ::TMP117_CONVT [C15mS5-C125mS-C250mS-C500mS-C1S-C4S-C8S-C16S]
*/
void      TMP117::setConvTime ( TMP117_CONVT convtime ) {
  uint16_t reg_value = readConfig ();
  reg_value &= ~((1UL << 9) | (1UL << 8) | (1UL << 7));       // clear bits
  reg_value = reg_value | ( convtime  & 0x07 ) << 7;          // set bits
  writeConfig ( reg_value );
}
/*!
    @brief    Set averaging mode
    
    @param    ave  ::TMP117_AVE [NOAVE-AVE8-AVE32-AVE64]
*/
void      TMP117::setAveraging ( TMP117_AVE ave ) {
  uint16_t reg_value = readConfig ();
  reg_value &= ~((1UL << 6) | (1UL << 5) );       // clear bits
  reg_value = reg_value | ( ave & 0x03 ) << 5;          // set bits
  writeConfig ( reg_value );
}

/*!
    @brief    Set offset temperature
    
    @param    double  target offset temperature  in the range of ±256°C  
*/
void      TMP117::setOffsetTemperature ( double offset ) {
  int16_t offset_temp_value = offset / TMP117_RESOLUTION;
  i2cWrite2B (TMP117_REG_TEMPERATURE_OFFSET , offset_temp_value);
}

/*!
    @brief    Set target temperature for calibration purpose
    
    @param    double  target temperature to calibrate to in the range of ±256°C  
*/
void      TMP117::setTargetTemperature ( double target ) {
  double actual_temp = getTemperature ( );
  double delta_temp =  target - actual_temp;
  setOffsetTemperature ( delta_temp );
}

/*!
    @brief    Read configuration register and handle events.

    @return   uint16_t  read value of the configuration regsiter          
*/
uint16_t  TMP117::readConfig (void) {
  uint16_t reg_value = i2cRead2B ( TMP117_REG_CONFIGURATION );
  bool high_alert = reg_value >> 15 & 1UL;
  bool low_alert = reg_value >> 14 & 1UL;   
  bool data_ready = reg_value >> 13 & 1UL;   
  bool eeprom_busy = reg_value >> 12 & 1UL;   

  if (data_ready && newDataCallback != NULL)
    newDataCallback ();

  if (reg_value >> 15 & 1UL) {
    alert_type = HIGHALERT;
  }
  else if (reg_value >> 14 & 1UL) {
    alert_type = LOWALERT;
  }
  else {
    alert_type = NOALERT;
  }
  
  //printConfig ( reg_value );
    
  return reg_value;  
}

/*!
    @brief    Returns the recalculated temperature
    
    @return   double  temperature in °C
*/
double    TMP117::getTemperature (void) {
  int16_t temp = i2cRead2B( TMP117_REG_TEMPERATURE );
  if (!err)
	  return  (temp * TMP117_RESOLUTION);
  else return -9999;
}
/*!
    @brief    Get Device Revision 
    
    @return   uint16_t device revision
*/
uint16_t  TMP117::getDeviceRev (void) {
  // read bits [15:12]
  uint16_t raw = i2cRead2B( TMP117_REG_DEVICE_ID );
  
  return ( (raw >> 12) & 0x3);
}

/*!
    @brief    Get Device ID (always 0x117)
    
    @return   uint16_t  device ID
*/
uint16_t  TMP117::getDeviceID (void) {
  // read bits [11:0]
  uint16_t raw = i2cRead2B( TMP117_REG_DEVICE_ID );
  return (raw & 0x0fff);
}

/*!
    @brief    Returns the information which alert type happend
    
    @return   TMP117_ALERT [NoAlert-HighTempAlert-LowTempAlert]
*/
TMP117_ALERT TMP117::getAlertType ( void ) {
  return alert_type;
}

/*!
    @brief    Returns the content of the offset register in °C
    
    @return   double  offset temperature in °C
*/
double    TMP117::getOffsetTemperature (void) {
  int16_t temp = i2cRead2B( TMP117_REG_TEMPERATURE_OFFSET );
  return  (temp * TMP117_RESOLUTION);
}

/*!
    @brief    Write data to EEPROM register
    
    @param    data        data to write to the EEPROM
    
    @param    eeprom_nr   represents the EEPROM number [1 - 3] 
*/
void      TMP117::writeEEPROM (uint16_t data, uint8_t eeprom_nr) {
  if (!EEPROMisBusy()) {
    unlockEEPROM();
      switch (eeprom_nr) {
        case 1 : i2cWrite2B ( TMP117_REG_EEPROM1, data); break;
        case 2 : i2cWrite2B ( TMP117_REG_EEPROM2, data); break;
        case 3 : i2cWrite2B ( TMP117_REG_EEPROM3, data); break;
        default:break;// Serial.println("EEPROM value must be between 1 and 3");
      }
    lockEEPROM();
  }
//  else {
//    Serial.println("EEPROM is busy");
//  }
}

/*!
    @brief    Read data from EEPROM register
    
    @param    eeprom_nr  represents the EEPROM number [1 - 3] 
    
    @return   uint16_t   read EEPROM data
*/
uint16_t  TMP117::readEEPROM (uint8_t eeprom_nr) {
  // read the 48 bit number from the EEPROM
  if (!EEPROMisBusy()) {
    uint16_t eeprom_data = 0;
    switch (eeprom_nr) {
        case 1 : eeprom_data = i2cRead2B( TMP117_REG_EEPROM1 ); break;
        case 2 : eeprom_data = i2cRead2B( TMP117_REG_EEPROM2 ); break;
        case 3 : eeprom_data = i2cRead2B( TMP117_REG_EEPROM3 ); break;
        default:  break; // Serial.println("EEPROM value must be between 1 and 3");
      }
    return eeprom_data;
  }
  else {
 //   Serial.println("EEPROM is busy");
    return 999;
  }
}



/*!
    @brief    Write two bytes (16 bits) to TMP117 I2C sensor
    
    @param    reg  target register
    @param    data data to write
*/



void  TMP117::i2cWrite2B (uint8_t reg, uint16_t data){
	esp_err_t err = 0;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);
	i2c_master_write_byte(cmd,(data>>8), ACK_CHECK_EN);
	i2c_master_write_byte(cmd,(uint8_t) data, ACK_CHECK_EN);
	i2c_master_stop(cmd);
	err = i2c_master_cmd_begin(i2cportNum, cmd, 1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (err)
		printf (" err i2c write %x", err);
	vTaskDelay(10);
}



/*!
    @brief    Read two bytes (16 bits) from TMP117 I2C sensor
    
    @param    reg  target register to read from
    
    @return   uint16_t  read data
*/


uint16_t  TMP117::i2cRead2B (uint8_t reg) {

//esp_err_t MPR121::read(uint8_t register_address,uint8_t *data, uint8_t byte_count) {
	uint8_t data[2] = {0};
	uint16_t datac;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | WRITE_BIT, ACK_CHECK_EN);
	i2c_master_write_byte(cmd, reg, ACK_CHECK_EN);

	err=i2c_master_cmd_begin(i2cportNum, cmd, 000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);

	cmd = i2c_cmd_link_create();
	i2c_master_start(cmd);
	i2c_master_write_byte(cmd, (address << 1) | READ_BIT, ACK_CHECK_EN);
	i2c_master_read_byte(cmd, data, I2C_MASTER_ACK);
	i2c_master_read_byte(cmd, data + 1, I2C_MASTER_NACK);
	i2c_master_stop(cmd);
	err |= i2c_master_cmd_begin(i2cportNum, cmd,	1000 / portTICK_PERIOD_MS);
	i2c_cmd_link_delete(cmd);
	if (err) {
		printf (" err i2c readd %x", err);
		return 0xFFFF;
	}


	datac = ((data[0] << 8) | data[1]);
	return datac;

}




/*!
    @brief    Write configuration to config register
    
    @param    config_data  configuration
*/
void      TMP117::writeConfig (uint16_t config_data) {
  i2cWrite2B (TMP117_REG_CONFIGURATION, config_data);
}

/*!
    @brief    Prints configuration in user readable format
    
    @param    reg_value  configuration value
*/
void      TMP117::printConfig (uint16_t reg_value) {

//  Serial.println(reg_value, BIN);
//
//  Serial.println("XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX");
//  Serial.print ("HIGH alert:  ");
//  Serial.println( ( reg_value >> 15) & 0b1 , BIN);
//  Serial.print ("LOW alert:   ");
//  Serial.println( ( reg_value >> 14) & 0b1 , BIN);
//  Serial.print ("Data ready:  ");
//  Serial.println( ( reg_value >> 13) & 0b1 , BIN);
//  Serial.print ("EEPROM busy: ");
//  Serial.println( ( reg_value >> 12) & 0b1 , BIN);
//  Serial.print ("MOD[1:0]:    ");
//  Serial.println( ( reg_value >> 10) & 0b11 , BIN);
//  Serial.print ("CONV[2:0]:   ");
//  Serial.println( ( reg_value >> 7)  & 0b111 , BIN);
//  Serial.print ("AVG[1:0]:    ");
//  Serial.println( ( reg_value >> 5)  & 0b11 , BIN);
//  Serial.print ("T/nA:        ");
//  Serial.println( ( reg_value >> 4) & 0b1 , BIN);
//  Serial.print ("POL:         ");
//  Serial.println( ( reg_value >> 3) & 0b1 , BIN);
//  Serial.print ("DR/Alert:    ");
//  Serial.println( ( reg_value >> 2) & 0b1 , BIN);
//  Serial.print ("Soft_Reset:  ");
//  Serial.println( ( reg_value >> 1) & 0b1 , BIN);
}
/*!
    @brief    Lock EEPROM, write protection
*/
void      TMP117::lockEEPROM (void) {
  // clear bit 15
  uint16_t code = 0;
  code &= ~(1UL << 15);
  i2cWrite2B ( TMP117_REG_EEPROM_UNLOCK, code );
//  delay(100);
}

/*!
    @brief    Unlock EEPROM, remove write protection
*/
void      TMP117::unlockEEPROM (void) {
  // set bit 15
  uint16_t code = 0;
  code |= 1UL << 15;
  i2cWrite2B ( TMP117_REG_EEPROM_UNLOCK, code );
//  delay(100);
}

/*!
    @brief    States if the EEPROM is busy
    
    @return   Ture if the EEPROM is busy, fals else
*/
bool      TMP117::EEPROMisBusy (void) {
  // Bit 14 indicates the busy state of the eeprom
  uint16_t code = i2cRead2B ( TMP117_REG_EEPROM_UNLOCK );
  return (bool) ((code >> 14) & 0x01);
}


