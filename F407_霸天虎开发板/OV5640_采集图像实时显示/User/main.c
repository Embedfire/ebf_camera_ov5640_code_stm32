/**
  ******************************************************************************
  * @file    main.c
  * @author  fire
  * @version V1.0
  * @date    2015-xx-xx
  * @brief   OV5640摄像头显示例程
  ******************************************************************************
  * @attention
  *
  * 实验平台:野火 STM32  F407开发板 
  * 论坛    :http://www.firebbs.cn
  * 淘宝    :https://fire-stm32.taobao.com
  *
  ******************************************************************************
  */
  
#include "stm32f4xx.h"
#include "./usart/bsp_debug_usart.h"
#include "./lcd/bsp_ili9806g_lcd.h"
#include "./camera/bsp_ov5640.h"
#include "./camera/ov5640_AF.h"
#include "./systick/bsp_SysTick.h"
#include "./sram/bsp_sram.h"	  
#include "./key/bsp_key.h" 
#include "./led/bsp_led.h"   



/*简单任务管理*/
uint32_t Task_Delay[NumOfTask];

char dispBuf[100];
OV5640_IDTypeDef OV5640_Camera_ID;

uint8_t fps=0;



/**
  * @brief  液晶屏开窗，使能摄像头数据采集
  * @param  无
  * @retval 无
  */
void ImagDisp(void)
{
		//扫描模式，横屏
    ILI9806G_GramScan(cam_mode.lcd_scan);
    LCD_SetFont(&Font16x32);
		LCD_SetColors(RED,BLACK);
	
    ILI9806G_Clear(0,0,LCD_X_LENGTH,LCD_Y_LENGTH);	/* 清屏，显示全黑 */

	/*DMA会把数据传输到液晶屏，开窗后数据按窗口排列 */
    ILI9806G_OpenWindow(cam_mode.lcd_sx,
													cam_mode.lcd_sy,
													cam_mode.cam_out_width,
													cam_mode.cam_out_height);	
	
		OV5640_Capture_Control(ENABLE);
}


//修改cam_mode参数，修改后需要重新调用配置函数才生效
void Camera_Mode_Reconfig(void)
{
	cam_mode.frame_rate = FRAME_RATE_30FPS,	
	
	//ISP窗口
	cam_mode.cam_isp_sx = 0;
	cam_mode.cam_isp_sy = 0;	
	
	cam_mode.cam_isp_width = 1920;
	cam_mode.cam_isp_height = 1080;
	
	//输出窗口
	cam_mode.scaling = 1;      //使能自动缩放
	cam_mode.cam_out_sx = 16;	//使能自动缩放后，一般配置成16即可
	cam_mode.cam_out_sy = 4;	  //使能自动缩放后，一般配置成4即可
	cam_mode.cam_out_width = 320;
	cam_mode.cam_out_height = 240;
	
	//LCD位置
	cam_mode.lcd_sx = 270;
	cam_mode.lcd_sy = 120;
	cam_mode.lcd_scan = 5; //LCD扫描模式，
	
	//以下可根据自己的需要调整，参数范围见结构体类型定义	
	cam_mode.light_mode = 0;//自动光照模式
	cam_mode.saturation = 0;	
	cam_mode.brightness = 0;
	cam_mode.contrast = 0;
	cam_mode.effect = 0;		//正常模式
	cam_mode.exposure = 0;		

	cam_mode.auto_focus = 1;//自动对焦
}


//【*】注意事项：
//本程序暂时不支持摄像头延长线！！！
/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
int main(void)
{	
  uint8_t focus_status = 0;
	
	ILI9806G_Init ();         //LCD 初始化
	
	LCD_SetFont(&Font16x32);
	LCD_SetColors(RED,BLACK);

  ILI9806G_Clear(0,0,LCD_X_LENGTH,LCD_Y_LENGTH);	/* 清屏，显示全黑 */

  Debug_USART_Config();   
	
	/* 配置SysTick 为1ms中断一次,时间到后触发定时中断，
	*进入stm32fxx_it.c文件的SysTick_Handler处理，通过数中断次数计时
	*/
	SysTick_Init();
	
	Key_GPIO_Config();

	//液晶扫描方向
	ILI9806G_GramScan(5);
	
  CAMERA_DEBUG("STM32F407 DCMI 驱动OV5640例程");

  /* 初始化摄像头GPIO及IIC */
  OV5640_HW_Init();   

  /* 读取摄像头芯片ID，确定摄像头正常连接 */
  OV5640_ReadID(&OV5640_Camera_ID);

   if(OV5640_Camera_ID.PIDH  == 0x56)
  {
    sprintf((char*)dispBuf, "OV5640 Camera ID:0x%x,initializing... ", OV5640_Camera_ID.PIDH);
		ILI9806G_DispStringLine_EN(LINE(0),dispBuf);
    CAMERA_DEBUG("检测到摄像头 %x %x",OV5640_Camera_ID.PIDH ,OV5640_Camera_ID.PIDL);

  }
  else
  {
    LCD_SetTextColor(RED);
    ILI9806G_DispString_EN(10,10, "Can not detect OV5640 module,please check the connection!");
    CAMERA_DEBUG("没有检测到OV5640摄像头，请重新检查连接。");

    while(1);  
  }

  
  OV5640_Init();
	
  OV5640_RGB565_Default_Config();
	
	OV5640_USER_Config();

	OV5640_FOCUS_AD5820_Init();
	

	if(cam_mode.auto_focus ==1)
	{
		OV5640_FOCUS_AD5820_Constant_Focus();
		focus_status = 1;
	}
    
	/*DMA直接传输摄像头数据到LCD屏幕显示*/
	ImagDisp();
	
  while(1)
	{		
		//运行时动态修改摄像头参数示例
		if( Key_Scan(KEY1_GPIO_PORT,KEY1_PIN) == KEY_ON  )
		{
			//关闭采集
			OV5640_Capture_Control(DISABLE);
			
			//修改Cam mode 参数
			Camera_Mode_Reconfig();
			
			OV5640_USER_Config();

			if(cam_mode.auto_focus ==1)
			{
				OV5640_FOCUS_AD5820_Constant_Focus();
				focus_status = 1;
			}
				
			/*DMA直接传输摄像头数据到LCD屏幕显示*/
			ImagDisp();			
			
		}   
		
		//反转对焦状态，若原来在持续对焦则暂停，否则开启
		if( Key_Scan(KEY2_GPIO_PORT,KEY2_PIN) == KEY_ON  )
		{
			if(focus_status == 1)
			{
				//暂停对焦
				OV5640_FOCUS_AD5820_Pause_Focus();
				focus_status = 0 ;
			}
			else
			{
				//自动对焦
				OV5640_FOCUS_AD5820_Constant_Focus();
				focus_status = 1 ;
			}
		}   


		
		//使用串口输出帧率
		if(Task_Delay[0]==0)
		{			
			/*输出帧率*/
			CAMERA_DEBUG("\r\n帧率:%.1f/s \r\n", (double)fps/5.0);
			//重置
			fps =0;			
			
			Task_Delay[0]=5000; //此值每1ms会减1，减到0才可以重新进来这里

		}		
	}
}



/*********************************************END OF FILE**********************/

