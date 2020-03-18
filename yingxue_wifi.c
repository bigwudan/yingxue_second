#include "yingxue_wifi.h"
#include "scene.h"
#include <mqueue.h>
#include "ite/itp.h"

#ifdef _WIN32
//测试
extern mqd_t test_mq;


#endif
extern ITUScene  theScene;
extern struct yingxue_base_tag yingxue_base;
extern mqd_t toWifiQueue;
//测试数据
const uint8_t test_buf[] = { 0xfc, 0x00, 0x01, 0x08, 0x01, 0x06 };
//const uint8_t test_buf[] = { 0xfc, 0x00, 0x00, 0x00, 0xfc };


//设备信息上报
const uint8_t to_wifi_info[] = { 
	0xFC, 0x00, 0x2D, 0x02, 0x01, 0x06, 0x6D, 0x67, 0x62, 0x69, 
	0x74, 0x65, 0x20, 0x33, 0x30, 0x31, 0x31, 0x35, 0x32, 0x65,
	0x33, 0x36, 0x33, 0x38, 0x62, 0x34, 0x39, 0x30, 0x31, 0x39, 
	0x62, 0x64, 0x31, 0x64, 0x35, 0x32, 0x64, 0x35, 0x33, 0x32, 
	0x64, 0x38, 0x31, 0x38, 0x39, 0x01, 0x00, 0x00, 0x01, 0x98 };
//配网
const uint8_t to_wifi_net[] = { 0xfc, 0x00, 0x01, 0x08, 0x01, 0x06 };

//心跳fc 00 02 03 05 00 06



//定义一个缓存数据
struct wifi_cache_tag wifi_cache_g;
//定义一个接受到数据
struct wifi_frame_tag wifi_frame_g;
//定义一个wifi基础信息
struct wifi_base_tag wifi_base_g;

//初始化wifi
void 
yingxue_wifi_init()
{

	//初始化缓存数据
	memset(&wifi_cache_g, 0, sizeof(struct wifi_cache_tag));
	//初始化接受数据
	memset(&wifi_frame_g, 0, sizeof(struct wifi_frame_tag));
	//初始化wifi基础信息
	memset(&wifi_base_g, 0, sizeof(struct wifi_base_tag));


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

	//打印
/*	printf("wifi rec:");

	printf("cmd=0x%02X,data=", wifi_frame->command);
	for (int i = 0; i < wifi_frame->data_len; i++){
		printf("0x%02X ", wifi_frame->data[i]);
	}
	printf(" end\n");*/
	CLEAN_WIFI_CACHE(wifi_cache);
	return 1;
}


//串口接受数据
//@param 环形列表头
//return 0未获得一个完整的帧 1获得一个完整的帧
int 
yingxue_wifi_data_from_wifi()
{
	uint8_t rece_buf[60];
	uint8_t len = 0;
	uint8_t flag = 0;
#ifdef _WIN32
	//读出的数据进入环形队列
	//in_chain_list(p_chain_list, data);
	struct wifi_uart_mq_tag wifi_uart_mq;
	flag = mq_receive(test_mq, &wifi_uart_mq, sizeof(struct wifi_uart_mq_tag), NULL);
	if (flag > 0){
		len = wifi_uart_mq.len;
		memmove(rece_buf, wifi_uart_mq.data, len);
	}

#else
	//串口,读取一个字节
	len = read(UART_PORT_WIFI, rece_buf, 1);
#endif
	//如果接受到数据进入分析
	if (len > 0){
		for (int i = 0; i < len; i++){

			//是否是第一次遇到0xfc
			if ((wifi_cache_g.idx == 0 && rece_buf[i] == 0xfc) ||
				(wifi_cache_g.idx  > 0))
			{
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
					return 1;
				}
			}
		}
	}
	return 0;
}


