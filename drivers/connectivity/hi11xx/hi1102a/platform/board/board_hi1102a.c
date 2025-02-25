

/* 头文件包含 */
#define HI11XX_LOG_MODULE_NAME     "[HI1102A_BOARD]"
#define HI11XX_LOG_MODULE_NAME_VAR hi1102a_board_loglevel
#ifdef _PRE_CONFIG_USE_DTS
#include <linux/of.h>
#include <linux/of_gpio.h>
#endif
/*lint -e322*/ /*lint -e7*/
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/timer.h>
#include <linux/delay.h>
#ifdef CONFIG_PINCTRL
#include <linux/pinctrl/consumer.h>
#endif
/*lint +e322*/ /*lint +e7*/
#include "board.h"
#include "plat_debug.h"
#include "plat_pm.h"
#include "oal_hcc_host_if.h"
#include "plat_uart.h"
#include "plat_firmware.h"
#include "hisi_ini.h"

#undef GPIO_BASE_ADDR
#define GPIO_BASE_ADDR     0X50005000
#define WLAN_HOST2DEV_GPIO ((unsigned int)(1 << 1))
#define WLAN_DEV2HOST_GPIO ((unsigned int)(1 << 0))
#define WLAN_FLOWCTRL_GPIO ((unsigned int)(1 << 13))

#define GPIO_LEVEL_CONFIG_REGADDR       0x0  /* GPIO管脚的电平值拉高或拉低寄存器 */
#define GPIO_INOUT_CONFIG_REGADDR       0x04 /* GPIO管脚的数据方向存器 */
#define GPIO_TYPE_CONFIG_REGADDR        0x30 /* GPIO管脚的模式寄存器:IO or INT */
#define GPIO_INT_POLARITY_REGADDR       0x3C /* GPIO中断极性寄存器 */
#define GPIO_INT_TYPE_REGADDR           0x38 /* GPIO中断触发类型寄存器:电平触发或边沿触发 */
#define GPIO_INT_CLEAR_REGADDR          0x4C /* GPIO清除中断寄存器，只对边沿触发的中断有效 */
#define GPIO_LEVEL_GET_REGADDR          0x50 /* GPIO管脚当前电平值寄存器 */
#define GPIO_INTERRUPT_DEBOUNCE_REGADDR 0x48 /* GPIO管脚是否使能去抖动 */

/* just for p10+ start */
#define DTS_PROP_TCXO_CTRL_ENABLE    "hi110x,tcxo_ctrl_enable"
#define DTS_PROP_GPIO_TCXO_1P95V     "hi110x,gpio_tcxo_1p95v"
#define PROC_NAME_GPIO_TCXO_1P95V    "hi110x_tcxo_1p95v"
#define PROC_NAME_GPIO_WLAN_FLOWCTRL "hi110x_wlan_flowctrl"

/* 全局变量定义 */
#ifdef PLATFORM_DEBUG_ENABLE
int32_t g_device_monitor_enable = 0;
#endif

#ifndef _PRE_WLAN_EXPORT
/* just for p10+ end */
// function hi110x_power_tcxo() just for p10+
OAL_STATIC void hi110x_power_tcxo(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    struct device_node *np = NULL;
    int32 ret;
    int32 physical_gpio = 0;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return;
    }

    ret = of_property_read_bool(np, DTS_PROP_TCXO_CTRL_ENABLE);
    if (ret) {
        ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_TCXO_1P95V, &physical_gpio);
        if (ret != BOARD_SUCC) {
            ps_print_err("get dts prop %s failed\n", DTS_PROP_GPIO_TCXO_1P95V);
            return;
        }
        g_board_info.tcxo_1p95_enable = physical_gpio;
#ifdef GPIOF_OUT_INIT_LOW
        ret = gpio_request_one(g_board_info.tcxo_1p95_enable, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_TCXO_1P95V);
        if (ret) {
            ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_TCXO_1P95V);
            return;
        }
#else
        ret = gpio_request(g_board_info.tcxo_1p95_enable, PROC_NAME_GPIO_TCXO_1P95V);
        if (ret) {
            ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_TCXO_1P95V);
            return;
        }
#endif

        gpio_direction_output(g_board_info.tcxo_1p95_enable, GPIO_LOWLEVEL);
        ps_print_info("hi110X tcxo 1p95v enable gpio:%d\n", g_board_info.tcxo_1p95_enable);
    }
#endif

    return;
}
#endif

STATIC int32_t hi1102a_config_output_gpio_by_dts(int32_t *gpio_enable,
                                                 const char *dts_prop_gpio,
                                                 const char *proc_name_gpio,
                                                 const char *dts_node)
{
    int32_t ret;
    int32_t physical_gpio = 0;

    if (gpio_enable == NULL || dts_prop_gpio == NULL || proc_name_gpio == NULL || dts_node == NULL) {
        ps_print_err("func has NULL input param!!!\n");
        return BOARD_FAIL;
    }

    ret = get_board_gpio(dts_node, dts_prop_gpio, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", dts_prop_gpio);
        return BOARD_FAIL;
    }

#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, proc_name_gpio);
    if (ret) {
        ps_print_err("%s gpio_request failed, gpio_nr = %d\n", proc_name_gpio, physical_gpio);
        return BOARD_FAIL;
    }
#else
    ret = gpio_request(physical_gpio, proc_name_gpio);
    if (ret) {
        ps_print_err("%s gpio_request failed, gpio_nr = %d\n", proc_name_gpio, physical_gpio);
        return BOARD_FAIL;
    }

    gpio_direction_output(physical_gpio, GPIO_LOWLEVEL);
#endif

    *gpio_enable = physical_gpio;

    return BOARD_SUCC;
}

STATIC int32_t hi1102a_config_input_gpio_by_dts(int32_t *gpio_enable,
                                                const char *dts_prop_gpio,
                                                const char *proc_name_gpio,
                                                const char *dts_node)
{
    int32_t ret;
    int32_t physical_gpio = 0;

    if (gpio_enable == NULL || dts_prop_gpio == NULL || proc_name_gpio == NULL || dts_node == NULL) {
        ps_print_err("func has NULL input param!!!\n");
        return BOARD_FAIL;
    }

    ret = get_board_gpio(dts_node, dts_prop_gpio, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", dts_prop_gpio);
        return BOARD_FAIL;
    }

#ifdef GPIOF_IN
    ret = gpio_request_one(physical_gpio, GPIOF_IN, proc_name_gpio);
    if (ret) {
        ps_print_err("%s gpio_request failed, gpio_nr = %d\n", proc_name_gpio, physical_gpio);
        return BOARD_FAIL;
    }
#else
    ret = gpio_request(physical_gpio, proc_name_gpio);
    if (ret) {
        ps_print_err("%s gpio_request failed, gpio_nr = %d\n", proc_name_gpio, physical_gpio);
        return BOARD_FAIL;
    }
    gpio_direction_input(physical_gpio);
#endif

    *gpio_enable = physical_gpio;

    return BOARD_SUCC;
}

