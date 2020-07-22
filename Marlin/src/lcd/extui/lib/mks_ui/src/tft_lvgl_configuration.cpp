/**
 * @file lcd_lvgl_configuration.c
 * @date    2020-02-21
 * */
#include "../../../../../MarlinCore.h"
#if ENABLED(TFT_LITTLE_VGL_UI)
#include "../inc/tft_lvgl_configuration.h"
#include "lvgl.h"
#include "../../../../../feature/touch/xpt2046.h"
#include "../inc/draw_ready_print.h"
#include "../inc/W25Qxx.h"
#include "../inc/pic_manager.h"


#include "../inc/mks_hardware_test.h"
#include "../inc/draw_ui.h"
#if ENABLED(POWER_LOSS_RECOVERY)
#include "../../../../../feature/powerloss.h"
#endif
#include <SPI.h>
#if ENABLED(SPI_GRAPHICAL_TFT)
#include "../inc/SPI_TFT.h"
#endif


//#include "../../../Configuration.h"
//#include "../../../src/core/macros.h"
extern void init_gb2312_font();

static lv_disp_buf_t disp_buf;
//static lv_color_t buf[LV_HOR_RES_MAX * 18]; 
//static lv_color_t buf[10*5]; 
//extern lv_obj_t * scr;
#if ENABLED (SDSUPPORT)
extern void UpdatePic();
extern void UpdateFont();
#endif
uint16_t DeviceCode = 0x9488;
extern uint8_t sel_id;

#define SetCs  
#define ClrCs 


#define  HDP  799  //Horizontal Display Period     //**
#define  HT   1000 //Horizontal Total
#define  HPS  51  //LLINE Pulse Start Position
#define  LPS  3   //	Horizontal Display Period Start Position
#define  HPW  8   //	LLINE Pulse Width


#define  VDP  479	//Vertical Display Period
#define  VT   530	//Vertical Total
#define  VPS  24	//	LFRAME Pulse Start Position
#define  FPS  23	//Vertical Display Period Start Positio
#define  VPW  3 	// LFRAME Pulse Width     //**

#define MAX_HZ_POSX HDP+1
#define MAX_HZ_POSY VDP+1 

extern uint8_t gcode_preview_over;
extern uint8_t flash_preview_begin;
extern uint8_t default_preview_flg;

void SysTick_Callback() 
{
     lv_tick_inc(1);
	 print_time_count();
}

#if DISABLED(SPI_GRAPHICAL_TFT)

extern void LCD_IO_Init(uint8_t cs, uint8_t rs);
extern void LCD_IO_WriteData(uint16_t RegValue);
extern void LCD_IO_WriteReg(uint16_t Reg);

extern void LCD_IO_WriteMultiple(uint16_t color, uint32_t count);

