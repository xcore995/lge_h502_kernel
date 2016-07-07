/*
 * Copyright (C) 2013 LG Electironics, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/****************************************************************************
* Include Files
****************************************************************************/
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/bitops.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/byteorder/generic.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif 
#include <linux/interrupt.h>
#include <linux/input/mt.h>
#include <linux/time.h>
#include <linux/rtpm_prio.h>
#include <linux/proc_fs.h>
#include <linux/jiffies.h>
#include <linux/spinlock.h>
#include <linux/wakelock.h>
#include <linux/dma-mapping.h>

#include <mach/wd_api.h>
#include <mach/eint.h>
#include <mach/mt_wdt.h>
#include <mach/mt_gpt.h>
#include <mach/mt_reg_base.h>
#include <mach/mt_pm_ldo.h>
#include <mach/mt_typedefs.h>
#include <mach/mt_boot.h>

#include <asm/uaccess.h>
#include <cust_eint.h>

#include "tpd.h"
#include "s7020_driver.h"

#define LGE_USE_SYNAPTICS_FW_UPGRADE
#if defined(LGE_USE_SYNAPTICS_FW_UPGRADE)
#include <linux/workqueue.h>
#include "s7020_fw.h"
#endif

#define LGE_USE_SYNAPTICS_F54
#if defined(LGE_USE_SYNAPTICS_F54)
#include "./RefCode.h"
#include "./RefCode_PDTScan.h"

#endif



/****************************************************************************
* Feature
****************************************************************************/
#define LGE_USE_PROXI_SENSOR
#define ONLY_S7020_RESET_PIN
#define WXSERIES_FW
#define WORKQUEUE_FW
//#define MT_PROTOCOL_A
#define MAX_RESET_CNT 3
//                                          

/****************************************************************************
* Constants / Definitions
****************************************************************************/
#define LGE_TOUCH_NAME						"lge_touch"
#define	TPD_DEV_NAME						"synaptics_S7020"
#define TPD_I2C_ADDRESS						0x20

#define BUFFER_SIZE							128

#define MAX_NUM_OF_FINGER					10

#define X_POSITION							0
#define Y_POSITION							1
#define XY_POSITION							2
#define WX_WY								3
#define PRESSURE							4
#define NUM_OF_EACH_FINGER_DATA_REG			5

#define MAX_POINT_SIZE_FOR_LPWG				12

#define TOUCH_PRESSED						1
#define TOUCH_RELEASED						0
#define BUTTON_CANCLED						0xFF


#define PAGE_MAX_NUM						5
#define PAGE_SELECT_REG						0xFF
#define DESCRIPTION_TABLE_START				0xE9

#define RMI_DEVICE_CONTROL					0x01
#define TOUCHPAD_SENSORS					0x11
#define CAPACITIVE_BUTTON_SENSORS			0x1A
#define FLASH_MEMORY_MANAGEMENT				0x34
#define MULTI_TAP_GESTURE					0x51

/* Function $01 (RMI_DEVICE_CONTROL) */
#define MANUFACTURER_ID_REG					(device_control.query_base)
#define FW_REVISION_REG						(device_control.query_base+3)
#define PRODUCT_ID_REG						(device_control.query_base+11)

#define DEVICE_CONTROL_REG					(device_control.control_base)
#define NORMAL_OPERATION_MASK				0x00
#define SENSOR_SLEEP_MASK					0x01
#define NOSLEEP_MASK						0x04
#define CONFIGURED_MASK						0x80
#define INTERRUPT_ENABLE_REG				(device_control.control_base+1)
#define ABS0_MASK							0x04
#define BUTTON_MASK							0x10

#define DEVICE_STATUS_REG					(device_control.data_base)
#define DEVICE_FAILURE_MASK					0x03
#define FW_CRC_FAILURE_MASK					0x04
#define FLASH_PROG_MASK						0x40
#define UNCONFIGURED_MASK					0x80
#define INTERRUPT_STATUS_REG				(device_control.data_base+1)

/* Function $11 (TOUCHPAD_SENSORS) */
#define TWO_D_COMMAND						(finger.command_base)

#define TWO_D_REPORT_MODE_REG				(finger.control_base)
#define CONTINUOUS_REPORTING_MODE_MASK		0x0
#define REDUCED_REPORTING_MODE_MASK			0x01
#define WAKEUP_GESTURE_REPORTING_MODE_MASK	0x04
#define ABS_POS_FILTER_MASK					0x08
#define REPORT_BEYOND_CLIP_MASK				0x80
#define TWO_D_DELTA_X_THRESH_REG			(finger.control_base+2)
#define TWO_D_DELTA_Y_THRESH_REG			(finger.control_base+3)
#define DELTA_POS_THRESHOLD					1
#define LPWG_CONTROL_REG					(finger.control_base+44)

#define TWO_D_FINGER_STATE_REG				(finger.data_base)
#define FINGER_STATE_MASK					0x03
#define TWO_D_EXTENDED_STATUS_REG			(finger.data_base+53)

/* Function $1A (CAPACITIVE_BUTTON_SENSORS) */
#define BUTTON_DATA_REG						(button.data_base)


/* Function $34 (FLASH_MEMORY_MANAGEMENT) */
#define FLASH_CONFIG_ID_REG					(flash_memory.control_base)
#define FLASH_CONTROL_REG					(flash_memory.data_base+18)
#define FLASH_STATUS_MASK					0xF0
#define F34_FUNCTION_EXIST					0x00EE


/* Function $51 (MULTI_TAP_GESTURE) */
#define MULTITAP_ENABLE_REG					(multi_tap.control_base+22)
#define ENABLE_MULTITAP_REPORTING_MASK		0x01
#define MULTITAP_COUNT_REG					(multi_tap.control_base+22)
#define MAXIMUM_INTERTAP_TIME_REG			(multi_tap.control_base+24)
#define MAXIMUM_INTERTAP_DISTANCE_REG		(multi_tap.control_base+26)

#define GESTURE_STATUS_REG					(multi_tap.data_base)
#define GESTURE_PROPERTY_REG				(multi_tap.data_base+1)

/*F51 Others */
#define LPWG_STATUS_REG						0x00 // 4-page
#define LPWG_DATA_REG						0x01 // 4-page
#define LPWG_TAPCOUNT_REG					0x47 // 4-page

#define LPWG_MIN_INTERTAP_REG				0x48 // 4-page
#define LPWG_MAX_INTERTAP_REG				0x49 // 4-page
#define LPWG_TOUCH_SLOP_REG					0x4A // 4-page
#define LPWG_TAP_DISTANCE_REG				0x4B // 4-page

/*LPWG Control Value*/
#define REPORT_MODE_CTRL	1
#define TCI_ENABLE_CTRL		2
#define TAP_COUNT_CTRL		3
#define MIN_INTERTAP_CTRL	4
#define MAX_INTERTAP_CTRL	5
#define TOUCH_SLOP_CTRL		6
#define TAP_DISTANCE_CTRL	7
#define INTERRUPT_DELAY_CTRL   8

#define TCI_ENABLE_CTRL2	22
#define TAP_COUNT_CTRL2		23
#define MIN_INTERTAP_CTRL2	24
#define MAX_INTERTAP_CTRL2	25
#define TOUCH_SLOP_CTRL2	26
#define TAP_DISTANCE_CTRL2	27
#define INTERRUPT_DELAY_CTRL2   28

#define TPD_HAVE_BUTTON

#ifdef TPD_HAVE_BUTTON
#ifdef LGE_USE_DOME_KEY
#define TPD_KEY_COUNT	2
static int tpd_keys_local[TPD_KEY_COUNT] = {KEY_BACK , KEY_MENU};
#else
#define TPD_KEY_COUNT	3
static int tpd_keys_local[TPD_KEY_COUNT] = {KEY_BACK, KEY_MENU, KEY_HOMEPAGE};
#endif
#endif

/****************************************************************************
* Macros
****************************************************************************/
#define GET_X_POSITION(high, low)		((int)(high<<4)|(int)(low&0x0F))
#define GET_Y_POSITION(high, low)		((int)(high<<4)|(int)((low&0xF0)>>4))

#define get_time_interval(a,b)		a>=b ? a-b : 1000000+a-b
#define jitter_abs(x)				(x > 0 ? x : -x)
#define jitter_sub(x, y)			(x > y ? x - y : y - x)

#define MS_TO_NS(x)		(x * 1E6L)


/*Do Function Macro*/
#define DO_IF(do_work, goto_error) 								\
	do {												\
		if(do_work){										\
			TPD_ERR("[Touch E] Action Failed");	\
			goto goto_error;				\
		}							\
	} while (0)

#define DO_SAFE(do_work, goto_error) 				\
		DO_IF(unlikely((do_work) < 0), goto_error)
#define LPWG_PAGE				0x04
#define INTERRUPT_MASK_ABS0			0x04


/****************************************************************************
* Type Definitions
****************************************************************************/
typedef struct {
	u8 query_base;
	u8 command_base;
	u8 control_base;
	u8 data_base;
	u8 int_source_count;
	u8 function_exist;
} function_descriptor;

typedef struct {
	unsigned char finger_state[3];
	unsigned char finger_data[MAX_NUM_OF_FINGER][NUM_OF_EACH_FINGER_DATA_REG];
} touch_sensor_data;

typedef struct {
	unsigned int pos_x[MAX_NUM_OF_FINGER];
	unsigned int pos_y[MAX_NUM_OF_FINGER];
	unsigned char pressure[MAX_NUM_OF_FINGER];
	int total_num;
	char palm;
} touch_finger_info;

typedef struct {
	unsigned int booting;
	unsigned int reset;
} touch_delay;

enum {
	IGNORE_INTERRUPT = 100,
	NEED_TO_OUT,
	NEED_TO_INIT,
};

enum {
	IC_CTRL_CODE_NONE = 0,
	IC_CTRL_BASELINE,
	IC_CTRL_READ,
	IC_CTRL_WRITE,
	IC_CTRL_RESET_CMD,
	IC_CTRL_REPORT_MODE,
	IC_CTRL_DOUBLE_TAP_WAKEUP_MODE,
};

enum {
	BASELINE_OPEN = 0,
	BASELINE_FIX,
	BASELINE_REBASE,
};

enum {
	TIME_EX_INIT_TIME,
	TIME_EX_FIRST_INT_TIME,
	TIME_EX_PREV_PRESS_TIME,
	TIME_EX_CURR_PRESS_TIME,
	TIME_EX_BUTTON_PRESS_START_TIME,
	TIME_EX_BUTTON_PRESS_END_TIME,
	TIME_EX_FIRST_GHOST_DETECT_TIME,
	TIME_EX_SECOND_GHOST_DETECT_TIME,
	TIME_EX_CURR_INT_TIME,
	TIME_EX_PROFILE_MAX
};

enum {
	LPWG_NONE = 0,
	LPWG_DOUBLE_TAP,
	LPWG_MULTI_TAP,
	LPWG_ACTIVE_MODE,
};

enum{
    LPWG_READ = 1,
    LPWG_ENABLE,
    LPWG_LCD_X,
    LPWG_LCD_Y,
    LPWG_ACTIVE_AREA_X1,
    LPWG_ACTIVE_AREA_X2,
    LPWG_ACTIVE_AREA_Y1,
    LPWG_ACTIVE_AREA_Y2,
    LPWG_TAP_COUNT,
    LPWG_REPLY,
};

enum {
	IC_POWER_OFF = 0,
	IC_POWER_ON,
};
struct point {
    int x;
    int y;
};

struct st_i2c_msgs {
	struct i2c_msg *msg;
	int count;
} i2c_msgs;

struct foo_obj{
	struct kobject kobj;
	int interrupt;
};

static struct foo_obj *foo_obj;


/****************************************************************************
* Variables
****************************************************************************/
static function_descriptor device_control;
static function_descriptor finger;
static function_descriptor button;
static function_descriptor flash_memory;
static function_descriptor multi_tap;

u8 device_control_page;
u8 finger_page;
u8 button_page;
u8 flash_memory_page;
u8 multi_tap_page;

u8 button_enable_mask;

touch_finger_info pre_touch_info;
touch_delay delay_time;
char button_prestate[TPD_KEY_COUNT];
char finger_prestate[MAX_NUM_OF_FINGER];

static bool Power_status=0;
static int Power_status_cnt=0;
char knock_on_type;

static int knock_on_enable = 0;

/*Knock on/code parameter*/
u8 double_tap_enable = 0;
u8 multi_tap_enable = 0;
u8 multi_tap_count;
u8 lpwg_mode = 0;
u8 screen = 1;
u8 sensor = 1;
u8 qcover = 0;
u8 case_mode = 1;
u32 Double_Tap_Area_X1 = 0;
u32 Double_Tap_Area_X2 = 0;
u32 Double_Tap_Area_Y1 = 0;
u32 Double_Tap_Area_Y2 = 0;
u32 lcd_touch_ratio_x;
u32 lcd_touch_ratio_y;

static u8 custom_gesture_status = 0;
static u8 gesture_property[MAX_POINT_SIZE_FOR_LPWG*4+1] = {0};
static struct point lpwg_data[MAX_POINT_SIZE_FOR_LPWG+1];
static char *lpwg_uevent[2][2] = {
	{ "TOUCH_GESTURE_WAKEUP=WAKEUP", NULL },
	{ "TOUCH_GESTURE_WAKEUP=PASSWORD", NULL }
};
struct wake_lock knock_code_lock;
struct hrtimer multi_tap_timer;
struct work_struct multi_tap_work;
struct workqueue_struct* touch_multi_tap_wq;

//                      
struct workqueue_struct* notify_work_wq;
struct work_struct notify_work;
struct hrtimer notify_timer;
struct wake_lock notify_lock;
struct workqueue_struct* knock_set_work_wq;
struct delayed_work knock_set_work;
static int g_reset_cnt=MAX_RESET_CNT;

/* ghost finger detection */
int x_max;
int	y_max;
static int pressure_zero = 0;
touch_finger_info old_touch_info;
char button_oldstate[TPD_KEY_COUNT];
char finger_oldstate[MAX_NUM_OF_FINGER];

