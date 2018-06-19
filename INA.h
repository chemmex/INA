/*******************************************************************************************************************
** Class definition header for the INA class. This library gives a common interface to various INA power monitor  **
** devices, see https://github.com/SV-Zanshin/INA/wiki  for a full list of currently supported devices. The INA   **
** devices have a 3-5V power  supply and, depending upon the model, measure up to 26 or 36V. The are High-Side or **
** Low-Side Measurement, Bi-Directional Current and Power Monitor with I2C Compatible Interface. The devices      **
** are documented at http://www.ti.com/amplifier-circuit/current-sense/power-current-monitors/products.html       **
**                                                                                                                **
** Detailed documentation can be found on the GitHub Wiki pages at https://github.com/SV-Zanshin/INA/wiki         **
**                                                                                                                **
** The INA chips require an external shunt of known resistance to be placed across the high-side or low-side      **
** supply or ground line and it uses the small current generated by the shunt to compute the amperage going       **
** through the circuit.  This value, coupled with the voltage measurement, allows the Amperage and Wattage to be  **
** computed by the INA device and all of these values can be read using the industry standard I2C protocol.       **
**                                                                                                                **
** Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated   **
** documentation files (the "Software"), to deal in the Software without restriction, including without limitation**
** the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,   **
** and to permit persons to whom the Software is furnished to do so, subject to the following conditions:         **
** The above copyright notice and this permission notice shall be included in all copies or substantial portions  **
** of the Software.                                                                                               **
**                                                                                                                **
** Although programming for the Arduino and in c/c++ is new to me, I'm a professional programmer and have learned,**
** over the years, that it is much easier to ignore superfluous comments than it is to decipher non-existent ones;**
** so both my comments and variable names tend to be verbose. The code is written to fit in the first 80 spaces   **
** and the comments start after that and go to column 117 - allowing the code to be printed in A4 landscape mode. **
**                                                                                                                **
** This program is free software: you can redistribute it and/or modify it under the terms of the GNU General     **
** Public License as published by the Free Software Foundation, either version 3 of the License, or (at your      **
** option) any later version. This program is distributed in the hope that it will be useful, but WITHOUT ANY     **
** WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the   **
** GNU General Public License for more details. You should have received a copy of the GNU General Public License **
** along with this program.  If not, see <http://www.gnu.org/licenses/>.                                          **
**                                                                                                                **
** Vers.  Date       Developer                     Comments                                                       **
** ====== ========== ============================= ============================================================== **
** 1.0.0b 2018-06-17 https://github.com/SV-Zanshin Continued coding, tested on INA219 and INA226                  **
** 1.0.0a 2018-06-10 https://github.com/SV-Zanshin Initial coding began                                           **
**                                                                                                                **
*******************************************************************************************************************/
#if ARDUINO >= 100                                                            // The Arduino IDE versions before  //
  #include "Arduino.h"                                                        // 100 need to use the older library//
#else                                                                         // rather than the new one          //
  #include "WProgram.h"                                                       //                                  //