int32 hi1102a_get_board_power_gpio(void)
{
    int32 ret;

#ifndef _PRE_WLAN_EXPORT
    int32 physical_gpio = 0;

    /* just for p10+ start */
    hi110x_power_tcxo();
    /* just for p10+ end */
    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_GPIO_SSI_CLK, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_info("get dts prop %s failed, gpio-ssi don't support\n", DTS_PROP_GPIO_HI110X_GPIO_SSI_CLK);
    } else {
        g_board_info.ssi_gpio_clk = physical_gpio;
        ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_GPIO_HI110X_GPIO_SSI_DATA, &physical_gpio);
        if (ret != BOARD_SUCC) {
            ps_print_info("get dts prop %s failed, gpio-ssi don't support\n", DTS_PROP_GPIO_HI110X_GPIO_SSI_DATA);
            g_board_info.ssi_gpio_clk = 0;
        } else {
            g_board_info.ssi_gpio_data = physical_gpio;
            ps_print_info("gpio-ssi support, ssi clk gpio:%d, ssi data gpio:%d\n",
                          g_board_info.ssi_gpio_clk, g_board_info.ssi_gpio_data);
        }
    }
#endif

    /* power enable */
    ret = hi1102a_config_output_gpio_by_dts(&g_board_info.power_on_enable, DTS_PROP_GPIO_HI110X_POWEN_ON,
                                            PROC_NAME_GPIO_POWEN_ON, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
#ifndef _PRE_WLAN_EXPORT
        goto err_get_power_gpio;
#else
        ps_print_info("get dts prop %s failed, gpio_power_on don't support\n", DTS_PROP_GPIO_HI110X_POWEN_ON);
#endif
    }

#ifndef _PRE_NO_USE_BFGX
    /* bfgn enable */
    ret = hi1102a_config_output_gpio_by_dts(&g_board_info.bfgn_power_on_enable, DTS_PROP_GPIO_BFGX_POWEN_ON_ENABLE,
                                            PROC_NAME_GPIO_BFGX_POWEN_ON, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        goto err_get_bfgx_power_gpio;
    }
#endif

    /* wlan enable */
    ret = hi1102a_config_output_gpio_by_dts(&g_board_info.wlan_power_on_enable, DTS_PROP_GPIO_WLAN_POWEN_ON_ENABLE,
                                            PROC_NAME_GPIO_WLAN_POWEN_ON, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        goto err_get_wlan_power_gpio;
    }

    return BOARD_SUCC;

err_get_wlan_power_gpio:
#ifndef _PRE_NO_USE_BFGX
    gpio_free(g_board_info.bfgn_power_on_enable);
err_get_bfgx_power_gpio:
#endif
    gpio_free(g_board_info.power_on_enable);
#ifndef _PRE_WLAN_EXPORT
err_get_power_gpio:
#endif
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_GPIO, CHR_PLAT_DRV_ERROR_POWER_GPIO);

    return BOARD_FAIL;
}

void hi1102a_free_board_power_gpio(void)
{
    gpio_free(g_board_info.power_on_enable);
    gpio_free(g_board_info.bfgn_power_on_enable);
    gpio_free(g_board_info.wlan_power_on_enable);
}

int32 hi1102a_board_wakeup_gpio_init(void)
{
    int32 ret;

#ifndef _PRE_NO_USE_BFGX
    /* bfgx wake host gpio request */
    ret = hi1102a_config_input_gpio_by_dts(&g_board_info.bfgn_wakeup_host, DTS_PROP_HI110X_GPIO_BFGX_WAKEUP_HOST,
                                           PROC_NAME_GPIO_BFGX_WAKEUP_HOST, DTS_NODE_HI110X_BFGX);
    if (ret != BOARD_SUCC) {
        goto err_get_bfgx_wkup_host_gpio;
    }
    ps_print_info("hi110x bfgx wkup host gpio is %d\n", g_board_info.bfgn_wakeup_host);
#endif

    /* wifi wake host gpio request */
    ret = hi1102a_config_input_gpio_by_dts(&g_board_info.wlan_wakeup_host, DTS_PROP_HI110X_GPIO_WLAN_WAKEUP_HOST,
                                           PROC_NAME_GPIO_WLAN_WAKEUP_HOST, DTS_NODE_HI110X_WIFI);
    if (ret != BOARD_SUCC) {
        goto err_get_wlan_wkup_host_gpio;
    }
    ps_print_info("hi110x wifi wkup host gpio is %d\n", g_board_info.wlan_wakeup_host);

    /* host wake wlan gpio request */
    ret  = hi1102a_config_output_gpio_by_dts(&g_board_info.host_wakeup_wlan, DTS_PROP_GPIO_HOST_WAKEUP_WLAN,
                                             PROC_NAME_GPIO_HOST_WAKEUP_WLAN, DTS_NODE_HI110X_WIFI);
    if (ret != BOARD_SUCC) {
#ifndef _PRE_WLAN_EXPORT
        goto err_get_host_wake_up_wlan_fail;
#else
        ps_print_info("get dts prop %s failed, gpio_host_wakeup_wlan don't support\n",
                      DTS_PROP_GPIO_HOST_WAKEUP_WLAN);
#endif
    }
    ps_print_info("hi110x host wkup wifi gpio is %d\n", g_board_info.host_wakeup_wlan);

    return BOARD_SUCC;

#ifndef _PRE_WLAN_EXPORT
err_get_host_wake_up_wlan_fail:
#endif
    gpio_free(g_board_info.wlan_wakeup_host);
err_get_wlan_wkup_host_gpio:
#ifndef _PRE_NO_USE_BFGX
    gpio_free(g_board_info.bfgn_wakeup_host);
err_get_bfgx_wkup_host_gpio:
#endif
    chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                         CHR_PLT_DRV_EVENT_GPIO, CHR_PLAT_DRV_ERROR_WAKEUP_GPIO);

    return BOARD_FAIL;
}

void hi1102a_free_board_wakeup_gpio(void)
{
    gpio_free(g_board_info.bfgn_wakeup_host);
    gpio_free(g_board_info.wlan_wakeup_host);
    gpio_free(g_board_info.host_wakeup_wlan);
}

