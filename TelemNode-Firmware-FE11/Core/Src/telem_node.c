#include "telem_node.h"

typedef struct{
	int16_t inlet_temp;
	int16_t outlet_temp;
	int16_t inlet_pres;
	int16_t outlet_pres;
} cooling_data_t;


// HANDLE TYPE DEFS from main
extern ADC_HandleTypeDef hadc1;
extern CAN_HandleTypeDef hcan;
extern TIM_HandleTypeDef htim1;


void TelemNode_Init(){
	HAL_CAN_Start(&hcan);
}

void TelemNode_Update()
{
	cooling_data_t cooling_data;

	get_cooling_data(&cooling_data);
	can_send_cooling_data(&cooling_data);
	update_pwm();

	for(int i = 0; i < WHEEL_SPEED_LOOPS; i++)
	{
		get_wheel_speed();
		can_send_wheel_speed();
		delay(LOOP_PERIOD_MS);
	}
}

void update_pwm()
{
	//TODO: implement fan curve

}

void get_temp_pres(cooling_data_t* cd)
{
	cd->inlet_temp = get_temp(&hadc1, ADC_CHANNEL_0);
	cd->outlet_temp = get_temp(&hadc1, ADC_CHANNEL_1);
	//cd->inlet_pres = get_pres();
	//cd->outlet_pres = get_pres();
}

int16_t get_pres(ADC_HandleTypeDef* hadc, uint32_t channel)
{
	float adc_val = (float)get_adc_single_conversion(hadc, channel);
	float mv = adc_val * 5000.0/4095.0;
	//TODO: convert to temperature
	return (int16_t)mv;
}

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
