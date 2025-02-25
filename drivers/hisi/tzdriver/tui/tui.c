/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2012-2022. All rights reserved.
 * Description: tui agent for tui display and interact
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */
#include "tui.h"
#include <linux/kthread.h>
#include <linux/sched.h>
#if (KERNEL_VERSION(5, 10, 0) <= LINUX_VERSION_CODE)
#include <linux/sched/types.h>
#include <uapi/linux/sched/types.h>
#else
#include <linux/sched/types.h>
#endif
#include <linux/sched/rt.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/spinlock.h>
#include <linux/debugfs.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/atomic.h>
#include <linux/time.h>
#include <linux/timer.h>
#include <linux/wait.h>
#include <linux/version.h>
#ifndef CONFIG_DMABUF_MM
#include <linux/ion.h>
#endif
#include <linux/cma.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/workqueue.h>
#include <linux/sysfs.h>
#ifdef CONFIG_TEE_TUI_MTK
#include <linux/sched/task.h>
#include <linux/uaccess.h>
#include <linux/scatterlist.h>
#endif
/* add for CMA malloc framebuffer */
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/fs.h>
#include <linux/mm.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>
#include <linux/kmemleak.h>
#include <securec.h>
#include "teek_client_constants.h"
#include "agent.h"
#include "mem.h"
#include "teek_ns_client.h"
#include "smc_smp.h"
#include "tc_ns_client.h"
#include "tc_ns_log.h"
#include "mailbox_mempool.h"
#ifndef CONFIG_TEE_TUI_MTK
#include <platform_include/basicplatform/linux/powerkey_event.h>
#ifdef CONFIG_DMABUF_MM
#include <linux/dmabuf/mm_dma_heap.h>
#else
#include <linux/ion/mm_ion.h>
#endif
#ifdef CONFIG_TEE_TUI_DISPLAY_3_0
#include "dpu_comp_mgr.h"
#else
#include <hisi_fb.h>
#endif
#endif
#include "dynamic_ion_mem.h"
#ifdef CONFIG_TEE_TUI_MTK
#include "teek_client_type.h"
#include "teek_client_api.h"
#include <memory_ssmr.h>
#include <linux/dma-mapping.h>
#ifdef CONFIG_HW_SECMEM
#include "secmem_api.h"
#endif
#ifdef CONFIG_ITRUSTEE_TRUSTED_UI
#include <mtk_debug.h>
#endif

#ifdef CONFIG_HW_COMB_KEY
#include <huawei_platform/comb_key/power_key_event.h>
#endif

#ifndef CONFIG_ITRUSTEE_TRUSTED_UI
#include <lcd_kit_utils.h>
struct mtk_fb_data_type {
	bool panel_power_on;
	struct mtk_panel_info panel_info;
};
#endif
#endif
#include "internal_functions.h"

static void tui_poweroff_work_func(struct work_struct *work);
static ssize_t tui_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static void tui_msg_del(const char *name);
static DECLARE_DELAYED_WORK(tui_poweroff_work, tui_poweroff_work_func);
static struct kobject *g_tui_kobj          = NULL;
static struct kobj_attribute tui_attribute =
	__ATTR(c_state, 0440, tui_status_show, NULL);
static struct attribute *attrs[] = {
	&tui_attribute.attr,
	NULL,
};

static struct attribute_group g_tui_attr_group = {
	.attrs = attrs,
};

DEFINE_MUTEX(g_tui_drv_lock);
static struct task_struct *g_tui_task  = NULL;
static struct tui_ctl_shm *g_tui_ctl   = NULL;
static atomic_t g_tui_usage            = ATOMIC_INIT(0);
static atomic_t g_tui_state            = ATOMIC_INIT(TUI_STATE_UNUSED);
static struct list_head g_tui_drv_head = LIST_HEAD_INIT(g_tui_drv_head);
static atomic_t g_tui_attached_device  = ATOMIC_INIT(TUI_PID_CLEAR);
static atomic_t g_tui_pid              = ATOMIC_INIT(TUI_PID_CLEAR);
static bool g_normal_load_flag         = false;

static spinlock_t g_tui_msg_lock;
static struct list_head g_tui_msg_head;
static wait_queue_head_t g_tui_state_wq;
static int g_tui_state_flag;
static wait_queue_head_t g_tui_msg_wq;
static int32_t g_tui_msg_flag;
#ifdef CONFIG_TEE_TUI_MTK
static struct mtk_fb_data_type *g_dss_fd;
#elif defined CONFIG_TEE_TUI_DISPLAY_3_0
static struct dpu_composer *g_dss_fd;
#else
static struct hisi_fb_data_type *g_dss_fd;
#endif
#define TUI_DSS_NAME         "DSS"
#define TUI_GPIO_NAME        "fff0d000.gpio"
#define TUI_TP_NAME          "tp"
#define TUI_FP_NAME          "fp"

/* EMUI 11.1 need use the ttf of HarmonyOSHans.ttf */
#define TTF_NORMAL_BUFF_SIZE (20 * 1024 * 1024)

#ifdef TUI_DAEMON_UID_IN_OH
#define TTF_NORMAL_FILE_PATH "/system/fonts/HarmonyOS_Sans_SC_Regular.ttf"
#else
#define TTF_NORMAL_FILE_PATH "/system/fonts/HarmonyOS_Sans_SC.ttf"
#endif

/* 2M memory size is 2^21 */
#define ALIGN_SIZE              21
#define ALIGN_M                 (1 << 21)
#define MAX_SCREEN_RESOLUTION   8192
#define TP_BASE_VALUE           10

/* dss and tp couple mode: 0 is init dss and tp; 1 is only init dss; 2 is only init tp */
#define DSS_TP_COUPLE_MODE      0
#define NORMAL_MODE             0 /* init all driver */
#define ONLY_INIT_DSS           1 /* only init dss */
#define ONLY_INIT_TP            2 /* only init tp */

/*
 * do fp init(disable fp irq) before gpio init in order not response
 * sensor in normal world(when gpio secure status is set)
 */
#if ONLY_INIT_DSS == DSS_TP_COUPLE_MODE
#define DRIVER_NUM 1
static char *g_init_driver[DRIVER_NUM]   = {TUI_DSS_NAME};
static char *g_deinit_driver[DRIVER_NUM] = {TUI_DSS_NAME};
#endif

#if ONLY_INIT_TP == DSS_TP_COUPLE_MODE
#define DRIVER_NUM 3
static char *g_init_driver[DRIVER_NUM]   = {TUI_TP_NAME, TUI_FP_NAME, TUI_GPIO_NAME};
static char *g_deinit_driver[DRIVER_NUM] = {TUI_TP_NAME, TUI_FP_NAME, TUI_GPIO_NAME};
#endif

#if NORMAL_MODE == DSS_TP_COUPLE_MODE
#define DRIVER_NUM 4
static char *g_init_driver[DRIVER_NUM]   = {TUI_DSS_NAME, TUI_TP_NAME, TUI_FP_NAME, TUI_GPIO_NAME};
static char *g_deinit_driver[DRIVER_NUM] = {TUI_DSS_NAME, TUI_TP_NAME, TUI_FP_NAME, TUI_GPIO_NAME};
#endif

#define TIME_OUT_FOWER_ON    100
#define DOWN_VAL             22 /* 4M */
#define UP_VAL               27 /* 64M */
#define COLOR_TYPE           4 /* ARGB */
#define BUFFER_NUM           2
#define UID_MAX_VAL          1000
#define HIGH_VALUES          32
#define ION_NENTS_FLAG       1
#define INVALID_CFG_NAME     (-2)

static tui_ion_mem g_tui_display_mem;
static tui_ion_mem g_normal_font_mem;

unsigned int get_frame_size(unsigned int num)
{
	if (num % ALIGN_M != 0)
		return (((num >> ALIGN_SIZE) + 1) << ALIGN_SIZE);
	else
		return num;
}

unsigned int get_tui_size(unsigned int num)
{
	unsigned int i;
	for (i = DOWN_VAL; i < UP_VAL; i++)
		if ((num >> i) == 0)
			break;
	return (unsigned int)1 << i;
}

/*
 * alloc order: 4M-font, framebuffer, 20M-unusualfont
 * 1.4M alloc when boot so from ION_TUI_HEAP_ID
 * 2.20M and frambuffer alloc when tui init so from ION_MISC_HEAP_ID
 */
static size_t get_tui_font_file_size(void)
{
	int ret;
	struct kstat ttf_file_stat;
	mm_segment_t old_fs;

	old_fs = get_fs();
	set_fs(KERNEL_DS);
	/* get ttf file size */
	ret = vfs_stat(TTF_NORMAL_FILE_PATH, &ttf_file_stat);
	if (ret < 0) {
		tloge("Failed to get ttf extend file size, ret is %d\n", ret);
		set_fs(old_fs);
		return 0;
	}
	set_fs(old_fs);

	return ttf_file_stat.size;
}

