#include "cc1101.h"
#include "cc1101_registers.h"
#include "pwr.h"
#include "counterModule.h"
#include "led.h"
typedef struct {
	uint8_t reg;
	uint8_t value;
} cc1101_regValue;

static uint8_t cc1101_registers[0x40];

uint16_t cc1101_rTime;
uint8_t cc1101_rSlot;

#define CC1101_RSSI_OFFSET 		74

typedef enum {
	CC1101_STATE_NONE = (uint8_t)0,
	CC1101_STATE_WAIT_RX,
	CC1101_STATE_RX,
	CC1101_STATE_TX,
	CC1101_STATE_IDLE,
} cc1101_moduleStateT;

static cc1101_moduleStateT cc1101_moduleState = CC1101_STATE_NONE;
static bool cc1101_rxFlag;

static uint64_t cc1101_timer = 0;

//867.999 / GFSK / 38.4kBaud / Dev=20kHz
static const cc1101_regValue cc1101_config[] = {
		  {CC1101_REG_IOCFG0,      0x46},
		  {CC1101_REG_FIFOTHR,     0x07},
		  {CC1101_REG_PKTLEN,      0xff},
		  {CC1101_REG_PKTCTRL1,    0x0c},
		  {CC1101_REG_PKTCTRL0,    0x45},
		  {CC1101_REG_ADDR,		   0x04},
		  {CC1101_REG_FSCTRL1,     0x06},
		  {CC1101_REG_FREQ2,       0x21},
		  {CC1101_REG_FREQ1,       0x62},
		  {CC1101_REG_FREQ0,       0x76},
		  {CC1101_REG_MDMCFG4,     0xca},
		  {CC1101_REG_MDMCFG3,     0x83},
		  {CC1101_REG_MDMCFG2,     0x13},
		  {CC1101_REG_MDMCFG1,     0x22},
		  {CC1101_REG_DEVIATN,     0x35},
		  {CC1101_REG_MCSM0,       0x18},
		  {CC1101_REG_FOCCFG,      0x16},
		  {CC1101_REG_BSCFG,	   0x6c},
		  {CC1101_REG_AGCTRL2,     0x43},
		  {CC1101_REG_AGCTRL1,     0x40},
		  {CC1101_REG_AGCTRL0,     0x91},
		  {CC1101_REG_WORCTL,      0xFB},
		  {CC1101_REG_FREND1,	   0x56},
		  {CC1101_REG_FSCAL3,      0xe9},
		  {CC1101_REG_FSCAL2,      0x2a},
		  {CC1101_REG_FSCAL1,      0x00},
		  {CC1101_REG_FSCAL0,      0x1F},
		  {CC1101_REG_TEST2,       0x81},
		  {CC1101_REG_TEST1,       0x35},
		  {CC1101_REG_TEST0,       0x09},
};

//static const cc1101_regValue cc1101_config[] = {
//	{CC1101_REG_IOCFG0,      0x06},
//	{CC1101_REG_FIFOTHR,     0x47},
//	{CC1101_REG_PKTCTRL0,    0x05},
//	{CC1101_REG_FSCTRL1,     0x06},
//	{CC1101_REG_FREQ2,       0x21},
//	{CC1101_REG_FREQ1,       0x62},
//	{CC1101_REG_FREQ0,       0x76},
//	{CC1101_REG_MDMCFG4,     0xF5},
//	{CC1101_REG_MDMCFG3,     0x83},
//	{CC1101_REG_MDMCFG2,     0x93},
//	{CC1101_REG_DEVIATN,     0x15},
//	{CC1101_REG_MCSM0,       0x18},
//	{CC1101_REG_FOCCFG,      0x16},
//	{CC1101_REG_WORCTL,      0xFB},
//	{CC1101_REG_FSCAL3,      0xE9},
//	{CC1101_REG_FSCAL2,      0x2A},
//	{CC1101_REG_FSCAL1,      0x00},
//	{CC1101_REG_FSCAL0,      0x1F},
//	{CC1101_REG_TEST2,       0x81},
//	{CC1101_REG_TEST1,       0x35},
//	{CC1101_REG_TEST0,       0x09}
//};

static void cc1101_setCS(){
	LL_GPIO_ResetOutputPin(SPI1_CS_GPIO_Port, SPI1_CS_Pin);
}

static void cc1101_resetCS(){
	LL_GPIO_SetOutputPin(SPI1_CS_GPIO_Port, SPI1_CS_Pin);
}

