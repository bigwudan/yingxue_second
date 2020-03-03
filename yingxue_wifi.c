#include "yingxue_wifi.h"


//测试数据
//const uint8_t test_buf[] = { 0xfc, 0x00, 0x01, 0x08, 0x01, 0x06 };
//const uint8_t test_buf[] = { 0xfc, 0x00, 0x00, 0x00, 0xfc };



//const uint8_t test_buf[] = { 0xFC, 0x00, 0x2D, 0x02, 0x01, 0x06, 0x6D, 0x67, 0x62, 0x69, 0x74, 0x65, 0x20, 0x33, 0x30, 0x31, 0x31, 0x35, 0x32, 0x65, 0x33, 0x36, 0x33, 0x38, 0x62, 0x34, 0x39, 0x30, 0x31, 0x39, 0x62, 0x64, 0x31, 0x64, 0x35, 0x32, 0x64,
//0x35, 0x33, 0x32, 0x64, 0x38, 0x31, 0x38, 0x39, 0x01, 0x00, 0x00, 0x01, 0x98 };


//心跳fc 00 02 03 05 00 06



//定义一个缓存数据
struct wifi_cache_tag wifi_cache_g;
//定义一个接受到数据
struct wifi_frame_tag wifi_frame_g;

//初始化wifi
void 
yingxue_wifi_init()
{

	//初始化缓存数据
	memset(wifi_cache_g, 0, sizeof(struct wifi_cache_tag));
	//初始化接受数据
	memset(wifi_frame_g, 0, sizeof(struct wifi_frame_tag));


}

//从缓存中分析数据，并且验证正确性
//return 0错误 1正确
int 
yingxue_wifi_data_check(struct wifi_cache_tag *wifi_cache, struct wifi_frame_tag *wifi_frame)
{
	uint8_t *p_data = wifi_cache->data;
	uint16_t data_len = wifi_cache->data_len;
	uint8_t check_no = 0; //验证码
	uint8_t out_check_len = 0; //除去验证码的总长度
	//判断头是否是0xfc
	if (p_data[0] != 0xfc){
		CLEAN_WIFI_CACHE(wifi_cache);
		return 0;
	}
	//效验码是否正确

	GET_CHECK_VAL(wifi_cache, check_no);

	//验证码错误
	if (check_no != p_data[wifi_cache->idx - 1]){
		CLEAN_WIFI_CACHE(wifi_cache);
		return 0;
	}

	//得到一个完整的帧
	//得到命令
	wifi_frame->command = p_data[3];
	wifi_frame->data_len = data_len;

	//如何命令数据是0x00,就是ack
	if (wifi_frame->command != 0x00){
		memmove(wifi_frame->data, wifi_cache->data + 4, wifi_cache->data_len);
	}
	CLEAN_WIFI_CACHE(wifi_cache);
	return 1;
}


//串口接受数据
//@param 环形列表头
int 
yingxue_wifi_data_from_wifi()
{
	uint8_t rece_buf[20];
	uint8_t len = 0;
	uint8_t flag = 0;
#ifdef _WIN32
	//读出的数据进入环形队列
	//in_chain_list(p_chain_list, data);
	len = sizeof(test_buf);
	memmove(rece_buf, test_buf, len);
#else
#endif
	//如果接受到数据进入分析
	if (len){
		for (int i = 0; i < len; i++){
			//写入缓存
			wifi_cache_g.data[wifi_cache_g.idx] = rece_buf[i];
			wifi_cache_g.idx += 1;
			//写入数据长度
			if (wifi_cache_g.idx == 4){
				wifi_cache_g.data_len = (wifi_cache_g.data[1] << 8) | (wifi_cache_g.data[2]);
			}
			//数据读取完成,头+命令+长度+效验 = 5
			if (wifi_cache_g.idx == 5 + wifi_cache_g.data_len){
				//进入数据校正
				flag = yingxue_wifi_data_check(&wifi_cache_g, &wifi_frame_g);
			}
			//超过最大值舍去所有数据
			if (wifi_cache_g.idx >= MAX_CACHE_NUM){
				CLEAN_WIFI_CACHE((&wifi_cache_g));
			}
			//如果得到一个完整的帧，开始分析帧
			if (flag == 1){
				printf("frame ok\n");
			}
		
		}

	}

	return 0;

}
