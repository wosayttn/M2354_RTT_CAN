/**************************************************************************//**
*
* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
*
* SPDX-License-Identifier: Apache-2.0
*
* Change Logs:
* Date            Author       Notes
* 2020-8-26       Wayne        First version
*
******************************************************************************/

#include <rtconfig.h>
#include <rtdevice.h>
#include <drv_gpio.h>

/* defined the LEDR pin: PD3 */
#define LEDR   NU_GET_PININDEX(NU_PD, 3)

int main(int argc, char **argv)
{
#if defined(RT_USING_PIN)
    int counter = 100;
    /* set LEDR pin mode to output */
    rt_pin_mode(LEDR, PIN_MODE_OUTPUT);

    while (counter--)
    {
        rt_pin_write(LEDR, PIN_HIGH);
        rt_thread_mdelay(500);
        rt_pin_write(LEDR, PIN_LOW);
        rt_thread_mdelay(500);
    }
#endif
    return 0;
}


/////////////////////////////////////////////////////////
//#include "NuMicro.h"
////CAN0初始化
//void CAN0_Init(uint32_t u32BaudRate, uint32_t u32Mode)
//{
//	CLK->APBCLK0 |= CLK_APBCLK0_CAN0CKEN_Msk;
//	CAN_Open(CAN0,u32BaudRate, u32Mode);
//}

////CAN0发送数据帧
//int CAN0_TX_FRAME(STR_CANMSG_T *TX_MSG)
//{
//	if(TX_MSG==NULL) return -1;
//	if(CAN_BasicSendMsg(CAN0,TX_MSG)==TRUE) return 1;
//	return 0;
//}

////CAN0接收数据帧
//int CAN0_RX_FRAME(STR_CANMSG_T *RX_MSG)
//{
//	if(RX_MSG==NULL) return -1;
//	if(CAN_BasicReceiveMsg(CAN0,RX_MSG)==TRUE)
//	{
//		CAN0->IF[1].MCON = 0;
//		return 1;
//	}
//	return 0;
//}

//void show_msg(STR_CANMSG_T *msg)
//{
//	int i=0;
//	rt_kprintf("[ID:%x]  ", msg->Id);
//    for (i = 0; i < msg->DLC; i++)
//    {
//		rt_kprintf("%2x ", msg->Data[i]);
//    }
//	rt_kprintf("\n\r");
//}

//static void can_rx_thread(void *parameter)
//{
//	STR_CANMSG_T RX_MSG;
//	CAN0_Init(500000,CAN_BASIC_MODE);
//	while(1)
//	{
//		if(CAN0_RX_FRAME(&RX_MSG))
//		{
//			show_msg(&RX_MSG);
//			CAN0_TX_FRAME(&RX_MSG);
//		}
//		else
//		{
//			rt_thread_delay(2);
//		}
//	}
//}

//int can_sample(int argc, char *argv[])
//{
//		
//    struct rt_can_msg msg = {0};
//    rt_err_t res;
//    rt_size_t  size;
//    rt_thread_t thread;

//    /* 创建数据接收线程 */
//    thread = rt_thread_create("can_rx", can_rx_thread, RT_NULL, 1024, 25, 10);
//    if (thread != RT_NULL)
//    {
//        rt_thread_startup(thread);
//    }
//    else
//    {
//        rt_kprintf("create can_rx thread failed!\n");
//    }
//    return res;
//}
///* 导出到 msh 命令列表中 */
//MSH_CMD_EXPORT(can_sample, can device sample);
/////////////////////////////////////////////////////////

////////////////////////////////////////////
/*
 * 程序清单：这是一个 CAN 设备使用例程
 * 例程导出了 can_sample 命令到控制终端
 * 命令调用格式：can_sample can1
 * 命令解释：命令第二个参数是要使用的 CAN 设备名称，为空则使用默认的 CAN 设备
 * 程序功能：通过 CAN 设备发送一帧，并创建一个线程接收数据然后打印输出。
*/

#include <rtthread.h>
#include "rtdevice.h"

#define CAN_DEV_NAME       "can0"      /* CAN 设备名称 */

static struct rt_semaphore rx_sem;     /* 用于接收消息的信号量 */
static rt_device_t can_dev;            /* CAN 设备句柄 */

/* 接收数据回调函数 */
static rt_err_t can_rx_call(rt_device_t dev, rt_size_t size)
{
    /* CAN 接收到数据后产生中断，调用此回调函数，然后发送接收信号量 */
    rt_sem_release(&rx_sem);

    return RT_EOK;
}