static int check_ion_sg_table(const struct sg_table *sg_table)
{
	if (sg_table == NULL) {
		tloge("invalid sgtable\n");
		return -1;
	}

	/* nent must be 1, because ion addr for tui is continuous */
	if (sg_table->nents != ION_NENTS_FLAG) {
		tloge("nent is invalid\n");
		return -1;
	}
	return 0;
}
static int get_tui_ion_sglist(tui_ion_mem *tui_mem)
{
	struct sglist *tmp_tui_sglist = NULL;
	struct scatterlist *tui_sg = NULL;
	struct page *tui_page = NULL;
	uint32_t tui_sglist_size;
	uint32_t i = 0;

	struct sg_table *tui_ion_sg_table = tui_mem->tui_sg_table;
	if (check_ion_sg_table(tui_ion_sg_table) != 0)
		return -1;

	tui_sglist_size = sizeof(struct ion_page_info) * tui_ion_sg_table->nents +
		sizeof(struct sglist);
	tmp_tui_sglist = (struct sglist *)mailbox_alloc(tui_sglist_size, MB_FLAG_ZERO);
	if (tmp_tui_sglist == NULL) {
		tloge("in %s err: mailbox_alloc failed\n", __func__);
		return -1;
	}

	tmp_tui_sglist->sglist_size = (uint64_t)tui_sglist_size;
	tmp_tui_sglist->ion_size = (uint64_t)tui_mem->len;
	tmp_tui_sglist->info_length = (uint64_t)tui_ion_sg_table->nents;
	tui_mem->info_length = (uint64_t)tui_ion_sg_table->nents;

	/* get tui_sg to fetch ion for tui */
	for_each_sg(tui_ion_sg_table->sgl, tui_sg, tui_ion_sg_table->nents, i) {
		if (tui_sg == NULL) {
			tloge("tui sg is NULL");
			mailbox_free(tmp_tui_sglist);
			return -1;
		}
		tui_page = sg_page(tui_sg);
		tmp_tui_sglist->page_info[0].phys_addr = page_to_phys(tui_page);
		tmp_tui_sglist->page_info[0].npages = tui_sg->length / PAGE_SIZE;
		tui_mem->npages = tmp_tui_sglist->page_info[0].npages;
		tui_mem->tui_ion_virt_addr = phys_to_virt(tmp_tui_sglist->page_info[0].phys_addr);
		tui_mem->fb_phys_addr = tmp_tui_sglist->page_info[0].phys_addr;
	}

	tui_mem->tui_ion_phys_addr = mailbox_virt_to_phys((uintptr_t)(void *)tmp_tui_sglist); // sglist phys-addr
	if (tui_mem->tui_ion_phys_addr == 0) {
		tloge("Failed to get tmp_tui_sglist physaddr, configid=%d\n",
		tui_mem->configid);
		mailbox_free(tmp_tui_sglist);
		return -1;
	}
	tui_mem->size = tui_sglist_size;
	return 0;
}
static int alloc_ion_mem(tui_ion_mem *tui_mem)
{
	struct sg_table *tui_ion_sg_table = NULL;
	if (tui_mem == NULL) {
		tloge("bad input params\n");
		return -1;
	}
#ifdef CONFIG_HW_SECMEM
	tui_ion_sg_table = cma_secmem_alloc(SEC_TUI, tui_mem->len);
#endif
#ifndef CONFIG_TEE_TUI_MTK
	tui_ion_sg_table = mm_secmem_alloc(SEC_TUI, tui_mem->len);
#endif
	if (tui_ion_sg_table == NULL) {
		tloge("failed to get ion page for tui,configid is %d\n", tui_mem->configid);
		return -1;
	}
	tui_mem->tui_sg_table = tui_ion_sg_table;
	return 0;
}

static void free_ion_mem(tui_ion_mem *tui_mem)
{
	if (tui_mem->tui_sg_table == NULL || tui_mem->tui_ion_phys_addr == 0) {
		tloge("bad input params\n");
		return;
	}
#ifdef CONFIG_HW_SECMEM
	cma_secmem_free(SEC_TUI, tui_mem->tui_sg_table);
#endif
#ifndef CONFIG_TEE_TUI_MTK
	mm_secmem_free(SEC_TUI, tui_mem->tui_sg_table);
#endif
	tui_mem->tui_ion_phys_addr = 0;
	return;
}

static void free_tui_font_mem(void)
{
	free_ion_mem(&g_normal_font_mem);
	g_normal_load_flag = false;
	tloge("normal tui font file freed\n");
}

static int get_tui_font_mem(tui_ion_mem *tui_font_mem)
{
	int ret;

	ret = alloc_ion_mem(tui_font_mem);
	if (ret < 0) {
		tloge("Failed to alloc cma mem for tui font lib\n");
		return -ENOMEM;
	}

	return 0;
}

/* size is calculated dynamically according to the screen resolution */
#ifdef CONFIG_TEE_TUI_DISPLAY_3_0
static phys_addr_t get_frame_addr(void)
{
	int screen_r;
	int ret;
	bool check_params = false;
	if(g_dss_fd == NULL)
		return 0;

	check_params = (g_dss_fd->comp.base.xres > MAX_SCREEN_RESOLUTION) ||
		(g_dss_fd->comp.base.yres > MAX_SCREEN_RESOLUTION);
	if (check_params) {
		tloge("Horizontal resolution or Vertical resolution is too large\n");
		return 0;
	}
	screen_r = g_dss_fd->comp.base.xres * g_dss_fd->comp.base.yres * COLOR_TYPE * BUFFER_NUM;
	g_tui_display_mem.len = get_frame_size(screen_r);
	ret = alloc_ion_mem(&g_tui_display_mem);
	if (ret) {
		tloge("Failed to alloc mem for tui display\n");
		return 0;
	}

	if (get_tui_ion_sglist(&g_tui_display_mem)) {
		tloge("get sglist failed\n");
		free_ion_mem(&g_tui_display_mem);
		return 0;
	}

	return g_tui_display_mem.fb_phys_addr;
}
#else
static phys_addr_t get_frame_addr(void)
{
	int screen_r;
	int ret;
	bool check_params = false;
	if(g_dss_fd == NULL)
		return 0;

	check_params = (g_dss_fd->panel_info.xres > MAX_SCREEN_RESOLUTION) ||
		(g_dss_fd->panel_info.yres > MAX_SCREEN_RESOLUTION);
	if (check_params) {
		tloge("Horizontal resolution or Vertical resolution is too large\n");
		return 0;
	}
	screen_r = g_dss_fd->panel_info.xres * g_dss_fd->panel_info.yres * COLOR_TYPE * BUFFER_NUM;
	g_tui_display_mem.len = get_frame_size(screen_r);
	ret = alloc_ion_mem(&g_tui_display_mem);
	if (ret != 0) {
		tloge("Failed to alloc mem for tui display\n");
		return 0;
	}

	if (get_tui_ion_sglist(&g_tui_display_mem) != 0) {
		tloge("get sglist failed\n");
		free_ion_mem(&g_tui_display_mem);
		return 0;
	}

	return g_tui_display_mem.fb_phys_addr;
}
#endif

void free_frame_addr(void)
{
	mailbox_free(phys_to_virt(g_tui_display_mem.tui_ion_phys_addr));
	free_ion_mem(&g_tui_display_mem);
	return;
}

static int32_t tc_ns_register_tui_font_mem(tui_ion_mem *tui_font_mem,
	size_t font_file_size)
{
	struct tc_ns_smc_cmd smc_cmd = { {0}, 0 };
	int ret = 0;
	struct mb_cmd_pack *mb_pack = NULL;

	mb_pack = mailbox_alloc_cmd_pack();
	if (!mb_pack) {
		tloge("alloc cmd pack failed\n");
		return -ENOMEM;
	}

	smc_cmd.cmd_type = CMD_TYPE_GLOBAL;
	smc_cmd.cmd_id = GLOBAL_CMD_ID_REGISTER_TTF_MEM;

	mb_pack->operation.paramtypes = teec_param_types(
		TEEC_MEMREF_TEMP_INPUT,
		TEEC_VALUE_INPUT,
		TEEC_NONE,
		TEEC_NONE
	);

	mb_pack->operation.params[0].memref.size = (uint32_t)(tui_font_mem->size);
	mb_pack->operation.params[0].memref.buffer = (uint32_t)(tui_font_mem->tui_ion_phys_addr & 0xFFFFFFFF);
	mb_pack->operation.buffer_h_addr[0] = tui_font_mem->tui_ion_phys_addr >> HIGH_VALUES;
	mb_pack->operation.params[1].value.a = font_file_size;

	smc_cmd.operation_phys = (unsigned int)mailbox_virt_to_phys((uintptr_t)&mb_pack->operation);
	smc_cmd.operation_h_phys = mailbox_virt_to_phys((uintptr_t)&mb_pack->operation) >> HIGH_VALUES;
	if (tc_ns_smc(&smc_cmd)) {
		ret = -EPERM;
		tloge("send ttf mem info failed. ret = 0x%x\n", smc_cmd.ret_val);
	}
	mailbox_free(mb_pack);

	return ret;
}