//命令解析
int 
yingxue_wifi_process_command(struct wifi_frame_tag *wifi_frame)
{
	uint8_t data_len = wifi_frame->data[1];
	uint32_t command = 0;
	//属性ID 2个字节
	uint16_t command_id = (wifi_frame->data[2] << 8) | (wifi_frame->data[3] & 0xff);
	//大于4个字节的长度错误
	if (data_len > 4){
		return -1;
	}
	for (int i = 0; i < data_len; i++){
		command = (wifi_frame->data[4] << 8 * 3) | (wifi_frame->data[5] << 8 * 2) | (wifi_frame->data[6] << 8) | wifi_frame->data[7];
	}
	//开关
	if (command_id == WIFI_CTR_OPEN){
		//关机
		if (command == 0){
			yingxue_base.run_state = 2;
			SEND_OPEN_CMD();
		}
		//开机
		else{
			yingxue_base.run_state = 1;
			SEND_OPEN_CMD();
		}
		

		ituLayerGoto(ituSceneFindWidget(&theScene, "welcom"));

	}
	//选择模式0：常规模式1：果蔬模式2：ECO模式
	else if (command_id == WIFI_CTR_MODE){
		if (command == 0){
			yingxue_base.moshi_mode = 0;
		}
		else if (command == 1){
			yingxue_base.moshi_mode = 3;
		}
		else if (command == 2){
			yingxue_base.moshi_mode = 2;
		}
	}
	//无
	else if (command_id == WIFI_CTR_TEMP){

	}
	//无
	else if (command_id == WIFI_CTR_CHUSHUI){

	}
	//手动预约模式
	else if (command_id == WIFI_CTR_SHOUDONG){
		//无
		if (command == 0){

			yingxue_base.yure_mode = 0;
		}
		//单次循环
		else if (command == 1){
			yingxue_base.yure_mode = 1;
			get_rtc_time(&yingxue_base.yure_begtime, NULL);
			//2个小时 
			yingxue_base.yure_endtime.tv_sec = yingxue_base.yure_begtime.tv_sec + 60 * 60 * 2;
			SEND_OPEN_YURE_CMD();

		}
		//24小时循环
		else if (command == 2){
			yingxue_base.yure_mode = 2;
			SEND_OPEN_YURE_CMD();
		}


	}
	//设置回水温度

	else if (command_id == WIFI_CTR_HUISHUI){
		yingxue_base.huishui_temp = (uint8_t)command;
	
	}
	//无
	else if (command_id == WIFI_CTR_STATE){

	}
	//预热开关
	else if (command_id == WIFI_CTR_YUYUE_SW){
		if (yingxue_base.yure_mode == 3){
			yingxue_base.yure_mode = 0;
			memset(&yingxue_base.yure_endtime, 0, sizeof(struct timeval));
			//发送取消
			SEND_CLOSE_YURE_CMD();
		}
		else{
			yingxue_base.yure_mode = 3;
		}
	}
	else if (command_id == WIFI_CTR_YUTIME){
		for (int i = 0; i < 24; i++){
			if ((uint8_t)((command >> i) & 0x01) == 1){
			
				yingxue_base.dingshi_list[i] = 1;
			}
			else{
				yingxue_base.dingshi_list[i] = 0;
			}
		}

	}
	BACK_COMMAND_SUCCESS(command_id, 0x01);
}

