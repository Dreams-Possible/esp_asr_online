#include"asr_online.h"

//配置
#define ASR_API "wss://www.funasr.com:10096/"

//定义
#define ONCE_LEN 1024*8

//保存静态数据
typedef struct static_t
{
    esp_websocket_client_handle_t client;
    uint8_t state;
    uint8_t busy;
}static_t;
static static_t*static_data=NULL;

//websocket客户端接收
static void websocket_receive(void *handler_args,esp_event_base_t base,int32_t event_id,void*event_data);
//ASR初始化
uint8_t asr_init();
//ASR后台服务
static void service(void*arg);
//发起语音识别
static void start();
//正在语音识别
static void run();
//结束语音识别
static void end();
//发起语音识别
void asr_call();
//结束语音识别
void asr_finish();
//语音识别忙碌状态
uint8_t asr_ifbusy();

//websocket客户端接收
static void websocket_receive(void *handler_args,esp_event_base_t base,int32_t event_id,void*event_data)
{
    esp_websocket_event_data_t*data=(esp_websocket_event_data_t*)event_data;
    switch(event_id)
    {
        case WEBSOCKET_EVENT_BEGIN:
            printf("WEBSOCKET_EVENT_BEGIN\n");
        break;
        case WEBSOCKET_EVENT_CONNECTED:
            printf("WEBSOCKET_EVENT_CONNECTED\n");
        break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            printf("WEBSOCKET_EVENT_DISCONNECTED\n");
        break;
        case WEBSOCKET_EVENT_DATA:
            printf("WEBSOCKET_EVENT_DATA\n");
            //如果有数据
            if(data->data_len)
            {
                cJSON*root=cJSON_Parse(data->data_ptr);
                if(!root)
                {
                    printf("JSON 解析失败\n");
                    return;
                }
                //提取内容
                cJSON*text=cJSON_GetObjectItem(root,"text");
                if(!text)
                {
                    printf("JSON 无内容\n");
                    return;
                }
                //判断是否结束
                cJSON*is_final=cJSON_GetObjectItem(root,"is_final");
                if(!is_final)
                {
                    printf("JSON 无结束标志\n");
                    return;
                }
                if(is_final->valueint)
                {
                    printf("[DATA]已结束->");
                    printf("text: %s\n", text->valuestring);         
                }else
                {
                    printf("[DATA]未结束->");
                    printf("text: %s\n", text->valuestring);
                }
                //释放
                cJSON_Delete(root);
            }
        break;
        case WEBSOCKET_EVENT_ERROR:
            printf("WEBSOCKET_EVENT_ERROR\n");
        break;
        case WEBSOCKET_EVENT_FINISH:
            printf("WEBSOCKET_EVENT_FINISH\n");
        break;
    }
}

//ASR初始化
uint8_t asr_init()
{
    //初始化静态数据
    if(static_data)
    {
        free(static_data);
        static_data=NULL;
    }
    static_data=calloc(1,sizeof(static_t));
    if(!static_data)
    {
        printf("asr: asr_init: out memory\n");
        return 1;
    }
    esp_err_t ret=ESP_OK;
    //创建默认事件循环列表
    ret=esp_event_loop_create_default();
    if(ret==ESP_ERR_INVALID_STATE)
    {
        ret=ESP_OK;
    }
    //初始化websocket客户端
    esp_websocket_client_config_t websocket_cfg={0};
    websocket_cfg.uri=ASR_API;
    websocket_cfg.task_prio=12;
    websocket_cfg.buffer_size=ONCE_LEN;
    static_data->client=esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(static_data->client,WEBSOCKET_EVENT_ANY,websocket_receive,NULL);
    ret=esp_websocket_client_start(static_data->client);
    if(ret!=ESP_OK)
    {
        printf("asr: asr_init: init fail\n");
        return 1;
    }
    while(!esp_websocket_client_is_connected(static_data->client))
    {
        printf("asr: asr_init: retry connect\n");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    printf("asr: asr_init: init ok\n");
    return 0;
}

//ASR后台服务
static void service(void*arg)
{
    //发起语音识别
    start();
    //正在语音识别
    run();
    //结束语音识别
    end();
    static_data->busy=0;
    //删除任务
    vTaskDelete(NULL);
}

//发起语音识别
static void start()
{
    cJSON*root=cJSON_CreateObject();
    //添加字段
    cJSON_AddNumberToObject(root,"chunk_interval",10);
    cJSON *chunk_size=cJSON_CreateIntArray((int[]){5,10,5},3);
    cJSON_AddItemToObject(root,"chunk_size",chunk_size);
    cJSON_AddStringToObject(root,"hotwords","{}");
    cJSON_AddBoolToObject(root,"is_speaking",true);
    cJSON_AddBoolToObject(root,"itn",false);
    cJSON_AddStringToObject(root,"mode","2pass");
    cJSON_AddStringToObject(root,"wav_name","h5");
    //生成字符串
    char*json_str=cJSON_Print(root);
    //发送JSON数据
    esp_websocket_client_send_text(static_data->client,json_str,strlen(json_str),portMAX_DELAY);
    //释放
    cJSON_Delete(root);
    free(json_str);
}

//正在语音识别
static void run()
{
    //开始录音
    uint8_t*data=recorder_start();
    //当前录音最大大小
    uint32_t max_len=0;
    //当前发送位置
    uint32_t now_len=0;
    //等待录音数据
    while(!recorder_get_size())
    {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    //正在语音识别
    while(1)
    {
        //更新录音大小
        max_len=recorder_get_size();
        //一次理想发送大小
        uint32_t once=max_len-now_len;
        if(once>ONCE_LEN)
        {
            once=ONCE_LEN;
        }
        printf("[asr_online]once=%ld\n",once);
        //发送
        if(data&&esp_websocket_client_is_connected(static_data->client))
        {
            esp_websocket_client_send_bin(static_data->client,(char*)&data[now_len],once,portMAX_DELAY);
        }else
        {
            printf("[asr_online]rec fail\n");
        }
        //更新发送位置
        now_len+=once;
        //是否停止语音识别
        if(!static_data->state)
        {
            //结束录音
            recorder_end();
        }
        //语音识别完成
        if(!static_data->state&&now_len>=max_len)
        {
            break;
        }
    }
}

//结束语音识别
static void end()
{
    cJSON*root=cJSON_CreateObject();
    //添加字段
    cJSON_AddNumberToObject(root,"chunk_interval",10);
    cJSON*chunk_size=cJSON_CreateIntArray((int[]){5,10,5},3);
    cJSON_AddItemToObject(root,"chunk_size",chunk_size);
    cJSON_AddBoolToObject(root,"is_speaking",false);
    cJSON_AddStringToObject(root,"mode","2pass");
    cJSON_AddStringToObject(root,"wav_name","h5");
    //生成字符串
    char*json_str=cJSON_Print(root);
    // printf("%s\n",json_str);
    //发送JSON数据
    esp_websocket_client_send_text(static_data->client,json_str,strlen(json_str),portMAX_DELAY);
    //释放
    free(json_str);
    cJSON_Delete(root);
}

//发起语音识别
void asr_call()
{
    static_data->state=1;
    static_data->busy=1;
    //创建任务
    xTaskCreate(service,"asr_online: service",8092,NULL,12,NULL);
}

//结束语音识别
void asr_finish()
{
    static_data->state=0;
}

//语音识别忙碌状态
uint8_t asr_ifbusy()
{
    return static_data->busy;
}
