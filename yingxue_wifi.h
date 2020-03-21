#ifndef YINGXUE_WIFI_H
#define YINGXUE_WIFI_H
#include <stdint.h>
#include <sys/time.h>
#define WIFI 1

//最大缓存数
#define MAX_CACHE_NUM 255
#define MAX_FRAME_NUM 255

//回复命令成功
#define BACK_COMMAND_SUCCESS(COMMADN_ID, STATE) yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, COMMADN_ID, STATE)



//101	开关键
#define WIFI_CTR_OPEN 101

//102	模式
#define WIFI_CTR_MODE 102

//103	设置温度
#define WIFI_CTR_TEMP 103

//104	设置出水量
#define WIFI_CTR_CHUSHUI 104

//105	手动预约模式
#define WIFI_CTR_SHOUDONG 105

//106	回水温差
#define WIFI_CTR_HUISHUI 106

//107	状态
#define WIFI_CTR_STATE 107

//108	预约开关
#define WIFI_CTR_YUYUE_SW 108


//109	预约时间点设置
#define WIFI_CTR_YUTIME 109



#define CLEAN_WIFI_CACHE(x) do{x->idx = 0; x->data_len = 0;}while(0)

//得到验证
#define GET_CHECK_VAL(wifi_cache, check_no)  do{ \
  for (int i = 0; i < wifi_cache->idx - 1; i++){ \
	check_no = (uint8_t)((check_no + wifi_cache->data[i]) & 0xff); \
    }    }while(0)

//发送指令到wifi模块
#define WIFI_SEND_UART(data) do{yingxue_wifi_senduart(data);}while(0);

//定义缓存
struct wifi_cache_tag
{
	uint8_t data[MAX_CACHE_NUM];
	uint8_t idx; //当前缓存的索引
	uint16_t data_len; //数据帧的长度

};

//接受到数据帧
struct wifi_frame_tag
{
	uint8_t data[MAX_FRAME_NUM];//数据域
	uint16_t data_len;//数据长度
	uint8_t command; // 命令

};

enum wifi_command_state
{
	WIFI_CMD_EQUIP_UP = 0x02, //设备信息上报
	WIFI_CMD_NET = 0x08, //配网命令
	WIFI_CMD_STATE_UP = 0x05, //状态上报 
	WIFI_CMD_HEART = 0x03, //心跳
	WIFI_CMD_STATE_QUERY = 0x04, //状态查询
	WIFI_CMD_STATE_CTR = 0x07, //状态控制 
	WIFI_CMD_STATE_ERR = 0x09, //故障上报 
	WIFI_CMD_STATE_OK = 0x00 //接收确认（Ack） 
};

//WiFi串口发送数据
//主线程通过消息队列传送数据
struct wifi_uart_mq_tag{
	uint8_t data[60];   //需要发送的数据
	uint16_t  len;			// 需要发送数据的长度
};

//wifi基础数据结构
struct wifi_base_tag{
	uint8_t run_state; //运行状态 0上报状态 1已经上报完成
	uint8_t ack_state; //ack状态 0未等待状态 1 需要等待ack
	struct timeval last_send_time; //记录最后一次发送的时间 
	uint8_t online_state; //wifi状态..Bit[4~7] 配网状态 0=不处于配网状态 1=处于配网状态，Bit[1~3] 信号强度0=无1=弱2=中3=强，Bit[0] 在线状态 0=离线1=在线
	uint8_t beg_upstate; // 开始上传状态 0未开始 大于0开始，每次减一 101 开机102 模式103 设置温度104 出水量105 手动预约106 回水温差107 状态108 预约开关109 预约时间

};
//发送任务
void yingxue_wifi_send_task();
//wifi模块任务
void yingxue_wifi_task();
//串口发送信息
void yingxue_wifi_senduart(struct wifi_uart_mq_tag *wifi_uart_mq);

void yingxue_wifi_to_wifi(enum wifi_command_state cmd_state, uint16_t state_id, uint32_t data);
#endif