//单片机回复数据
//@param cmd_state 命令
//@param state_id 状态ID
//@param data     上传状态下的数据
void yingxue_wifi_to_wifi(enum wifi_command_state cmd_state, uint16_t state_id, uint32_t data)
{
	struct wifi_uart_mq_tag wifi_uart_mq;
	struct timespec tm;
	memset(&tm, 0, sizeof(struct timespec));
	tm.tv_sec = 1;
	memset(&wifi_uart_mq, 0, sizeof(struct wifi_uart_mq_tag));
	//发送配网指令
	if (cmd_state == WIFI_CMD_EQUIP_UP){


		memmove(wifi_uart_mq.data, to_wifi_info, sizeof(to_wifi_info));
		wifi_uart_mq.len = sizeof(to_wifi_info);


	}
	//配网指令
	else if (cmd_state == WIFI_CMD_NET){
		
		memmove(wifi_uart_mq.data, to_wifi_net, sizeof(to_wifi_net));
		wifi_uart_mq.len = sizeof(to_wifi_net);

	}
	else if (cmd_state == WIFI_CMD_STATE_UP){
		wifi_uart_mq.data[0] = 0xfc;
		wifi_uart_mq.data[1] = 0x00;
		wifi_uart_mq.data[2] = 0x08;
		wifi_uart_mq.data[3] = 0x05;
		wifi_uart_mq.data[4] = 0x00;
		wifi_uart_mq.data[5] = 0x04;

		//熟悉id
		for (int i = 0; i < 2; i++){
			wifi_uart_mq.data[6 + i] = (uint8_t)((state_id >> (8 * (1 - i))) & 0xff);
			
		}
		//数据
		for (int i = 0; i < 4; i++){
			wifi_uart_mq.data[8 + i] = (uint8_t)((data >> (8 * (3 - i))) & 0xff);
		}
		for (int i = 0; i < 12; i++){
			wifi_uart_mq.data[12] = (uint8_t)((wifi_uart_mq.data[12] + wifi_uart_mq.data[i]) & 0xff);
		}
		wifi_uart_mq.data[12] = 0x73;
		wifi_uart_mq.len = 13;

	}
	else if (cmd_state == WIFI_CMD_STATE_CTR){
		wifi_uart_mq.data[0] = 0xfc;
		wifi_uart_mq.data[1] = 0x00;
		wifi_uart_mq.data[2] = 0x03;
		wifi_uart_mq.data[3] = 0x07;
		//熟悉id
		for (int i = 0; i < 2; i++){
			wifi_uart_mq.data[4 + i] = (uint8_t)((state_id >> (8 * (1 - i))) & 0xff);

		}
		wifi_uart_mq.data[6] = 0x01;
		wifi_uart_mq.data[7] = 0x8;
		wifi_uart_mq.len = 8;
	}
	//fc 00 00 00 fc
	else if (cmd_state == WIFI_CMD_STATE_OK){
		wifi_uart_mq.data[0] = 0xfc;
		wifi_uart_mq.data[1] = 0x00;
		wifi_uart_mq.data[2] = 0x00;
		wifi_uart_mq.data[3] = 0x00;
		wifi_uart_mq.data[4] = 0xfc;
		wifi_uart_mq.len = 5;
	}

	if (wifi_uart_mq.len != 0){
		mq_timedsend(toWifiQueue, &wifi_uart_mq, sizeof(struct wifi_uart_mq_tag), 1, &tm);
	}

}




//wifi模块任务
void
yingxue_wifi_task()
{
	int flag = 0;
	//获得串口数据
	flag = yingxue_wifi_data_from_wifi();

	if (flag == 1){
		//解析数据
		//是否是ack回复
		if (wifi_frame_g.command == WIFI_CMD_STATE_OK){
			
			if (wifi_base_g.run_state == 0){
				wifi_base_g.run_state = 1;
			}
			wifi_base_g.ack_state = 0;//ack已经回复
		}
		//心跳
		else if (wifi_frame_g.command == WIFI_CMD_HEART){
			wifi_base_g.online_state = wifi_frame_g.data[0];
		}
		//状态查询
		else if (wifi_frame_g.command == WIFI_CMD_STATE_QUERY){
			wifi_base_g.beg_upstate = 9;
			//回复ack
			yingxue_wifi_to_wifi(WIFI_CMD_STATE_OK, 0, 0);

		}
		//状态控制
		else if (wifi_frame_g.command == WIFI_CMD_STATE_CTR){
			yingxue_wifi_process_command(&wifi_frame_g);
		}
	
	}
	//运行发送任务
	yingxue_wifi_send_task();
}

