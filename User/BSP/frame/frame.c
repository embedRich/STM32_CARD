#include <includes.h>
#include "frame.h"
#include "WAV_C_xiexie.h"
#include "WAV_C_anjianquka.h"

CPU_INT08U g_ucSerNum = '0';  // 帧序号   全局

RSCTL_FREME g_tP_RsctlFrame = {'<','0','0','>'};        // 正应答帧
RSCTL_FREME g_tN_sctlFrame =  {'<','1','0','>'};        // 负应答帧

/* 卡机上电信息(41H)帧          4字节 */
CARD_MACHINE_POWER_ON_FREME      g_tCardMechinePowerOnFrame = {'<', '0', CARD_MACHINE_POWER_ON, '>'};;

/* 状态信息(42H)帧             30字节 */
CARD_MACHINE_STATUES_FRAME       g_tCardMechineStatusFrame =    {'<', '0', 'B', '1','3',
                                                                '0', '0', '9', '9', '9', '1',
                                                                '0', '0', '9', '9', '9', '1',
                                                                '0', '0', '9', '9', '9', '1',
                                                                '0', '0', '9', '9', '9', '1',
                                                                '>'};

/* 已出卡信息(43H)帧            6字节 */
CARD_MECHINE_TO_PC_FRAME         g_tCardSpitOutFrame = {'<', '0', CARD_SPIT_OUT, '1', '1', '>'};

/* 按钮取卡信息(44H)帧          6字节 */
CARD_MECHINE_TO_PC_FRAME        g_tCardKeyPressFrame = {'<', '0', CARD_KEY_PRESS, '1', '1', '>'};

/* 卡被取走信息(45H)帧          6字节 */
CARD_MECHINE_TO_PC_FRAME         g_tCardTakeAwayFrame = {'<', '0', CARD_TAKE_AWAY, '1', '1', '>'};

/* 上报卡夹号编号信息(46H)帧   36字节 */
CARD_REPORT_SPIT_STATUES_FRAME   g_tCardReportSpitStatusFrame = {'<', '0', CARD_REPORT_SPIT_STATUES, '0', '0', '0', '0', '0', '0', '0', '0',
                                                                                                     '0', '0', '0', '0', '0', '0', '0', '0',
                                                                                                     '0', '0', '0', '0', '0', '0', '0', '0',
                                                                                                     '0', '0', '0', '0', '0', '0', '0', '0', '>'};