#if defined(LGE_USE_SYNAPTICS_FW_UPGRADE)
char fw_path[256];
unsigned char *fw_start;
unsigned long fw_size;
bool fw_force_update;
bool download_status;		/* avoid power off during F/W upgrade */
bool suspend_status;		/* avoid power off during F/W upgrade */
bool key_lock_status;
struct wake_lock fw_suspend_lock;
struct device *touch_fw_dev;
struct work_struct work_fw_upgrade;
#endif


#if defined(LGE_USE_SYNAPTICS_F54)
struct i2c_client *ds4_i2c_client;
static int f54_fullrawcap_mode = 0;
struct device *touch_debug_dev;
#endif

static u8* I2CDMABuf_va = NULL;
static u32 I2CDMABuf_pa = NULL;


extern struct tpd_device *tpd;
struct i2c_client *tpd_i2c_client = NULL;
static int tpd_flag = 0;

struct class *touch_class;
struct workqueue_struct*	touch_wq;

static DEFINE_MUTEX(i2c_access);
static DEFINE_MUTEX(notify_access);
static DEFINE_MUTEX(knock_access);
static DECLARE_WAIT_QUEUE_HEAD(tpd_waiter);


/****************************************************************************
* Extern Function Prototypes
****************************************************************************/
extern int FirmwareUpgrade ( struct i2c_client *client, const char* fw_path, unsigned long fw_size, unsigned char* fw_start );
extern UINT32 DISP_GetScreenHeight ( void );
extern UINT32 DISP_GetScreenWidth ( void );
extern int mtk_wdt_enable ( enum wk_wdt_en en );
#if 0
extern g_qem_check;
#endif

/****************************************************************************
* Local Function Prototypes
****************************************************************************/
static void touch_eint_interrupt_handler ( void );
static int synaptics_ic_status_check (void );
void synaptics_interrupt_clear ( struct i2c_client *client );
void synaptics_initialize ( struct i2c_client *client );
int synaptics_ts_read ( struct i2c_client *client, u8 reg, int num, u8 *buf );
int synaptics_ts_write ( struct i2c_client *client, u8 reg, u8 * buf, int len );
#ifdef WORKQUEUE_FW
static int synaptics_firmware_update ( struct work_struct *work_fw_upgrade );
#else
static int synaptics_firmware_update ( struct i2c_client *client );
#endif

static void synaptics_power ( unsigned int on );
static void synaptics_reset_pin_power ( int on );
static void synaptics_reset_pin ( int reset_pin );

static int synaptics_knock_lpwg ( struct i2c_client* client, u32 code, u32 value, struct point *data );
static int synaptics_lpwg_update_all( struct delayed_work *knock_set_work );
static int lpwg_control(int mode);
static int tci_control(struct i2c_client *client, int type, u8 value);
static int sleep_control(struct i2c_client* client, int mode, int recal);

static enum hrtimer_restart synaptics_multi_tap_timer_handler ( struct hrtimer *multi_tap_timer );
static int synaptics_lpwg_update_all( struct delayed_work *knock_set_work );
static void synaptics_multi_tap_work ( struct work_struct *multi_tap_work );
static int synaptics_workqueue_init(void);


/****************************************************************************
* Platform(AP) dependent functions
****************************************************************************/
static void synaptics_setup_eint ( void )
{
	TPD_FUN ();

	
	/* Configure GPIO settings for external interrupt pin  */
		mt_set_gpio_mode(GPIO_CTP_EINT_PIN, GPIO_CTP_EINT_PIN_M_EINT);
		mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
		mt_set_gpio_pull_enable(GPIO_CTP_EINT_PIN, GPIO_PULL_ENABLE);
		mt_set_gpio_pull_select(GPIO_CTP_EINT_PIN, GPIO_PULL_UP);

	msleep(50);

	/* Configure external interrupt settings for external interrupt pin */
	//mt65xx_eint_set_sens ( CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_SENSITIVE );
	//mt65xx_eint_set_hw_debounce ( CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_DEBOUNCE_CN );
	mt_eint_registration( CUST_EINT_TOUCH_PANEL_NUM, CUST_EINT_TOUCH_PANEL_TYPE, touch_eint_interrupt_handler, 1 );

	/* unmask external interrupt */
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);

}

static void synaptics_power ( unsigned int on )
{

	TPD_FUN();
	TPD_LOG("TOUCH IC Power = %s", on ? "ON": "OFF");
	if(on)
	{
		hwPowerOn ( MT6323_POWER_LDO_VGP1, VOL_3000, "TP" );
		msleep(delay_time.booting);	
	}
	else
	{
		hwPowerDown ( MT6323_POWER_LDO_VGP1, "TP" );
		msleep(10);
	}
	Power_status = on;

}

/****************************************************************************
* Synaptics I2C  Read / Write Funtions
****************************************************************************/
int i2c_dma_write ( struct i2c_client *client, const uint8_t *buf, int len )
{
	int i = 0;
	for ( i = 0 ; i < len ; i++ )
	{
		I2CDMABuf_va[i] = buf[i];
	}

	if ( len < 8 )
	{
		client->addr = client->addr & I2C_MASK_FLAG | I2C_ENEXT_FLAG;
		//client->timing = 400;
		return i2c_master_send(client, buf, len);
	}
	else
	{
		client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
		//client->timing = 400;
		return i2c_master_send(client, I2CDMABuf_pa, len);
	}
}

int i2c_dma_read ( struct i2c_client *client, uint8_t *buf, int len )
{
	int i = 0, ret = 0;
	if ( len < 8 )
	{
		client->addr = client->addr & I2C_MASK_FLAG | I2C_ENEXT_FLAG;
		//client->timing = 400;
		return i2c_master_recv(client, buf, len);
	}
	else
	{
		client->addr = client->addr & I2C_MASK_FLAG | I2C_DMA_FLAG | I2C_ENEXT_FLAG;
		//client->timing = 400;
		ret = i2c_master_recv(client, I2CDMABuf_pa, len);
		if ( ret < 0 )
		{
			return ret;
		}

		for ( i = 0 ; i < len ; i++ )
		{
			buf[i] = I2CDMABuf_va[i];
		}
	}

	return ret;
}

static int i2c_msg_transfer ( struct i2c_client *client, struct i2c_msg *msgs, int count )
{
	int i = 0, ret = 0;
	for ( i = 0 ; i < count ; i++ )
	{
		if ( msgs[i].flags & I2C_M_RD )
		{
			ret = i2c_dma_read(client, msgs[i].buf, msgs[i].len);
		}
		else
		{
			ret = i2c_dma_write(client, msgs[i].buf, msgs[i].len);
		}

		if ( ret < 0 )
		{
			return ret;
		}
	}

	return 0;
}

int synaptics_ts_read_f54 ( struct i2c_client *client, u8 reg, int num, u8 *buf )
{
	int message_count = ( ( num - 1 ) / BUFFER_SIZE ) + 2;
	int message_rest_count = num % BUFFER_SIZE;
	int i, data_len;

	if ( i2c_msgs.msg == NULL || i2c_msgs.count < message_count )
	{
		if ( i2c_msgs.msg != NULL )
			kfree(i2c_msgs.msg);

#if 0		
		i2c_msgs.msg = (struct i2c_msg*)kcalloc(message_count, sizeof(struct i2c_msg), GFP_KERNEL);	
#else
		i2c_msgs.msg = (struct i2c_msg*)kcalloc(message_count, sizeof(struct i2c_msg), GFP_KERNEL);
#endif
		i2c_msgs.count = message_count;
		//dev_dbg(&client->dev, "%s: Update message count %d(%d)\n",  __func__, i2c_msgs.count, message_count);
	}

	i2c_msgs.msg[0].addr = client->addr;
	i2c_msgs.msg[0].flags = 0;
	i2c_msgs.msg[0].len = 1;
	i2c_msgs.msg[0].buf = &reg;

	if ( !message_rest_count )
		message_rest_count = BUFFER_SIZE;
	for ( i = 0 ; i < message_count - 1 ; i++ )
	{
		if ( i == message_count - 1 )
			data_len = message_rest_count;
		else
			data_len = BUFFER_SIZE;

		i2c_msgs.msg[i + 1].addr = client->addr;
		i2c_msgs.msg[i + 1].flags = I2C_M_RD;
		i2c_msgs.msg[i + 1].len = data_len;
		i2c_msgs.msg[i + 1].buf = buf + BUFFER_SIZE * i;
	}

#if 1
	if ( i2c_msg_transfer(client, i2c_msgs.msg, message_count) < 0 )
	{
#else
	if (i2c_transfer(client->adapter, i2c_msgs.msg, message_count) < 0) {
#endif
		if ( printk_ratelimit() )
			TPD_ERR ( "transfer error\n" );
		return -EIO;
	}
	else
		return 0;
}

int synaptics_ts_read ( struct i2c_client *client, u8 reg, int num, u8 *buf )
{
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = 0,
			.len = 1,
			.buf = &reg,
		},
		{
			.addr = client->addr,
			.flags = I2C_M_RD,
			.len = num,
			.buf = buf,
		},
	};

#if 1
	if ( i2c_msg_transfer(client, msgs, 2) < 0 )
	{
#else
	if (i2c_transfer(client->adapter, msgs, 2) < 0) {
#endif
		if ( printk_ratelimit() )
			TPD_ERR ( "transfer error\n" );
		return -EIO;
	}
	else
		return 0;
}
EXPORT_SYMBOL ( synaptics_ts_read );

int synaptics_ts_write ( struct i2c_client *client, u8 reg, u8 * buf, int len )
{
	unsigned char send_buf[len + 1];
	struct i2c_msg msgs[] = {
		{
			.addr = client->addr,
			.flags = client->flags,
			.len = len+1,
			.buf = send_buf,
		},
	};

	send_buf[0] = (unsigned char) reg;
	memcpy(&send_buf[1], buf, len);

#if 1
	if ( i2c_msg_transfer(client, msgs, 1) < 0 )
	{
#else
	if (i2c_transfer(client->adapter, msgs, 1) < 0) {
#endif
		if ( printk_ratelimit() )
			TPD_ERR ( "transfer error\n" );
		return -EIO;
	}
	else
		return 0;
}
EXPORT_SYMBOL ( synaptics_ts_write );

int synaptics_page_data_read ( struct i2c_client *client, u8 page, u8 reg, int size, u8 *data )
{
	int ret = 0;

	ret = synaptics_ts_write ( client, PAGE_SELECT_REG, &page, 1 );
	if ( ret < 0 )
	{
		TPD_ERR ( "PAGE_SELECT_REG write fail\n" );
		return ret;
	}

	ret = synaptics_ts_read ( client, reg, size, data );
	if ( ret < 0 )
	{
		TPD_ERR ( "synaptics_page_data_read fail\n" );
		return ret;
	}

	ret = synaptics_ts_write ( client, PAGE_SELECT_REG, &device_control_page, 1 );
	if ( ret < 0 )
	{
		TPD_ERR ( "PAGE_SELECT_REG write fail\n" );
		return ret;
	}

	return 0;
}

int synaptics_page_data_write ( struct i2c_client *client, u8 page, u8 reg, int size, u8 *data )
{
	int ret = 0;

	ret = synaptics_ts_write ( client, PAGE_SELECT_REG, &page, 1 );
	if ( ret < 0 )
	{
		TPD_ERR ( "PAGE_SELECT_REG write fail\n" );
		return ret;
	}

	ret = synaptics_ts_write ( client, reg, data, size );
	if ( ret < 0 )
	{
		TPD_ERR ( "synaptics_page_data_write fail\n" );
		return ret;
	}

	ret = synaptics_ts_write ( client, PAGE_SELECT_REG, &device_control_page, 1 );
	if ( ret < 0 )
	{
		TPD_ERR ( "PAGE_SELECT_REG write fail\n" );
		return ret;
	}

	return 0;
}

/****************************************************************************
* Touch malfunction Prevention Function
****************************************************************************/
static void synaptics_release_all_finger ( void )
{
	TPD_FUN ();
	unsigned int finger_count=0;

	/* Reset finger position data */
	memset(&old_touch_info, 0x0, sizeof(touch_finger_info));
	memset(&pre_touch_info, 0x0, sizeof(touch_finger_info));

	/* Reset finger & button status data */
	memset(finger_oldstate, 0x0, sizeof(char) * MAX_NUM_OF_FINGER);
	memset(button_oldstate, 0x0, sizeof(char) * TPD_KEY_COUNT);
	memset(finger_prestate, 0x0, sizeof(char) * MAX_NUM_OF_FINGER);
	memset(button_prestate, 0x0, sizeof(char) * TPD_KEY_COUNT);

#if defined(MT_PROTOCOL_A)
	input_mt_sync ( tpd->dev );
#else
	for(finger_count = 0; finger_count < MAX_NUM_OF_FINGER; finger_count++)
	{
		input_mt_slot( tpd->dev, finger_count);
		input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, false);
	}
#endif
	input_sync ( tpd->dev );
}


#if defined(ONLY_S7020_RESET_PIN)
static void synaptics_reset_pin_power ( int on )
{
#if 1
	TPD_ERR ("on_off:0x%x\n", on);
	mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
	mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );

	if ( on ) 
	{	
		mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
		mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
		mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
		msleep(delay_time.reset);
	}
	else
	{		
		mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
		mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
		mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );
	}
#else
	TPD_ERR ("on_off:0x%x\n", on);
	if ( on ) 
	{
		mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
		mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
		mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
		msleep(delay_time.reset);
	}
	else
	{
		mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
		mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
		mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );
	}
#endif
}

static void synaptics_reset_pin ( int reset_pin )
{
	TPD_FUN ();

	synaptics_reset_pin_power(0);
	synaptics_reset_pin_power(1);
}
#endif