int32 hi1102a_board_wifi_tas_gpio_init(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret;
    int32 physical_gpio = 0;
    int32 wifi_tas_state = 0;
    struct device_node *np = NULL;

    if (get_board_dts_node(&np, DTS_NODE_HI110X_WIFI) != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_WIFI);
        return BOARD_FAIL;
    }

    if (of_property_read_bool(np, DTS_PROP_WIFI_TAS_EN)) {
        ps_print_info("Hi110x wifi tas enable\n");
        g_board_info.wifi_tas_enable = WIFI_TAS_ENABLE;
    } else {
        ps_print_info("Hi110x wifi tas not enable\n");
        g_board_info.wifi_tas_enable = WIFI_TAS_DISABLE;
        return BOARD_SUCC;
    }

    /* wifi tas control gpio request */
    if (get_board_gpio(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_WIFI_TAS, &physical_gpio) != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", DTS_PROP_GPIO_WIFI_TAS);
        return BOARD_FAIL;
    }

    g_board_info.rf_wifi_tas = physical_gpio;
    ps_print_info("hi110x wifi tas gpio is %d\n", g_board_info.rf_wifi_tas);

#ifdef GPIOF_OUT_INIT_LOW
    ret = of_property_read_u32(np, DTS_PROP_HI110X_WIFI_TAS_STATE, &wifi_tas_state);
    if (ret) {
        ps_print_err("read prop [%s] fail, ret=%d\n", DTS_PROP_HI110X_WIFI_TAS_STATE, ret);
    }

    g_board_info.wifi_tas_gpio_init = (wifi_tas_state == 0) ? GPIOF_INIT_LOW : GPIOF_INIT_HIGH;
    wifi_tas_state = (wifi_tas_state == 0) ?  GPIOF_OUT_INIT_LOW : GPIOF_OUT_INIT_HIGH;
    if (gpio_request_one(physical_gpio, wifi_tas_state, PROC_NAME_GPIO_WIFI_TAS)) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_WIFI_TAS);
        return BOARD_FAIL;
    }
#else
    if (gpio_request(physical_gpio, PROC_NAME_GPIO_WIFI_TAS)) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_WIFI_TAS);
        return BOARD_FAIL;
    } else {
        gpio_direction_input(physical_gpio);
    }
#endif

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

void hi1102a_free_board_wifi_tas_gpio(void)
{
    gpio_free(g_board_info.rf_wifi_tas);
}


int32 hi1102a_board_timesync_gpio_init(void)
{
    int32 ret;
    int32 physical_gpio = 0;

    /* 根据设备树DTS文件获取gnss_timesync GPIO在host侧对应的管脚，保存在physical_gpio中 */
    ret = get_board_gpio(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_GPIO_TIMESYNC, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", DTS_PROP_HI110X_GPIO_TIMESYNC);
        return BOARD_FAIL;
    }

    g_board_info.timesync_gpio = physical_gpio;

    /* 向内核申请使用该管脚 */
#ifdef GPIOF_IN
    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_HI110X_GPIO_TIMESYNC);
    if (ret) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_HI110X_GPIO_TIMESYNC);
        return BOARD_FAIL;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_HI110X_GPIO_TIMESYNC);
    if (ret) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_HI110X_GPIO_TIMESYNC);
        return BOARD_FAIL;
    } else {
        ps_print_info("%s gpio_request succ\n", PROC_NAME_HI110X_GPIO_TIMESYNC);
        gpio_direction_input(physical_gpio);
    }
#endif

    return BOARD_SUCC;
}

void hi1102a_free_board_timesync_gpio(void)
{
    gpio_free(g_board_info.timesync_gpio);
}

/*
 * 函 数 名  : hi1102a_board_flowctrl_gpio_init
 * 功能描述  : 注册gpio流控中断
 */
int32 hi1102a_board_flowctrl_gpio_init(void)
{
    int32 ret;
    int32 physical_gpio = 0;

    /* 根据设备树DTS文件获取流控GPIO在host侧对应的管脚，保存在physical_gpio中 */
    ret = get_board_gpio(DTS_NODE_HI110X_WIFI, DTS_PROP_GPIO_WLAN_FLOWCTRL, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", DTS_PROP_GPIO_WLAN_FLOWCTRL);
        return BOARD_FAIL;
    }

    g_board_info.flowctrl_gpio = physical_gpio;

    /* 向内核申请使用该管脚 */
#ifdef GPIOF_IN
    ret = gpio_request_one(physical_gpio, GPIOF_IN, PROC_NAME_GPIO_WLAN_FLOWCTRL);
    if (ret) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_FLOWCTRL);
        return BOARD_FAIL;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_GPIO_WLAN_FLOWCTRL);
    if (ret) {
        ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_WLAN_FLOWCTRL);
        return BOARD_FAIL;
    } else {
        ps_print_err("%s gpio_request succ\n", PROC_NAME_GPIO_WLAN_FLOWCTRL);
        gpio_direction_input(physical_gpio);
    }
#endif

    return BOARD_SUCC;
}

void hi1102a_free_board_flowctrl_gpio(void)
{
    gpio_free(g_board_info.flowctrl_gpio);
}

void hi1102a_tcxo_enable(void)
{
    if (g_board_info.tcxo_1p95_enable != 0) {
        ps_print_info("hi110x enable tcxo\n");
        power_state_change(g_board_info.tcxo_1p95_enable, BOARD_POWER_ON);
    } else {
        return;
    }
}

void hi1102a_tcxo_disable(void)
{
    if (g_board_info.tcxo_1p95_enable != 0) {
        ps_print_info("hi110x disable tcxo\n");
        power_state_change(g_board_info.tcxo_1p95_enable, BOARD_POWER_OFF);
    } else {
        return;
    }
}

void hi1102a_chip_power_on(void)
{
    hi1102a_tcxo_enable();
    board_host_wakeup_dev_set(0);
    power_state_change(g_board_info.power_on_enable, BOARD_POWER_ON);
#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
#if defined(PLATFORM_DEBUG_ENABLE)
#endif
#endif
}

void hi1102a_chip_power_off(void)
{
#if defined(_PRE_CONFIG_GPIO_TO_SSI_DEBUG)
#endif
    power_state_change(g_board_info.power_on_enable, BOARD_POWER_OFF);
    hi1102a_tcxo_disable();
    board_host_wakeup_dev_set(0);
}

int32 hi1102a_wifi_enable(void)
{
    int32 ret;
    uintptr_t gpio = (uintptr_t)g_board_info.wlan_power_on_enable;

    /* 第一次枚举时BUS 还未初始化 */
    ret = hcc_bus_power_ctrl_register(hcc_get_current_110x_bus(), HCC_BUS_CTRL_POWER_UP,
                                      board_wlan_gpio_power_on, (void *)gpio);
    if (ret) {
        ps_print_info("power ctrl down reg failed, ret=%d", ret);
    }
    ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_UP);
    return ret;
}

