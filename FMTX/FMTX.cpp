/*****************************************************************************/
/*!
    @file     	FMTX.cpp
    @author   	www.elechouse.com
	@version  	V1.0
	@date		2012-11-1
	@brief    	FMTX demo header file

	@section  HISTORY
	
    V1.0    Initial version.

    Copyright (c) 2012 www.elechouse.com  All right reserved.
*/
/*****************************************************************************/
#include <Arduino.h>
#include <Wire.h>
#include <FMTX.h>

void i2c_init()
{
    Wire.begin(SDA,SCL); // join i2c bus (address optional for master)
}

uint8_t fmtx_reg[18]={
    0x00, 0x01, 0x02, 0x04, 0x0B,
    0x0C, 0x0E, 0x0F, 0x10, 0x12,
    0x13, 0x14, 0x15, 0x16, 0x17,
    0x1E, 0x26, 0x27,
};

uint8_t fmtx_reg_val[18]={

};

void fmtx_write_reg(uint8_t reg, uint8_t dt)
{

    Wire.beginTransmission(FMTX_CMD_ADDRESS);
    Wire.write(reg);            // sends instruction byte
    Wire.write(dt);             // sends value byte
    Wire.endTransmission(true);

}

uint8_t fmtx_read_reg(uint8_t reg)
{
    uint8_t dt=0;

    Wire.beginTransmission(FMTX_CMD_ADDRESS );
    Wire.write(reg);
    Wire.endTransmission(false);
    size_t bytesReceived =  Wire.requestFrom((uint8_t)FMTX_CMD_ADDRESS, (size_t)1,true);
#ifdef __FM_DEBUG
   Serial.printf("bytesReceived: %u\n",bytesReceived);
#endif
   if(bytesReceived){

        dt=Wire.read();
    }
#ifdef __FM_DEBUG
    Serial.printf("recieved: %d\n",dt);
#endif

    return dt;
}

/**
    Register    0x00, 0x01, 0x02, 0x04, 0x0B, 0x0C, 0x0E, 0x0F, 0x10, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x1E, 0x26, 0x27,
    Read        0x5C, 0xC3, 0x40, 0x04, 0x00, 0x00, 0xC2, 0xFB, 0xA8, 0x80, 0x80, 0x00, 0xE0, 0x00, 0x00, 0x00, 0xA0, 0x00,
    Default     0x5C, 0xC3, 0x40, 0x04, 0x00, 0x00, 0x02, 0x14, 0xA8, 0x80, 0x80, 0x00, 0xE0, 0x00, 0x00, 0x00, 0xA0, 0x00,
    others      0x7A, 0x03, 0x01, 0x04, 0x00, 0x00, 0xC2, 0xFB, 0xA8, 0x80, 0x80, 0x00, 0xE0, 0x00, 0x04, 0x40, 0xA0, 0x00,
				0x5C, 0xC3, 0x40, 0x04, 0x00, 0x00, 0xC2, 0x00, 0xA8, 0x80, 0x80, 0x00, 0xE0, 0x00, 0x00, 0x40, 0xA0, 0x00,
*/
void fmtx_read_all(uint8_t *buf)
{
    uint8_t i;
    for(i=0; i<18; i++){
        buf[i] = fmtx_read_reg(fmtx_reg[i]);
    }
}

void fmtx_set_freq(float freq)
{
    uint16_t f;
    uint8_t reg0, reg1_8, reg9_11;

    reg0 = fmtx_read_reg(0x02);
    reg9_11 = fmtx_read_reg(0x01);

    freq *= 20;
    f = (uint16_t)freq;
    f &= 0x0FFF;

    if(f&0x01){
        reg0 |= 0x80;
    }else{
        reg0 &= ~0x80;
    }

    reg1_8 = (uint8_t)(f>>1);
    reg9_11 = (reg9_11&0xF8) | (uint8_t)(f>>9);

    fmtx_write_reg(0x02, reg0);
    fmtx_write_reg(0x01, reg9_11);
    fmtx_write_reg(0x00, reg1_8);

}

void fmtx_set_pga(fmtx_pga_type pga)
{
    uint8_t reg;
    uint8_t pga_val;
    reg = fmtx_read_reg(0x01);
    pga_val = (uint8_t)pga;
    pga_val &= ~0xC7;
    reg = (reg&0xC7) | pga_val;
    fmtx_write_reg(0x01, reg);
}

void fmtx_set_rfgain(uint8_t rfgain)
{
    uint8_t reg3, reg0_1, reg2;

    reg0_1 = fmtx_read_reg(0x01);
    reg2 = fmtx_read_reg(0x13);
    reg3 = fmtx_read_reg(0x02);

    rfgain &= 0x0F;
    reg0_1 = (reg0_1&0x3F) | (rfgain<<6);
    if(rfgain & 0x04){
        reg2 |= 0x80;
    }else{
        reg2 &= ~0x80;
    }

    if(rfgain & 0x08){
        reg3 |= 0x40;
    }else{
        reg3 &= ~0x40;
    }

    fmtx_write_reg(0x01, reg0_1);
    fmtx_write_reg(0x13, reg2);
    fmtx_write_reg(0x02, reg3);
}

void fmtx_set_alc(uint8_t sta)
{
    uint8_t reg;
    reg = fmtx_read_reg(0x04);
    if(!sta){
        reg &= ~0x80;
    }else{
        reg |= 0x80;
    }
    fmtx_write_reg(0x04, reg);
}

void fmtx_pa_external()
{
    uint8_t reg;

    reg = fmtx_read_reg(0x13);
    while( !(fmtx_read_reg(0x0F)&0x10));

    reg |= 0x04;
    fmtx_write_reg(0x13, reg);
}

void fmtx_set_sl(void)
{
    uint8_t reg;
    reg = fmtx_read_reg(0x12);
    fmtx_write_reg(0x12, (reg&0x81) | 0x7E);

    reg = fmtx_read_reg(0x14);
    fmtx_write_reg(0x14, (reg&0x02) | 0xDD);

    reg = fmtx_read_reg(0x16);
    fmtx_write_reg(0x16, (reg&0xF8) | 0x07);

    reg = fmtx_read_reg(0x0B);
    fmtx_write_reg(0x1B, reg | 0x04);

    reg = fmtx_read_reg(0x12);
    fmtx_write_reg(0x12, reg&0x7F);
}

void fmtx_set_phcnst(country_type country)
{
    uint8_t reg;
    reg = fmtx_read_reg(0x02);
    switch(country){
        case USA:
        case JAPAN:
            reg &= ~0x01;
            break;
        case EUROPE:
        case AUSTRALIA:
        case CHINA:
            reg |= 0x01;
            break;
        default:
            break;
    }
    fmtx_write_reg(0x02, reg);
}

void fmtx_set_au_enhance(void)
{
    uint8_t reg;
    reg = fmtx_read_reg(0x17);

    fmtx_write_reg(0x17, reg |= 0x20);
}

void fmtx_set_xtal(void)
{
    uint8_t reg;
    reg = fmtx_read_reg(0x1E);
    fmtx_write_reg(0x1E, reg | 0x40);
}

void fmtx_init(float freq, country_type country)
{

    i2c_init();
	fmtx_set_freq(freq);
	fmtx_set_rfgain(4);
	fmtx_set_pga(PGA_0DB);
	fmtx_set_phcnst(country);
	fmtx_set_xtal();
}