void tft_set_cursor(uint16_t x,uint16_t y)
{
    #if 0
	if(DeviceCode==0x8989)
	{
	 	LCD_WriteReg(0x004e,y);       
    	LCD_WriteReg(0x004f,x);  
	}
	else if((DeviceCode==0x9919))
	{
		LCD_WriteReg(0x004e,x); 
  		LCD_WriteReg(0x004f,y); 
	}
    else if((DeviceCode==0x5761))      
	{
		LCD_WrtReg(0x002A);	
        LCD_WrtRAM(x>>8);	    
        LCD_WrtRAM(x&0x00ff);
        LCD_WrtRAM(HDP>>8);	    
        LCD_WrtRAM(HDP&0x00ff);
        LCD_WrtReg(0x002b);	
        LCD_WrtRAM(y>>8);	    
        LCD_WrtRAM(y&0x00ff);
        LCD_WrtRAM(VDP>>8);	    
        LCD_WrtRAM(VDP&0x00ff);	
	}
	else if(DeviceCode == 0x9488)
    {
        ILI9488_WriteCmd(0X002A); 
        ILI9488_WriteData(x>>8); 
        ILI9488_WriteData(x&0X00FF); 
        ILI9488_WriteData(x>>8); 
        ILI9488_WriteData(x&0X00FF);			
        //ILI9488_WriteData(0X01); 
        //ILI9488_WriteData(0XDF);			
        ILI9488_WriteCmd(0X002B); 
        ILI9488_WriteData(y>>8); 
        ILI9488_WriteData(y&0X00FF);
        ILI9488_WriteData(y>>8); 
        ILI9488_WriteData(y&0X00FF);			
        //ILI9488_WriteData(0X01); 
        //ILI9488_WriteData(0X3F);			
    }				
	else
	{
  		LCD_WriteReg(0x0020,y); 
  		LCD_WriteReg(0x0021,0x13f-x); 
	}  	
    #else
    LCD_IO_WriteReg(0X002A); 
    LCD_IO_WriteData(x>>8); 
    LCD_IO_WriteData(x&0X00FF); 
    LCD_IO_WriteData(x>>8); 
    LCD_IO_WriteData(x&0X00FF);			
    //ILI9488_WriteData(0X01); 
    //ILI9488_WriteData(0XDF);			
    LCD_IO_WriteReg(0X002B); 
    LCD_IO_WriteData(y>>8); 
    LCD_IO_WriteData(y&0X00FF);
    LCD_IO_WriteData(y>>8); 
    LCD_IO_WriteData(y&0X00FF);			
    //ILI9488_WriteData(0X01); 
    //ILI9488_WriteData(0X3F);
    #endif
}

void LCD_WriteRAM_Prepare(void)
{
    #if 0
    if((DeviceCode==0x9325)||(DeviceCode==0x9328)||(DeviceCode==0x8989))
	{
  	ClrCs
  	LCD->LCD_REG = R34;
  	SetCs
	}
	else
	{
  	LCD_WrtReg(0x002C);
	}
    #else
    LCD_IO_WriteReg(0x002C);
    #endif
}

void tft_set_point(uint16_t x,uint16_t y,uint16_t point)
{
    if ( (x>480)||(y>320) ) return;
    tft_set_cursor(x,y);    
    LCD_WriteRAM_Prepare();    
    LCD_IO_WriteData(point);
}

void LCD_WriteReg(uint16_t LCD_Reg,uint16_t LCD_RegValue)
{
  /* Write 16-bit Index, then Write Reg */
  ClrCs
  LCD_IO_WriteReg(LCD_Reg);
  /* Write 16-bit Reg */
  LCD_IO_WriteData(LCD_RegValue);
  SetCs
}

