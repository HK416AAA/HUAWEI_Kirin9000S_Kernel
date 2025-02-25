/**
 * Copyright (c) @CompanyNameMagicTag 2016-2023. All rights reserved.
 *
 * Description: Driver Entry Function Declaration.
 * Author: @CompanyNameTag
 */

/* 头文件包含 */
#define MPXX_LOG_MODULE_NAME     "[MP16C_BOARD]"
#define MPXX_LOG_MODULE_NAME_VAR mp16c_board_loglevel
#include "board_mp16c.h"

#include "chr_user.h"
#include "plat_debug.h"
#include "bfgx_dev.h"
#include "plat_pm.h"
#include "plat_uart.h"
#include "plat_firmware.h"
#include "plat_firmware_download.h"
#include "oam_dsm.h"
#include "bfgx_core.h"
#include "gpio_mp16c.h"

#ifdef BFGX_UART_DOWNLOAD_SUPPORT
#include "bfgx_data_parse.h"
#include "plat_firmware_uart.h"
#endif

#define TRY_MAX_COUNT 2

STATIC int32_t mp16c_board_w_power_on(void)
{
    if (bfgx_is_shutdown() && sle_is_shutdown()) {
        ps_print_info("wifi bootup pull up pwr_en\n");
        gugong_clk_power_on(W_SYS);
        board_chip_power_on();
    } else {
        gugong_check_subsys_power_on(W_SYS);
    }

    if (board_sys_enable(W_SYS)) {
        ps_print_info("wlan sys enable fail");
        return WIFI_POWER_BFGX_OFF_PULL_WLEN_FAIL;
    }

    if (mpxx_firmware_download_mode() == MODE_COMBO) {
        if (bfgx_is_shutdown() && sle_is_shutdown()) {
            board_sys_enable(B_SYS);
            board_sys_enable(G_SYS);
        }
    }

    return OAL_SUCC;
}

STATIC int32_t mp16c_board_b_power_on(void)
{
    if (wlan_is_shutdown() && sle_is_shutdown()) {
        ps_print_info("bi bootup pull up pwr_en!\n");
        gugong_clk_power_on(B_SYS);
        board_chip_power_on();
    } else {
        gugong_check_subsys_power_on(B_SYS);
    }
    board_sys_enable(B_SYS);
    return  SUCC;
}

STATIC int32_t mp16c_board_g_power_on(void)
{
    if (wlan_is_shutdown() && bfgx_is_shutdown()) {
        ps_print_info("gf bootup pull up pwr_en!\n");
        gugong_clk_power_on(G_SYS);
        board_chip_power_on();
    }
    board_sys_enable(G_SYS);
    return SUCC;
}

STATIC int32_t mp16c_board_power_on(uint32_t ul_subsystem)
{
    if (ul_subsystem == W_SYS) {
        return mp16c_board_w_power_on();
    } else if (ul_subsystem == B_SYS) {
        return mp16c_board_b_power_on();
    } else if (ul_subsystem == G_SYS) {
        return mp16c_board_g_power_on();
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return WIFI_POWER_FAIL;
    }
}

STATIC void mp16c_board_w_power_off(void)
{
    board_sys_disable(W_SYS);
    if (bfgx_is_shutdown() && sle_is_shutdown()) {
        ps_print_info("wifi close pull down pwr_en\n");
        board_chip_power_off();
        gugong_clk_power_off();
    }
}

STATIC void mp16c_board_b_power_off(void)
{
    board_sys_disable(B_SYS);
    if (wlan_is_shutdown() && sle_is_shutdown()) {
        ps_print_info("bi close pull down pwr_en\n");
        board_sys_disable(W_SYS);
        board_chip_power_off();
        gugong_clk_power_off();
    }
}

STATIC void mp16c_board_g_power_off(void)
{
    board_sys_disable(G_SYS);
    if (wlan_is_shutdown() && bfgx_is_shutdown()) {
        ps_print_info("gf close pull down pwr_en\n");
        board_sys_disable(W_SYS);
        board_chip_power_off();
        gugong_clk_power_off();
    } else {
        gugong_power_off_with_chip_on();
    }
}

STATIC int32_t mp16c_board_power_off(uint32_t ul_subsystem)
{
    if (ul_subsystem == W_SYS) {
        mp16c_board_w_power_off();
    } else if (ul_subsystem == B_SYS) {
        mp16c_board_b_power_off();
    } else if (ul_subsystem == G_SYS) {
        mp16c_board_g_power_off();
    } else {
        ps_print_err("power input system:%d error\n", ul_subsystem);
        return -EFAIL;
    }

    return SUCC;
}

