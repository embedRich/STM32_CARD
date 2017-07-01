/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                          (c) Copyright 2003-2013; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                     Micrium uC-Eval-STM32F107
*                                        Evaluation Board
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : EHS
*                 DC
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/

CPU_TS             ts_start;       //时间戳变量
CPU_TS             ts_end;



/*
*********************************************************************************************************
*                                                 TCB
*********************************************************************************************************
*/

static  OS_TCB   AppTaskStartTCB;                                //任务控制块

static  OS_TCB   AppTaskTmrTCB;

OS_TCB   AppTaskOledTCB;                // OLED更新任务控制块

OS_TCB   AppTaskWriteFrameTCB;          // 发送数据任务控制块

OS_TCB   AppTaskReadFrameTCB;           // 读取解析数据任务控制块

OS_Q queue_uart1;     // 消息队列

OS_Q queue_task_write;     // 消息队列

CPU_INT08U g_ucSerNum = 0;

/*
*********************************************************************************************************
*                                                STACKS
*********************************************************************************************************
*/

static  CPU_STK  AppTaskStartStk [ APP_TASK_START_STK_SIZE ];       //任务堆栈

static  CPU_STK  AppTaskTmrStk [ APP_TASK_TMR_STK_SIZE ];

static  CPU_STK  AppTaskOLEDStk [ APP_TASK_OLED_STK_SIZE ];

static  CPU_STK  AppTaskWriteFrameStk [ APP_TASK_WRITE_FRAME_STK_SIZE ];

static  CPU_STK  AppTaskReadFrameStk [ APP_TASK_READ_FRAME_STK_SIZE ];
/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);                       //任务函数声明

static  void  AppTaskTmr  ( void * p_arg );

static  void  AppTaskOLED  ( void * p_arg );

static void  AppTaskWriteFrame ( void * p_arg );

static void  AppTaskReadFrame ( void * p_arg );