void ili9320_SetWindows(uint16_t StartX,uint16_t StartY,uint16_t width,uint16_t heigh)
{
	uint16_t s_h,s_l, e_h, e_l;
	
	uint16_t xEnd, yEnd;
	xEnd = StartX + width;
  yEnd = StartY + heigh-1;
  if(DeviceCode==0x8989)
  {
  	
	/*LCD_WriteReg(0x0044, (StartX & 0xff) | (xEnd << 8));
	 LCD_WriteReg(0x0045, StartY);
	 LCD_WriteReg(0x0046, yEnd);*/
	 LCD_WriteReg(0x0044, (StartY& 0xff) | (yEnd << 8));
	 LCD_WriteReg(0x0045, StartX);
	 LCD_WriteReg(0x0046, xEnd);
    
  }
	else if(DeviceCode == 0X9488)
	{
	 	s_h = (StartX >> 8) & 0X00ff;
		s_l = StartX & 0X00ff;
		e_h = ((StartX + width - 1) >> 8) & 0X00ff;
		e_l = (StartX + width - 1) & 0X00ff;
		
		LCD_IO_WriteReg(0x002A);
		LCD_IO_WriteData(s_h);
		LCD_IO_WriteData(s_l);
		LCD_IO_WriteData(e_h);
		LCD_IO_WriteData(e_l);

		s_h = (StartY >> 8) & 0X00ff;
		s_l = StartY & 0X00ff;
		e_h = ((StartY + heigh - 1) >> 8) & 0X00ff;
		e_l = (StartY + heigh - 1) & 0X00ff;
		
		LCD_IO_WriteReg(0x002B);
		LCD_IO_WriteData(s_h);
		LCD_IO_WriteData(s_l);
		LCD_IO_WriteData(e_h);
		LCD_IO_WriteData(e_l);		
	}	
  else if((DeviceCode==0x9325)||(DeviceCode==0x9328)||(DeviceCode==0x1505))
  {
	 /* LCD_WriteReg(0x0050, StartX);
	  LCD_WriteReg(0x0052, StartY);
	  LCD_WriteReg(0x0051, xEnd);
	  LCD_WriteReg(0x0053, yEnd);*/
	  LCD_WriteReg(0x0050,StartY);        //Specify the start/end positions of the window address in the horizontal direction by an address unit
		LCD_WriteReg(0x0051,yEnd);        //Specify the start positions of the window address in the vertical direction by an address unit 
		LCD_WriteReg(0x0052,320 - xEnd); 
		LCD_WriteReg(0x0053,320 - StartX - 1);        //Specify the end positions of the window address in the vertical direction by an address unit
	
  }	
	else
	 {
	 	s_h = (StartX >> 8) & 0Xff;
		s_l = StartX & 0Xff;
		e_h = ((StartX + width - 1) >> 8) & 0Xff;
		e_l = (StartX + width - 1) & 0Xff;
		
		LCD_IO_WriteReg(0x2A);
		LCD_IO_WriteData(s_h);
		LCD_IO_WriteData(s_l);
		LCD_IO_WriteData(e_h);
		LCD_IO_WriteData(e_l);

		s_h = (StartY >> 8) & 0Xff;
		s_l = StartY & 0Xff;
		e_h = ((StartY + heigh - 1) >> 8) & 0Xff;
		e_l = (StartY + heigh - 1) & 0Xff;
		
		LCD_IO_WriteReg(0x2B);
		LCD_IO_WriteData(s_h);
		LCD_IO_WriteData(s_l);
		LCD_IO_WriteData(e_h);
		LCD_IO_WriteData(e_l);
	 }
}

void LCD_Clear(uint16_t  Color)
{
  uint32_t index=0;

  
  unsigned int count; 
	
	if(DeviceCode ==0x9488)
	{
		tft_set_cursor(0,0);
    ili9320_SetWindows(0,0,480,320);
		LCD_WriteRAM_Prepare();
		//index = (160*480);
    for(index=0;index<320*480;index++)
    {
        LCD_IO_WriteData(Color);
    }
    //LCD_IO_WriteMultiple(Color, (480*320));
    //while(index --)
		//LCD_IO_WriteData(Color);
	}
	else if(DeviceCode == 0x5761)
	{
	    LCD_IO_WriteReg(0x002a);	
	    LCD_IO_WriteData(0);	    
	    LCD_IO_WriteData(0);
	    LCD_IO_WriteData(HDP>>8);	    
	    LCD_IO_WriteData(HDP&0x00ff);
	    LCD_IO_WriteReg(0x002b);	
	    LCD_IO_WriteData(0);	    
	    LCD_IO_WriteData(0);
	    LCD_IO_WriteData(VDP>>8);	    
	    LCD_IO_WriteData(VDP&0x00ff);
	    LCD_IO_WriteReg(0x002c);	
	    LCD_IO_WriteReg(0x002c);
	    for(count=0;count<(HDP+1)*(VDP+1);count++)
			{
	       LCD_IO_WriteData(Color);
			}
	}
	else
	{
		  tft_set_cursor(0,0); 
		  LCD_WriteRAM_Prepare(); /* Prepare to write GRAM */
		  for(index=0;index<76800;index++)
		  {
		     LCD_IO_WriteData(Color);
		  }		
	}
}

extern uint16_t ILI9488_ReadRAM();


