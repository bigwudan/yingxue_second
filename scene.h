/** @file
 * ITE Display Control Board Scene Definition.
 *
 * @author Jim Tan
 * @version 1.0
 * @date 2015
 * @copyright ITE Tech. Inc. All Rights Reserved.
 */
/** @defgroup ctrlboard ITE Display Control Board Modules
 *  @{
 */
#ifndef SCENE_H
#define SCENE_H

#include "ite/itu.h"
#include "ctrlboard.h"
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @defgroup ctrlboard_scene Scene
 *  @{
 */

#define MS_PER_FRAME                17              ///< Drawing delay per frame

typedef enum
{
	EVENT_CUSTOM_SCREENSAVER = ITU_EVENT_CUSTOM,    ///< Ready to enter screensaver mode. Custom0 event on GUI Designer.
    EVENT_CUSTOM_SD_INSERTED,                       ///< #1: SD card inserted.
    EVENT_CUSTOM_SD_REMOVED,                        ///< #2: SD card removed.
	EVENT_CUSTOM_USB_INSERTED,                      ///< #3: USB drive inserted.
    EVENT_CUSTOM_USB_REMOVED,                       ///< #4: USB drive removed.
    EVENT_CUSTOM_KEY0,                              ///< #5: Key #0 pressed.
    EVENT_CUSTOM_KEY1,                              ///< #6: Key #1 pressed.
	EVENT_CUSTOM_KEY2,                              ///< #7: Key #2 pressed.
    EVENT_CUSTOM_KEY3,                              ///< #8: Key #3 pressed.
    EVENT_CUSTOM_UART                               ///< #9: UART message.

} CustomEvent;

// scene
/**
 * Initializes scene module.
 */
void SceneInit(void);

/**
 * Exits scene module.
 */
void SceneExit(void);

/**
 * Loads ITU file.
 */
void SceneLoad(void);

/**
 * Runs the main loop to receive events, update and draw scene.
 *
 * @return The QuitValue.
 */
int SceneRun(void);

/**
 * Gotos main menu layer.
 */
void SceneGotoMainMenu(void);

/**
 * Sets the status of scene.
 *
 * @param ready true for ready, false for not ready yet.
 */
void SceneSetReady(bool ready);

/**
 * Quits the scene.
 *
 * @param value The reason to quit the scene.
 */
void SceneQuit(QuitValue value);

/**
 * Gets the current quit value.
 *
 * @return The current quit value.
 */
QuitValue SceneGetQuitValue(void);

void SceneEnterVideoState(int timePerFrm);
void SceneLeaveVideoState(void);

/**
 * Changes language file.
 */
void SceneChangeLanguage(void);

/**
 * Predraw scene.
 *
 * @param arg Unused.
 */
void ScenePredraw(int arg);

/**
 * Global instance variable of scene.
 */
extern ITUScene theScene;

/** @} */ // end of ctrlboard_scene

//樱雪
//按键回调函数 
typedef void(*node_widget_cb)(struct node_widget *widget, unsigned char state);

//发送指令的状态
enum main_pthread_mq_state
{
	OPEN_CMD =1, //开机
	CLOSE_CMD =2, //关机
	RUN_YURE=3, //开预热
	STOP_YURE=4, //关预热
	SET_TEMP=5, //设置温度
	SET_CHUCHANG //出厂设置
};

//控件结构体
//控制控件
struct node_widget
{
	unsigned char value;
	struct node_widget_tag *up; //上一个控件
	struct node_widget_tag *down; //下一个控件
	char *name; //控件名称
	char *focus_back_name; //选中控件背景,就是选中边框
	char *checked_back_name; //确定控件背景
	uint8_t state; //状态0焦点 1锁定
	uint8_t type; //类型 0 普通 1可以锁定 2长按
	node_widget_cb updown_cb; //点击向上回调
	node_widget_cb confirm_cb; //确认回调
	node_widget_cb long_press_cb; //长按
};

//模式
struct moshi_data{
	unsigned char temp;
};

//樱雪基础数据
struct yingxue_base_tag{
	unsigned char adjust_temp_state; // 0调整温度状态 1正在调整 2闪烁 3解除锁定，可以上下移动
	unsigned char shezhi_temp; //设置温度
	unsigned char run_state; //0第一次上电 1开机 2关机
	unsigned char state_show;//第0位 有水  第1位 风机 第2位 火焰 第3位 风压
	unsigned char shizhe_temp;//设置温度
	unsigned char chushui_temp;//出水温度
	unsigned char jinshui_temp;//进水温度
	unsigned char huishui_temp; //回水温度 ,预热设置回差

	unsigned char moshi_mode; // 模式 0无模式 1 普通模式 2 超级模式 3 节能模式 4 水果模式

	struct moshi_data normal_moshi; //普通模式
	struct moshi_data super_moshi; //超级模式
	struct moshi_data eco_moshi; //节能模式
	struct moshi_data fruit_moshi; //水果模式
	unsigned char select_set_moshi_mode;// 选择设置模式 0无模式 1 普通模式 2 超级模式 3 节能模式 4 水果模式

	unsigned char yure_mode; //预热模式 0无模式 1单巡航模式 2 全天候模式 3 预约模式
	struct timeval yure_begtime; //预热开始时间
	struct timeval yure_endtime; //预热结束时间
	unsigned char dingshi_list[24]; // //定时时间数组,0未开启 1开启
	unsigned char err_no;//错误代码
	unsigned char is_err;//是否故障
	//是否已经预热
	unsigned char yure_state; //0 未预热 1已经预热
	unsigned char wind_rate;//[0][10]风机转速
	unsigned char fa_num;//[2][0] 当前气源号
	unsigned char dh_num;//[2][3]
	unsigned char ph_num;//[2][1]
	unsigned char ne_num;//[2][4]
	unsigned char huishui_temp_1;//[3][1] 回水温度 显示的回水温度
};


//主线程通过消息队列传送数据
struct main_pthread_mq_tag{
	unsigned char s_data[11];
	unsigned char state;			// 0 控制板数据 1开机命令 2关机命令
};

//子线程通过消息队列传送数据到主线程  
/*
PA Fa [2][0]    DH    [2][3]
PH    [2][1]    FY  nf [2][9] (空)
PL [2][2]       FD 空
pd 空            hs 回水温度
HI  NE [2][4]
*/
struct child_to_pthread_mq_tag{
	//第0位 有水  第1位 风机 第2位 火焰 第3位 风压
	unsigned char state_show;
	//设置温度
	unsigned char shezhi_temp;
	//出水温度
	unsigned char chushui_temp;
	//进水温度
	unsigned char jinshui_temp;
	//错误代码
	unsigned char err_no;
	//主机状态 0关机 1待机 2正常燃烧 3
	unsigned char machine_state;
	//是否故障
	unsigned char is_err;
	//[0][10]风机转速
	unsigned char wind_rate;
	//[2][0] 当前气源号
	unsigned char fa_num;
	//[2][3]
	unsigned char dh_num;
	//[2][1]
	unsigned char ph_num;
	//[2][4]
	unsigned char ne_num;
	//[3][1] 回水温度
	unsigned char huishui_temp;
};

//串口的数据
struct uart_data_tag{
	unsigned char count;//当前数据数
	unsigned char buf_data[17]; //当前缓存
	unsigned char state; //状态  0正常 1错误 2已经完成
};




//串口
#define UART_PORT       ITP_DEVICE_UART3
#define UART_DEVICE     itpDeviceUart3	
#define MAX_CHAIN_NUM 50

//开机
#define SEND_OPEN_CMD() do{sendCmdToCtr(0x03, 0x01, 0x00, 0x00, 0x00, OPEN_CMD);}while(0)
//关机
#define SEND_CLOSE_CMD() do{sendCmdToCtr(0x03, 0x00, 0x00, 0x00, 0x00, CLOSE_CMD);}while(0)

//开始预热 预热	命令9	(0 - 预热关 2 - 循环预热)	 随机	预热回差设置	 随机
#define SEND_OPEN_YURE_CMD() do{sendCmdToCtr(0x09, 0x02, 0x00, yingxue_base.huishui_temp, 0x00, RUN_YURE);}while(0)
//结束预热
#define SEND_CLOSE_YURE_CMD() do{sendCmdToCtr(0x09, 0x00, 0x00, yingxue_base.huishui_temp, 0x00, STOP_YURE);}while(0)



//环形队列
struct chain_list_tag{
	unsigned char rear; //尾结点
	unsigned char front; //头结点
	unsigned char count; //当前数量
	unsigned char buf[MAX_CHAIN_NUM];
};

//获取当前时间
int get_rtc_time(struct  timeval *dst, unsigned char *zone);

//设置当前时间
void set_rtc_time(unsigned char hour, unsigned char min);

//计算下次的预约时间
void calcNextYure(int *beg, int *end);

//发送命令
void sendCmdToCtr(unsigned char cmd, unsigned char data_1, unsigned char data_2, unsigned char data_3, unsigned char data_4, enum main_pthread_mq_state state);

//组合数据
void processCmdToCtrData(unsigned char cmd, unsigned char data_1,
	unsigned char data_2, unsigned char data_3, unsigned char data_4, unsigned char *dst);

#define LOG_WRITE_UART(arr) do{\
								for(int i=0; i<11;i++) \
									printf("%02X ", arr[i]);\
																}while(0)
#define LOG_RECE_UART(arr) do{for(int i=0; i < 17; i++) printf("%02X ", arr[i]); }while(0)



//打印子线程发送的消息队列
#define LOG_CHILD_MQ(arr)  do{\
								printf("state_show=%02X, shezhi_temp=%02X, chushui_temp=%02X, jinshui_temp=%02X, err_no=%02X, machine_state=%02X, is_err=%02X, wind_rate=%02X, fa_num=%02X, dh_num=%02X, ph_num=%02X, ne_num=%02X, huishui_temp=%02X\n",\
										arr.state_show, arr.shezhi_temp, arr.chushui_temp, arr.jinshui_temp, arr.err_no, arr.machine_state,\
									    arr.is_err,arr.wind_rate,arr.fa_num,arr.dh_num,arr.ph_num,arr.ne_num,arr.huishui_temp);}while(0);

//获取长按的时间
#define LONG_PRESS_TIME(a, b, c) 	do{get_rtc_time(&a, NULL);c = a.tv_sec - b.tv_sec;}while(0)					
#ifdef __cplusplus
}
#endif

#endif /* SCENE_H */
/** @} */ // end of ctrlboard