STATIC int32_t mp16c_board_power_reset(uint32_t ul_subsystem)
{
    int32_t ret;
    board_sys_disable(B_SYS);
    board_sys_disable(G_SYS);
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    board_sys_disable(W_SYS);
#endif
    board_chip_power_off();
    board_clk_config_reset();
    board_chip_power_on();
#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    ret = board_sys_enable(W_SYS);
#endif
    ret = board_sys_enable(B_SYS);
    ret = board_sys_enable(G_SYS);
    return ret;
}

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
STATIC int32_t wlan_download_bfgx_frw(struct ps_core_s *ps_core_d, uint32_t cfg, uint32_t sys)
{
    int32_t ret;
    int32_t try_cnt = 1;
    struct pm_top* pm_top_data = pm_get_top();

    if (wlan_is_shutdown()) {
        for (;;try_cnt++) {
            if (board_sys_enable(W_SYS) == SUCC) {
                break;
            }
            if (try_cnt == TRY_MAX_COUNT) {
                return -EFAIL;
            }
            board_sys_disable(W_SYS);
        }
    }

    if (mpxx_firmware_download_mode() == MODE_COMBO &&
        oal_atomic_read(&ps_core_d->ir_only) == 0) {
        if (sle_is_shutdown() == true) {
            board_sys_enable(G_SYS);
        }
        if (bfgx_is_shutdown() == true) {
            board_sys_enable(B_SYS);
        }
    }

    ret = firmware_download_function(cfg, hcc_get_bus(HCC_EP_WIFI_DEV));
    if (ret != BFGX_POWER_SUCCESS) {
        hcc_bus_disable_state(pm_top_data->wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
        ps_print_err("bfgx download firmware fail!\n");
        return (ret == -OAL_EINTR) ? BFGX_POWER_DOWNLOAD_FIRMWARE_INTERRUPT : BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
    }

    if (mpxx_firmware_download_mode() == MODE_COMBO &&
        oal_atomic_read(&ps_core_d->ir_only) == 0) {
        board_sys_disable(B_SYS);
        board_sys_disable(G_SYS);
        oal_msleep(SLEEP_10_MSEC);
        board_sys_enable(sys);
    }

    if (wlan_is_shutdown()) {
        hcc_bus_disable_state(pm_top_data->wlan_pm_info->pst_bus, OAL_BUS_STATE_ALL);
        board_sys_disable(W_SYS);
    }

    return OAL_SUCC;
}

STATIC int32_t mp16c_bfgx_download_by_wifi(struct ps_core_s *ps_core_d, uint32_t sys)
{
    uint32_t whitch_cfg;
    int32_t download_mode;

    download_mode = mpxx_firmware_download_mode();

    if (oal_atomic_read(&ps_core_d->ir_only) == 1) {
        whitch_cfg = IR_CFG;
    } else {
        if (download_mode == MODE_COMBO) {
            // 混合加载时wifi上电已经加载完所有子系统FRW
            if (sle_is_shutdown() == false || bfgx_is_shutdown() == false ||
                wlan_is_shutdown() == false) {
                ps_print_info("board already power on");
                return BFGX_POWER_SUCCESS;
            }
            whitch_cfg = BFGX_CFG;
        } else if (download_mode == MODE_SEPARATE) {
            whitch_cfg = (sys == B_SYS) ? BT_CFG : SLE_CFG;
        } else {
            ps_print_err("download mode[%d] not recognize", download_mode);
            return BFGX_POWER_FAILED;
        }
    }

    if (wlan_download_bfgx_frw(ps_core_d, whitch_cfg, sys) != OAL_SUCC) {
        ps_print_info("wifi download bfgx frw fail");
        return BFGX_POWER_DOWNLOAD_FIRMWARE_FAIL;
    }
    return BFGX_POWER_SUCCESS;
}
#else
STATIC int32_t mp16c_bfgx_download_by_uart(struct ps_core_s *ps_core_d)
{
    int32_t error;

    // 在bootloader阶段，先使用2M 波特率协商
    pm_uart_set_baudrate(ps_core_d, UART_BAUD_RATE_2M);
    error = ps_core_d->pm_data->download_patch(ps_core_d);
    if (error) { /* if download patch err,and close uart */
        release_tty_drv(ps_core_d);
        ps_print_err(" download_patch is failed!\n");
        return error;
    }

    ps_print_suc("download_patch is successfully!\n");
    return error;
}
#endif

STATIC void mp16c_judge_ir_mode(struct ps_core_s *ps_core_d)
{
    int32_t download_mode;

    download_mode = mpxx_firmware_download_mode();
    if (download_mode == MODE_COMBO) {
        if (sle_is_shutdown() == true && bfgx_is_shutdown() == true &&
            wlan_is_shutdown() == true) {
            oal_atomic_set(&ps_core_d->ir_only, 1);
        }
    } else if (download_mode == MODE_SEPARATE) {
        if (bfgx_is_shutdown() == true) {
            oal_atomic_set(&ps_core_d->ir_only, 1);
        }
    }
}

STATIC int32_t mp16c_bfgx_dev_power_on(struct ps_core_s *ps_core_d, uint32_t sys, uint32_t subsys)
{
    int32_t ret;
    int32_t error = BFGX_POWER_SUCCESS;

    if ((ps_core_d == NULL) || (ps_core_d->pm_data == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if (subsys == BFGX_IR) {
        mp16c_judge_ir_mode(ps_core_d);
        ps_print_info("ir mode[%d]\n", oal_atomic_read(&ps_core_d->ir_only));
    }

    bfgx_gpio_intr_enable(ps_core_d->pm_data, OAL_TRUE);

    ret = mp16c_board_power_on(sys);
    if (ret) {
        ps_print_err("mp16c_board_power_on bfg failed, ret=%d\n", ret);
        error = BFGX_POWER_PULL_POWER_GPIO_FAIL;
        goto bfgx_power_on_fail;
    }

    if (open_tty_drv(ps_core_d) != BFGX_POWER_SUCCESS) {
        ps_print_err("open tty fail!\n");
        error = BFGX_POWER_TTY_OPEN_FAIL;
        goto bfgx_power_on_fail;
    }

#ifndef BFGX_UART_DOWNLOAD_SUPPORT
    error = mp16c_bfgx_download_by_wifi(ps_core_d, sys);
#else
    error = mp16c_bfgx_download_by_uart(ps_core_d);
#endif
    if (error != BFGX_POWER_SUCCESS) {
        goto bfgx_power_on_fail;
    }

    return BFGX_POWER_SUCCESS;

bfgx_power_on_fail:
#if defined(CONFIG_HUAWEI_OHOS_DSM) || defined(CONFIG_HUAWEI_DSM)
    if (error != BFGX_POWER_DOWNLOAD_FIRMWARE_INTERRUPT) {
        hw_mpxx_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_MPXX_DOWNLOAD_FIRMWARE,
                                  "bcpu download firmware failed,wifi %s,ret=%d,process:%s\n",
                                  wlan_is_shutdown() ? "off" : "on", error, current->comm);
    }
#endif
    (void)mp16c_board_power_off(sys);
    return error;
}

STATIC int32_t mp16c_bfgx_dev_power_off(struct ps_core_s *ps_core_d, uint32_t sys, uint32_t subsys)
{
    struct pm_drv_data *pm_data = NULL;

    ps_print_info("%s sys[%d] subsys[%d] \n", __func__, sys, subsys);
    if ((ps_core_d == NULL) || (ps_core_d->pm_data == NULL)) {
        ps_print_err("ps_core_d is err\n");
        return BFGX_POWER_FAILED;
    }

    if (uart_bfgx_close_cmd(ps_core_d) != SUCCESS) {
        /* bfgx self close fail 了，后面也要通过wifi shutdown bcpu */
        ps_print_err("bfgx self close fail\n");
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_CLOSE_BCPU);
    }

    pm_data = ps_core_d->pm_data;
    bfgx_gpio_intr_enable(pm_data, OAL_FALSE);

    if (wait_bfgx_memdump_complete(ps_core_d) != EXCEPTION_SUCCESS) {
        ps_print_err("wait memdump complete failed\n");
    }

    if (release_tty_drv(ps_core_d) != SUCCESS) {
        /* 代码执行到此处，说明六合一所有业务都已经关闭，无论tty是否关闭成功，device都要下电 */
        ps_print_err("wifi off, close tty is err!");
    }

    pm_data->bfgx_dev_state = BFGX_SLEEP;
    pm_data->uart_state = UART_NOT_READY;

    (void)mp16c_board_power_off(sys);

    if (subsys == BFGX_IR) {
        oal_atomic_set(&ps_core_d->ir_only, 0);
    }
    return BFGX_POWER_SUCCESS;
}

STATIC int32_t mp16c_wlan_power_off(void)
{
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();

    /* 先关闭SDIO TX通道 */
    hcc_bus_disable_state(hcc_get_bus(HCC_EP_WIFI_DEV), OAL_BUS_STATE_TX);

    /* wakeup dev,send poweroff cmd to wifi */
    if (wlan_pm_poweroff_cmd() != OAL_SUCC) {
        /* wifi self close 失败了也继续往下执行，uart关闭WCPU，异常恢复推迟到wifi下次open的时候执行 */
        declare_dft_trace_key_info("wlan_poweroff_cmd_fail", OAL_DFT_TRACE_FAIL);
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_CLOSE, CHR_PLAT_DRV_ERROR_CLOSE_WCPU);
    }
    hcc_bus_disable_state(hcc_get_bus(HCC_EP_WIFI_DEV), OAL_BUS_STATE_ALL);

    (void)mp16c_board_power_off(W_SYS);

    if (wlan_pm_info != NULL) {
        wlan_pm_info->wlan_power_state = POWER_STATE_SHUTDOWN;
    }
    return SUCCESS;
}

