/*
 * IMU.c
 *
 *  Created on: Nov 18, 2026
 *      Author: cooke
 */

#include "IMU.h"

static void IMU_write(uint8_t reg, uint8_t new_val){
	// we send an address byte first and then the actual data byte second
	// The first address byte's MSB is 1 for Read and 0 for Write, so we're always going to make the MSB zero in this function

	uint8_t data_packet[2] = {reg & 0x7F, new_val};
	HAL_GPIO_WritePin(SPI1_IMU_CS_GPIO_PORT, SPI1_IMU_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_Transmit(hspi1, data_packet, 2, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(SPI1_IMU_CS_GPIO_PORT, SPI1_IMU_CS_Pin, GPIO_PIN_SET);
}

static uint8_t IMU_read(uint8_t reg){
	// almost same idea as IMU_write
	uint8_t read_val;
	uint8_t data_packet[2] = {reg | 0x80, read_val};
	HAL_GPIO_WritePin(SPI1_IMU_CS_GPIO_PORT, SPI1_IMU_CS_Pin, GPIO_PIN_RESET);
	HAL_SPI_TransmitReceive(hsp1, data_packet, 2, HAL_MAX_DELAY);
	HAL_GPIO_WritePin(SPI1_IMU_CS_GPIO_PORT, SPI1_IMU_CS_Pin, GPIO_PIN_SET);

	return data_packet[1];
}

static void IMU_init(){
	// under the assumption this is ONLY used at the start and we don't know what state the cs pin is at
	HAL_GPIO_WritePin(SPI1_IMU_CS_GPIO_PORT, SPI1_IMU_CS_Pin, GPIO_PIN_SET);
	HAL_Delay(50);

	IMU_write(DEVICE_CONFIG, 0x01);
	HAL_Delay(5); // needs at least 1ms to reset

	IMU_write(PWR_MGMT0, 0x0F);
	HAL_Delay(5); // needs at least 200 microseconds before any other writes

	IMU_write(FIFO_CONFIG, 0x10);
	IMU_write(FIFO_CONFIG1, 0x07);


	IMU_write(SIGNAL_PATH_RESET, 0x02);
}