static int32_t copy_tui_font_file(size_t font_file_size, const void *font_virt_addr)
{
	struct file *filep = NULL;
	mm_segment_t old_fs;
	loff_t pos = 0;
	unsigned int count;
	int ret = 0;

	if (font_virt_addr == NULL)
		return -1;

	filep = filp_open(TTF_NORMAL_FILE_PATH, O_RDONLY, 0);
	if (IS_ERR(filep) || filep == NULL) {
		tloge("Failed to open ttf file\n");
		return -1;
	}

	old_fs = get_fs();
	set_fs(KERNEL_DS);

	count = (unsigned int)vfs_read(filep, (char __user *)font_virt_addr, font_file_size, &pos);

	if (font_file_size != count) {
		tloge("read ttf file failed\n");
		ret = -1;
	}

	set_fs(old_fs);
	filp_close(filep, 0);
	filep = NULL;
	return ret;
}

static int32_t send_ttf_mem(tui_ion_mem *tui_ttf_mem)
{
	int ret;
	size_t tui_font_file_size;
	bool check_params = false;

	tui_font_file_size = get_tui_font_file_size();
	check_params = (tui_font_file_size == 0) || (tui_font_file_size > tui_ttf_mem->len);
	if (check_params) {
		tloge("Failed to get the tui font file size or the tui_font_file_size is too big\n");
		return -1;
	}

	ret = copy_tui_font_file(tui_font_file_size, tui_ttf_mem->tui_ion_virt_addr);
	if (ret < 0) {
		tloge("Failed to do ttf file copy\n");
		return -1;
	}

	__dma_map_area(tui_ttf_mem->tui_ion_virt_addr, tui_ttf_mem->len, DMA_BIDIRECTIONAL);
	__dma_map_area(tui_ttf_mem->tui_ion_virt_addr, tui_ttf_mem->len, DMA_FROM_DEVICE);

	ret = tc_ns_register_tui_font_mem(tui_ttf_mem, tui_font_file_size);
	if (ret != 0) {
		tloge("Failed to do ttf file register ret is 0x%x\n", ret);
		return -1;
	}

	return 0;
}

static int32_t load_tui_font_file(void)
{
	int ret = 0;
	tui_ion_mem *tui_ttf_mem = NULL;

	tloge("====load ttf start =====\n");

	mutex_lock(&g_tui_drv_lock);
	if (g_normal_load_flag) {
		tloge("normal tui font file has been loaded\n");
		mutex_unlock(&g_tui_drv_lock);
		return 0;
	}

	g_normal_font_mem.len = TTF_NORMAL_BUFF_SIZE;
	ret = get_tui_font_mem(&g_normal_font_mem);
	tui_ttf_mem = &g_normal_font_mem;
	if (ret != 0) {
		tloge("Failed to get tui font memory\n");
		mutex_unlock(&g_tui_drv_lock);
		return -1;
	}

	if (get_tui_ion_sglist(tui_ttf_mem) != 0) {
		tloge("get tui sglist failed\n");
		free_tui_font_mem();
		mutex_unlock(&g_tui_drv_lock);
		return -1;
	}

	ret = send_ttf_mem(tui_ttf_mem);
	if (ret != 0) {
		mailbox_free(phys_to_virt(tui_ttf_mem->tui_ion_phys_addr));
		free_tui_font_mem();
		mutex_unlock(&g_tui_drv_lock);
		return -1;
	}

	tloge("normal tui font file loaded\n");
	g_normal_load_flag = true;
	mutex_unlock(&g_tui_drv_lock);

	mailbox_free(phys_to_virt(tui_ttf_mem->tui_ion_phys_addr));
	tloge("=====load ttf end=====\n");
	return ret;
}

int register_tui_driver(tui_drv_init fun, const char *name,
	void *pdata)
{
	struct tui_drv_node *tui_drv = NULL;
	struct tui_drv_node *pos = NULL;

	/* Return error if name is invalid */
	if (name == NULL || fun == NULL) {
		tloge("name or func is null");
		return -EINVAL;
	}

	if (strncmp(name, TUI_DSS_NAME, (size_t)TUI_DRV_NAME_MAX) == 0) {
		if (pdata == NULL)
			return -1;
		else
#ifdef CONFIG_TEE_TUI_MTK
			g_dss_fd = (struct mtk_fb_data_type *)pdata;
#elif defined CONFIG_TEE_TUI_DISPLAY_3_0
			g_dss_fd = (struct dpu_composer *)pdata;
#else
			g_dss_fd = (struct hisi_fb_data_type *)pdata;
#endif
	}

	if ((strncmp(name, TUI_TP_NAME, (size_t)TUI_DRV_NAME_MAX) == 0) && pdata == NULL)
		return -1;

	mutex_lock(&g_tui_drv_lock);

	/* name should not have been registered */
	list_for_each_entry(pos, &g_tui_drv_head, list) {
		if (!strncmp(pos->name, name, TUI_DRV_NAME_MAX - 1)) {
			tloge("this drv(%s) have registered\n", name);
			mutex_unlock(&g_tui_drv_lock);
			return -EINVAL;
		}
	}
	mutex_unlock(&g_tui_drv_lock);

	/* Allocate memory for tui_drv */
	tui_drv = kzalloc(sizeof(struct tui_drv_node), GFP_KERNEL);
	if (tui_drv == NULL)
		return -ENOMEM;

	if (memset_s(tui_drv, sizeof(struct tui_drv_node), 0, sizeof(struct tui_drv_node)) != 0) {
		tloge("tui_drv memset failed");
		kfree(tui_drv);
		return -1;
	}
	/* Assign content for tui_drv */
	tui_drv->init_func = fun;
	tui_drv->pdata = pdata;

	if (strncpy_s(tui_drv->name, TUI_DRV_NAME_MAX, name, TUI_DRV_NAME_MAX - 1) != 0) {
		tloge("strncpy_s error\n");
		kfree(tui_drv);
		return -1;
	}

	INIT_LIST_HEAD(&tui_drv->list);

	/* Link the new tui_drv to the list */
	mutex_lock(&g_tui_drv_lock);
	list_add_tail(&tui_drv->list, &g_tui_drv_head);
	mutex_unlock(&g_tui_drv_lock);

	return 0;
}
EXPORT_SYMBOL(register_tui_driver);

void unregister_tui_driver(const char *name)
{
	struct tui_drv_node *pos = NULL, *tmp = NULL;

	/* Return error if name is invalid */
	if (name == NULL) {
		tloge("name is null");
		return;
	}

	mutex_lock(&g_tui_drv_lock);
	list_for_each_entry_safe(pos, tmp, &g_tui_drv_head, list) {
		if (!strncmp(pos->name, name, TUI_DRV_NAME_MAX)) {
			list_del(&pos->list);
			kfree(pos);
			break;
		}
	}
	mutex_unlock(&g_tui_drv_lock);
}
EXPORT_SYMBOL(unregister_tui_driver);

static int add_tui_msg(int type, int val, void *data)
{
	struct tui_msg_node *tui_msg = NULL;
	unsigned long flags;

	/* Return error if pdata is invalid */
	if (data == NULL) {
		tloge("data is null");
		return -EINVAL;
	}

	/* Allocate memory for tui_msg */
	tui_msg = kzalloc(sizeof(*tui_msg), GFP_KERNEL);
	if (tui_msg == NULL)
		return -ENOMEM;

	if (memset_s(tui_msg, sizeof(*tui_msg), 0, sizeof(*tui_msg)) != 0) {
		tloge("tui_msg memset failed");
		kfree(tui_msg);
		return -1;
	}

	/* Assign the content of tui_msg */
	tui_msg->type = type;
	tui_msg->val = val;
	tui_msg->data = data;
	INIT_LIST_HEAD(&tui_msg->list);

	/* Link the new tui_msg to the list */
	spin_lock_irqsave(&g_tui_msg_lock, flags);
	list_add_tail(&tui_msg->list, &g_tui_msg_head);
	g_tui_msg_flag = 1;
	spin_unlock_irqrestore(&g_tui_msg_lock, flags);
	return 0;
}