int32 hi1102a_wifi_disable(void)
{
    int32 ret;
    uintptr_t gpio = (uintptr_t)g_board_info.wlan_power_on_enable;

    ret = hcc_bus_power_ctrl_register(hcc_get_current_110x_bus(), HCC_BUS_CTRL_POWER_DOWN,
                                      board_wlan_gpio_power_off, (void *)gpio);
    if (ret) {
        ps_print_info("power ctrl down reg failed, ret=%d", ret);
    }
    ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_DOWN);
    return ret;
}

void hi1102a_bfgx_enable(void)
{
    power_state_change(g_board_info.bfgn_power_on_enable, BOARD_POWER_ON);
}

void hi1102a_bfgx_disable(void)
{
    power_state_change(g_board_info.bfgn_power_on_enable, BOARD_POWER_OFF);
}

int32 hi1102a_board_power_on(uint32 ul_subsystem)
{
    int32 ret = SUCC;
    if (ul_subsystem == WLAN_POWER) {
        if (bfgx_is_shutdown()) {
            ps_print_info("wifi pull up power_on_enable gpio!\n");
            hi1102a_chip_power_on();
            hi1102a_bfgx_enable();
        }
        ret = hi1102a_wifi_enable();
    } else if (ul_subsystem == BFGX_POWER) {
        if (wlan_is_shutdown()) {
            ps_print_info("bfgx pull up power_on_enable gpio!\n");
            hi1102a_chip_power_on();
            ret = hi1102a_wifi_enable();
        }
        hi1102a_bfgx_enable();
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
    }

    return ret;
}

int32 hi1102a_board_power_off(uint32 ul_subsystem)
{
    if (ul_subsystem == WLAN_POWER) {
        hi1102a_wifi_disable();
        if (bfgx_is_shutdown()) {
            ps_print_info("wifi pull down power_on_enable!\n");
            hi1102a_bfgx_disable();
            hi1102a_chip_power_off();
        }
    } else if (ul_subsystem == BFGX_POWER) {
        hi1102a_bfgx_disable();
        if (wlan_is_shutdown()) {
            ps_print_info("bfgx pull down power_on_enable!\n");
            hi1102a_wifi_disable();
            hi1102a_chip_power_off();
        }
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return -EFAIL;
    }

    return SUCC;
}

int32 hi1102a_board_power_reset(uint32 ul_subsystem)
{
    int32 ret = 0;
    hi1102a_bfgx_disable();
    hi1102a_wifi_disable();
    hi1102a_chip_power_off();
    hi1102a_chip_power_on();
    ret = hi1102a_wifi_enable();
    hi1102a_bfgx_enable();
    return ret;
}

void hi1102a_bfgx_subsys_reset(void)
{
    hi1102a_bfgx_disable();
    mdelay(10);
    hi1102a_bfgx_enable();
}

int32 hi1102a_wifi_subsys_reset(void)
{
    int32 ret;
    hi1102a_wifi_disable();
    mdelay(10);
    ret = hi1102a_wifi_enable();
    return ret;
}

int32 hi1102a_bfgx_dev_power_on(void)
{
    int32 ret;
    int32 error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata();

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    ret = hi1102a_board_power_on(BFGX_POWER);
    if (ret) {
        ps_print_err("hi1103_board_power_on bfg failed, ret=%d\n", ret);
        return BFGX_POWER_FAILED;
    }

    if (open_tty_drv(ps_core_d->pm_data) != BFGX_POWER_SUCCESS) {
        ps_print_err("open tty fail!\n");
        error = BFGX_POWER_TTY_OPEN_FAIL;
        goto bfgx_power_on_fail;
    }

    if (wlan_is_shutdown()) {
        if (firmware_download_function(BFGX_CFG) != BFGX_POWER_SUCCESS) {
            hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
            ps_print_err("bfgx download firmware fail!\n");
            error = BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
            goto bfgx_power_on_fail;
        }
        hcc_bus_disable_state(pm_data->pst_wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
#ifdef PLATFORM_DEBUG_ENABLE
        if (!g_device_monitor_enable) {
#endif
            hi1102a_wifi_disable();
#ifdef PLATFORM_DEBUG_ENABLE
        }
#endif
    } else {
        /* 此时BFGX 需要解复位BCPU */
        ps_print_err("wifi dereset bcpu\n");
        if (wlan_pm_open_bcpu() != BFGX_POWER_SUCCESS) {
            ps_print_err("wifi dereset bcpu fail!\n");
            error = BFGX_POWER_WIFI_DERESET_BCPU_FAIL;
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_BFGX_PWRON_BY_WIFI);

            goto bfgx_power_on_fail;
        }
    }

    return BFGX_POWER_SUCCESS;

bfgx_power_on_fail:
    hi1102a_board_power_off(BFGX_POWER);
    return error;
}

int32 hi1102a_bfgx_dev_power_off(void)
{
    int32 error = BFGX_POWER_SUCCESS;
    struct ps_core_s *ps_core_d = NULL;
    struct pm_drv_data *pm_data = pm_get_drvdata();

    ps_print_info("%s\n", __func__);

    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return BFGX_POWER_FAILED;
    }

    ps_get_core_reference(&ps_core_d);
    if (unlikely(ps_core_d == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if (uart_bfgx_close_cmd() != SUCCESS) {
        /* bfgx self close fail 了，后面也要通过wifi shutdown bcpu */
        ps_print_err("bfgx self close fail\n");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_CLOSE_BCPU);
    }

    if (release_tty_drv(ps_core_d->pm_data) != SUCCESS) {
        /* 代码执行到此处，说明六合一所有业务都已经关闭，无论tty是否关闭成功，device都要下电 */
        ps_print_err("wifi off, close tty is err!");
    }

    if (!wlan_is_shutdown()) {
        ps_print_err("wifi shutdown bcpu\n");
        if (wlan_pm_shutdown_bcpu_cmd() != SUCCESS) {
            ps_print_err("wifi shutdown bcpu fail\n");
            chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                                 CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_WIFI_CLOSE_BCPU);
            error = BFGX_POWER_FAILED;
        }
    }

    ps_print_info("%s,set BFGX_SLEEP\n", __func__);
    pm_data->bfgx_dev_state = BFGX_SLEEP;
    pm_data->uart_state = UART_NOT_READY;
    pm_data->uart_baud_switch_to = g_default_baud_rate;

    hi1102a_board_power_off(BFGX_POWER);

    return error;
}

int32 hi1102a_hitalk_power_off(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    /* 先关闭SDIO TX通道 */
    hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_TX);

    /* wakeup dev,send poweroff cmd to wifi */
    if (wlan_pm_poweroff_cmd() != OAL_SUCC) {
        /* wifi self close 失败了也继续往下执行，uart关闭WCPU，异常恢复推迟到wifi下次open的时候执行 */
        declare_dft_trace_key_info("hitalk_poweroff_by_sdio_fail", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_HITALK_CLOSE_WCPU);
    }

    hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_ALL);

    hi1102a_board_power_off(WLAN_POWER);

    pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;

    return SUCCESS;
}

