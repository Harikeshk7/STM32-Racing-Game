/**
  ******************************************************************************
  * @file	main.c
  * @author  Ac6
  * @version V1.0
  * @date	01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

extern bool start_game;

//#include "nunchuk.h"
//===========================================================================
// Initialize I2C
//===========================================================================

//#define GPIOEX_ADDR 0x00  // ENTER GPIO EXPANDER I2C ADDRESS HERE
//#define EEPROM_ADDR 0x00  // ENTER EEPROM I2C ADDRESS HERE
// PB8 - AF1 - SCL
// PB9 - AF1 - SDA

void init_i2c(void) {
	// ENABLE PINS
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	//GPIOB->MODER |= GPIO_MODER_MODER8 | GPIO_MODER_MODER9;                      	// MODER PB8 AND PB9 ALTERNATE
	GPIOB->MODER &= ~(0xf0000); //  Alternate function mode
	GPIOB->MODER |= 0xa0000;
	GPIOB->AFR[1] |= 0x11;                                                      	// ALTERNATE FUNCTION 1 FOR PB8 AND PB9


	// START I2C STUFF
	RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

	I2C1->CR1 &= ~I2C_CR1_PE;                                                   	// DISABLE TO PERFORM RESET
	I2C1->CR1 &= ~I2C_CR1_ANFOFF;   	// 0: ANALOG NOISE FILTER ON ** shouldn't affect
	I2C1->CR1 &= ~I2C_CR1_ERRIE;   	// ERROR INTERRUPT DISABLE ** stop or continue
	I2C1->CR1 &= ~I2C_CR1_NOSTRETCH;	// ENABLE CLOCK STRETCHING ** not sure why we need this!!!!!

	// FROM TABLE 83; SET FOR 100KHZ WITH 8MHZ CLK
	I2C1->TIMINGR = 0;
	I2C1->TIMINGR &= ~I2C_TIMINGR_PRESC;                                       	// CLEAR PRESCALER
	I2C1->TIMINGR |= 1 << 28;                                                  	// SET PRESCALER TO 1
	I2C1->TIMINGR |= 4 << 20;                                                  	// SCLDEL - 0x4
	I2C1->TIMINGR |= 2 << 16;                                                  	// SDADEL - 0x2
	I2C1->TIMINGR |= 0xF << 8;                                                 	// SCLH   - 0xF
	I2C1->TIMINGR |= 0x13 << 0;                                                	// SCLL   - 0x13

	// I2C "OWN ADDRESS" 1 REGISTER (I2C_OAR1)
	I2C1->OAR1 &= ~I2C_OAR1_OA1EN;   	// DISABLE OWN ADDRESS 1 ** not sure if both are needed
	I2C1->OAR2 &= ~I2C_OAR2_OA2EN;   	// DISABLE OWN ADDRESS 2 ** not sure if both are needed
	I2C1->CR2 &= ~I2C_CR2_ADD10;     	// ADD10 OF CR2 - 7 BIT ADDRESSING ** check if 7 or 10 bits
	I2C1->CR2 |= I2C_CR2_AUTOEND;                                               	// TURN ON THE AUTOEND SETTING TO ENABLE AUTOMATIC END (STOP BIT AUTOMATICALLY SENT)


	I2C1->CR1 |= I2C_CR1_PE;                                                    	// ENABLE CHANNEL (PE IN CR1)

}

//===========================================================================
// I2C helpers
//===========================================================================

void i2c_waitidle(void) {
	while ((I2C1->ISR & I2C_ISR_BUSY) == I2C_ISR_BUSY); // WHILE BUSY, WAIT
}

void i2c_start(uint32_t devaddr, uint8_t size, uint8_t dir) {
	uint32_t tmpreg = I2C1->CR2;
	tmpreg &= ~(I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND
            	| I2C_CR2_RD_WRN | I2C_CR2_START| I2C_CR2_STOP);

	if (dir == 1)
    	tmpreg |= I2C_CR2_RD_WRN; // read from slave
	else
    	tmpreg &= ~I2C_CR2_RD_WRN; // WRITE TO SLAVE

	tmpreg |= ((devaddr<<1) & I2C_CR2_SADD) | ((size << 16) & I2C_CR2_NBYTES);
	tmpreg |= I2C_CR2_START;
	I2C1->CR2 = tmpreg;
}

void i2c_stop(void) {
	if (I2C1->ISR & I2C_ISR_STOPF)
    	return;
	//master: generate stop bit after current byte has been transferred
	I2C1->CR2 |= I2C_CR2_STOP;
	// WAIT UNTIL STOPF FLAG IS RESET
	while( (I2C1->ISR & I2C_ISR_STOPF) == 0);
	I2C1->ICR |= I2C_ICR_STOPCF; // WRITE TO CLEAR STOPF FLAG
}

int i2c_checknack(void) {
	int i;
	i = (I2C1->ISR >> 4) & 0x1; // get NACKF BIT 4

	return i;
}

void i2c_clearnack(void) {
	I2C1->ICR |= I2C_ICR_NACKCF; // "WRITING A 1 TO THIS BIT CLEARS THE ACKF FLAG
}

int i2c_senddata(uint8_t devaddr, const void *data, uint8_t size) {
	int i;
	if (size <= 0 || data == 0) return (-1);
	uint8_t *udata = (uint8_t*)data;
	i2c_waitidle();
	// LAST ARGUMENT IS DIR: 0 = SENDING DATA TO THE SLAVE DEVICE
	i2c_start(devaddr, size, 0);

	// put data byte by byte in TXDR
	for(i=0; i<size; i++) {
    	int count = 0;
    	while( (I2C1->ISR & I2C_ISR_TXIS) == 0) {
        	count += 1;
        	if (count > 1000000) return -1;
        	if (i2c_checknack()) {i2c_clearnack(); i2c_stop(); return (-1); }
    	}
    	//TXIS IS CLEARED BY WRITING TO THE TXDR REGISTER
    	I2C1->TXDR = udata[i] & I2C_TXDR_TXDATA;
	}
	//WAIT UNTIL TransferComplete FLAG IS SET OR THE NACK FLAG IS SET
	while((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);

	if ((I2C1->ISR & I2C_ISR_NACKF) != 0){
    	return (-1);
	}
	i2c_stop();
	return 0;
}

int i2c_recvdata(uint8_t devaddr, void *data, uint8_t size) {
    	int i;
    	if (size <= 0 || data == 0) return -1;
    	uint8_t *udata = (uint8_t*)data;
    	i2c_waitidle();
    	// Last argument is dir: 1 = receiving data from the slave device
    	i2c_start(devaddr, size, 1);
    	for(i=0; i<size; i++) {
        	int count = 0;
        	while ((I2C1->ISR & I2C_ISR_RXNE) == 0){
            	count += 1;
            	if (count > 1000000) return -1;
            	if (i2c_checknack()) { i2c_clearnack(); i2c_stop(); return -1; }
        	}
        	udata[i] = I2C1->RXDR;
    	}
    	// WAIT UNTIL TC FLAG IS SET OR THE NACK FLAG IS SET
    	while((I2C1->ISR & I2C_ISR_TC) == 0 && (I2C1->ISR & I2C_ISR_NACKF) == 0);
    	if (( I2C1->ISR & I2C_ISR_NACKF) != 0)
        	return -1;
    	i2c_stop();
    	return 0;
}

//===========================================================================
// Timer for I2C
//===========================================================================
void init_tim7(void){

    	RCC->APB1ENR |= RCC_APB1ENR_TIM7EN;
    	TIM7->PSC = 4800-1; //  10 times per second to read and write from I2C
    	TIM7->ARR = 1000-1;
    	TIM7->DIER |= TIM_DIER_UIE;
    	TIM7->CR1 |= TIM_CR1_CEN;
    	NVIC->ISER[0] = 1<<TIM7_IRQn;

}

void TIM7_IRQHandler(void) {

    	TIM7->SR &= ~0x1; // clear the status register to acknowledge the interrupt
    	// the following occurs in the ISR???

    	uint8_t data2[1];
    	data2[0] = 0x0;
    	i2c_senddata(0x52,data2,1);
    	uint8_t control_data1[6];
    	i2c_recvdata(0x52, control_data1, 6);
    	separate_bytes(control_data1);
}



//===========================================================================
// Get XYZ data from nunchuck
//===========================================================================
 /**
 *  0x00:   X-axis of joy stick
 *  0x01:   Y-axis of joy stick
 *  0x02:   X-axis data of accelerometer sensor
 *  0x03:   Y-axis data of accelerometer sensor
 *  0x04:   Z-axis data of accelerometer sensor
 *  0x05:   bit 0   Z button status
 *      	bit 1   C button status
 *      	bits [3:2]  X-axis data of accelerometer (lower 2 bits)
 *      	bits [5:4]  Y-axis data of accelerometer (lower 2 bits)
 *      	bits [7:6]  Z-axis data of accelerometer (lower 2 bits)
 */