static int32_t init_each_tui_driver(struct tui_drv_node *pos, int32_t secure)
{
	if (secure == 0) {
		tlogi("drv(%s) state=%d,%d\n", pos->name, secure, pos->state);
		if (pos->state == 0)
			return 0;
		if (pos->init_func(pos->pdata, secure) != 0)
			pos->state = -1; /* Process init_func() fail */

		/* set secure state will be proceed in tui msg */
		pos->state = 0;
	} else {
		tlogi("init tui drv(%s) state=%d\n", pos->name, secure);
		/* when init, tp and dss should be async */
		if (pos->init_func(pos->pdata, secure) != 0) {
			pos->state = -1;
			return -1;
		} else {
#ifndef CONFIG_TEE_TUI_MTK
			if (strncmp(TUI_DSS_NAME, pos->name, TUI_DRV_NAME_MAX) != 0)
#endif
				pos->state = 1;
		}
	}
	return 0;
}

enum tui_driver_env {
	UNSECURE_ENV = 0,
	SECURE_ENV = 1,
};

#define WAIT_POWER_ON_SLEEP_SPAN 20
static int init_tui_dss_msg(const struct tui_drv_node *pos, int secure, int *count)
{
	if ((strncmp(TUI_DSS_NAME, pos->name, TUI_DRV_NAME_MAX) == 0) && (secure != 0)) {
		tloge("init_tui_driver wait power on status---\n");
#ifdef CONFIG_TEE_TUI_DISPLAY_3_0
		while (!g_dss_fd->comp.power_on && (*count) < TIME_OUT_FOWER_ON) {
#else
		while (!g_dss_fd->panel_power_on && (*count) < TIME_OUT_FOWER_ON) {
#endif
			(*count)++;
			msleep(WAIT_POWER_ON_SLEEP_SPAN);
		}
		if ((*count) == TIME_OUT_FOWER_ON) {
			/* Time out. So return error. */
			tloge("wait status time out\n");
			return -1;
		}
		spin_lock(&g_tui_msg_lock);
		tui_msg_del(TUI_DSS_NAME);
		spin_unlock(&g_tui_msg_lock);
	}
	return 0;
}

static bool is_dss_registered(void) {
	struct tui_drv_node *pos = NULL;
#if ONLY_INIT_TP == DSS_TP_COUPLE_MODE
	return true;
#endif
	list_for_each_entry(pos, &g_tui_drv_head, list) {
		if (strncmp(TUI_DSS_NAME, pos->name, TUI_DRV_NAME_MAX) == 0)
			return true;
	}
	return false;
}

/* WARNING: Too many leading tabs - consider code refactoring */
/* secure : 0-unsecure, 1-secure */
static int init_tui_driver(int secure)
{
	struct tui_drv_node *pos = NULL;
	char *drv_name = NULL;
	char **drv_array = g_deinit_driver;
	int count = 0;
	int i = 0;
	int ret = 0;
	if (g_dss_fd == NULL)
		return -1;

	if (secure != 0)
		drv_array = g_init_driver;

	while (i < DRIVER_NUM) {
		drv_name = drv_array[i];
		i++;
		mutex_lock(&g_tui_drv_lock);

		if (!is_dss_registered()) {
			tloge("dss not registered\n");
			mutex_unlock(&g_tui_drv_lock);
			return -1;
		}

		/* Search all the tui_drv in their list */
		list_for_each_entry(pos, &g_tui_drv_head, list) {
			if (strncmp(drv_name, pos->name, TUI_DRV_NAME_MAX) != 0)
				continue;

			if (!strncmp(TUI_TP_NAME, pos->name, TUI_DRV_NAME_MAX)) {
				/* If the name is "tp", assign pos->pdata to g_tui_ctl */
				g_tui_ctl->n2s.tp_info = (int)virt_to_phys(pos->pdata);
				g_tui_ctl->n2s.tp_info_h_addr = virt_to_phys(pos->pdata) >> HIGH_VALUES;
			}
			if (pos->init_func == 0)
				continue;

			ret = init_tui_dss_msg(pos, secure, &count);
			if (ret != 0) {
				mutex_unlock(&g_tui_drv_lock);
				return ret;
			}

			if (init_each_tui_driver(pos, secure) != 0) {
				mutex_unlock(&g_tui_drv_lock);
				return -1;
			}
		}
		mutex_unlock(&g_tui_drv_lock);
	}

	return 0;
}

/* Only after all drivers cfg ok or some one failed, it need
 * to add_tui_msg.
 * ret val:  1 - all cfg ok
 *			 0 - cfg is not complete, or have done
 *			-1 - cfg failed
 *			-2 - invalid name
 */
static int tui_cfg_filter(const char *name, bool ok)
{
	struct tui_drv_node *pos = NULL;
	int find = 0;
	int lock_flag = 0;

	/* Return error if name is invalid */
	if (name == NULL) {
		tloge("name is null");
		return INVALID_CFG_NAME;
	}

	/* some drivers may call send_tui_msg_config at the end
	 * of drv_init_func which had got the lock.
	 */
	if (mutex_is_locked(&g_tui_drv_lock))
		lock_flag = 1;
	if (!lock_flag)
		mutex_lock(&g_tui_drv_lock);
	list_for_each_entry(pos, &g_tui_drv_head, list) {
		if (strncmp(pos->name, name, TUI_DRV_NAME_MAX) != 0)
			continue;

		find = 1;
		if (ok) {
			pos->state = 1;
		} else {
			if (!lock_flag)
				mutex_unlock(&g_tui_drv_lock);
			return -1;
		}
	}
	if (!lock_flag)
		mutex_unlock(&g_tui_drv_lock);

	if (find == 0)
		return INVALID_CFG_NAME;

	return 1;
}

enum poll_class {
	CLASS_POLL_CONFIG,
	CLASS_POLL_RUNNING,
	CLASS_POLL_COMMON,
	CLASS_POLL_INVALID
};

static enum poll_class tui_poll_class(int event_type)
{
	enum poll_class class = CLASS_POLL_INVALID;

	switch (event_type) {
	case TUI_POLL_CFG_OK:
	case TUI_POLL_CFG_FAIL:
	case TUI_POLL_RESUME_TUI:
		class = CLASS_POLL_CONFIG;
		break;
	case TUI_POLL_TP:
	case TUI_POLL_TICK:
	case TUI_POLL_DELAYED_WORK:
		class = CLASS_POLL_RUNNING;
		break;
	case TUI_POLL_CANCEL:
		class = CLASS_POLL_COMMON;
		break;
	default:
		break;
	}
	return class;
}

int send_tui_msg_config(int type, int val, void *data)
{
	int ret;

	if (type >= TUI_POLL_MAX  || type < 0 || data == NULL) {
		tloge("invalid tui event type\n");
		return -EINVAL;
	}

	/* The g_tui_state should be CONFIG */
	if (atomic_read(&g_tui_state) != TUI_STATE_CONFIG) {
		tloge("failed to send tui msg(%s)\n", poll_event_type_name[type]);
		return -EINVAL;
	}

	if (tui_poll_class(type) == CLASS_POLL_RUNNING) {
		tloge("invalid tui event type(%s) in config state\n", poll_event_type_name[type]);
		return -EINVAL;
	}

	tlogi("send config event type %s(%s)\n", poll_event_type_name[type], (char *)data);

	if (type == TUI_POLL_CFG_OK || type == TUI_POLL_CFG_FAIL) {
		int cfg_ret;

		cfg_ret = tui_cfg_filter((const char *)data, TUI_POLL_CFG_OK == type);
		tlogd("tui driver(%s) cfg ret = %d\n", (char *)data, cfg_ret);
		if (cfg_ret == INVALID_CFG_NAME) {
			tloge("tui cfg filter failed, cfg_ret = %d\n", cfg_ret);
			return -EINVAL;
		}
	}

	ret = add_tui_msg(type, val, data);
	if (ret != 0) {
		tloge("add tui msg ret=%d\n", ret);
		return ret;
	}

	tlogi("add config msg type %s\n", poll_event_type_name[type]);

	/* wake up tui kthread */
	wake_up(&g_tui_msg_wq);

	return 0;
}

#define make32(high, low) ((((uint32_t)(high)) << 16) | (uint16_t)(low))

static bool package_notch_msg(struct mb_cmd_pack *mb_pack, uint8_t **buf_to_tee,
	struct teec_tui_parameter *tui_param)
{
	uint32_t buf_len = sizeof(*tui_param) - sizeof(tui_param->event_type);
	*buf_to_tee = mailbox_alloc(buf_len, 0);
	if (*buf_to_tee == NULL) {
		tloge("failed to alloc memory!\n");
		return false;
	}
	if (memcpy_s(*buf_to_tee, buf_len, &tui_param->value,
			 sizeof(*tui_param) - sizeof(tui_param->event_type)) != EOK) {
		tloge("copy notch data failed");
		mailbox_free(*buf_to_tee);
		return false;
	}
	mb_pack->operation.paramtypes =
		TEE_PARAM_TYPE_VALUE_INPUT |
		(TEE_PARAM_TYPE_VALUE_INPUT << TEE_PARAM_NUM);
	mb_pack->operation.params[0].value.a =
		(uint32_t)mailbox_virt_to_phys((uintptr_t)*buf_to_tee);
	mb_pack->operation.params[0].value.b =
		(uint64_t)mailbox_virt_to_phys((uintptr_t)*buf_to_tee) >> ADDR_TRANS_NUM;
	mb_pack->operation.params[1].value.a = buf_len;
	return true;
}

static void package_fold_msg(struct mb_cmd_pack *mb_pack,
	const struct teec_tui_parameter *tui_param)
{
	mb_pack->operation.paramtypes = teec_param_types(TEE_PARAM_TYPE_VALUE_INPUT,
			TEE_PARAM_TYPE_VALUE_INPUT,
			TEE_PARAM_TYPE_VALUE_INPUT,
			TEE_PARAM_TYPE_VALUE_INPUT);
	mb_pack->operation.params[0].value.a = tui_param->notch;
#ifdef CONFIG_TEE_TUI_DISPLAY_3_0
	mb_pack->operation.params[0].value.b = make32(g_dss_fd->comp.base.xres, g_dss_fd->comp.base.yres);
#else
	mb_pack->operation.params[0].value.b = make32(g_dss_fd->panel_info.xres, g_dss_fd->panel_info.yres);
#endif
	mb_pack->operation.params[1].value.a = tui_param->phy_width;
	mb_pack->operation.params[1].value.b = tui_param->phy_height;
	mb_pack->operation.params[2].value.a = tui_param->width;
	mb_pack->operation.params[2].value.b = tui_param->height;
	mb_pack->operation.params[3].value.a = tui_param->fold_state;
	mb_pack->operation.params[3].value.b = tui_param->display_state;
}

static bool check_uid_valid(uint32_t uid)
{
#ifdef TUI_DAEMON_UID_IN_OH
	return uid == TUI_DAEMON_UID_IN_OH;
#else
	return uid <= UID_MAX_VAL;
#endif
}

static int32_t tui_send_smc_cmd(int32_t event, struct mb_cmd_pack *mb_pack, struct tc_ns_smc_cmd smc_cmd)
{
	uint32_t uid;
	kuid_t kuid;

	kuid = current_uid();
	uid = kuid.val;

	if (check_uid_valid(uid) == false) {
		tloge("get invalid uid %d\n", uid);
		return -1;
	}

	if ((event != TUI_POLL_CANCEL) && (event != TUI_POLL_NOTCH) && (event != TUI_POLL_FOLD)) {
		tloge("no permission to send msg\n");
		return -1;
	}

	smc_cmd.cmd_type = CMD_TYPE_GLOBAL;
	smc_cmd.operation_phys = mailbox_virt_to_phys((uintptr_t)&mb_pack->operation);
	smc_cmd.operation_h_phys = mailbox_virt_to_phys((uintptr_t)&mb_pack->operation) >> HIGH_VALUES;
	smc_cmd.agent_id = event;
	smc_cmd.uid = uid;
	livepatch_down_read_sem();
	int32_t ret = tc_ns_smc(&smc_cmd);
	livepatch_up_read_sem();
	if (ret != 0) {
		tloge("tc ns smc fail 0x%x", ret);
		return ret;
	}

	return 0;
}

/* Send tui event by smc_cmd */
int tui_send_event(int event, struct teec_tui_parameter *tui_param)
{
	int status_temp;
	bool check_value = false;
	uint8_t *buf_to_tee = NULL;

	if (tui_param == NULL)
		return -1;

	if (event == TUI_POLL_NOTCH) {
		check_value = true;
	} else {
		if (g_dss_fd == NULL)
			return -1;

		status_temp = atomic_read(&g_tui_state);
#ifdef CONFIG_TEE_TUI_DISPLAY_3_0
		check_value = (status_temp != TUI_STATE_UNUSED && g_dss_fd->comp.power_on) || event == TUI_POLL_FOLD;
#else
		check_value = (status_temp != TUI_STATE_UNUSED && g_dss_fd->panel_power_on) || event == TUI_POLL_FOLD;
#endif
	}

	if (check_value) {
		struct tc_ns_smc_cmd smc_cmd = { {0}, 0 };
		struct mb_cmd_pack *mb_pack = NULL;
		int ret = 0;

		mb_pack = mailbox_alloc_cmd_pack();
		if (mb_pack == NULL) {
			tloge("alloc cmd pack failed\n");
			return -1;
		}

		switch(event) {
		case TUI_POLL_CANCEL:
			smc_cmd.cmd_id = GLOBAL_CMD_ID_TUI_EXCEPTION;
			break;
		case TUI_POLL_NOTCH:
			if (!package_notch_msg(mb_pack, &buf_to_tee,
				tui_param)) {
				mailbox_free(mb_pack);
				tloge("package notch msg failed\n");
				return -1;
			}
			smc_cmd.cmd_id = GLOBAL_CMD_ID_TUI_NOTCH;
			break;
		case TUI_POLL_FOLD:
			package_fold_msg(mb_pack, tui_param);
			smc_cmd.cmd_id = GLOBAL_CMD_ID_TUI_FOLD;
			break;
		default:
			tloge("invalid event type : %d\n",event);
			break;
		}

		ret = tui_send_smc_cmd(event, mb_pack, smc_cmd);
		if (ret != 0)
			tloge("tui_send_smc_cmd error 0x%x", ret);

		mailbox_free(mb_pack);
		if (buf_to_tee != NULL)
			mailbox_free(buf_to_tee);
		return ret;
	} else {
		tlogi("tui unused no need send tui event!\n");
		return 0;
	}
}

static void tui_poweroff_work_func(struct work_struct *work)
{
	struct teec_tui_parameter tui_param = {0};
	tui_send_event(TUI_POLL_CANCEL, &tui_param);
}

void tui_poweroff_work_start(void)
{
	tlogi("tui_poweroff_work_start----------\n");
	if (g_dss_fd == NULL)
		return;

#ifdef CONFIG_TEE_TUI_DISPLAY_3_0
	if (atomic_read(&g_tui_state) != TUI_STATE_UNUSED && g_dss_fd->comp.power_on) {
#else
	if (atomic_read(&g_tui_state) != TUI_STATE_UNUSED && g_dss_fd->panel_power_on) {
#endif
		tlogi("come in tui_poweroff_work_start state=%d--\n",
		atomic_read(&g_tui_state));
		queue_work(system_wq, &tui_poweroff_work.work);
	}
}

static void wait_tui_msg(void)
{
#ifndef CONFIG_TEE_TUI_MTK
	if (wait_event_interruptible(g_tui_msg_wq, g_tui_msg_flag))
		tloge("get tui state is interrupted\n");
#endif
	/* mtk is sync mess, don't need wait */
}

static int valid_msg(int msg_type)
{
	switch (msg_type) {
	case TUI_POLL_RESUME_TUI:
		if (atomic_read(&g_tui_state) == TUI_STATE_RUNNING)
			return 0;
		break;
	case TUI_POLL_CANCEL:
		if (atomic_read(&g_tui_state) == TUI_STATE_UNUSED)
			return 0;
		break;
	default:
		break;
	}

	return 1;
}
/*
 * 1: init ok
 * 0: still do init
 * -1: init failed
 */
static int get_cfg_state(const char *name)
{
	const struct tui_msg_node *tui_msg = NULL;

	/* Return error if name is invalid */
	if (name == NULL) {
		tloge("name is null");
		return -1;
	}

	list_for_each_entry(tui_msg, &g_tui_msg_head, list) {
		/* Names match */
		if (!strncmp(tui_msg->data, name, TUI_DRV_NAME_MAX)) {
			if (TUI_POLL_CFG_OK == tui_msg->type)
				return 1;
			else if (TUI_POLL_CFG_FAIL == tui_msg->type)
				return -1;
			else
				tloge("other state\n");
		}
	}

	return 0;
}

static void tui_msg_del(const char *name)
{
	struct tui_msg_node *tui_msg = NULL, *tmp = NULL;

	/* Return error if name is invalid */
	if (name == NULL) {
		tloge("name is null");
		return;
	}

	list_for_each_entry_safe(tui_msg, tmp, &g_tui_msg_head, list) {
		/* Names match */
		if (!strncmp(tui_msg->data, name, TUI_DRV_NAME_MAX)) {
			list_del(&tui_msg->list);
			kfree(tui_msg);
		}
	}
}
#define DSS_CONFIG_INDEX 1
#define TP_CONFIG_INDEX  2

static int32_t process_tui_poll_cfg(int32_t type)
{
	/* pre-process tui poll event if needed */
	switch (type) {
	case TUI_POLL_CFG_OK:
		if (DSS_CONFIG_INDEX == g_tui_ctl->s2n.value) {
			phys_addr_t tui_addr_t;
			tui_addr_t = get_frame_addr();
			if (tui_addr_t == 0)
				tloge("get frame addr error\n");

			g_tui_ctl->n2s.addr = (unsigned int)tui_addr_t;
			g_tui_ctl->n2s.addr_h = tui_addr_t >> HIGH_VALUES;
			g_tui_ctl->n2s.npages = g_tui_display_mem.npages;
			g_tui_ctl->n2s.info_length = g_tui_display_mem.info_length;
			g_tui_ctl->n2s.phy_size = g_tui_display_mem.len;
			if (g_tui_ctl->n2s.addr == 0)
				return -1;
		}
		break;
	default:
		break;
	}

	return 0;
}

static int32_t process_tui_msg_dss(void)
{
	int32_t type = TUI_POLL_CFG_OK;

#if ONLY_INIT_TP != DSS_TP_COUPLE_MODE
	/* Wait, until DSS init finishs */
	spin_lock(&g_tui_msg_lock);
#ifdef CONFIG_TEE_TUI_MTK
	if (get_cfg_state(TUI_DSS_NAME) == 0) {
#else
	while (get_cfg_state(TUI_DSS_NAME) == 0) {
#endif
		tlogi("waiting for dss tui msg\n");
		g_tui_msg_flag = 0;
		spin_unlock(&g_tui_msg_lock);
		wait_tui_msg();
		tlogi("get dss init ok tui msg\n");
		spin_lock(&g_tui_msg_lock);
	}
	if (get_cfg_state(TUI_DSS_NAME) == -1) {
		tloge("dss init failed\n");
		type = TUI_POLL_CFG_FAIL;
	}
	/* Delete DSS msg from g_tui_msg_head */
	tui_msg_del(TUI_DSS_NAME);
	spin_unlock(&g_tui_msg_lock);
#endif

	return type;
}

static int32_t process_tui_msg_tp(void)
{
	int32_t type = 0;

	spin_lock(&g_tui_msg_lock);
#if ONLY_INIT_DSS != DSS_TP_COUPLE_MODE
	while (get_cfg_state(TUI_TP_NAME) == 0) {
		tlogi("waiting for tp tui msg\n");
		g_tui_msg_flag = 0;
		spin_unlock(&g_tui_msg_lock);
		wait_tui_msg();
		tlogi("get tp init ok tui msg\n");
		spin_lock(&g_tui_msg_lock);
	}
	if (get_cfg_state(TUI_TP_NAME) == -1) {
		tloge("tp failed to do init\n");
		tui_msg_del(TUI_TP_NAME);
		spin_unlock(&g_tui_msg_lock);
		return TUI_POLL_CFG_FAIL;
	}
	tui_msg_del(TUI_TP_NAME);
#if defined CONFIG_TEE_TUI_FP
	if (init_tui_driver(1) == 0) {
		while (get_cfg_state(TUI_GPIO_NAME) == 0 ||
			   get_cfg_state(TUI_FP_NAME) == 0) {
			tlogd("waiting for gpio/fp tui msg\n");
			g_tui_msg_flag = 0;
			spin_unlock(&g_tui_msg_lock);
			wait_tui_msg();
			tlogd("get gpio/fp init ok tui msg\n");
			spin_lock(&g_tui_msg_lock);
		}
		if (get_cfg_state(TUI_GPIO_NAME) == -1 ||
			get_cfg_state(TUI_FP_NAME) == -1) {
			tloge("one of gpio/fp failed to do init\n");
			type = TUI_POLL_CFG_FAIL;
		}
	}
	tui_msg_del(TUI_GPIO_NAME);
	tui_msg_del(TUI_FP_NAME);
#endif
	tlogd("tp/gpio/fp is config result:type = 0x%x\n", type);
#endif
	spin_unlock(&g_tui_msg_lock);
	return type;
}

static void process_tui_msg(void)
{
	int32_t val = 0;
	int32_t type = TUI_POLL_CFG_OK;

fetch_msg:
	if (g_tui_ctl->s2n.value == DSS_CONFIG_INDEX)
		type = process_tui_msg_dss();
	else if (g_tui_ctl->s2n.value == TP_CONFIG_INDEX)
		type = process_tui_msg_tp();
	else
		tloge("wait others dev\n");

	val = process_tui_poll_cfg(type);

	g_tui_ctl->n2s.event_type = type;
	g_tui_ctl->n2s.value = val;

	if (!valid_msg(g_tui_ctl->n2s.event_type)) {
		tlogi("refetch tui msg\n");
		goto fetch_msg;
	}
}

static int init_tui_agent(void)
{
	int ret;

	ret = tc_ns_register_agent(NULL, TEE_TUI_AGENT_ID, SZ_4K, (void **)(&g_tui_ctl), false);
	if (ret != 0) {
		tloge("register tui agent failed, ret = 0x%x\n", ret);
		g_tui_ctl = NULL;
		return -EFAULT;
	}

	return 0;
}

static void exit_tui_agent(void)
{
	if (tc_ns_unregister_agent(TEE_TUI_AGENT_ID) != 0)
		tloge("unregister tui agent failed\n");

	g_tui_ctl = NULL;
}

static void set_tui_state(int state)
{
	if (state < TUI_STATE_UNUSED || state > TUI_STATE_ERROR) {
		tloge("state=%d is invalid\n",state);
		return;
	}
	if (atomic_read(&g_tui_state) != state) {
		atomic_set(&g_tui_state, state);
		tloge("set ree tui state is %d, 0: unused, 1:config , 2:running\n", state);
		g_tui_state_flag = 1;
		wake_up(&g_tui_state_wq);
	}
}

int is_tui_in_use(int pid_value)
{
	if (pid_value == atomic_read(&g_tui_pid))
		return 1;
	return 0;
}

void free_tui_caller_info(void)
{
	atomic_set(&g_tui_attached_device, TUI_PID_CLEAR);
	atomic_set(&g_tui_pid, TUI_PID_CLEAR);
}

static int agent_process_work_tui(void)
{
	struct smc_event_data *event_data = NULL;

	event_data = find_event_control(TEE_TUI_AGENT_ID);
	if (event_data == NULL || atomic_read(&event_data->agent_ready) == AGENT_CRASHED) {
		/* if return, the pending task in S can't be resumed!! */
		tloge("tui agent is not exist\n");
		put_agent_event(event_data);
		return TEEC_ERROR_GENERIC;
	}

	isb();
	wmb();
	event_data->ret_flag = 1;
	/* Wake up tui agent that will process the command */
	wake_up(&event_data->wait_event_wq);

	tlogi("agent 0x%x request, goto sleep, pe->run=%d\n",
		TEE_TUI_AGENT_ID, atomic_read(&event_data->ca_run));
	wait_event(event_data->ca_pending_wq, atomic_read(&event_data->ca_run));
	atomic_set(&event_data->ca_run, 0);
	put_agent_event(event_data);

	return TEEC_SUCCESS;
}

void do_ns_tui_release(void)
{
	if (atomic_read(&g_tui_state) != TUI_STATE_UNUSED) {
		g_tui_ctl->s2n.command = TUI_CMD_EXIT;
		g_tui_ctl->s2n.ret = -1;
		tloge("exec tui do_ns_tui_release\n");
		if (agent_process_work_tui() != 0)
			tloge("wake up tui agent error\n");
	}
}
static int32_t do_tui_ttf_work(void)
{
	int ret = 0;
	switch (g_tui_ctl->s2n.command) {
	case TUI_CMD_LOAD_TTF:
		ret = load_tui_font_file();
		if (ret == 0) {
			tlogi("=======succeed to load ttf\n");
			g_tui_ctl->n2s.event_type = TUI_POLL_CFG_OK;
		} else {
			tloge("Failed to load normal ttf ret is 0x%x\n", ret);
			g_tui_ctl->n2s.event_type = TUI_POLL_CFG_FAIL;
		}
		break;
	case TUI_CMD_EXIT:
		if (atomic_read(&g_tui_state) != TUI_STATE_UNUSED &&
			atomic_dec_and_test(&g_tui_usage)) {
			tlogi("tui disable\n");
			(void)init_tui_driver(UNSECURE_ENV);
			free_frame_addr();
			free_tui_font_mem();
			free_tui_caller_info();
			set_tui_state(TUI_STATE_UNUSED);
		}
		break;
	case TUI_CMD_FREE_TTF_MEM:
		free_tui_font_mem();
		ret = 0;
		break;
	default:
		ret = -EINVAL;
		tloge("get error ttf tui command(0x%x)\n", g_tui_ctl->s2n.command);
		break;
	}
	return ret;
}

static void process_tui_enable(void)
{
	if (atomic_read(&g_tui_state) == TUI_STATE_CONFIG)
		return;

	tlogi("tui enable\n");
	set_tui_state(TUI_STATE_CONFIG);
	/* do dss and tp init */
	if (init_tui_driver(SECURE_ENV) != 0) {
		g_tui_ctl->s2n.ret = -1;
		set_tui_state(TUI_STATE_ERROR);
		(void)init_tui_driver(UNSECURE_ENV);
		free_tui_caller_info();
		set_tui_state(TUI_STATE_UNUSED);
		return;
	}
	atomic_inc(&g_tui_usage);
}

static void process_tui_disable(void)
{
	if (atomic_read(&g_tui_state) == TUI_STATE_UNUSED ||
		!atomic_dec_and_test(&g_tui_usage))
		return;

	tlogi("tui disable\n");
	(void)init_tui_driver(UNSECURE_ENV);
	free_frame_addr();
	free_tui_caller_info();
	set_tui_state(TUI_STATE_UNUSED);
}
static void process_tui_pause(void)
{
	if (atomic_read(&g_tui_state) == TUI_STATE_UNUSED)
		return;

	tlogi("tui pause\n");
	(void)init_tui_driver(UNSECURE_ENV);
	set_tui_state(TUI_STATE_CONFIG);
}
static int do_tui_config_work(void)
{
	int ret = 0;

	switch (g_tui_ctl->s2n.command) {
	case TUI_CMD_ENABLE:
		process_tui_enable();
		break;
	case TUI_CMD_DISABLE:
		process_tui_disable();
		break;
	case TUI_CMD_PAUSE:
		process_tui_pause();
		break;
	case TUI_CMD_POLL:
		process_tui_msg();
		break;
	case TUI_CMD_DO_SYNC:
		tlogd("enable tp irq cmd\n");
		break;
	case TUI_CMD_SET_STATE:
		tlogi("tui set state %d\n", g_tui_ctl->s2n.value);
		set_tui_state(g_tui_ctl->s2n.value);
		break;
	case TUI_CMD_START_DELAY_WORK:
		tlogd("start delay work\n");
		break;
	case TUI_CMD_CANCEL_DELAY_WORK:
		tlogd("cancel delay work\n");
		break;
	default:
		ret = -EINVAL;
		tloge("get error config tui command(0x%x)\n", g_tui_ctl->s2n.command);
		break;
	}
	return ret;
}
static int do_tui_work(void)
{
	int ret = 0;

	/* clear s2n cmd ret */
	g_tui_ctl->s2n.ret = 0;
	switch (g_tui_ctl->s2n.command) {
	case TUI_CMD_ENABLE:
	case TUI_CMD_DISABLE:
	case TUI_CMD_PAUSE:
	case TUI_CMD_POLL:
	case TUI_CMD_DO_SYNC:
	case TUI_CMD_SET_STATE:
	case TUI_CMD_START_DELAY_WORK:
	case TUI_CMD_CANCEL_DELAY_WORK:
		ret = do_tui_config_work();
		break;
	case TUI_CMD_LOAD_TTF:
	case TUI_CMD_EXIT:
	case TUI_CMD_FREE_TTF_MEM:
		ret = do_tui_ttf_work();
		break;
	default:
		ret = -EINVAL;
		tloge("get error tui command\n");
		break;
	}
	return ret;
}

void set_tui_caller_info(unsigned int devid, int pid)
{
	atomic_set(&g_tui_attached_device, (int)devid);
	atomic_set(&g_tui_pid, pid);
}

unsigned int tui_attach_device(void)
{
	return (unsigned int)atomic_read(&g_tui_attached_device);
}

static int tui_kthread_work_fn(void *data)
{
	int ret;
	ret = init_tui_agent();
	if (ret != 0) {
		tloge("init tui agent error, ret = %d\n", ret);
		return ret;
	}

	while (1) {
		tc_ns_wait_event(TEE_TUI_AGENT_ID);

		if (kthread_should_stop())
			break;

		do_tui_work();

		if (tc_ns_send_event_response(TEE_TUI_AGENT_ID) != 0)
			tloge("send event response error\n");
	}

	exit_tui_agent();

	return 0;
}

#define READ_BUF 128
static ssize_t tui_dbg_state_read(struct file *filp,char __user *ubuf,
				  size_t cnt, loff_t *ppos)
{
	char buf[READ_BUF] = {0};
	unsigned int r;
	int ret;
	struct tui_drv_node *pos = NULL;

	if (filp == NULL || ubuf == NULL || ppos == NULL)
		return -EINVAL;

	ret = snprintf_s(buf, READ_BUF, READ_BUF - 1, "tui state:%s\n",
			 state_name[atomic_read(&g_tui_state)]);
	if (ret < 0) {
		tloge("tui dbg state read 1 snprintf is failed, ret = 0x%x\n", ret);
		return -EINVAL;
	}
	r = (unsigned int)ret;

	ret = snprintf_s(buf + r, READ_BUF - r, READ_BUF - r - 1, "%s", "drv config state:");
	if (ret < 0) {
		tloge("tui dbg state read 2 snprintf is failed, ret = 0x%x\n", ret);
		return -EINVAL;
	}
	r += (unsigned int)ret;

	mutex_lock(&g_tui_drv_lock);
	list_for_each_entry(pos, &g_tui_drv_head, list) {
		ret = snprintf_s(buf + r, READ_BUF - r, READ_BUF - r - 1, "%s-%s,", pos->name, 1 == pos->state ? "ok" : "no ok");
		if (ret < 0) {
			tloge("tui dbg state read 3 snprintf is failed, ret = 0x%x\n", ret);
			mutex_unlock(&g_tui_drv_lock);
			return -EINVAL;
		}
		r += (unsigned int)ret;
	}
	mutex_unlock(&g_tui_drv_lock);
	if (r < READ_BUF)
		buf[r-1]='\n';

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static const struct file_operations tui_dbg_state_fops = {
	.owner = THIS_MODULE,
	.read = tui_dbg_state_read,
};

#define MAX_SHOW_BUFF_LEN 32
static ssize_t tui_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	int r;
	size_t buf_len = 0;
	if (kobj == NULL || attr == NULL || buf == NULL)
		return -EINVAL;

	g_tui_state_flag = 0;
	r = wait_event_interruptible(g_tui_state_wq, g_tui_state_flag);
	if (r != 0) {
		tloge("get tui state is interrupted\n");
		return r;
	}
	buf_len = MAX_SHOW_BUFF_LEN;
	r = snprintf_s(buf, buf_len, buf_len - 1, "%s", state_name[atomic_read(&g_tui_state)]);
	if (r < 0) {
		tloge("tui statue show snprintf is failed, ret = 0x%x\n", r);
		return -1;
	}

	return r;
}

#define MSG_BUF 512
static ssize_t tui_dbg_msg_read(struct file *filp, char __user *ubuf,
				size_t cnt, loff_t *ppos)
{
	char buf[MSG_BUF] = {0};
	int ret;
	int i;
	struct tui_drv_node *pos = NULL;

	if (filp == NULL || ubuf == NULL || ppos == NULL)
		return -EINVAL;

	ret = snprintf_s(buf, MSG_BUF, MSG_BUF - 1, "%s", "event format: event_type:val\n"
			 "event type:\n");
	if (ret < 0)
		return -EINVAL;

	unsigned int r = (unsigned int)ret;

	/* event type list */
	for (i = 0; i < TUI_POLL_MAX - 1; i++) {
		ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r - 1, "%s, ",
				 poll_event_type_name[i]);
		if (ret < 0) {
			tloge("tui db msg read 2 snprint is error, ret = 0x%x\n", ret);
			return -EINVAL;
		}
		r += (unsigned int)ret;
	}
	ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r - 1, "%s\n", poll_event_type_name[i]);
	if (ret < 0) {
		tloge("tui db msg read 3 snprint is error, ret = 0x%x\n", ret);
		return -EINVAL;
	}
	r += (unsigned int)ret;

	/* cfg drv type list */
	ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r - 1, "val type for %s or %s:\n",
		poll_event_type_name[TUI_POLL_CFG_OK], poll_event_type_name[TUI_POLL_CFG_FAIL]);
	if (ret < 0) {
		tloge("tui db msg read 4 snprint is error, ret = 0x%x\n", ret);
		return -EINVAL;
	}
	r += (unsigned int)ret;

	mutex_lock(&g_tui_drv_lock);
	list_for_each_entry(pos, &g_tui_drv_head, list) {
		ret = snprintf_s(buf + r, MSG_BUF - r, MSG_BUF - r - 1, "%s,", pos->name);
		if (ret < 0) {
			tloge("tui db msg read 5 snprint is error, ret = 0x%x\n", ret);
			mutex_unlock(&g_tui_drv_lock);
			return -EINVAL;
		}
		r += (unsigned int)ret;
	}
	mutex_unlock(&g_tui_drv_lock);
	if (r < MSG_BUF)
		buf[r - 1] = '\n';

	return simple_read_from_buffer(ubuf, cnt, ppos, buf, r);
}