void init_tft()
{
	uint16_t i;
	//************* Start Initial Sequence **********//
	
    LCD_IO_Init(FSMC_CS_PIN, FSMC_RS_PIN);
	
	_delay_ms(5);
	
	LCD_IO_WriteReg(0X00D3);				   
	DeviceCode=ILI9488_ReadRAM();	//dummy read 	
	DeviceCode=ILI9488_ReadRAM();	
	DeviceCode=ILI9488_ReadRAM();   								   
	DeviceCode<<=8;
	DeviceCode|=ILI9488_ReadRAM();  	
	
	if(DeviceCode == 0x9488)
	{
		LCD_IO_WriteReg(0x00E0); 
		LCD_IO_WriteData(0x0000); 
		LCD_IO_WriteData(0x0007); 
		LCD_IO_WriteData(0x000f); 
		LCD_IO_WriteData(0x000D); 
		LCD_IO_WriteData(0x001B); 
		LCD_IO_WriteData(0x000A); 
		LCD_IO_WriteData(0x003c); 
		LCD_IO_WriteData(0x0078); 
		LCD_IO_WriteData(0x004A); 
		LCD_IO_WriteData(0x0007); 
		LCD_IO_WriteData(0x000E); 
		LCD_IO_WriteData(0x0009); 
		LCD_IO_WriteData(0x001B); 
		LCD_IO_WriteData(0x001e); 
		LCD_IO_WriteData(0x000f);  

		LCD_IO_WriteReg(0x00E1); 
		LCD_IO_WriteData(0x0000); 
		LCD_IO_WriteData(0x0022); 
		LCD_IO_WriteData(0x0024); 
		LCD_IO_WriteData(0x0006); 
		LCD_IO_WriteData(0x0012); 
		LCD_IO_WriteData(0x0007); 
		LCD_IO_WriteData(0x0036); 
		LCD_IO_WriteData(0x0047); 
		LCD_IO_WriteData(0x0047); 
		LCD_IO_WriteData(0x0006); 
		LCD_IO_WriteData(0x000a); 
		LCD_IO_WriteData(0x0007); 
		LCD_IO_WriteData(0x0030); 
		LCD_IO_WriteData(0x0037); 
		LCD_IO_WriteData(0x000f); 

		LCD_IO_WriteReg(0x00C0); 
		LCD_IO_WriteData(0x0010); 
		LCD_IO_WriteData(0x0010); 

		LCD_IO_WriteReg(0x00C1); 
		LCD_IO_WriteData(0x0041); 

		LCD_IO_WriteReg(0x00C5); 
		LCD_IO_WriteData(0x0000); 
		LCD_IO_WriteData(0x0022); 
		LCD_IO_WriteData(0x0080); 

		LCD_IO_WriteReg(0x0036); 
		//ILI9488_WriteData(0x0068);
		//if(gCfgItems.overturn_180 != 0xEE)
		//{
			LCD_IO_WriteData(0x0068); 
		//}
		//else
		//{
			//ILI9488_WriteData(0x00A8);
		//}

		LCD_IO_WriteReg(0x003A); //Interface Mode Control
		LCD_IO_WriteData(0x0055);

		LCD_IO_WriteReg(0X00B0);  //Interface Mode Control  
		LCD_IO_WriteData(0x0000); 
		LCD_IO_WriteReg(0x00B1);   //Frame rate 70HZ  
		LCD_IO_WriteData(0x00B0); 
		LCD_IO_WriteData(0x0011); 
		LCD_IO_WriteReg(0x00B4); 
		LCD_IO_WriteData(0x0002);   
		LCD_IO_WriteReg(0x00B6); //RGB/MCU Interface Control
		LCD_IO_WriteData(0x0002); 
		LCD_IO_WriteData(0x0042); 

		LCD_IO_WriteReg(0x00B7); 
		LCD_IO_WriteData(0x00C6); 

		//WriteComm(0XBE);
		//WriteData(0x00);
		//WriteData(0x04);

		LCD_IO_WriteReg(0x00E9); 
		LCD_IO_WriteData(0x0000);

		LCD_IO_WriteReg(0X00F7);    
		LCD_IO_WriteData(0x00A9); 
		LCD_IO_WriteData(0x0051); 
		LCD_IO_WriteData(0x002C); 
		LCD_IO_WriteData(0x0082);

		LCD_IO_WriteReg(0x0011); 
		for(i=0;i<65535;i++);
		LCD_IO_WriteReg(0x0029); 	

		ili9320_SetWindows(0,0,480,320);
		LCD_Clear(0x0000);
		
	      OUT_WRITE(LCD_BACKLIGHT_PIN, HIGH);
	}

}
#endif

