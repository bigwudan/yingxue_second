#ifndef YINGXUE_WIFI_H
#define YINGXUE_WIFI_H
#include <stdint.h>
#define WIFI 1

//最大缓存数
#define MAX_CACHE_NUM 255
#define MAX_FRAME_NUM 255

#define CLEAN_WIFI_CACHE(x) do{x->idx = 0; x->data_len = 0;}while(0)

//得到验证
#define GET_CHECK_VAL(wifi_cache, check_no)  do{ \
  for (int i = 0; i < wifi_cache->idx - 1; i++){ \
	check_no = (uint8_t)((check_no + wifi_cache->data[i]) & 0xff); \
  }    }while(0)

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


#endif