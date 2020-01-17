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

//樱雪基础数据
struct yingxue_base_tag{
	unsigned char adjust_temp_state; // 0调整温度状态 1正在调整 2闪烁 3解除锁定，可以上下移动
	unsigned char shezhi_temp; //设置温度
	unsigned char run_state; //0第一次上电 1开机 2关机
	unsigned char state_show;//第0位 有水  第1位 风机 第2位 火焰 第3位 风压
	unsigned char shizhe_temp;//设置温度
	unsigned char chushui_temp;//出水温度
	unsigned char jinshui_temp;//进水温度
	unsigned char yure_mode; //预热模式 0无模式 1单巡航模式 2 全天候模式 3 预约模式
	struct timeval yure_begtime; //预热开始时间
	struct timeval yure_endtime; //预热结束时间
	unsigned char err_no;//错误代码
	unsigned char is_err;//是否故障
	
};

//获取当前时间
extern int get_rtc_time(struct  timeval *dst, unsigned char *zone);

#ifdef __cplusplus
}
#endif

#endif /* SCENE_H */
/** @} */ // end of ctrlboard