#include"recorder.h"

//配置
#define IIS_CLK 2
#define IIS_SDA 5
#define IIS_WS 1
#define SAMPLE_RATE 16000
#define SAMPLE_BIT I2S_DATA_BIT_WIDTH_16BIT
#define SAMPLE_MODE I2S_SLOT_MODE_MONO
#define MAX_TIME 8000

//定义
#define BYTE_RATE SAMPLE_RATE*(SAMPLE_BIT/8)*SAMPLE_MODE
#define MAX_LEN BYTE_RATE*MAX_TIME/1000

//保存静态数据
typedef struct static_t
{
    i2s_chan_handle_t chan;
    uint8_t*data;
    uint8_t state;
    uint32_t len;
}static_t;
static static_t*static_data=NULL;

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

//初始化录音机
uint8_t recorder_init()
{
    if(static_data)
    {
        free(static_data);
    }
    static_data=calloc(1,sizeof(static_t));
    if(!static_data)
    {
        printf("recorder: recorder_init: mem fail\n");
        return 1;
    }
    static_data->data=NULL;
    static_data->chan=NULL;
    esp_err_t ret=ESP_OK;
    i2s_chan_config_t ch_cfg=I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO,I2S_ROLE_MASTER);
    ret=i2s_new_channel(&ch_cfg,NULL,&static_data->chan);
    if(ret!=ESP_OK)
    {
        printf("recorder: recorder_init: init fail\n");
        return 1;
    }
    i2s_std_config_t iis_cfg={0};
    iis_cfg.clk_cfg=(i2s_std_clk_config_t)I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE);
    iis_cfg.slot_cfg=(i2s_std_slot_config_t)I2S_STD_MSB_SLOT_DEFAULT_CONFIG(SAMPLE_BIT,SAMPLE_MODE);
    iis_cfg.gpio_cfg.mclk=I2S_GPIO_UNUSED;
    iis_cfg.gpio_cfg.bclk=IIS_CLK;
    iis_cfg.gpio_cfg.ws=IIS_WS;
    iis_cfg.gpio_cfg.dout=I2S_GPIO_UNUSED;
    iis_cfg.gpio_cfg.din=IIS_SDA;
    iis_cfg.slot_cfg.slot_mask=I2S_STD_SLOT_LEFT;
    ret=i2s_channel_init_std_mode(static_data->chan,&iis_cfg);
    if(ret!=ESP_OK)
    {
        printf("recorder: recorder_init: init fail\n");
        return 1;
    }
    ret=i2s_channel_enable(static_data->chan);
    if(ret!=ESP_OK)
    {
        printf("recorder: recorder_init: init fail\n");
        return 1;
    }
    printf("recorder: recorder_init: init ok\n");
    return 0;
}


//录音服务
static void service(void*arg)
{
    //录音
    while(1)
    {
        //一次实际读取大小
        uint32_t once_len=0;
        //一次理想读取大小
        uint32_t once=MAX_LEN-static_data->len;
        //每100ms更新一次
        if(once>BYTE_RATE/10)
        {
            once=BYTE_RATE/10;
        }
        //读取
        esp_err_t ret=i2s_channel_read(static_data->chan,(char*)&static_data->data[static_data->len],once,(size_t*)&once_len,portMAX_DELAY);
        if(ret==ESP_OK)
        {
            static_data->len+=once_len;
        }else
        {
            printf("recorder: recorder_time: record fail\n");

            break;
        }
        //超出缓冲区自动结束
        if(static_data->len>=MAX_LEN)
        {
            break;
        }
        //如果手动结束
        if(!static_data->state)
        {
            break;
        }
    }
    //录音完成
    static_data->state=0;
    //删除任务
    vTaskDelete(NULL);
}

//开始录音
uint8_t*recorder_start()
{
    //重置音频数据
    static_data->len=0;
    if(static_data->data)
    {
        free(static_data->data);
        static_data->data=NULL;
    }
    static_data->data=calloc(MAX_LEN,sizeof(uint8_t));
    if(!static_data->data)
    {
        printf("recorder: recorder_time: mem out\n");
        return NULL;
    }
    //开始录音
    static_data->state=1;
    xTaskCreate(service,"recorder: service",8092,NULL,12,NULL);
    return static_data->data;
}

//结束录音
void recorder_end()
{
    //结束录音
    static_data->state=0;
}

//获取录音大小
uint32_t recorder_get_size()
{
    return static_data->len;
}

//获取录音状态
uint8_t recorder_get_state()
{
    return static_data->state;
}