extern unsigned char BMP1[];
/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR  err;


    OSInit(&err);                                                           //初始化 uC/OS-III

    /* 创建起始任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB,                            //任务控制块地址
                 (CPU_CHAR   *)"App Task Start",                            //任务名称
                 (OS_TASK_PTR ) AppTaskStart,                               //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_START_PRIO,                        //任务的优先级
                 (CPU_STK    *)&AppTaskStartStk[0],                         //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE / 10,               //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_START_STK_SIZE,                    //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型

    OSStart(&err);                                                          //启动多任务管理（交由uC/OS-III控制）
}


/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
    OS_ERR      err;


   (void)p_arg;

    BSP_Init();                                                 //板级初始化
    CPU_Init();                                                 //初始化 CPU 组件（时间戳、关中断时间测量和主机名）

    cpu_clk_freq = BSP_CPU_ClkFreq();                           //获取 CPU 内核时钟频率（SysTick 工作时钟）
    cnts = cpu_clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;        //根据用户设定的时钟节拍频率计算 SysTick 定时器的计数值
    OS_CPU_SysTickInit(cnts);                                   //调用 SysTick 初始化函数，设置定时器计数值和启动定时器

    Mem_Init();                                                 //初始化内存管理组件（堆内存池和内存池表）

#if OS_CFG_STAT_TASK_EN > 0u                                    //如果使能（默认使能）了统计任务
    OSStatTaskCPUUsageInit(&err);                               //计算没有应用任务（只有空闲任务）运行时 CPU 的（最大）
#endif                                                          //容量（决定 OS_Stat_IdleCtrMax 的值，为后面计算 CPU
                                                                //使用率使用）。
    CPU_IntDisMeasMaxCurReset();                                //复位（清零）当前最大关中断时间

    /* 创建消息队列 queue */
    OSQCreate     ((OS_Q       *)&queue_uart1,      //指向消息队列的指针
                (CPU_CHAR    *)"Queue Frame",       //队列的名字
                (OS_MSG_QTY   )30,                  //最多可存放消息的数目
                (OS_ERR      *)&err);               //返回错误类型

    /* 创建消息队列 queue */
    OSQCreate     ((OS_Q       *)&queue_task_write,      //指向消息队列的指针
                (CPU_CHAR    *)"Queue Frame",       //队列的名字
                (OS_MSG_QTY   )5,                  //最多可存放消息的数目
                (OS_ERR      *)&err);               //返回错误类型

    /* 创建 AppTaskTmr 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskTmrTCB,                              //任务控制块地址
                 (CPU_CHAR   *)"App Task Tmr",                              //任务名称
                 (OS_TASK_PTR ) AppTaskTmr,                                 //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_TMR_PRIO,                          //任务的优先级
                 (CPU_STK    *)&AppTaskTmrStk[0],                           //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_TMR_STK_SIZE / 10,                 //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_TMR_STK_SIZE,                      //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);                                       //返回错误类型

    /* 创建 AppTaskOLED 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskOledTCB,                             //任务控制块地址
                 (CPU_CHAR   *)"OLED_APP",                                  //任务名称
                 (OS_TASK_PTR ) AppTaskOLED,                                //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_OLED_PRIO,                         //任务的优先级
                 (CPU_STK    *)&AppTaskOLEDStk[0],                          //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_OLED_STK_SIZE / 10,                //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_OLED_STK_SIZE,                     //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);

    /* 创建 AppTaskWriteFRAME 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskWriteFrameTCB,                       //任务控制块地址
                 (CPU_CHAR   *)"WRITE_FRAME_APP",                           //任务名称
                 (OS_TASK_PTR ) AppTaskWriteFrame,                          //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_WRITE_FRAME_PRIO,                  //任务的优先级
                 (CPU_STK    *)&AppTaskWriteFrameStk[0],                    //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_WRITE_FRAME_STK_SIZE / 10,         //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_WRITE_FRAME_STK_SIZE,              //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);

    /* 创建 AppTaskReadFRAME 任务 */
    OSTaskCreate((OS_TCB     *)&AppTaskReadFrameTCB,                        //任务控制块地址
                 (CPU_CHAR   *)"READ_FRAME_APP",                                 //任务名称
                 (OS_TASK_PTR ) AppTaskReadFrame,                           //任务函数
                 (void       *) 0,                                          //传递给任务函数（形参p_arg）的实参
                 (OS_PRIO     ) APP_TASK_READ_FRAME_PRIO,                   //任务的优先级
                 (CPU_STK    *)&AppTaskReadFrameStk[0],                     //任务堆栈的基地址
                 (CPU_STK_SIZE) APP_TASK_READ_FRAME_STK_SIZE / 10,          //任务堆栈空间剩下1/10时限制其增长
                 (CPU_STK_SIZE) APP_TASK_READ_FRAME_STK_SIZE,               //任务堆栈空间（单位：sizeof(CPU_STK)）
                 (OS_MSG_QTY  ) 5u,                                         //任务可接收的最大消息数
                 (OS_TICK     ) 0u,                                         //任务的时间片节拍数（0表默认值OSCfg_TickRate_Hz/10）
                 (void       *) 0,                                          //任务扩展（0表不扩展）
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR), //任务选项
                 (OS_ERR     *)&err);

    OSTaskDel ( & AppTaskStartTCB, & err );                     //删除起始任务本身，该任务不再运行
}


