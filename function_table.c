#include "ite/itu.h"

//樱雪
//每个进入页面初始化
extern bool YX_MenuOnEnter(ITUWidget* widget, char* param);

//主页面定时任务
//extern bool MainLayerOnTimer(ITUWidget* widget, char* param);

//开机画面
//extern bool WelcomeOnTimer(ITUWidget* widget, char* param);


//错误画面
extern bool ERROnTimer(ITUWidget* widget, char* param);


ITUActionFunction actionFunctions[] =
{
	"YX_MenuOnEnter", YX_MenuOnEnter,
	//"MainLayerOnTimer", MainLayerOnTimer,
	//"WelcomeOnTimer", WelcomeOnTimer,
	"ERROnTimer", ERROnTimer,
	NULL, NULL
};
