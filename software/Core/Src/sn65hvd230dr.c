//
// Created by marcin on 10.04.2021.
//

#include "sn65hvd230dr.h"
#include "main.h"

void sn65Hvd230DrNormalMode(void){
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_RESET);
}
void sn65Hvd230DrStandbyMode(void){
    HAL_GPIO_WritePin(STBY_GPIO_Port, STBY_Pin, GPIO_PIN_SET);
}