/* 初始化卡机信息(61H)帧       20字节 */
PC_TO_CARD_INIT_FREME            g_tPcToCardInitFrame = {'<', '0', PC_INIT_MECHINE, '9', '9', '9', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0','>'};

/* 出卡信息(62H)帧              5字节 */
PC_TO_CARD_MECHINE_FRAME         g_tPcSpitOutCardFrame = {'<', '0', PC_SPIT_OUT_CARD, '1', '>'};

/* 坏卡信息(63H)帧              5字节 */
PC_TO_CARD_MECHINE_FRAME         g_tPcBadCardFrame = {'<', '0', PC_BAD_CARD, '1', '>'};

/* 查询卡机状态(65H)帧          5字节 */
PC_TO_CARD_MECHINE_FRAME         g_tPcQuetyCardMechineFrame = {'<', '0', PC_QUERY_CARD_MECHINE, '1', '>'};

 /* 查询卡夹(66H)帧              5字节 */
PC_TO_CARD_MECHINE_FRAME         g_tPcQuetyCardCpipFrame = {'<', '0', PC_QUERY_CARD_CLIP, '1', '>'};

/* 设置卡夹卡数(67H)帧          8字节*/
PC_SET_CARD_NUM_FRAME            g_tPcSetCardNumFrame = {'<', '0', PC_SET_CARD_NUM, '1', '9', '9' , '9', '>'};





//#pragma  diag_suppress 870          // 不显示警告

const Print_msg g_taPri_msg[] = {
                            {'$',                           "/* 无效信息 */"},
                            {CARD_MACHINE_POWER_ON,         "/* 卡机上电信息(41H)帧          4字节 */"},
                            {CARD_MACHINE_STATUES,          "/* 状态信息(42H)帧             30字节 */"},
                            {CARD_SPIT_OUT,                 "/* 已出卡信息(43H)帧            6字节 */"},
                            {CARD_KEY_PRESS,                "/* 按钮取卡信息(44H)帧          6字节 */"},
                            {CARD_TAKE_AWAY,                "/* 卡被取走信息(45H)帧          6字节 */"},
                            {CARD_REPORT_SPIT_STATUES,      "/* 上报卡夹号编号信息(46H)帧   36字节 */"},

                            {PC_INIT_MECHINE,               "/* 初始化卡机信息(61H)帧       20字节 */"},
                            {PC_SPIT_OUT_CARD,              "/* 出卡信息(62H)帧              5字节 */"},
                            {PC_BAD_CARD,                   "/* 坏卡信息(63H)帧              5字节 */"},
                            {PC_QUERY_CARD_MECHINE,         "/* 查询卡机状态(65H)帧          5字节 */"},
                            {PC_QUERY_CARD_CLIP,            "/* 查询卡夹(66H)帧              5字节 */"},
                            {PC_SET_CARD_NUM,               "/* 设置卡夹卡数(67H)帧          8字节 */"},
                            {'0',NULL}
                        };

const Print_msg g_taShow_msg[] = {
                            {'$',                           "NULL"},
                            {MACHINE_CHECK_CARD,            "验卡"},
                            {KEY_PRESS,                     "按键"},
                            {CARD_SPIT_NOTICE,              "出卡"},
                            {CARD_TAKE_AWAY_NOTICE,         "取卡"},
                            {'0',NULL}
                        };

const Print_msg g_taShowStatus_msg[] = {
                            {'$',                           "NULL"},
                            {CARD_IS_OK,                    "好卡"},
                            {CARD_IS_BAD,                   "坏卡"},
                            {'0',NULL}
                        };

// 找到打印的字符串，并返回其首地址
CPU_INT08U * CheckShowStatusmsg(CPU_INT08U ch)
{
    CPU_INT08U i = 0;
    for (i = 0; i < (sizeof (g_taShowStatus_msg) / sizeof (g_taShowStatus_msg[0])); i++)
    {
        if(g_taShowStatus_msg[i].CTL == ch)
        {
            return (CPU_INT08U *)g_taShowStatus_msg[i].Msg;
        }
    }
    return (CPU_INT08U *)g_taShowStatus_msg[0].Msg;
}

// 找到打印的字符串，并返回其首地址
CPU_INT08U * CheckShowmsg(CPU_INT08U ch)
{
    CPU_INT08U i = 0;
    for (i = 0; i < (sizeof (g_taShow_msg) / sizeof (g_taShow_msg[0])); i++)
    {
        if(g_taShow_msg[i].CTL == ch)
        {
            return (CPU_INT08U *)g_taShow_msg[i].Msg;
        }
    }
    return (CPU_INT08U *)g_taShow_msg[0].Msg;
}

// 找到打印的字符串，并返回其首地址
CPU_INT08U * CheckPrimsg(CPU_INT08U ch)
{
    CPU_INT08U i = 0;
    for (i = 0; i < (sizeof (g_taPri_msg) / sizeof (g_taPri_msg[0])); i++)
    {
        if(g_taPri_msg[i].CTL == ch)
        {
            return (CPU_INT08U *)g_taPri_msg[i].Msg;
        }
    }
    return (CPU_INT08U *)g_taPri_msg[0].Msg;
}



CPU_INT08U  AnalyzeCANFrame ( void * p_arg )
{
    CanRxMsg *pRxMessage = (CanRxMsg *)p_arg;                // can数据接收缓存
    OS_ERR      err;
    unsigned char i;
    unsigned char str_id[10] = {0};

    switch(pRxMessage->Data[3])
    {
        case MACHINE_CHECK_CARD:    // 指定工位验卡
            DEBUG_printf ("%s\r\n",(char *)CheckPrimsg(CARD_KEY_PRESS));
            MyCANTransmit(&gt_TxMessage, pRxMessage->Data[1], pRxMessage->Data[1], MACHINE_STATUES, CARD_IS_OK, 0, 0, NO_FAIL);
            printf ("%s\n",(char *)&g_tCardKeyPressFrame);
            break;
        case KEY_PRESS:             // 司机已按键
            //OLED_ShowStr(0,0,p_arg,1);
            DEBUG_printf ("%s\r\n",(char *)CheckPrimsg(CARD_KEY_PRESS));
            MyCANTransmit(&gt_TxMessage, pRxMessage->Data[1], pRxMessage->Data[1], WRITE_CARD_STATUS, CARD_IS_OK, 0, 0, NO_FAIL);
            printf ("%s\n",(char *)&g_tCardKeyPressFrame);
            //OLED_ShowStr(0,0,p_arg,1);
            break;
        case CARD_SPIT_NOTICE:      // 出卡通知
            dacSet(DATA_anjianquka,SOUND_LENGTH_anjianquka);
            //OSTimeDly ( 2000, OS_OPT_TIME_DLY, & err );
            break;
        case CARD_TAKE_AWAY_NOTICE: // 卡已被取走通知
            dacSet(DATA_xiexie,SOUND_LENGTH_xiexie);
            //OSTimeDly ( 2500, OS_OPT_TIME_DLY, & err );
            break;
        case CARD_IS_READY:
            break;
        case SERCH_CARD_MECHINE_ACK:// 查询卡机的回复
            g_usCurID = pRxMessage->Data[5] << 8 | pRxMessage->Data[6];
            sprintf(str_id,"%x   ",g_usCurID);
            for (i = 0; i < 6; i++)
            {
                g_dlg[DLG_CARD_ID].MsgRow[1][i + 10] = str_id[i];
            }
            g_ucKeyValues = KEY_ENTRY;      // 更新界面
            break;
        case IS_NO_CARD_WARNING:    // 无卡报警
            break;
        default:
            //OLED_ShowStr(0,0,p_arg,1);
            break;
    }
    return 0;
}

CPU_INT08U  AnalyzeUartFrame ( void * p_arg )
{
    CPU_SR_ALLOC();      //使用到临界段（在关/开中断时）时必需该宏，该宏声明和定义一个局部变
                                     //量，用于保存关中断前的 CPU 状态寄存器 SR（临界段关中断只需保存SR）
                                     //，开中断时将该值还原.
    //OS_ERR      err;
    CPU_INT08U ucSerNum = 0;
    CPU_INT08U ucNum = *((CPU_INT08U *)p_arg + 1);
    CPU_INT08U type_frame = *((CPU_INT08U *)p_arg + 2);

    if (POSITIVE_ACK == type_frame)    // 正应答帧
    {
        return 0;
    }
    else if (NAGATIVE_ACK == type_frame)    // 负应答帧
    {

    }
    else if (PC_INIT_MECHINE <= type_frame <= PC_SET_CARD_NUM)  // 检测数据合法性
    {
        g_tP_RsctlFrame.RSCTL = ucNum;
        OS_CRITICAL_ENTER();                 //进入临界段，不希望下面串口打印遭到中断
        printf("%s",(char *)&g_tP_RsctlFrame);   //发送正应答帧
        DEBUG_printf ("%s\r\n",(char *)CheckPrimsg(type_frame));
        //printf ("%s\r\n","收到信息");
        OS_CRITICAL_EXIT();
        switch(type_frame)
        {
            case PC_INIT_MECHINE:               /* 初始化卡机信息(61H)帧 */
                OLED_ShowStr(0,0,p_arg,1);
                display_GB2312_string (0, 2, "初始化", 0);
            break;
            case PC_SPIT_OUT_CARD:              /* 出卡信息(62H)帧 */
                OLED_ShowStr(0,0,p_arg,1);
                display_GB2312_string (0, 2, "出卡信息", 0);
            break;
            case PC_BAD_CARD:                  /* 坏卡信息(63H)帧 */
                OLED_ShowStr(0,0,p_arg,1);
                display_GB2312_string (0, 2, "坏卡", 0);
            break;
            case PC_QUERY_CARD_MECHINE:         /* 查询卡机状态(65H)帧 */
                OLED_ShowStr(0,0,p_arg,1);
                display_GB2312_string (0, 2, "查询卡机", 0);
            break;
            case PC_QUERY_CARD_CLIP:
                OLED_ShowStr(0,0,p_arg,1);   /* 查询卡夹(66H)帧 */
                display_GB2312_string (0, 2, "查询卡夹", 0);
            break;
            case PC_SET_CARD_NUM:
                OLED_ShowStr(0,0,p_arg,1);   /* 设置卡夹卡数(67H)帧 */
                display_GB2312_string (0, 2, "设置卡夹", 0);
            break;
            default:
                display_GB2312_string (0, 2, "无效信息", 0);
                printf("错误信息帧！\n");
            break;
        }
    }


    ucSerNum = (g_ucSerNum++) % 10 + '0';
    return 0;
}