static void can_rx_thread(void *parameter)
{
    int i;
    rt_err_t res;
    struct rt_can_msg rxmsg = {0};

    /* 设置接收回调函数 */
    rt_device_set_rx_indicate(can_dev, can_rx_call);

#ifdef RT_CAN_USING_HDR
    struct rt_can_filter_item items[5] =
    {
        RT_CAN_FILTER_ITEM_INIT(0x100, 0, 0, 1, 0x700, RT_NULL, RT_NULL), /* std,match ID:0x100~0x1ff，hdr 为 - 1，设置默认过滤表 */
        RT_CAN_FILTER_ITEM_INIT(0x300, 0, 0, 1, 0x700, RT_NULL, RT_NULL), /* std,match ID:0x300~0x3ff，hdr 为 - 1 */
        RT_CAN_FILTER_ITEM_INIT(0x211, 0, 0, 1, 0x7ff, RT_NULL, RT_NULL), /* std,match ID:0x211，hdr 为 - 1 */
        RT_CAN_FILTER_STD_INIT(0x486, RT_NULL, RT_NULL),                  /* std,match ID:0x486，hdr 为 - 1 */
        {0x555, 0, 0, 1, 0x7ff, 7,}                                       /* std,match ID:0x555，hdr 为 7，指定设置 7 号过滤表 */
    };
    struct rt_can_filter_config cfg = {5, 1, items}; /* 一共有 5 个过滤表 */
    /* 设置硬件过滤表 */
    res = rt_device_control(can_dev, RT_CAN_CMD_SET_FILTER, &cfg);
    RT_ASSERT(res == RT_EOK);
#endif

    while (1)
    {
        /* hdr 值为 - 1，表示直接从 uselist 链表读取数据 */
        rxmsg.hdr = -1;
        /* 阻塞等待接收信号量 */
        rt_sem_take(&rx_sem, RT_WAITING_FOREVER);
        /* 从 CAN 读取一帧数据 */
        rt_device_read(can_dev, 0, &rxmsg, sizeof(rxmsg));
        /* 打印数据 ID 及内容 */
        rt_kprintf("ID:%x", rxmsg.id);
        for (i = 0; i < 8; i++)
        {
            rt_kprintf("%2x", rxmsg.data[i]);
        }

        rt_kprintf("\n");
    }
}


int can_sample(int argc, char *argv[])
{
    struct rt_can_msg msg = {0};
    rt_err_t res;
    rt_size_t  size;
    rt_thread_t thread;
    char can_name[RT_NAME_MAX];

    if (argc == 2)
    {
        rt_strncpy(can_name, argv[1], RT_NAME_MAX);
    }
    else
    {
        rt_strncpy(can_name, CAN_DEV_NAME, RT_NAME_MAX);
    }
    /* 查找 CAN 设备 */
    can_dev = rt_device_find(can_name);
    if (!can_dev)
    {
        rt_kprintf("find %s failed!\n", can_name);
        return RT_ERROR;
    }

    /* 初始化 CAN 接收信号量 */
    rt_sem_init(&rx_sem, "rx_sem", 0, RT_IPC_FLAG_FIFO);

	/* 以中断接收及发送方式打开 CAN 设备 */
	res = rt_device_open(can_dev, RT_DEVICE_FLAG_INT_TX | RT_DEVICE_FLAG_INT_RX);
	RT_ASSERT(res == RT_EOK);

	/* 设置 CAN 通信的波特率为 500kbit/s*/
	res = rt_device_control(can_dev, RT_CAN_CMD_SET_BAUD, (void *)CAN500kBaud);
	RT_ASSERT(res == RT_EOK);

	/* 设置 CAN 的工作模式为正常工作模式 */
	res = rt_device_control(can_dev, RT_CAN_CMD_SET_MODE, (void *)RT_CAN_MODE_NORMAL);
	RT_ASSERT(res == RT_EOK);

    /* 创建数据接收线程 */
    thread = rt_thread_create("can_rx", can_rx_thread, RT_NULL, 2048, 25, 10);
    if (thread != RT_NULL)
    {
        rt_thread_startup(thread);
        rt_kprintf("create can_rx thread ok!\n");		
    }
    else
    {
        rt_kprintf("create can_rx thread failed!\n");
    }

    return res;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(can_sample, can device sample);

int can_sendmsg(int argc, char *argv[])
{
    struct rt_can_msg msg = {0};
    rt_err_t res;
    rt_size_t  size;

    msg.id = 0x78;              /* ID 为 0x78 */
    msg.ide = RT_CAN_STDID;     /* 标准格式 */
    msg.rtr = RT_CAN_DTR;       /* 数据帧 */
    msg.len = 8;                /* 数据长度为 8 */
    /* 待发送的 8 字节数据 */
    msg.data[0] = 0x00;
    msg.data[1] = 0x11;
    msg.data[2] = 0x22;
    msg.data[3] = 0x33;
    msg.data[4] = 0x44;
    msg.data[5] = 0x55;
    msg.data[6] = 0x66;
    msg.data[7] = 0x77;
    /* 发送一帧 CAN 数据 */
    size = rt_device_write(can_dev, 0, &msg, sizeof(msg));
    if (size == 0)
    {
        rt_kprintf("can dev write data failed!\n");
    }
	else
	{
		rt_kprintf("can dev write data success!\n");
	}

    return res;
}
/* 导出到 msh 命令列表中 */
MSH_CMD_EXPORT(can_sendmsg, can device send);






