static ssize_t tui_dbg_process_tp(const char *tokens, char **begins)
{
	long value = 0;
	int base = TP_BASE_VALUE;

	/* simple_strtol is obsolete, use kstrtol instead */
	int32_t ret = kstrtol(tokens, base, &value);
	if (ret != 0)
		return -EFAULT;
	g_tui_ctl->n2s.status = (int)value;

	tokens = strsep(begins, ":");
	if (tokens == NULL)
		return -EFAULT;

	ret = kstrtol(tokens, base, &value);
	if (ret != 0)
		return -EFAULT;
	g_tui_ctl->n2s.x = (int)value;

	tokens = strsep(begins, ":");
	if (tokens == NULL)
		return -EFAULT;

	ret = kstrtol(tokens, base, &value);
	if (ret != 0)
		return -EFAULT;
	g_tui_ctl->n2s.y = (int)value;
	return 0;
}

static ssize_t tui_dbg_msg_write(struct file *filp,
				const char __user *ubuf, size_t cnt,
				loff_t *ppos)
{
	char buf[64];
	int i;
	int event_type = -1;
	char *tokens = NULL, *begins = NULL;
	struct teec_tui_parameter tui_param = {0};

	if (ubuf == NULL || filp == NULL || ppos == NULL)
		return -EINVAL;

	if (cnt >= sizeof(buf)/sizeof(char))
		return -EINVAL;

	if (copy_from_user(buf, ubuf, cnt) != 0)
		return -EFAULT;

	buf[cnt] = 0;
	begins = buf;

	/* event type */
	tokens = strsep(&begins, ":");
	if (tokens == NULL)
		return -EFAULT;

	tlogd("1: tokens:%s\n", tokens);
	for (i = 0; i < TUI_POLL_MAX; i++) {
		if (strncmp(tokens, poll_event_type_name[i], strlen(poll_event_type_name[i])) == 0) {
			event_type = i;
			break;
		}
	}

	/* only for tp and cancel  */
	if (event_type != TUI_POLL_TP  && event_type != TUI_POLL_CANCEL)
		return -EFAULT;
	/* drv type */
	tokens = strsep(&begins, ":");
	if (tokens == NULL)
		return -EFAULT;

	tlogd("2: tokens:%s\n", tokens);
	if (event_type == TUI_POLL_TP) {
		if (tui_dbg_process_tp((const char *)tokens, &begins) != 0)
			return -EFAULT;
	}
	tlogd("status=%d x=%d y=%d\n", g_tui_ctl->n2s.status, g_tui_ctl->n2s.x, g_tui_ctl->n2s.y);

	if (tui_send_event(event_type, &tui_param))
		return -EFAULT;

	*ppos += cnt;

	return cnt;
}