static int synaptics_interrupt_enable ( int enable )
{
	TPD_FUN ();
	int ret =0;
	u8 int_enable;
	u8 r_cmd=0;
	if ( enable )
	{
		r_cmd = 0x7F;
		ret = synaptics_ts_write( tpd_i2c_client, INTERRUPT_ENABLE_REG, (u8 *) &r_cmd, 1 );	
		if ( ret < 0 )
		{
			TPD_ERR ( "INTERRUPT_ENABLE_REG write fail\n" );
		}
		else
		{
			ret = synaptics_ts_read ( tpd_i2c_client, INTERRUPT_ENABLE_REG, 1, (u8 *) &int_enable );
			if ( ret < 0 )
			{
				TPD_ERR ( "INTERRUPT_ENABLE_REG read fail\n" );
			}
		}
	}
	else
	{
		ret = synaptics_ts_read ( tpd_i2c_client, INTERRUPT_ENABLE_REG, 1, (u8 *) &int_enable );
		if ( ret < 0 )
		{
			TPD_ERR ( "INTERRUPT_ENABLE_REG read fail\n" );
		}
		else
		{
			r_cmd = 0x00;
			ret = synaptics_ts_write( tpd_i2c_client, INTERRUPT_ENABLE_REG, (u8 *) &r_cmd, 1 );	
			if ( ret < 0 )
			{
				TPD_ERR ( "INTERRUPT_ENABLE_REG write fail\n" );
			}
		}
	}
}
static void synaptics_ic_reset ( void )
{
	TPD_FUN ();
	
	mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );
	msleep(10);
	mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
	msleep(delay_time.reset);
}

//static int synaptics_ic_status_check ( void )
static int synaptics_ic_status_check (void )
{
	int ret;
	u8 device_status = 0;
	u8 temp = 0;

	/* read device status */
	 	
	ret = synaptics_ts_read ( tpd_i2c_client, DEVICE_STATUS_REG, 1, (u8 *) &device_status );
	if ( ret < 0 )
	{
		TPD_ERR ( "DEVICE_STATUS_REG read fail\n" );
		return -1;
	}

	/* ESD damage check */
	if ( ( device_status & DEVICE_FAILURE_MASK ) == DEVICE_FAILURE_MASK )
	{
		TPD_ERR ( "ESD damage occured. Reset Touch IC\n" );
		synaptics_ic_reset ();
		synaptics_initialize ( tpd_i2c_client );
		return -1;
	}

	/* Internal reset check */
	if ( ( ( device_status & UNCONFIGURED_MASK ) >> 7 ) == 1 )
	{
		TPD_ERR ( "Touch IC resetted internally. Reconfigure register setting\n");
		synaptics_initialize ( tpd_i2c_client );
		return -1;
	}

	return 0;
}

void touch_keylock_enable ( int key_lock )
{
	TPD_FUN ();

	if ( !key_lock )
	{
		mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );
		key_lock_status = 0;
	}
	else
	{
		mt_eint_mask ( CUST_EINT_TOUCH_PANEL_NUM );
		key_lock_status = 1;
	}
}
EXPORT_SYMBOL ( touch_keylock_enable );

/****************************************************************************
* Synaptics Knock_On Funtions
****************************************************************************/
static enum hrtimer_restart synaptics_multi_tap_timer_handler ( struct hrtimer *multi_tap_timer )
{
	TPD_FUN ();
	queue_work ( touch_multi_tap_wq, &multi_tap_work );

	return HRTIMER_NORESTART;
}

static void synaptics_multi_tap_work ( struct work_struct *multi_tap_work )
{
	TPD_FUN ();
	u8 r_mem = 0;
	
	tci_control(tpd_i2c_client,REPORT_MODE_CTRL,1);//wakeup gesture only
	DO_SAFE(sleep_control(tpd_i2c_client,0,0),error);
	/* uevent report */
	kobject_uevent_env ( &foo_obj->kobj, KOBJ_CHANGE, lpwg_uevent[LPWG_MULTI_TAP-1] );

	error:
		return;
}
static int get_tci_data(struct i2c_client * client,int count)
{
	TPD_FUN();
	u8 i=0;

	if(!count)
	{
		TPD_ERR("Knock count = 0");
		return 0;
	}
	else
	{	
		TPD_LOG("Knock count = %d", count);
	}

	DO_SAFE(synaptics_page_data_read(client,LPWG_PAGE,LPWG_DATA_REG,4*count,&gesture_property),error);
	/*
	if(custom_gesture_status)
	{
		for(i=0; i<count; i++)
		{
			TPD_LOG ( "lpwg data %d: [0:0x%-4x 1:0x%-4x 2:0x%-4x 3:0x%-4x]\n", i, gesture_property[4*i], gesture_property[4*i+1], gesture_property[4*i+2], gesture_property[4*i+3] );
		}
	}
	*/
	return 0;
	error:
		return -1;
}

static int synaptics_knock_check ( void )
{
	TPD_FUN ();
	int ret = 0;
	u8  device_status = 0;
	u8 gesture_status = 0;
	
	synaptics_page_data_read ( tpd_i2c_client, multi_tap_page, GESTURE_STATUS_REG, 1, (u8 *) &gesture_status );
	if ( ret < 0 )
	{
		TPD_ERR ( "GESTURE_STATUS_REG read fail\n" );
		return -1;
	}
	TPD_LOG ( "MultipleTap gesture status = %d\n", gesture_status );
	custom_gesture_status = gesture_status;

	DO_SAFE(synaptics_ts_read(tpd_i2c_client, DEVICE_STATUS_REG, 1 , &device_status), error);
	//knock code => DEVICE_STATUS_REG = 0

	DO_IF((device_status & DEVICE_FAILURE_MASK) == DEVICE_FAILURE_MASK, error);

	
	if ( double_tap_enable & gesture_status == 0x1)
	{
	    get_tci_data(tpd_i2c_client, 2);
		kobject_uevent_env ( &foo_obj->kobj, KOBJ_CHANGE, lpwg_uevent[LPWG_DOUBLE_TAP-1] );
	}
	else if ( multi_tap_enable && gesture_status == 0x1)
	{
		get_tci_data(tpd_i2c_client,multi_tap_count);
		wake_lock ( &knock_code_lock );
		tci_control(tpd_i2c_client, REPORT_MODE_CTRL, 0);
		hrtimer_try_to_cancel ( &multi_tap_timer );
		if ( !hrtimer_callback_running ( &multi_tap_timer ) )
		{
			hrtimer_start ( &multi_tap_timer, ktime_set(0, MS_TO_NS(200)), HRTIMER_MODE_REL );
		}
	}
	else
	{
		TPD_ERR ( "Ignore interrupt [double_tap_enable=%d, multi_tap_enable=%d, gesture_status=0x%02x]\n", double_tap_enable, multi_tap_enable, gesture_status );
		goto error;
	}

	return 0;
error :
	return -1;
}

static int sleep_control(struct i2c_client* client, int mode, int recal)
{
	u8 curr = 0;
	u8 next = 0;

	/*
        * NORMAL == 0 : resume & lpwg state
	 * SLEEP  == 1 : uevent reporting time - sleep
	 * NO_CAL == 2 : proxi near - sleep when recal is not needed
     */
	DO_SAFE(synaptics_ts_read(client, DEVICE_CONTROL_REG, 1, &curr), error);

	TPD_LOG("CURR = %d",curr);

    next = (curr & 0xFC) | mode ? NORMAL_OPERATION_MASK : SENSOR_SLEEP_MASK;
			// (recal ? DEVICE_CONTROL_SLEEP : DEVICE_CONTROL_SLEEP_NO_RECAL);

	TPD_LOG("NEXT = %d", next);

    TPD_LOG( "%s : current = [%6s] => next [%6s]\n", "sleep_control",
		(curr == 0 ? "NORMAL" : (curr == 1 ? "SLEEP" : "NO_CAL")),
		(next == 0 ? "NORMAL" : (next == 1 ? "SLEEP" : "NO_CAL")));

	if ( curr != next )
	{
    DO_SAFE(synaptics_ts_write(client, DEVICE_CONTROL_REG, &next,1), error);
	}
	return 0;
error:
	return -1;
}


static int tci_control(struct i2c_client *client, int type, u8 value)
{
	u8 buffer[3] = {0};
	u8 temp = 0;
	switch (type) {
	case REPORT_MODE_CTRL:
		DO_SAFE(synaptics_ts_read(client, INTERRUPT_ENABLE_REG, 1, buffer), error);
		temp = value ? buffer[0] & ~INTERRUPT_MASK_ABS0 : buffer[0] | INTERRUPT_MASK_ABS0 ; 		
		DO_SAFE(synaptics_ts_write(client, INTERRUPT_ENABLE_REG, &temp,1), error);
		TPD_LOG("%s -> %s, Interrupt ENABLE %s",(value ? "normal" : "wakeup_gesure_only"),(value ? "wakeup_gesture_only" : "normal"),(value ? "only ABS masking(0x7B)" : "only ABS unmaking(0x04)"));

		DO_SAFE(synaptics_ts_read(client, TWO_D_REPORT_MODE_REG, 1, buffer), error);	
		temp = (buffer[0] & 0xf8) | (value ? WAKEUP_GESTURE_REPORTING_MODE_MASK : REDUCED_REPORTING_MODE_MASK);
		DO_SAFE(synaptics_ts_write(client, TWO_D_REPORT_MODE_REG, &temp,1), error);
		break;
		
	case TCI_ENABLE_CTRL: 
		DO_SAFE(synaptics_page_data_read(client, LPWG_PAGE, LPWG_TAPCOUNT_REG, 1, buffer), error);
		buffer[0] = (buffer[0] & 0xfe) | (value & 0x1);
		DO_SAFE(synaptics_page_data_write(client, LPWG_PAGE, LPWG_TAPCOUNT_REG, 1, buffer), error);
		break;
		
	case TAP_COUNT_CTRL: 
		DO_SAFE(synaptics_page_data_read(client, LPWG_PAGE, LPWG_TAPCOUNT_REG, 1, buffer), error);
		buffer[0] = ((value << 3) & 0xf8) | (buffer[0] & 0x7);
		DO_SAFE(synaptics_page_data_write(client, LPWG_PAGE, LPWG_TAPCOUNT_REG, 1, buffer), error);
		break;	
		
	case MIN_INTERTAP_CTRL:
		DO_SAFE(synaptics_page_data_write(client, LPWG_PAGE, LPWG_MIN_INTERTAP_REG, 1,&value), error);
		break;
		
	case MAX_INTERTAP_CTRL:
		DO_SAFE(synaptics_page_data_write(client, LPWG_PAGE, LPWG_MAX_INTERTAP_REG, 1,&value), error);
		break;

	case TOUCH_SLOP_CTRL:
		DO_SAFE(synaptics_page_data_write(client, LPWG_PAGE, LPWG_TOUCH_SLOP_REG, 1,&value), error);
		break;
		
	case TAP_DISTANCE_CTRL:
		DO_SAFE(synaptics_page_data_write(client, LPWG_PAGE, LPWG_TAP_DISTANCE_REG, 1,&value), error);
		break;
		
	default:
		break;
		}
error:
	return -1;
}

static int lpwg_control(int mode)
{
	TPD_FUN();
	u8 temp = 0;
	u8 r_mem = 0;
	TPD_LOG("lpwg_control MODE = %d", mode);
		switch (mode) {
		case LPWG_DOUBLE_TAP:	

			TPD_LOG("DOUBLE TAP REGISTER SETTING");
			tci_control(tpd_i2c_client, TCI_ENABLE_CTRL, 1);
			tci_control(tpd_i2c_client, TAP_COUNT_CTRL, 2);

			if ( synaptics_ts_read ( tpd_i2c_client, DEVICE_CONTROL_REG, 1, &r_mem ) < 0 )
			{
				TPD_ERR ( "DEVICE_CONTROL_REG read fail!\n" );
				return -EIO;
			}
			else
			{
				r_mem = CONFIGURED_MASK;
				if ( synaptics_ts_write ( tpd_i2c_client, DEVICE_CONTROL_REG, &r_mem, 1 ) < 0 )
				{
					TPD_ERR ( "DEVICE_CONTROL_REG write fail\n" );
					return -EIO;
				}
			}

			tci_control(tpd_i2c_client, MAX_INTERTAP_CTRL, 70);
			tci_control(tpd_i2c_client, TAP_DISTANCE_CTRL, 10);
			tci_control(tpd_i2c_client, REPORT_MODE_CTRL, 1);	
			break;
			
		case LPWG_MULTI_TAP:	
			
			TPD_LOG("MULTI TAP REGISTER SETTING");	
			tci_control(tpd_i2c_client, TCI_ENABLE_CTRL, 1);
		
			if ( synaptics_ts_read ( tpd_i2c_client, DEVICE_CONTROL_REG, 1, &r_mem ) < 0 )
			{
				TPD_ERR ( "DEVICE_CONTROL_REG read fail!\n" );
				return -EIO;
			}
			else
			{
				r_mem = CONFIGURED_MASK;
				if ( synaptics_ts_write ( tpd_i2c_client, DEVICE_CONTROL_REG, &r_mem, 1 ) < 0 )
				{
					TPD_ERR ( "DEVICE_CONTROL_REG write fail\n" );
					return -EIO;
				}
			}
			tci_control(tpd_i2c_client, MAX_INTERTAP_CTRL, 70);
			tci_control(tpd_i2c_client, TAP_DISTANCE_CTRL, 255);
			tci_control(tpd_i2c_client, TAP_COUNT_CTRL, multi_tap_count);

			tci_control(tpd_i2c_client, REPORT_MODE_CTRL, 1);	
			break;
		default:
			
			TPD_LOG("IDLE REGISTER SETTING");
			tci_control(tpd_i2c_client, TCI_ENABLE_CTRL, 0);		
			tci_control(tpd_i2c_client, REPORT_MODE_CTRL, 0);		
			sleep_control(tpd_i2c_client, 1, 0);
			break;
			}
		return 0;
}