#endif                                                                        //                                  //
#ifndef INA_Class_h                                                           // Guard code definition            //
  #define debug_Mode                                                          // Comment out when not needed      //
  #define INA__Class_h                                                        // Define the name inside guard code//
  /*****************************************************************************************************************
  ** Declare structures and enumerated types used in the class                                                    **
  *****************************************************************************************************************/
  typedef struct {                                                            // Structure of values per device   //
    uint8_t  address;                                                         // I2C Address of device            //
    uint8_t  type;                                                            // see enumerated "ina_Type"        //
    uint16_t calibration;                                                     // Calibration register value       //
    uint16_t shuntVoltage_LSB;                                                // Device dependent LSB factor      //
    uint16_t busVoltage_LSB;                                                  // Device dependent LSB factor      //
    uint32_t current_LSB;                                                     // Amperage LSB                     //
    uint32_t power_LSB;                                                       // Wattage LSB                      //
    uint8_t  programmableGain;                                                // Only on some devices             //
    uint8_t  operatingMode;                                                   // Default continuous mode operation//
    uint16_t calibConst;                                                      // Device specific calibration const//
    uint16_t powerConstant;                                                   // Device specific calibration const//
    uint8_t  maxBusAmps;                                                      // Store initialization value       //
    uint32_t microOhmR;                                                       // Store initialization value       //
    char     deviceName[7];                                                   // Device name as a character array //
  } inaDet; // of structure                                                   //                                  //
                                                                              //                                  //
  enum ina_Type { INA219,                                                     // SOT-23, SOIC-8, multiple gain    //
                  //INA220,                                                   // Indistinguishable from INA219    //
                  INA226,                                                     // VSSOP-10                         //
                  INA230,                                                     // QFN-16                           //
                  INA231,                                                     // DSBGA-12                         //
                  INA233,                                                     // VSSOP-10                         //
                  INA250,                                                     // TSSOP-16, integrated shunt       //
                  INA253,                                                     // TSSOP-20, integrated shunt       //
                  INA260,                                                     // TSSOP-16, integrated shunt       //
                  INA3221,                                                    // VQFN-16                          //
                  UNKNOWN };                                                  //                                  //
                                                                              //                                  //
  enum ina_Mode { INA_MODE_SHUTDOWN,                                          // Device powered down              //
                  INA_MODE_TRIGGERED_SHUNT,                                   // Triggered shunt, no bus          //
                  INA_MODE_TRIGGERED_BUS,                                     // Triggered bus, no shunt          //
                  INA_MODE_TRIGGERED_BOTH,                                    // Triggered bus and shunt          //
                  INA_MODE_POWER_DOWN,                                        // shutdown or power-down           //
                  INA_MODE_CONTINUOUS_SHUNT,                                  // Continuous shunt, no bus         //
                  INA_MODE_CONTINUOUS_BUS,                                    // Continuous bus, no shunt         //
                  INA_MODE_CONTINUOUS_BOTH };                                 // Both continuous, default value   //
  /*****************************************************************************************************************
  ** Declare constants used in the class                                                                          **
  *****************************************************************************************************************/
  const uint8_t  INA_CONFIGURATION_REGISTER     =      0;                     //==================================//
  const uint8_t  INA_SHUNT_VOLTAGE_REGISTER     =      1;                     // Values common to all INAs        //
  const uint8_t  INA_BUS_VOLTAGE_REGISTER       =      2;                     //==================================//
  const uint8_t  INA_POWER_REGISTER             =      3;                     //                                  //
  const uint8_t  INA_CURRENT_REGISTER           =      4;                     //                                  //
  const uint8_t  INA_CALIBRATION_REGISTER       =      5;                     //                                  //
  const uint8_t  INA_MASK_ENABLE_REGISTER       =      6;                     // Not found on all devices         //
  const uint8_t  INA_ALERT_LIMIT_REGISTER       =      7;                     // Not found on all devices         //
  const uint8_t  INA_MANUFACTURER_ID_REGISTER   =   0xFE;                     // Not found on all devices         //
  const uint8_t  INA_DIE_ID_REGISTER            =   0xFF;                     // Not found on all devices         //
  const uint16_t INA_RESET_DEVICE               = 0x8000;                     // Write to configuration to reset  //
  const uint16_t INA_CONVERSION_READY_MASK      = 0x0080;                     // Bit 4                            //
  const uint16_t INA_CONFIG_MODE_MASK           = 0x0007;                     // Bits 0-3                         //
  const uint16_t INA_ALERT_MASK                 = 0x03FF;                     // Mask off bits 0-9                //
  const uint8_t  INA_ALERT_SHUNT_OVER_VOLT_BIT  =     15;                     // Register bit                     //
  const uint8_t  INA_ALERT_SHUNT_UNDER_VOLT_BIT =     14;                     // Register bit                     //
  const uint8_t  INA_ALERT_BUS_OVER_VOLT_BIT    =     13;                     // Register bit                     //
  const uint8_t  INA_ALERT_BUS_UNDER_VOLT_BIT   =     12;                     // Register bit                     //
  const uint8_t  INA_ALERT_POWER_OVER_WATT_BIT  =     11;                     // Register bit                     //
  const uint8_t  INA_ALERT_CONVERSION_RDY_BIT   =     10;                     // Register bit                     //
                                                                              //==================================//
                                                                              // Device-specific values           //
                                                                              //==================================//
  const uint16_t INA219_BUS_VOLTAGE_LSB         =    400;                     // LSB in uV *100 4.00mV            //
  const uint16_t INA219_SHUNT_VOLTAGE_LSB       =    100;                     // LSB in uV *10  10.0uV            //
  const uint16_t INA219_CONFIG_AVG_MASK         = 0x07F8;                     // Bits 3-6, 7-10                   //
  const uint16_t INA219_CONFIG_PG_MASK          = 0xE7FF;                     // Bits 11-12 masked                //
  const uint16_t INA219_CONFIG_BADC_MASK        = 0x0780;                     // Bits 7-10  masked                //
  const uint16_t INA219_CONFIG_SADC_MASK        = 0x0038;                     // Bits 3-5                         //
  const uint8_t  INA219_BRNG_BIT                =     13;                     // Bit for BRNG in config register  //
  const uint8_t  INA219_PG_FIRST_BIT            =     11;                     // first bit of Programmable Gain   //
                                                                              //----------------------------------//
  const uint16_t INA226_BUS_VOLTAGE_LSB         =    125;                     // LSB in uV *100 1.25mV            //
  const uint16_t INA226_SHUNT_VOLTAGE_LSB       =     25;                     // LSB in uV *10  2.5uV             //
  const uint16_t INA226_CONFIG_AVG_MASK         = 0x0E00;                     // Bits 9-11                        //
  const uint16_t INA226_DIE_ID_VALUE            = 0x2260;                     // Hard-coded Die ID for INA226     //
  const uint16_t INA226_CONFIG_BADC_MASK        = 0x01C0;                     // Bits 7-10  masked                //
  const uint16_t INA226_CONFIG_SADC_MASK        = 0x0018;                     // Bits 3-4                         //
                                                                              //==================================//
  const uint8_t  I2C_DELAY                      =     10;                     // Microsecond delay on write       //
  /*****************************************************************************************************************
  ** Declare class header                                                                                         **
  *****************************************************************************************************************/
  class INA_Class {                                                           // Class definition                 //
    public:                                                                   // Publicly visible methods         //
      INA_Class();                                                            // Class constructor                //
      ~INA_Class();                                                           // Class destructor                 //
      uint8_t  begin             (const uint8_t maxBusAmps,                   // Class initializer                //
                                  const uint32_t microOhmR,                   //                                  //
                                  const uint8_t  deviceNumber = UINT8_MAX );  //                                  //
      void     setMode           (const uint8_t mode,                         // Set the monitoring mode          //
                                  const uint8_t devNumber=UINT8_MAX);         //                                  //
      void     setAveraging      (const uint16_t averages,                    // Set the number of averages taken //
                                  const uint8_t deviceNumber=UINT8_MAX);      //                                  //
      void     setBusConversion  (const uint32_t convTime,                    // Set timing for Bus conversions   //
                                  const uint8_t deviceNumber=UINT8_MAX);      //                                  //
      void     setShuntConversion(const uint32_t convTime,                    // Set timing for Shunt conversions //
                                  const uint8_t deviceNumber=UINT8_MAX);      //                                  //
      uint16_t getBusMilliVolts  (const uint8_t deviceNumber=0);              // Retrieve Bus voltage in mV       //
      int32_t  getShuntMicroVolts(const uint8_t deviceNumber=0);              // Retrieve Shunt voltage in uV     //
      int32_t  getBusMicroAmps   (const uint8_t deviceNumber=0);              // Retrieve micro-amps              //
      int32_t  getBusMicroWatts  (const uint8_t deviceNumber=0);              // Retrieve micro-watts             //
      char *   getDeviceName     (const uint8_t deviceNumber=0);              // Retrieve device name as char[7]  //
      void     reset             (const uint8_t deviceNumber=0);              // Reset the device                 //
      void     waitForConversion (const uint8_t deviceNumber=UINT8_MAX);      // wait for conversion to complete  //
      bool     AlertOnConversion (const bool alertState,                      // Enable pin change on conversion  //
                                  const uint8_t deviceNumber=UINT8_MAX);      //                                  //
      bool     AlertOnShuntOverVoltage (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnShuntUnderVoltage(const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnBusOverVoltage   (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnBusUnderVoltage  (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliVolts,             //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
      bool     AlertOnPowerOverLimit   (const bool alertState,                // Enable pin change on conversion  //
                                        const int32_t milliAmps,              //                                  //
                                        const uint8_t deviceNumber=UINT8_MAX);//                                  //
    private:                                                                  // Private variables and methods    //
      int16_t  readWord         (const uint8_t addr,                          // Read a word from an I2C address  //
                                 const uint8_t deviceAddress);                //                                  //
      void     writeWord        (const uint8_t addr, const uint16_t data,     // Write a word to an I2C address   //
                                 const uint8_t deviceAddress);                //                                  //
      void     readInafromEEPROM(const uint8_t deviceNumber);                 // Retrieve structure from EEPROM   //
      void     writeInatoEEPROM (const uint8_t deviceNumber);                 // Write structure to EEPROM        //
      void     initINA219_INA220(const uint8_t  maxBusAmps,                   // Initialize INA219 or INA220      //
                                 const uint32_t microOhmR,                    //                                  //
                                 const uint8_t  deviceNumber);                //                                  //
      void     initINA226       (const uint8_t maxBusAmps,                    // Initialize INA226                //
                                 const uint32_t microOhmR,                    //                                  //
                                 const uint8_t deviceNumber);                 //                                  //
      uint8_t  _DeviceCount        = 0;                                       // Number of INAs detected          //
      inaDet   ina;                                                           // Declare a single global value    //
  }; // of INA_Class definition                                               //                                  //
#endif                                                                        //----------------------------------//
