

#define HISI_LOG_TAG "[DFT]"
#include "oal_util.h"
#include "oal_net.h"

#include "oal_schedule.h"

#include "oal_kernel_file.h"
#include "securec.h"

char *g_hi11xx_loglevel_format[] = {
    KERN_ERR "[HI11XX]"
    "[E]",
    KERN_WARNING "[HI11XX]"
    "[W]",
    KERN_INFO "[HI11XX]"
    "[I]",
    KERN_INFO "[HI11XX]"
    "[D]",
    KERN_INFO "[HI11XX]"
    "[V]"
};
oal_module_symbol(g_hi11xx_loglevel_format);
oal_int32 g_hi11xx_loglevel = HI11XX_LOG_INFO;
oal_module_symbol(g_hi11xx_loglevel);
oal_debug_module_param(g_hi11xx_loglevel, int, S_IRUGO | S_IWUSR);

#ifdef _PRE_OAL_FEATURE_KEY_PROCESS_TRACE
oal_define_spinlock(g_dft_head_lock);
oal_list_create_head(g_dft_head);
oal_module_symbol(g_dft_head_lock);
oal_module_symbol(g_dft_head);

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
#define KERN_PRINT BIT0
#define BUFF_PRINT BIT1

OAL_STATIC oal_uint32 g_dft_print_mask = (KERN_PRINT | BUFF_PRINT);
oal_debug_module_param(g_dft_print_mask, uint, S_IRUGO | S_IWUSR);
OAL_STATIC oal_int32 oal_dft_dump_key_info(char *buf, oal_int32 key_type, oal_uint32 print_tag)
{
    oal_int32 ret;
    oal_ulong flags;
    oal_int32 count = 0;

    oal_int32 last_time_flag;

    oal_timespec_stru first_tv;
    oal_timespec_stru last_tv;
    struct rtc_time first_tm;
    struct rtc_time last_tm;

    oal_list_entry_stru *pst_pos = NULL;
    oal_dft_trace_item *pst_dft_item = NULL;

    oal_spin_lock_irq_save(&g_dft_head_lock, &flags);
    oal_list_search_for_each(pst_pos, &g_dft_head)
    {
        oal_spin_unlock_irq_restore(&g_dft_head_lock, &flags);
        last_time_flag = 0;
        pst_dft_item = oal_list_get_entry(pst_pos, oal_dft_trace_item, list);
        if (key_type != pst_dft_item->dft_type) {
            oal_spin_lock_irq_save(&g_dft_head_lock, &flags);
            continue;
        }

        memset_s(&first_tm, OAL_SIZEOF(struct rtc_time), 0, OAL_SIZEOF(struct rtc_time));
        memcpy_s(&first_tv, OAL_SIZEOF(first_tv), &pst_dft_item->first_timestamp, OAL_SIZEOF(first_tv));
        first_tv.tv_sec -= sys_tz.tz_minuteswest * OAL_SEC_PER_MIN;
        oal_rtc_ktime_to_tm(first_tv, &first_tm);

        memset_s(&last_tm, OAL_SIZEOF(struct rtc_time), 0, OAL_SIZEOF(struct rtc_time));
        memcpy_s(&last_tv, OAL_SIZEOF(last_tv), &pst_dft_item->last_timestamp, OAL_SIZEOF(last_tv));
        if (last_tv.tv_sec) {
            last_tv.tv_sec -= sys_tz.tz_minuteswest * OAL_SEC_PER_MIN;
            oal_rtc_ktime_to_tm(last_tv, &last_tm);
            last_time_flag = 1;
        }

        if (!last_time_flag) {
            if (print_tag & KERN_PRINT)
                printk(KERN_INFO "%s,1st at %02d:%02d:%02d:%02d:%02d,total:%u\n",
                       pst_dft_item->name,
                       first_tm.tm_mon + 1,
                       first_tm.tm_mday,
                       first_tm.tm_hour, first_tm.tm_min, first_tm.tm_sec,
                       pst_dft_item->trace_count);
            if (print_tag & BUFF_PRINT) {
                if (buf != NULL) {
                    ret = snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
                                     "%s,1st at %02d:%02d:%02d:%02d:%02d,total:%u\n",
                                     pst_dft_item->name,
                                     first_tm.tm_mon + 1,
                                     first_tm.tm_mday,
                                     first_tm.tm_hour, first_tm.tm_min, first_tm.tm_sec,
                                     pst_dft_item->trace_count);
                    if (ret < 0) {
                        OAL_IO_PRINT("log str format err line[%d]\n", __LINE__);
                        return -OAL_EFAIL;
                    }
                    count += ret;
                }
            }
        } else {
            if (print_tag & KERN_PRINT)
                printk(KERN_INFO "%s,1st at %02d:%02d:%02d:%02d:%02d,last at %02d:%02d:%02d:%02d:%02d,total:%u\n",
                       pst_dft_item->name,
                       first_tm.tm_mon + 1,
                       first_tm.tm_mday,
                       first_tm.tm_hour, first_tm.tm_min, first_tm.tm_sec,
                       last_tm.tm_mon + 1,
                       last_tm.tm_mday,
                       last_tm.tm_hour, last_tm.tm_min, last_tm.tm_sec,
                       pst_dft_item->trace_count);
            if (print_tag & BUFF_PRINT) {
                if (buf != NULL) {
                    ret = snprintf_s(buf + count, PAGE_SIZE - count, PAGE_SIZE - count - 1,
                                     "%s,1st at %02d:%02d:%02d:%02d:%02d,last at %02d:%02d:%02d:%02d:%02d,total:%u\n",
                                     pst_dft_item->name,
                                     first_tm.tm_mon + 1,
                                     first_tm.tm_mday,
                                     first_tm.tm_hour, first_tm.tm_min, first_tm.tm_sec,
                                     last_tm.tm_mon + 1,
                                     last_tm.tm_mday,
                                     last_tm.tm_hour, last_tm.tm_min, last_tm.tm_sec,
                                     pst_dft_item->trace_count);
                    if (ret < 0) {
                        OAL_IO_PRINT("log str format err line[%d]\n", __LINE__);
                        return -OAL_EFAIL;
                    }
                    count += ret;
                }
            }
        }

        oal_spin_lock_irq_save(&g_dft_head_lock, &flags);
    }
    oal_spin_unlock_irq_restore(&g_dft_head_lock, &flags);

    return count;
}

