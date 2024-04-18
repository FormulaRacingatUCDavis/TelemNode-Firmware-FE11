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

HAL_StatusTypeDef CAN_Init()
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

	return HAL_CAN_Start(&hcan);
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
	}
}