static void cc1101_SPI_TransmitReceive(uint8_t *txData, uint8_t *rxData, uint8_t count){
//	cc1101_setCS();
	uint8_t tmp = LL_SPI_ReceiveData8(SPI1);
	uint8_t rxCount = count;
	uint8_t txCount = count;

	uint8_t txEnabled = 1;

	while(rxCount || txCount){
		if(LL_SPI_IsActiveFlag_TXE(SPI1) && txCount && txEnabled){
			LL_SPI_TransmitData8(SPI1, *txData);
			txData++;
			txCount--;
			txEnabled = 0;
		}
		if(LL_SPI_IsActiveFlag_RXNE(SPI1) && rxCount){
			*rxData = LL_SPI_ReceiveData8(SPI1);
			rxData++;
			rxCount--;
			txEnabled = 1;
		}
	}

	while(LL_SPI_IsActiveFlag_BSY(SPI1)){}

//	cc1101_resetCS();
}

static void cc1101_SPI_Transmit(uint8_t *txData, uint8_t count){
//	cc1101_setCS();

	while(count){
		if(LL_SPI_IsActiveFlag_TXE(SPI1) && count){
			LL_SPI_TransmitData8(SPI1, *txData);
			txData++;
			count--;
		}
	}

	while(LL_SPI_IsActiveFlag_BSY(SPI1)){}

//	cc1101_resetCS();
}

static cc1101_SPI_Reveive(uint8_t *rxData, uint8_t count){
	uint8_t tmp = LL_SPI_ReceiveData8(SPI1);
	uint8_t rxCount = count;
	uint8_t txCount = count;

	uint8_t txEnabled = 1;

	while(rxCount || txCount){
		if(LL_SPI_IsActiveFlag_TXE(SPI1) && txCount && txEnabled){
			SPI1->DR = 0;
			txCount--;
			txEnabled = 0;
		}
		if(LL_SPI_IsActiveFlag_RXNE(SPI1) && rxCount){
			*rxData = SPI1->DR;
			rxData++;
			rxCount--;
			txEnabled = 1;
		}
	}

	while(LL_SPI_IsActiveFlag_BSY(SPI1)){}
}


static void cc1101_strobe(uint8_t strobe){
	cc1101_setCS();
	uint8_t data = (strobe & CC1101_ADDRESS_MASK) | CC1101_WRITE_SINGLE;
	cc1101_SPI_Transmit(&data, 1);
	cc1101_resetCS();
}

static void cc1101_writeReg(uint8_t reg, uint8_t value){
	cc1101_setCS();
	uint8_t txbuff[2];
	txbuff[0] = (reg & CC1101_ADDRESS_MASK) | CC1101_WRITE_SINGLE;
	txbuff[1] = value;
	cc1101_SPI_Transmit(txbuff, 2);
	cc1101_resetCS();
}

static uint8_t cc1101_writeRegBurst(uint8_t reg, uint8_t * value, uint8_t count){
	cc1101_setCS();
	uint8_t address = (reg & CC1101_ADDRESS_MASK) | CC1101_WRITE_BURST;

	cc1101_SPI_Transmit(&address, 1);
	cc1101_SPI_Transmit(value, count);
	cc1101_resetCS();
}

static uint8_t cc1101_readReg(uint8_t reg){
	cc1101_setCS();
	uint8_t value;
	uint8_t address = (reg & CC1101_ADDRESS_MASK) | CC1101_READ_BURST;
	cc1101_SPI_Transmit(&address, 1);
	cc1101_SPI_Reveive(&value, 1);
	cc1101_resetCS();
	return value;
}

static void cc1101_readRegBurst(uint8_t reg, uint8_t * value, uint8_t count){
	cc1101_setCS();
	uint8_t address = (reg & CC1101_ADDRESS_MASK) | CC1101_READ_BURST;
	cc1101_SPI_Transmit(&address, 1);
	cc1101_SPI_Reveive(value, count);
	cc1101_resetCS();
}

static void cc1101_pushTXFIFO(uint8_t * data, uint8_t size){
	cc1101_strobe(CC1101_STROBE_IDLE);
    cc1101_writeReg(CC1101_REG_PATABLE, 0x50);
	cc1101_writeRegBurst(CC1101_REG_FIFO, data, size);
	cc1101_moduleState = CC1101_STATE_TX;
	cc1101_strobe(CC1101_STROBE_TX);
}

static void cc1101_popRXFIFO(uint8_t * data, uint8_t * size){
	cc1101_strobe(CC1101_STROBE_IDLE);
	uint8_t count = cc1101_readReg(CC1101_REG_RXBYTES);
	if (count > *size){
		count = *size;
	} else {
		*size = count;
	}
	cc1101_readRegBurst(CC1101_REG_FIFO, data, count);
//	cc1101_strobe(CC1101_STROBE_FRX);
//	cc1101_strobe(CC1101_STROBE_RX);
}