static const struct file_operations tui_dbg_msg_fops = {
	.owner = THIS_MODULE,
	.read = tui_dbg_msg_read,
	.write = tui_dbg_msg_write,
};

static struct dentry *g_dbg_dentry = NULL;

static int tui_powerkey_notifier_call(struct notifier_block *powerkey_nb, unsigned long event, void *data)
{
#ifndef CONFIG_TEE_TUI_MTK
	if (event == PRESS_KEY_DOWN) {
		tui_poweroff_work_start();
	} else if (event == PRESS_KEY_UP) {
	} else if (event == PRESS_KEY_1S) {
	} else if (event == PRESS_KEY_6S) {
	} else if (event == PRESS_KEY_8S) {
	} else if (event == PRESS_KEY_10S) {
	} else {
		tloge("[%s]invalid event %ld !\n", __func__, event);
	}
#endif
#ifdef CONFIG_HW_COMB_KEY
	if (event == POWER_KEY_PRESS_DOWN) {
		tui_poweroff_work_start();
	} else {
		tloge("[%s]invalid event %ld !\n", __func__, event);
	}
#endif
	return 0;
}
static struct notifier_block tui_powerkey_nb;
int register_tui_powerkey_listener(void)
{
	tui_powerkey_nb.notifier_call = tui_powerkey_notifier_call;
#ifdef CONFIG_HW_COMB_KEY
	return power_key_register_notifier(&tui_powerkey_nb);
#else
	return powerkey_register_notifier(&tui_powerkey_nb);
#endif
}
int unregister_tui_powerkey_listener(void)
{
	tui_powerkey_nb.notifier_call = tui_powerkey_notifier_call;
#ifdef CONFIG_HW_COMB_KEY
	return power_key_unregister_notifier(&tui_powerkey_nb);
#else
	return powerkey_unregister_notifier(&tui_powerkey_nb);
#endif
}

