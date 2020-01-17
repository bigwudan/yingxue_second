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

//#define FPS_ENABLE
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

//ӣѩ

//�Ƿ��Ѿ�����ʱ 0 δ���� 1 �Ѿ�����
unsigned char is_deal_over_time;

//ӣѩȫ������
struct yingxue_base_tag yingxue_base;

//��ǰѡ�еĿռ�
struct node_widget *curr_node_widget;


//������
struct node_widget mainlayer_0;
struct node_widget mainlayer_1;
struct node_widget mainlayer_2;

//Ԥ�Ƚ���
struct node_widget yureLayer_0;
struct node_widget yureLayer_1;
struct node_widget yureLayer_2;
struct node_widget yureLayer_3;
struct node_widget yureLayer_4;
struct node_widget yureLayer_5;



//Ԥ��ʱ��ҳ��
struct node_widget yureshijian_widget_0; //Ԥ��ʱ����ƿؼ�1
struct node_widget yureshijian_widget_num_0; //Ԥ��ʱ����ƿؼ�2
struct node_widget yureshijian_widget_num_1; //Ԥ��ʱ����ƿؼ�3
struct node_widget yureshijian_widget_num_2; //Ԥ��ʱ����ƿؼ�4
struct node_widget yureshijian_widget_num_3; //Ԥ��ʱ����ƿؼ�5
struct node_widget yureshijian_widget_num_4; //Ԥ��ʱ����ƿؼ�6
struct node_widget yureshijian_widget_num_5; //Ԥ��ʱ����ƿؼ�2
struct node_widget yureshijian_widget_num_6; //Ԥ��ʱ����ƿؼ�3
struct node_widget yureshijian_widget_num_7; //Ԥ��ʱ����ƿؼ�4
struct node_widget yureshijian_widget_num_8; //Ԥ��ʱ����ƿؼ�5
struct node_widget yureshijian_widget_num_9; //Ԥ��ʱ����ƿؼ�6
struct node_widget yureshijian_widget_num_10; //Ԥ��ʱ����ƿؼ�2
struct node_widget yureshijian_widget_num_11; //Ԥ��ʱ����ƿؼ�3
struct node_widget yureshijian_widget_num_12; //Ԥ��ʱ����ƿؼ�4
struct node_widget yureshijian_widget_num_13; //Ԥ��ʱ����ƿؼ�5
struct node_widget yureshijian_widget_num_14; //Ԥ��ʱ����ƿؼ�6
struct node_widget yureshijian_widget_num_15; //Ԥ��ʱ����ƿؼ�2
struct node_widget yureshijian_widget_num_16; //Ԥ��ʱ����ƿؼ�3
struct node_widget yureshijian_widget_num_17; //Ԥ��ʱ����ƿؼ�4
struct node_widget yureshijian_widget_num_18; //Ԥ��ʱ����ƿؼ�5
struct node_widget yureshijian_widget_num_19; //Ԥ��ʱ����ƿؼ�6
struct node_widget yureshijian_widget_num_20; //Ԥ��ʱ����ƿؼ�6
struct node_widget yureshijian_widget_num_21; //Ԥ��ʱ����ƿؼ�6
struct node_widget yureshijian_widget_num_22; //Ԥ��ʱ����ƿؼ�6
struct node_widget yureshijian_widget_num_23; //Ԥ��ʱ����ƿؼ�6

//ԤԼʱ��
struct node_widget yureshezhiLayer_0;
struct node_widget yureshezhiLayer_1;
struct node_widget yureshezhiLayer_2;
struct node_widget yureshezhiLayer_3;

//ģʽ
struct node_widget moshiLayer_0;
struct node_widget moshiLayer_1;
struct node_widget moshiLayer_2;
struct node_widget moshiLayer_3;
struct node_widget moshiLayer_4;

//��ˮģʽ
struct node_widget chushui_0;
struct node_widget chushui_1;
struct node_widget chushui_2;

//���һ�λ��ʱ��
struct timeval last_down_time;