static void cc1101_initSPI(){

	  /* USER CODE BEGIN SPI1_Init 0 */

	  /* USER CODE END SPI1_Init 0 */

	  LL_SPI_InitTypeDef SPI_InitStruct = {0};

	  LL_GPIO_InitTypeDef GPIO_InitStruct = {0};

	  /* Peripheral clock enable */
	  LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SPI1);

	  LL_IOP_GRP1_EnableClock(LL_IOP_GRP1_PERIPH_GPIOA);
	  /**SPI1 GPIO Configuration
	  PA5   ------> SPI1_SCK
	  PA6   ------> SPI1_MISO
	  PA7   ------> SPI1_MOSI
	  */
	  GPIO_InitStruct.Pin = LL_GPIO_PIN_4;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_OUTPUT;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  GPIO_InitStruct.Pin = LL_GPIO_PIN_5;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  GPIO_InitStruct.Pin = LL_GPIO_PIN_6;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  GPIO_InitStruct.Pin = LL_GPIO_PIN_7;
	  GPIO_InitStruct.Mode = LL_GPIO_MODE_ALTERNATE;
	  GPIO_InitStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
	  GPIO_InitStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
	  GPIO_InitStruct.Pull = LL_GPIO_PULL_NO;
	  GPIO_InitStruct.Alternate = LL_GPIO_AF_0;
	  LL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	  /* USER CODE BEGIN SPI1_Init 1 */

	  /* USER CODE END SPI1_Init 1 */
	  SPI_InitStruct.TransferDirection = LL_SPI_FULL_DUPLEX;
	  SPI_InitStruct.Mode = LL_SPI_MODE_MASTER;
	  SPI_InitStruct.DataWidth = LL_SPI_DATAWIDTH_8BIT;
	  SPI_InitStruct.ClockPolarity = LL_SPI_POLARITY_LOW;
	  SPI_InitStruct.ClockPhase = LL_SPI_PHASE_1EDGE;
	  SPI_InitStruct.NSS = LL_SPI_NSS_SOFT;
	  SPI_InitStruct.BaudRate = LL_SPI_BAUDRATEPRESCALER_DIV16;
	  SPI_InitStruct.BitOrder = LL_SPI_MSB_FIRST;
	  SPI_InitStruct.CRCCalculation = LL_SPI_CRCCALCULATION_DISABLE;
	  SPI_InitStruct.CRCPoly = 7;
	  LL_SPI_Init(SPI1, &SPI_InitStruct);
	  LL_SPI_SetStandard(SPI1, LL_SPI_PROTOCOL_MOTOROLA);
	  LL_SPI_Enable(SPI1);
}

static void cc1101_readAllRegisters(){
	cc1101_readRegBurst(CC1101_REG_IOCFG2, cc1101_registers, 0x2f);
	for (int i = CC1101_REG_PARTNUM; i < CC1101_REG_RCCTRL0_STATUS; i++){
		cc1101_registers[i] = cc1101_readReg(i);
	}
}

static void cc1101_initCC1101(){
	uint8_t count = sizeof(cc1101_config) / sizeof(cc1101_regValue);
	cc1101_strobe(CC1101_STROBE_RES);
	for (int i = 0; i < 1000; i ++);
	for (int i = 0; i < count; i++){
		cc1101_writeReg(cc1101_config[i].reg, cc1101_config[i].value);
	}
	cc1101_strobe(CC1101_STROBE_IDLE);
	cc1101_strobe(CC1101_STROBE_FRX);
	cc1101_strobe(CC1101_STROBE_FTX);
	cc1101_moduleState = CC1101_STATE_IDLE;
	NVIC_SetPriority(EXTI4_15_IRQn, 0);

	NVIC_EnableIRQ(EXTI4_15_IRQn);

}

uint8_t RSSIDBM (uint8_t rssi_dec){
	uint8_t rssi_dbm;
		if (rssi_dec >= 128) {
			rssi_dbm=CC1101_RSSI_OFFSET + (256-rssi_dec)/2;
		} else {
			rssi_dbm=CC1101_RSSI_OFFSET - (rssi_dec/2);
		}
	return rssi_dbm;
}

