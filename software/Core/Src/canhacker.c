//
// Created by marcin on 08.04.2021.
//

#include "canhacker.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>

void CanHacker_Init(CanHacker_HandleTypeDef *canHacker){
    canHacker->timestamp = CANHACKER_TIMESTAMP_DISABLED;
}

__weak void CanHacker_ErrorCallback(CanHacker_HandleTypeDef *canHacker, char *message){

}

__weak void CanHacker_CanTxMsgReadyCallback(CanHacker_HandleTypeDef *canHacker, CanTxMsgTypeDef *txMsg){

}

__weak void CanHacker_UartMsgReadyCallback(CanHacker_HandleTypeDef *canHacker, uint8_t *line) {

}

void CanHacker_ExecGetSWVersion(CanHacker_HandleTypeDef *canHacker) {
    uint8_t str[7];
    str[0] = CANHACKER_GET_SW_VERSION;
    str[1] = nibble2ascii(CANHACKER_SW_VER_MAJOR >> 4);
    str[2] = nibble2ascii(CANHACKER_SW_VER_MAJOR);
    str[3] = nibble2ascii(CANHACKER_SW_VER_MINOR >> 4);
    str[4] = nibble2ascii(CANHACKER_SW_VER_MINOR);
    str[5] = CANHACKER_CR;
    str[6] = '\0';
    CanHacker_UartMsgReadyCallback(canHacker, str);
}

void CanHacker_ExecGetVersion(CanHacker_HandleTypeDef *canHacker) {
    uint8_t str[7];
    str[0] = CANHACKER_GET_VERSION;
    str[1] = nibble2ascii(CANHACKER_HW_VER >> 4);
    str[2] = nibble2ascii(CANHACKER_HW_VER);
    str[3] = nibble2ascii(CANHACKER_SW_VER >> 4);
    str[4] = nibble2ascii(CANHACKER_SW_VER);
    str[5] = nibble2ascii(CANHACKER_CR);
    str[6] = '\0';
    CanHacker_UartMsgReadyCallback(canHacker, str);
}

void CanHacker_ExecGetSerial(CanHacker_HandleTypeDef *canHacker){
    uint8_t str[7];
    str[0] = CANHACKER_GET_SERIAL;
    strcpy((char *)str+1, CANHACKER_SERIAL);
    str[5] = CANHACKER_CR;
    str[6] = '\0';
    CanHacker_UartMsgReadyCallback(canHacker, str);
}

void CanHacker_ExecTransmitR11bit(CanHacker_HandleTypeDef *canHacker, uint8_t *str){
    const int idOffset = 1;
    const int dlcOffset = 4;
    uint8_t cmd_len = strlen((char *)str);
    if (cmd_len != 5){
        CanHacker_ErrorCallback(canHacker, "Error: unexpected command length");
        return;
    }

    CanTxMsgTypeDef CAN_Tx_Msg;
    CAN_Tx_Msg.header.RTR = CAN_RTR_REMOTE;
    CAN_Tx_Msg.header.IDE = CAN_ID_STD;
    // store ID
    CAN_Tx_Msg.header.StdId = ascii2byte(str[idOffset]) << 8 | ascii2byte(str[idOffset+1]) << 4 |
            ascii2byte(str[idOffset + 2]);
    // store data length
    // CAN_Tx_Msg.header.DLC = ascii2byte(str[dlcOffset]);
    CAN_Tx_Msg.header.DLC = str[dlcOffset] & 0x0F;
    // check number of data bytes supplied against data length byte
    if (CAN_Tx_Msg.header.DLC != ((cmd_len - 5) / 2)) {
        char message[80];
        sprintf(message, "Invalid DLC: %d and %d", (int)CAN_Tx_Msg.header.DLC, (int)((cmd_len - 5) / 2));
        CanHacker_ErrorCallback(canHacker, message);
        return;
    }
    // check for valid length
    if (CAN_Tx_Msg.header.DLC > 8){
        CanHacker_ErrorCallback(canHacker, "DLC > 8");
        return;
    }
    CanHacker_CanTxMsgReadyCallback(canHacker, &CAN_Tx_Msg);
}

void CanHacker_ExecTransmit11bit(CanHacker_HandleTypeDef *canHacker, uint8_t* str){
    const int idOffset = 1;
    const int dataOffset = 5;
    const int dlcOffset = 4;
    uint8_t cmd_len = strlen((char *)str);
    if ((cmd_len < 5) || (cmd_len > 21)) {
        CanHacker_ErrorCallback(canHacker, "Error: unexpected command length");
        return;
    }
    uint8_t *cmd_buf_pntr2 = &(*str); // point to start of received string
    cmd_buf_pntr2++; // skip command identifier
    while(*cmd_buf_pntr2) {
        if (!isxdigit (*cmd_buf_pntr2)) {
            CanHacker_ErrorCallback(canHacker, "Error: unexpected command data character");
        }
        ++cmd_buf_pntr2;
    }
    CanTxMsgTypeDef CAN_Tx_Msg;
    CAN_Tx_Msg.header.RTR = CAN_RTR_DATA; // no remote transmission request
    CAN_Tx_Msg.header.IDE = CAN_ID_STD;
    // store ID
    CAN_Tx_Msg.header.StdId = ascii2byte(str[idOffset]) << 8 | ascii2byte(str[idOffset + 1]) << 4 |
            ascii2byte(str[idOffset + 2]);
    // store data length
    //CAN_Tx_Msg.header.DLC = ascii2byte(str[dlcOffset]);
    CAN_Tx_Msg.header.DLC = str[dlcOffset] & 0x0F;
    // check number of bytes supplied against data length byte
    if (CAN_Tx_Msg.header.DLC != ((cmd_len - 5) / 2)) {
        char message[80];
        sprintf(message, "Invalid DLC: %d and %d", (int)CAN_Tx_Msg.header.DLC, (int)((cmd_len - 5) / 2));
        CanHacker_ErrorCallback(canHacker, message);
        return;
    }
    if (CAN_Tx_Msg.header.DLC > 8) {
        CanHacker_ErrorCallback(canHacker, "DLC > 8");
        return;
    }
    for(int i = 0; i < CAN_Tx_Msg.header.DLC; i++){
        CAN_Tx_Msg.data[i] = (ascii2byte(str[dataOffset+(i*2)]) << 4) + ascii2byte(str[dataOffset+((i*2)+1)]);
    }
    CanHacker_CanTxMsgReadyCallback(canHacker, &CAN_Tx_Msg);
}