//������һ��ԤԼ��ʱ��
void calcNextYure(int *beg, int *end)
{
	//����ʱ��
	struct timeval curr_time;
	struct tm *t_tm;
	unsigned char cur_hour;
	unsigned char num;
	*beg = 0;
	*end = 0;
	get_rtc_time(&curr_time, NULL);
	t_tm = localtime(&curr_time);
	cur_hour = t_tm->tm_hour;
	//��ʼ�Ƿ��ҵ�
	unsigned char is_beg = 0;

	//����ǰ�����û���ҵ����¿�ʼ��
	for (int i = 0; i < 2; i++){
		if (i == 0){
			num = cur_hour;
		}
		else{
			num = 0;
		}
		for (int j = num; j < 24; j++)
		{
			//����ʱ��
			if (*(yingxue_base.dingshi_list + j) == 1){
				//��ʼ����
				if (is_beg == 0){
					*beg = j;
					is_beg = 1;
				}
				//��������ʱ��һֱ����
				else if (is_beg == 1){
					*end = j;
				}
			}
			else{
				//����жϲ���������
				if (*beg != 0) break;
			}
		}
		if (*beg != 0) break;
	}

	return;
}


//�õ���ǰʱ���
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

//ͨ�������ƶ�
static void command_widget_up_down(struct node_widget *t_node_widget)
{
	struct ITUWidget *t_widget = NULL;
	//���֮ǰ�Ŀؼ�����Ҫ�����任������
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

	//������ڵĿؼ�����Ҫ�任����
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

		//ԭ���Ŀؼ�ȥ���߿�
		//ԭ���ؼ���radio
		if (strcmp(curr_node_widget->focus_back_name, "radio") == 0){
			t_widget = ituSceneFindWidget(&theScene, curr_node_widget->name);
			ituWidgetSetActive(t_widget, false);
		}
		//�ؼ���ͨ
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
		//���ص�ǰ�ؼ�ѡ��״̬
		t_widget = ituSceneFindWidget(&theScene, curr_node_widget->focus_back_name);
		ituWidgetSetVisible(t_widget, false);
		//��ʾ��ǰ�Ŀؼ�
		t_widget = ituSceneFindWidget(&theScene, t_node_widget->focus_back_name);
		ituWidgetSetVisible(t_widget, true);
		curr_node_widget = t_node_widget;
	}

}


//��ҳ��ȷ���ص��¼�
//@param widget ����ؼ�
//@param state ����
static void main_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	
	//ֻ����0״̬����һ��������µ���
	if (yingxue_base.adjust_temp_state == 0){
		yingxue_base.adjust_temp_state = 3;
	}
	//ֻ����״̬3�ſ��Խ���
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

//��ҳ�����»ص��¼�
static void main_widget_up_down_cb(struct node_widget *widget, unsigned char state)
{
	struct node_widget *t_node_widget = NULL;
	struct ITUWidget *t_widget = NULL;
	char t_buf[20] = { 0 };
	//ֻ��״̬3�ſ������µ���
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
		//�������µ����¶�
		if (yingxue_base.adjust_temp_state == 0 || yingxue_base.adjust_temp_state == 1){
			static int count_idx;
			yingxue_base.adjust_temp_state = 1;
			count_idx = yingxue_base.shezhi_temp;
			if (state == 0){
				count_idx += 1;
			}
			else{
				count_idx -= 1;
			}
			t_widget = ituSceneFindWidget(&theScene, "Text17");
			sprintf(t_buf, "%d", count_idx);
			yingxue_base.shezhi_temp = count_idx;
			ituTextSetString(t_widget, t_buf);
		}
	}
}