int __init init_tui(const struct device *class_dev)
{
	int retval;
	struct sched_param param;
	param.sched_priority = MAX_RT_PRIO - 1;

	if (class_dev == NULL)
		return -1;

	g_tui_task = kthread_create(tui_kthread_work_fn, NULL, "tuid");
	if (IS_ERR_OR_NULL(g_tui_task)) {
		tloge("kthread create is error\n");
		return PTR_ERR(g_tui_task);
	}

	sched_setscheduler_nocheck(g_tui_task, SCHED_FIFO, &param);
	get_task_struct(g_tui_task);

	tz_kthread_bind_mask(g_tui_task);
	wake_up_process(g_tui_task);

	INIT_LIST_HEAD(&g_tui_msg_head);
	spin_lock_init(&g_tui_msg_lock);

	init_waitqueue_head(&g_tui_state_wq);
	init_waitqueue_head(&g_tui_msg_wq);
	g_dbg_dentry = debugfs_create_dir("tui", NULL);
#ifdef DEBUG_TUI
	debugfs_create_file("message", 0440, g_dbg_dentry, NULL, &tui_dbg_msg_fops);
#endif
	debugfs_create_file("d_state", 0440, g_dbg_dentry, NULL, &tui_dbg_state_fops);
	g_tui_kobj = kobject_create_and_add("tui", kernel_kobj);
	if (g_tui_kobj == NULL) {
		tloge("tui kobj create error\n");
		retval =  -ENOMEM;
		goto error2;
	}
	retval = sysfs_create_group(g_tui_kobj, &g_tui_attr_group);

	if (retval) {
		tloge("sysfs_create_group error, retval = 0x%x\n", retval);
		goto error1;
	}

	retval = register_tui_powerkey_listener();
	if (retval != 0) {
		tloge("tui register failed, retval = 0x%x\n", retval);
		goto error1;
	}
	return 0;
error1:
	kobject_put(g_tui_kobj);
error2:
	kthread_stop(g_tui_task);
	return retval;
}

