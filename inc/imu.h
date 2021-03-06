/****************************************************************************
	[Project] FlexSEA: Flexible & Scalable Electronics Architecture
	[Sub-project] 'flexsea-execute' Advanced Motion Controller
	Copyright (C) 2016 Dephy, Inc. <http://dephy.com/>

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*****************************************************************************
	[Lead developper] Jean-Francois (JF) Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab 
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors] Erin Main (ermain@mit.edu)
*****************************************************************************
	[This file] fm_i2c: IMU configuration
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2016-09-23 | jfduval | Initial GPL-3.0 release
	*
****************************************************************************/

#ifndef INC_IMU_H_
#define INC_IMU_H_

//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"
	
//****************************************************************************
// Shared variable(s)
//****************************************************************************	
	
extern struct imu_s imu;
//extern uint8 imu_last_request;
	
//****************************************************************************
// Define Flag(s):
//****************************************************************************

//set to 1 if we want to use blocking read/write
#define IMU_BLOCKING 	1

//****************************************************************************
// Definition(s):
//****************************************************************************

// For choosing what subset of the data we want from the IMU
typedef enum {
  ACCEL_ONLY,
  GYRO_ONLY,
  ALL
} imu_data_set;

// I2C Comms Constants
#define IMU_BLOCK_TIMEOUT 		10000 	//may want to reduce this?
//#define IMU_ADDR 0x68 //0b1101000
#define IMU_ADDR 				0x68 	//device address of the IMU, 7bits right justified
#define IMU_MAX_BUF_SIZE 		100 	//(in bytes) (somewhat arbitrary)

// IMU Register Addresses (names correspond to those in the datasheet)
// These are internal to the imu.
/// Config Regs
#define IMU_CONFIG 				26
#define IMU_GYRO_CONFIG 		27
#define IMU_ACCEL_CONFIG		28
#define IMU_ACCEL_CONFIG2		29
#define IMU_FIFO_EN				0x23
#define IMU_INT_PIN_CFG			55
#define IMU_INT_ENABLE			56
#define IMU_INT_STATUS			58
#define IMU_SIGNAL_PATH_RESET	104
#define IMU_USER_CTRL			106
#define IMU_PWR_MGMT_1			107
#define IMU_PWR_MGMT_2			108
/// Data Regs
#define IMU_ACCEL_XOUT_H		59
#define IMU_ACCEL_XOUT_L		60
#define IMU_ACCEL_YOUT_H		61
#define IMU_ACCEL_YOUT_L		62
#define IMU_ACCEL_ZOUT_H		63
#define IMU_ACCEL_ZOUT_L		64
#define IMU_GYRO_XOUT_H			67
#define IMU_GYRO_XOUT_L			68
#define IMU_GYRO_YOUT_H			69
#define IMU_GYRO_YOUT_L			70
#define IMU_GYRO_ZOUT_H			71
#define IMU_GYRO_ZOUT_L			72
#define IMU_FIFO_COUNT_L		115
#define IMU_FIFO_R_W			116

// IMU Default Register Values
/// CONFIG = 0x0 results in:
///  Gyro: 250Hz bandwidth, 8kHz Fs
#define D_IMU_CONFIG			0x0 	//0b00000000
/// GYRO_CONFIG = 0x0 results in: +/-250dps max range
/// GYRO_CONFIG = 0x08 results in: +/-500dps max range
/// GYRO_CONFIG = 0x10 results in: +/-1000dps max range
/// GYRO_CONFIG = 0x18 results in: +/-2000dps max range
#define D_IMU_GYRO_CONFIG		0x10		//0b00010000
/// ACCEL_CONFIG = 0x8 results in:
///  Accel: +/- 4g max range
#define D_IMU_ACCEL_CONFIG		0x08	//0b00001000
/// ACCEL_CONFIG2 = 0x0 results in:
///  Accel: 460Hz bandwidth, 1.94ms delay, 220ug/rtHz noise density, 1kHz rate
#define D_IMU_ACCEL_CONFIG2		0x0		//0b00000000

// IMU Reset Register Values (bits auto clear)
#define D_SIG_COND_RST			0x1		//write to USER_CTRL: reset signal paths + clear regs
#define D_SIGNAL_PATH_RESET		0x07	//write to SIGNAL_PATH_RESET: reset signal paths only
#define D_DEVICE_RESET			0x80	//write to PWR_MGMT_1: reset internal regs

//A lot of the I2C macros are blocking. Modified version:

#define I2C_0_TRANSMIT_DATA_MANUAL_TIMEOUT													\
                                    do{														\
                                        I2C_0_TRANSMIT_DATA;								\
                                        while(I2C_0_CHECK_BYTE_COMPLETE(I2C_0_CSR_REG))		\
                                        {													\
                                            /* Wait when byte complete is cleared */		\
											t++;											\
											if(t > timeout)									\
												break;										\
											else											\
												CyDelayUs(1);								\
                                        }													\
                                    }while(0)


		
//****************************************************************************
// Prototype(s):
//****************************************************************************
// functions you'll use in main.c, probably
void init_imu(void);					//turn on IMU, initialize IMU registers to starting values
int16 get_accel_x(void);				//read&return IMU Xaccel
int16 get_accel_y(void);				//Yaccel data
int16 get_accel_z(void);				//Zaccel data
void get_accel_xyz(void);
int16 get_gyro_x(void);					//Xgyro data
int16 get_gyro_y(void);					//Ygyro data
int16 get_gyro_z(void);					//Zgyro data
void get_gyro_xyz(void);
void reset_imu(void);					//reset IMU registers to default
void disable_imu(void);					//disable the IMU by shutting down clocks, etc.

// low level functions; probably don't have to call them in main.c
int imu_write(uint8 internal_reg_addr, uint8* pData, uint16 length); 
void imu_test_code_blocking(void);

//****************************************************************************
// Structure(s):
//****************************************************************************

#endif //INC_IMU_H_ 
