#include <sys/ioctl.h>
#include <sys/time.h>
#include <assert.h>
#include <math.h>
#include <mqueue.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "SDL/SDL.h"
#include "ite/itp.h"
#include "scene.h"
#include "ctrlboard.h"

#ifdef _WIN32
    #include <crtdbg.h>

    #ifndef CFG_VIDEO_ENABLE
        #define DISABLE_SWITCH_VIDEO_STATE
    #endif
#endif // _WIN32

#ifndef CFG_POWER_WAKEUP_DOUBLE_CLICK_INTERVAL
    #define DOUBLE_KEY_INTERVAL 200
#endif

#define FPS_ENABLE
#define DOUBLE_KEY_ENABLE

#define GESTURE_THRESHOLD           40
#define MAX_COMMAND_QUEUE_SIZE      8
#define MOUSEDOWN_LONGPRESS_DELAY   1000

extern ITUActionFunction actionFunctions[];
extern void resetScene(void);

// status
static QuitValue    quitValue;
static bool         inVideoState;

// command
typedef enum
{
    CMD_NONE,
    CMD_LOAD_SCENE,
    CMD_CALL_CONNECTED,
    CMD_GOTO_MAINMENU,
    CMD_CHANGE_LANG,
    CMD_PREDRAW
} CommandID;

#define MAX_STRARG_LEN 32

typedef struct
{
    CommandID   id;
    int         arg1;
    int         arg2;
    char        strarg1[MAX_STRARG_LEN];
} Command;

static mqd_t        commandQueue = -1;

// scene
ITUScene            theScene;
static SDL_Window   *window;
static ITUSurface   *screenSurf;
static int          screenWidth;
static int          screenHeight;
static float        screenDistance;
static bool         isReady;
static int          periodPerFrame;

#if defined(CFG_USB_MOUSE) || defined(_WIN32)
static ITUIcon      *cursorIcon;
#endif

extern void ScreenSetDoubleClick(void);
//樱雪crc效验数组
static const unsigned short crc16tab[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
	0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
	0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
	0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
	0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
	0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
	0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
	0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
	0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
	0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
	0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
	0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
	0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
	0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
	0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
	0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
	0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
	0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
	0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
	0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
	0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
	0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
	0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
	0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
	0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
	0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
	0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
	0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
	0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
	0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
	0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
	0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0
};
//串口消息
mqd_t uartQueue = -1;

//子线程向主线程发送
mqd_t childQueue = -1;


//是否已经处理超时 0 未处理 1 已经处理
unsigned char is_deal_over_time;

//樱雪全局数据
struct yingxue_base_tag yingxue_base;

//当前选中的空间
struct node_widget *curr_node_widget;


//主界面
struct node_widget mainlayer_0;
struct node_widget mainlayer_1;
struct node_widget mainlayer_2;

//预热界面
struct node_widget yureLayer_0;
struct node_widget yureLayer_1;
struct node_widget yureLayer_2;
struct node_widget yureLayer_3;
struct node_widget yureLayer_4;
struct node_widget yureLayer_5;



//预热时间页面
struct node_widget yureshijian_widget_0; //预热时间控制控件1
struct node_widget yureshijian_widget_num_0; //预热时间控制控件2
struct node_widget yureshijian_widget_num_1; //预热时间控制控件3
struct node_widget yureshijian_widget_num_2; //预热时间控制控件4
struct node_widget yureshijian_widget_num_3; //预热时间控制控件5
struct node_widget yureshijian_widget_num_4; //预热时间控制控件6
struct node_widget yureshijian_widget_num_5; //预热时间控制控件2
struct node_widget yureshijian_widget_num_6; //预热时间控制控件3
struct node_widget yureshijian_widget_num_7; //预热时间控制控件4
struct node_widget yureshijian_widget_num_8; //预热时间控制控件5
struct node_widget yureshijian_widget_num_9; //预热时间控制控件6
struct node_widget yureshijian_widget_num_10; //预热时间控制控件2
struct node_widget yureshijian_widget_num_11; //预热时间控制控件3
struct node_widget yureshijian_widget_num_12; //预热时间控制控件4
struct node_widget yureshijian_widget_num_13; //预热时间控制控件5
struct node_widget yureshijian_widget_num_14; //预热时间控制控件6
struct node_widget yureshijian_widget_num_15; //预热时间控制控件2
struct node_widget yureshijian_widget_num_16; //预热时间控制控件3
struct node_widget yureshijian_widget_num_17; //预热时间控制控件4
struct node_widget yureshijian_widget_num_18; //预热时间控制控件5
struct node_widget yureshijian_widget_num_19; //预热时间控制控件6
struct node_widget yureshijian_widget_num_20; //预热时间控制控件6
struct node_widget yureshijian_widget_num_21; //预热时间控制控件6
struct node_widget yureshijian_widget_num_22; //预热时间控制控件6
struct node_widget yureshijian_widget_num_23; //预热时间控制控件6

//预约时间
struct node_widget yureshezhiLayer_0;
struct node_widget yureshezhiLayer_1;
struct node_widget yureshezhiLayer_2;
struct node_widget yureshezhiLayer_3;

//模式
struct node_widget moshiLayer_0;
struct node_widget moshiLayer_1;
struct node_widget moshiLayer_2;
struct node_widget moshiLayer_3;
struct node_widget moshiLayer_4;

//出水模式
struct node_widget chushui_0;
struct node_widget chushui_1;
struct node_widget chushui_2;

//出厂设置
struct node_widget layer1_0; //返回按键
struct node_widget layer1_1; //水流
struct node_widget layer1_2; //火焰
struct node_widget layer1_3; //风机
struct node_widget layer1_4; //出水温度
struct node_widget layer1_5; //风速
struct node_widget layer1_6; //pa
struct node_widget layer1_7; //dh
struct node_widget layer1_8; //ph
struct node_widget layer1_9;//fy
struct node_widget layer1_10;//pl
struct node_widget layer1_11;//fd
struct node_widget layer1_12;//pd
struct node_widget layer1_13;//hs
struct node_widget layer1_14;//hi
struct node_widget layer1_15;//ed


//最后一次活动的时间
struct timeval last_down_time;

//环形队列缓存
struct chain_list_tag chain_list;

/**
*初始化环形列
*@param p_chain_list 环形队列指针
*@return
*/
int create_chain_list(struct chain_list_tag *p_chain_list)
{
	//尾
	p_chain_list->rear = 0;
	//头
	p_chain_list->front = 0;
	//个数
	p_chain_list->count = 0;
	memset(p_chain_list->buf, 0, sizeof(p_chain_list->buf));
	return 0;
}
/**
*进入环形队列
*@param p_chain_list 环形队列指针
*@param src 入队数据
*@return 0满 1成功
*/
int in_chain_list(struct chain_list_tag *p_chain_list, unsigned char src)
{
	int position = (p_chain_list->rear + 1) % MAX_CHAIN_NUM;
	//已经满
	if (position == p_chain_list->front){
		return 0;
	}
	*(p_chain_list->buf + p_chain_list->rear) = src;
	p_chain_list->rear = position;
	return 1;
}
/**
*出环形队列
*@param p_chain_list 环形队列指针
*@param src 出队数据
*@return 0空 1成功
*/
int out_chain_list(struct chain_list_tag *p_chain_list, unsigned char *src)
{
	//空
	if (p_chain_list->rear == p_chain_list->front){
		return 0;
	}
	*src = *(p_chain_list->buf + p_chain_list->front);
	p_chain_list->front = (p_chain_list->front + 1) % MAX_CHAIN_NUM;
	return 1;
}
/**
*CRC效验
*@param buf 目标数据
*@param len 数据长度
*@return 返回CRC数据
*/
unsigned short crc16_ccitt(const char *buf, int len)
{
	register int counter;
	register unsigned short crc = 0;
	for (counter = 0; counter < len; counter++)
		crc = (crc << 8) ^ crc16tab[((crc >> 8) ^ *(char *)buf++) & 0x00FF];
	return crc;
}



//计算下一次预约的时间
void calcNextYure(int *beg, int *end)
{
	//计算时间
	struct timeval curr_time;
	struct tm *t_tm;
	unsigned char cur_hour;
	unsigned char num;
	*beg = -1;
	*end = -1;
	get_rtc_time(&curr_time, NULL);
	t_tm = localtime(&curr_time);
	cur_hour = t_tm->tm_hour;
	//开始是否找到
	unsigned char is_beg = 0;
	//是否开始计数 0 未开始计数 1 开始计数
	unsigned char is_count = 0;

	//如果当前时间已经开始加热
	if (*(yingxue_base.dingshi_list + cur_hour) == 1){
		is_count = 0;
	}
	//未开始加热，开始计数
	else{
		is_count = 1;
	}
	//先向前。如何没有找到从新开始找
	for (int i = 0; i < 2; i++){
		//连续预热，中间已经中断 0连续 1已经不连续
		int continue_flag = 0;
		//向前查找,起始值应该等于当前时间
		if (i == 0){
			num = cur_hour;
		}
		else{
			num = 0;
		}
		for (int j = num; j < 24; j++)
		{

			//未开始计数，找到下一个起始
			if (is_count == 0){
				//连续中,如果有中断，置位
				if ((continue_flag == 0) && (*(yingxue_base.dingshi_list + j) == 0)){
					continue_flag = 1;
				}
				//不连续中断中，找到一个开始预热
				else if ((continue_flag == 1) && (*(yingxue_base.dingshi_list + j) == 1)){
					is_count = 1;
				}
			}
			//如果已经计时
			if (is_count == 1){
				//存在时间
				if (*(yingxue_base.dingshi_list + j) == 1){
					//开始计算
					if (is_beg == 0){
						*beg = j;
						*end = j;
						is_beg = 1;
					}
					//结束连续时间一直计算
					else if (is_beg == 1){
						*end = j;
					}
				}
				else{
					//如果有断层立即结束
					if (*beg != -1) break;
				}
			}
		}
		if (*beg != -1) break;
	}
	//如果下次预热时间，开始时间等于当前时间，去掉不显示
	if (*beg == cur_hour){
		*beg = -1;
		*end = -1;
	}
	return;
}

//设置当前时间
void set_rtc_time(unsigned char hour, unsigned char min)
{
	struct timeval tv;
	struct tm *tm, mytime;
	//设置时间
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	memcpy(&mytime, tm, sizeof(struct tm));
	if (hour != -1){
		mytime.tm_hour = hour; //小时
	}
	if (min != -1){
		mytime.tm_min = min; //分
	}
	tv.tv_sec = mktime(&mytime);
	tv.tv_usec = 0;
	settimeofday(&tv, NULL);
	return;
}


//得到当前时间戳
int get_rtc_time(struct  timeval *dst, unsigned char *zone)
{
	struct timeval tv;
	struct tm *tm;
	struct tm mytime;
	gettimeofday(&tv, NULL);
	tm = localtime(&tv.tv_sec);
	memcpy(&mytime, tm, sizeof(struct tm));
	dst->tv_sec = mktime((struct tm*)&mytime);
	dst->tv_usec = 0;
	return 1;
}

//按键时间发生时的触发事件
static void key_down_process()
{
	//最后一次的时间
	get_rtc_time(&last_down_time, NULL);
	//更新处理标识
	is_deal_over_time = 0;
}

//锁定上下移动
static void lock_widget_up_down(struct node_widget *widget, unsigned char state)
{
	enum style_set { HUISHUI_TEMP, BEIJING_SHIJIAN_H, BEIJING_SHIJIAN_M,CHUSHUI_TEMP };
	struct ITUWidget *t_widget = NULL;
	char t_buf[20] = { 0 };
	int t_num = 0;
	enum style_set style;
	//回水温度
	if (strcmp(widget->name, "Background2") == 0){
		style = HUISHUI_TEMP;
		t_widget = ituSceneFindWidget(&theScene, "Text3");
		t_num = atoi(ituTextGetString((ITUText*)t_widget));
	}
	//设置北京时间小时
	else if (strcmp(widget->name, "Background3") == 0){
		style = BEIJING_SHIJIAN_H;
		t_widget = ituSceneFindWidget(&theScene, "Text42");
		t_num = atoi(ituTextGetString((ITUText*)t_widget));
	}
	//设置北京时间分
	else if (strcmp(widget->name, "Background4") == 0){
		style = BEIJING_SHIJIAN_M;
		t_widget = ituSceneFindWidget(&theScene, "Text43");
		t_num = atoi(ituTextGetString((ITUText*)t_widget));
	}
	//设置出水温度
	else if (strcmp(widget->name, "chushui_Background13") == 0){
		style = CHUSHUI_TEMP;
		t_widget = ituSceneFindWidget(&theScene, "Text38");
		t_num = atoi(ituTextGetString((ITUText*)t_widget));
	}
	if (state == 0){
		t_num = t_num + 1;
	}
	else{
		t_num = t_num - 1;
	}

	//判断范围
	//回水温差范围
	if (style == HUISHUI_TEMP && (t_num < 0 || t_num >= 100)){
		return;
	}
	//设置北京时间
	else if (style == BEIJING_SHIJIAN_H || style == BEIJING_SHIJIAN_M){
		//小时0-23
		if (style == BEIJING_SHIJIAN_H && (t_num < 0 || t_num >= 24)){
			return;
		}
	}
	else if (style == CHUSHUI_TEMP){
		//35-50
		if (yingxue_base.select_set_moshi_mode == 1 && (t_num < 35 || t_num > 50)){
			return;
		}
		else if (yingxue_base.select_set_moshi_mode == 2 && (t_num < 35 || t_num > 65)){
			return;
		}
		else if (yingxue_base.select_set_moshi_mode == 3 && (t_num < 0 || t_num > 42)){
			return;
		}
		else if (yingxue_base.select_set_moshi_mode == 4 && (t_num < 0 || t_num > 38)){
			return;
		}


	
	}

	sprintf(t_buf, "%d", t_num);
	ituTextSetString(t_widget, t_buf);
}