int32 hi1102a_hitalk_power_on(void)
{
    int32 ret;
    int32 error = WIFI_POWER_SUCCESS;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    ret = hi1102a_board_power_on(WLAN_POWER);
    if (ret) {
        ps_print_err("hi1102a_board_power_on wlan failed ret=%d\n", ret);
        return -FAILURE;
    }

    hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    if (bfgx_is_shutdown()) {
        error = firmware_download_function(BFGX_AND_HITALK_CFG);
        hi1102a_bfgx_disable();
    } else {
        error = firmware_download_function(HITALK_CFG);
    }

    if (error != WIFI_POWER_SUCCESS) {
        ps_print_err("firmware download fail\n");
        error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        goto hitalk_power_fail;
    } else {
        pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_OPEN;
    }

    ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
    if (ret != 0) {
        declare_dft_trace_key_info("hitalk_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio_fail", OAL_DFT_TRACE_FAIL);
        ps_print_err("hitalk_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio fail ret=%d", ret);
        error = WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL;
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_HITALK_WCPU_BOOTUP);
        goto hitalk_power_fail;
    }

    return WIFI_POWER_SUCCESS;
hitalk_power_fail:
#ifdef CONFIG_HUAWEI_DSM
    hw_1102a_dsm_client_notify(DSM_1102A_DOWNLOAD_FIRMWARE, "%s: failed to download firmware\n", __FUNCTION__);
#endif
    return error;
}

int32 hi1102a_wlan_power_off(void)
{
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    /* 先关闭SDIO TX通道 */
    hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_TX);

    /* wakeup dev,send poweroff cmd to wifi */
    if (wlan_pm_poweroff_cmd() != OAL_SUCC) {
        /* wifi self close 失败了也继续往下执行，uart关闭WCPU，异常恢复推迟到wifi下次open的时候执行 */
        declare_dft_trace_key_info("wlan_poweroff_by_sdio_fail", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_CLOSE_WCPU);
    }

    hcc_bus_disable_state(hcc_get_current_110x_bus(), OAL_BUS_STATE_ALL);

    hi1102a_board_power_off(WLAN_POWER);

    pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_SHUTDOWN;

    return SUCCESS;
}

int32 hi1102a_wlan_power_on(void)
{
    int32 ret;
    int32 error = WIFI_POWER_SUCCESS;
    struct pm_drv_data *pm_data = pm_get_drvdata();
    if (pm_data == NULL) {
        ps_print_err("pm_data is NULL!\n");
        return -FAILURE;
    }

    ret = hi1102a_board_power_on(WLAN_POWER);
    if (ret) {
        ps_print_err("hi1102a_board_power_on wlan failed ret=%d\n", ret);
        return -FAILURE;
    }

    hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    if (bfgx_is_shutdown()) {
        error = firmware_download_function(BFGX_AND_WIFI_CFG);
        hi1102a_bfgx_disable();
    } else {
        error = firmware_download_function(WIFI_CFG);
    }

    if (error != WIFI_POWER_SUCCESS) {
        ps_print_err("firmware download fail\n");
        error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        goto wifi_power_fail;
    } else {
        pm_data->pst_wlan_pm_info->ul_wlan_power_state = POWER_STATE_OPEN;
    }

    ret = hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LAUCH);
    if (ret != 0) {
        declare_dft_trace_key_info("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio_fail", OAL_DFT_TRACE_FAIL);
        ps_print_err("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio fail ret=%d", ret);
        error = WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL;
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP);
        goto wifi_power_fail;
    }

    return WIFI_POWER_SUCCESS;
wifi_power_fail:
#ifdef CONFIG_HUAWEI_DSM
    hw_1102a_dsm_client_notify(DSM_1102A_DOWNLOAD_FIRMWARE, "%s: failed to download firmware\n", __FUNCTION__);
#endif
    return error;
}

int32 hi1102a_get_board_pmu_clk32k(void)
{
#ifndef  _PRE_WLAN_EXPORT
    int32 ret;
    const char *clk_name = NULL;

    ret = get_board_custmize(DTS_NODE_HISI_HI110X, DTS_PROP_CLK_32K, &clk_name);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }
    g_board_info.clk_32k_name = clk_name;

    ps_print_info("hi110x 32k clk name is %s\n", g_board_info.clk_32k_name);
#endif
    return BOARD_SUCC;
}

int32 hi1102a_get_board_uart_port(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret;
    struct device_node *np = NULL;
    const char *uart_port = NULL;
#endif

#ifdef _PRE_NO_USE_BFGX
    ps_print_info("Hi110x bfgx not enable\n\n");
    return BOARD_SUCC;
#endif

#ifdef _PRE_CONFIG_USE_DTS
    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return BOARD_FAIL;
    }

    /* 使用uart4，需要在dts里新增DTS_PROP_UART_PCLK项，指明uart4不依赖sensorhub */
    ret = of_property_read_bool(np, DTS_PROP_HI110X_UART_PCLK);
    if (ret) {
        ps_print_info("uart pclk normal\n");
        g_board_info.uart_pclk = UART_PCLK_NORMAL;
    } else {
        ps_print_info("uart pclk from sensorhub\n");
        g_board_info.uart_pclk = UART_PCLK_FROM_SENSORHUB;
    }

    ret = get_board_custmize(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_UART_POART, &uart_port);
    if (ret != BOARD_SUCC) {
        return BOARD_FAIL;
    }
    g_board_info.uart_port = uart_port;

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

#ifdef HAVE_HISI_IR
int32 hi1102a_board_ir_ctrl_gpio_init(void)
{
    int32 ret;
    int32 physical_gpio = 0;

    ret = get_board_gpio(DTS_NODE_HI110X_BFGX, DTS_PROP_GPIO_BFGX_IR_CTRL, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_info("dts prop %s not exist\n", DTS_PROP_GPIO_BFGX_IR_CTRL);
        g_board_info.bfgx_ir_ctrl_gpio = -1;
    } else {
        g_board_info.bfgx_ir_ctrl_gpio = physical_gpio;

#ifdef GPIOF_OUT_INIT_LOW
        ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_GPIO_BFGX_IR_CTRL);
        if (ret) {
            ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_IR_CTRL);
        }
#else
        ret = gpio_request(physical_gpio, PROC_NAME_GPIO_BFGX_IR_CTRL);
        if (ret) {
            ps_print_err("%s gpio_request failed\n", PROC_NAME_GPIO_BFGX_IR_CTRL);
        } else {
            gpio_direction_output(physical_gpio, 0);
        }
#endif
    }

    return BOARD_SUCC;
}

