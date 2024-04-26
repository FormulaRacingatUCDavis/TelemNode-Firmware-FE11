#include "telem_node.h"
#include "adc.h"
#include "pwm.h"
#include "can_manager.h"
#include "main.h"
#include "wheel_speed.h"

#define VOLTAGE_DIVIDER_RATIO (12.0 / (12.0 + 6.04))
#define HI8(x) ((x>>8)&0xFF)
#define LO8(x) (x&0xFF);
#define BUZZ_TIME_MS 1500

// HANDLE TYPE DEFS from main
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;

extern CAN_DATA_t can_data;

// PRIVATE GLOBALS
ADC_Input_t adc_inlet_temp;
ADC_Input_t adc_outlet_temp;
PWM_Output_t pwm_fan;
PWM_Output_t pwm_pump;
WheelSpeed_t wheel_rr;
WheelSpeed_t wheel_rl;


// PRIVATE FUNCTION PROTOTYPES
int16_t get_pres(uint16_t adc_val);
int16_t get_temp(uint16_t adc_val);
void set_fan_speed(uint8_t speed);
void set_pump_speed(uint8_t speed);
void update_pwm(int16_t inlet_temp);
void buzzerer();

void TelemNode_Init(){

	CAN_Init();

	ADC_Input_Init(&adc_inlet_temp, &hadc1, ADC_CHANNEL_4, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
	ADC_Input_Init(&adc_outlet_temp, &hadc1, ADC_CHANNEL_5, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);

	PWM_Init(&pwm_fan, &htim1, TIM_CHANNEL_1);
	PWM_Init(&pwm_pump, &htim1, TIM_CHANNEL_2);

	WheelSpeed_Init(&wheel_rr);
	WheelSpeed_Init(&wheel_rl);

	set_pump_speed(255);
	set_fan_speed(0);
}

void TelemNode_Update()
{
	static uint8_t tx_data[8];

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

	if(CAN_Send(0x400, tx_data, 8) != HAL_OK)
	{
		Error_Handler();
	}

	for(int i = 0; i < WHEEL_SPEED_LOOPS; i++)
	{
		// casting to uint16_t, should never overflow
		uint16_t cps_rr = (uint16_t)WheelSpeed_GetCPS(&wheel_rr);
		uint16_t cps_rl = (uint16_t)WheelSpeed_GetCPS(&wheel_rl);

		tx_data[0] = HI8(cps_rr);
		tx_data[1] = LO8(cps_rr);
		tx_data[2] = HI8(cps_rl);
		tx_data[3] = LO8(cps_rl);

		if(CAN_Send(0x401, tx_data, 4) != HAL_OK)
		{
			Error_Handler();
		}

		buzzerer();
		update_pwm(inlet_temp);
		HAL_Delay(LOOP_PERIOD_MS);
	}
}

void update_pwm(int16_t inlet_temp)
{
	//TODO: update these values to consider ambient air temp, vehicle speed, etc?
	if(can_data.inverter_enable || (can_data.mc_temp_max > 400) || (can_data.motor_temp > 400)){
		set_pump_speed(255);
	} else {
		set_pump_speed(0);
	}

	if(inlet_temp > 600){
		set_fan_speed(255);
	} else if(inlet_temp > 500){
		set_fan_speed(180);
	} else if(inlet_temp > 400){
		set_fan_speed(100);
	} else {
		set_fan_speed(0);
	}

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
	//v = v - 0.040; // ground reference is 40m
	//float temp = ((v - 0.5) * (120 + 40) / 4.0) - 40;
	float temp = (46.7558 * v) - 65.9775; // values from calibration run
	return (int16_t)(temp * 10);
}

void set_pump_speed(uint8_t speed)
{
	PWM_SetDutyCycle(&pwm_pump, 255-speed);
}

void set_fan_speed(uint8_t speed)
{
	PWM_SetDutyCycle(&pwm_fan, 255-speed);
}

void buzzerer()
{
	static VCU_STATE_t last_vcu_state = LV;
	static uint32_t buzz_start = 0;
	uint32_t tick = HAL_GetTick();

	if(last_vcu_state == HV_ENABLED && can_data.vcu_state == DRIVE)
	{
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, 1); // turn on buzzer
		buzz_start = tick;
	}
	else if(can_data.vcu_state != DRIVE || (tick - buzz_start) > BUZZ_TIME_MS)
	{
		HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, 0); // turn off buzzer
	}
	else
	{
		// ice cream?
	}

	last_vcu_state = can_data.vcu_state;
}

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	switch(GPIO_Pin){
		case GPIO_PIN_15:
			wheel_rr.count++;
			break;
		case GPIO_PIN_8:
			wheel_rl.count++;
			break;
	}
}



