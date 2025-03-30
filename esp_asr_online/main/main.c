#include"main.h"

#include "esp_timer.h"

void app_main(void)
{
    //WiFi初始化
    while(wifi_init())
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    //等待WiFi连接
    while(wifi_state())
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
        printf("main: no wifi\n");
    }

    //初始化录音机
    while(recorder_init())
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    //初始化ASR
    while(asr_init())
    {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    while(1)
    {
        asr_call();
        vTaskDelay(pdMS_TO_TICKS(5000));
        asr_finish();
        printf("[MAIN wait]\n");
        vTaskDelay(pdMS_TO_TICKS(20000));
        while(asr_ifbusy())
        {
            printf("[MAIN asr is busy]\n");
        }
        printf("[MAIN restart]\n");
    }
}