//通用上下移动
static void command_widget_up_down(struct node_widget *t_node_widget)
{
	struct ITUWidget *t_widget = NULL;
	//如果之前的控件是需要整个变换背景，
	if ((strcmp(curr_node_widget->name, "BackgroundButton47") == 0) ||
		(strcmp(curr_node_widget->name, "BackgroundButton65") == 0) ||
		(strcmp(curr_node_widget->name, "BackgroundButton60") == 0) ||
		(strcmp(curr_node_widget->name, "BackgroundButton68") == 0) ||
		(strcmp(curr_node_widget->name, "moshi_BackgroundButton10") == 0) ||
		(strcmp(curr_node_widget->name, "moshi_BackgroundButton11") == 0) ||
		(strcmp(curr_node_widget->name, "moshi_BackgroundButton12") == 0) ||
		(strcmp(curr_node_widget->name, "moshi_BackgroundButton13") == 0) ||
		(strcmp(curr_node_widget->name, "chushui_BackgroundButton73") == 0) ||
		(strcmp(curr_node_widget->name, "chushui_BackgroundButton1") == 0)
		){
		t_widget = ituSceneFindWidget(&theScene, curr_node_widget->focus_back_name);
		ituWidgetSetVisible(t_widget, false);
		t_widget = ituSceneFindWidget(&theScene, curr_node_widget->name);
		ituWidgetSetVisible(t_widget, true);
	}

	//如果现在的控件是需要变换背景
	if ((strcmp(t_node_widget->name, "BackgroundButton47") == 0) ||
		(strcmp(t_node_widget->name, "BackgroundButton65") == 0) ||
		(strcmp(t_node_widget->name, "BackgroundButton60") == 0) ||
		(strcmp(t_node_widget->name, "BackgroundButton68") == 0) ||
		(strcmp(t_node_widget->name, "moshi_BackgroundButton10") == 0) ||
		(strcmp(t_node_widget->name, "moshi_BackgroundButton11") == 0) ||
		(strcmp(t_node_widget->name, "moshi_BackgroundButton12") == 0) ||
		(strcmp(t_node_widget->name, "moshi_BackgroundButton13") == 0) ||
		(strcmp(t_node_widget->name, "chushui_BackgroundButton73") == 0) ||
		(strcmp(t_node_widget->name, "chushui_BackgroundButton1") == 0)
		){
		t_widget = ituSceneFindWidget(&theScene, t_node_widget->focus_back_name);
		ituWidgetSetVisible(t_widget, true);
		t_widget = ituSceneFindWidget(&theScene, t_node_widget->name);
		ituWidgetSetVisible(t_widget, false);

		//原来的控件去掉边框
		//原来控件是radio
		if (strcmp(curr_node_widget->focus_back_name, "radio") == 0){
			t_widget = ituSceneFindWidget(&theScene, curr_node_widget->name);
			ituWidgetSetActive(t_widget, false);
		}
		//控件普通
		else{
			t_widget = ituSceneFindWidget(&theScene, curr_node_widget->focus_back_name);
			ituWidgetSetVisible(t_widget, false);
		}
		curr_node_widget = t_node_widget;
	}
	else if (strcmp(t_node_widget->focus_back_name, "radio") == 0){
		t_widget = ituSceneFindWidget(&theScene, t_node_widget->name);
		ituFocusWidget(t_widget);
		curr_node_widget = t_node_widget;
	}
	else{
		//隐藏当前控件选中状态
		t_widget = ituSceneFindWidget(&theScene, curr_node_widget->focus_back_name);
		ituWidgetSetVisible(t_widget, false);
		//显示当前的控件
		t_widget = ituSceneFindWidget(&theScene, t_node_widget->focus_back_name);
		ituWidgetSetVisible(t_widget, true);
		curr_node_widget = t_node_widget;
	}

}
//主页面确定回调事件
//@param widget 点击控件
//@param state 参数
static void main_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{

	//只有在0状态，第一点击，上下调整
	if (yingxue_base.adjust_temp_state == 0){
		yingxue_base.adjust_temp_state = 3;
	}
	//只有在状态3才可以进入
	else if (yingxue_base.adjust_temp_state == 3){
		ITUWidget *t_widget = NULL;
		if (strcmp(widget->name, "BackgroundButton47") == 0){
			t_widget = ituSceneFindWidget(&theScene, "MainLayer");
			ituLayerGoto((ITULayer *)t_widget);
		}
		else if (strcmp(widget->name, "yureSprite") == 0){
			t_widget = ituSceneFindWidget(&theScene, "yureLayer");
			ituLayerGoto((ITULayer *)t_widget);
		}
		else if (strcmp(widget->name, "moshiSprite") == 0){
			t_widget = ituSceneFindWidget(&theScene, "moshiLayer");
			ituLayerGoto((ITULayer *)t_widget);
		}
	}
}

static void main_widget_up_down_cb(struct node_widget *widget, unsigned char state)
{
	//记录上传的数据
	static int count_idx;
	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;
	char t_buf[20] = { 0 };
	//只有状态3才可以上下调整
	if (yingxue_base.adjust_temp_state == 3){
		if (state == 0){
			if (widget->up)
				t_node_widget = widget->up;
		}
		else{
			if (widget->down){
				t_node_widget = widget->down;
			}
		}
		if (t_node_widget){
			command_widget_up_down(t_node_widget);
		}
	}
	else{
		//可以上下调整温度
		if (yingxue_base.adjust_temp_state == 0 || yingxue_base.adjust_temp_state == 1){
			//如果才开始调整
			if (yingxue_base.adjust_temp_state == 0){
				yingxue_base.adjust_temp_state = 1;
				count_idx = yingxue_base.shezhi_temp;
			}
			if (state == 0){
				count_idx += 1;
			}
			else{
				count_idx -= 1;
			}

			//温度范围
			if (count_idx < 0 || count_idx > 65){
				return ;
			}


			t_widget = ituSceneFindWidget(&theScene, "Text17");
			sprintf(t_buf, "%d", count_idx);
			yingxue_base.shezhi_temp = count_idx;
			ituTextSetString(t_widget, t_buf);
			//设置温度 模式设置	4	模式设置	设置温度	定升设定
			sendCmdToCtr(0x04, 0x00, count_idx, 0x00, 0x00, SET_TEMP);
		}
	}
}

