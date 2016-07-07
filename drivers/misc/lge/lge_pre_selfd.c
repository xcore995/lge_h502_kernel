/* arch/arm/mach-msm/lge/lge_pre_selfd.c
 *
 * Interface to calibrate display color temperature.
 *
 * Copyright (C) 2012 LGE
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

#include <linux/kernel.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/xlog.h>

static struct pre_selfd_platform_data *pre_selfd_pdata;
static char pre_selfd_buf[4096];
unsigned int selfd_cnt= 0;

typedef enum LGBmCableIdTag
{
	USB_CABLE_ID_NONE = 0,
	USB_CABLE_ID_OPEN,
	USB_CABLE_ID_56K,
	USB_CABLE_ID_130K,
	USB_CABLE_ID_180K,
	USB_CABLE_ID_910K,
	USB_CABLE_ID_UNKNOWN,

	USB_CABLE_ID_MAX
}
LGBmCableId;

extern unsigned char g_qem_check;
extern LGBmCableId g_lgbmBootUsbCableId;

static ssize_t pre_selfd_store(struct device *dev, struct device_attribute *attr,
        const char *buf, size_t count)
{
    char str[512];
    int str_len = 0;
    if (!(g_qem_check == '1' || g_lgbmBootUsbCableId == USB_CABLE_ID_56K || g_lgbmBootUsbCableId == USB_CABLE_ID_130K || g_lgbmBootUsbCableId  == USB_CABLE_ID_910K)) return -EINVAL;
    if ( buf[0] =='$' )
    {
        memset(pre_selfd_buf, 0, 4096); selfd_cnt = 0;
        return -EINVAL;
    }
    str_len = sprintf(str, "%s", buf);
    if ( (selfd_cnt+str_len) > 4096 )
    {
        sprintf((char *) &pre_selfd_buf[selfd_cnt], "$");
        selfd_cnt++;
        return selfd_cnt;
    }

    sprintf((char *) &pre_selfd_buf[selfd_cnt], "%s", buf);
    selfd_cnt += str_len;

    return selfd_cnt;
}

static ssize_t pre_selfd_show(struct device *dev, struct device_attribute *attr,
        char *buf)
{
    ssize_t ret_len = 0;
    ret_len = snprintf(buf, PAGE_SIZE, "%s", pre_selfd_buf);
    return ret_len;

}

static DEVICE_ATTR(pre_selfd, 0644, pre_selfd_show, pre_selfd_store);

int lge_pre_self_diagnosis(char *drv_bus_code, int func_code, char *dev_code, char *drv_code, int errno)
{
    char str[512];
    int str_len = 0;

	if (!(g_qem_check == '1' || g_lgbmBootUsbCableId == USB_CABLE_ID_56K || g_lgbmBootUsbCableId == USB_CABLE_ID_130K || g_lgbmBootUsbCableId  == USB_CABLE_ID_910K)) return 1;
    str_len = sprintf(str, "%s|%d|%s|%s|%d\n", drv_bus_code, func_code, dev_code, drv_code, errno);
    if ( (selfd_cnt+str_len) > 4096 )
    {
        sprintf((char *) &pre_selfd_buf[selfd_cnt], "$");
        selfd_cnt++;
        return 1;
    }

    sprintf((char *) &pre_selfd_buf[selfd_cnt], "%s|%d|%s|%s|%d\n", drv_bus_code, func_code, dev_code, drv_code, errno);
    selfd_cnt += str_len;

    return 0;
}

static int pre_selfd_ctrl_probe(struct platform_device *pdev)
{
    int rc = 0;

    pre_selfd_pdata = pdev->dev.platform_data;

    rc = device_create_file(&pdev->dev, &dev_attr_pre_selfd);
    if (rc != 0)
        return -1;
    return 0;
}

static struct platform_driver this_driver = {
    .probe  = pre_selfd_ctrl_probe,
    .driver = {
        .name   = "pre_selfd_ctrl",
    },
};

int __init pre_selfd_ctrl_init(void)
{
    //return platform_driver_register(&this_driver);

    int ret = 0;
    ret = platform_driver_register(&this_driver);
    if (ret)
    {
        xlog_printk(ANDROID_LOG_ERROR, "PRE_SELFD", "failed to register pre_selfd driver\n");
        return ret;
    }
    else
    {
        xlog_printk(ANDROID_LOG_ERROR, "PRE_SELFD", "pre_selfd driver registration done\n");
        return 0;
    }
}


module_init(pre_selfd_ctrl_init);

static void __exit pre_selfd_ctrl_exit(void)
{
    return;
}
module_exit(pre_selfd_ctrl_exit);

MODULE_DESCRIPTION("LGE Pre Self Diagnosis driver");
MODULE_LICENSE("GPL v2");
