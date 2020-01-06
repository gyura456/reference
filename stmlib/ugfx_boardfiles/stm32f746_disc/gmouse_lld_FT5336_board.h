/*
 * This file is subject to the terms of the GFX License. If a copy of
 * the license was not distributed with this file, you can obtain one at:
 *
 *              http://ugfx.org/license.html
 */

#ifndef _GINPUT_LLD_MOUSE_BOARD_H
#define _GINPUT_LLD_MOUSE_BOARD_H

// Resolution and Accuracy Settings
#define GMOUSE_FT5336_PEN_CALIBRATE_ERROR		8
#define GMOUSE_FT5336_PEN_CLICK_ERROR			6
#define GMOUSE_FT5336_PEN_MOVE_ERROR			4
#define GMOUSE_FT5336_FINGER_CALIBRATE_ERROR	14
#define GMOUSE_FT5336_FINGER_CLICK_ERROR		18
#define GMOUSE_FT5336_FINGER_MOVE_ERROR			14

// How much extra data to allocate at the end of the GMouse structure for the board's use
#define GMOUSE_FT5336_BOARD_DATA_SIZE			0

// The FT5336 I2C slave address (including the R/W bit)
#define FT5336_SLAVE_ADDR 0x70

#define TIMINGS 0x40912732		// Discovery BSP code from ST examples

#if !GFX_USE_OS_CHIBIOS
	#define AFRL	AFR[0]
	#define AFRH	AFR[1]
#endif

static I2CConfig touch_i2c = {
    TIMINGS,
    I2C_CR1_ANFOFF | I2C_CR1_PECEN,
    0
};

void ft5336_i2cInit(void){
    i2cStart(&I2CD3, &touch_i2c);
}


/*
 * The CR2 register needs atomic access. Hence always use this function to setup a transfer configuration.
 */
static void _i2cConfigTransfer(I2C_TypeDef* i2c, uint16_t slaveAddr, uint8_t numBytes, uint32_t mode, uint32_t request)
{
	uint32_t tmpreg = 0;

	// Get the current CR2 register value
	tmpreg = i2c->CR2;

	// Clear tmpreg specific bits
	tmpreg &= (uint32_t) ~((uint32_t) (I2C_CR2_SADD | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_AUTOEND | I2C_CR2_RD_WRN | I2C_CR2_START | I2C_CR2_STOP));

	// update tmpreg
	tmpreg |= (uint32_t) (((uint32_t) slaveAddr & I2C_CR2_SADD) | (((uint32_t) numBytes << 16) & I2C_CR2_NBYTES) | (uint32_t) mode | (uint32_t) request);

	// Update the actual CR2 contents
	i2c->CR2 = tmpreg;
}

/*
 * According to the STM32Cube HAL the CR2 register needs to be reset after each transaction.
 */
static void _i2cResetCr2(I2C_TypeDef* i2c)
{
	i2c->CR2 &= (uint32_t) ~((uint32_t) (I2C_CR2_SADD | I2C_CR2_HEAD10R | I2C_CR2_NBYTES | I2C_CR2_RELOAD | I2C_CR2_RD_WRN));
}

static void i2cSend(I2C_TypeDef* i2c, uint8_t slaveAddr, uint8_t* data, uint16_t length)
{
	// We are currently not able to send more than 255 bytes at once
	if (length > 255) {
		return;
	}

	// Setup the configuration
	_i2cConfigTransfer(i2c, slaveAddr, length, (!I2C_CR2_RD_WRN) | I2C_CR2_AUTOEND, I2C_CR2_START);

	// Transmit the whole buffer
	while (length > 0) {
		while (!(i2c->ISR & I2C_ISR_TXIS));
		i2c->TXDR = *data++;
		length--;
	}

	// Wait until the transfer is complete
	while (!(i2c->ISR & I2C_ISR_TXE));

	// Wait until the stop condition was automagically sent
	while (!(i2c->ISR & I2C_ISR_STOPF));

	// Reset the STOP bit
	i2c->ISR &= ~I2C_ISR_STOPF;

	// Reset the CR2 register
	_i2cResetCr2(i2c);
}


void i2cWriteReg(uint8_t slaveAddr, uint8_t regAddr, uint8_t value)
{
	uint8_t txbuf[2];
	txbuf[0] = regAddr;
	txbuf[1] = value;

	i2cSend(I2CD3.i2c, slaveAddr, txbuf, 2);
}

static void i2cRead(I2C_TypeDef* i2c, uint8_t slaveAddr, uint8_t* data, uint16_t length)
{
	int		i;

	// We are currently not able to read more than 255 bytes at once
	if (length > 255) {
		return;
	}

	// Setup the configuration
	_i2cConfigTransfer(i2c, slaveAddr, length, I2C_CR2_RD_WRN | I2C_CR2_AUTOEND, I2C_CR2_START);

	// Transmit the whole buffer
	for (i = 0; i < length; i++) {
		while (!(i2c->ISR & I2C_ISR_RXNE));
		data[i] = i2c->RXDR;
	}

	// Wait until the stop condition was automagically sent
	while (!(i2c->ISR & I2C_ISR_STOPF));

	// Reset the STOP bit
	i2c->ISR &= ~I2C_ISR_STOPF;

	// Reset the CR2 register
	_i2cResetCr2(i2c);
}

uint8_t i2cReadByte(uint8_t slaveAddr, uint8_t regAddr)
{
	uint8_t ret = 0xAA;

	i2cSend(I2CD3.i2c, slaveAddr, &regAddr, 1);
	i2cRead(I2CD3.i2c, slaveAddr, &ret, 1);

	return ret;
}

uint16_t i2cReadWord(uint8_t slaveAddr, uint8_t regAddr)
{
	uint8_t ret[2] = { 0xAA, 0xAA };

	i2cSend(I2CD3.i2c, slaveAddr, &regAddr, 1);
	i2cRead(I2CD3.i2c, slaveAddr, ret, 2);

	return (uint16_t)((ret[0] << 8) | (ret[1] & 0x00FF));
}

static bool_t init_board(GMouse* m, unsigned instance)
{
	(void)m;
	(void)instance;
    ft5336_i2cInit();
	return TRUE;
}

static void write_reg(GMouse* m, uint8_t reg, uint8_t val)
{
	(void)m;
	i2cWriteReg(FT5336_SLAVE_ADDR, reg, val);

}

static uint8_t read_byte(GMouse* m, uint8_t reg)
{
	(void)m;
    return i2cReadByte(FT5336_SLAVE_ADDR, reg);
}

static uint16_t read_word(GMouse* m, uint8_t reg)
{
	(void)m;
    return i2cReadWord(FT5336_SLAVE_ADDR, reg);
}

#endif /* _GINPUT_LLD_MOUSE_BOARD_H */