extern uint8_t bmp_public_buf[17 * 1024];
void tft_lvgl_init()
{
	//uint16_t test_id=0;
    W25QXX.init(SPI_QUARTER_SPEED);
    //test_id=W25QXX.W25QXX_ReadID();
    #if ENABLED (SDSUPPORT)
    card.mount();
    UpdatePic();
    UpdateFont();
    mks_test_get();
    #endif
    gCfgItems_init();
	ui_cfg_init();
    disp_language_init();
    //spi_flash_read_test();

	#if ENABLED(SPI_GRAPHICAL_TFT)
       SPI_TFT.spi_init(SPI_FULL_SPEED);
	SPI_TFT.LCD_init();
	#else
	init_tft();
	#endif
	
    lv_init();	

    lv_disp_buf_init(&disp_buf, bmp_public_buf, NULL, LV_HOR_RES_MAX * 18);		/*Initialize the display buffer*/

    lv_disp_drv_t disp_drv;				/*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);			/*Basic initialization*/
    disp_drv.flush_cb = my_disp_flush;	/*Set your driver function*/
    disp_drv.buffer = &disp_buf;			/*Assign the buffer to the display*/
    lv_disp_drv_register(&disp_drv);		/*Finally register the driver*/

    lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);			 /*Descriptor of a input device driver*/
    indev_drv.type = LV_INDEV_TYPE_POINTER;	 /*Touch pad is a pointer-like device*/
    indev_drv.read_cb = my_touchpad_read; 	 /*Set your driver function*/
    lv_indev_drv_register(&indev_drv);		 /*Finally register the driver*/

	systick_attach_callback(SysTick_Callback);

	init_gb2312_font();

    tft_style_init();
	
	filament_pin_setup();

    #if ENABLED(POWER_LOSS_RECOVERY)
    if((recovery.info.valid_head != 0) && 
	(recovery.info.valid_head == recovery.info.valid_foot))
    {
    	if(gCfgItems.from_flash_pic == 1)
			flash_preview_begin = 1;
		else
			default_preview_flg = 1;
		
	    uiCfg.print_state = REPRINTING;

		memset(public_buf_m,0,sizeof(public_buf_m));
		strncpy(public_buf_m,recovery.info.sd_filename,sizeof(public_buf_m));
		card.printLongPath(public_buf_m);
		
		strncpy(list_file.long_name[sel_id],card.longFilename,sizeof(list_file.long_name[sel_id]));
		
		lv_draw_printing();
    }
    else
    #endif
    lv_draw_ready_print();
	
	if(mks_test_flag == 0x1e)
	{
		mks_gpio_test();
	}
}



#if 0
void LCD_WriteRAM(uint16_t RGB_Code)					 
{
    #if 0
    ClrCs
    /* Write 16-bit GRAM Reg */
    LCD->LCD_RAM = RGB_Code;
    SetCs
    #else
    LCD_IO_WriteData(RGB_Code);
    #endif
}
#endif