//定时上报数据
static void
yingxue_wifi_upstate()
{
	uint32_t cmd_data = 0;//上传的数据到wifi
	//预约时间
	if (wifi_base_g.beg_upstate == 9){

		for (int i = 0; i < 24; i++){

			cmd_data = cmd_data | (yingxue_base.dingshi_list[i] << i);
		}
		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 109, cmd_data);
	}
	else if (wifi_base_g.beg_upstate == 8){
		if (yingxue_base.yure_mode == 3){
			cmd_data = 0x01;
		}
		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 108, cmd_data);
	}
	else if (wifi_base_g.beg_upstate == 7){
		//	unsigned char state_show;//第0位 有水  第1位 风机 第2位 火焰 第3位 风压
		cmd_data = yingxue_base.state_show & 0x07;
		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 107, cmd_data);
	}
	else if (wifi_base_g.beg_upstate == 6){

		cmd_data = yingxue_base.huishui_temp;
		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 106, cmd_data);
	}
	else if (wifi_base_g.beg_upstate == 5){
		//unsigned char yure_mode; //预热模式 0无模式 1单巡航模式 2 全天候模式 3 预约模式
		if (yingxue_base.yure_mode == 1){
			cmd_data = 1;
		}
		else if (yingxue_base.yure_mode == 2){
			cmd_data = 2;
		}

		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 105, cmd_data);
	}
	//无
	else if (wifi_base_g.beg_upstate == 4){

	}
	else if (wifi_base_g.beg_upstate == 3){
		cmd_data = yingxue_base.shizhe_temp;
		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 103, cmd_data);
	}
	else if (wifi_base_g.beg_upstate == 2){
		//unsigned char moshi_mode; // 模式 0无模式 1 普通模式 2 超级模式 3 节能模式 4 水果模式
		if (yingxue_base.moshi_mode == 4){
			cmd_data = 1;
		}
		else if (yingxue_base.moshi_mode == 3){
			cmd_data = 2;
		}
		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 102, cmd_data);
	}
	else if (wifi_base_g.beg_upstate == 1){
		//unsigned char run_state; //0第一次上电 1开机 2关机
		if (yingxue_base.run_state == 1){
			cmd_data = 1;
		}
		else if (yingxue_base.run_state == 2){
			cmd_data = 0;
		}

		yingxue_wifi_to_wifi(WIFI_CMD_STATE_UP, 101, cmd_data);

	}
	wifi_base_g.beg_upstate--;
}

//发送任务
void
yingxue_wifi_send_task()
{
	int flag = 0;

	//现在的时间
	struct timeval curtime;
	struct wifi_uart_mq_tag wifi_uart_mq;
	get_rtc_cache_time(&curtime, NULL);
	//上传设备信息,未收到信号两秒钟发送一次
	if (wifi_base_g.run_state == 0 && (curtime.tv_sec - wifi_base_g.last_send_time.tv_sec) > 2){
		yingxue_wifi_to_wifi(WIFI_CMD_EQUIP_UP, 0, 0);

	}
	
	//判断是否需要上传查询数据
	if (wifi_base_g.beg_upstate != 0){
		yingxue_wifi_upstate();
	}


	//1)无等待ack 或者 2)获得已经超时2秒钟可以再次发送
	if (wifi_base_g.ack_state == 0 || (curtime.tv_sec - wifi_base_g.last_send_time.tv_sec) > 2){
		//从队列中获得数据，然后发送到串口
		flag = mq_receive(toWifiQueue, &wifi_uart_mq, sizeof(struct wifi_uart_mq_tag), NULL);
		//如果存在信息就发送消息
		if (flag > 0){
			//发送到串口
			WIFI_SEND_UART(&wifi_uart_mq);
			//记录最后一次的时间
			memmove(&wifi_base_g.last_send_time, &curtime, sizeof(struct timeval));
			wifi_base_g.ack_state = 1;//需要等待回复
		}
	}

}

//串口发送信息
void
yingxue_wifi_senduart(struct wifi_uart_mq_tag *wifi_uart_mq)
{
	int len = 0;
#ifdef _WIN32
	printf(" time=%d to wifi:", time(NULL));
	for (int i = 0; i < wifi_uart_mq->len; i++){
		printf("0x%02X ", wifi_uart_mq->data[i]);
	}
	printf("\r\n");


#else
	//发送串口数据
	len = write(UART_PORT_WIFI, wifi_uart_mq->data, wifi_uart_mq->len);
	/*printf(" wifi send:%d\n", len);
	for (int i = 0; i < wifi_uart_mq->len; i++){
		printf("0x%02X ", wifi_uart_mq->data[i]);
	}
	printf(" end\r\n");*/
#endif
	return;
}

