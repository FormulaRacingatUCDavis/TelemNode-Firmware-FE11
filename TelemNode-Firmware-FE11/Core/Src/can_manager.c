/*
 * can_manager.c
 *
 *  Created on: Apr 18, 2024
 *      Author: leoja
 */

#include "can_manager.h"
#include "main.h"

extern CAN_HandleTypeDef hcan;

CAN_DATA_t can_data;

void CAN_Init()
{
	// Filter vehicle state messages into FIFO0
	CAN_FilterTypeDef can_filter;
	can_filter.FilterActivation = CAN_FILTER_ENABLE;
	can_filter.SlaveStartFilterBank = 28;
	can_filter.FilterBank = 0;
	can_filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
	can_filter.FilterMode = CAN_FILTERMODE_IDMASK;
	can_filter.FilterScale = CAN_FILTERSCALE_32BIT;
	can_filter.FilterIdHigh = 0;
	can_filter.FilterIdLow = 0;
	can_filter.FilterMaskIdHigh = 0;
	can_filter.FilterMaskIdLow = 0;
	if (HAL_CAN_ConfigFilter(&hcan, &can_filter) != HAL_OK) {
	  Error_Handler();
	}
	if (HAL_CAN_ActivateNotification(&hcan, CAN_IT_RX_FIFO0_MSG_PENDING) != HAL_OK) {
		Error_Handler();
	}
	if ( HAL_CAN_Start(&hcan) != HAL_OK) {
		Error_Handler();
	}
}

HAL_StatusTypeDef CAN_Send(uint32_t id, uint8_t* data, uint8_t len)
{
	static uint32_t TxMailbox;
	static CAN_TxHeaderTypeDef msg_hdr;
	msg_hdr.IDE = CAN_ID_STD;
	msg_hdr.StdId = id;
	msg_hdr.RTR = CAN_RTR_DATA;
	msg_hdr.DLC = len;

	if(HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) return HAL_OK;
	return HAL_CAN_AddTxMessage(&hcan, &msg_hdr, data, &TxMailbox);
}

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef* hcan_ptr)
{
	CAN_RxHeaderTypeDef can_rx_header;
	uint8_t can_rx_data[8];

 	if (HAL_CAN_GetRxMessage(hcan_ptr, CAN_RX_FIFO0, &can_rx_header, can_rx_data) != HAL_OK) {
		Error_Handler();
	}

	switch(can_rx_header.StdId)
	{
		case VEHICLE_STATE:
			can_data.vcu_state = can_rx_data[5];
			break;
		case BMS_STATUS_MSG:
			can_data.bms_temp = can_rx_data[0];
			break;
		case MC_TEMPS_1:
			can_data.mc_temp_module_a = (can_rx_data[1] << 8);
			can_data.mc_temp_module_a |= can_rx_data[0];

			can_data.mc_temp_module_c = (can_rx_data[3] << 8);
			can_data.mc_temp_module_c |= can_rx_data[2];

			can_data.mc_temp_module_c = (can_rx_data[5] << 8);
			can_data.mc_temp_module_c |= can_rx_data[4];

			can_data.mc_temp_max = can_data.mc_temp_module_a;
			if(can_data.mc_temp_module_b > can_data.mc_temp_max){
				can_data.mc_temp_max = can_data.mc_temp_module_b;
			}
			if(can_data.mc_temp_module_c > can_data.mc_temp_max){
				can_data.mc_temp_max = can_data.mc_temp_module_c;
			}
			break;
		case MC_TEMPS_3:
			can_data.motor_temp = (can_rx_data[5] << 8);
			can_data.motor_temp |= can_rx_data[4];
			break;
		case TORQUE_REQUEST:
			can_data.inverter_enable = can_rx_data[5] & 0x01;
			break;
		case PUMP_PWM:
			can_data.PWM_requested = 1;
			can_data.pumpPWM = can_rx_data[0];
			can_data.fanPWM = can_rx_data[1];
			break;
	}
}