oal_void oal_dft_print_error_key_info(oal_void)
{
    oal_int32 i;
    printk(KERN_WARNING "[E]dump error key_info:\n");
    for (i = (OAL_DFT_TRACE_BUTT - 1); i >= OAL_DFT_TRACE_FAIL; i--) {
        oal_dft_dump_key_info(NULL, i, (KERN_PRINT));
    }
}
oal_module_symbol(oal_dft_print_error_key_info);

oal_void oal_dft_print_all_key_info(oal_void)
{
    oal_int32 i;
    for (i = 0; i < OAL_DFT_TRACE_BUTT; i++) {
        oal_dft_dump_key_info(NULL, i, (KERN_PRINT));
    }
}
oal_module_symbol(oal_dft_print_all_key_info);

OAL_STATIC ssize_t oal_dft_get_key_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    oal_int32 i;
    oal_int32 ret = 0;
    oal_int32 count = 0;
    oal_uint32 dft_mask;

    if (buf == NULL) {
        OAL_IO_PRINT("buf is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (attr == NULL) {
        OAL_IO_PRINT("attr is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (dev == NULL) {
        OAL_IO_PRINT("dev is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    dft_mask = g_dft_print_mask & (KERN_PRINT | BUFF_PRINT);
    g_dft_print_mask = dft_mask;

    if (oal_list_is_empty(&g_dft_head)) {
        printk(KERN_INFO "key info empty\n");
        ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "key info empty\n");
        if (ret < 0) {
            OAL_IO_PRINT("log str format err line[%d]\n", __LINE__);
        }
        return ret;
    }

    if (OAL_DFT_TRACE_BUTT == 0) {
        return ret;
    }

    for (i = (OAL_DFT_TRACE_BUTT - 1); i >= 0; i--) {
        ret = oal_dft_dump_key_info(buf + count, i, dft_mask);
        if (ret < 0) {
            OAL_IO_PRINT("oal_dft_dump_key_info::info str format err.\n");
            return count;
        }
        count += ret;
    }

    return count;
}
OAL_STATIC struct kobj_attribute g_dev_attr_key_info =
    __ATTR(key_info, S_IRUGO, oal_dft_get_key_info, NULL);

#ifdef HI110X_DRV_VERSION
OAL_STATIC oal_int32 oal_dft_print_version_info(char *buf)
{
    oal_int32 ret;
    ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%s\n", HI110X_DRV_VERSION);
    if (ret < 0) {
        OAL_IO_PRINT("log str format err line[%d]\n", __LINE__);
    }
    return ret;
}

OAL_STATIC ssize_t oal_dft_get_version_info(struct kobject *dev, struct kobj_attribute *attr, char *buf)
{
    oal_int32 ret = 0;

    if (buf == NULL) {
        OAL_IO_PRINT("buf is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (attr == NULL) {
        OAL_IO_PRINT("attr is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    if (dev == NULL) {
        OAL_IO_PRINT("dev is null r failed!%s\n", __FUNCTION__);
        return 0;
    }

    ret += oal_dft_print_version_info(buf + ret);

    return ret;
}
OAL_STATIC struct kobj_attribute g_dev_attr_version =
    __ATTR(version, S_IRUGO, oal_dft_get_version_info, NULL);
#endif

OAL_STATIC struct attribute *g_dft_sysfs_entries[] = {
    &g_dev_attr_key_info.attr,
#ifdef HI110X_DRV_VERSION
    &g_dev_attr_version.attr,
#endif
    NULL
};

OAL_STATIC struct attribute_group g_dft_attribute_group = {
    .name = "dft",
    .attrs = g_dft_sysfs_entries,
};

OAL_STATIC oal_int32 dft_sysfs_entry_init(oal_void)
{
    oal_int32 ret;
    oal_kobject *pst_root_object = NULL;
    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object == NULL) {
        OAL_IO_PRINT("{frw_sysfs_entry_init::get sysfs root object failed!}\n");
        return -OAL_EFAIL;
    }

    ret = sysfs_create_group(pst_root_object, &g_dft_attribute_group);
    if (ret) {
        OAL_IO_PRINT("{frw_sysfs_entry_init::sysfs create group failed!}\n");
        return ret;
    }
    return OAL_SUCC;
}

OAL_STATIC oal_void dft_sysfs_entry_exit(oal_void)
{
    oal_kobject *pst_root_object = NULL;
    pst_root_object = oal_get_sysfs_root_object();
    if (pst_root_object != NULL) {
        sysfs_remove_group(pst_root_object, &g_dft_attribute_group);
    }
}
#endif

oal_int32 oal_dft_init(oal_void)
{
    oal_int32 ret = OAL_SUCC;
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    ret = dft_sysfs_entry_init();
#endif
    return ret;
}

oal_void oal_dft_exit(oal_void)
{
#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
    dft_sysfs_entry_exit();
#endif
}

oal_module_symbol(oal_dft_init);
oal_module_symbol(oal_dft_exit);
#endif