void free_tui(void)
{
	if(unregister_tui_powerkey_listener() < 0)
		tloge("tui power key unregister failed\n");
	kthread_stop(g_tui_task);
	put_task_struct(g_tui_task);
	debugfs_remove(g_dbg_dentry);
	sysfs_remove_group(g_tui_kobj, &g_tui_attr_group);
	kobject_put(g_tui_kobj);
}

int tc_ns_tui_event(struct tc_ns_dev_file *dev_file, const void *argp)
{
	struct teec_tui_parameter tui_param = {0};
	int ret;

	if (!dev_file || !argp) {
		tloge("argp or dev is NULL\n");
		return -EINVAL;
	}

	if (copy_from_user(&tui_param, argp, sizeof(tui_param))) {
		tloge("copy from user failed\n");
		return -ENOMEM;
	}

	if (tui_param.event_type == TUI_POLL_CANCEL ||
		tui_param.event_type == TUI_POLL_NOTCH ||
		tui_param.event_type == TUI_POLL_FOLD) {
		ret = tui_send_event(tui_param.event_type, &tui_param);
	} else {
		tloge("no permission to send event\n");
		ret = -EACCES;
	}

	return ret;
}

bool is_tui_agent(unsigned int agent_id)
{
	return agent_id == TEE_TUI_AGENT_ID;
}
