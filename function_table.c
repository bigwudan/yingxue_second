#include "ite/itu.h"

//ӣѩ
//ÿ������ҳ���ʼ��
extern bool YX_MenuOnEnter(ITUWidget* widget, char* param);

//��ҳ�涨ʱ����
//extern bool MainLayerOnTimer(ITUWidget* widget, char* param);

//��������
//extern bool WelcomeOnTimer(ITUWidget* widget, char* param);


//������
extern bool ERROnTimer(ITUWidget* widget, char* param);


ITUActionFunction actionFunctions[] =
{
	"YX_MenuOnEnter", YX_MenuOnEnter,
	//"MainLayerOnTimer", MainLayerOnTimer,
	//"WelcomeOnTimer", WelcomeOnTimer,
	"ERROnTimer", ERROnTimer,
	NULL, NULL
};