static int synaptics_knock_lpwg ( struct i2c_client* client, u32 code, u32 value, struct point *data )
{
	TPD_FUN();
	u8 buf = 0;
	int i = 0, ret = 0;
	if ( mt_get_gpio_out ( GPIO_CTP_RST_PIN) == GPIO_OUT_ZERO )
		synaptics_reset_pin_power(1);
	
	switch ( code )
	{
		case LPWG_READ:
			if ( multi_tap_enable )
			{
				if ( custom_gesture_status == 0 )
				{
					data[0].x = 1;
					data[0].y = 1;
					data[1].x = -1;
					data[1].y = -1;
					break;
				}

				for ( i = 0 ; i < multi_tap_count ; i++ )
				{
					data[i].x = ( gesture_property[4*i+1] << 8 | gesture_property[4*i] ) / lcd_touch_ratio_x;
					data[i].y = ( gesture_property[4*i+3] << 8 | gesture_property[4*i+2] ) / lcd_touch_ratio_y;
					TPD_LOG ( "TAP Position x[%3d], y[%3d]\n", data[i].x, data[i].y );
					// '-1' should be assinged to the last data.
					// Each data should be converted to LCD-resolution.
				}
				data[i].x = -1;
				data[i].y = -1;
			}
			break;

		case LPWG_ENABLE://not use
			break;

		// If touch-resolution is not same with LCD-resolution,
		// position-data should be converted to LCD-resolution.	
		case LPWG_LCD_X:
			lcd_touch_ratio_x = x_max / value;
			TPD_LOG ( "LPWG GET X LCD INFO = %d , RATIO_X = %d", value, lcd_touch_ratio_x );
			break;

		case LPWG_LCD_Y:
			lcd_touch_ratio_y = y_max / value;
			TPD_LOG ( "LPWG GET Y LCD INFO = %d , RATIO_Y = %d", value, lcd_touch_ratio_y );
			break;

		case LPWG_ACTIVE_AREA_X1:
		case LPWG_ACTIVE_AREA_X2:
		case LPWG_ACTIVE_AREA_Y1:
		case LPWG_ACTIVE_AREA_Y2:
			break;

		case LPWG_TAP_COUNT:
			if ( value )
			{
				multi_tap_count = value;
				case_mode = 2;
				tci_control(client,TAP_COUNT_CTRL,multi_tap_count);
			}
			else
			{
				multi_tap_count = 2;
				case_mode = 1;
				tci_control(client,TAP_COUNT_CTRL,2);
			}
			break;

		case LPWG_REPLY:
			// Do something, if you need.
			ret = synaptics_ts_read ( client, DEVICE_CONTROL_REG, 1, &buf );
			if ( ret < 0 )
			{
				TPD_ERR ( "DEVICE_CONTROL_REG read fail\n" );
				return -1;
			}

			buf = ( buf & 0xFC ) | NORMAL_OPERATION_MASK;
			ret = synaptics_ts_write ( client, DEVICE_CONTROL_REG, &buf, 1 );
			if ( ret < 0 )
			{
				TPD_ERR ( "DEVICE_CONTROL_REG write fail\n" );
				return -1;
			}
			break;

		default:
			break;
	}

	return 0;
}



/****************************************************************************
* Touch Interrupt Service Routines
****************************************************************************/
static void touch_eint_interrupt_handler ( void )
{
	mt_eint_mask ( CUST_EINT_TOUCH_PANEL_NUM );
	TPD_DEBUG_PRINT_INT;
	tpd_flag = 1;
	wake_up_interruptible ( &tpd_waiter );
}

static int synaptics_touch_event_handler ( void *unused )
{
	int ret = 0;
	u8 int_status;
	u8 int_enable;
	u8 finger_count, button_count;
	u8 reg_num, finger_order;
	int width_min = 0, width_max = 0;
	int width_orientation;
	int index;
	touch_sensor_data sensor_data;
	touch_finger_info finger_info;
	char report_enable = 0;

	struct sched_param param = { .sched_priority = RTPM_PRIO_TPD }; 

	sched_setscheduler ( current, SCHED_RR, &param );

	do
	{
		set_current_state ( TASK_INTERRUPTIBLE );
		wait_event_interruptible ( tpd_waiter, tpd_flag != 0 );

		tpd_flag = 0;
		set_current_state ( TASK_RUNNING );

		mutex_lock ( &i2c_access );

		index = 0;
		memset(&sensor_data, 0x0, sizeof(touch_sensor_data));
		memset(&finger_info, 0x0, sizeof(touch_finger_info));
		pressure_zero = 0;

		ret = synaptics_ic_status_check ();
		if ( ret != 0 )
		{
			TPD_LOG("ignore ic status ");
			goto exit_work;
		}

		/* read interrupt information */
		ret = synaptics_ts_read ( tpd_i2c_client, INTERRUPT_STATUS_REG, 1, (u8 *) &int_status );
		if ( ret < 0 )
		{
			TPD_ERR ( "INTERRUPT_STATUS_REG read fail\n" );
			goto exit_work;
		}
		if (int_status == 0 || int_status == 0x2 )
			goto exit_work;

		/* Knock On or Knock Code Check */
		if ( suspend_status && knock_on_enable )
		{
			if ( download_status == 1 )
			{
				TPD_LOG ( "knock_on is not checked (F/W downloading...)\n" );
			}
			else
			{
				ret = synaptics_knock_check ();
				if ( ret != 0 )
				{
					TPD_LOG ( "Touch Knock_On fail\n" );
				}
			}

			goto exit_work;
		}

		ret = synaptics_ts_read ( tpd_i2c_client, INTERRUPT_ENABLE_REG, 1, (u8 *) &int_enable );
		if ( ret < 0 )
		{
			TPD_ERR ( "INTERRUPT_ENABLE_REG read fail\n" );
			goto exit_work;
		}

		/* read finger state & finger data */
		ret = synaptics_ts_read ( tpd_i2c_client, TWO_D_FINGER_STATE_REG, sizeof(sensor_data), (u8 *) &sensor_data.finger_state[0] );
		if (ret < 0)
		{
			TPD_ERR ( "TWO_D_FINGER_STATE_REG read fail\n" );
			goto exit_work;
		}


		/* Finger Event Processing */
		if ( ( int_status & ABS0_MASK ) && ( int_enable & ABS0_MASK ) )
		{
			for ( finger_count = 0 ; finger_count < MAX_NUM_OF_FINGER ; finger_count++ )
			{
				reg_num = finger_count / 4;
				finger_order = finger_count % 4;

				if ( ( ( sensor_data.finger_state[reg_num] >> ( finger_order * 2 ) ) & FINGER_STATE_MASK ) == 1 )
				{
					finger_info.pos_x[finger_count] = ( int ) GET_X_POSITION(sensor_data.finger_data[finger_count][X_POSITION], sensor_data.finger_data[finger_count][XY_POSITION]);
					finger_info.pos_y[finger_count] = ( int ) GET_Y_POSITION(sensor_data.finger_data[finger_count][Y_POSITION], sensor_data.finger_data[finger_count][XY_POSITION]);

					if ( ( ( sensor_data.finger_data[finger_count][WX_WY] & 0xF0 ) >> 4 ) > ( sensor_data.finger_data[finger_count][WX_WY] & 0x0F ) )
					{
						width_max = ( sensor_data.finger_data[finger_count][WX_WY] & 0xF0 ) >> 4;
						width_min = sensor_data.finger_data[finger_count][WX_WY] & 0x0F;
						width_orientation = 0;
					}
					else
					{
						width_max = sensor_data.finger_data[finger_count][WX_WY] & 0x0F;
						width_min = ( sensor_data.finger_data[finger_count][WX_WY] & 0xF0 ) >> 4;
						width_orientation = 1;
					}

					finger_info.pressure[finger_count] = sensor_data.finger_data[finger_count][PRESSURE];
					if(qcover == 1)//Touch disable in Quick circle area when Qcover is closed
					{	
						if(finger_info.pos_x[finger_count] < Double_Tap_Area_X1|| finger_info.pos_x[finger_count] > Double_Tap_Area_X2 
							|| finger_info.pos_y[finger_count] < Double_Tap_Area_Y1 || finger_info.pos_y[finger_count] > Double_Tap_Area_Y2 )
						{	 
					
							continue;
						}	
					}

#if !defined(MT_PROTOCOL_A)
					input_mt_slot( tpd->dev, finger_count);
					input_mt_report_slot_state( tpd->dev, MT_TOOL_FINGER, true);
#endif
					input_report_abs ( tpd->dev, ABS_MT_POSITION_X, finger_info.pos_x[finger_count] );
					input_report_abs ( tpd->dev, ABS_MT_POSITION_Y, finger_info.pos_y[finger_count] );
					input_report_abs ( tpd->dev, ABS_MT_PRESSURE, finger_info.pressure[finger_count] );
					input_report_abs ( tpd->dev, ABS_MT_WIDTH_MAJOR, width_max );
					input_report_abs ( tpd->dev, ABS_MT_WIDTH_MINOR, width_min );
					input_report_abs ( tpd->dev, ABS_MT_ORIENTATION, width_orientation );
					input_report_abs ( tpd->dev, ABS_MT_TRACKING_ID, finger_count );

					report_enable = 1;

					old_touch_info.pos_x[finger_count] = pre_touch_info.pos_x[finger_count];
					old_touch_info.pos_y[finger_count] = pre_touch_info.pos_y[finger_count];
					old_touch_info.pressure[finger_count]= pre_touch_info.pressure[finger_count];
					pre_touch_info.pos_x[finger_count] = finger_info.pos_x[finger_count];
					pre_touch_info.pos_y[finger_count] = finger_info.pos_y[finger_count];
					pre_touch_info.pressure[finger_count] = finger_info.pressure[finger_count];
					index++;

					/* ignore Key event during Finger event processing */
					if ( finger_prestate[finger_count] == TOUCH_RELEASED )
					{
						finger_oldstate[finger_count] = finger_prestate[finger_count];
						finger_prestate[finger_count] = TOUCH_PRESSED;
						TPD_LOG ( "%d key is %s ( x/y = %d, %d )\n", finger_count, finger_prestate[finger_count] ? "pressed" : "released", 
						pre_touch_info.pos_x[finger_count], pre_touch_info.pos_y[finger_count] );
					
					}
				}
				else if ( finger_prestate[finger_count] == TOUCH_PRESSED )
				{
#if !defined(MT_PROTOCOL_A)
					input_mt_slot(tpd->dev, finger_count);
					input_mt_report_slot_state(tpd->dev, MT_TOOL_FINGER, false);
#endif
					finger_oldstate[finger_count] = finger_prestate[finger_count];
					finger_prestate[finger_count] = TOUCH_RELEASED;
					TPD_LOG ( "%d key is %s ( x/y =                    %d, %d )\n", finger_count,finger_prestate[finger_count] ? "pressed" : "released", 
						pre_touch_info.pos_x[finger_count], pre_touch_info.pos_y[finger_count] );
					report_enable = 1;

					pre_touch_info.pos_x[finger_count] = 0;
					pre_touch_info.pos_y[finger_count] = 0;
				}

				if ( report_enable )
				{
#if defined(MT_PROTOCOL_A)
					input_mt_sync ( tpd->dev );
#endif
					report_enable = 0;
				}
			}

			finger_info.total_num = index;
			old_touch_info.total_num = pre_touch_info.total_num;
			pre_touch_info.total_num = finger_info.total_num;

			input_sync ( tpd->dev );
		}


exit_work:
		mutex_unlock ( &i2c_access );
		mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );
	} while ( !kthread_should_stop () );

	return 0;
}

/****************************************************************************
* Synaptics Notify Funtions
****************************************************************************/
static enum hrtimer_restart synaptics_notify_timer_handler ( struct hrtimer *notify_timer )
{
	TPD_FUN ();
	queue_delayed_work ( knock_set_work_wq, &knock_set_work, 0);
	return HRTIMER_NORESTART;
}

/****************************************************************************
* SYSFS function for Touch
****************************************************************************/
static ssize_t show_knock_on_type ( struct device *dev, struct device_attribute *attr, char *buf )
{
	TPD_FUN();
	int ret = 0;
	ret = sprintf ( buf, "%d\n", knock_on_type );
	return ret;
}
static DEVICE_ATTR ( knock_on_type, 0664, show_knock_on_type, NULL );


static ssize_t show_lpwg_data ( struct device *dev, struct device_attribute *attr, char *buf )
{
	TPD_FUN ();
	int i = 0, ret = 0;

	memset(lpwg_data, 0, sizeof(struct point)*MAX_POINT_SIZE_FOR_LPWG);

	synaptics_knock_lpwg ( tpd_i2c_client, LPWG_READ, 0, lpwg_data );
	for ( i = 0 ; i < MAX_POINT_SIZE_FOR_LPWG ; i++ )
	{
		if ( lpwg_data[i].x == -1 && lpwg_data[i].y == -1 )
		{
			break;
		}
		ret += sprintf ( buf+ret, "%d %d\n", lpwg_data[i].x, lpwg_data[i].y );
	}

	return ret;
}

static ssize_t store_lpwg_data ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	TPD_FUN ();
	int reply = 0;

	sscanf ( buf, "%d", &reply );
	TPD_LOG ( "LPWG RESULT = %d ", reply );

	synaptics_knock_lpwg ( tpd_i2c_client, LPWG_REPLY, reply, NULL );

	wake_unlock ( &knock_code_lock );

	return size;
}
static DEVICE_ATTR ( lpwg_data, 0664, show_lpwg_data, store_lpwg_data );