void CanHacker_ExecTimestamp(CanHacker_HandleTypeDef *canHacker) {
    switch (canHacker->timestamp) {
        case CANHACKER_TIMESTAMP_DISABLED:
            canHacker->timestamp = CANHACKER_TIMESTAMP_ENABLED;
            break;
        case CANHACKER_TIMESTAMP_ENABLED:
            canHacker->timestamp = CANHACKER_TIMESTAMP_DISABLED;
            break;
    }
}

void CanHacker_ExecOpen(CanHacker_HandleTypeDef *canHacker) {
    if (canHacker->timestamp == CANHACKER_TIMESTAMP_ENABLED) {
        CanHacker_StartTimerCallback(canHacker);
    }
}

void CanHacker_ExecClose(CanHacker_HandleTypeDef *canHacker) {
    if (canHacker->timestamp == CANHACKER_TIMESTAMP_ENABLED) {
        CanHacker_StopTimerCallback(canHacker);
    }
}

void CanHacker_ExecSetBitRate(CanHacker_HandleTypeDef *canHacker, uint8_t *str) {
    if(strlen((char *)str) != 2) {
        CanHacker_ErrorCallback(canHacker, "Length of SetBitrate command must be 2");
    }
    if(str[1] != '4'){
        CanHacker_ErrorCallback(canHacker, "Only S4 (125kb/s) is supported");
    }
}

void CanHacker_Receive_Cmd(CanHacker_HandleTypeDef *canHacker, uint8_t *cmd_buf){
    char firstChar = *cmd_buf;
    switch (firstChar) {
        case CANHACKER_GET_SERIAL: {
            CanHacker_ExecGetSerial(canHacker);
            return;
        }
        case CANHACKER_GET_VERSION: {
            CanHacker_ExecGetVersion(canHacker);
            return;
        }
        case CANHACKER_GET_SW_VERSION: {
            CanHacker_ExecGetSWVersion(canHacker);
            return;
        }
        case CANHACKER_SEND_11BIT_ID: {
            CanHacker_ExecTransmit11bit(canHacker, cmd_buf);
            return;
        }
        case CANHACKER_SEND_R11BIT_ID: {
            CanHacker_ExecTransmitR11bit(canHacker, cmd_buf);
            return;
        }
        case CANHACKER_TIME_STAMP: {
            CanHacker_ExecTimestamp(canHacker);
            return;
        }
        case CANHACKER_OPEN_CAN_CHAN: {
            CanHacker_ExecOpen(canHacker);
            return;
        }
        case CANHACKER_CLOSE_CAN_CHAN: {
            CanHacker_ExecClose(canHacker);
            return;
        }
        case CANHACKER_SET_BITRATE: {
            CanHacker_ExecSetBitRate(canHacker, cmd_buf);
            return;
        }
        default:
            CanHacker_ErrorCallback(canHacker, "Unexpected command");
            return;
    }
}

void CanHacker_CanRxMsgToString(CanHacker_HandleTypeDef *canHacker, CanRxMsgTypeDef *msg, uint8_t *str) {
    if (msg->header.RTR == CAN_RTR_DATA){
        str[0] = CANHACKER_SEND_11BIT_ID;
    } else {
        str[0] = CANHACKER_SEND_R11BIT_ID;
    }
    int i = 0;
    uint32_t id = msg->header.StdId;
    for(i = 3; i >= 1; i--){
        str[i] = nibble2ascii(id);
        id >>= 4;
    }
    str[4] = nibble2ascii(msg->header.DLC);
    uint32_t length = 5;
    if(msg->header.RTR == CAN_RTR_DATA) {
        for(i = 0; i < msg->header.DLC; i++){
            uint8_t byte = msg->data[i];
            str[5 + (i * 2)] = nibble2ascii(byte >> 4);
            str[5 + ((i * 2) + 1)] = nibble2ascii(byte);
        }
        length += msg->header.DLC * 2;
    }
    if (canHacker->timestamp) {
        uint32_t timestamp = CanHacker_GetTimestampCallback(canHacker);
        for(i = 3; i >= 0; i--) {
            str[length + i] = nibble2ascii(timestamp);
            timestamp >>=4;
        }
        length += 4;
    }
    str[length] = CANHACKER_CR;
    str[length+1] = '\0';
}

void CanHacker_Receive_CanMsg(CanHacker_HandleTypeDef *canHacker, CanRxMsgTypeDef *msg){
    uint8_t str[CANHACKER_CMD_MAX_LENGTH];
    CanHacker_CanRxMsgToString(canHacker, msg, str);
    CanHacker_UartMsgReadyCallback(canHacker, str);
}