void cc1101_init(){
	cc1101_rxFlag = false;
	cc1101_moduleState = CC1101_STATE_NONE;
	cc1101_initSPI();
	cc1101_strobe(CC1101_STROBE_RES);
//	cc1101_initAddGPIO();
	for (int i = 0; i < 1000; i ++){

	}
	cc1101_readAllRegisters();
	cc1101_initCC1101();

	cc1101_turnOn();

//	volatile static uint8_t thr;
//	volatile static uint8_t part;
//	volatile static uint8_t vers;
//	cc1101_readRegBurst(CC1101_REG_PARTNUM, &res, 1);

//	cc1101_strobe(CC1101_STROBE_RES);

//	part = cc1101_readReg(CC1101_REG_PARTNUM);
//	vers = cc1101_readReg(CC1101_REG_VERSION);
//	thr = cc1101_readReg(CC1101_REG_FIFOTHR);
//
//	cc1101_readAllRegisters();

//	LL_LPUART_Enable(LPUART1);
//	for (int i = 0; i < sizeof(cc1101_registers); i++){
//		while(!LL_LPUART_IsActiveFlag_TXE(LPUART1)){}
//		LL_LPUART_TransmitData8(LPUART1, cc1101_registers[i]);
//	}

#ifdef RX
//	cc1101_strobe(CC1101_STROBE_RX);
#endif
//	if (thr != 7){
//		thr = cc1101_readReg(CC1101_REG_FIFOTHR);
//	}
	cc1101_timer = 0;
}

void cc1101_extI(){

	if (cc1101_moduleState == CC1101_STATE_WAIT_RX){
		cc1101_rTime = LL_LPTIM_GetCounter(LPTIM1);
		cc1101_moduleState = CC1101_STATE_RX;
//		cc1101_strobe(CC1101_STROBE_RX);
		cc1101_rxFlag = true;
	} else {
//		led_led1Off();
//		cc1101_moduleState = CC1101_STATE_WAIT_RX;
//		cc1101_strobe(CC1101_STROBE_RX);
	}
}


void cc1101_exec(){
	if (cc1101_moduleState == CC1101_STATE_RX){
		static uint8_t tmp[128];
		NVIC_DisableIRQ(EXTI4_15_IRQn);
		uint8_t bytes = cc1101_readReg(CC1101_REG_RXBYTES) & 0x7f;
		uint8_t br = bytes;
		cc1101_popRXFIFO(tmp, &br);
		if (bytes == br){
			if (br){
//				if (tmp[1] == 'D' &&
//						tmp[2] == 'T'){
//					cc1101_rSlot = tmp[3];
//					cc1101_sendAck(cc1101_rTime, cc1101_rSlot);
//				}
//				led_led1Toggle();
				pcUart_pushSendBuff(tmp, br);
//				pcUart_pushSendBuff("\n", 1);
			}
			cc1101_moduleState = CC1101_STATE_WAIT_RX;
			cc1101_strobe(CC1101_STROBE_RX);
		}
		NVIC_EnableIRQ(EXTI4_15_IRQn);

			/// Send data to parse
	}

	static uint8_t cntr = 0;
}

static uint8_t cc1101_syncCount = 0;

void cc1101_sendAck(uint16_t t, uint8_t slot){
//	for (int i = 0; i < 50000; i++){
//		__asm("nop");
//	}
//	uint8_t tmp[17];
//	tmp[0] = 6;
//	memcpy(&tmp[1], "AK", 2);
//	tmp[3] = slot;
//	tmp[4] = t & 0xff;
//	tmp[5] = (t >> 8) & 0xff;
//	cc1101_pushTXFIFO(tmp, 7);
	uint8_t tmp[17];
	tmp[0] = 6;
	memcpy(&tmp[1],"AK  ", 4);
	tmp[5] = cc1101_syncCount;
	cc1101_syncCount ++;
	cc1101_pushTXFIFO(tmp, 7);
}

void cc1101_sendSync(){
//	led_led1On();
//	uint8_t tmp[17];
//	tmp[0] = 6;
//	memcpy(&tmp[1],"SYNC", 4);
//	tmp[5] = cc1101_syncCount;
//	cc1101_syncCount ++;
//	cc1101_pushTXFIFO(tmp, 7);
////	led_led1Off();
////					led_led2Toggle();

}

void cc1101_sendData(uint8_t * data){
	led_led1On();
	uint8_t tmp[17];
	tmp[0] = 16;
	for (int i = 0; i < 16; i++){
		tmp[i + 1] = data[i];
	}
	cc1101_pushTXFIFO(tmp, 17);
	led_led1Off();
//					led_led2Toggle();

}


void cc1101_turnOn(){
	cc1101_strobe(CC1101_STROBE_FRX);
	cc1101_strobe(CC1101_STROBE_RX);
	cc1101_moduleState = CC1101_STATE_WAIT_RX;
	pwr_sleep();
}

void cc1101_turnOff(){

}