static ssize_t show_Knock_Parameter ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	TPD_FUN ();
	int ret = 0;
	TPD_LOG("--------Show Knock on/code Paramter Value--------");
	TPD_LOG("- knock_on_enalbe   = 	%d                       -", knock_on_enable);
	TPD_LOG("- double_tap_enalbe = 	%d                       -", double_tap_enable);
	TPD_LOG("- multi_tap_enalbe  =	%d                       -", multi_tap_enable);
	TPD_LOG("- lpwg_mode         =	%d                       -", lpwg_mode);
	TPD_LOG("- screen            =  %d                       -", screen);
	TPD_LOG("- sensor            =  %d                       -", sensor);
	TPD_LOG("- qcover            =  %d                       -", qcover);
	TPD_LOG("- Power_status      =  %d                       -", Power_status);
	TPD_LOG("-Double_Tap_Area    =  %d,%d [from]             -", Double_Tap_Area_X1,Double_Tap_Area_Y1);
	TPD_LOG("-Double_Tap_Area    =  %d,%d [to]               -", Double_Tap_Area_X2,Double_Tap_Area_Y2);
	TPD_LOG("-------------------------------------------------");
	ret += sprintf(buf+ret,"--------Show Knock on/code Paramter Value--------\n");
	ret += sprintf(buf+ret,"| knock_on_enalbe   = 	%d                       |\n", knock_on_enable);
	ret += sprintf(buf+ret,"| double_tap_enalbe = 	%d                       |\n", double_tap_enable);
	ret += sprintf(buf+ret,"| multi_tap_enalbe  =	%d                       |\n", multi_tap_enable);
	ret += sprintf(buf+ret,"| lpwg_mode         =	%d                       |\n", lpwg_mode);
	ret += sprintf(buf+ret,"| screen            =   %d                       |\n", screen);
	ret += sprintf(buf+ret,"| sensor            =   %d                       |\n", sensor);
	ret += sprintf(buf+ret,"| qcover            =   %d                       |\n", qcover);
	ret += sprintf(buf+ret,"| Power_status      =   %d                       |\n", Power_status);
	ret += sprintf(buf+ret,"| Double_Tap_Area    =  %d,%d [from]             |", Double_Tap_Area_X1,Double_Tap_Area_Y1);
	ret += sprintf(buf+ret,"| Double_Tap_Area    =  %d,%d [to]               |", Double_Tap_Area_X2,Double_Tap_Area_Y2);
	ret += sprintf(buf+ret,"-------------------------------------------------\n");
	return ret;
}
static DEVICE_ATTR ( Knock_Parameter, 0664, show_Knock_Parameter, NULL );

static ssize_t show_Int_Pin ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int ret = 0;
	TPD_LOG(" mt_get_gpio_in(current) : 0x%x\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));

	ret += sprintf(buf+ret,"INTERRUPT PIN = 0x%x\n",mt_get_gpio_in(GPIO_CTP_EINT_PIN) );
	return ret;
}
static DEVICE_ATTR ( Int_Pin, 0664, show_Int_Pin, NULL );

static ssize_t show_Reset_Pin ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int ret = 0;
	TPD_LOG(" mt_get_gpio_in(current) : 0x%x\n", mt_get_gpio_in(GPIO_CTP_RST_PIN));
	ret += sprintf(buf+ret,"INTERRUPT PIN = 0x%x\n",mt_get_gpio_in(GPIO_CTP_RST_PIN) );
	return ret;
}
static DEVICE_ATTR ( Reset_Pin, 0664, show_Reset_Pin, NULL );

static ssize_t show_Do_Read_Int_Status ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int ret = 0;
	u8 int_status = 0;
	int_status =0;
	DO_SAFE(synaptics_ts_read(tpd_i2c_client, INTERRUPT_STATUS_REG, 1, &int_status), error);// It always should be done last.==> Interrupt Pin Clear
	TPD_LOG("Read Interrupt status = 0x%d\n", int_status);
	ret += sprintf(buf+ret,"Read Interrupt status = 0x%d\n", int_status);
	return ret;
	error:
		return -1;
}
static DEVICE_ATTR ( Do_Read_Int_Status, 0664, show_Do_Read_Int_Status, NULL );

static ssize_t show_Do_Ic_Reset(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	synaptics_ic_reset();
	ret += sprintf(buf+ret,"Do Ic Reset");
	return ret;
}
static DEVICE_ATTR(Do_Ic_Reset, 0664, show_Do_Ic_Reset, NULL);

static ssize_t show_Do_Initialize(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	synaptics_initialize(tpd_i2c_client);
	ret += sprintf(buf+ret,"Do Initialize");
	return ret;
}
static DEVICE_ATTR(Do_Initialize, 0664, show_Do_Initialize, NULL);

static ssize_t show_Do_Updata_All(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	hrtimer_start ( &notify_timer, ktime_set(0, MS_TO_NS(50)), HRTIMER_MODE_REL );
	return ret;
}
static DEVICE_ATTR(Do_Updata_All, 0664, show_Do_Updata_All, NULL);

static ssize_t show_Reset_Pin_Raw(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	TPD_FUN();
	mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
	mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
	/*reset pin LOW*/
	mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
	mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
	mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ZERO );	
	/*and i2c read*//////
	ret += sprintf(buf+ret,"Do Reset Pin Raw");
	return ret;
}
static DEVICE_ATTR(Reset_Pin_Raw, 0664, show_Reset_Pin_Raw, NULL);

static ssize_t show_Reset_Pin_High(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	TPD_FUN();
	mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
	mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
	mt_set_gpio_out ( GPIO_CTP_RST_PIN, GPIO_OUT_ONE );
	msleep(delay_time.reset);		
	ret += sprintf(buf+ret,"Do Reset Pin High");
	return ret;
}
static DEVICE_ATTR(Reset_Pin_High, 0664, show_Reset_Pin_High, NULL);

static ssize_t show_Do_Mask_Int(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret = 0;
	mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);
	ret += sprintf(buf+ret,"Do Mask Int");
	return ret;
}
static DEVICE_ATTR(Do_Mask_Int, 0664, show_Do_Mask_Int, NULL);

static ssize_t show_Do_Un_Mask_Int(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	int ret  = 0;
	mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	ret += sprintf(buf+ret,"Do Un Mask Int");
	return ret;
}
static DEVICE_ATTR(Do_Un_Mask_Int, 0664, show_Do_Un_Mask_Int, NULL);

static ssize_t show_Do_Release_All_Finger ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int ret = 0;
	TPD_LOG("Do Release All finger");
	synaptics_release_all_finger();
	ret += sprintf(buf+ret,"Do Release all finger.");
	return ret;
}
static DEVICE_ATTR ( Do_Release_All_Finger, 0664, show_Do_Release_All_Finger, NULL );

static ssize_t store_lpwg_notify ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{	
	int type = 0;
	int value[4] = {0};
	int ret = 0;
	sscanf ( buf, "%d %d %d %d %d", &type, &value[0], &value[1], &value[2], &value[3] );
	if(type == 6 || type == 1 || type ==7)
	{
		return size;
	}
	TPD_LOG ( "[store_lpwg_notify]touch notify type = %d , value[0] = %d, value[1] = %d, value[2] = %d, value[3] = %d ", type, value[0], value[1], value[2], value[3] );
	mutex_lock(&knock_access);

	switch ( type )
	{
		case 1:			
			break;
        case 2:
			synaptics_knock_lpwg ( tpd_i2c_client, LPWG_LCD_X, value[0], NULL );
			synaptics_knock_lpwg ( tpd_i2c_client, LPWG_LCD_Y, value[1], NULL );
			break;

        case 3:
			Double_Tap_Area_X1 = value[0]*2;//set lcd / touch 
			Double_Tap_Area_X2 = value[1]*2;
			Double_Tap_Area_Y1 = value[2]*2;
			Double_Tap_Area_Y2 = value[3]*2;

			if(suspend_status == 0 && qcover == 1)
			{
				synaptics_release_all_finger ();
			}
			break;

        case 4:
			synaptics_knock_lpwg ( tpd_i2c_client, LPWG_TAP_COUNT, value[0], NULL );
			break;
			
		case 6:
			break;
        case 7:	
			break;
		case 8:
			break;
			
		case 9:
			if ( cancel_delayed_work_sync( &knock_set_work ) )//cancel knock_set_work*
			{
				TPD_LOG("Pending queueu work");
			}
			else
			{
				TPD_LOG("Cancle knock set work");
			}
			ret = hrtimer_try_to_cancel ( &notify_timer );
			TPD_LOG("notify timer cancle  = %d ",ret	);
			if ( ret  < 0 )								  //  0 when the timer was not active  
			{
				TPD_LOG("Pending htimer callback");
				hrtimer_cancel ( &notify_timer );
			}

			int *v = &value[0];
			lpwg_mode = *(v + 0);
			screen    = *(v + 1);
			sensor    = *(v + 2);
			qcover    = *(v + 3);
			
			if(lpwg_mode == 1)//Knock on
			{
				knock_on_enable   = 1; 
				double_tap_enable = 1;
				multi_tap_enable  = 0;				
			}
			else if(lpwg_mode == 2)//Knock code
			{
				knock_on_enable   = 1;
				double_tap_enable = 0;
				multi_tap_enable  = 1; 
			}
			else
			{
				knock_on_enable = 0;
				double_tap_enable = 0;
				multi_tap_enable = 0;
			}	
			/*Touch IC Sleep control */
			/*sensor 0 => sensor NEAR touch ic disble*/
			/*sensor 1 => sensor FAR normal operation(enable knock on/code)*/
			if ( suspend_status == 1 && screen == 1 )
				break;
			if(suspend_status == 1 && qcover == 0 /*&& sensor ==0*/)
			{
				DO_SAFE(sleep_control(tpd_i2c_client, sensor, 0), error);			
			}
			else if(suspend_status == 1 && qcover == 1 /*&& sensor == 0*/)//quick cover is close ==> normal operation 
			{
				DO_SAFE(sleep_control(tpd_i2c_client, 1, 0), error);			
				hrtimer_start ( &notify_timer, ktime_set(0, MS_TO_NS(50)), HRTIMER_MODE_REL );
			}
			if(ret == 1)
			{
				hrtimer_start ( &notify_timer, ktime_set(0, MS_TO_NS(50)), HRTIMER_MODE_REL );
				TPD_LOG("Cancle notify timer");
			}
			TPD_LOG("END NOTIFY");
            break;
		
		default:
			break;
		}

	mutex_unlock(&knock_access);
	return size;
error:
	mutex_unlock(&knock_access);
	return -1;
}
static DEVICE_ATTR ( lpwg_notify, 0664, NULL, store_lpwg_notify );

static int Firmware_verification(char *buf)
{
	int ret = 0;
	u8 fw_config_id[5] = {0};
	char fw_product_id[11] = {0};
	char manufacture_id = 0;

	TPD_FUN();
	ret += sprintf ( buf+ret, "\n\n\n" );
	write_log ( buf );
	msleep ( 30 );
	//write_time_log ();
	msleep ( 30 );

	ret = synaptics_ts_read ( tpd_i2c_client, PRODUCT_ID_REG, 10, &fw_product_id[0] );
	if ( ret < 0 )
	{
		TPD_ERR ( "PRODUCT_ID_REG read fail\n" );
	}
	ret = synaptics_ts_read ( tpd_i2c_client, FLASH_CONFIG_ID_REG, 4, &fw_config_id[0] );
	if ( ret < 0 )
	{
		TPD_ERR ( "FLASH_CONFIG_ID_REG read fail\n" );
	}

	ret = synaptics_ts_read ( tpd_i2c_client, MANUFACTURER_ID_REG, 1, &manufacture_id );
	if ( ret < 0 )
	{
		TPD_ERR ( "MANUFACTURER_ID_REG read fail\n" );
	}
	
	ret += sprintf ( buf+ret, "IC_FW_Version (RAW) \t: [%02X%02X%02X%02X]\n", fw_config_id[0], fw_config_id[1], fw_config_id[2], fw_config_id[3] );
	ret += sprintf ( buf+ret, "Product_ID \t\t: [%s] \n", fw_product_id );			
	ret += sprintf ( buf+ret, "Manufacture_ID \t\t: [%d] \n", manufacture_id );		
	ret += sprintf ( buf+ret, "IC_FW_Version\n");		
	

	switch(fw_config_id[0] & 0xF0)		//maker
	{
		case 0x00:
			ret += sprintf ( buf+ret, "Maker \t\t\t: [ELK] \n");
			break;
		case 0x10:
			ret += sprintf ( buf+ret, "Maker \t\t\t: [Suntel] \n");
			break;
		case 0x20:
			ret += sprintf ( buf+ret, "Maker \t\t\t: [Tovis] \n");
			break;
		case 0x30:
			ret += sprintf ( buf+ret, "Maker \t\t\t: [Innotek] \n");
			break;
		default:								
			break;
	}

	switch(fw_config_id[0] & 0x0F)		//key
	{
		case 0x00:
			ret += sprintf ( buf+ret, "Key \t\t\t: [No key] \n");
			break;
		case 0x01:
			ret += sprintf ( buf+ret, "Key \t\t\t: [1 Key] \n");
			break;
		case 0x02:
			ret += sprintf ( buf+ret, "Key \t\t\t: [2 Key] \n");
			break;
		case 0x03:
			ret += sprintf ( buf+ret, "Key \t\t\t: [3 Key] \n");
			break;
		case 0x04:
			ret += sprintf ( buf+ret, "Key \t\t\t: [4Key] \n");
		break;
		default:								
			break;
	}

	switch(fw_config_id[1] & 0xF0 )		//Supplier
	{
		case 0x00:
			ret += sprintf ( buf+ret, "Supplier \t\t: [Synaptics] \n");
			break;
		default:								
			break;
	}

	ret += sprintf ( buf+ret, "Panel(inch) \t\t: [%d.%d] \n", fw_config_id[1] & 0x0F, fw_config_id[2] & 0xF0);		//Panel
	ret += sprintf ( buf+ret, "Version \t\t: [v0.%d] \n", fw_config_id[3] & 0xEF);		//Panel

	return ret;
}