#if ENABLED(SPI_GRAPHICAL_TFT)
void my_disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
	uint16_t i,width,height;
	uint16_t Color;
	lv_color_t *colorIndex;
	//uint8_t tbuf[480*2];
	
	SPI_TFT.spi_init(SPI_FULL_SPEED);
	
    width = area->x2 - area->x1 + 1;
    height = area->y2 - area->y1 +1;
	#if 0
	for(int j=0;j<height;j++)
	{
		SPI_TFT.SetCursor(0,0);
    	SPI_TFT.SetWindows((uint16_t)area->x1,(uint16_t)area->y1+j,width,1);
    	SPI_TFT.LCD_WriteRAM_Prepare();	
		
		for(i=0;i<width*2;)
		{
	    	/*clr_temp = (uint16_t)(((uint16_t)color_p->ch.red<<11)
							|((uint16_t)color_p->ch.green<<5)
							|((uint16_t)color_p->ch.blue));	*/
			
			tbuf[i]=color_p->full >> 8;
			tbuf[i+1]=color_p->full;
			i+=2;
			color_p++;
		}
		SPI_TFT_CS_L;
		SPI_TFT_DC_H;
		SPI.dmaSend(tbuf,width*2,true);
		SPI_TFT_CS_H;	
	}
	#else
	SPI_TFT.SetCursor(0,0);
    	SPI_TFT.SetWindows((uint16_t)area->x1,(uint16_t)area->y1,width,height);
    	SPI_TFT.LCD_WriteRAM_Prepare();
	
	//for(int j=0;j<height;j++)
	//{
			
		
		//for(i=0;i<width*2;)
		//{
			//tbuf[i]=color_p->full >> 8;
			//tbuf[i+1]=color_p->full;
			//i+=2;
			//color_p++;
		//}
		colorIndex = color_p;
		//if(cur_pic.got_addr == 0)
		//{
			for(i=0;i<width*height;)
			{
			       Color = (color_p->full >> 8);
				color_p->full = Color | ((color_p->full & 0xff) << 8);
				color_p++;
				i++;
			}
		//}
		SPI_TFT_CS_L;
		SPI_TFT_DC_H;
		SPI.dmaSend(colorIndex,width*height*2,true);
		SPI_TFT_CS_H;	
	//}
	#endif
	lv_disp_flush_ready(disp);         /* Indicate you are ready with the flushing*/

	W25QXX.init(SPI_QUARTER_SPEED);
}
#else
void my_disp_flush(lv_disp_drv_t * disp, const lv_area_t * area, lv_color_t * color_p)
{
    #if 1
    uint16_t i,width,height;
    uint16_t clr_temp;
    #if 0
    int32_t x,y;
    for(y = area->y1; y <= area->y2; y++) {
        for(x = area->x1; x <= area->x2; x++) {
            //set_pixel(x, y, *color_p);  /* Put a pixel to the display.*/
			clr_temp = (uint16_t)(((uint16_t)color_p->ch.red<<11)
						|((uint16_t)color_p->ch.green<<5)
						|((uint16_t)color_p->ch.blue));
			tft_set_point(x, y,clr_temp);            
            color_p++;
        }
    }
    #else
    width = area->x2 - area->x1 + 1;
    height = area->y2 - area->y1 +1;
    //tft_set_cursor((uint16_t)area->x1,(uint16_t)area->y1);
    ili9320_SetWindows((uint16_t)area->x1,(uint16_t)area->y1,width,height);
    LCD_WriteRAM_Prepare();
    for(i=0;i<(width*height);i++)
    {
    	  /*clr_temp = (uint16_t)(((uint16_t)color_p->ch.red<<11)
						|((uint16_t)color_p->ch.green<<5)
						|((uint16_t)color_p->ch.blue));*/
        LCD_IO_WriteData(color_p->full);
	 color_p++;
    }
    #endif
	
    lv_disp_flush_ready(disp);         /* Indicate you are ready with the flushing*/
    #endif
}
#endif


#define TICK_CYCLE 1

static int32_t touch_time1 = 0;

unsigned int  getTickDiff(unsigned int curTick, unsigned int  lastTick)
{
	if(lastTick <= curTick)
	{
		return (curTick - lastTick) * TICK_CYCLE;
	}
	else
	{
		return (0xffffffff - lastTick + curTick) * TICK_CYCLE;
	}
}

#if ENABLED(SPI_GRAPHICAL_TFT)

#ifndef USE_XPT2046
#define USE_XPT2046         1
#define XPT2046_XY_SWAP  	1
#define XPT2046_X_INV   	1
#define XPT2046_Y_INV   	0
#endif

#if USE_XPT2046
#define XPT2046_HOR_RES     480
#define XPT2046_VER_RES     320
#define XPT2046_X_MIN       201  
#define XPT2046_Y_MIN       164
#define XPT2046_X_MAX       3919 
#define XPT2046_Y_MAX       3776
#define XPT2046_AVG         4
#define XPT2046_INV         1
#endif

