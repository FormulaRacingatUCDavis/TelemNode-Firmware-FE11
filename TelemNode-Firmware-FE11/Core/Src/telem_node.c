#include "telem_node.h"
#include "adc.h"

#define VOLTAGE_DIVIDER_RATIO (12.0 / (12.0 + 6.04))

// HANDLE TYPE DEFS from main
extern ADC_HandleTypeDef hadc1;
extern CAN_HandleTypeDef hcan;
extern TIM_HandleTypeDef htim1;


// PRIVATE GLOBALS
ADC_Input_t adc_inlet_temp = {};
ADC_Input_t adc_outlet_temp = {};

// PRIVATE FUNCTION PROTOTYPES
void get_cooling_data(cooling_data_t* cd);
int16_t get_pres(uint16_t adc_val);
int16_t get_temp(uint16_t adc_val);


void TelemNode_Init(){
	HAL_CAN_Start(&hcan);
	ADC_Init(&adc_inlet_temp, &hadc1, ADC_CHANNEL_0, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_1CYCLE_5);
	ADC_Init(&adc_outlet_temp, &hadc1, ADC_CHANNEL_1, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_1CYCLE_5);
}

void TelemNode_Update()
{
	ADC_Measure(&adc_inlet_temp, 1000);
	ADC_Measure(&adc_outlet_temp, 1000);

	int16_t inlet_temp = get_temp(adc_inlet_temp.value);
	int16_t outlet_temp = get_temp(adc_outlet_temp.value);

	//can_send_cooling_data(&cooling_data);
	//update_pwm();

	for(int i = 0; i < WHEEL_SPEED_LOOPS; i++)
	{
		//get_wheel_speed();
		//can_send_wheel_speed();
		HAL_Delay(LOOP_PERIOD_MS);
	}
}

void update_pwm()
{
	//TODO: implement fan curve

<<<<<<< Updated upstream
}

void get_temp_pres(cooling_data_t* cd)
=======

int16_t get_pres(uint16_t adc_val)
>>>>>>> Stashed changes
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

<<<<<<< Updated upstream
int16_t get_temp(ADC_HandleTypeDef* hadc, uint32_t channel)
{
	float adc_val = (float)get_adc_single_conversion(hadc, channel);
	float mv = adc_val * 5000.0/4095.0;
	//TODO: convert to temperature
	return (int16_t)mv;
}

uint32_t get_adc_single_conversion(ADC_HandleTypeDef* hadc, uint32_t channel)
{
	set_adc_channel(hadc, channel);

	if(HAL_ADC_Start(hadc) != HAL_OK){
		Error_Handler();
	}

	if(HAL_ADC_PollForConversion(hadc, 100) != HAL_OK){
		Error_Handler();
	}

	return HAL_ADC_GetValue(hadc);
}

void set_adc_channel(ADC_HandleTypeDef* hadc, uint32_t channel)
{
	ADC_ChannelConfTypeDef sConfig = {0};
	sConfig.Channel = ADC_CHANNEL_0;
	sConfig.Rank = ADC_REGULAR_RANK_1;
	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
	{
		Error_Handler();
	}
}

void set_pump_speed(uint8_t percent_speed)
{
	uint32_t CCR = htim1.Init->Period * percentage / 100;
	htim1.Instance->CCR1 = CCR;
}

void set_fan_speed(uint8_t percent_speed)
{
	uint32_t CCR = htim1.Init->Period * percentage / 100;
	htim1.Instance->CCR2 = CCR;
}
=======
//uint32_t get_adc_single_conversion(ADC_HandleTypeDef* hadc, uint32_t channel)
//{
//	set_adc_channel(hadc, channel);
//
//	if(HAL_ADC_Start(hadc) != HAL_OK){
//		Error_Handler();
//	}
//
//	if(HAL_ADC_PollForConversion(hadc, 100) != HAL_OK){
//		Error_Handler();
//	}
//
//	return HAL_ADC_GetValue(hadc);
//}
//
//void set_adc_channel(ADC_HandleTypeDef* hadc, uint32_t channel)
//{
//	ADC_ChannelConfTypeDef sConfig = {0};
//	sConfig.Channel = ADC_CHANNEL_0;
//	sConfig.Rank = ADC_REGULAR_RANK_1;
//	sConfig.SamplingTime = ADC_SAMPLETIME_1CYCLE_5;
//	if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
//	{
//		Error_Handler();
//	}
//}
>>>>>>> Stashed changes