static ssize_t show_sd ( struct device *dev, struct device_attribute *attr, char *buf )
{
	int ret = 0;
	int rx_to_rx = 0;
	int tx_to_tx = 0;
	int tx_to_gnd = 0;
	int high_registance = 0;
	int full_raw_cap = 0;
	u8 fw_config_id[5] = {0};
	char fw_product_id[11] = {0};

	TPD_FUN();
	if ( !suspend_status )
	{
		synaptics_interrupt_enable(0);
		
		ret = Firmware_verification(buf);

		write_log ( buf );
		msleep ( 30 );

		SYNA_PDTScan ();
		SYNA_ConstructRMI_F54 ();
		SYNA_ConstructRMI_F1A ();

		mt_eint_mask ( CUST_EINT_TOUCH_PANEL_NUM );
		rx_to_rx = F54_RxToRxReport();

		if ( rx_to_rx == 2 )
		{
			ret = 0;
			ret += sprintf ( buf+ret, "\nRxToRxReport fail!! try again\n" );
			write_log ( buf );
			synaptics_initialize ( tpd_i2c_client );
			mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );

			//return ret;
		}

		tx_to_tx = F54_TxToTxReport ();
		tx_to_gnd = F54_TxToGndReport ();
		high_registance = F54_HighResistance ();

		if ( get_limit ( numberOfTx, numberOfRx ) < 0 )
		{
			TPD_ERR ( "Can not check the limit of rawcap\n" );
			full_raw_cap = F54_FullRawCap ( 5 );
			TPD_LOG ("F54_FullRawCap ( 5 )\n");
		}
		else
		{
			full_raw_cap = F54_FullRawCap ( 0 );
			TPD_LOG ("F54_FullRawCap ( 0 )\n");
		}

		TPD_LOG ("full_raw_cap=%d\n",full_raw_cap);

		ret += sprintf ( buf+ret, "=======RESULT========\n" );
		ret += sprintf ( buf+ret, "Channel Status : %s\n", ( tx_to_tx && tx_to_gnd && high_registance ) ? "PASS" : "FAIL" );
		ret += sprintf ( buf+ret, "Raw Data : %s\n", (full_raw_cap > 0) ? "PASS" : "FAIL" );

		synaptics_initialize ( tpd_i2c_client );
		mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );
	}
	else
	{
		//write_time_log ();
		ret += sprintf ( buf+ret, "state=[suspend]. we cannot use I2C, now. Test Result: Fail\n" );
	}

	return ret;
}
static DEVICE_ATTR ( sd, 0664, show_sd, NULL );

static ssize_t show_factory_version ( struct device *dev, struct device_attribute *attr, char *buf )
{
	int ret = 0;
	
	if ( !suspend_status )
	{
		ret = Firmware_verification(buf);
	}
	else
	{
		//write_time_log ();
		ret += sprintf ( buf+ret, "state=[suspend]. we cannot use I2C, now. Test Result: Fail\n" );
	}
	return ret;
}
static DEVICE_ATTR ( factory_version, 0664, show_factory_version, NULL );

static struct attribute *lge_touch_attrs[] = {

	&dev_attr_knock_on_type.attr,
	&dev_attr_lpwg_data.attr,
	&dev_attr_lpwg_notify.attr,
	&dev_attr_sd.attr,
	&dev_attr_factory_version.attr,
	&dev_attr_Knock_Parameter.attr,
	&dev_attr_Int_Pin.attr,
	&dev_attr_Do_Release_All_Finger.attr,
	&dev_attr_Do_Initialize.attr,
	&dev_attr_Do_Ic_Reset.attr,
	&dev_attr_Reset_Pin_Raw.attr,
	&dev_attr_Reset_Pin_High.attr,
	&dev_attr_Do_Mask_Int.attr,
	&dev_attr_Do_Un_Mask_Int.attr,
	&dev_attr_Do_Updata_All.attr,
	&dev_attr_Do_Read_Int_Status.attr,
	&dev_attr_Reset_Pin.attr,
	NULL,
};

static struct attribute_group lge_touch_group = {
	.name = LGE_TOUCH_NAME,
	.attrs = lge_touch_attrs,
};


#if defined(LGE_USE_SYNAPTICS_F54)
static ssize_t synaptics_show_f54 ( struct device *dev, struct device_attribute *attr, char *buf )
{
	int ret = 0;

	if ( suspend_status == 0 )
	{
		SYNA_PDTScan ();
		SYNA_ConstructRMI_F54 ();
		SYNA_ConstructRMI_F1A ();

		ret = sprintf ( buf, "====== F54 Function Info ======\n" );

		switch ( f54_fullrawcap_mode )
		{
			case 0:
				ret += sprintf ( buf + ret, "fullrawcap_mode = For sensor\n" );
				break;
			case 1:
				ret += sprintf ( buf + ret, "fullrawcap_mode = For FPC\n" );
				break;
			case 2:
				ret += sprintf ( buf + ret, "fullrawcap_mode = CheckTSPConnection\n" );
				break;
			case 3:
				ret += sprintf ( buf + ret, "fullrawcap_mode = Baseline\n" );
				break;
			case 4:
				ret += sprintf ( buf + ret, "fullrawcap_mode = Delta image\n" );
				break;
		}
		mt_eint_mask(CUST_EINT_TOUCH_PANEL_NUM);

		ret += sprintf ( buf + ret, "F54_FullRawCap(%d) Test Result: %s", f54_fullrawcap_mode, (F54_FullRawCap(f54_fullrawcap_mode) > 0) ? "Pass\n" : "Fail\n" );
		ret += sprintf ( buf + ret, "F54_TxToTxReport() Test Result: %s", (F54_TxToTxReport() > 0) ? "Pass\n" : "Fail\n" );
		ret += sprintf ( buf + ret, "F54_RxToRxReport() Test Result: %s", (F54_RxToRxReport() > 0) ? "Pass\n" : "Fail\n" );
		ret += sprintf ( buf + ret, "F54_TxToGndReport() Test Result: %s", (F54_TxToGndReport() > 0) ? "Pass\n" : "Fail\n" );
		ret += sprintf ( buf + ret, "F54_HighResistance() Test Result: %s", (F54_HighResistance() > 0) ? "Pass\n" : "Fail\n" );

		mt_eint_unmask(CUST_EINT_TOUCH_PANEL_NUM);
	}
	else
	{
		ret = sprintf ( buf + ret, "state=[suspend]. we cannot use I2C, now. Test Result: Fail\n" );
	}

	return ret;
}

static ssize_t synaptics_store_f54 ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int ret = 0;

	ret = sscanf ( buf, "%d", &f54_fullrawcap_mode );

	return size;
}
static DEVICE_ATTR ( f54, 0664, synaptics_show_f54, synaptics_store_f54 );
#endif

#if defined(LGE_USE_SYNAPTICS_FW_UPGRADE)
static ssize_t synaptics_store_firmware ( struct device *dev, struct device_attribute *attr, const char *buf, size_t size )
{
	int ret;
	char path[256] = {0};

	sscanf ( buf, "%s", path );
	TPD_LOG ( "Firmware image update: %s\n", path );

	if ( !strncmp ( path, "1", 1 ) )
	{
		fw_force_update = 1;
	}
	else
	{
		memcpy ( fw_path, path, sizeof(fw_path) );
	}

	ret = synaptics_firmware_update ( tpd_i2c_client );

	synaptics_initialize ( tpd_i2c_client );

	return size;
}
static DEVICE_ATTR ( firmware, 0664, NULL, synaptics_store_firmware );
#endif

static ssize_t synaptics_show_version ( struct device *dev, struct device_attribute *attr, char *buf )
{
	char fw_config_id[5] = {0};
	char fw_product_id[11] = {0};
	int ret;

	ret = synaptics_ts_read ( tpd_i2c_client, PRODUCT_ID_REG, 10, &fw_product_id[0] );
	if ( ret < 0 )
	{
		TPD_ERR ( "PRODUCT_ID_REG read fail\n" );
	}
	ret = synaptics_ts_read ( tpd_i2c_client, FLASH_CONFIG_ID_REG, 4, &fw_config_id[0] );
	if ( ret < 0 )
	{
		TPD_ERR ( "FLASH_CONFIG_ID_REG read fail\n" );
	}
	return sprintf ( buf, "%02x%02x%02x%02x\n", fw_config_id[0],fw_config_id[1],fw_config_id[2],fw_config_id[3]);
	
}
static DEVICE_ATTR ( version, 0664, synaptics_show_version, NULL );

static ssize_t synaptics_store_write ( struct device *dev, struct device_attribute *attr, char *buf, size_t size )
{
	u8 temp = 0;	
	u8 reg = 0;
	u8 value = 0;
	unsigned int page = 0;
	int ret;

	// ( buf, "%d %x %x", &page, &reg, &value );
	TPD_LOG ( "(write) page=%d, reg=0x%x, value=0x%x\n", page, reg, value );

	ret = synaptics_page_data_write ( tpd_i2c_client, page, reg, 1, &value );
	if ( ret < 0 )
	{
		TPD_ERR ( "REGISTER write fail\n" );
	}

	return size;
}
static DEVICE_ATTR ( write, 0664, NULL, synaptics_store_write );

static ssize_t synaptics_store_read ( struct device *dev, struct device_attribute *attr, char *buf, size_t size )
{
	u8 temp = 0;
	u8 reg = 0;
	unsigned int page = 0;
	int ret;

	//sscanf ( buf, "%d %x", &page, &reg );

	ret = synaptics_page_data_read ( tpd_i2c_client, page, reg, 1, &temp );
	if ( ret < 0 )
	{
		TPD_ERR ( "REGISTER read fail\n" );
	}

	TPD_LOG ( "(read) page=%d, reg=0x%x, value=0x%x\n", page, reg, temp );

	return size;
}
static DEVICE_ATTR ( read, 0664, NULL, synaptics_store_read );