#else
#ifndef USE_XPT2046
#define USE_XPT2046         1
#define XPT2046_XY_SWAP  	1
#define XPT2046_X_INV   	0
#define XPT2046_Y_INV   	1
#endif

#if USE_XPT2046
#define XPT2046_HOR_RES     480
#define XPT2046_VER_RES     320
#define XPT2046_X_MIN       201 
#define XPT2046_Y_MIN       164
#define XPT2046_X_MAX       3919
#define XPT2046_Y_MAX       3776
#define XPT2046_AVG         4
#define XPT2046_INV         0
#endif
#endif

static void xpt2046_corr(uint16_t * x, uint16_t * y)
{
#if XPT2046_XY_SWAP     
	int16_t swap_tmp;    
	swap_tmp = *x;    
	*x = *y;    
	*y = swap_tmp;
#endif    
	if((*x) > XPT2046_X_MIN)
		(*x) -= XPT2046_X_MIN;    
	else
		(*x) = 0;    
	if((*y) > XPT2046_Y_MIN)
		(*y) -= XPT2046_Y_MIN;    
	else
		(*y) = 0;    
	#if LV_USE_ROTATION_180
	(*x) = XPT2046_HOR_RES - (uint32_t)((uint32_t)(*x) * XPT2046_HOR_RES)/(XPT2046_X_MAX - XPT2046_X_MIN);    
	(*y) = XPT2046_VER_RES - (uint32_t)((uint32_t)(*y) * XPT2046_VER_RES)/(XPT2046_Y_MAX - XPT2046_Y_MIN);
	#else
	(*x) = (uint32_t)((uint32_t)(*x) * XPT2046_HOR_RES)/(XPT2046_X_MAX - XPT2046_X_MIN);    
	(*y) = (uint32_t)((uint32_t)(*y) * XPT2046_VER_RES)/(XPT2046_Y_MAX - XPT2046_Y_MIN);
	#endif
#if XPT2046_X_INV     
	(*x) =  XPT2046_HOR_RES - (*x);
#endif
#if XPT2046_Y_INV     
	(*y) =  XPT2046_VER_RES - (*y);
#endif
}

#define  times  8
#define	CHX 	0x90//0x90 
#define	CHY 	0xD0//0xd0

int SPI2_ReadWrite2Bytes(void)  
{
	volatile uint16_t ans=0;
        uint16_t temp = 0;
	#if ENABLED(SPI_GRAPHICAL_TFT)
	temp=SPI_TFT.spi_read_write_byte(0xff);
	ans=temp<<8;
	temp=SPI_TFT.spi_read_write_byte(0xff);
	ans|=temp;
	ans>>=3;	
	#else
	temp=W25QXX.spi_flash_read_write_byte(0xff);
	ans=temp<<8;
	temp=W25QXX.spi_flash_read_write_byte(0xff);
	ans|=temp;
	ans>>=3;
	#endif
	return ans&0x0fff;
}

