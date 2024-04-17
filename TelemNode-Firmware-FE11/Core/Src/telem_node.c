#include "telem_node.h"
#include "adc.h"

#define VOLTAGE_DIVIDER_RATIO (12.0 / (12.0 + 6.04))
#define HI8(x) ((x>>8)&0xFF)
#define LO8(x) (x&0xFF);

// HANDLE TYPE DEFS from main
extern ADC_HandleTypeDef hadc1;
extern CAN_HandleTypeDef hcan;
extern TIM_HandleTypeDef htim1;


// PRIVATE GLOBALS
ADC_Input_t adc_inlet_temp;
ADC_Input_t adc_outlet_temp;
uint32_t TxMailbox2;

// PRIVATE FUNCTION PROTOTYPES
int16_t get_pres(uint16_t adc_val);
int16_t get_temp(uint16_t adc_val);
HAL_StatusTypeDef can_send(uint32_t id, uint8_t* data, uint8_t len);


void TelemNode_Init(){
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	sConfig.Rank = 1;

	sConfig.Channel = ADC_CHANNEL_0;
	adc_inlet_temp.h_adc = &hadc1;
	adc_inlet_temp.sConfig = &sConfig,
	adc_inlet_temp.value = 0;

	sConfig.Channel = ADC_CHANNEL_1;
	adc_outlet_temp.h_adc = &hadc1,
	adc_outlet_temp.sConfig = &sConfig,
	adc_outlet_temp.value = 0;

	HAL_CAN_Start(&hcan);
	ADC_Input_Init(&adc_inlet_temp, &hadc1, ADC_CHANNEL_0, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_1CYCLE_5);
	ADC_Input_Init(&adc_outlet_temp, &hadc1, ADC_CHANNEL_1, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_1CYCLE_5);
}

void TelemNode_Update()
{
	uint8_t tx_data[8];

	ADC_Measure(&adc_inlet_temp, 1000);
	ADC_Measure(&adc_outlet_temp, 1000);

	int16_t inlet_temp = get_temp(adc_inlet_temp.value);
	int16_t outlet_temp = get_temp(adc_outlet_temp.value);

	tx_data[0] = HI8(inlet_temp);
	tx_data[1] = LO8(inlet_temp);
	tx_data[2] = HI8(outlet_temp);
	tx_data[3] = LO8(outlet_temp);
	tx_data[4] = 0;
	tx_data[5] = 0;
	tx_data[6] = 0;
	tx_data[7] = 0;

	can_send(0x400, tx_data, 8);

	for(int i = 0; i < WHEEL_SPEED_LOOPS; i++)
	{
		//get_wheel_speed();
		//can_send_wheel_speed();
		HAL_Delay(LOOP_PERIOD_MS);
	}
}

HAL_StatusTypeDef can_send(uint32_t id, uint8_t* data, uint8_t len)
{
	CAN_TxHeaderTypeDef msg_hdr;
	msg_hdr.IDE = CAN_ID_STD;
	msg_hdr.StdId = id;
	msg_hdr.RTR = CAN_RTR_DATA;
	msg_hdr.DLC = len;

	return HAL_CAN_AddTxMessage(&hcan, &msg_hdr, data, &TxMailbox2);
}

void update_pwm()
{
	//TODO: implement fan curve
}

int16_t get_pres(uint16_t adc_val)
{
	// TDH60W temp/pressure sensors have a FUCKING TRASH datasheet that doesn't list output curves
	// 25psi sensors?
	// assuming pressure range is 0-25psi, mapped to 0.5V-4.5V

	float v = (float)adc_val * (3.3/4095.0) / VOLTAGE_DIVIDER_RATIO;
	float temp = (v - 0.5) * 25.0 / 4.0;
	return (int16_t)(temp * 10);
}

int16_t get_temp(uint16_t adc_val)
{
	// TDH60W temp/pressure sensors have a FUCKING TRASH datasheet that doesn't list output curves
	// only reference to temperature is 125C max
	// assuming temperature range is 0C-125C mapped to 0.5V-4.5V

	float v = (float)adc_val * (3.3/4095.0) / VOLTAGE_DIVIDER_RATIO;
	float temp = (v - 0.5) * 125.0 / 4.0;
	return (int16_t)(temp * 10);
}