STATIC int32_t mp16c_wlan_firmware_download(void)
{
    int32_t ret;

    if ((mpxx_firmware_download_mode() == MODE_COMBO)) {
        if (bfgx_is_shutdown() && sle_is_shutdown()) {
            ret = firmware_download_function(BFGX_AND_WIFI_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
            board_sys_disable(B_SYS);
            board_sys_disable(G_SYS);
        }  else {
            ret = firmware_download_function(WIFI_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
        }
    } else {
        ret = firmware_download_function(WIFI_CFG, hcc_get_bus(HCC_EP_WIFI_DEV));
    }

    return ret;
}

STATIC int32_t mp16c_wlan_power_on(void)
{
    int32_t ret, error;
    int32_t try_cnt = 1;
    struct wlan_pm_s *wlan_pm_info = wlan_pm_get_drv();

    for (;;try_cnt++) {
        error = mp16c_board_power_on(W_SYS);
        if (error == SUCC) {
            break;
        }
        if (try_cnt == TRY_MAX_COUNT) {
            return error;
        }
        mp16c_board_power_off(W_SYS);
    }

    hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LOAD_PREPARE);

    error = mp16c_wlan_firmware_download();
    if (error != WIFI_POWER_SUCCESS) {
        ps_print_err("firmware download fail\n");
        if (error == -OAL_EINTR) {
            error = WIFI_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT;
        } else if (error == -OAL_ENOPERMI) {
            error = WIFI_POWER_ON_FIRMWARE_FILE_OPEN_FAIL;
        } else {
            error = WIFI_POWER_BFGX_OFF_FIRMWARE_DOWNLOAD_FAIL;
        }
        goto wifi_power_fail;
    } else {
        wlan_pm_info->wlan_power_state = POWER_STATE_OPEN;
    }

    ret = hcc_bus_power_action(hcc_get_bus(HCC_EP_WIFI_DEV), HCC_BUS_POWER_PATCH_LAUCH);
    if (ret != 0) {
        declare_dft_trace_key_info("wlan_poweron HCC_BUS_POWER_PATCH_LAUCH by gpio_fail", OAL_DFT_TRACE_FAIL);
        error = WIFI_POWER_BFGX_OFF_BOOT_UP_FAIL;
        chr_exception_report(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_PLAT, CHR_LAYER_DRV,
                             CHR_PLT_DRV_EVENT_OPEN, CHR_PLAT_DRV_ERROR_WCPU_BOOTUP);
        goto wifi_power_fail;
    }

    return WIFI_POWER_SUCCESS;
wifi_power_fail:
#if defined(CONFIG_HUAWEI_OHOS_DSM) || defined(CONFIG_HUAWEI_DSM)
    if (error != WIFI_POWER_ON_FIRMWARE_DOWNLOAD_INTERRUPT && error != WIFI_POWER_ON_FIRMWARE_FILE_OPEN_FAIL) {
        hw_mpxx_dsm_client_notify(SYSTEM_TYPE_PLATFORM, DSM_MPXX_DOWNLOAD_FIRMWARE,
                                  "%s: failed to download firmware, bfgx %s, error=%d\n",
                                  __FUNCTION__, bfgx_is_shutdown() ? "off" : "on", error);
    }
#endif
    return error;
}

STATIC int32_t mp16c_board_power_notify(uint32_t ul_subsystem, int32_t en)
{
    if ((en == OAL_TRUE) || (ul_subsystem != BFGX_GNSS)) {
        return SUCC;
    }

    if (ps_core_single_subsys_active(BFGX_GNSS) == OAL_TRUE) {
        ps_print_info("only gnss is active\n");
        return SUCC;
    }

    gugong_power_off_with_chip_on();

    return SUCC;
}

STATIC void board_callback_init_mp16c_power(void)
{
    g_st_board_info.bd_ops.bfgx_dev_power_on = mp16c_bfgx_dev_power_on;
    g_st_board_info.bd_ops.bfgx_dev_power_off = mp16c_bfgx_dev_power_off;
    g_st_board_info.bd_ops.wlan_power_off = mp16c_wlan_power_off;
    g_st_board_info.bd_ops.wlan_power_on = mp16c_wlan_power_on;
    g_st_board_info.bd_ops.board_power_on = mp16c_board_power_on;
    g_st_board_info.bd_ops.board_power_off = mp16c_board_power_off;
    g_st_board_info.bd_ops.board_power_reset = mp16c_board_power_reset;
    g_st_board_info.bd_ops.board_power_notify = mp16c_board_power_notify;
}

void board_info_init_mp16c(void)
{
    board_callback_init_dts();

    board_callback_init_mp16c_power();
}
