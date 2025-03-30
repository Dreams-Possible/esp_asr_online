#pragma once

//头文件
#include<stdio.h>
#include"esp_err.h"
#include"driver/i2s_std.h"
#include"freertos/FreeRTOS.h"

//初始化录音机
uint8_t recorder_init();
//开始录音
uint8_t*recorder_start();
//结束录音
void recorder_end();
//获取录音大小
uint32_t recorder_get_size();
//获取录音状态
uint8_t recorder_get_state();