/*
*********************************************************************************************************
*                                          TMR TASK
*********************************************************************************************************
*/
void TmrCallback (OS_TMR *p_tmr, void *p_arg) //软件定时器MyTmr的回调函数
{
    CPU_INT32U       cpu_clk_freq;
    CPU_SR_ALLOC();      //使用到临界段（在关/开中断时）时必需该宏，该宏声明和定义一个局部变
                                             //量，用于保存关中断前的 CPU 状态寄存器 SR（临界段关中断只需保存SR）
                                             //，开中断时将该值还原。
    static CPU_INT08U ucNum = 0;                // 发送数据的序号


    u8 para[] = { '<', '0', 'B', '1','3',
                    '0', '0', '5', '0', '0', '1',
                    '0', '0', '5', '0', '0', '1',
                    '0', '0', '5', '0', '0', '1',
                    '0', '0', '5', '0', '0', '1',
                    '>',0};
    para[1] = (ucNum++ % 10) + '0';
    //DEBUG_printf ( "%s", ( char * ) p_arg );

    cpu_clk_freq = BSP_CPU_ClkFreq();                   //获取CPU时钟，时间戳是以该时钟计数

    //macLED2_TOGGLE ();

    ts_end = OS_TS_GET() - ts_start;     //获取定时后的时间戳（以CPU时钟进行计数的一个计数值）
                                         //，并计算定时时间。
    OS_CRITICAL_ENTER();                 //进入临界段，不希望下面串口打印遭到中断

    //printf ( "\r\n定时1s，通过时间戳测得定时 %07d us，即 %04d ms。\r\n",
    //                    ts_end / ( cpu_clk_freq / 1000000 ),     //将定时时间折算成 us
    //                    ts_end / ( cpu_clk_freq / 1000 ) );      //将定时时间折算成 ms

    printf ( "%s\n", ( char * ) para );
    OS_CRITICAL_EXIT();

    ts_start = OS_TS_GET();                            //获取定时前时间戳
}


static  void  AppTaskTmr ( void * p_arg )
{
    OS_ERR      err;
    OS_TMR      my_tmr;   //声明软件定时器对象

    (void)p_arg;

    /* 创建软件定时器 */
    OSTmrCreate ((OS_TMR              *)&my_tmr,           //软件定时器对象
               (CPU_CHAR            *)"MySoftTimer",       //命名软件定时器
               (OS_TICK              )20,                  //定时器初始值，依10Hz时基计算，即为1s
               (OS_TICK              )20,                  //定时器周期重载值，依10Hz时基计算，即为1s
               (OS_OPT               )OS_OPT_TMR_PERIODIC, //周期性定时
               (OS_TMR_CALLBACK_PTR  )TmrCallback,         //回调函数
               (void                *)"Timer Over!",       //传递实参给回调函数
               (OS_ERR              *)err);                //返回错误类型

    /* 启动软件定时器 */
    OSTmrStart ((OS_TMR   *)&my_tmr, //软件定时器对象
              (OS_ERR   *)err);    //返回错误类型

    ts_start = OS_TS_GET();                       //获取定时前时间戳

    while (DEF_TRUE)
    {                            //任务体，通常写成一个死循环

        OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err ); //不断阻塞该任务

    }
}


static  void AppTaskOLED ( void * p_arg )
{
    OS_ERR      err;
#ifdef OLED
    //u8 arr[]
    macLED1_ON();
    //macLED3_ON();
    OLED_Init ();
    //OSTimeDly ( 10, OS_OPT_TIME_DLY, & err ); //不断阻塞该任务
    OLED_SetPos(0,0);
    OLED_CLS();
    //OLED_ShowCN(0,0,2);
    //OLED_ShowCN(16,0,3);
    //OLED_Fill(0xff);
    //OLED_DrawBMP(0,0,128,8,BMP1);
#endif
    while (DEF_TRUE)
    {                            //任务体，通常写成一个死循环
        //macLED2_TOGGLE ();
        OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err ); //不断阻塞该任务

    }
}



void  AppTaskWriteFrame ( void * p_arg )
{
    OS_ERR      err;
    OS_MSG_SIZE    msg_size;
    //CPU_TS         ts;
    CPU_INT08U * pMsg = NULL;

    OSTimeDly ( 2000, OS_OPT_TIME_DLY, & err ); //不断阻塞该任务

    while (DEF_TRUE)
    {                            //任务体，通常写成一个死循环
        /* 阻塞任务，等待任务消息
        pMsg = OSTaskQPend ((OS_TICK        )10,                   //无期限等待
                            (OS_OPT         )OS_OPT_PEND_BLOCKING, //没有消息就阻塞任务
                            (OS_MSG_SIZE   *)&msg_size,            //返回消息长度
                            (CPU_TS        *)&ts,                  //返回消息被发布的时间戳
                            (OS_ERR        *)&err);                //返回错误类型
        */

        /* 请求消息队列 queue 的消息
        pMsg = OSQPend ((OS_Q         *)&queue_uart1,          //消息变量指针
                        (OS_TICK       )10,                    //等待时长为无限
                        (OS_OPT        )OS_OPT_PEND_BLOCKING,  //如果没有获取到信号量就等待
                        (OS_MSG_SIZE  *)&msg_size,             //获取消息的字节大小
                        (CPU_TS       *)0,                     //获取任务发送时的时间戳
                        (OS_ERR       *)&err);                 //返回错误
        WriteFrame (pMsg);

        if(pMsg != NULL)
        {
            OLED_ShowStr(0,0,pMsg,1);
        }
        */
        //macLED2_TOGGLE ();
        OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err ); //不断阻塞该任务
    }
}

