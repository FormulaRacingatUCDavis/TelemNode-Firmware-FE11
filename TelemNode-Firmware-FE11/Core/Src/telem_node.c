#include "telem_node.h"
#include "adc.h"
#include "pwm.h"
#include "can_manager.h"
#include "main.h"
#include "wheel_speed.h"

#define VOLTAGE_DIVIDER_RATIO (12.0 / (12.0 + 6.04))
#define PSI_PER_KPA 0.145038
#define HI8(x) ((x>>8)&0xFF)
#define LO8(x) (x&0xFF);
#define BUZZ_TIME_MS 1500

#define PUMP_THRESH 400
#define FAN_THRESH_1 400
#define FAN_THRESH_2 500
#define FAN_THRESH_3 600
#define HYSTERESIS 30

// HANDLE TYPE DEFS from main
extern ADC_HandleTypeDef hadc1;
extern TIM_HandleTypeDef htim1;

extern CAN_DATA_t can_data;

// PRIVATE GLOBALS
ADC_Input_t adc_inlet_temp;
ADC_Input_t adc_outlet_temp;
ADC_Input_t adc_inlet_pres;
ADC_Input_t adc_outlet_pres;
ADC_Input_t adc_air_in_temp;
ADC_Input_t adc_air_out_temp;

PWM_Output_t pwm_fan;
PWM_Output_t pwm_pump;
WheelSpeed_t wheel_rr;
WheelSpeed_t wheel_rl;


// PRIVATE FUNCTION PROTOTYPES
uint16_t get_pres(uint16_t adc_val);
int16_t get_temp(uint16_t adc_val);
int16_t get_air_temp(uint16_t adc_val);
void set_fan_speed(uint8_t speed);
void set_pump_speed(uint8_t speed);
void update_pwm(int16_t inlet_temp);
void buzzerer();

void TelemNode_Init(){

	CAN_Init();

	ADC_Input_Init(&adc_inlet_pres, &hadc1, ADC_CHANNEL_4, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
	ADC_Input_Init(&adc_outlet_pres, &hadc1, ADC_CHANNEL_5, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
	ADC_Input_Init(&adc_inlet_temp, &hadc1, ADC_CHANNEL_0, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
	ADC_Input_Init(&adc_outlet_temp, &hadc1, ADC_CHANNEL_2, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
	ADC_Input_Init(&adc_air_in_temp, &hadc1, ADC_CHANNEL_1, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);
	ADC_Input_Init(&adc_air_out_temp, &hadc1, ADC_CHANNEL_3, ADC_REGULAR_RANK_1, ADC_SAMPLETIME_239CYCLES_5);

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
	ADC_Measure(&adc_air_in_temp, 1000);
	ADC_Measure(&adc_air_out_temp, 1000);

	int16_t inlet_temp = get_temp(adc_inlet_temp.value);
	int16_t outlet_temp = get_temp(adc_outlet_temp.value);
	int16_t temp_air_in = get_air_temp(adc_air_in_temp.value);
	int16_t temp_air_out = get_air_temp(adc_air_out_temp.value);

	tx_data[0] = HI8(inlet_temp);
	tx_data[1] = LO8(inlet_temp);
	tx_data[2] = HI8(outlet_temp);
	tx_data[3] = LO8(outlet_temp);
	tx_data[4] = HI8(temp_air_in);
	tx_data[5] = LO8(temp_air_in);
	tx_data[6] = HI8(temp_air_out);
	tx_data[7] = LO8(temp_air_out);
	CAN_Send(0x400, tx_data, 8);

	ADC_Measure(&adc_inlet_pres, 1000);
	ADC_Measure(&adc_outlet_pres, 1000);

	uint16_t inlet_pres = get_pres(adc_inlet_pres.value);
	uint16_t outlet_pres = get_pres(adc_outlet_pres.value);

	tx_data[0] = HI8(inlet_pres);
	tx_data[1] = LO8(inlet_pres);
	tx_data[2] = HI8(outlet_pres);
	tx_data[3] = LO8(outlet_pres);
	CAN_Send(0x402, tx_data, 4);

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
	// allow manual fan and pump speed via CAN
	if (can_data.PWM_requested) {
		set_pump_speed(can_data.pumpPWM);
		set_fan_speed(can_data.fanPWM);
		return;
	}

	// threshold variables to add hysteresis
	static int fan_t1 = FAN_THRESH_1 + HYSTERESIS;
	static int fan_t2 = FAN_THRESH_2 + HYSTERESIS;
	static int fan_t3 = FAN_THRESH_2 + HYSTERESIS;
	static int pump_t = PUMP_THRESH + HYSTERESIS;

	//TODO: update these values to consider ambient air temp, vehicle speed, etc?
	if(can_data.inverter_enable || (can_data.mc_temp_max > pump_t) || (can_data.motor_temp > pump_t)){
		set_pump_speed(255);
		pump_t = PUMP_THRESH;
	} else {
		set_pump_speed(0);
		pump_t = PUMP_THRESH + HYSTERESIS;
	}

	if(inlet_temp > fan_t3){
		set_fan_speed(255);
		fan_t1 = FAN_THRESH_1;
		fan_t2 = FAN_THRESH_2;
		fan_t3 = FAN_THRESH_3;
	} else if(inlet_temp > fan_t2){
		set_fan_speed(180);
		fan_t1 = FAN_THRESH_1;
		fan_t2 = FAN_THRESH_2;
		fan_t3 = FAN_THRESH_3 + HYSTERESIS;
	} else if(inlet_temp > fan_t1){
		fan_t1 = FAN_THRESH_1;
		fan_t2 = FAN_THRESH_2 + HYSTERESIS;
		fan_t3 = FAN_THRESH_3 + HYSTERESIS;
		set_fan_speed(100);
	} else {
		fan_t1 = FAN_THRESH_1 + HYSTERESIS;
		fan_t2 = FAN_THRESH_2 + HYSTERESIS;
		fan_t3 = FAN_THRESH_3 + HYSTERESIS;
		set_fan_speed(0);
	}
}

uint16_t get_pres(uint16_t adc_val)
{
	// GE2098 sensor
	// equation from datasheet
	float v = (float)adc_val * (3.3/4095.0) / VOLTAGE_DIVIDER_RATIO;
	float pres = (((v / 5.0) + 0.011453) / 0.0045726) * PSI_PER_KPA;
	return (uint16_t)(pres*100);
}

int16_t get_temp(uint16_t adc_val)
{
	// need to recalibrate these sensors with new GE2098
	return adc_val;
}

int16_t get_air_temp(uint16_t adc_val)
{
	float temp = 83.35412 - 0.03634221 * adc_val +0.0000034466 * adc_val * adc_val;
	return (int16_t) (temp *10);
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