int32 hi1102a_board_ir_ctrl_pmic_init(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret = BOARD_FAIL;
    struct device_node *np = NULL;
    int32 irled_voltage = 0;
    if (pdev == NULL) {
        ps_print_err("board pmu pdev is NULL!\n");
        return ret;
    }

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        return ret;
    }

    g_board_info.bfgn_ir_ctrl_ldo = regulator_get(&pdev->dev, DTS_PROP_HI110X_IRLED_LDO_POWER);

    if (IS_ERR(g_board_info.bfgn_ir_ctrl_ldo)) {
        ps_print_err("board_ir_ctrl_pmic_init get ird ldo failed\n");
        return ret;
    }

    ret = of_property_read_u32(np, DTS_PROP_HI110X_IRLED_VOLTAGE, &irled_voltage);
    if (ret == BOARD_SUCC) {
        ps_print_info("set irled voltage %d mv\n", irled_voltage / VOLATAGE_V_TO_MV); /* V->mV */
        ret = regulator_set_voltage(g_board_info.bfgn_ir_ctrl_ldo, (int)irled_voltage, (int)irled_voltage);
        if (ret) {
            ps_print_err("board_ir_ctrl_pmic_init set voltage ldo failed\n");
            return ret;
        }
    } else {
        ps_print_err("get irled voltage failed ,use default\n");
    }

    ret = regulator_set_mode(g_board_info.bfgn_ir_ctrl_ldo, REGULATOR_MODE_NORMAL);
    if (ret) {
        ps_print_err("board_ir_ctrl_pmic_init set ldo mode failed\n");
        return ret;
    }
    ps_print_info("board_ir_ctrl_pmic_init success\n");
    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}
#endif

int32 hi1102a_board_ir_ctrl_init(struct platform_device *pdev)
{
#if (defined HAVE_HISI_IR) && (defined _PRE_CONFIG_USE_DTS)
    int ret;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HI110X_BFGX);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HI110X_BFGX);
        goto err_get_ir_ctrl_gpio;
    }
    g_board_info.have_ir = of_property_read_bool(np, "have_ir");
    if (!g_board_info.have_ir) {
        ps_print_err("board has no Ir");
    } else {
        ret = of_property_read_u32(np, DTS_PROP_HI110X_IR_LDO_TYPE, &g_board_info.irled_power_type);
        ps_print_info("read property ret is %d, irled_power_type is %d\n", ret, g_board_info.irled_power_type);
        if (ret != BOARD_SUCC) {
            ps_print_err("get dts prop %s failed\n", DTS_PROP_HI110X_IR_LDO_TYPE);
            goto err_get_ir_ctrl_gpio;
        }

        if (g_board_info.irled_power_type == IR_GPIO_CTRL) {
            ret = hi1102a_board_ir_ctrl_gpio_init();
            if (ret != BOARD_SUCC) {
                ps_print_err("ir_ctrl_gpio init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        } else if (g_board_info.irled_power_type == IR_LDO_CTRL) {
            ret = hi1102a_board_ir_ctrl_pmic_init(pdev);
            if (ret != BOARD_SUCC) {
                ps_print_err("ir_ctrl_pmic init failed\n");
                goto err_get_ir_ctrl_gpio;
            }
        } else {
            ps_print_err("get ir_ldo_type failed!err num is %d\n", g_board_info.irled_power_type);
            goto err_get_ir_ctrl_gpio;
        }
    }
    return BOARD_SUCC;

err_get_ir_ctrl_gpio:
    return BOARD_FAIL;
#else
    return BOARD_SUCC;
#endif
}

int32 hi1102a_board_fm_lan_gpio_init(void)
{
    int32 ret;
    int32 physical_gpio = 0;

    /* 根据设备树DTS文件获fm_lan在host侧对应的管脚，保存在physical_gpio中 */
    ret = get_board_gpio(DTS_NODE_HI110X_BFGX, DTS_PROP_HI110X_GPIO_FM_LAN, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("[fm_lan]get dts prop %s failed\n", DTS_PROP_HI110X_GPIO_FM_LAN);
        return BOARD_FAIL;
    }

    ps_print_info("[fm_lan] get gpio %d", physical_gpio);

    g_board_info.fm_lan_gpio = physical_gpio;

#ifdef GPIOF_OUT_INIT_LOW
    ret = gpio_request_one(physical_gpio, GPIOF_OUT_INIT_LOW, PROC_NAME_HI110X_GPIO_FM_LAN);
    if (ret) {
        ps_print_err("[fm_lan]%s gpio_request failed\n", PROC_NAME_HI110X_GPIO_FM_LAN);
        return BOARD_FAIL;
    }
#else
    ret = gpio_request(physical_gpio, PROC_NAME_HI110X_GPIO_FM_LAN);
    if (ret) {
        ps_print_err("[fm_lan]%s gpio_request failed\n", PROC_NAME_HI110X_GPIO_FM_LAN);
        return BOARD_FAIL;
    } else {
        gpio_direction_output(physical_gpio, 0);
    }
#endif

    return BOARD_SUCC;
}

void hi1102a_board_free_fm_lan_gpio(void)
{
    gpio_free(g_board_info.fm_lan_gpio);
}

int32 hi1102a_check_evb_or_fpga(void)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret;
    struct device_node *np = NULL;

    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        return BOARD_FAIL;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_VERSION);
    if (ret) {
        ps_print_info("chip_type %d ASIC VERSION\n", BOARD_VERSION_HI1102A);
        g_board_info.is_asic = VERSION_ASIC;
    } else {
        ps_print_info("chip_type %d FPGA VERSION\n", BOARD_VERSION_HI1102A);
        g_board_info.is_asic = VERSION_FPGA;
    }

    return BOARD_SUCC;
#else
    return BOARD_SUCC;
#endif
}