void  AppTaskReadFrame ( void * p_arg )
{
    CPU_SR_ALLOC();      //使用到临界段（在关/开中断时）时必需该宏，该宏声明和定义一个局部变
                                     //量，用于保存关中断前的 CPU 状态寄存器 SR（临界段关中断只需保存SR）
                                     //，开中断时将该值还原.
    OS_ERR      err;
    OS_MSG_SIZE    msg_size;
    CPU_INT08U * pMsg = NULL;
    //CPU_INT08U ucaMsg[30] = "0aiwesky uC/OS-III";
    CPU_INT08U ucPowerOnPara[4] =   {FRAME_START,
                                    '0',
                                    CARD_MACHINE_POWER_ON,
                                    FRAME_END};
    RSCTL_FREME t_PowerOnFrame =  {FRAME_START,'0',CARD_MACHINE_POWER_ON,FRAME_END};        // 上电数据

    OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err ); //等待1S
    OS_CRITICAL_ENTER();                 //进入临界段，不希望下面串口打印遭到中断
    DEBUG_printf ("%s","你好");
    //printf ("%s\r\n","你好");
    USART4_SendString(mac4USART, (char *)&t_PowerOnFrame);
    OS_CRITICAL_EXIT();

    //OSTimeDly ( 2000, OS_OPT_TIME_DLY, & err ); //不断阻塞该任务
    while (DEF_TRUE)
    {                            //任务体，通常写成一个死循环
        //macLED2_TOGGLE ();

        /* 发布消息到任务 AppTaskPend
        OSTaskQPost ((OS_TCB      *)&AppTaskWriteFrameTCB,          //目标任务的控制块
                    (void        *)"Binghuo uC/OS-III",             //消息内容
                    (OS_MSG_SIZE  )sizeof ( "Binghuo uC/OS-III" ),  //消息长度
                    (OS_OPT       )OS_OPT_POST_FIFO,                //发布到任务消息队列的入口端
                    (OS_ERR      *)&err);                           //返回错误类型
        */



        //strcat((char *)ucaMsg,(const char *)ucaUCOS);
        /* 发布消息到消息队列 queue
        OSQPost ((OS_Q        *)&queue,                             //消息变量指针
                 (void        *)ucaMsg,                             //要发送的数据的指针，将内存块首地址通过队列"发送出去"
                 (OS_MSG_SIZE  )sizeof ( ucaMsg ),                  //数据字节大小
                 (OS_OPT       )OS_OPT_POST_FIFO | OS_OPT_POST_ALL, //先进先出和发布给全部任务的形式
                 (OS_ERR      *)&err);                                //返回错误类型
        ucSerNum++;*/

        pMsg = OSQPend ((OS_Q         *)&queue_uart1,           //消息变量指针
                        (OS_TICK       )10,                     //等待时长为无限
                        (OS_OPT        )OS_OPT_PEND_BLOCKING,   //如果没有获取到信号量就等待
                        (OS_MSG_SIZE  *)&msg_size,              //获取消息的字节大小
                        (CPU_TS       *)0,                      //获取任务发送时的时间戳
                        (OS_ERR       *)&err);                  //返回错误

        //macLED2_TOGGLE ();
        if (pMsg != NULL)
        {
            AnalyzeUartFrame((void *)pMsg);
        }
        if(pMsg != NULL)
        {
            //OLED_ShowStr(0,0,pMsg,1);
        }
        OSTimeDly ( 1000, OS_OPT_TIME_DLY, & err );     //不断阻塞该任务
    }
}


