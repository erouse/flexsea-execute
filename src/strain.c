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
	[Lead developper] Jean-Francois Duval, jfduval at dephy dot com.
	[Origin] Based on Jean-Francois Duval's work at the MIT Media Lab 
	Biomechatronics research group <http://biomech.media.mit.edu/>
	[Contributors]
*****************************************************************************
	[This file] strain: strain gauge amplifier
*****************************************************************************
	[Change log] (Convention: YYYY-MM-DD | author | comment)
	* 2016-09-29 | jfduval | Released under GPL-3.0 release
	*
****************************************************************************/

//****************************************************************************
// Include(s)
//****************************************************************************

#include "main.h"
#include "strain.h"

//****************************************************************************
// Variable(s)
//****************************************************************************

struct strain_s strain1;
uint16 adc_strain_filtered = 0;
volatile uint16 adc_strain = 0;
volatile uint16 adc_delsig_dma_array[8];

//****************************************************************************
// Function(s)
//****************************************************************************

//Enables the peripherals associated with the strain amplifier and
//sets the default values.
//Make sure that you initialize I2C1 first!
void init_strain(void)
{
	//Peripherals:
	//=-=-=-=-=-=
	
	Opamp_2_Start();		//VR1
	
	//16-bits ADC:
	ADC_DelSig_1_Start();
	ADC_DelSig_1_IRQ_Enable();
	dma_2_config();
	isr_delsig_Start();
	
	//Defaults:
	//=-=-=-=-=-=
	
	strain1.ch[0].offset = STRAIN_DEFAULT_OFFSET;
	strain1.ch[0].gain = STRAIN_DEFAULT_GAIN;
	//strain.oref = STRAIN_DEFAULT_OREF;	
	strain1.ch[0].vo1 = 0;
	//strain.vo2 = 0;
	strain1.ch[0].strain_filtered = 0;
	strain_config(strain1.ch[0].offset, strain1.ch[0].gain);
}

//Configure the strain gauge amplifier
void strain_config(uint8 offs, uint8 gain)
{
	uint8 i2c_init_buf[2];
	
	//Offset:
	i2c_init_buf[0] = STRAIN_OFFSET;
	i2c_init_buf[1] = offs;		//Offset
	I2C_1_MasterWriteBuf(I2C_POT_ADDR, (uint8 *) i2c_init_buf, 2, I2C_1_MODE_COMPLETE_XFER);	
	CyDelayUs(100);
	
	//Second stage gain:
	i2c_init_buf[0] = STRAIN_GAIN;
	i2c_init_buf[1] = gain;		//Gain
	I2C_1_MasterWriteBuf(I2C_POT_ADDR, (uint8 *) i2c_init_buf, 2, I2C_1_MODE_COMPLETE_XFER);
	CyDelayUs(100);
}

//Returns the latest filtered strain measurement
uint16 strain_read(void)
{
	//ADC is auto-sampling, this function simply returns the last filtered value
	
	//return strain.filtered_strain;	//Buggy
	return adc_strain_filtered;
}

//Moving average filter:
uint16 strain_filter(void)
{
	uint32 sum = 0;
	uint8 cnt = 0;
	uint16 avg = 0;
	static uint16 vo2_buf[STRAIN_BUF_LEN];
	
	//Shift buffer and sum all but last term
	for(cnt = 1; cnt < (STRAIN_BUF_LEN); cnt++)
	{
		vo2_buf[cnt-1] = vo2_buf[cnt];
		sum += vo2_buf[cnt-1];
	}
	
	//vo2_buf[STRAIN_BUF_LEN - 1] = strain.vo2;	//Buggy, see Evernote
	vo2_buf[STRAIN_BUF_LEN - 1] = adc_strain;
	sum += vo2_buf[STRAIN_BUF_LEN - 1];
		
	//Average
	avg = (uint16)(sum >> STRAIN_SHIFT);
	
	//Store in structure:
	strain1.ch[0].strain_filtered = avg;
	adc_strain_filtered = avg;
	
	return avg;	
}

//With DMA transfers we get a full buffer (6 bytes) per interrupt
//Note: we take 6 samples and ignore the first 2. This is a workaround, as the first 2 values are often contaminated.
uint16 strain_filter_dma(void)
{
	uint32 sum = 0;
	uint8 cnt = 0;
	uint16 avg = 0;
	
	//Sum all the terms
	for(cnt = 2; cnt < STRAIN_BUF_LEN; cnt++)
	{
		sum += adc_delsig_dma_array[cnt];
	}
	
	//Average
	avg = (uint16)(sum >> STRAIN_SHIFT);
	
	//Store in structure:
	strain1.ch[0].strain_filtered = avg;
	adc_strain_filtered = avg;
	
	return avg;	
}