static ssize_t synaptics_store_ic_reset ( struct device *dev, struct device_attribute *attr, char *buf, size_t size )
{
	TPD_LOG(" mt_get_gpio_in(before) : 0x%x\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));
	synaptics_interrupt_clear(tpd_i2c_client);
	synaptics_ic_reset();
	TPD_LOG(" mt_get_gpio_in(after) : 0x%x\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));
	return size;
}
static DEVICE_ATTR ( debug_ic_reset, 0664, NULL, synaptics_store_ic_reset );

/****************************************************************************
* Synaptics_Touch_IC Initialize Function
****************************************************************************/
static void read_page_description_table ( struct i2c_client *client )
{
	TPD_FUN ();
	int ret = 0;
	function_descriptor buffer;
	u8 page_num;
	u16 u_address;

	memset(&buffer, 0x0, sizeof(function_descriptor));

	for ( page_num = 0 ; page_num < PAGE_MAX_NUM ; page_num++ )
	{
		ret = synaptics_ts_write ( client, PAGE_SELECT_REG, &page_num, 1 );
		if ( ret < 0 )
		{
			TPD_ERR ( "PAGE_SELECT_REG write fail (page_num=%d)\n", page_num );
		}

		for ( u_address = DESCRIPTION_TABLE_START ; u_address > 10 ; u_address -= sizeof(function_descriptor) )
		{
			ret = synaptics_ts_read ( client, u_address, sizeof(buffer), (u8 *) &buffer );
			if ( ret < 0 )
			{
				TPD_ERR ( "function_descriptor read fail\n" );
				return;
			}

			TPD_LOG ("buffer.function_exist=%x, page_num=%d\n",buffer.function_exist,page_num);
			if ( buffer.function_exist == 0 )
				break;

			switch ( buffer.function_exist )
			{
				case RMI_DEVICE_CONTROL:
					device_control = buffer;
					device_control_page = page_num;
					break;
				case TOUCHPAD_SENSORS:
					finger = buffer;
					finger_page = page_num;
					break;
				case CAPACITIVE_BUTTON_SENSORS:
					button = buffer;
					button_page = page_num;
					break;
				case FLASH_MEMORY_MANAGEMENT:
					flash_memory = buffer;
					flash_memory_page = page_num;
					break;
				case MULTI_TAP_GESTURE:
					multi_tap = buffer;
					multi_tap_page = page_num;
					break;
			}
		}
	}

	ret = synaptics_ts_write ( client, PAGE_SELECT_REG, &device_control_page, 1 );
	if ( ret < 0 )
	{
		TPD_ERR ( "PAGE_SELECT_REG write fail\n" );
	}
}

#if defined(LGE_USE_SYNAPTICS_FW_UPGRADE)
static int synaptics_firmware_check ( struct i2c_client *client )
{
	TPD_FUN ();
	int ret;
	u8 device_status = 0;
	u8 flash_control = 0;
	u8 fw_ver, image_ver;
	char fw_config_id[5] = {0};
	char fw_product_id[11] = {0};
	char image_config_id[5] = {0};
	char image_product_id[11] = {0};

	/* read Firmware information in Download Image */
	fw_start = (unsigned char *) &SynaFirmware[0];
	fw_size = sizeof(SynaFirmware);
	strncpy ( image_product_id, &SynaFirmware[0x0040], 6 );
	strncpy ( image_config_id, &SynaFirmware[0xb100], 4 );

	if ( fw_path[0] != 0 )
	{
		TPD_LOG ( "Firmware force update by fw file\n" );
		return -1;
	}
	else if ( fw_force_update == 1 )
	{
		TPD_LOG ( "Firmware force update by buffer 1 [%02x%02x%02x%02x Ver]\n", image_config_id[0], image_config_id[1], image_config_id[2], image_config_id[3] );
		return -1;
	}

	ret = synaptics_ts_read ( client, DEVICE_STATUS_REG, 1, &device_status );
	if ( ret < 0 )
	{
		TPD_ERR ( "DEVICE_STATUS_REG read fail\n" );
	}
	ret = synaptics_ts_read ( client, FLASH_CONTROL_REG, 1, &flash_control );
	if ( ret < 0 )
	{
		TPD_ERR ( "FLASH_CONTROL_REG read fail\n" );
	}

	if ( ( device_status & FLASH_PROG_MASK ) || ( device_status & FW_CRC_FAILURE_MASK ) != 0 || ( flash_control & FLASH_STATUS_MASK ) != 0 )
	{
		TPD_ERR ( "Firmware has a problem. [device_status=%x, flash_control=%x]\nso it needs Firmware update. [%02x%02x%02x%02x Ver]\n", device_status, flash_control, image_config_id[0], image_config_id[1], image_config_id[2], image_config_id[3] );
		return -1;
	}
	else
	{
		/* read Firmware information in Touch IC */
		ret = synaptics_ts_read ( client, PRODUCT_ID_REG, 10, &fw_product_id[0] );
		if ( ret < 0 )
		{
			TPD_ERR ( "PRODUCT_ID_REG read fail\n" );
		}
		else
		{
			TPD_LOG ( "PRODUCT_ID_REG : %s \n", fw_product_id );
		}

		ret = synaptics_ts_read ( client, FLASH_CONFIG_ID_REG, 4, &fw_config_id[0] );
		if ( ret < 0 )
		{
			TPD_ERR ( "FLASH_CONFIG_ID_REG read fail\n" );
		}
		else
		{
			TPD_LOG ( "FLASH_CONFIG_ID_REG : %02x%02x%02x%02x \n", fw_config_id[0], fw_config_id[1], fw_config_id[2], fw_config_id[3] );
		}
#ifdef WXSERIES_FW
		fw_ver = fw_config_id[3] & 0x7F;
		image_ver = image_config_id[3] & 0x7F;
#else
		fw_ver = ( int ) simple_strtol ( &fw_config_id[3], NULL, 10 );
		image_ver = ( int ) simple_strtol ( &image_config_id[3], NULL, 10 );
#endif
		TPD_LOG("fw_ver : 0x%02x, image_ver : 0x%02x\n", fw_ver , image_ver);

#if 0
		if ( ( !strcmp ( fw_product_id, image_product_id ) ) && ( image_ver != fw_ver ) )
#else
		if ( ( image_ver != fw_ver ) ) //temp
#endif
		{
			TPD_LOG ( "[%02x ver ==> %02x ver] Firmware Update\n", fw_ver, image_ver );
			return -1;
		}
		else
		{
			TPD_LOG ( "No need to update Firmware\n" );
			return 0;
		}
	}
}
#ifdef WORKQUEUE_FW
static int synaptics_firmware_update ( struct work_struct *work_fw_upgrade )
#else
static int synaptics_firmware_update ( struct i2c_client *client )
#endif
{
	TPD_FUN ();
	int ret;
	
#ifdef WORKQUEUE_FW
	struct i2c_client *client;

	if ( tpd_i2c_client != NULL ) 
		client = tpd_i2c_client;
	else
		return 0;
#endif
	ret	= synaptics_firmware_check ( client );
	
	if ( ret != 0 )
	{
		if ( !download_status )
		{
			download_status = 1;
			wake_lock ( &fw_suspend_lock );

			mt_eint_mask ( CUST_EINT_TOUCH_PANEL_NUM );

			if ( !knock_on_enable && suspend_status )
			{
				synaptics_power ( 1 );
			}

			mtk_wdt_enable ( WK_WDT_DIS );
			TPD_LOG ( "Watchdog disable\n" );

			ret = FirmwareUpgrade ( client, fw_path, fw_size, fw_start );
			if ( ret < 0 )
			{
				TPD_ERR ( "Firmware update Fail!!!\n" );
			}
			else
			{
				TPD_ERR ( "Firmware upgrade Complete\n" );
			}

			mtk_wdt_enable ( WK_WDT_EN );
			TPD_LOG ( "Watchdog enable\n" );
			mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );
			
			memset(fw_path, 0x00, sizeof(fw_path));
			fw_force_update = 0;

				synaptics_ic_reset ();


			wake_unlock ( &fw_suspend_lock );
			download_status = 0;
		}
		else
		{
			TPD_ERR ( "Firmware Upgrade process is aready working on\n" );
		}
		read_page_description_table ( client );
	}	
#ifdef WORKQUEUE_FW
	synaptics_initialize( client );
#endif
}
#endif

void synaptics_interrupt_clear ( struct i2c_client *client )
{
	TPD_FUN ();
	int ret;
	u8 temp = 0;

	
	temp = 0x7F;
	ret = synaptics_ts_write ( client, INTERRUPT_ENABLE_REG, &temp, 1 );
	if (ret < 0)
	{
		TPD_ERR ( "INTERRUPT_ENABLE_REG write fail\n" );
	}

	ret = synaptics_ts_read ( client, INTERRUPT_STATUS_REG, 1, &temp );
	if ( ret < 0 )
	{
		TPD_ERR ( "INTERRUPT_STATUS_REG read fail\n" );
	}
	TPD_LOG ( "INTERRUPT_STATUS_REG value = %d\n", temp );

}
void synaptics_initialize ( struct i2c_client *client )
{
	TPD_FUN ();
	int ret;
	u8 temp = 0;
	u8 Seg_Aggres = 0;
	ret = synaptics_ts_read ( client, DEVICE_CONTROL_REG, 1, &temp );
	if ( ret < 0 )
	{
		TPD_ERR ( "DEVICE_CONTROL_REG read fail\n" );
	}
	
	temp = NOSLEEP_MASK | CONFIGURED_MASK;
//	temp = temp | CONFIGURED_MASK;
	ret = synaptics_ts_write ( client, DEVICE_CONTROL_REG, &temp, 1 );
	if ( ret < 0 )
	{
		TPD_ERR ( "DEVICE_CONTROL_REG write fail\n" );
	}

	if ( button.function_exist != 0 )
	{
		button_enable_mask = BUTTON_MASK;
	}

	synaptics_interrupt_enable(1);

	temp = REDUCED_REPORTING_MODE_MASK | ABS_POS_FILTER_MASK | REPORT_BEYOND_CLIP_MASK;
	ret = synaptics_ts_write ( client, TWO_D_REPORT_MODE_REG, &temp, 1 );
	if (ret < 0)
	{
		TPD_ERR ( "TWO_D_REPORT_MODE_REG write fail\n" );
	}

	temp = (u8) DELTA_POS_THRESHOLD;
	ret = synaptics_ts_write ( client, TWO_D_DELTA_X_THRESH_REG, &temp, 1 );
	if (ret < 0)
	{
		TPD_ERR ( "TWO_D_DELTA_X_THRESH_REG write fail\n" );
	}

	temp = (u8) DELTA_POS_THRESHOLD;
	ret = synaptics_ts_write ( client, TWO_D_DELTA_Y_THRESH_REG, &temp, 1 );
	if (ret < 0)
	{
		TPD_ERR ( "TWO_D_DELTA_Y_THRESH_REG write fail\n" );
	}

	ret = synaptics_ts_read ( client, INTERRUPT_STATUS_REG, 1, &temp );
	if ( ret < 0 )
	{
		TPD_ERR ( "INTERRUPT_STATUS_REG read fail\n" );
	}
	TPD_LOG ( "INTERRUPT_STATUS_REG value = %d\n", temp );

	Seg_Aggres = finger.control_base+31;
	ret = synaptics_ts_read ( client,Seg_Aggres , 1, &temp );
	if ( ret < 0 )
	{
		TPD_ERR ( "SEG_AGGRESS_REGUSTER read fail\n" );
	}
	TPD_LOG("BEFORE1 SEG AGGRESS = %d",temp);
	temp = 0x32;
	ret = synaptics_ts_write ( client, Seg_Aggres, &temp, 1);
	if ( ret < 0 )
	{
		TPD_ERR ( "SEG_AGGRESS_REGUSTER write fail\n" );
	}
/*
	if ( mt_get_gpio_in(GPIO_CTP_EINT_PIN) == 0x0 && g_reset_cnt && temp != ABS0_MASK )
	{
		TPD_LOG("interrupt not clear. run the touch ic reset & clear (%d/%d) \n", g_reset_cnt, MAX_RESET_CNT);
		synaptics_interrupt_clear(tpd_i2c_client);
		synaptics_ic_reset();
		TPD_LOG(" mt_get_gpio_in(after in synaptics_initialize) : 0x%x\n", mt_get_gpio_in(GPIO_CTP_EINT_PIN));
		g_reset_cnt--;
	}

	if ( !g_reset_cnt ) 
		TPD_LOG("check the touch ic.. there are posssible to break interrupt line or registance. can't not clear bit. (%d)\n", temp);

	*/	
}

void synaptics_init_sysfs ( void )
{
	TPD_FUN ();
	int err;

	touch_class = class_create(THIS_MODULE, "touch");

#if defined(LGE_USE_SYNAPTICS_F54)
	touch_debug_dev = device_create ( touch_class, NULL, 0, NULL, "debug" );
	err = device_create_file ( touch_debug_dev, &dev_attr_f54 );
	if ( err )
	{
		TPD_ERR ( "Touchscreen : [f54] touch device_create_file Fail\n" );
		device_remove_file ( touch_debug_dev, &dev_attr_f54 );
	}


	err = device_create_file ( touch_debug_dev, &dev_attr_debug_ic_reset );
	if ( err )
	{
		TPD_ERR ( "Touchscreen : [exec] touch device_create_file Fail\n" );
		device_remove_file ( touch_debug_dev, &dev_attr_debug_ic_reset );
	}
#endif

#if defined(LGE_USE_SYNAPTICS_FW_UPGRADE)
	touch_fw_dev = device_create ( touch_class, NULL, 0, NULL, "firmware" );
	err = device_create_file ( touch_fw_dev, &dev_attr_firmware );
	if ( err )
	{
		TPD_ERR ( "Touchscreen : [firmware] touch device_create_file Fail\n" );
		device_remove_file ( touch_fw_dev, &dev_attr_firmware );
	}

	err = device_create_file ( touch_fw_dev, &dev_attr_version );
	if ( err )
	{
		TPD_ERR ( "Touchscreen : [version] touch device_create_file Fail\n" );
		device_remove_file ( touch_fw_dev, &dev_attr_version );
	}
#endif

	err = device_create_file ( touch_fw_dev, &dev_attr_write );
	if ( err )
	{
		TPD_ERR ( "Touchscreen : [write] touch device_create_file Fail\n" );
		device_remove_file ( touch_fw_dev, &dev_attr_write );
	}

	err = device_create_file ( touch_fw_dev, &dev_attr_read );
	if ( err )
	{
		TPD_ERR ( "Touchscreen : [read] touch device_create_file Fail\n" );
		device_remove_file ( touch_fw_dev, &dev_attr_read );
	}

	sysfs_create_group(tpd->dev->dev.kobj.parent , &lge_touch_group);
}
EXPORT_SYMBOL ( synaptics_init_sysfs );

#define to_foo_obj(x) container_of(x, struct foo_obj, kobj)
struct foo_attribute {
	struct attribute attr;
	ssize_t (*show)(struct foo_obj *foo, struct foo_attribute *attr, char *buf);
	ssize_t (*store)(struct foo_obj *foo, struct foo_attribute *attr, const char *buf, size_t count);
};
#define to_foo_attr(x) container_of(x, struct foo_attribute, attr)
static ssize_t foo_attr_show(struct kobject *kobj,
			     struct attribute *attr,
			     char *buf){
		struct foo_attribute *attribute;
	struct foo_obj *foo;

	attribute = to_foo_attr(attr);
	foo = to_foo_obj(kobj);

	if (!attribute->show)
		return -EIO;

	return attribute->show(foo, attribute, buf);
}
static ssize_t foo_attr_store(struct kobject *kobj,
			      struct attribute *attr,
			      const char *buf, size_t len)
{
	struct foo_attribute *attribute;
	struct foo_obj *foo;

	attribute = to_foo_attr(attr);
	foo = to_foo_obj(kobj);

	if (!attribute->store)
		return -EIO;

	return attribute->store(foo, attribute, buf, len);
}
static const struct sysfs_ops foo_sysfs_ops = {
	.show = foo_attr_show,
	.store = foo_attr_store,
};
static void foo_release(struct kobject *kobj)
{
	struct foo_obj *foo;

	foo = to_foo_obj(kobj);
	kfree(foo);
}

static ssize_t foo_show(struct foo_obj *foo_obj, struct foo_attribute *attr,
			char *buf)
{
	return sprintf(buf, "%d\n", foo_obj->interrupt);
}
static ssize_t foo_store(struct foo_obj *foo_obj, struct foo_attribute *attr,
			 const char *buf, size_t count)
{
	sscanf(buf, "%du", &foo_obj->interrupt);
	return count;
}
static struct foo_attribute foo_attribute =__ATTR(interrupt, 0664, foo_show, foo_store);
static struct attribute *foo_default_attrs[] = {
	&foo_attribute.attr,
	NULL,	/* need to NULL terminate the list of attributes */
};
static struct kobj_type foo_ktype = {
	.sysfs_ops = &foo_sysfs_ops,
	.release = foo_release,
	.default_attrs = foo_default_attrs,
};
static struct kset *example_kset;

static struct foo_obj *create_foo_obj(const char *name){
	struct foo_obj *foo;
	int retval;
	foo = kzalloc(sizeof(*foo), GFP_KERNEL);
	if (!foo)
		return NULL;
	foo->kobj.kset = example_kset;
	retval = kobject_init_and_add(&foo->kobj, &foo_ktype, NULL, "%s", name);
	if (retval) {
		kobject_put(&foo->kobj);
		return NULL;
	}
	kobject_uevent(&foo->kobj, KOBJ_ADD);
	return foo;
}

static int synaptics_knock_sysfs_init(void)
{
	TPD_FUN();
	struct foo_obj *foo;
	knock_on_type = 1; 
	example_kset = kset_create_and_add("lge", NULL, kernel_kobj);
	foo_obj = create_foo_obj(LGE_TOUCH_NAME);
	if(foo_obj == NULL)
	{
		return -1;
	}
	return 0; 
}

static int synaptics_set_qcover_area(u32 X1, u32 X2, u32 Y1, u32 Y2)
{
	TPD_FUN ();
	int ret = 0;
	u8 buffer[50] = {0};
	int i=0;
	u8 doubleTap_area_reg_addr = LPWG_CONTROL_REG;
	u32 value[4] = {Double_Tap_Area_X1, Double_Tap_Area_X2,Double_Tap_Area_Y1,Double_Tap_Area_Y2};
	if ( synaptics_ts_read ( tpd_i2c_client, LPWG_CONTROL_REG, 7, buffer ) < 0 )
	{
		TPD_ERR ( "LPWG_CONTROL_REG read fail\n" );
		return -EIO;
	}
	TPD_LOG("AREA %d,%d  %d,%d",Double_Tap_Area_X1,Double_Tap_Area_Y1,Double_Tap_Area_X2,Double_Tap_Area_Y2);
	buffer[1] = value[0];//X1 LSB
	buffer[2] = value[2];//Y1 LSB
	buffer[3] = (value[2] & 0xF00) | (value[0]>>8);///Y1 MSB | X1 MSB
	buffer[4] = value[1];
	buffer[5] = value[3];
	buffer[6] = ((value[3] & 0xF00)>>4) | (value[1]>>8);
	ret = synaptics_page_data_write(tpd_i2c_client, device_control_page, doubleTap_area_reg_addr, 8, buffer);
	if(ret < 0)
	{
		TPD_ERR("Doble Tap area set fail");
	}
	return ret;
}

static int synaptics_lpwg_update_all( struct delayed_work *knock_set_work )
{	
	TPD_FUN();

	bool req_lpwg_param = false;
	int sleep_status = 0;
	int lpwg_status = 0;
	u8 interrupt_status = 0;

	TPD_LOG("===================MODE CHANGE SETTING===================\n");

	if(suspend_status == 1)
	{
		synaptics_ic_reset();
		synaptics_initialize ( tpd_i2c_client );
	}
	if(screen == 1)//idle status
	{
		TPD_LOG("LCD = ON idle SETTING");
		sleep_status = 1;
		lpwg_status  = 0;
	}
	else if(screen == 0 && qcover == 1)// only knock on when qcover close up (knock code is disable)
	{
		TPD_LOG("LCD = OFF, QCOVER = CLOSED");
		sleep_status = 1;
		lpwg_status = 1; 
		synaptics_set_qcover_area(Double_Tap_Area_X1, Double_Tap_Area_X2, Double_Tap_Area_Y1, Double_Tap_Area_Y2);//Set Knock on/code area when qcover is closed
	}
	else if(screen == 0 && qcover == 0 &&sensor == 1)//can use knock on/code when hand close up
	{
		TPD_LOG("LCD = OFF, QCOVER = OPEN, SENSOR = FAR, LPWG_MODE = %s", lpwg_mode == 1 ? "double tap mode" : (lpwg_mode == 2 ? "multi tap mode" : (lpwg_mode == 0 ? "Idle mode" : "None")));
		sleep_status = 1;
		lpwg_status = lpwg_mode;
	}
	else if(screen == 0 && qcover == 0 && sensor == 0)// but not use knock on/code when sensor is near
	{
		TPD_LOG("LCD = OFF, QCOVER = OPEN, SENSOR = NEAR");

		sleep_status = 0;
		lpwg_status = case_mode;//current mode setting /knock on/code
		//req_lpwg_param = true;
	}
	DO_SAFE(sleep_control(tpd_i2c_client, sleep_status, 0), error);

	if(req_lpwg_param == false)
	{
		DO_SAFE(lpwg_control(lpwg_status), error);
	}
	else
	{
		//set knock parameter
	}

	synaptics_interrupt_clear(tpd_i2c_client);
	if ( key_lock_status == 0 )
	{
		mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );
	}
	else
	{
		mt_eint_mask ( CUST_EINT_TOUCH_PANEL_NUM );
	}
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	TPD_LOG("===================MODE CHANGE END===================\n");
		return 0; 

	error : 
	TPD_LOG("===================MODE CHANGE ERROR END===================\n");
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );	
		return -1;	

}