//Ԥ��ҳ��ȷ��
static void yure_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	if (strcmp(widget->name, "BackgroundButton47") == 0){
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//Background27
	else if (strcmp(widget->name, "BackgroundButton14") == 0){
		//����Ѳ�� �����ڵ�2��Сʱ����
		//ͬһ����ȡ��
		if (yingxue_base.yure_mode == 1){
			yingxue_base.yure_mode = 0;
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//����ȡ��
			
		
		}
		else{
			//���Ϳ�ʼ
			
			yingxue_base.yure_mode = 1;
			get_rtc_time(&yingxue_base.yure_begtime, NULL);
			//2��Сʱ 
			yingxue_base.yure_endtime.tv_sec = yingxue_base.yure_begtime.tv_sec + 60 * 60 * 2;


		}
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	else if (strcmp(widget->name, "BackgroundButton19") == 0){
		//ȫ���ģʽ��
		//�������ӵ��Ѳ��ģʽ�����̿�ʼ������һ�δ������Ԥ���е�����2��ѭ��Ԥ�ȡ�
		if (yingxue_base.yure_mode == 2){
			yingxue_base.yure_mode = 0;
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//����ȡ��
		}
		else{
			yingxue_base.yure_mode = 2;
			get_rtc_time(&yingxue_base.yure_begtime, NULL);
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//����Ԥ�ȿ�ʼ
		}
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//BackgroundButton20
	else if (strcmp(widget->name, "BackgroundButton20") == 0){
		/*
		ԤԼģʽ��
		��������ʱʱ�䵽�����һ�δ������Ԥ���е�����2��ѭ��Ԥ�ȡ���
		�������Զ��ӳ�1��Сʱ������һ�δ������TFT�ڷ��� Ԥ������  0- Ԥ�ȹر��ǰ�
		*/
		if (yingxue_base.yure_mode == 3){
			yingxue_base.yure_mode = 0;
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//����ȡ��
		}
		else{
			yingxue_base.yure_mode = 3;
		}
		t_widget = ituSceneFindWidget(&theScene, "MainLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//����Ԥ��ʱ��
	else if (strcmp(widget->name, "BackgroundButton2") == 0){
		//����ԤԼʱ��
		t_widget = ituSceneFindWidget(&theScene, "yureshijianLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//����Ԥ�Ȼ�ˮ�¶Ⱥͱ���ʱ��
	else if (strcmp(widget->name, "BackgroundButton21") == 0){
		t_widget = ituSceneFindWidget(&theScene, "yureshezhiLayer");
		ituLayerGoto((ITULayer *)t_widget);

	}

}

//Ԥ��ҳ�������ƶ�
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

//Ԥ��ʱ�������ƶ�
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

//Ԥ�ȶ�ʱȷ��
static void yure_settime_widget_confirm_cb(struct node_widget *widget, unsigned char state)
{
	ITUWidget *t_widget = NULL;
	int value = widget->value;
	if (strcmp(widget->name, "BackgroundButton65") == 0){
		t_widget = ituSceneFindWidget(&theScene, "yureLayer");
		ituLayerGoto((ITULayer *)t_widget);
	}
	//��������
	else if (strcmp(widget->focus_back_name, "radio") == 0){
		t_widget = ituSceneFindWidget(&theScene, widget->name);
		//����Ѿ��ǵ��״̬��ȡ��״̬
		if (ituRadioBoxIsChecked(t_widget)){
			ituCheckBoxSetChecked((ITUCheckBox *)t_widget, false);
			*(yingxue_base.dingshi_list + value) = 0;
		}
		//���ǵ��״̬��������״̬
		else{
			ituCheckBoxSetChecked((ITUCheckBox *)t_widget, true);
			*(yingxue_base.dingshi_list + value) = 1;
		}
	}
}


//����ʱ�䷢��ʱ�Ĵ����¼�
static void key_down_process()
{
	//���һ�ε�ʱ��
	get_rtc_time(&last_down_time, NULL);
	//���´����ʶ
	is_deal_over_time = 0;
}

//����ģ���ʼ��
static void node_widget_init()
{
	curr_node_widget = NULL;
	//��ҳ��
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


	//Ԥ�Ƚ���
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

	//Ԥ�ȶ�ʱ
	yureshijian_widget_0.up = NULL;
	yureshijian_widget_0.down = &yureshijian_widget_num_0;
	yureshijian_widget_0.focus_back_name = "BackgroundButton30"; //ѡ��
	yureshijian_widget_0.name = "BackgroundButton65";//δѡ��
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

#if 0
	//ԤԼʱ��
	yureshezhiLayer_0.up = NULL;
	yureshezhiLayer_0.down = &yureshezhiLayer_1;
	yureshezhiLayer_0.focus_back_name = "BackgroundButton85";
	yureshezhiLayer_0.name = "BackgroundButton60";
	yureshezhiLayer_0.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_0.updown_cb = node_widget_up_down;

	yureshezhiLayer_1.up = &yureshezhiLayer_0;
	yureshezhiLayer_1.down = &yureshezhiLayer_2;
	yureshezhiLayer_1.focus_back_name = "Background37";
	yureshezhiLayer_1.checked_back_name = "Background45";
	yureshezhiLayer_1.name = "Background2";
	yureshezhiLayer_1.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_1.updown_cb = node_widget_up_down;
	yureshezhiLayer_1.type = 1;

	yureshezhiLayer_2.up = &yureshezhiLayer_1;
	yureshezhiLayer_2.down = &yureshezhiLayer_3;
	yureshezhiLayer_2.focus_back_name = "Background33";
	yureshezhiLayer_2.checked_back_name = "Background105";
	yureshezhiLayer_2.name = "Background3";
	yureshezhiLayer_2.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_2.updown_cb = node_widget_up_down;
	yureshezhiLayer_2.type = 1;


	yureshezhiLayer_3.up = &yureshezhiLayer_2;
	yureshezhiLayer_3.down = NULL;
	yureshezhiLayer_3.focus_back_name = "Background40";
	yureshezhiLayer_3.checked_back_name = "Background107";
	yureshezhiLayer_3.name = "Background4";
	yureshezhiLayer_3.confirm_cb = yure_yureshezhiLayer_widget_confirm_cb;
	yureshezhiLayer_3.updown_cb = node_widget_up_down;
	yureshezhiLayer_3.type = 1;


	//ģʽ
	moshiLayer_0.up = NULL;
	moshiLayer_0.down = &moshiLayer_1;
	moshiLayer_0.focus_back_name = "BackgroundButton33";
	moshiLayer_0.name = "BackgroundButton68";
	moshiLayer_0.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_0.updown_cb = node_widget_up_down;

	moshiLayer_1.up = &moshiLayer_0;
	moshiLayer_1.down = &moshiLayer_2;
	moshiLayer_1.focus_back_name = "moshi_BackgroundButton80";
	moshiLayer_1.name = "moshi_BackgroundButton10";
	moshiLayer_1.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_1.updown_cb = node_widget_up_down;
	moshiLayer_1.long_press_cb = moshi_widget_longpress_cb;

	moshiLayer_2.up = &moshiLayer_1;
	moshiLayer_2.down = &moshiLayer_3;
	moshiLayer_2.focus_back_name = "moshi_BackgroundButton79";
	moshiLayer_2.name = "moshi_BackgroundButton11";
	moshiLayer_2.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_2.updown_cb = node_widget_up_down;
	moshiLayer_2.long_press_cb = moshi_widget_longpress_cb;

	moshiLayer_3.up = &moshiLayer_2;
	moshiLayer_3.down = &moshiLayer_4;
	moshiLayer_3.focus_back_name = "moshi_BackgroundButton81";
	moshiLayer_3.name = "moshi_BackgroundButton12";
	moshiLayer_3.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_3.updown_cb = node_widget_up_down;
	moshiLayer_3.long_press_cb = moshi_widget_longpress_cb;

	moshiLayer_4.up = &moshiLayer_3;
	moshiLayer_4.down = NULL;
	moshiLayer_4.focus_back_name = "moshi_BackgroundButton82";
	moshiLayer_4.name = "moshi_BackgroundButton13";
	moshiLayer_4.confirm_cb = moshi_widget_confirm_cb;
	moshiLayer_4.updown_cb = node_widget_up_down;
	moshiLayer_4.long_press_cb = moshi_widget_longpress_cb;

	//��ˮ
	chushui_0.up = NULL;
	chushui_0.down = &chushui_1;
	chushui_0.focus_back_name = "chushui_BackgroundButton7";
	chushui_0.name = "chushui_BackgroundButton73";
	chushui_0.confirm_cb = chushui_widget_confirm_cb;
	chushui_0.updown_cb = node_widget_up_down;

	chushui_1.up = &chushui_0;
	chushui_1.down = &chushui_2;
	chushui_1.focus_back_name = "chushui_Background37";
	chushui_1.checked_back_name = "chushui_Background45";
	chushui_1.name = "chushui_Background13";
	chushui_1.confirm_cb = chushui_widget_confirm_cb;
	chushui_1.updown_cb = node_widget_up_down;
	chushui_1.type = 1;

	chushui_2.up = &chushui_1;
	chushui_2.down = NULL;
	chushui_2.focus_back_name = "chushui_Background51";
	chushui_2.name = "chushui_BackgroundButton1";
	chushui_2.confirm_cb = chushui_widget_confirm_cb;
	chushui_2.updown_cb = node_widget_up_down;

#endif

	//��ʼһ�ΰ�����ʱ����
	key_down_process();
}
//�Ƿ���˸
extern char is_shake;

//��ʱ�Ĵ�����
static void over_time_process()
{
	struct timeval now_t = {0};
	get_rtc_time(&now_t, NULL);
	//�Ѿ������
	if (is_deal_over_time == 1) return;
	//��ε�ǰ��ʱ�����3��
	if (now_t.tv_sec > last_down_time.tv_sec + 3){
		memset(&last_down_time, 0, sizeof(struct timeval));
		//�Ѿ������
		is_deal_over_time = 1;
		//����ǵ����¶�,����˸
		if (yingxue_base.adjust_temp_state == 1){
			printf("beg share\n");
			is_shake = 1;
		}
		else{
			if (strcmp(curr_node_widget->name, "BackgroundButton3") != 0){
				ituLayerGoto(ituSceneFindWidget(&theScene, "MainLayer"));
			}
		}
	}
	return;
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
    ituSceneSetRotation(&theScene, ITU_ROT_90, CFG_LCD_WIDTH, CFG_LCD_HEIGHT);
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


int SceneRun(void)
{
    SDL_Event   ev;
    int         delay, frames, lastx, lasty;
    uint32_t    tick, dblclk, lasttick, mouseDownTick;
    static bool first_screenSurf = true, sleepModeDoubleClick = false;
#if defined(CFG_POWER_WAKEUP_IR)
    static bool sleepModeIR = false;
#endif

    /* Watch keystrokes */
    dblclk = frames = lasttick = lastx = lasty = mouseDownTick = 0;

	//ӣѩ��ʼ��
	node_widget_init();
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
            printf("fps: %d\n", frames);
            frames      = 0;
            lasttick    = tick;
        }
#endif     // FPS_ENABLE

		//ӣѩ
		over_time_process();

#ifdef CFG_LCD_ENABLE
        while (SDL_PollEvent(&ev))
        {
			//���������¼�
			key_down_process();
            switch (ev.type)
            {
            case SDL_KEYDOWN:
                ScreenSaverRefresh();
                result = ituSceneUpdate(&theScene, ITU_EVENT_KEYDOWN, ev.key.keysym.sym, 0, 0);
                switch (ev.key.keysym.sym)
                {
                case SDLK_UP:
					curr_node_widget->updown_cb(curr_node_widget, 0);
                    //ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY0, NULL);
                    break;

                case SDLK_DOWN:
					curr_node_widget->updown_cb(curr_node_widget, 1);
                    //ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY1, NULL);
                    break;
				case 13:
					//pcȷ��
					curr_node_widget->confirm_cb(curr_node_widget, 1);
					break;

                case SDLK_LEFT:
                    //ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY2, NULL);
					//�ػ�
					if (yingxue_base.run_state == 1){
						yingxue_base.run_state = 2;
					}
					else if (yingxue_base.run_state == 2){
						yingxue_base.run_state = 1;
					}
					ituLayerGoto(ituSceneFindWidget(&theScene, "welcom"));
                    break;
                case SDLK_RIGHT:
                    ituSceneSendEvent(&theScene, EVENT_CUSTOM_KEY3, NULL);
                    break;

                case SDLK_INSERT:
                    break;


                }
                if (result && !ScreenIsOff() && !StorageIsInUsbDeviceMode())
                    AudioPlayKeySound();

                break;

            case SDL_KEYUP:
                result = ituSceneUpdate(&theScene, ITU_EVENT_KEYUP, ev.key.keysym.sym, 0, 0);
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