uint16_t		x_addata[times],y_addata[times];
void XPT2046_Rd_Addata(uint16_t *X_Addata,uint16_t *Y_Addata)
{

	uint16_t		i,j,k;
    //int result;

       //#if ENABLED(TOUCH_BUTTONS)
	   
	#if ENABLED(SPI_GRAPHICAL_TFT)
	SPI_TFT.spi_init(SPI_SPEED_6);
	#else
	W25QXX.init(SPI_SPEED_6);
	#endif
	
	for(i=0;i<times;i++)					
	{
		#if ENABLED(SPI_GRAPHICAL_TFT)
		OUT_WRITE(TOUCH_CS_PIN, LOW);
		SPI_TFT.spi_read_write_byte(CHX);
		y_addata[i] = SPI2_ReadWrite2Bytes();
		WRITE(TOUCH_CS_PIN, HIGH);
		
		OUT_WRITE(TOUCH_CS_PIN, LOW);
		SPI_TFT.spi_read_write_byte(CHY);
		x_addata[i] = SPI2_ReadWrite2Bytes(); 
		WRITE(TOUCH_CS_PIN, HIGH);		
		#else
			//#if ENABLED(TOUCH_BUTTONS)
			OUT_WRITE(TOUCH_CS_PIN, LOW);
			W25QXX.spi_flash_read_write_byte(CHX);
			y_addata[i] = SPI2_ReadWrite2Bytes();
			WRITE(TOUCH_CS_PIN, HIGH);
			
			OUT_WRITE(TOUCH_CS_PIN, LOW);
			W25QXX.spi_flash_read_write_byte(CHY);
			x_addata[i] = SPI2_ReadWrite2Bytes(); 
			WRITE(TOUCH_CS_PIN, HIGH);
			//#endif
		#endif

	}
	#if DISABLED(SPI_GRAPHICAL_TFT)
	W25QXX.init(SPI_QUARTER_SPEED);
	#endif
	//#endif
	//result = x_addata[0];
	for(i=0;i<times;i++)					
	{
		for(j = i + 1; j < times; j++)
		{
			if(x_addata[j] > x_addata[i])
			{
				k = x_addata[j];
				x_addata[j] = x_addata[i];
				x_addata[i] = k;
			}
		}
	}
	if(x_addata[times / 2 -1] - x_addata[times / 2 ] > 50)
	{
            *X_Addata = 0;
            *Y_Addata = 0;
            return ;
        }

	*X_Addata = (x_addata[times / 2 -1] + x_addata[times / 2 ]) /2;

	
	//result = y_addata[0];
	for(i=0;i<times;i++)					
	{
		for(j = i + 1; j < times; j++)
		{
			if(y_addata[j] > y_addata[i])
			{
				k = y_addata[j];
				y_addata[j] = y_addata[i];
				y_addata[i] = k;
			}
		}
	}

	
	if(y_addata[times / 2 -1] - y_addata[times / 2 ] > 50)
	{
            *X_Addata = 0;
            *Y_Addata = 0;
            return ;
        }

	*Y_Addata = (y_addata[times / 2 -1] + y_addata[times / 2 ]) /2;
	
	
}

#define ADC_VALID_OFFSET	30

uint8_t	TOUCH_PressValid(uint16_t _usX, uint16_t _usY)
{
	if ((_usX <= ADC_VALID_OFFSET) || (_usY <= ADC_VALID_OFFSET)
		|| (_usX >= 4095 - ADC_VALID_OFFSET)
		|| (_usY >= 4095 - ADC_VALID_OFFSET))
	{
		return 0;
	}
	else
	{
		return 1;
	}
}

static lv_coord_t last_x = 0;
   static lv_coord_t last_y = 0;
bool my_touchpad_read(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
    #if 1
	uint32_t tmpTime, diffTime = 0;

     

	tmpTime = millis();
	diffTime = getTickDiff(tmpTime, touch_time1);
    /*Save the state and save the pressed coordinate*/
    //data->state = TOUCH_PressValid(last_x, last_y) ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL; 
    //if(data->state == LV_INDEV_STATE_PR)  ADS7843_Rd_Addata((u16 *)&last_x, (u16 *)&last_y);
		//touchpad_get_xy(&last_x, &last_y);
    /*Save the pressed coordinates and the state*/
	if(diffTime > 4)
	{
		XPT2046_Rd_Addata((uint16_t *)&last_x, (uint16_t *)&last_y);
	    if(TOUCH_PressValid(last_x, last_y)) {
	        
	        data->state = LV_INDEV_STATE_PR;
			
		    /*Set the coordinates (if released use the last pressed coordinates)*/
	
			xpt2046_corr((uint16_t *)&last_x, (uint16_t *)&last_y);
		    data->point.x = last_x;
		    data->point.y = last_y;			
			
	    } else {
	        data->state = LV_INDEV_STATE_REL;
	    }  
	    touch_time1 = tmpTime;
	}

    return false; /*Return `false` because we are not buffering and no more data to read*/
    #endif
}

#endif