static int synaptics_workqueue_init(void)
{
	u8 ret = 0;

#ifdef WORKQUEUE_FW
	touch_wq = create_singlethread_workqueue("touch_wq");
	if (!touch_wq) {
		TPD_ERR("CANNOT create new workqueue\n");
		ret = -1;
	}

	INIT_WORK(&work_fw_upgrade, synaptics_firmware_update);
#endif

	touch_multi_tap_wq = create_singlethread_workqueue ( "touch_multi_tap_wq" );
	if ( touch_multi_tap_wq )
	{
		INIT_WORK ( &multi_tap_work, synaptics_multi_tap_work );
		hrtimer_init ( &multi_tap_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		multi_tap_timer.function = synaptics_multi_tap_timer_handler;
	}
	else
	{
		TPD_LOG("touch_multi_tap_wq");
		return -1;
	}
	wake_lock_init ( &knock_code_lock, WAKE_LOCK_SUSPEND, "knock_code" );


#ifdef LGE_USE_PROXI_SENSOR
	notify_work_wq = create_singlethread_workqueue ( "notify_work_wq" );
	if ( notify_work_wq )
	{
		hrtimer_init ( &notify_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL );
		notify_timer.function = synaptics_notify_timer_handler;
	}
	else
	{
		TPD_LOG("notify_work_wq Create err");
		return -1;
	}
	wake_lock_init ( &notify_lock, WAKE_LOCK_SUSPEND, "notify_lock" );

	knock_set_work_wq = create_singlethread_workqueue ( "knock_set_work_wq" );
	if ( knock_set_work_wq )
	{
		TPD_LOG("knock_set_work_wq init delay work");
		INIT_DELAYED_WORK ( &knock_set_work, synaptics_lpwg_update_all );	
	}
	else
	{
		TPD_LOG("Knock_set_work_wq Create err");
		return -1;
	}
#endif

	return 0;
}



/****************************************************************************
* I2C BUS Related Functions
****************************************************************************/
static int synaptics_i2c_probe ( struct i2c_client *client, const struct i2c_device_id *id )
{
	TPD_FUN ();
	int err = 0; 
 	int ret = 0;
	struct task_struct *thread = NULL;
	
	/* i2c_check_functionality */
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		TPD_ERR("i2c functionality check error\n");
		ret = -ENODEV;
		goto err_check_functionality_failed;
	}

	/* delay time configuration */
	delay_time.booting = 200;
	delay_time.reset = 100;
	
	/* X, Y max touch position */
	x_max = DISP_GetScreenWidth ();
	y_max = DISP_GetScreenHeight ();

	tpd_i2c_client = client;
#if defined(LGE_USE_SYNAPTICS_F54)
	ds4_i2c_client = client;
#endif

	/* Turn on the power for TOUCH */

	synaptics_power ( 1 );

	mt_set_gpio_mode ( GPIO_CTP_RST_PIN, GPIO_MODE_00 );
	mt_set_gpio_dir ( GPIO_CTP_RST_PIN, GPIO_DIR_OUT );
	synaptics_ic_reset();

	/*Initialize work queue  */
	synaptics_workqueue_init();

	thread = kthread_run ( synaptics_touch_event_handler, 0, TPD_DEVICE );
	if ( IS_ERR ( thread ) )
	{
		err = PTR_ERR ( thread );
		TPD_ERR ( "failed to create kernel thread: %d\n", err );
	}

	/* Configure external ( GPIO ) interrupt */
	synaptics_setup_eint ();

	/* find register map */
	read_page_description_table ( client );

	/*process notify*/
	synaptics_knock_sysfs_init();

#if defined(LGE_USE_SYNAPTICS_FW_UPGRADE)
	wake_lock_init ( &fw_suspend_lock, WAKE_LOCK_SUSPEND, "fw_wakelock" );
	/* Touch Firmware Update */
#ifdef WORKQUEUE_FW
	queue_work(touch_wq, &work_fw_upgrade);
#else
	/* Touch Firmware Update */
	ret = synaptics_firmware_update ( client );
	synaptics_initialize ( client );
#endif 
#endif

	tpd_load_status = 1;
	return 0;

err_check_functionality_failed:
	return ret;
}

static int synaptics_i2c_remove(struct i2c_client *client)
{
	TPD_FUN ();
	return 0;
}

static int synaptics_i2c_detect ( struct i2c_client *client, struct i2c_board_info *info )
{
	TPD_FUN ();
	strcpy ( info->type, "mtk-tpd" );
	return 0;
}

static const struct i2c_device_id tpd_i2c_id[] = { { TPD_DEV_NAME, 0 },	{} };

static struct i2c_driver tpd_i2c_driver = {
	.driver.name = "mtk-tpd",
	.probe = synaptics_i2c_probe,
	.remove = synaptics_i2c_remove,
	.detect = synaptics_i2c_detect,
	.id_table = tpd_i2c_id,
};

/****************************************************************************
* Linux Device Driver Related Functions
****************************************************************************/
static int synaptics_local_init ( void )
{
	TPD_FUN ();

	if ( i2c_add_driver ( &tpd_i2c_driver ) != 0 )
	{
		TPD_ERR ( "i2c_add_driver failed\n" );
		return -1;
	}

	if ( tpd_load_status == 0 )
	{
		TPD_ERR ( "touch driver probing failed\n" );
		i2c_del_driver ( &tpd_i2c_driver );
		return -1;
	}

	tpd_type_cap = 1;

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void synaptics_suspend ( struct early_suspend *h )
{
	TPD_FUN ();
	suspend_status = 1;	
	int ret = 0;


	//TPD_LOG("QEM = %c\n", g_qem_check);
	
	//if(g_qem_check == '1')
	{	
		TPD_LOG("MINI OS MODE_SUSPEND");
		synaptics_power(0);
		return ;
	}///

	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);
	msleep(40);
	synaptics_release_all_finger ();
	//synaptics_release_all_finger();
	
	if ( cancel_delayed_work_sync( &knock_set_work ) )
		TPD_LOG("pending queue work\n");
	if ( hrtimer_try_to_cancel ( &notify_timer ) < 0 )
	{
		TPD_LOG("pending callback\n");
		hrtimer_cancel ( &notify_timer );
	}	
	TPD_LOG("lpwg_mode = %d, screen = %d, sensor = %d, qcover = %d, knock_on_enable = %d, double_tap_enable = %d, multi_tap_enable = %d"
				  , lpwg_mode ,screen ,sensor ,qcover, knock_on_enable, double_tap_enable, multi_tap_enable);
	ret = hrtimer_start ( &notify_timer, ktime_set(0, MS_TO_NS(50)), HRTIMER_MODE_REL );
	TPD_LOG("hrtimer_start return value  = %d",ret);
}

static void synaptics_resume ( struct early_suspend *h )
{
	TPD_FUN ();
	int ret=0;
	u8 buffer[2] = {0,};
	//TPD_LOG("QEM = %c\n", g_qem_check);

	//if(g_qem_check == '1')
	{
		suspend_status = 0;
		TPD_LOG("MINI OS MODE_RESUME");
		synaptics_power(1);
		synaptics_ic_reset();
		synaptics_initialize(tpd_i2c_client);
		synaptics_release_all_finger();
		return ;
	}
	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_OUT);
	msleep(40);
	if ( cancel_delayed_work_sync( &knock_set_work ) )
		TPD_LOG("pending queue work\n");
	if ( hrtimer_try_to_cancel ( &notify_timer ) < 0 )
	{
		TPD_LOG("pending callback\n");
		hrtimer_cancel ( &notify_timer );
	}
	

	synaptics_ic_reset ();
	synaptics_initialize ( tpd_i2c_client );
	synaptics_release_all_finger();
	//hrtimer_start ( &notify_timer, ktime_set(0, MS_TO_NS(50)), HRTIMER_MODE_REL );

	if ( key_lock_status == 0 )
	{
		mt_eint_unmask ( CUST_EINT_TOUCH_PANEL_NUM );
	}
	else
	{
		mt_eint_mask ( CUST_EINT_TOUCH_PANEL_NUM );
	}

	mt_set_gpio_dir(GPIO_CTP_EINT_PIN, GPIO_DIR_IN);
	suspend_status = 0;
}
#endif
static unsigned int synaptics_compare_id()
{
	TPD_FUN ();

	return FALSE;
	/*
	if(mt_get_gpio_in(GPIO_TOUCH_MAKER_ID) == GPIO_IN_ZERO)
	{
	 TPD_LOG("SYNAPTICS detected\n");
	 return TRUE;
	}
	else
	{
	   return FALSE;
	}
	*/
}

static struct i2c_board_info __initdata i2c_tpd = {	I2C_BOARD_INFO ( TPD_DEV_NAME, TPD_I2C_ADDRESS ) };

static struct tpd_driver_t tpd_device_driver = {
	.tpd_device_name = TPD_DEV_NAME,
	.tpd_local_init = synaptics_local_init,
#ifdef CONFIG_HAS_EARLYSUSPEND
	.suspend = synaptics_suspend,
	.resume = synaptics_resume,
#endif
	//.compare_id = synaptics_compare_id,
#ifdef TPD_HAVE_BUTTON
	.tpd_have_button = 1,
#else
	.tpd_have_button = 0,
#endif
};

static int __init synaptics_driver_init ( void )
{
	TPD_FUN ();
	int ret;
	TPD_LOG("NSM CHECK33");///
	I2CDMABuf_va = (u8 *) dma_alloc_coherent ( NULL, 4096, &I2CDMABuf_pa, GFP_KERNEL );
	if ( !I2CDMABuf_va )
	{
		TPD_ERR ( "Allocate Touch DMA I2C Buffer failed!\n" );
		return ENOMEM;
	}
	i2c_register_board_info ( 0, &i2c_tpd, 1 );
	if ( tpd_driver_add ( &tpd_device_driver ) < 0 )
	{
		TPD_ERR ( "tpd_driver_add failed\n" );
	}

	return 0;
}

static void __exit synaptics_driver_exit ( void )
{
	TPD_FUN ();

	tpd_driver_remove ( &tpd_device_driver );

	if ( I2CDMABuf_va )
	{
		dma_free_coherent ( NULL, 4096, I2CDMABuf_va, I2CDMABuf_pa );
		I2CDMABuf_va = NULL;
		I2CDMABuf_pa = 0;
	}
	if ( touch_multi_tap_wq )
	{
		destroy_workqueue ( touch_multi_tap_wq );
	}
}


module_init ( synaptics_driver_init );
module_exit ( synaptics_driver_exit );

MODULE_DESCRIPTION ( "Synaptics 2202 Touchscreen Driver for MTK platform" );
MODULE_AUTHOR ( "BoRam Kim <ticklewind.kim@lge.com>" );
MODULE_LICENSE ( "GPL" );

/* End Of File */