int32 hi1102a_board_get_power_pinctrl(struct platform_device *pdev)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret;
    int32 physical_gpio = 0;
    struct device_node *np = NULL;
    struct pinctrl *pinctrl = NULL;
    struct pinctrl_state *pinctrl_def = NULL;
    struct pinctrl_state *pinctrl_idle = NULL;

    /* 检查是否需要prepare before board power on */
    /* JTAG SELECT 拉低，XLDO MODE选择2.8v */
    ret = get_board_dts_node(&np, DTS_NODE_HISI_HI110X);
    if (ret != BOARD_SUCC) {
        ps_print_err("DTS read node %s fail!!!\n", DTS_NODE_HISI_HI110X);
        goto err_read_dts_node;
    }

    ret = of_property_read_bool(np, DTS_PROP_HI110X_POWER_PREPARE);
    if (ret) {
        ps_print_info("need prepare before board power on\n");
        g_board_info.need_power_prepare = NEED_POWER_PREPARE;
    } else {
        ps_print_info("no need prepare before board power on\n");
        g_board_info.need_power_prepare = NO_NEED_POWER_PREPARE;
    }

    if (g_board_info.need_power_prepare == NO_NEED_POWER_PREPARE) {
        return BOARD_SUCC;
    }

    pinctrl = devm_pinctrl_get(&pdev->dev);
    if (OAL_IS_ERR_OR_NULL(pinctrl)) {
        ps_print_err("iomux_lookup_block failed, and the value of pinctrl is %ld\n", PTR_ERR(pinctrl));
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_DTS, CHR_PLAT_DRV_ERROR_PWR_PINCTRL);
        goto err_pinctrl_get;
    }
    g_board_info.pctrl = pinctrl;

    pinctrl_def = pinctrl_lookup_state(pinctrl, "default");
    if (OAL_IS_ERR_OR_NULL(pinctrl_def)) {
        ps_print_err("pinctrl_lookup_state default failed, and the value of pinctrl_def is %ld\n",
                     PTR_ERR(pinctrl_def));
        goto err_lookup_default;
    }
    g_board_info.pins_normal = pinctrl_def;

    pinctrl_idle = pinctrl_lookup_state(pinctrl, "idle");
    if (OAL_IS_ERR_OR_NULL(pinctrl_idle)) {
        ps_print_err("pinctrl_lookup_state idel failed, and the value of pinctrl_idle is %ld\n", PTR_ERR(pinctrl_idle));
        goto err_lookup_idle;
    }
    g_board_info.pins_idle = pinctrl_idle;

    ret = pinctrl_select_state(g_board_info.pctrl, g_board_info.pins_normal);
    if (ret < 0) {
        ps_print_err("pinctrl_select_state default failed.\n");
        goto err_select_state;
    }

    /* LTE_TX_ACTIVT GPIO */
    ret = get_board_gpio(DTS_NODE_HISI_HI110X, DTS_PROP_HI110X_GPIO_XLDO_PINMUX, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", DTS_PROP_HI110X_GPIO_XLDO_PINMUX);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_DTS, CHR_PLAT_DRV_ERROR_XLDO_GPIO_PINMUX);
        goto err_get_xldo_pinmux;
    }

    g_board_info.xldo_pinmux = physical_gpio;

    ret = of_property_read_u32(np, DTS_PROP_GPIO_XLDO_LEVEL, &physical_gpio);
    if (ret != BOARD_SUCC) {
        ps_print_err("get dts prop %s failed\n", DTS_PROP_GPIO_XLDO_LEVEL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_DTS, CHR_PLAT_DRV_ERROR_XLDO_GPIO_LEVEL);
        goto err_read_xldo_level;
    }

    g_board_info.gpio_xldo_level = physical_gpio;

    return BOARD_SUCC;

err_read_xldo_level:
err_get_xldo_pinmux:
err_select_state:
err_lookup_idle:
err_lookup_default:
    devm_pinctrl_put(g_board_info.pctrl);
err_pinctrl_get:
err_read_dts_node:

    return BOARD_FAIL;
#else
    return BOARD_SUCC;
#endif
}

int32 hi1102a_get_ini_file_name_from_dts(int8 *dts_prop, int8 *prop_value, uint32 size)
{
#ifdef _PRE_CONFIG_USE_DTS
    int32 ret;
    struct device_node *np = NULL;
    int32 len;
    int8 out_str[HISI_CUST_NVRAM_LEN] = {0};

    np = of_find_compatible_node(NULL, NULL, COST_HI110X_COMP_NODE);
    if (np == NULL) {
        ini_error("dts node %s not found", COST_HI110X_COMP_NODE);
        return INI_FAILED;
    }

    len = of_property_count_u8_elems(np, dts_prop);
    if (len < 0) {
        ini_error("can't get len of dts prop(%s)", dts_prop);
        return INI_FAILED;
    }

    len = ini_min(len, (int32)sizeof(out_str));
    ini_debug("read len of dts prop %s is:%d", dts_prop, len);
    ret = of_property_read_u8_array(np, dts_prop, out_str, len);
    if (ret < 0) {
        ini_error("read dts prop (%s) fail", dts_prop);
        return INI_FAILED;
    }

    len = ini_min(len, (int32)size);
    ret = memcpy_s(prop_value, (size_t)size, out_str, (size_t)len);
    if (ret != EOK) {
        ini_error("memcpy_s error, destlen=%d, srclen=%d\n ", size, len);
        return INI_FAILED;
    }
    ini_debug("dts prop %s value is:%s", dts_prop, prop_value);
#endif
    return INI_SUCC;
}

void hi1102a_dump_gpio_regs(void)
{
    int16 value;
    int32 ret;
    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if (ret) {
        return;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gpio reg 0x%x = 0x%x", GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, &value);
    if (ret) {
        return;
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "gpio reg 0x%x = 0x%x", GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, value);
}

int32 hi1102a_check_device_ready(void)
{
    int16 value;
    int32 ret;

    value = 0;
    ret = read_device_reg16(CHECK_DEVICE_RDY_ADDR, &value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d", CHECK_DEVICE_RDY_ADDR, ret);
        return -OAL_EFAIL;
    }

    /* 读到0x101表示成功 */
    if (value != 0x101) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "device sysctrl reg error, value=0x%x", value);
        return -OAL_EFAIL;
    }

    return OAL_SUCC;
}

int32 hi1102a_check_wlan_wakeup_host(void)
{
    int32 i;
    uint16 value;
    int32 ret;
    const uint32 ul_test_times = 2;

    if (g_board_info.wlan_wakeup_host == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_wakeup_host gpio is zero!");
        return -OAL_EIO;
    }

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d",
                             (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR), ret);
        return -1;
    }

    /* 输出 */
    value |= (WLAN_DEV2HOST_GPIO);

    ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                             (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR), value, ret);
        return -1;
    }

    for (i = 0; i < ul_test_times; i++) {
        ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR, WLAN_DEV2HOST_GPIO);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                                 (GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR), WLAN_DEV2HOST_GPIO, ret);
            return -1;
        }

        oal_msleep(1);

        if (gpio_get_value(g_board_info.wlan_wakeup_host) == 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio high failed!");
            hi1102a_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR, 0x0);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                                 (GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR), WLAN_DEV2HOST_GPIO, ret);
            return -1;
        }

        oal_msleep(1);

        if (gpio_get_value(g_board_info.wlan_wakeup_host) != 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio low failed!");
            hi1102a_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "check d2h wakeup io %d times ok", i + 1);
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "check d2h wakeup io done");
    return OAL_SUCC;
}

