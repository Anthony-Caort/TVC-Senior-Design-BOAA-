/*
 * IMU.h
 *
 *  Created on: Nov 18, 2026
 *      Author: cooke
 */

#ifndef INC_IMU_H_
#define INC_IMU_H_

/*
 * Defines for registers
 *
 *	1: Initialization registers
 *		DEVICE_CONFIG     -> Enable soft reset for sanity check
 *		FIFO_CONFIG       -> Stream-to-FIFO mode
 *		SIGNAL_PATH_RESET -> Flushes FIFO
 *		PWR_MGMT0         -> Leave the temp on (makes our lives easier) and turn on the gyroscope and accelerometer
 *		FIFO_CONFIG1      -> Turn on reading temp, gyro, and acceleraton
 *		WHO_AM_I          -> Reading a value, and if it's 0x47 that means we are reading right and its most certainly an IMU, might be redundant
 *	2: Registers in use while running :D
 */

#define DEVICE_CONFIG	   0x11 // Used to reset
#define FIFO_CONFIG        0x16
#define SIGNAL_PATH_RESET  0x4B // flushes previous FIFO data
#define PWR_MGMT0          0x4E // turning on accelerometer and gyroscope
#define GYRO_CONFIG0       0x4F // Scales how much dps we read and odr for gyro, probably not gonna use but good to keep for debugging
#define ACCEL_CONFIG0      0x50 // Pretty much the same thing as the gyro but for acceleration, same purpose as well
#define FIFO_CONFIG1       0x5F
#define WHO_AM_I           0x75 // identity register

#define FIFO_COUNTH        0x2E // reads the FIFO buffer to show the available bytes in there
#define FIFO_DATA          0x30 // read port
#define REG_BANK_SEL       0x76 // selects the register bank, we usually stay in bank 0

extern SPI_HandleTypeDef hspi1;

//TODO: change the read and write functions to incorporate adjustable cs pins when we get the magnetometer .. maybe
static void IMU_write(uint8_t reg, uint8_t new_val);
static uint8_t IMU_read(uint8_t reg);
static void IMU_init(); // might change back to void

// retrieve FIFO packet

#endif /* INC_IMU_H_ */