void init_nunchuck(void){
	i2c_waitidle();
	i2c_start(0x52, 0, 2);
	uint8_t data[2];
	data[0] = 0xF0;
	data[1] = 0x55;
	i2c_clearnack();
	i2c_stop();
	i2c_senddata(0x52,data,2);

	data[0] = 0xFB;
	data[1] = 0x00;
	i2c_senddata(0x52,data,2);
}

void separate_bytes(uint8_t *control_data){
	uint8_t xData;
	xData = control_data[0];
	uint8_t yData;
	yData = control_data[1];

	uint8_t start_button;
	start_button = ((control_data[5]) & 0x1);

	if(!start_button){
    	//start_screen();
    	start_game = true;
	}

	// initialize to stay the same
	int val = -1;
	int speed = -1;

	if(xData == 0xff){
    	// moving right
    	val = 1;     	// 1 is right
//    	start_screen();
    	change_lane(val);
	}
	else if(xData == 0x00){
    	// moving left
    	val = 0;    	// 0 is left

    	change_lane(val);
	}
	else{
    	// staying still
    	val = -1;
    	change_lane(val);

	}

//	if(yData > 0x80){
//    	// faster
//    	speed = 1;
//    	change_speed(speed);
//
//	}
//	else if(yData < 0x80){
//    	// slower
//    	speed = 0;
//    	change_speed(speed);
//	}
//	else{
//    	// same speed
//    	speed = -1;
//    	change_speed(speed);
//	}

}


int main_i2c(void)
{
	/**
 	* 0xf0 -> 0x55
 	* 0xfb -> 0x00
 	* Device address:  0x52
 	* Reading address: 0xA5
 	* Writing address: 0xA4
 	* */

	init_i2c();
	init_nunchuck();
	init_tim7();
}