//Compress 6x uint16 to 9 bytes (12bits per sensor).
//Needed to send all data with RIC/NU Read All function
uint8_t compressAndSplit6ch(uint8_t *buf, uint16_t ch0, uint16_t ch1, uint16_t ch2, \
                            uint16_t ch3, uint16_t ch4, uint16_t ch5)
{
    uint8_t tmp0 = 0, tmp1 = 0;
    uint16_t tmp[6] = {0,0,0,0,0,0};
    uint16_t combo[5] = {0,0,0,0,0};

    //Compress to 12bits
    tmp[0] = (ch0 >> 4) & 0x0FFF;
    tmp[1] = (ch1 >> 4) & 0x0FFF;
    tmp[2] = (ch2 >> 4) & 0x0FFF;
    tmp[3] = (ch3 >> 4) & 0x0FFF;
    tmp[4] = (ch4 >> 4) & 0x0FFF;
    tmp[5] = (ch5 >> 4) & 0x0FFF;

    //We want:
    //combo[0]: 0000 0000 0000 1111
    //combo[1]: 1111 1111 2222 2222
    //combo[2]: 2222 3333 3333 3333
    //combo[3]: 4444 4444 4444 5555
    //combo[4]: ____ ____ 5555 5555

    //Combine:
    combo[0] = (tmp[0] << 4) | ((tmp[1] >> 8) & 0xFF);
    combo[1] = (tmp[1] << 8) | ((tmp[2] >> 4) & 0xFFFF);
    combo[2] = (tmp[2] << 12) | (tmp[3]);
    combo[3] = (tmp[4] << 4) | ((tmp[5] >> 8) & 0xFF);
    combo[4] = (tmp[5] & 0xFF);


    //Stock in uint8_t buffer:
    uint16_to_bytes((uint16_t)combo[0], &tmp0, &tmp1);
    *(buf) = tmp0;
    *(buf+1) = tmp1;
    uint16_to_bytes((uint16_t)combo[1], &tmp0, &tmp1);
    *(buf+2) = tmp0;
    *(buf+3) = tmp1;
    uint16_to_bytes((uint16_t)combo[2], &tmp0, &tmp1);
    *(buf+4) = tmp0;
    *(buf+5) = tmp1;
    uint16_to_bytes((uint16_t)combo[3], &tmp0, &tmp1);
    *(buf+6) = tmp0;
    *(buf+7) = tmp1;
    *(buf+8) = (uint8_t)combo[4];

    return 0;
}

//Unpack from buffer
void unpackCompressed6ch(uint8_t *buf, uint16_t *v0, uint16_t *v1, uint16_t *v2, \
                            uint16_t *v3, uint16_t *v4, uint16_t *v5)
{
    *v0 = ((*(buf+0) << 8 | *(buf+1)) >> 4) << 4;
    *v1 = (((*(buf+1) << 8 | *(buf+2))) & 0xFFF) << 4;
    *v2 = ((*(buf+3) << 8 | *(buf+4)) >> 4) << 4;
    *v3 = (((*(buf+4) << 8 | *(buf+5))) & 0xFFF) << 4;
    *v4 = ((*(buf+6) << 8 | *(buf+7)) >> 4) << 4;
    *v5 = (((*(buf+7) << 8 | *(buf+8))) & 0xFFF) << 4;
}

void compress6chTestCodeBlocking(void)
{
    uint8_t buffer[20];
    uint16_t strainValues[6] = {0x1111, 0x2222, 0x3333, 0x4444, 0x5555, 0x6666};
    uint16_t results[6] = {0,0,0,0,0,0};

    while(1)
    {
        compressAndSplit6ch(buffer, strainValues[0], strainValues[1], \
                            strainValues[2],strainValues[3],\
                            strainValues[4],strainValues[5]);

        unpackCompressed6ch(buffer, &results[0], &results[1], &results[2], \
                            &results[3], &results[4], &results[5]);
    }
}

//Copy of the test code used in main.c to test the hardware:
void strain_test_blocking(void)
{
	//Strain Amplifier test:
	
	uint8 i2c_test_wbuf[9];
	uint8 vr1 = 0;
	uint8 ledg_state = 0;
	
	i2c_test_wbuf[0] = STRAIN_OFFSET;
	i2c_test_wbuf[1] = 127;	//Offset of ~ V/2
	I2C_1_MasterWriteBuf(I2C_POT_ADDR, (uint8 *) i2c_test_wbuf, 2, I2C_1_MODE_COMPLETE_XFER);	
	CyDelay(10);
	i2c_test_wbuf[0] = STRAIN_GAIN;
	i2c_test_wbuf[1] = 10;	//Relatively small gain
	I2C_1_MasterWriteBuf(I2C_POT_ADDR, (uint8 *) i2c_test_wbuf, 2, I2C_1_MODE_COMPLETE_XFER);	
	
	i2c_test_wbuf[0] = STRAIN_OFFSET;
	while(1)
	{
		vr1++;		
		i2c_test_wbuf[1] = vr1;	//Enable this line to test the offset
		I2C_1_MasterWriteBuf(I2C_POT_ADDR, (uint8 *) i2c_test_wbuf, 2, I2C_1_MODE_COMPLETE_XFER);	
		
		ledg_state ^= 1;
		LED_G_Write(ledg_state);
		
		CyDelayUs(1000);
	}
}

//DMA for Delta Sigma ADC
void dma_2_config(void)
{
	/* Variable declarations for DMA_2 */
	/* Move these variable declarations to the top of the function */
	uint8 DMA_2_Chan;
	uint8 DMA_2_TD[1];

	/* DMA Configuration for DMA_2 */
	#define DMA_2_BYTES_PER_BURST 2
	#define DMA_2_REQUEST_PER_BURST 1
	#define DMA_2_SRC_BASE (CYDEV_PERIPH_BASE)
	#define DMA_2_DST_BASE (CYDEV_SRAM_BASE)
	DMA_2_Chan = DMA_2_DmaInitialize(DMA_2_BYTES_PER_BURST, DMA_2_REQUEST_PER_BURST, 
	    HI16(DMA_2_SRC_BASE), HI16(DMA_2_DST_BASE));
	DMA_2_TD[0] = CyDmaTdAllocate();
	CyDmaTdSetConfiguration(DMA_2_TD[0], DMA2_BYTES_PER_XFER, DMA_2_TD[0], DMA_2__TD_TERMOUT_EN | TD_INC_DST_ADR);
	CyDmaTdSetAddress(DMA_2_TD[0], LO16((uint32)ADC_DelSig_1_DEC_SAMP_PTR), LO16((uint32)adc_delsig_dma_array));
	CyDmaChSetInitialTd(DMA_2_Chan, DMA_2_TD[0]);
	CyDmaChEnable(DMA_2_Chan, 1);
}