int32 hi1102a_check_host_wakeup_wlan(void)
{
    int32 i;
    uint16 value;
    int32 ret;
    const uint32 ul_test_times = 2;

    if (g_board_info.host_wakeup_wlan == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "host_wakeup_wlan gpio is zero!");
        return -OAL_EIO;
    }

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d",
                             (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR), ret);
        return -1;
    }

    /* 输入 */
    value &= (~WLAN_HOST2DEV_GPIO);

    ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                             (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR), value, ret);
        return -1;
    }

    for (i = 0; i < ul_test_times; i++) {
        gpio_direction_output(g_board_info.host_wakeup_wlan, GPIO_HIGHLEVEL);
        oal_msleep(1);

        value = 0;
        ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, &value);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d",
                                 (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR), ret);
            return -1;
        }

        oal_print_hi11xx_log(HI11XX_LOG_DBG, "read 0x%x reg=0x%x", (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR), value);

        value &= WLAN_HOST2DEV_GPIO;

        if (value == 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio high failed!");
            hi1102a_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        gpio_direction_output(g_board_info.host_wakeup_wlan, GPIO_LOWLEVEL);
        oal_msleep(1);

        value = 0;
        ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR, &value);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d",
                                 (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR), ret);
            return -1;
        }

        oal_print_hi11xx_log(HI11XX_LOG_DBG, "read 0x%x reg=0x%x", (GPIO_BASE_ADDR + GPIO_LEVEL_GET_REGADDR), value);

        value &= WLAN_HOST2DEV_GPIO;

        if (value != 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio low failed!");
            hi1102a_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "check h2d wakeup io %d times ok", i + 1);
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "check h2d wakeup io done");
    return 0;
}

int32 hi1102a_check_wlan_flow_ctrl(uint8_t flowctrl_type)
{
    int32 i;
    uint16 value;
    int32 ret;
    const uint32 ul_test_times = 2;

    if (flowctrl_type == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "flowctrl is not supported!");
        return OAL_SUCC;
    }

    if (g_board_info.flowctrl_gpio == 0) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan_flow_ctrl gpio is zero!");
        return -OAL_EIO;
    }

    value = 0;
    ret = read_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, &value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "read 0x%x reg failed, ret=%d",
                             (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR), ret);
        return -1;
    }

    /* 输出 */
    value |= (WLAN_FLOWCTRL_GPIO);

    ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR, value);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                             (GPIO_BASE_ADDR + GPIO_INOUT_CONFIG_REGADDR), value, ret);
        return -1;
    }

    for (i = 0; i < ul_test_times; i++) {
        ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR, WLAN_FLOWCTRL_GPIO);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                                 (GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR), WLAN_FLOWCTRL_GPIO, ret);
            return -1;
        }

        oal_msleep(1);

        if (gpio_get_value(g_board_info.flowctrl_gpio) == 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio high failed!");
            hi1102a_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        ret = write_device_reg16(GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR, 0x0);
        if (ret) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "write 0x%x reg failed,value=0x%x, ret=%d",
                                 (GPIO_BASE_ADDR + GPIO_LEVEL_CONFIG_REGADDR), WLAN_FLOWCTRL_GPIO, ret);
            return -1;
        }

        oal_msleep(1);

        if (gpio_get_value(g_board_info.flowctrl_gpio) != 0) {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "pull gpio low failed!");
            hi1102a_dump_gpio_regs();
            return -OAL_EFAIL;
        }

        oal_print_hi11xx_log(HI11XX_LOG_INFO, "check wlan flow ctrl io %d times ok", i + 1);
    }

    oal_print_hi11xx_log(HI11XX_LOG_INFO, "check wlan flow ctrl io done");
    return OAL_SUCC;
}

STATIC int32_t hi1102a_dev_io_test_prepare(struct pm_drv_data *pm_data)
{
    if (pm_data == NULL) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "pm_data is NULL!");
        return -FAILURE;
    }

    if (!bfgx_is_shutdown()) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "bfgx is open, test abort!");
        bfgx_print_subsys_state();
        return -OAL_ENODEV;
    }

    if (!wlan_is_shutdown()) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "wlan is open, test abort!");
        return -OAL_ENODEV;
    }

    return OAL_SUCC;
}

STATIC int32_t hi1102a_start_gpio_test(struct pm_drv_data *pm_data,
                                       uint8_t flowctrl_type)
{
    int32_t ret;

    hcc_bus_power_action(hcc_get_current_110x_bus(), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    ret = hcc_bus_reinit(pm_data->pst_wlan_pm_info->pst_bus);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "reinit bus %d failed, ret=%d",
                             pm_data->pst_wlan_pm_info->pst_bus->bus_type, ret);
        return ret;
    }

    wlan_pm_init_dev();

    ret = hi1102a_check_device_ready();
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_device_ready failed, ret=%d", ret);
        return ret;
    }

    ret = hi1102a_check_host_wakeup_wlan();
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_host_wakeup_wlan failed, ret=%d", ret);
        return ret;
    }

    ret = hi1102a_check_wlan_wakeup_host();
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_wlan_wakeup_host failed, ret=%d", ret);
        return ret;
    }

    ret = hi1102a_check_wlan_flow_ctrl(flowctrl_type);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "check_wlan_flow_ctrl failed, ret=%d", ret);
        return ret;
    }

    return OAL_SUCC;
}


int32_t g_ft_fail_powerdown_bypass = 0;
int32 hi1102a_factory_io_test(uint8_t flowctrl_type)
{
    int32_t ret;
    declare_time_cost_stru(cost);
    struct pm_drv_data *pm_data = pm_get_drvdata();

    ret = hi1102a_dev_io_test_prepare(pm_data);
    if (ret != OAL_SUCC) {
        return ret;
    }

    oal_get_time_cost_start(cost);

    hcc_bus_wake_lock(pm_data->pst_wlan_pm_info->pst_bus);
    hcc_bus_lock(pm_data->pst_wlan_pm_info->pst_bus);
    /* power on wifi, need't download firmware */
    ret = hi1102a_board_power_on(WLAN_POWER);
    if (ret) {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "power on wlan failed=%d", ret);
        hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);
        return ret;
    }

    ret = hi1102a_start_gpio_test(pm_data, flowctrl_type);
    if (!g_ft_fail_powerdown_bypass || (ret == OAL_SUCC)) {
        hi1102a_board_power_off(WLAN_POWER);
    }

    hcc_bus_unlock(pm_data->pst_wlan_pm_info->pst_bus);
    hcc_bus_wake_unlock(pm_data->pst_wlan_pm_info->pst_bus);

    oal_get_time_cost_end(cost);
    oal_calc_time_cost_sub(cost);
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hi1102a device io test cost %llu us, ret = %d",
                         time_cost_var_sub(cost), ret);
    return ret;
}
oal_module_symbol(hi1102a_factory_io_test);
EXPORT_SYMBOL(hi1102a_chip_power_on);
EXPORT_SYMBOL(hi1102a_chip_power_off);
EXPORT_SYMBOL(hi1102a_bfgx_disable);
EXPORT_SYMBOL(hi1102a_bfgx_enable);