//预热页面确认
static void yure_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	if (strcmp(widget->name, "BackgroundButton47") == 0){
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//Background27
	else if (strcmp(widget->name, "BackgroundButton14") == 0){
		//单次巡航 从现在到2个小时结束
		//同一个，取消
		if (yingxue_base.yure_mode == 1){
			yingxue_base.yure_mode = 0;
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//发送取消
			SEND_CLOSE_YURE_CMD();
		}
		else{
			//发送开始
			yingxue_base.yure_mode = 1;
			get_rtc_time(&yingxue_base.yure_begtime, NULL);
			//2个小时 
			yingxue_base.yure_endtime.tv_sec = yingxue_base.yure_begtime.tv_sec + 60 * 60 * 2;
			SEND_OPEN_YURE_CMD();

		}
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	else if (strcmp(widget->name, "BackgroundButton19") == 0){
		//全天候模式：
		//启动：从点击巡航模式，即刻开始，发送一次串口命令：预热中的数据2“循环预热”
		if (yingxue_base.yure_mode == 2){
			yingxue_base.yure_mode = 0;
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//发送取消
			SEND_CLOSE_YURE_CMD();
		}
		else{
			yingxue_base.yure_mode = 2;
			get_rtc_time(&yingxue_base.yure_begtime, NULL);
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//发送预热开始
			SEND_OPEN_YURE_CMD();
		}
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//BackgroundButton20
	else if (strcmp(widget->name, "BackgroundButton20") == 0){
		/*
		预约模式：
		启动：定时时间到达，发送一次串口命令：预热中的数据2“循环预热”，
		结束：自动延迟1个小时，发送一次串口命令，TFT在发送 预热命令  0- 预热关闭是吧
		*/
		if (yingxue_base.yure_mode == 3){
			yingxue_base.yure_mode = 0;
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//发送取消
			SEND_CLOSE_YURE_CMD();
		}
		else{
			yingxue_base.yure_mode = 3;
		}
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//设置预热时间
	else if (strcmp(widget->name, "BackgroundButton2") == 0){
		//设置预约时间
		t_widget = ituSceneFindWidget(&theScene, "yureshijianLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//设置预热回水温度和北京时间
	else if (strcmp(widget->name, "BackgroundButton21") == 0){
		t_widget = ituSceneFindWidget(&theScene, "yureshezhiLayer");
		ituLayerGoto((ITULayer *)t_widget);

	}

}

//预热页面上下移动
static void yure_widget_up_down_cb(struct node_widget *widget, unsigned char state)
{
	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;
	if (state == 0){
		if (widget->up)
			t_node_widget = widget->up;
	}
	else{
		if (widget->down){
			t_node_widget = widget->down;
		}
	}

	if (t_node_widget){
		command_widget_up_down(t_node_widget);
	}

}

//预热时间上下移动
static void yure_settime_up_down_cb(struct node_widget *widget, unsigned char state)
{
	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;
	if (state == 0){
		if (widget->up)
			t_node_widget = widget->up;
	}
	else{
		if (widget->down){
			t_node_widget = widget->down;
		}
	}

	if (t_node_widget){
		command_widget_up_down(t_node_widget);
	}
}

//预热定时确定
static void yure_settime_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	int value = widget->value;
	if (strcmp(widget->name, "BackgroundButton65") == 0){
		t_widget = ituSceneFindWidget(&theScene, "yureLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//点击单项按键
	else if (strcmp(widget->focus_back_name, "radio") == 0){
		t_widget = ituSceneFindWidget(&theScene, widget->name);
		//如果已经是点击状态，取消状态
		if (ituRadioBoxIsChecked(t_widget)){
			ituCheckBoxSetChecked((ITUCheckBox *)t_widget, false);
			*(yingxue_base.dingshi_list + value) = 0;
		}
		//不是点击状态，加入点击状态
		else{
			ituCheckBoxSetChecked((ITUCheckBox *)t_widget, true);
			*(yingxue_base.dingshi_list + value) = 1;
		}
	}
}

//预热设置温度和北京时间 上下移动
static void yure_yureshezhiLayer_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	char *t_buf = NULL;
	unsigned char num = 0;
	if (strcmp(widget->name, "BackgroundButton60") == 0){
		t_widget = ituSceneFindWidget(&theScene, "yureLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//支持长按
	else if (widget->type == 1){
		if (widget->state == 0){
			//锁定
			widget->state = 1;
			t_widget = ituSceneFindWidget(&theScene, widget->checked_back_name);
			ituWidgetSetVisible(t_widget, true);
		}
		else{
			//解除锁定Background2 Background3 Background4
			//改变数据
			//回水温度
			if (strcmp(widget->name, "Background2") == 0){
				t_widget = ituSceneFindWidget(&theScene, "Text3");
				t_buf = ituTextGetString(t_widget);
				num = atoi(t_buf);
				//发送改变回水温度,也就是预热回温温度
				yingxue_base.huishui_temp = num;
			}
			//北京时间小时
			else if ((strcmp(widget->name, "Background3") == 0) || (strcmp(widget->name, "Background4") == 0)){
				unsigned char hour = 0;
				unsigned char min = 0;
				struct timeval curr_time;
				struct tm *t_tm;
				get_rtc_time(&curr_time, NULL);
				t_tm = localtime(&curr_time);
				//hour
				t_widget = ituSceneFindWidget(&theScene, "Text42");
				t_buf = ituTextGetString(t_widget);
				hour = atoi(t_buf);
				//min
				t_widget = ituSceneFindWidget(&theScene, "Text43");
				t_buf = ituTextGetString(t_widget);
				min = atoi(t_buf);
				set_rtc_time(hour, min);
			}
			widget->state = 0;
			t_widget = ituSceneFindWidget(&theScene, widget->checked_back_name);
			ituWidgetSetVisible(t_widget, false);
			//设置时间后从新记录一次最后时间
			key_down_process();
		}
	}
}


//预热时间上下移动
static void yure_yureshezhiLayer_up_down_cb(struct node_widget *widget, unsigned char state)
{
	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;

	if (widget->state == 1){ //如果已经锁定
		lock_widget_up_down(widget, state);
	}
	else{
		if (state == 0){
			if (widget->up)
				t_node_widget = widget->up;
		}
		else{
			if (widget->down){
				t_node_widget = widget->down;
			}
		}

		if (t_node_widget){
			command_widget_up_down(t_node_widget);
		}
	}
}

//模式确定
static void moshi_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	//初始化一个控制板数据
	ITUWidget *t_widget = NULL;
	if (strcmp(widget->name, "BackgroundButton68") == 0){
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	else if (strcmp(widget->name, "moshi_BackgroundButton10") == 0){
		//发送模式命令就发指令 4 ： 模式设置  ： 默认 0 ，设置温度 ： XX ， 定升设定  ： 默认值时发0 
		sendCmdToCtr(0x04, 0x00, yingxue_base.normal_moshi.temp, 0x00, 0x00, SET_TEMP);
		yingxue_base.moshi_mode = 1;
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	else if (strcmp(widget->name, "moshi_BackgroundButton11") == 0){
		//发送模式命令就发指令 4 ： 模式设置  ： 默认 0 ，设置温度 ： XX ， 定升设定  ： 默认值时发0 
		sendCmdToCtr(0x04, 0x00, yingxue_base.super_moshi.temp, 0x00, 0x00, SET_TEMP);
		yingxue_base.moshi_mode = 2;
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	else if (strcmp(widget->name, "moshi_BackgroundButton12") == 0){
		//发送模式命令就发指令 4 ： 模式设置  ： 默认 0 ，设置温度 ： XX ， 定升设定  ： 默认值时发0 
		sendCmdToCtr(0x04, 0x00, yingxue_base.eco_moshi.temp, 0x00, 0x00, SET_TEMP);
		yingxue_base.moshi_mode = 3;
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	else if (strcmp(widget->name, "moshi_BackgroundButton13") == 0){
		//发送模式命令就发指令 4 ： 模式设置  ： 默认 0 ，设置温度 ： XX ， 定升设定  ： 默认值时发0 
		sendCmdToCtr(0x04, 0x00, yingxue_base.fruit_moshi.temp, 0x00, 0x00, SET_TEMP);
		yingxue_base.moshi_mode = 4;
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
}

//长按模式
static void moshi_widget_longpress_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	t_widget = ituSceneFindWidget(&theScene, "chushui");
	//chushui
	if (strcmp(widget->name, "moshi_BackgroundButton10") == 0){
		yingxue_base.select_set_moshi_mode = 1;
	}
	else if (strcmp(widget->name, "moshi_BackgroundButton11") == 0){
		yingxue_base.select_set_moshi_mode = 2;
	}
	else if (strcmp(widget->name, "moshi_BackgroundButton12") == 0){
		yingxue_base.select_set_moshi_mode = 3;
	}
	else if (strcmp(widget->name, "moshi_BackgroundButton13") == 0){
		yingxue_base.select_set_moshi_mode = 4;
	}
	ituLayerGoto(t_widget);
}

//预热时间上下移动
static void moshi_up_down_cb(struct node_widget *widget, unsigned char state)
{
	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;

	if (state == 0){
		if (widget->up)
			t_node_widget = widget->up;
	}
	else{
		if (widget->down){
			t_node_widget = widget->down;
		}
	}
	if (t_node_widget){
		command_widget_up_down(t_node_widget);
	}
}

//设置模式温度
static void chushui_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	char *t_buf;
	int num = 0;
	t_widget = ituSceneFindWidget(&theScene, "moshiLayer");
	if (strcmp(widget->name, "chushui_BackgroundButton73") == 0){
		ituLayerGoto((ITULayer *)t_widget);
	}
	//支持长按
	else if (widget->type == 1){
		if (widget->state == 0){
			//锁定
			widget->state = 1;
			t_widget = ituSceneFindWidget(&theScene, widget->checked_back_name);
			ituWidgetSetVisible(t_widget, true);
		}
		else{
			//解除锁定
			widget->state = 0;
			t_widget = ituSceneFindWidget(&theScene, widget->checked_back_name);
			ituWidgetSetVisible(t_widget, false);
		}
	}
	//点击确认，后确定
	else if (strcmp(widget->name, "chushui_BackgroundButton1") == 0){
		//Text38
		t_widget = ituSceneFindWidget(&theScene, "Text38");
		t_buf = ituTextGetString((ITUText*)t_widget);
		num = atoi(t_buf);
		if (yingxue_base.select_set_moshi_mode > 0){
			if (yingxue_base.select_set_moshi_mode == 1){
				yingxue_base.normal_moshi.temp = num;
			}
			else if (yingxue_base.select_set_moshi_mode == 2){
				yingxue_base.super_moshi.temp = num;
			}
			else if (yingxue_base.select_set_moshi_mode == 3){
				yingxue_base.eco_moshi.temp = num;
			}
			else if (yingxue_base.select_set_moshi_mode == 4){
				yingxue_base.fruit_moshi.temp = num;
			}
		}
		t_widget = ituSceneFindWidget(&theScene, "moshiLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}

}

//设置模式温度
static void chushui_up_down_cb(struct node_widget *widget, unsigned char state)
{
	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;

	if (widget->state == 1){ //如果已经锁定
		lock_widget_up_down(widget, state);
	}
	else{
		if (state == 0){
			if (widget->up)
				t_node_widget = widget->up;
		}
		else{
			if (widget->down){
				t_node_widget = widget->down;
			}
		}

		if (t_node_widget){
			command_widget_up_down(t_node_widget);
		}
	}
}


//设置出厂模式
static void layer1_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	char *t_buf;
	int num = 0;
	t_widget = ituSceneFindWidget(&theScene, "MainLayer");
	if (strcmp(widget->name, "BackgroundButton60") == 0){
		ituLayerGoto((ITULayer *)t_widget);
	}
	//支持长按
	else if (widget->type == 1){
		if (widget->state == 0){
			//锁定
			widget->state = 1;
			if (widget->checked_back_name){
				t_widget = ituSceneFindWidget(&theScene, widget->checked_back_name);
				ituWidgetSetVisible(t_widget, true);
			}
		}
		else{
			//解除锁定
			widget->state = 0;
			if (widget->checked_back_name){
				t_widget = ituSceneFindWidget(&theScene, widget->checked_back_name);
				ituWidgetSetVisible(t_widget, false);
			}
		}
	}
	


}

static void layer1_up_down_cb(struct node_widget *widget, unsigned char state)
{

	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;
	char t_buf[20] = { 0 };
	unsigned char t_num = 0;
	if (widget->state == 1){ //如果已经锁定
		//lock_widget_up_down(widget, state);
		//pa 对应 FA
		//(unsigned char cmd, unsigned char data_1, unsigned char data_2, unsigned char data_3, unsigned char data_4, enum main_pthread_mq_state state)
		if (strcmp(widget->name, "Text22") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text22");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
			sendCmdToCtr(0x10, t_num, 0, 0, 0, SET_CHUCHANG);
		}
		else if (strcmp(widget->name, "Text90") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text90");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
			sendCmdToCtr(0x13, t_num, 0, 0, 0, SET_CHUCHANG);
		}
		else if (strcmp(widget->name, "Text33") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text33");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
			sendCmdToCtr(0x11, t_num, 0, 0, 0, SET_CHUCHANG);
		}
		else if (strcmp(widget->name, "Text82") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text82");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
		}
		else if (strcmp(widget->name, "Text39") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text39");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
			sendCmdToCtr(0x12, t_num, 0, 0, 0, SET_CHUCHANG);
			
		}
		//空
		else if (strcmp(widget->name, "Text80") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text80");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
		}
		else if (strcmp(widget->name, "Text45") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text45");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
			sendCmdToCtr(0x13, t_num, 0, 0, 0, SET_CHUCHANG);
		}
		else if (strcmp(widget->name, "Text73") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text73");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
			yingxue_base.huishui_temp = t_num;
		}
		else if (strcmp(widget->name, "Text58") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text58");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
			sendCmdToCtr(0x14, t_num, 0, 0, 0, SET_CHUCHANG);
		}
		else if (strcmp(widget->name, "Text65") == 0){
			t_widget = ituSceneFindWidget(&theScene, "Text65");
			t_num = atoi(ituTextGetString((ITUText*)t_widget));
		}
		if (state == 0){
			t_num = t_num + 1;
		}
		else{
			t_num = t_num - 1;
		}
		sprintf(t_buf, "%d", t_num);
		ituTextSetString(t_widget, t_buf);

	}
	else{
		if (state == 0){
			if (widget->up)
				t_node_widget = widget->up;
		}
		else{
			if (widget->down){
				t_node_widget = widget->down;
			}
		}

		if (t_node_widget){
			curr_node_widget = t_node_widget;
		}
	}

}





//初始化数据,基础数据
static void yingxue_base_init()
{
	memset(&yingxue_base, 0, sizeof(struct yingxue_base_tag));

	yingxue_base.normal_moshi.temp = 37;
	yingxue_base.super_moshi.temp = 36;
	yingxue_base.eco_moshi.temp = 35;
	yingxue_base.fruit_moshi.temp = 34;
	yingxue_base.shezhi_temp = 35;
	yingxue_base.is_err = 0;

}

//控制模块初始化
static void node_widget_init()
{
	curr_node_widget = NULL;
	//主页面
	mainlayer_0.up = NULL;
	mainlayer_0.down = &mainlayer_1;
	mainlayer_0.focus_back_name = "Background100";
	mainlayer_0.name = "yureSprite";
	mainlayer_0.confirm_cb = main_widget_confirm_cb;
	mainlayer_0.updown_cb = main_widget_up_down_cb;

	mainlayer_1.up = &mainlayer_0;
	mainlayer_1.down = &mainlayer_2;
	mainlayer_1.focus_back_name = "Background134";
	mainlayer_1.name = "BackgroundButton3";
	mainlayer_1.confirm_cb = main_widget_confirm_cb;
	mainlayer_1.updown_cb = main_widget_up_down_cb;

	mainlayer_2.up = &mainlayer_1;
	mainlayer_2.down = NULL;
	mainlayer_2.focus_back_name = "Background102";
	mainlayer_2.name = "moshiSprite";
	mainlayer_2.confirm_cb = main_widget_confirm_cb;
	mainlayer_2.updown_cb = main_widget_up_down_cb;


	//预热界面
	yureLayer_0.up = NULL;
	yureLayer_0.down = &yureLayer_1;
	yureLayer_0.focus_back_name = "BackgroundButton78";
	yureLayer_0.name = "BackgroundButton47";
	yureLayer_0.confirm_cb = yure_widget_confirm_cb;
	yureLayer_0.updown_cb = yure_widget_up_down_cb;

	yureLayer_1.up = &yureLayer_0;
	yureLayer_1.down = &yureLayer_2;
	yureLayer_1.focus_back_name = "Background27";
	yureLayer_1.name = "BackgroundButton14";
	yureLayer_1.confirm_cb = yure_widget_confirm_cb;
	yureLayer_1.updown_cb = yure_widget_up_down_cb;

	yureLayer_2.up = &yureLayer_1;
	yureLayer_2.down = &yureLayer_3;
	yureLayer_2.focus_back_name = "Background30";
	yureLayer_2.name = "BackgroundButton19";
	yureLayer_2.confirm_cb = yure_widget_confirm_cb;
	yureLayer_2.updown_cb = yure_widget_up_down_cb;


	yureLayer_3.up = &yureLayer_2;
	yureLayer_3.down = &yureLayer_4;
	yureLayer_3.focus_back_name = "Background132";
	yureLayer_3.name = "BackgroundButton20";
	yureLayer_3.confirm_cb = yure_widget_confirm_cb;
	yureLayer_3.updown_cb = yure_widget_up_down_cb;

	yureLayer_4.up = &yureLayer_3;
	yureLayer_4.down = &yureLayer_5;
	yureLayer_4.focus_back_name = "Background94";
	yureLayer_4.name = "BackgroundButton2";
	yureLayer_4.confirm_cb = yure_widget_confirm_cb;
	yureLayer_4.updown_cb = yure_widget_up_down_cb;

	yureLayer_5.up = &yureLayer_4;
	yureLayer_5.down = NULL;
	yureLayer_5.focus_back_name = "Background46";
	yureLayer_5.name = "BackgroundButton21";
	yureLayer_5.confirm_cb = yure_widget_confirm_cb;
	yureLayer_5.updown_cb = yure_widget_up_down_cb;

	//预热定时
	yureshijian_widget_0.up = NULL;
	yureshijian_widget_0.down = &yureshijian_widget_num_0;
	yureshijian_widget_0.focus_back_name = "BackgroundButton30"; //选中
	yureshijian_widget_0.name = "BackgroundButton65";//未选中
	yureshijian_widget_0.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_0.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_0.value = 0;
	yureshijian_widget_num_0.up = &yureshijian_widget_0;
	yureshijian_widget_num_0.down = &yureshijian_widget_num_1;
	yureshijian_widget_num_0.focus_back_name = "radio";
	yureshijian_widget_num_0.name = "RadioBox2";
	yureshijian_widget_num_0.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_0.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_1.value = 1;
	yureshijian_widget_num_1.up = &yureshijian_widget_num_0;
	yureshijian_widget_num_1.down = &yureshijian_widget_num_2;
	yureshijian_widget_num_1.focus_back_name = "radio";
	yureshijian_widget_num_1.name = "RadioBox5";
	yureshijian_widget_num_1.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_1.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_2.value = 2;
	yureshijian_widget_num_2.up = &yureshijian_widget_num_1;
	yureshijian_widget_num_2.down = &yureshijian_widget_num_3;
	yureshijian_widget_num_2.focus_back_name = "radio";
	yureshijian_widget_num_2.name = "RadioBox31";
	yureshijian_widget_num_2.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_2.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_3.value = 3;
	yureshijian_widget_num_3.up = &yureshijian_widget_num_2;
	yureshijian_widget_num_3.down = &yureshijian_widget_num_4;
	yureshijian_widget_num_3.focus_back_name = "radio";
	yureshijian_widget_num_3.name = "RadioBox32";
	yureshijian_widget_num_3.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_3.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_4.value = 4;
	yureshijian_widget_num_4.up = &yureshijian_widget_num_3;
	yureshijian_widget_num_4.down = &yureshijian_widget_num_5;
	yureshijian_widget_num_4.focus_back_name = "radio";
	yureshijian_widget_num_4.name = "RadioBox59";
	yureshijian_widget_num_4.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_4.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_5.value = 5;
	yureshijian_widget_num_5.up = &yureshijian_widget_num_4;
	yureshijian_widget_num_5.down = &yureshijian_widget_num_6;
	yureshijian_widget_num_5.focus_back_name = "radio";
	yureshijian_widget_num_5.name = "RadioBox58";
	yureshijian_widget_num_5.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_5.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_6.value = 6;
	yureshijian_widget_num_6.up = &yureshijian_widget_num_5;
	yureshijian_widget_num_6.down = &yureshijian_widget_num_7;
	yureshijian_widget_num_6.focus_back_name = "radio";
	yureshijian_widget_num_6.name = "RadioBox57";
	yureshijian_widget_num_6.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_6.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_7.value = 7;
	yureshijian_widget_num_7.up = &yureshijian_widget_num_6;
	yureshijian_widget_num_7.down = &yureshijian_widget_num_8;
	yureshijian_widget_num_7.focus_back_name = "radio";
	yureshijian_widget_num_7.name = "RadioBox56";
	yureshijian_widget_num_7.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_7.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_8.value = 8;
	yureshijian_widget_num_8.up = &yureshijian_widget_num_7;
	yureshijian_widget_num_8.down = &yureshijian_widget_num_9;
	yureshijian_widget_num_8.focus_back_name = "radio";
	yureshijian_widget_num_8.name = "RadioBox75";
	yureshijian_widget_num_8.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_8.updown_cb = yure_settime_up_down_cb;


	yureshijian_widget_num_9.value = 9;
	yureshijian_widget_num_9.up = &yureshijian_widget_num_8;
	yureshijian_widget_num_9.down = &yureshijian_widget_num_10;
	yureshijian_widget_num_9.focus_back_name = "radio";
	yureshijian_widget_num_9.name = "RadioBox74";
	yureshijian_widget_num_9.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_9.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_10.value = 10;
	yureshijian_widget_num_10.up = &yureshijian_widget_num_9;
	yureshijian_widget_num_10.down = &yureshijian_widget_num_11;
	yureshijian_widget_num_10.focus_back_name = "radio";
	yureshijian_widget_num_10.name = "RadioBox73";
	yureshijian_widget_num_10.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_10.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_11.value = 11;
	yureshijian_widget_num_11.up = &yureshijian_widget_num_10;
	yureshijian_widget_num_11.down = &yureshijian_widget_num_12;
	yureshijian_widget_num_11.focus_back_name = "radio";
	yureshijian_widget_num_11.name = "RadioBox72";
	yureshijian_widget_num_11.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_11.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_12.value = 12;
	yureshijian_widget_num_12.up = &yureshijian_widget_num_11;
	yureshijian_widget_num_12.down = &yureshijian_widget_num_13;
	yureshijian_widget_num_12.focus_back_name = "radio";
	yureshijian_widget_num_12.name = "RadioBox67";
	yureshijian_widget_num_12.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_12.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_13.value = 13;
	yureshijian_widget_num_13.up = &yureshijian_widget_num_12;
	yureshijian_widget_num_13.down = &yureshijian_widget_num_14;
	yureshijian_widget_num_13.focus_back_name = "radio";
	yureshijian_widget_num_13.name = "RadioBox66";
	yureshijian_widget_num_13.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_13.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_14.value = 14;
	yureshijian_widget_num_14.up = &yureshijian_widget_num_13;
	yureshijian_widget_num_14.down = &yureshijian_widget_num_15;
	yureshijian_widget_num_14.focus_back_name = "radio";
	yureshijian_widget_num_14.name = "RadioBox65";
	yureshijian_widget_num_14.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_14.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_15.value = 15;
	yureshijian_widget_num_15.up = &yureshijian_widget_num_14;
	yureshijian_widget_num_15.down = &yureshijian_widget_num_16;
	yureshijian_widget_num_15.focus_back_name = "radio";
	yureshijian_widget_num_15.name = "RadioBox64";
	yureshijian_widget_num_15.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_15.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_16.value = 16;
	yureshijian_widget_num_16.up = &yureshijian_widget_num_15;
	yureshijian_widget_num_16.down = &yureshijian_widget_num_17;
	yureshijian_widget_num_16.focus_back_name = "radio";
	yureshijian_widget_num_16.name = "RadioBox92";
	yureshijian_widget_num_16.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_16.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_17.value = 17;
	yureshijian_widget_num_17.up = &yureshijian_widget_num_16;
	yureshijian_widget_num_17.down = &yureshijian_widget_num_18;
	yureshijian_widget_num_17.focus_back_name = "radio";
	yureshijian_widget_num_17.name = "RadioBox91";
	yureshijian_widget_num_17.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_17.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_18.value = 18;
	yureshijian_widget_num_18.up = &yureshijian_widget_num_17;
	yureshijian_widget_num_18.down = &yureshijian_widget_num_19;
	yureshijian_widget_num_18.focus_back_name = "radio";
	yureshijian_widget_num_18.name = "RadioBox90";
	yureshijian_widget_num_18.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_18.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_19.value = 19;
	yureshijian_widget_num_19.up = &yureshijian_widget_num_18;
	yureshijian_widget_num_19.down = &yureshijian_widget_num_20;
	yureshijian_widget_num_19.focus_back_name = "radio";
	yureshijian_widget_num_19.name = "RadioBox89";
	yureshijian_widget_num_19.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_19.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_20.value = 20;
	yureshijian_widget_num_20.up = &yureshijian_widget_num_19;
	yureshijian_widget_num_20.down = &yureshijian_widget_num_21;
	yureshijian_widget_num_20.focus_back_name = "radio";
	yureshijian_widget_num_20.name = "RadioBox83";
	yureshijian_widget_num_20.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_20.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_21.value = 21;
	yureshijian_widget_num_21.up = &yureshijian_widget_num_20;
	yureshijian_widget_num_21.down = &yureshijian_widget_num_22;
	yureshijian_widget_num_21.focus_back_name = "radio";
	yureshijian_widget_num_21.name = "RadioBox82";
	yureshijian_widget_num_21.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_21.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_22.value = 22;
	yureshijian_widget_num_22.up = &yureshijian_widget_num_21;
	yureshijian_widget_num_22.down = &yureshijian_widget_num_23;
	yureshijian_widget_num_22.focus_back_name = "radio";
	yureshijian_widget_num_22.name = "RadioBox81";
	yureshijian_widget_num_22.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_22.updown_cb = yure_settime_up_down_cb;

	yureshijian_widget_num_23.value = 23;
	yureshijian_widget_num_23.up = &yureshijian_widget_num_22;
	yureshijian_widget_num_23.down = NULL;
	yureshijian_widget_num_23.focus_back_name = "radio";
	yureshijian_widget_num_23.name = "RadioBox80";
	yureshijian_widget_num_23.confirm_cb = yure_settime_widget_confirm_cb;
	yureshijian_widget_num_23.updown_cb = yure_settime_up_down_cb;

	//预约时间
	yureshezhiLayer_0.up = NULL;
	yureshezhiLayer_0.down = &yureshezhiLayer_1;
	yureshezhiLayer_0.focus_back_name = "BackgroundButton85";
	yureshezhiLayer_0.name = "BackgroundButton60";
	yureshezhiLayer_0.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_0.updown_cb = yure_yureshezhiLayer_up_down_cb;

	yureshezhiLayer_1.up = &yureshezhiLayer_0;
	yureshezhiLayer_1.down = &yureshezhiLayer_2;
	yureshezhiLayer_1.focus_back_name = "Background37";
	yureshezhiLayer_1.checked_back_name = "Background45";
	yureshezhiLayer_1.name = "Background2";
	yureshezhiLayer_1.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_1.updown_cb = yure_yureshezhiLayer_up_down_cb;
	yureshezhiLayer_1.type = 1;

	yureshezhiLayer_2.up = &yureshezhiLayer_1;
	yureshezhiLayer_2.down = &yureshezhiLayer_3;
	yureshezhiLayer_2.focus_back_name = "Background33";
	yureshezhiLayer_2.checked_back_name = "Background105";
	yureshezhiLayer_2.name = "Background3";
	yureshezhiLayer_2.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_2.updown_cb = yure_yureshezhiLayer_up_down_cb;
	yureshezhiLayer_2.type = 1;


	yureshezhiLayer_3.up = &yureshezhiLayer_2;
	yureshezhiLayer_3.down = NULL;
	yureshezhiLayer_3.focus_back_name = "Background40";
	yureshezhiLayer_3.checked_back_name = "Background107";
	yureshezhiLayer_3.name = "Background4";
	yureshezhiLayer_3.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_3.updown_cb = yure_yureshezhiLayer_up_down_cb;
	yureshezhiLayer_3.type = 1;

	//模式
	moshiLayer_0.up = NULL;
	moshiLayer_0.down = &moshiLayer_1;
	moshiLayer_0.focus_back_name = "BackgroundButton33";
	moshiLayer_0.name = "BackgroundButton68";
	moshiLayer_0.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_0.updown_cb = moshi_up_down_cb;

	moshiLayer_1.up = &moshiLayer_0;
	moshiLayer_1.down = &moshiLayer_2;
	moshiLayer_1.focus_back_name = "moshi_BackgroundButton80";
	moshiLayer_1.name = "moshi_BackgroundButton10";
	moshiLayer_1.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_1.updown_cb = moshi_up_down_cb;
	moshiLayer_1.long_press_cb = moshi_widget_longpress_cb;

	moshiLayer_2.up = &moshiLayer_1;
	moshiLayer_2.down = &moshiLayer_3;
	moshiLayer_2.focus_back_name = "moshi_BackgroundButton79";
	moshiLayer_2.name = "moshi_BackgroundButton11";
	moshiLayer_2.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_2.updown_cb = moshi_up_down_cb;
	moshiLayer_2.long_press_cb = moshi_widget_longpress_cb;

	moshiLayer_3.up = &moshiLayer_2;
	moshiLayer_3.down = &moshiLayer_4;
	moshiLayer_3.focus_back_name = "moshi_BackgroundButton81";
	moshiLayer_3.name = "moshi_BackgroundButton12";
	moshiLayer_3.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_3.updown_cb = moshi_up_down_cb;
	moshiLayer_3.long_press_cb = moshi_widget_longpress_cb;

	moshiLayer_4.up = &moshiLayer_3;
	moshiLayer_4.down = NULL;
	moshiLayer_4.focus_back_name = "moshi_BackgroundButton82";
	moshiLayer_4.name = "moshi_BackgroundButton13";
	moshiLayer_4.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_4.updown_cb = moshi_up_down_cb;
	moshiLayer_4.long_press_cb = moshi_widget_longpress_cb;

	//出水
	chushui_0.up = NULL;
	chushui_0.down = &chushui_1;
	chushui_0.focus_back_name = "chushui_BackgroundButton7";
	chushui_0.name = "chushui_BackgroundButton73";
	chushui_0.confirm_cb = chushui_widget_confirm_cb;
	chushui_0.updown_cb = chushui_up_down_cb;

	chushui_1.up = &chushui_0;
	chushui_1.down = &chushui_2;
	chushui_1.focus_back_name = "chushui_Background37";
	chushui_1.checked_back_name = "chushui_Background45";
	chushui_1.name = "chushui_Background13";
	chushui_1.confirm_cb = chushui_widget_confirm_cb;
	chushui_1.updown_cb = chushui_up_down_cb;
	chushui_1.type = 1;

	chushui_2.up = &chushui_1;
	chushui_2.down = NULL;
	chushui_2.focus_back_name = "chushui_Background51";
	chushui_2.name = "chushui_BackgroundButton1";
	chushui_2.confirm_cb = chushui_widget_confirm_cb;
	chushui_2.updown_cb = chushui_up_down_cb;

	//出厂设置
	//返回按键
	layer1_0.up = NULL;
	layer1_0.down = &layer1_6;
	layer1_0.focus_back_name = NULL;
	layer1_0.name = "BackgroundButton60";
	layer1_0.confirm_cb = layer1_widget_confirm_cb;
	layer1_0.updown_cb = layer1_up_down_cb;





	//水流
	layer1_1.up = &layer1_0;
	layer1_1.down = &layer1_2;
	layer1_1.focus_back_name = NULL;
	layer1_1.name = "Background99";
	layer1_1.confirm_cb = layer1_widget_confirm_cb;
	layer1_1.updown_cb = layer1_up_down_cb;



	//火焰
	layer1_2.up = &layer1_1;
	layer1_2.down = &layer1_3;
	layer1_2.focus_back_name = NULL;
	layer1_2.name = "Background101";
	layer1_2.confirm_cb = layer1_widget_confirm_cb;
	layer1_2.updown_cb = layer1_up_down_cb;


	//风机
	layer1_3.up = &layer1_2;
	layer1_3.down = &layer1_4;
	layer1_3.focus_back_name = NULL;
	layer1_3.name = "Background103";
	layer1_3.confirm_cb = layer1_widget_confirm_cb;
	layer1_3.updown_cb = layer1_up_down_cb;

	//出水温度
	layer1_4.up = &layer1_3;
	layer1_4.down = &layer1_5;
	layer1_4.focus_back_name = NULL;
	layer1_4.name = "Text94";
	layer1_4.confirm_cb = layer1_widget_confirm_cb;
	layer1_4.updown_cb = layer1_up_down_cb;


	//风速
	layer1_5.up = &layer1_4;
	layer1_5.down = &layer1_6;
	layer1_5.focus_back_name = NULL;
	layer1_5.name = "Text96";
	layer1_5.confirm_cb = layer1_widget_confirm_cb;
	layer1_5.updown_cb = layer1_up_down_cb;

	//pa
	layer1_6.up = &layer1_5;
	layer1_6.down = &layer1_7;
	layer1_6.focus_back_name = NULL;
	layer1_6.name = "Text22";
	layer1_6.confirm_cb = layer1_widget_confirm_cb;
	layer1_6.updown_cb = layer1_up_down_cb;
	layer1_6.type = 1;

	//dh
	layer1_7.up = &layer1_6;
	layer1_7.down = &layer1_8;
	layer1_7.focus_back_name = NULL;
	layer1_7.name = "Text90";
	layer1_7.confirm_cb = layer1_widget_confirm_cb;
	layer1_7.updown_cb = layer1_up_down_cb;
	layer1_7.type = 1;





	//ph
	layer1_8.up = &layer1_7;
	layer1_8.down = &layer1_9;
	layer1_8.focus_back_name = NULL;
	layer1_8.name = "Text33";
	layer1_8.confirm_cb = layer1_widget_confirm_cb;
	layer1_8.updown_cb = layer1_up_down_cb;
	layer1_8.type = 1;

	//fy
	layer1_9.up = &layer1_8;
	layer1_9.down = &layer1_10;
	layer1_9.focus_back_name = NULL;
	layer1_9.name = "Text82";
	layer1_9.confirm_cb = layer1_widget_confirm_cb;
	layer1_9.updown_cb = layer1_up_down_cb;
	layer1_9.type = 1;


	//pl
	layer1_10.up = &layer1_9;
	layer1_10.down = &layer1_11;
	layer1_10.focus_back_name = NULL;
	layer1_10.name = "Text39";
	layer1_10.confirm_cb = layer1_widget_confirm_cb;
	layer1_10.updown_cb = layer1_up_down_cb;
	layer1_10.type = 1;

	//fd
	layer1_11.up = &layer1_10;
	layer1_11.down = &layer1_12;
	layer1_11.focus_back_name = NULL;
	layer1_11.name = "Text80";
	layer1_11.confirm_cb = layer1_widget_confirm_cb;
	layer1_11.updown_cb = layer1_up_down_cb;
	layer1_11.type = 1;

	//DH
	layer1_12.up = &layer1_11;
	layer1_12.down = &layer1_13;
	layer1_12.focus_back_name = NULL;
	layer1_12.name = "Text45";
	layer1_12.confirm_cb = layer1_widget_confirm_cb;
	layer1_12.updown_cb = layer1_up_down_cb;
	layer1_12.type = 1;


	//hs
	layer1_13.up = &layer1_12;
	layer1_13.down = &layer1_14;
	layer1_13.focus_back_name = NULL;
	layer1_13.name = "Text73";
	layer1_13.confirm_cb = layer1_widget_confirm_cb;
	layer1_13.updown_cb = layer1_up_down_cb;
	layer1_13.type = 1;


	//hi
	layer1_14.up = &layer1_13;
	layer1_14.down = &layer1_15;
	layer1_14.focus_back_name = NULL;
	layer1_14.name = "Text58";
	layer1_14.confirm_cb = layer1_widget_confirm_cb;
	layer1_14.updown_cb = layer1_up_down_cb;
	layer1_14.type = 1;

	//ed
	layer1_15.up = &layer1_14;
	layer1_15.down = NULL;
	layer1_15.focus_back_name = NULL;
	layer1_15.name = "Text65";
	layer1_15.confirm_cb = layer1_widget_confirm_cb;
	layer1_15.updown_cb = layer1_up_down_cb;
	




	//初始一次按键超时数据
	key_down_process();
}

//ok
//是否闪烁
extern char is_shake;

//超时的处理函数
static void over_time_process()
{
	struct timeval now_t = { 0 };
	get_rtc_time(&now_t, NULL);
	//已经处理过
	if (is_deal_over_time == 1) return;
	//如何当前的时间大于3秒
	if (now_t.tv_sec > last_down_time.tv_sec + 3){
		memset(&last_down_time, 0, sizeof(struct timeval));
		//已经处理过
		is_deal_over_time = 1;
		//如果是调整温度,给闪烁
		if (yingxue_base.adjust_temp_state == 1){
			is_shake = 1;
		}
		else{
			if (curr_node_widget){
				if (strcmp(curr_node_widget->name, "BackgroundButton3") != 0){
					ituLayerGoto(ituSceneFindWidget(&theScene, "MainLayer"));
				}
			}

		}
	}
	return;
}

//发送命令到控制板
void sendCmdToCtr(unsigned char cmd, unsigned char data_1, unsigned char data_2, unsigned char data_3, unsigned char data_4, enum main_pthread_mq_state state)
{
	struct main_pthread_mq_tag mq_data;
	mq_data.state = (unsigned char)state;
	processCmdToCtrData(cmd, data_1, data_2, data_3, data_4, mq_data.s_data);
	struct timespec tm;
	memset(&tm, 0, sizeof(struct timespec));
	tm.tv_sec = 1;
	mq_timedsend(uartQueue, &mq_data, sizeof(struct main_pthread_mq_tag), 1, &tm);
	return 0;
}

//反馈数据 标识和 目标地址，优先级
unsigned char frame_0 = 0xEB;
unsigned char frame_1 = 0x1B;
//组合数据
void processCmdToCtrData(unsigned char cmd, unsigned char data_1,
	unsigned char data_2, unsigned char data_3, unsigned char data_4, unsigned char *dst)
{
	unsigned short crc = 0;
	unsigned char *old = dst;
	*dst++ = frame_0;
	*dst++ = frame_1;
	*dst++ = cmd;
	*dst++ = data_1;
	*dst++ = data_2;
	*dst++ = data_3;
	*dst++ = data_4;
	*dst++ = 0x01;
	*dst++ = 0x00;
	//计算CRC
	crc = crc16_ccitt(old + 1, 8);
	*dst++ = (unsigned char)(crc >> 8);
	*dst++ = (unsigned char)crc;
	return;
}


/*
从缓存中获取数据，并且分析帧数据
@param dst 从缓存中获取一帧完整的数据
@param p_chain_list 缓存队列
@return
*/
unsigned char
process_data(struct uart_data_tag *dst, struct chain_list_tag *p_chain_list)
{
	//错误标识
	unsigned char flag = 0;
	//crc验证
	unsigned short crc = 0;
	unsigned char buf = 0;

	while (out_chain_list(p_chain_list, &buf)){
		//上一个数据错误，等待一下个0xEA
		if (dst->state == 1){
			//下一个0xEA 重新计算
			if (buf == 0xEA){
				(dst->buf_data)[dst->count++] = buf;
				dst->state = 0;
			}
		}
		else{
			dst->buf_data[dst->count++] = buf;
			//接受完17个数据 结束
			if (dst->count == 17){
				//检查crc 
				crc = crc16_ccitt(dst->buf_data + 1, 14);
				if (((unsigned char)(crc >> 8) == dst->buf_data[15]) &&
					((unsigned char)crc == dst->buf_data[16])
					){
					flag = 0;
				}
				else{
					flag = 1;
				}
#ifdef _WIN32
				//测试方便去掉crc
				flag = 0;
#endif
				//如果出错
				if (flag == 1){
					dst->state = 1;
					dst->count = 0;
				}
				else{
					dst->count = 0;
					dst->state = 2;
					return 0;
				}
			}
		}
	}
	return 0;
}
/*
分析串口帧数据
@param dst 目标数据，分析一帧结束后，所存入的数据
@param p_chain_list 缓存队列
@return
*/
void process_frame(struct child_to_pthread_mq_tag *dst, const unsigned char *src)
{
	unsigned char *old = NULL;
	//第几帧 0 , 1 , 2 , 3
	unsigned char idx = 0;
	//判断第几个帧
	idx = (*(src + 2) & 0x0f);
	old = src + 3;
	if (idx == 0x00){
		//得到主机状态
		dst->machine_state = *(old+1) & 0x03;
		//判断是否故障
		if ((*(old+1) & 0x04) == 0){
			dst->is_err = 0;
		}
		else{
			dst->is_err = 1;
		}
		//判断状态 流水
		if (*(old + 1) & 0x10){
			dst->state_show = 0x01;
			//风机
		}
		if (*(old + 1) & 0x20){
			dst->state_show = dst->state_show | 0x2;
			//火焰
		}
		if (*(old + 1) & 0x40){
			dst->state_show = dst->state_show | 0x4;
			//风压
		}
		if (*(old + 1) & 0x80){
			dst->state_show = dst->state_show | 0x8;
		}
		//设置温度[0][4]
		dst->shezhi_temp = *(old+4);
		//出水温度[0][5]
		dst->chushui_temp = *(old + 5);
		//进水温度[0][6]
		dst->jinshui_temp = *(old + 6);
		//错误代码或者比例阀电流[0][8]
		dst->err_no = *(old + 8);
		//风机转速[0][10]
		dst->wind_rate = *(old + 10);

		//test
		dst->is_err = 0;
	}
	else if (idx == 0x01){
		//无需解析
	}
	else if (idx == 0x02){
		//[2][0] 当前气源号
		dst->fa_num = *old;
		//[2][3]
		dst->dh_num = *(old + 3);
		//[2][1]
		dst->ph_num = *(old + 1);
		//[2][4]
		dst->ne_num = *(old + 4);
	}
	else if (idx == 0x03){
		//[3][2] 回水温度
		dst->huishui_temp = *(old + 2);
	}
	
}


#ifdef _WIN32
//win虚拟机测试
static unsigned char win_test()
{
	/*  0xEA, 0x1B, 0x10, 0x4D, 0x00, 0x00, 0x00, 0x2D, 0x00, 0x00, 0x00, 0x00, 0x41, 0x00, 0x00, 0x79, 0x53,
	0xEA, 0x1B, 0x11, 0x01, 0x00, 0x00, 0x00, 0x1E, 0x0A, 0x2A, 0x28, 0x26, 0x2A, 0x00, 0x00, 0x48, 0x35,
	0xEA, 0x1B, 0x12, 0x00, 0xCD, 0x80, 0x9E, 0x1E, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x03, 0x05, 0x4B,
	0xEA, 0x1B, 0x13, 0x00, 0x00, 0x05, 0x40, 0x50, 0x00, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0xBE, 0x5F,*/
	unsigned char test_buf[68] = {
		//[0][0] //[0][1]                                                 //erno
		0xEA, 0x1B, 0x10, 0x4D, 0x00, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x10, 0x11, 0x79, 0x53,
		0xEA, 0x1B, 0x11, 0x01, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x20, 0x21, 0x48, 0x35,
		0xEA, 0x1B, 0x12, 0x02, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x30, 0x31, 0x05, 0x4B,
		0xEA, 0x1B, 0x13, 0x03, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x40, 0x41, 0xBE, 0x5F,
	};
	static int idx;
	unsigned char res;

	res = test_buf[idx++];
	if (idx == 68){
		idx = 0;
	}
	return res;
}
#endif
//ok
//线程串口回调函数
static void* UartFunc(void* arg)
{
	//记录当前时间
	struct   timeval rev_time;
	//主线程发送消息队列
	struct main_pthread_mq_tag main_pthread_mq;
	//缓存数据
	struct uart_data_tag uart_data;
	memset(&uart_data, 0, sizeof(struct uart_data_tag));

	//线程数据
	//struct main_uart_chg main_uart_chg_data;

	//子线程到主线程的数据
	struct child_to_pthread_mq_tag child_to_pthread_mq;

	uint8_t rece_buf[10];
	int len = 0;
	int flag = 0;
	int is_has = 0;
	struct timespec tm;
	//默认应答
	uint8_t texBufArray[11] = { 0 };
	uint8_t backBufArray[11] = { 0xEB, 0x1B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0xD8, 0x2A };
	//初始化时间
	get_rtc_time(&rev_time, NULL);
	while (1){

		memset(rece_buf, 0, sizeof(rece_buf));
		//如果是win虚拟测试
#ifdef _WIN32
		len = 1;
		rece_buf[0] = win_test();
#else
		len = read(UART_PORT, rece_buf, sizeof(rece_buf));
#endif
		//如果串口有数据
		if (len > 0){
			//记录当前收到数据的时间
			get_rtc_time(&rev_time, NULL);
			//写入环形缓存
			for (int i = 0; i < len; i++){
				flag = in_chain_list(&chain_list, rece_buf[i]);
			}
			process_data(&uart_data, &chain_list);
			//已经完成
			if (uart_data.state == 2){

				LOG_RECE_UART(uart_data.buf_data);
				printf("\n\n");

				//打印结束
				is_has = 0;
				uart_data.state = 0;
				uart_data.count = 0;
				//分析收到的数
				process_frame(&child_to_pthread_mq, uart_data.buf_data);

				memset(&tm, 0, sizeof(struct timespec));
				tm.tv_sec = 1;
				mq_timedsend(childQueue, &child_to_pthread_mq, sizeof(struct child_to_pthread_mq_tag), 1, &tm);
				//接受数据
				flag = mq_receive(uartQueue, &main_pthread_mq, sizeof(struct main_pthread_mq_tag), NULL);
				//如果存在信息就发送消息
				if (flag > 0){
					memcpy(texBufArray, main_pthread_mq.s_data, sizeof(texBufArray));
					is_has = 1;
				}
				//如果有指令需要发出
				if (is_has){
					struct timeval t_tm;
					get_rtc_time(&t_tm, NULL);
					printf("cur=%lu ，cur=%lu", t_tm.tv_sec, t_tm.tv_sec);
					LOG_WRITE_UART(texBufArray);
					printf("\n\n");
					write(UART_PORT, texBufArray, sizeof(texBufArray));
				}
				//没有指令就应答
				else{
					write(UART_PORT, backBufArray, sizeof(texBufArray));
				}
			}
		}
		else{
			struct timeval nodata_time;
			get_rtc_time(&nodata_time, NULL);
			//测试
			//printf("rev=%lu,rev=%lu,", rev_time.tv_sec, rev_time.tv_sec);
			//printf("no_time=%lu,no_time=%lu\r\n", nodata_time.tv_sec, nodata_time.tv_sec);
			if ((nodata_time.tv_sec - rev_time.tv_sec) > 30){
				printf("over time\n");
				//发送错误信息
				memset(&tm, 0, sizeof(struct timespec));
				tm.tv_sec = 1;
				child_to_pthread_mq.is_err = 1;
				child_to_pthread_mq.err_no = 0xEC;
				mq_timedsend(childQueue, &child_to_pthread_mq, sizeof(struct child_to_pthread_mq_tag), 1, &tm);
			}


		
		}
	}



	return NULL;
}


//判断是否有定时任务
static void run_time_task()
{
	struct tm *tm_t;
	struct   timeval tm;
	get_rtc_time(&tm, NULL);
	tm_t = localtime(&tm.tv_sec);
	struct child_to_pthread_mq_tag child_to_pthread_mq_tag;
	int flag = 0;
	//接受数据
	flag = mq_receive(childQueue, &child_to_pthread_mq_tag, sizeof(struct child_to_pthread_mq_tag), NULL);
	if (flag > 0){
		yingxue_base.shezhi_temp = child_to_pthread_mq_tag.shezhi_temp;
		yingxue_base.state_show = child_to_pthread_mq_tag.state_show;
		yingxue_base.chushui_temp = child_to_pthread_mq_tag.chushui_temp;
		yingxue_base.jinshui_temp = child_to_pthread_mq_tag.jinshui_temp;
		yingxue_base.err_no = child_to_pthread_mq_tag.err_no;
		yingxue_base.is_err = child_to_pthread_mq_tag.is_err;
		
		yingxue_base.wind_rate = child_to_pthread_mq_tag.wind_rate;//[0][10]风机转速
		yingxue_base.fa_num = child_to_pthread_mq_tag.fa_num;//[2][0] 当前气源号
		yingxue_base.dh_num = child_to_pthread_mq_tag.dh_num;//[2][3]
		yingxue_base.ph_num = child_to_pthread_mq_tag.ph_num;//[2][1]
		yingxue_base.ne_num = child_to_pthread_mq_tag.ne_num;//[2][4]
		yingxue_base.huishui_temp_1 = child_to_pthread_mq_tag.huishui_temp;//[3][1] 回水温度 显示的回水温度
		/*LOG_CHILD_MQ(child_to_pthread_mq_tag);*/
	
	}
	if (yingxue_base.yure_mode == 0) return;
	//单巡航模式
	if (yingxue_base.yure_mode == 1){
		if (tm.tv_sec > yingxue_base.yure_endtime.tv_sec){
			//结束预热指令
			SEND_CLOSE_YURE_CMD();
			memset(&yingxue_base.yure_begtime, 0, sizeof(struct timeval));
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			yingxue_base.yure_mode = 0;
			yingxue_base.yure_state = 0;
		}
	}
	//预热定时任务
	else if (yingxue_base.yure_mode == 3){
		//这个时间是否需要启动
		if (*(yingxue_base.dingshi_list + tm_t->tm_hour) == 1){
			if (yingxue_base.yure_state == 0){
				//开始
				SEND_OPEN_YURE_CMD();
				yingxue_base.yure_state = 1;
			}
		}
		//这个时间 是否需要关闭
		else{
			if (yingxue_base.yure_state == 1){
				//结束
				SEND_CLOSE_YURE_CMD();
				yingxue_base.yure_state = 0;
			}
		}
	}
}


void SceneInit(void)
{
    struct mq_attr  attr;
    ITURotation     rot;

#ifdef CFG_LCD_ENABLE
    screenWidth     = ithLcdGetWidth();
    screenHeight    = ithLcdGetHeight();

    window          = SDL_CreateWindow("Display Control Board", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screenWidth, screenHeight, 0);
    if (!window)
    {
        printf("Couldn't create window: %s\n", SDL_GetError());
        return;
    }

    // init itu
    ituLcdInit();

    #ifdef CFG_M2D_ENABLE
    ituM2dInit();
    #else
    ituSWInit();
    #endif // CFG_M2D_ENABLE

    ituSceneInit(&theScene, NULL);

    #ifdef CFG_ENABLE_ROTATE
    //ituSceneSetRotation(&theScene, ITU_ROT_90, CFG_LCD_WIDTH, CFG_LCD_HEIGHT);//for screen image and touch direction
	ituSetRotation(ITU_ROT_270);//just for screen image
    #endif

    #ifdef CFG_VIDEO_ENABLE
    ituFrameFuncInit();
    #endif // CFG_VIDEO_ENABLE

    #ifdef CFG_PLAY_VIDEO_ON_BOOTING
        #ifndef CFG_BOOT_VIDEO_ENABLE_WINDOW_MODE
    rot = itv_get_rotation();

    if (rot == ITU_ROT_90 || rot == ITU_ROT_270)
        PlayVideo(0, 0, ithLcdGetHeight(), ithLcdGetWidth(), CFG_BOOT_VIDEO_BGCOLOR, CFG_BOOT_VIDEO_VOLUME);
    else
        PlayVideo(0, 0, ithLcdGetWidth(), ithLcdGetHeight(), CFG_BOOT_VIDEO_BGCOLOR, CFG_BOOT_VIDEO_VOLUME);
        #else
    PlayVideo(CFG_VIDEO_WINDOW_X_POS, CFG_VIDEO_WINDOW_Y_POS, CFG_VIDEO_WINDOW_WIDTH, CFG_VIDEO_WINDOW_HEIGHT, CFG_BOOT_VIDEO_BGCOLOR, CFG_BOOT_VIDEO_VOLUME);
        #endif
    #endif

    #ifdef CFG_PLAY_MJPEG_ON_BOOTING
        #ifndef CFG_BOOT_VIDEO_ENABLE_WINDOW_MODE
    rot = itv_get_rotation();

    if (rot == ITU_ROT_90 || rot == ITU_ROT_270)
        PlayMjpeg(0, 0, ithLcdGetHeight(), ithLcdGetWidth(), CFG_BOOT_VIDEO_BGCOLOR, 0);
    else
        PlayMjpeg(0, 0, ithLcdGetWidth(), ithLcdGetHeight(), CFG_BOOT_VIDEO_BGCOLOR, 0);
        #else
    PlayMjpeg(CFG_VIDEO_WINDOW_X_POS, CFG_VIDEO_WINDOW_Y_POS, CFG_VIDEO_WINDOW_WIDTH, CFG_VIDEO_WINDOW_HEIGHT, CFG_BOOT_VIDEO_BGCOLOR, 0);
        #endif
    #endif

    screenSurf = ituGetDisplaySurface();

    ituFtInit();
    ituFtLoadFont(0, CFG_PRIVATE_DRIVE ":/font/" CFG_FONT_FILENAME, ITU_GLYPH_8BPP);

    //ituSceneInit(&theScene, NULL);
    ituSceneSetFunctionTable(&theScene, actionFunctions);

    attr.mq_flags   = 0;
    attr.mq_maxmsg  = MAX_COMMAND_QUEUE_SIZE;
    attr.mq_msgsize = sizeof(Command);

    commandQueue    = mq_open("scene", O_CREAT | O_NONBLOCK, 0644, &attr);
    assert(commandQueue != -1);

    screenDistance  = sqrtf(screenWidth * screenWidth + screenHeight * screenHeight);

    isReady         = false;
    periodPerFrame  = MS_PER_FRAME;
#endif
}

void SceneExit(void)
{
#ifdef CFG_LCD_ENABLE
    mq_close(commandQueue);
    commandQueue = -1;

    resetScene();

    if (theScene.root)
    {
        ituSceneExit(&theScene);
    }
    ituFtExit();

    #ifdef CFG_M2D_ENABLE
    ituM2dExit();
        #ifdef CFG_VIDEO_ENABLE
    ituFrameFuncExit();
        #endif // CFG_VIDEO_ENABLE
    #else
    ituSWExit();
    #endif // CFG_M2D_ENABLE

    SDL_DestroyWindow(window);
#endif
}

void SceneLoad(void)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    isReady = false;

    cmd.id  = CMD_LOAD_SCENE;

    mq_send(commandQueue, (const char *)&cmd, sizeof(Command), 0);
}

void SceneGotoMainMenu(void)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    cmd.id = CMD_GOTO_MAINMENU;
    mq_send(commandQueue, (const char *)&cmd, sizeof(Command), 0);
}

void SceneChangeLanguage(void)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    cmd.id = CMD_CHANGE_LANG;
    mq_send(commandQueue, (const char *)&cmd, sizeof(Command), 0);
}

void ScenePredraw(int arg)
{
    Command cmd;

    if (commandQueue == -1)
        return;

    cmd.id = CMD_PREDRAW;
    mq_send(commandQueue, (const char *)&cmd, sizeof(Command), 0);
}

void SceneSetReady(bool ready)
{
    isReady = ready;
}

static void LoadScene(void)
{
#ifdef CFG_LCD_ENABLE
    uint32_t tick1, tick2;

    resetScene();
    if (theScene.root)
    {
        ituSceneExit(&theScene);
    }

    // load itu file
    tick1 = SDL_GetTicks();

    #ifdef CFG_LCD_MULTIPLE
    {
        char filepath[PATH_MAX];

        sprintf(filepath, CFG_PRIVATE_DRIVE ":/itu/%ux%u/ctrlboard.itu", ithLcdGetWidth(), ithLcdGetHeight());
        ituSceneLoadFileCore(&theScene, filepath);
    }
    #else
    ituSceneLoadFileCore(&theScene, CFG_PRIVATE_DRIVE ":/ctrlboard.itu");
    #endif // CFG_LCD_MULTIPLE

    tick2 = SDL_GetTicks();
    printf("itu loading time: %dms\n", tick2 - tick1);

    if (theConfig.lang != LANG_ENG)
        ituSceneUpdate(&theScene, ITU_EVENT_LANGUAGE, theConfig.lang, 0, 0);

    //ituSceneSetRotation(&theScene, ITU_ROT_90, ithLcdGetWidth(), ithLcdGetHeight());

    tick1       = tick2;

    #if defined(CFG_USB_MOUSE) || defined(_WIN32)
    cursorIcon  = ituSceneFindWidget(&theScene, "cursorIcon");
    if (cursorIcon)
    {
        ituWidgetSetVisible(cursorIcon, true);
    }
    #endif // defined(CFG_USB_MOUSE) || defined(_WIN32)

    tick2 = SDL_GetTicks();
    printf("itu init time: %dms\n", tick2 - tick1);

    ExternalProcessInit();
#endif
}

void SceneEnterVideoState(int timePerFrm)
{
    if (inVideoState)
    {
        return;
    }

#ifndef DISABLE_SWITCH_VIDEO_STATE
    #ifdef CFG_VIDEO_ENABLE
    ituFrameFuncInit();
    #endif
    screenSurf      = ituGetDisplaySurface();
    inVideoState    = true;
    if (timePerFrm != 0)
        periodPerFrame = timePerFrm;
#endif
}

void SceneLeaveVideoState(void)
{
    if (!inVideoState)
    {
        return;
    }

#ifndef DISABLE_SWITCH_VIDEO_STATE
    #ifdef CFG_VIDEO_ENABLE
    ituFrameFuncExit();
    #endif
    #ifdef CFG_LCD_ENABLE
    ituLcdInit();
    #endif
    #ifdef CFG_M2D_ENABLE
    ituM2dInit();
    #else
    ituSWInit();
    #endif

    screenSurf      = ituGetDisplaySurface();
    periodPerFrame  = MS_PER_FRAME;
#endif
    inVideoState    = false;
}

static void GotoMainMenu(void)
{
    ITULayer *mainMenuLayer = ituSceneFindWidget(&theScene, "mainMenuLayer");
    assert(mainMenuLayer);
    ituLayerGoto(mainMenuLayer);
}

static void ProcessCommand(void)
{
    Command cmd;

    while (mq_receive(commandQueue, (char *)&cmd, sizeof(Command), 0) > 0)
    {
        switch (cmd.id)
        {
        case CMD_LOAD_SCENE:
            LoadScene();
#if defined(CFG_PLAY_VIDEO_ON_BOOTING)
            ituScenePreDraw(&theScene, screenSurf);
            WaitPlayVideoFinish();
#elif defined(CFG_PLAY_MJPEG_ON_BOOTING)
            ituScenePreDraw(&theScene, screenSurf);
            WaitPlayMjpegFinish();
#endif
            ituSceneStart(&theScene);
            break;

        case CMD_GOTO_MAINMENU:
            GotoMainMenu();
            break;

        case CMD_CHANGE_LANG:
            ituSceneUpdate( &theScene,  ITU_EVENT_LANGUAGE, theConfig.lang, 0,  0);
            ituSceneUpdate( &theScene,  ITU_EVENT_LAYOUT,   0,              0,  0);
            break;

#if !defined(CFG_PLAY_VIDEO_ON_BOOTING) && !defined(CFG_PLAY_MJPEG_ON_BOOTING)
        case CMD_PREDRAW:
            ituScenePreDraw(&theScene, screenSurf);
            break;
#endif
        }
    }
}

static bool CheckQuitValue(void)
{
    if (quitValue)
    {
        if (ScreenSaverIsScreenSaving() && theConfig.screensaver_type == SCREENSAVER_BLANK)
            ScreenSaverRefresh();

        return true;
    }
    return false;
}

static void CheckStorage(void)
{
    StorageAction action = StorageCheck();

    switch (action)
    {
    case STORAGE_SD_INSERTED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_SD_INSERTED, NULL);
        break;

    case STORAGE_SD_REMOVED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_SD_REMOVED, NULL);
        break;

    case STORAGE_USB_INSERTED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_USB_INSERTED, NULL);
        break;

    case STORAGE_USB_REMOVED:
        ituSceneSendEvent(&theScene, EVENT_CUSTOM_USB_REMOVED, NULL);
        break;

    case STORAGE_USB_DEVICE_INSERTED:
        {
            ITULayer *usbDeviceModeLayer = ituSceneFindWidget(&theScene, "usbDeviceModeLayer");
            assert(usbDeviceModeLayer);

            ituLayerGoto(usbDeviceModeLayer);
        }
        break;

    case STORAGE_USB_DEVICE_REMOVED:
        {
            ITULayer *mainMenuLayer = ituSceneFindWidget(&theScene, "mainMenuLayer");
            assert(mainMenuLayer);

            ituLayerGoto(mainMenuLayer);
        }
        break;
    }
}

static void CheckExternal(void)
{
    ExternalEvent   ev;
    int             ret = ExternalReceive(&ev);

    if (ret)
    {
        ScreenSaverRefresh();
        ExternalProcessEvent(&ev);
    }
}

#if defined(CFG_USB_MOUSE) || defined(_WIN32)

static void CheckMouse(void)
{
    if (ioctl(ITP_DEVICE_USBMOUSE, ITP_IOCTL_IS_AVAIL, NULL))
    {
        if (!ituWidgetIsVisible(cursorIcon))
            ituWidgetSetVisible(cursorIcon, true);
    }
    else
    {
        if (ituWidgetIsVisible(cursorIcon))
            ituWidgetSetVisible(cursorIcon, false);
    }
}

#endif // defined(CFG_USB_MOUSE) || defined(_WIN32)

unsigned long
my_mktime(const unsigned int year0, const unsigned int mon0,
const unsigned int day, const unsigned int hour,
const unsigned int min, const unsigned int sec)
{
	unsigned int mon = mon0, year = year0;

	/* 1..12 -> 11,12,1..10 */
	if (0 >= (int)(mon -= 2)) {
		mon += 12;	/* Puts Feb last since it has leap day */
		year -= 1;
	}

	return ((((unsigned long)
		(year / 4 - year / 100 + year / 400 + 367 * mon / 12 + day) +
		year * 365 - 719499
		) * 24 + hour /* now have hours */
		) * 60 + min /* now have minutes */
		) * 60 + sec; /* finally seconds */
}





int SceneRun(void)
{


	yingxue_wifi_data_from_wifi();
	sleep(1000);

    SDL_Event   ev;
    int         delay, frames, lastx, lasty;
    uint32_t    tick, dblclk, lasttick, mouseDownTick;
    static bool first_screenSurf = true, sleepModeDoubleClick = false;
#if defined(CFG_POWER_WAKEUP_IR)
    static bool sleepModeIR = false;
#endif

    /* Watch keystrokes */
    dblclk = frames = lasttick = lastx = lasty = mouseDownTick = 0;
	//樱雪初始化
	//串口
#ifndef _WIN32
	itpRegisterDevice(UART_PORT, &UART_DEVICE);
	ioctl(UART_PORT, ITP_IOCTL_INIT, NULL);
	ioctl(UART_PORT, ITP_IOCTL_RESET, (void *)CFG_UART3_BAUDRATE);
#endif


	//消息队列
	struct mq_attr mq_uart_attr;
	mq_uart_attr.mq_flags = 0;
	mq_uart_attr.mq_maxmsg = 10;
	mq_uart_attr.mq_msgsize = sizeof(struct main_pthread_mq_tag);
	uartQueue = mq_open("scene_1", O_CREAT | O_NONBLOCK, 0644, &mq_uart_attr);

	struct mq_attr mq_child_attr;
	mq_child_attr.mq_flags = 0;
	mq_child_attr.mq_maxmsg = 20;

	mq_child_attr.mq_msgsize = sizeof(struct child_to_pthread_mq_tag);
	childQueue = mq_open("scene_2", O_CREAT | O_NONBLOCK, 0644, &mq_child_attr);


	//收发串口线程
	pthread_t task;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_create(&task, &attr, UartFunc, NULL);

	//环形队列
	create_chain_list(&chain_list);
	//现在的时间，长按
	struct timeval curtime;

	//图形元素初始化
	node_widget_init();
	//基础数据初始化
	yingxue_base_init();

    for (;;)
    {
        bool result = false;

        if (CheckQuitValue())
            break;

#ifdef CFG_LCD_ENABLE
        ProcessCommand();
#endif
        CheckExternal();
        CheckStorage();

#if defined(CFG_USB_MOUSE) || defined(_WIN32)
        if (cursorIcon)
            CheckMouse();
#endif     // defined(CFG_USB_MOUSE) || defined(_WIN32)

        tick = SDL_GetTicks();

#ifdef FPS_ENABLE
        frames++;
        if (tick - lasttick >= 1000)
        {
            
            frames      = 0;
            lasttick    = tick;
        }
#endif     // FPS_ENABLE

		//樱雪
		over_time_process();
		//判断是否定时任务需要发送数据，并且接受子线程的数据
		run_time_task();
		if (yingxue_base.is_err){

			if (yingxue_base.err_no == 0xEC){
				printf("show err\r\n");
				ituLayerGoto(ituSceneFindWidget(&theScene, "ECLayer"));
			}
		}



		//判断是否有错误代码
		/*if (yingxue_base.is_err){
			if (yingxue_base.err_no == 0xe0){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E0Layer"));
			}
			else if (yingxue_base.err_no == 0xe1){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E1Layer"));
			}
			else if (yingxue_base.err_no == 0xe2){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E2Layer"));
			}
			else if (yingxue_base.err_no == 0xe3){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E3Layer"));
			}
			else if (yingxue_base.err_no == 0xe4){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E4Layer"));
			}
			else if (yingxue_base.err_no == 0xe5){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E5Layer"));
			}
			else if (yingxue_base.err_no == 0xe6){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E6Layer"));
			}
			else if (yingxue_base.err_no == 0xe7){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E7Layer"));
			}
			else if (yingxue_base.err_no == 0xe8){
				ituLayerGoto(ituSceneFindWidget(&theScene, "E8Layer"));
			}
			else if (yingxue_base.err_no == 0xfd){
				ituLayerGoto(ituSceneFindWidget(&theScene, "H1Layer"));
			}
			else{
				ituLayerGoto(ituSceneFindWidget(&theScene, "ECLayer"));
			}

		}*/



#ifdef CFG_LCD_ENABLE
        while (SDL_PollEvent(&ev))
        {
			
			//按键处理事件
			key_down_process();
            switch (ev.type)
            {
            case SDL_KEYDOWN:
                ScreenSaverRefresh();
                result = ituSceneUpdate(&theScene, ITU_EVENT_KEYDOWN, ev.key.keysym.sym, 0, 0);
				printf("down_key=%lu\n", ev.key.keysym.sym);
                switch (ev.key.keysym.sym)
                {
				//真实控制板按键
					//case SDLK_UP:
				case 1073741884:
					if (curr_node_widget){
						curr_node_widget->updown_cb(curr_node_widget, 0);
					}
					
					break;
				case 1073741889:
					//case SDLK_DOWN:
					if (curr_node_widget){
						curr_node_widget->updown_cb(curr_node_widget, 1);
					}
					
					break;
				//确定
				case 1073741883:
					if (curr_node_widget){
						get_rtc_time(&curtime, NULL);
					}
					break;
				//关机
				case 1073741885:
					get_rtc_time(&curtime, NULL);
					break;
				//键盘按键
                case SDLK_UP:
					curr_node_widget->updown_cb(curr_node_widget, 0);
                    break;

                case SDLK_DOWN:
					curr_node_widget->updown_cb(curr_node_widget, 1);
                    break;
				case 13:
					
					curr_node_widget->confirm_cb(curr_node_widget, 1);
					break;

                case SDLK_LEFT:
                    //ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY2, NULL);
					printf("curr=%s\n", curr_node_widget->name);
                    break;

                case SDLK_RIGHT:
                    //长按
					if (curr_node_widget){
						if (curr_node_widget->long_press_cb){
							curr_node_widget->long_press_cb(curr_node_widget, NULL);
						}
					}
					
                    break;

                case SDLK_INSERT:
                    break;

    #ifdef _WIN32
                case SDLK_e:
                    result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHPINCH, 20, 30, 40);
                    break;

                case SDLK_f:
                    {
                        ITULayer *usbDeviceModeLayer = ituSceneFindWidget(&theScene, "usbDeviceModeLayer");
                        assert(usbDeviceModeLayer);

                        ituLayerGoto(usbDeviceModeLayer);
                    }
                    break;

                case SDLK_g:
                    {
                        ExternalEvent ev;

                        ev.type = EXTERNAL_SHOW_MSG;
                        strcpy(ev.buf1, "test");

                        ScreenSaverRefresh();
                        ExternalProcessEvent(&ev);
                    }
                    break;

    #endif          // _WIN32
                }
                if (result && !ScreenIsOff() && !StorageIsInUsbDeviceMode())
                    AudioPlayKeySound();

                break;

            case SDL_KEYUP:
                result = ituSceneUpdate(&theScene, ITU_EVENT_KEYUP, ev.key.keysym.sym, 0, 0);
				printf("keyup_key=%lu\n", ev.key.keysym.sym);
				unsigned long t_curr = 0;
				struct timeval t_time = { 0 };
				switch (ev.key.keysym.sym)
				{
				//放开后是否长按
				case 1073741883:

					if (curr_node_widget){
						LONG_PRESS_TIME(t_time, curtime, t_curr);
						if (t_curr >= 2){
							if (curr_node_widget->long_press_cb)
								curr_node_widget->long_press_cb(curr_node_widget, 1);
						}
						else{
							curr_node_widget->confirm_cb(curr_node_widget, 2);
						}
					}
					break;
					//放开关机长按
				case 1073741885:
					LONG_PRESS_TIME(t_time, curtime, t_curr);
					//长按
					if (t_curr >= 2){
						if (yingxue_base.run_state == 1){
							yingxue_base.run_state = 2;
						}
						else{
							yingxue_base.run_state = 1;
						}
						ituLayerGoto(ituSceneFindWidget(&theScene, "welcom"));
					}
					break;
				case 1073741886:
					//出厂设置
					if (curr_node_widget){
						ituLayerGoto(ituSceneFindWidget(&theScene, "Layer1"));
					}
					
					break;
				}

                break;

            case SDL_MOUSEMOTION:
                ScreenSaverRefresh();
    #if defined(CFG_USB_MOUSE) || defined(_WIN32)
                if (cursorIcon)
                {
                    ituWidgetSetX(cursorIcon, ev.button.x);
                    ituWidgetSetY(cursorIcon, ev.button.y);
                    ituWidgetSetDirty(cursorIcon, true);
                    //printf("mouse: move %d, %d\n", ev.button.x, ev.button.y);
                }
    #endif             // defined(CFG_USB_MOUSE) || defined(_WIN32)
                result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEMOVE, ev.button.button, ev.button.x, ev.button.y);
                break;

            case SDL_MOUSEBUTTONDOWN:
                ScreenSaverRefresh();
                printf("mouse: down %d, %d\n", ev.button.x, ev.button.y);
                {
                    mouseDownTick = SDL_GetTicks();
    #ifdef DOUBLE_KEY_ENABLE
                    if (mouseDownTick - dblclk <= 200)
                    {
                        result  = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOUBLECLICK, ev.button.button, ev.button.x, ev.button.y);
                        dblclk  = 0;
                    }
                    else
    #endif             // DOUBLE_KEY_ENABLE
                    {
                        result  = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOWN, ev.button.button, ev.button.x, ev.button.y);
                        dblclk  = mouseDownTick;
                        lastx   = ev.button.x;
                        lasty   = ev.button.y;
                    }
                    if (result && !ScreenIsOff() && !StorageIsInUsbDeviceMode())
                        AudioPlayKeySound();

    #ifdef CFG_SCREENSHOT_ENABLE
                    if (ev.button.x < 50 && ev.button.y > CFG_LCD_HEIGHT - 50)
                        Screenshot(screenSurf);
    #endif             // CFG_SCREENSHOT_ENABLE
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (SDL_GetTicks() - dblclk <= 200)
                {
                    int xdiff   = abs(ev.button.x - lastx);
                    int ydiff   = abs(ev.button.y - lasty);

                    if (xdiff >= GESTURE_THRESHOLD && xdiff > ydiff)
                    {
                        if (ev.button.x > lastx)
                        {
                            printf("mouse: slide to right\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDERIGHT, xdiff, ev.button.x, ev.button.y);
                        }
                        else
                        {
                            printf("mouse: slide to left\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDELEFT, xdiff, ev.button.x, ev.button.y);
                        }
                    }
                    else if (ydiff >= GESTURE_THRESHOLD)
                    {
                        if (ev.button.y > lasty)
                        {
                            printf("mouse: slide to down\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEDOWN, ydiff, ev.button.x, ev.button.y);
                        }
                        else
                        {
                            printf("mouse: slide to up\n");
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEUP, ydiff, ev.button.x, ev.button.y);
                        }
                    }
                }
                result          |= ituSceneUpdate(&theScene, ITU_EVENT_MOUSEUP, ev.button.button, ev.button.x, ev.button.y);
                mouseDownTick   = 0;
                break;

            case SDL_FINGERMOTION:
                ScreenSaverRefresh();
                printf("touch: move %d, %d\n", ev.tfinger.x, ev.tfinger.y);
                result = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEMOVE, 1, ev.tfinger.x, ev.tfinger.y);
                break;

            case SDL_FINGERDOWN:
                ScreenSaverRefresh();
                printf("touch: down %d, %d\n", ev.tfinger.x, ev.tfinger.y);
                {
                    mouseDownTick = SDL_GetTicks();
    #ifdef DOUBLE_KEY_ENABLE
        #ifdef CFG_POWER_WAKEUP_DOUBLE_CLICK_INTERVAL
                    if (mouseDownTick - dblclk <= CFG_POWER_WAKEUP_DOUBLE_CLICK_INTERVAL)
        #else
                    if (mouseDownTick - dblclk <= 200)
        #endif
                    {
                        printf("double touch!\n");
                        if (sleepModeDoubleClick)
                        {
                            ScreenSetDoubleClick();
                            ScreenSaverRefresh();
                            sleepModeDoubleClick = false;
                        }
                        result  = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOUBLECLICK, 1, ev.tfinger.x, ev.tfinger.y);
                        dblclk  = mouseDownTick = 0;
                    }
                    else
    #endif             // DOUBLE_KEY_ENABLE
                    {
                        result  = ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOWN, 1, ev.tfinger.x, ev.tfinger.y);
                        dblclk  = mouseDownTick;
                        lastx   = ev.tfinger.x;
                        lasty   = ev.tfinger.y;
                    }
                    if (result && !ScreenIsOff() && !StorageIsInUsbDeviceMode())
                        AudioPlayKeySound();

    #ifdef CFG_SCREENSHOT_ENABLE
                    if (ev.tfinger.x < 50 && ev.tfinger.y > CFG_LCD_HEIGHT - 50)
                        Screenshot(screenSurf);
    #endif             // CFG_SCREENSHOT_ENABLE
                       //if (ev.tfinger.x < 50 && ev.tfinger.y > CFG_LCD_HEIGHT - 50)
                       //    SceneQuit(QUIT_UPGRADE_WEB);
                }
                break;

            case SDL_FINGERUP:
                printf("touch: up %d, %d\n", ev.tfinger.x, ev.tfinger.y);
                if (SDL_GetTicks() - dblclk <= 300)
                {
                    int xdiff   = abs(ev.tfinger.x - lastx);
                    int ydiff   = abs(ev.tfinger.y - lasty);

                    if (xdiff >= GESTURE_THRESHOLD && xdiff > ydiff)
                    {
                        if (ev.tfinger.x > lastx)
                        {
                            printf("touch: slide to right %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDERIGHT, xdiff, ev.tfinger.x, ev.tfinger.y);
                        }
                        else
                        {
                            printf("touch: slide to left %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDELEFT, xdiff, ev.tfinger.x, ev.tfinger.y);
                        }
                    }
                    else if (ydiff >= GESTURE_THRESHOLD)
                    {
                        if (ev.tfinger.y > lasty)
                        {
                            printf("touch: slide to down %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEDOWN, ydiff, ev.tfinger.x, ev.tfinger.y);
                        }
                        else
                        {
                            printf("touch: slide to up %d %d\n", ev.tfinger.x, ev.tfinger.y);
                            result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHSLIDEUP, ydiff, ev.tfinger.x, ev.tfinger.y);
                        }
                    }
                }
                result          |= ituSceneUpdate(&theScene, ITU_EVENT_MOUSEUP, 1, ev.tfinger.x, ev.tfinger.y);
                mouseDownTick   = 0;
                break;

            case SDL_MULTIGESTURE:
                printf("touch: multi %d, %d\n", ev.mgesture.x, ev.mgesture.y);
                if (ev.mgesture.dDist > 0.0f)
                {
                    int dist    = (int)(screenDistance * ev.mgesture.dDist);
                    int x       = (int)(screenWidth * ev.mgesture.x);
                    int y       = (int)(screenHeight * ev.mgesture.y);
                    result |= ituSceneUpdate(&theScene, ITU_EVENT_TOUCHPINCH, dist, x, y);
                }
                break;
            }
        }
        if (!ScreenIsOff())
        {
            if (mouseDownTick > 0 && (SDL_GetTicks() - mouseDownTick >= MOUSEDOWN_LONGPRESS_DELAY))
            {
                printf("long press: %d %d\n", lastx, lasty);
                result          |= ituSceneUpdate(&theScene, ITU_EVENT_MOUSELONGPRESS, 1, lastx, lasty);
                mouseDownTick   = 0;
            }
            result |= ituSceneUpdate(&theScene, ITU_EVENT_TIMER, 0, 0, 0);
            //printf("%d\n", result);
            if (result)
            {
                ituSceneDraw(&theScene, screenSurf);
                ituFlip(screenSurf);
                if (first_screenSurf)
                {
                    ScreenSetBrightness(theConfig.brightness);
                    first_screenSurf = false;
                }
            }

            if (theConfig.screensaver_type != SCREENSAVER_NONE &&
                ScreenSaverCheck())
            {
                ituSceneSendEvent(&theScene, EVENT_CUSTOM_SCREENSAVER, "0");

                if (theConfig.screensaver_type == SCREENSAVER_BLANK)
                {
                    // have a change to flush action commands
                    ituSceneUpdate(&theScene, ITU_EVENT_TIMER, 0, 0, 0);

                    // draw black screen
                    ituSceneDraw(&theScene, screenSurf);
                    ituFlip(screenSurf);

                    ScreenOff();

    #if defined(CFG_POWER_WAKEUP_IR)
                    sleepModeIR             = true;
    #endif
    #if defined(CFG_POWER_WAKEUP_TOUCH_DOUBLE_CLICK)
                    sleepModeDoubleClick    = true;
    #endif
                }
            }
        }

    #if defined(CFG_POWER_WAKEUP_IR)
        if (ScreenIsOff() && sleepModeIR)
        {
            printf("Wake up by remote IR!\n");
            ScreenSaverRefresh();
            ituSceneUpdate(&theScene, ITU_EVENT_MOUSEDOWN, 1, 0, 0);
            ituSceneDraw(&theScene, screenSurf);
            ituFlip(screenSurf);
            sleepModeIR = false;
        }
    #endif

        if (sleepModeDoubleClick)
        {
            if (theConfig.screensaver_type != SCREENSAVER_NONE &&
                ScreenSaverCheckForDoubleClick())
            {
                if (theConfig.screensaver_type == SCREENSAVER_BLANK)
                    ScreenOffContinue();
            }
        }
#endif
        delay = periodPerFrame - (SDL_GetTicks() - tick);
        //printf("scene loop delay=%d\n", delay);
        if (delay > 0)
        {
            SDL_Delay(delay);
        }
        else
            sched_yield();
    }

    return quitValue;
}

void SceneQuit(QuitValue value)
{
    quitValue = value;
}

QuitValue SceneGetQuitValue(void)
{
    return quitValue;
}