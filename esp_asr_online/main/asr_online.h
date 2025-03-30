#pragma once

//头文件
#include<stdio.h>
#include"esp_err.h"
#include"esp_websocket_client.h"
#include"recorder.h"
#include"cJSON.h"

//ASR初始化
uint8_t asr_init();
//发起语音识别
void asr_call();
//结束语音识别
void asr_finish();
//语音识别忙碌状态
uint8_t asr_ifbusy();
