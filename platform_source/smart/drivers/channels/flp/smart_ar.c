/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2017-2021. All rights reserved.
 * Description: Contexthub activity recognition driver.
 * Create: 2017-03-31
 */
#include "smart_ar.h"
#include <securec.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/pm.h>
#include <linux/platform_device.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/time.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/pm_wakeup.h>
#include <platform_include/smart/linux/iomcu_log.h>
#include <net/genetlink.h>
#include <platform_include/smart/linux/iomcu_status.h>
#include "shmem/shmem.h"

#define AR_COMPATIBLE_NAME "hisilicon,hismart-ar"

#ifdef CONFIG_EXT_INPUTHUB
#include "smart_1135_ar.h"
#endif

#ifdef __LLT_UT__
#define STATIC
#else
#define STATIC static
#endif

#define AR_OPEN_CLINET_LIMIT 32
static struct ar_device_t  g_ar_dev;
static int g_dts_status;

#define AR_MAX_BUF_LEN 1024
static char g_support_activities_buf[AR_MAX_BUF_LEN];
static int g_support_activities_buf_len;

#define AR_MAX_GET_DATA   (1024 * 200)

typedef int (*work_handler)(const struct pkt_header *head);

struct ar_report_work {
	struct work_struct worker;
	work_handler wh;
	void *data;
};

/*
 * AR sends commands through shared memory
 */
static int ar_send_cmd_through_shmem(const struct ar_hdr_t *head, const char *buf, size_t count)
{
	int ret;
	char *data = NULL;
	size_t len = count + sizeof(struct ar_hdr_t);

	if (len > MAX_CONFIG_SIZE || len < sizeof(struct ar_hdr_t)) {
		ctxhub_err("hismart:[%s]:line[%d] shmem send size[%d] too large ,error\n",  __func__, __LINE__, (int)len);
		return -EINVAL;
	}
	data = kzalloc(len, GFP_KERNEL);
	if (data == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] shmem send kzalloc[%d] fail\n", __func__, __LINE__, (int)len);
		return -ENOMEM;
	}
	ret = memcpy_s(data, sizeof(struct ar_hdr_t), head, len - count);
	if (ret)
		ctxhub_err("hismart [%s] memcpy_s fail\n", __func__);

	if (len > sizeof(struct ar_hdr_t) && buf != NULL) {
		ret = memcpy_s(data + sizeof(struct ar_hdr_t), count, buf, len - sizeof(struct ar_hdr_t));
		if (ret)
			ctxhub_err("hismart [%s] memset_s fail[%d]\n", __func__, ret);
	}

#ifdef CONFIG_CONTEXTHUB_SHMEM
	ret = shmem_send(TAG_AR, (void *)data, len);
#else
	ctxhub_err("hismart:[%s]:line[%d] shmem has not enable len[%d]\n", __func__, __LINE__, (int)len);
	ret = 0;
#endif
	kfree((void *)data);
	return ret;
}

/*
 * AR sends commands to contexthub
 */
STATIC int ar_send_cmd_from_kernel(unsigned char cmd_tag, unsigned char cmd_type,
					  unsigned int subtype, const char *buf, size_t count)
{
	unsigned int sub_cmd = 0;

	struct ar_hdr_t *ptr_hdr = (struct ar_hdr_t *)&sub_cmd;

	ptr_hdr->sub_cmd = (unsigned char)subtype;

#ifdef CONFIG_EXT_INPUTHUB
	if (g_dts_status == EXT_SENSORHUB_DTS_ENABLED)
		return send_cmd_ext_intputhub(cmd_type, subtype, buf, count);
#endif
	if (count > (MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE))
		return ar_send_cmd_through_shmem(ptr_hdr, buf, count);
	else
		return send_cmd_from_kernel(cmd_tag, cmd_type, sub_cmd, buf, count);
}

/*
 * AR sends commands to contexthub nolock
 */
static int ar_send_cmd_from_kernel_nolock(unsigned char cmd_tag, unsigned char cmd_type,
					unsigned int subtype, const char *buf, size_t count)
{
	unsigned int sub_cmd = 0;

	struct ar_hdr_t *ptr_hdr = (struct ar_hdr_t *)&sub_cmd;

	ptr_hdr->sub_cmd = (unsigned char)subtype;
	if (count > (MAX_PKT_LENGTH - CONTEXTHUB_HEADER_SIZE))
		return ar_send_cmd_through_shmem(ptr_hdr, buf, count);

	return send_cmd_from_kernel_nolock(cmd_tag, cmd_type, sub_cmd, buf, count);
}

/*
 * AR sends commands to contexthub and waits for reply
 */
static int ar_send_cmd_from_kernel_response(unsigned char cmd_tag, unsigned char cmd_type,
					unsigned int subtype, const char *buf, size_t count, struct read_info *rd)
{
	unsigned int sub_cmd = 0;

	struct ar_hdr_t *ptr_hdr = (struct ar_hdr_t *)&sub_cmd;

	ptr_hdr->sub_cmd = (unsigned char)subtype;
	if (g_dts_status == EXT_SENSORHUB_DTS_ENABLED)
		return ar_send_cmd_from_kernel(cmd_tag, cmd_type, subtype, buf, count);

	return send_cmd_from_kernel_response(cmd_tag, cmd_type, sub_cmd, buf, count, rd);
}

/*
 * fifo data input
 */
static void fifo_in(struct ar_data_buf_t *pdata, const char *data, unsigned int len, unsigned int align)
{
	unsigned int deta;
	int ret;

	if ((pdata == NULL) || (data == NULL) || (pdata->data_buf == NULL)) {
		ctxhub_err("hismart:[%s]:parm is null\n", __func__);
		return;
	}
	if (align == 0) {
		ctxhub_err("hismart:[%s]:align is zero\n", __func__);
		return;
	}

	/* no enough space , just overwrite */
	if ((pdata->data_count + len) > pdata->buf_size) {
		deta = pdata->data_count + len - pdata->buf_size;
		if (deta % align) {
			ctxhub_err("hismart:[%s]:line[%d]fifo in data not align\n", __func__, __LINE__);
			deta = (deta/align + 1)*align;
		}
		pdata->read_index = (pdata->read_index + deta)%pdata->buf_size;
	}
	/* copy data to flp pdr driver buffer */
	if ((pdata->write_index + len) >  pdata->buf_size) {
		ret = memcpy_s(pdata->data_buf + pdata->write_index, pdata->buf_size, data,
			       (size_t)(pdata->buf_size - pdata->write_index));
		if (ret)
			ctxhub_err("hismart [%s] memset_s fail[%d]\n", __func__, ret);
		ret = memcpy_s(pdata->data_buf, pdata->buf_size, data + pdata->buf_size - pdata->write_index,
			(size_t)(len + pdata->write_index - pdata->buf_size));
		if (ret)
			ctxhub_err("hismart [%s] memset_s fail[%d]\n", __func__, ret);
	} else {
		ret = memcpy_s(pdata->data_buf + pdata->write_index, pdata->buf_size - pdata->write_index, data, (size_t)len);
		if (ret)
			ctxhub_err("hismart [%s] memset_s fail[%d]\n", __func__, ret);
	}

	pdata->write_index = (pdata->write_index + len)%pdata->buf_size;
	pdata->data_count = (pdata->write_index - pdata->read_index + pdata->buf_size)%pdata->buf_size;
	/* if buf is full */
	if (!pdata->data_count)
		pdata->data_count = pdata->buf_size;
}

/*
 * fifo data out
 */
static int fifo_out(struct ar_data_buf_t *pdata, unsigned char *data, unsigned int count)
{
	int ret;

	if ((pdata == NULL) || (data == NULL) || (pdata->data_buf == NULL)) {
		ctxhub_err("hismart:[%s]:parm is null\n", __func__);
		return -EINVAL;
	}

	if (count  > pdata->data_count)
		count = pdata->data_count;

	/* copy data to user buffer */
	if ((pdata->read_index + count) >  pdata->buf_size) {
		ret = memcpy_s(data, count, pdata->data_buf + pdata->read_index, (size_t)(pdata->buf_size - pdata->read_index));
		if (ret)
			ctxhub_err("hismart:[%s] memcpy_s fail\n", __func__);
		ret = memcpy_s(data + pdata->buf_size - pdata->read_index, count,
		pdata->data_buf, (size_t)(count + pdata->read_index - pdata->buf_size));
	} else {
		ret = memcpy_s(data, pdata->buf_size - pdata->read_index, pdata->data_buf + pdata->read_index, (size_t)count);
	}
	if (ret)
		ctxhub_err("hismart:[%s] memcpy_s fail\n", __func__);
	pdata->read_index = (pdata->read_index + count)%pdata->buf_size;
	pdata->data_count -= count;
	return 0;
}

STATIC unsigned int fifo_len(struct ar_data_buf_t *pdata)
{
	if (pdata == NULL) {
		ctxhub_err("[%s] pdata is err.\n", __func__);
		return 0;
	}

	return pdata->data_count;
}


/*
 * why *dest use g_ar_dev.ar_devinfo.cfg, because cts open can be used directly.
 * dest:devinfo->cfg is dest port data
 * save ipc mem space
 */
STATIC unsigned int  context_cfg_set_to_iomcu(unsigned int context_max,
	struct context_iomcu_cfg_t *dest, const struct context_iomcu_cfg_t *new_set)
{
	int ret;
	unsigned int i = 0;
	unsigned char *ultimate_data = (unsigned char *)dest->context_list;
	(void)memset_s((void *)dest, sizeof(struct context_iomcu_cfg_t), 0, sizeof(struct context_iomcu_cfg_t));
#ifdef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
	dest->report_interval = (new_set->report_interval < 1)?1:(new_set->report_interval);
#endif
	for (i = 0; i < context_max; i++) {
		if (new_set->context_list[i].head.event_type) {
			ret = memcpy_s((void *)ultimate_data, sizeof(struct ar_context_cfg_header_t),
				(void *)&new_set->context_list[i], (unsigned long)sizeof(struct ar_context_cfg_header_t));
			if (ret)
				ctxhub_err("%s memset_s fail[%d]\n", __func__, ret);
			ultimate_data += sizeof(struct ar_context_cfg_header_t);
			if (new_set->context_list[i].head.len && new_set->context_list[i].head.len <= CONTEXT_PRIVATE_DATA_MAX) {
				ret = memcpy_s((void *)ultimate_data, CONTEXT_PRIVATE_DATA_MAX,
					new_set->context_list[i].buf, (unsigned long)new_set->context_list[i].head.len);
				if (ret)
					ctxhub_err("%s memset_s fail[%d]\n", __func__, ret);
				ultimate_data += new_set->context_list[i].head.len;
			}
			dest->context_num++;
		}
	}

	return (unsigned int)(ultimate_data - (unsigned char *)dest);
}


/* lint -e661 -e662 -e826 */
/*
 * Multi-instance scenarios,different buff data can impact on business,
 * but the kernel does not pay attention to this matter,APP service deal with this thing
 * config: already had cur ar_port->ar_config data,now update other port data into config
 */
static void ar_multi_instance(const struct ar_device_t *ar_dev, struct context_iomcu_cfg_t *config)
{
	int count;
	struct list_head *pos = NULL;
	struct ar_port_t *port = NULL;

	list_for_each(pos, &ar_dev->list) {
		port = container_of(pos, struct ar_port_t, list);
		if (port->channel_type & FLP_AR_DATA) {
			for (count = 0; count < AR_STATE_BUTT; count++) {
#ifndef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
				if (!config->context_list[count].head.event_type && port->ar_config.context_list[count].head.event_type) {
					config->context_list[count].head.report_interval = port->ar_config.context_list[count].head.report_interval;
				} else if (config->context_list[count].head.event_type && port->ar_config.context_list[count].head.event_type) {
					config->context_list[count].head.report_interval =
					(config->context_list[count].head.report_interval <= port->ar_config.context_list[count].head.report_interval) ?
					config->context_list[count].head.report_interval:port->ar_config.context_list[count].head.report_interval;

					config->context_list[count].head.report_interval =
					(config->context_list[count].head.report_interval < 1) ? 1:config->context_list[count].head.report_interval;
				}
#endif
				config->context_list[count].head.event_type |=
				port->ar_config.context_list[count].head.event_type;
			}
#ifdef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
		config->report_interval  =
		(config->report_interval <= port->ar_config.report_interval) ?
		config->report_interval:port->ar_config.report_interval;
#endif
		}
	}
}
/*lint -e715*/

/*
 * Send a command to close AR to contexthub
 */
STATIC int ar_stop(unsigned int delay)
{
	int count;
	struct context_dev_info_t *devinfo = &g_ar_dev.ar_devinfo;

	if (!(FLP_AR_DATA & g_ar_dev.service_type))
		return 0;

	if (devinfo->usr_cnt) {
		size_t len;
		struct context_iomcu_cfg_t *pcfg = kzalloc(sizeof(struct context_iomcu_cfg_t), GFP_KERNEL);

		if (pcfg == NULL) {
			ctxhub_err("hismart:[%s]:line[%d] kzalloc error\n", __func__, __LINE__);
			return -ENOMEM;
		}

		for (count = 0; count < AR_STATE_BUTT; count++)
			pcfg->context_list[count].head.context = (unsigned int)count;

#ifdef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
		pcfg->report_interval = ~0;
#endif
		ar_multi_instance(&g_ar_dev, pcfg);
		len = context_cfg_set_to_iomcu(AR_STATE_BUTT, &devinfo->cfg, pcfg);
		kfree((void *)pcfg);
		if (devinfo->cfg.context_num == 0) {
			ctxhub_err("hismart:[%s]:line[%d] context_cfg_set_to_iomcu context_num:0 error\n", __func__, __LINE__);
			return -EINVAL;
		}
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, len);
	} else {
		unsigned int delayed = 0;

		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_STOP_REQ, (char *)&delayed, sizeof(delayed));
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
		 g_ar_dev.service_type &= ~FLP_AR_DATA;
	}

	return 0;
}
/*lint +e661 +e662 +e826 +e715*/
/*
 * Release BUF resources
 */
static void data_buffer_exit(struct ar_data_buf_t *buf)
{
	if (buf->data_buf != NULL) {
		kfree((void *)buf->data_buf);
		buf->data_buf = NULL;
	}
}

/*
 * AR multi-instance judgment, send a command to close AR to contexthub
 */
static int  ar_stop_cmd(struct ar_port_t *ar_port, unsigned long arg)
{
	unsigned int delay = 0;
	int ret = 0;
	struct context_dev_info_t *devinfo = &g_ar_dev.ar_devinfo;

	mutex_lock(&g_ar_dev.lock);
	if (!(FLP_AR_DATA & ar_port->channel_type) || !(FLP_AR_DATA & g_ar_dev.service_type)) {
		ctxhub_err("hismart:[%s]:line[%d] had stopped[%x][%x]\n", __func__, __LINE__,
			   ar_port->channel_type, g_ar_dev.service_type);
		ret =  -EIO;
		goto AR_STOP_ERR;
	}

	if (copy_from_user(&delay, (void *)((uintptr_t)arg), sizeof(unsigned int))) {
		ctxhub_err("hismart:[%s]:line[%d] delay copy_from_user error\n", __func__, __LINE__);
		ret =  -EFAULT;
		goto AR_STOP_ERR;
	}
	ctxhub_info("hismart:[%s]:line[%d] delay[%u]\n", __func__, __LINE__, delay);
	(void)memset_s((void *)&ar_port->ar_config, sizeof(struct context_iomcu_cfg_t), 0, sizeof(struct context_iomcu_cfg_t));
	devinfo->usr_cnt--;

	ar_port->channel_type &= (~FLP_AR_DATA);
	ret = ar_stop(delay);
	if (ret) {
		ctxhub_err("hismart:[%s]:line[%d] ar_stop ret [%d] error\n", __func__, __LINE__, ret);
		ret = 0; /* Because the channel_type flag has been removed, the business has been closed for the application layer */
	}

AR_STOP_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

/*
 * Resource initialization
 */
static int data_buffer_init(struct ar_data_buf_t *buf, unsigned int buf_sz)
{
	int ret = 0;

	buf->buf_size = buf_sz;
	buf->read_index = 0;
	buf->write_index = 0;
	buf->data_count = 0;
	buf->data_buf = kzalloc((size_t)buf->buf_size, GFP_KERNEL);
	if (!buf->data_buf) {
		ctxhub_err("hismart:[%s]:line[%d] data_buf kzalloc err\n", __func__, __LINE__);
		return -ENOMEM;
	}

	return ret;
}

/*
 * devinfo->cfg.context_list :is INPUT PARM,SOURCE PARM.The order just like HAL layers,set of One app
 * config :ar_port->config,is OUT DEST PARM, natural order ,Global group
 */
static int create_ar_port_cfg(struct context_iomcu_cfg_t *config,
struct context_dev_info_t *devinfo, unsigned int context_max)
{
	unsigned int i;
	int ret;
	/* in fact, devinfo->cfg.context_list is hal data */
	struct context_config_t  *plist = (struct context_config_t  *)devinfo->cfg.context_list;

	/* make clean cur port data(out parm) */
	(void)memset_s(config, sizeof(struct context_iomcu_cfg_t), 0, sizeof(struct context_iomcu_cfg_t));
#ifdef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
	config->report_interval = devinfo->cfg.report_interval;
#endif
	for (i = 0; i < context_max; i++)
		config->context_list[i].head.context = i; /* int out parm natural enum order */

	for (i = 0; i < devinfo->cfg.context_num; i++, plist++) {
		if (plist->head.context >= context_max || plist->head.event_type >= AR_STATE_MAX ||
		    plist->head.len > CONTEXT_PRIVATE_DATA_MAX) {
			ctxhub_err("hismart:[%s]:line[%d] EPERM ERR c[%u]e[%u]len[%u]\n", __func__, __LINE__, plist->head.context,
			plist->head.event_type, plist->head.len);
			return -EPERM;
		}

		/* crate cur port cfg data(out parm) into natural enum order data */
		ret = memcpy_s((void *)&config->context_list[plist->head.context], sizeof(struct context_config_t),
			       (void *)plist, sizeof(struct context_config_t));
		if (ret)
			ctxhub_err("hismart [%s] memset_s fail[%d]\n", __func__, ret);
	}
	return 0;
}


/*lint -e838 -e438*/
STATIC int context_config(struct context_iomcu_cfg_t *config, struct context_dev_info_t *devinfo,
	unsigned long arg, unsigned int context_max)
{
	int ret = 0;
	struct context_hal_config_t hal_cfg_hdr;
	unsigned long usr_len;

	if (copy_from_user((void *)&hal_cfg_hdr, (void __user *)((uintptr_t)arg), sizeof(struct context_hal_config_t))) {
		ctxhub_err("hismart:[%s]:line[%d] copy_from_user context_hal_config_t err\n", __func__, __LINE__);
		return -EIO;
	}

	if (hal_cfg_hdr.context_num == 0 || hal_cfg_hdr.context_num > context_max) {
		ctxhub_err("hismart:[%s]:line[%d] num[%d]max[%d] is invalid\n", __func__, __LINE__,
			hal_cfg_hdr.context_num, context_max);
		return  -EINVAL;
	}

	(void)memset_s((void *)&devinfo->cfg, sizeof(struct context_iomcu_cfg_t), 0, sizeof(struct context_iomcu_cfg_t));
#ifdef CONFIG_CONTEXTHUB_UNIFORM_INTERVAL
	devinfo->cfg.report_interval = hal_cfg_hdr.report_interval;
#endif
	devinfo->cfg.context_num = hal_cfg_hdr.context_num;
	usr_len = sizeof(struct context_config_t) * hal_cfg_hdr.context_num;
	if (usr_len > sizeof(devinfo->cfg.context_list)) {
		ctxhub_err("hismart:[%s]:line[%d] usr_len[%lu] bufsize[%lu]err\n", __func__, __LINE__,
			usr_len, sizeof(devinfo->cfg.context_list));
		return  -ENOMEM;
	}
#if defined(CONFIG_COMPAT) && defined(CONFIG_EXT_INPUTHUB)
	if (copy_from_user((void *)devinfo->cfg.context_list,
			    (const void __user *)compat_ptr((uintptr_t)hal_cfg_hdr.context_addr),  usr_len)) {
		ctxhub_err("hismart:[%s]:line[%d] copy_from_user error context list\n", __func__, __LINE__);
		return -EDOM;
	}
#else
	if (copy_from_user((void *)devinfo->cfg.context_list, (const void __user *)hal_cfg_hdr.context_addr, usr_len)) {
		ctxhub_err("hismart:[%s]:line[%d] copy_from_user error context list\n", __func__, __LINE__);
		return -EDOM;
	}
#endif

	ret = create_ar_port_cfg(config, devinfo, context_max);
	if (ret)
		ctxhub_err("hismart:[%s]:line[%d] create_ar_port_cfg error\n", __func__, __LINE__);
	return ret;
}

/*lint +e838 +e438*/
STATIC int ar_config_to_iomcu(const struct ar_port_t *ar_port)
{
	int ret = 0;
	struct context_dev_info_t *devinfo = &g_ar_dev.ar_devinfo;

	if (devinfo->usr_cnt) {
		struct context_iomcu_cfg_t *pcfg = kzalloc(sizeof(struct context_iomcu_cfg_t), GFP_KERNEL);

		if (pcfg == NULL) {
			ctxhub_err("hismart:[%s]:line[%d] kzalloc error\n", __func__, __LINE__);
			ret = -ENOMEM;
			goto CFG_IOMCU_FIN;
		}
		ret = memcpy_s(pcfg, sizeof(struct context_iomcu_cfg_t), &ar_port->ar_config, sizeof(struct context_iomcu_cfg_t));
		if (ret)
			ctxhub_err("hismart [%s] memset_s fail[%d]\n", __func__, ret);

		ar_multi_instance(&g_ar_dev, pcfg);
		devinfo->cfg_sz = context_cfg_set_to_iomcu(AR_STATE_BUTT, &devinfo->cfg, pcfg);
		kfree((void *)pcfg);
		if (devinfo->cfg.context_num == 0) {
			ctxhub_err("hismart:[%s]:line[%d] context_cfg_set_to_iomcu context_num:0 error\n", __func__, __LINE__);
			ret = -ERANGE;
			goto CFG_IOMCU_FIN;
		}
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, (size_t)devinfo->cfg_sz);
	} else {
		struct read_info rd;
		(void)memset_s((void *)&rd, sizeof(struct read_info), 0, sizeof(struct read_info));
		devinfo->cfg_sz = context_cfg_set_to_iomcu(AR_STATE_BUTT, &devinfo->cfg, &ar_port->ar_config);
		if (devinfo->cfg.context_num == 0) {
			ctxhub_err("hismart:[%s]:line[%d] context_cfg_set_to_iomcu context_num:0 error\n", __func__, __LINE__);
			ret = -EDOM;
			goto CFG_IOMCU_FIN;
		}

		ret = ar_send_cmd_from_kernel_response(TAG_AR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
		if (ret) {
			ctxhub_err("hismart:[%s]:line[%d] hub not support context, open ar app fail[%d]\n", __func__, __LINE__, ret);
			goto CFG_IOMCU_FIN;
		}
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&devinfo->cfg, (size_t)devinfo->cfg_sz);
	}

	ctxhub_info("hismart:[%s]:line[%d] size[%u]\n", __func__, __LINE__, devinfo->cfg_sz);
CFG_IOMCU_FIN:
	return ret;
}


/*lint -e715*/
static int ar_config_cmd(struct ar_port_t *ar_port, unsigned int cmd, unsigned long arg)
{
	int ret;
	struct context_dev_info_t *devinfo = &g_ar_dev.ar_devinfo;

	mutex_lock(&g_ar_dev.lock);

	/* make hal data into ar_port->ar_config natural order data */
	ret = context_config(&ar_port->ar_config, devinfo, arg, AR_STATE_BUTT);
	if (ret) {
		ctxhub_err("hismart:[%s]:line[%d]context_config err\n", __func__, __LINE__);
		goto AR_CFG_FIN;
	}

	ret = ar_config_to_iomcu(ar_port);
	if (ret) {
		ctxhub_err("hismart:[%s]:line[%d] ar_config_to_iomcu err[%d]\n", __func__, __LINE__, ret);
		goto AR_CFG_FIN;
	}

	if (!(FLP_AR_DATA & ar_port->channel_type)) {
		devinfo->usr_cnt++;
		ar_port->channel_type |= FLP_AR_DATA;
		g_ar_dev.service_type |= FLP_AR_DATA;
	}

	mutex_unlock(&g_ar_dev.lock);
	return 0;

AR_CFG_FIN:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}
/*lint +e715*/
/*lint -e838*/

/*lint -e785*/
static struct genl_family ar_genl_family = {
#if (KERNEL_VERSION(4, 10, 0) > LINUX_VERSION_CODE)
	.id         = GENL_ID_GENERATE,
#endif
	.name       = AR_GENL_NAME,
	.version    = TASKAR_GENL_VERSION,
	.maxattr    = AR_GENL_ATTR_MAX,
};
/*lint +e785*/
/*
 * context_gnetlink resource allocation
 */
static struct sk_buff *context_gnetlink_alloc(struct ar_port_t *ar_port, unsigned int count, unsigned char cmd_type,
					      unsigned char **data)
{
	struct ar_data_buf_t *pdata = &ar_port->ar_buf;
	void *msg_header = NULL;
	struct sk_buff *skb = NULL;
	static unsigned int flp_event_seqnum;

	if (!ar_port->portid) {
		ctxhub_err("hismart:[%s]:line[%d] no portid error\n", __func__, __LINE__);
		return NULL;
	}

	skb = genlmsg_new((size_t)count, GFP_ATOMIC);
	if (skb == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] genlmsg_new error\n", __func__, __LINE__);
		return NULL;
	}

	msg_header = genlmsg_put(skb, 0, flp_event_seqnum++, &ar_genl_family, 0, cmd_type);
	if (msg_header == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] genlmsg_put error\n", __func__, __LINE__);
		nlmsg_free(skb);
		return NULL;
	}

	*data = nla_reserve_nohdr(skb, (int)count);
	if (*data == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] nla_reserve_nohdr error\n", __func__, __LINE__);
		nlmsg_free(skb);
		return NULL;
	}

	pdata->data_len = count;

	return skb;
}
/*lint -e826*/
/*
 * context_gnetlink data send
 */
static int context_gnetlink_send(struct ar_port_t *ar_port, struct sk_buff *skb)
{
	struct ar_data_buf_t *pdata =  &ar_port->ar_buf;
	struct nlmsghdr *nlh = NULL;
	int result;

	if (skb == NULL)
		return -EINVAL;
	nlh = nlmsg_hdr(skb);
	nlh->nlmsg_len = pdata->data_len + GENL_HDRLEN + (unsigned int)NLMSG_HDRLEN;
	result = genlmsg_unicast(&init_net, skb, ar_port->portid);
	if (result != 0)
		ctxhub_err("hismart:[%s]:line[%d] ar:genlmsg_unicast %d", __func__, __LINE__, result);

	return result;
}

/*
 * activity data report
 */
static int ar_data_report(struct ar_data_buf_t *pdata, struct ar_port_t *ar_port)
{
	unsigned char *data = NULL;
	struct sk_buff *skb = NULL;
	int ret = 0;

	if (fifo_len(pdata) == 0)
		goto AR_DATA_REPO_FIN;

	skb = context_gnetlink_alloc(ar_port, pdata->data_count, AR_GENL_CMD_AR_DATA, &data);
	if (data == NULL || skb == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] context_gnetlink_alloc ERR\n", __func__, __LINE__);
		ret = -EBUSY;
		goto AR_DATA_REPO_FIN;
	}

	ret = fifo_out(pdata, data, pdata->data_count);
	if (ret) {
		nlmsg_free(skb);
		ctxhub_err("hismart:[%s]:line[%d] fifo_out ERR[%d]\n", __func__, __LINE__, ret);
		goto AR_DATA_REPO_FIN;
	}
	ret = context_gnetlink_send(ar_port, skb);
AR_DATA_REPO_FIN:
	return ret;
}

/*
 * Send AR data reported by contexthub to HAL
 */

int get_ar_data_from_mcu(const struct pkt_header *head)
{
	unsigned int i, port_etype;
	struct ar_data_buf_t *pdata = NULL;
	const struct ar_data_req_t *pd = (const struct ar_data_req_t *)head;
	unsigned char *pcd = NULL;
	unsigned char *data_tail = (unsigned char *)head + sizeof(struct pkt_header) + head->length;
	struct context_event_t *pevent = NULL;
	int ret = 0;
	struct ar_port_t *ar_port = NULL;
	struct list_head *pos = NULL;

	if (pd->context_num == 0 || AR_STATE_BUTT < pd->context_num) {
		ctxhub_err("hismart:[%s]:line[%d] context_num [%d]err\n", __func__, __LINE__, pd->context_num);
		return -EBUSY;
	}
#ifndef CONFIG_EXT_INPUTHUB
	mutex_lock(&g_ar_dev.lock);
#endif
	ctxhub_info("hismart:[%s]:line[%d] num[%d]\n", __func__, __LINE__, pd->context_num);
	list_for_each(pos, &g_ar_dev.list) {
		ar_port = container_of(pos, struct ar_port_t, list);
		if (!(FLP_AR_DATA & ar_port->channel_type) || !ar_port->portid)
			continue;

		pdata = &ar_port->ar_buf;
		pcd = (unsigned char *)pd->context_event;
		for (i = 0; i < pd->context_num; i++) {
			if (pcd >= data_tail) {
				ctxhub_err("hismart:[%s]:line[%d] break[%d]\n", __func__, __LINE__, i);
				break;
			}
			pevent = (struct context_event_t *)pcd;
			if (pevent->context >= AR_STATE_BUTT) {
				ctxhub_err("hismart:[%s]:line[%d] pevent->context[%d] too large\n", __func__, __LINE__, pevent->context);
				break;
			}
			port_etype = ar_port->ar_config.context_list[pevent->context].head.event_type;
			if ((port_etype & pevent->event_type) || ((FLP_AR_DATA & ar_port->flushing) && (!pevent->event_type))) {
				if (pevent->buf_len <= CONTEXT_PRIVATE_DATA_MAX) {
					fifo_in(pdata, (char *)pevent,
					(unsigned int)(sizeof(struct context_event_t) + pevent->buf_len), 1);
				} else {
					ctxhub_err("hismart:[%s]:line[%d] type[%d]len[%d] private data too large\n", __func__, __LINE__,
						   pevent->event_type, pevent->buf_len);
					break;
				}

				if (!pevent->event_type)
					ar_port->flushing &= ~FLP_AR_DATA;
			}

			pcd += sizeof(struct context_event_t) + pevent->buf_len;
		}
		if (ar_data_report(pdata, ar_port) != 0)
			ctxhub_err("hismart:line[%s] num[%d] ar_data_report err.\n", __func__, __LINE__);
	}
#ifndef CONFIG_EXT_INPUTHUB
	mutex_unlock(&g_ar_dev.lock);
#endif

	return ret;
}

static void ar_work_handler(struct work_struct *work)
{
	int ret;
	struct ar_report_work *w = NULL;

	if (!work) {
		ctxhub_err("[%s] work is NULL\n", __func__);
		return;
	}

	w = container_of(work, struct ar_report_work, worker);
	if (!w || !w->data) {
		ctxhub_err("[%s] w or data is NULL\n", __func__);
		return;
	}

	if (w->wh) {
		ret = w->wh((const struct pkt_header *)w->data);
		if (ret < 0)
			ctxhub_warn("AR[%s]handle report data fail\n", __func__);
	} else {
		ctxhub_err("AR[%s]work handler is null\n", __func__);
	}

	kfree(w->data);
	kfree(w);
}

static int get_data_from_mcu(const struct pkt_header *head, work_handler wh)
{
	size_t len;
	struct ar_report_work *wk = NULL;

	if (!head || !wh) {
		ctxhub_err("[%s] head or work handler is NULL\n", __func__);
		return -EINVAL;
	}

	len = head->length + sizeof(struct pkt_header);
	if (len >= AR_MAX_GET_DATA) {
		ctxhub_err("[%s] date len too big, len:%zu\n", __func__, len);
		return -EINVAL;
	}

	wk = kzalloc(sizeof(struct ar_report_work), GFP_KERNEL);
	if (!wk) {
		ctxhub_err("[%s] alloc work error is err.\n", __func__);
		return -ENOMEM;
	}

	wk->data = kzalloc(len, GFP_KERNEL);
	if (!wk->data) {
		kfree(wk);
		ctxhub_err("[%s] kzalloc wk data error\n", __func__);
		return -ENOMEM;
	}

	if (memcpy_s(wk->data, len, head, len) != EOK) {
		kfree(wk->data);
		kfree(wk);
		ctxhub_err("%s memcpy_s error\n", __func__);
		return -EFAULT;
	}

	wk->wh = wh;

	INIT_WORK(&wk->worker, ar_work_handler);
	queue_work(g_ar_dev.wq, &wk->worker);
	return 0;
}

static int process_ar_data_from_mcu(const struct pkt_header *head)
{
	return get_data_from_mcu(head, get_ar_data_from_mcu);
}

int ar_state_shmem(const struct pkt_header *head)
{
	int ret;
	struct ar_state_t *staus;

	staus = (struct ar_state_t *)(head + 1);

	if (head->length < (uint16_t)sizeof(unsigned int) || (head->length > sizeof(struct ar_state_shmen_t))) {
		ctxhub_err("hismart:[%s]:line[%d] get state len is invalid[%d]\n", __func__, __LINE__, (int)head->length);
		goto STATE_SHMEM_ERROR;
	}

	if (staus->context_num == 0 || GET_STATE_NUM_MAX < staus->context_num) {
		ctxhub_err("hismart:[%s]:line[%d] context_num err[%d]\n", __func__, __LINE__, staus->context_num);
		goto STATE_SHMEM_ERROR;
	}

	g_ar_dev.activity_shemem.user_len = (unsigned int)(head->length - sizeof(unsigned int));
	ret = memcpy_s(g_ar_dev.activity_shemem.context_event, CONTEXT_TYPE_MAX * sizeof(struct context_event_t),
		       staus->context_event, g_ar_dev.activity_shemem.user_len);
	if (ret)
		ctxhub_err("hismart:[%s] memcpy_s fail\n", __func__);

STATE_SHMEM_ERROR:
	complete(&g_ar_dev.get_complete);
	return 0;
}

/*lint +e826*/
/*lint -e737*/
/*lint +e737*/
/*
 * Send get the AR status message to contexthub, and return the status reported by AR to HAL
 */
static int ar_state_v2_cmd(const struct ar_port_t *ar_port, unsigned long arg)
{
	int ret = 0;

	mutex_lock(&g_ar_dev.state_lock);
	if (!(FLP_AR_DATA & ar_port->channel_type) || !(FLP_AR_DATA & g_ar_dev.service_type)) {
		ctxhub_err("hismart:[%s]:line[%d] channel[0x%x] service[0x%x]\n", __func__, __LINE__,
			   ar_port->channel_type, g_ar_dev.service_type);
		goto AR_STATE_V2_ERR;
	}

	(void)memset_s(&g_ar_dev.activity_shemem, sizeof(struct ar_state_shmen_t), 0, sizeof(struct ar_state_shmen_t));
	reinit_completion(&g_ar_dev.get_complete);

	ret = ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ,
		SUB_CMD_FLP_AR_GET_STATE_REQ, NULL, (size_t)0);
	if (ret) {
		ctxhub_err("hismart:[%s]:line[%d] ar_send_cmd_from_kernel ERR[%d]\n", __func__, __LINE__, ret);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}

	if (!wait_for_completion_timeout(&g_ar_dev.get_complete, msecs_to_jiffies(2000))) { /* wait 2000 msecs */
		ctxhub_err("hismart:[%s]:line[%d] Get state timeout\n", __func__, __LINE__);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}

	if (g_ar_dev.activity_shemem.user_len > (unsigned int)(sizeof(struct ar_state_shmen_t) - sizeof(int))) {
		ctxhub_err("hismart:[%s]:line[%d] get len[%d] ERR\n", __func__, __LINE__, (int)g_ar_dev.activity_shemem.user_len);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}
	if (copy_to_user((void *)((uintptr_t)arg), g_ar_dev.activity_shemem.context_event,
	    g_ar_dev.activity_shemem.user_len)) {
		ctxhub_err("hismart:[%s]:line[%d] [STATE]copy_to_user ERR\n", __func__, __LINE__);
		ret = -EFAULT;
		goto AR_STATE_V2_ERR;
	}

	ret = (int)g_ar_dev.activity_shemem.user_len;
AR_STATE_V2_ERR:
	mutex_unlock(&g_ar_dev.state_lock);
	return ret;
}

/*
 * Send flush AR message to contexthub
 */
static int ar_flush_cmd(struct ar_port_t *ar_port)
{
	int ret = 0;

	mutex_lock(&g_ar_dev.lock);
	if (!(FLP_AR_DATA & ar_port->channel_type) || !(FLP_AR_DATA & g_ar_dev.service_type)) {
		ctxhub_err("hismart:[%s]:line[%d] activity has not start channel[0x%x] service[0x%x]\n", __func__,
			   __LINE__, ar_port->channel_type, g_ar_dev.service_type);
		ret = -EPERM;
		goto AR_FLUSH_ERR;
	}

	ret = ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ,
		SUB_CMD_FLP_AR_FLUSH_REQ, NULL, (size_t)0);
	if (ret) {
		ctxhub_err("hismart:[%s]:line[%d] ar_send_cmd_from_kernel ERR[%d]\n", __func__, __LINE__, ret);
		ret = -EFAULT;
		goto AR_FLUSH_ERR;
	}

	ar_port->flushing |= FLP_AR_DATA;
AR_FLUSH_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

/*
 * AR command verification
 */
static bool ar_check_cmd(unsigned int cmd, int type)
{
	switch (type) {
	case FLP_AR_DATA:
		if (((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_GPS) ||
		((cmd & FLP_IOCTL_TAG_MASK) == FLP_IOCTL_TAG_AR))
			return true;
		break;
	default:
		break;
	}
	return 0;
}

/*lint -e845*/
/*
 * AR ioctl message processing
 */
static int flp_ar_ioctl(struct ar_port_t *ar_port, unsigned int cmd, unsigned long arg)
{
	int ret;

	switch (cmd & FLP_IOCTL_CMD_MASK) {
	case flp_ioctl_ar_config(0):
		ret =  ar_config_cmd(ar_port, cmd, arg);
		break;
	case flp_ioctl_ar_stop(0):
		ret = ar_stop_cmd(ar_port, arg);
		break;
	case flp_ioctl_ar_flush(0):
		ret = ar_flush_cmd(ar_port);
		break;
	case flp_ioctl_ar_state_v2(0):
		ret = ar_state_v2_cmd(ar_port, arg);
		break;
	default:
		ctxhub_err("hismart:[%s]:line[%d] flp_ar_ioctrl input cmd[0x%x] error\n", __func__, __LINE__, cmd);
		return -EFAULT;
	}
	return ret;
}

/*
 * Send start the AR service message to contexthub
 */
static int ar_common_ioctl_open_service(void)
{
	struct read_info rd;
	int ret = 0;

	(void)memset_s((void *)&rd, sizeof(struct read_info), 0, sizeof(struct read_info));
	ctxhub_info("hismart:[%s]:line[%d] service[%x] AR user cnt[%d]\n", __func__, __LINE__,
		    g_ar_dev.service_type, g_ar_dev.ar_devinfo.usr_cnt);
	if ((g_ar_dev.service_type & FLP_AR_DATA) && g_ar_dev.ar_devinfo.usr_cnt) {
		ret = ar_send_cmd_from_kernel_response(TAG_AR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0, &rd);
		if (ret == 0 && g_ar_dev.ar_devinfo.cfg_sz && g_ar_dev.ar_devinfo.cfg.context_num)
			ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
			(char *)&g_ar_dev.ar_devinfo.cfg, (size_t)g_ar_dev.ar_devinfo.cfg_sz);
	}

	return ret;
}
/*
 * Send close the AR service message to contexthub
 */
static int ar_common_ioctl_close_service(void)
{
	unsigned int data = 0;

	if ((g_ar_dev.service_type & FLP_AR_DATA) && (g_ar_dev.ar_devinfo.usr_cnt)) {
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_STOP_REQ,
			(char *)&data, sizeof(int));
		ar_send_cmd_from_kernel(TAG_AR, CMD_CMN_CLOSE_REQ, 0, NULL, (size_t)0);
	}
	return 0;
}
/*lint -e455*/
/*
 * AR COMMON ioctl message process
 */
static int ar_common_ioctl(struct ar_port_t *ar_port, unsigned int cmd, unsigned long arg)
{
	unsigned int data = 0;

	if (cmd != FLP_IOCTL_COMMON_RELEASE_WAKELOCK) {
		if (copy_from_user(&data, (void *)((uintptr_t)arg), sizeof(unsigned int))) {
			ctxhub_err("hismart:[%s]:line[%d]\n", __func__, __LINE__);
			return -EFAULT;
		}
	}

	switch (cmd) {
	case FLP_IOCTL_COMMON_AWAKE_RET:
		mutex_lock(&g_ar_dev.lock);
		ar_port->need_awake = data;
		mutex_unlock(&g_ar_dev.lock);
		break;
	case FLP_IOCTL_COMMON_SETPID:
		mutex_lock(&g_ar_dev.lock);
		ar_port->portid = data;
		mutex_unlock(&g_ar_dev.lock);
		break;
	case FLP_IOCTL_COMMON_CLOSE_SERVICE:
		mutex_lock(&g_ar_dev.lock);
		g_ar_dev.denial_sevice = data;
		if (g_ar_dev.denial_sevice)
			ar_common_ioctl_close_service();
		else
			ar_common_ioctl_open_service();

		mutex_unlock(&g_ar_dev.lock);
		break;
	case FLP_IOCTL_COMMON_HOLD_WAKELOCK:
		mutex_lock(&g_ar_dev.lock);
		ar_port->need_hold_wlock = data;
		mutex_unlock(&g_ar_dev.lock);
		break;
	case FLP_IOCTL_COMMON_RELEASE_WAKELOCK:
		mutex_lock(&g_ar_dev.lock);
		if (ar_port->need_hold_wlock)
			__pm_relax(ar_port->wlock);
		mutex_unlock(&g_ar_dev.lock);
		break;
	default:
		ctxhub_err("hismart:[%s]:line[%d] ar common ioctl input cmd[0x%x] error\n", __func__, __LINE__, cmd);
		return -EFAULT;
	}
	return 0;
}
/*lint +e455*/
/*
 * AR ioctl message process
 */
static long ar_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	struct ar_port_t *ar_port = NULL;

	if (file == NULL) {
		ctxhub_err("hismart: flp_ioctl file is null\n");
		return -EINVAL;
	}
	ar_port  = (struct ar_port_t *)file->private_data;

	if (ar_port == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] flp_ioctl parameter error\n", __func__, __LINE__);
		return -EINVAL;
	}
	ctxhub_info("hismart:[%s]:line[%d] cmd[0x%x]\n\n", __func__, __LINE__, cmd & 0x0FFFF);
	mutex_lock(&g_ar_dev.lock);
	if ((g_ar_dev.denial_sevice) && (cmd != FLP_IOCTL_COMMON_CLOSE_SERVICE)) {
		mutex_unlock(&g_ar_dev.lock);
		return 0;
	}
	mutex_unlock(&g_ar_dev.lock);

	switch (cmd & FLP_IOCTL_TYPE_MASK) {
	case FLP_IOCTL_TYPE_AR:
		if (!ar_check_cmd(cmd, FLP_AR_DATA))
			return -EPERM;
		return (long)flp_ar_ioctl(ar_port, cmd, arg);
	case FLP_IOCTL_TYPE_COMMON:
		return (long)ar_common_ioctl(ar_port, cmd, arg);
	default:
		ctxhub_err("hismart:[%s]:line[%d] flp_ioctl input cmd[0x%x] error\n", __func__, __LINE__, cmd);
		return -EFAULT;
	}
}

/*
 * 32K
 */

#if defined(CONFIG_COMPAT) && defined(CONFIG_EXT_INPUTHUB)
static long ar_compat_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	return ar_ioctl(file, cmd, (unsigned long)compat_ptr(arg));
}
#endif

/*
 * contexthub application recovery
 */
static void  ar_service_recovery(void)
{
	if (g_ar_dev.service_type & FLP_AR_DATA) {
		ar_send_cmd_from_kernel_nolock(TAG_AR, CMD_CMN_OPEN_REQ, 0, NULL, (size_t)0);
		ar_send_cmd_from_kernel_nolock(TAG_AR, CMD_CMN_CONFIG_REQ, SUB_CMD_FLP_AR_START_REQ,
		(char *)&g_ar_dev.ar_devinfo.cfg, (size_t)g_ar_dev.ar_devinfo.cfg_sz);
	}

	ctxhub_info("hismart:[%s]:line[%d] service_type[%d]\n", __func__, __LINE__, g_ar_dev.service_type);
}
/*lint -e715*/
/*
 * contexthub reset message recall
 */
static int ar_notifier(struct notifier_block *nb,
			unsigned long action, void *data)
{
	switch (action) {
	case IOM3_RECOVERY_3RD_DOING:
		ar_service_recovery();
		break;
	default:
		break;
	}
	return 0;
}
/*lint +e715*/
extern int register_iom3_recovery_notifier(struct notifier_block *nb);

static struct notifier_block ar_reboot_notify = {
	.notifier_call = ar_notifier,
	.priority = -1,
};

static void ar_timerout_work(struct work_struct *wk)
{
	unsigned char *data = NULL;
	struct sk_buff *skb = NULL;
	struct ar_port_t *ar_port  = container_of(wk, struct ar_port_t, work);

	ctxhub_info("hismart:[%s]:line[%d]\n", __func__, __LINE__);
	mutex_lock(&g_ar_dev.lock);
	skb = context_gnetlink_alloc(ar_port, 0, ar_port->work_para, &data);
	if (skb != NULL)
		context_gnetlink_send(ar_port, skb);
	mutex_unlock(&g_ar_dev.lock);
}

static void ar_sleep_timeout(unsigned long data)
{
	struct ar_port_t *ar_port = (struct ar_port_t *)((uintptr_t)data);

	ctxhub_info("hismart:[%s]:line[%d]\n", __func__, __LINE__);
	if (ar_port != NULL) {
		ar_port->work_para = AR_GENL_CMD_NOTIFY_TIMEROUT;
		if (ar_port->portid)
			queue_work(system_power_efficient_wq, &ar_port->work);
		else
			ctxhub_info("hismart:[%s]:line[%d] not set pid, timer out but not report\n", __func__, __LINE__);

		if (ar_port->need_hold_wlock) {
			ctxhub_info("hismart:[%s]:line[%d] wake_lock_timeout 2\n", __func__, __LINE__);
			__pm_wakeup_event(ar_port->wlock, jiffies_to_msecs(2 * HZ)); /* wake_lock_timeout 2 */
		}
	}
}

/*
 * Device node open function
 */
static int ar_open(struct inode *inode, struct file *filp)/*lint -e715*/
{
	int ret = 0;
	struct ar_port_t *ar_port = NULL;
	struct list_head *pos = NULL;
	int count = 0;

	if (filp == NULL) {
		ctxhub_err("hismart: ar_open filp is null\n");
		return -EACCES;
	}
	mutex_lock(&g_ar_dev.lock);
	list_for_each(pos, &g_ar_dev.list) {
		count++;
	}

	if (count > AR_OPEN_CLINET_LIMIT) {
		ctxhub_err("hismart:[%s]:line[%d] ar open clinet limit\n", __func__, __LINE__);
		ret = -EACCES;
		goto ar_open_ERR;
	}

	ar_port = kzalloc(sizeof(struct ar_port_t), GFP_KERNEL);
	if (ar_port == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] no mem\n", __func__, __LINE__);
		ret = -ENOMEM;
		goto ar_open_ERR;
	}

	ret = data_buffer_init(&ar_port->ar_buf,
			(unsigned int)(AR_STATE_BUTT * (sizeof(struct context_event_t) + CONTEXT_PRIVATE_DATA_MAX)));
	if (ret != 0) {
		ctxhub_err("hismart:[%s]:line[%d] data_buffer_init err[%d]\n", __func__, __LINE__, ret);
		ret = -ENOMEM;
		goto ar_buffer_ERR;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5, 4, 0))
	ar_port->wlock = wakeup_source_register(NULL, "smart_ar");
#else
	ar_port->wlock = wakeup_source_register("smart_ar");
#endif
	if (ar_port->wlock == NULL) {
		ctxhub_err("%s wakeup source register failed\n", __func__);
		goto ar_wakeup_register_ERR;
	}

	INIT_LIST_HEAD(&ar_port->list);
	smart_softtimer_create(&ar_port->sleep_timer, ar_sleep_timeout, (unsigned long)((uintptr_t)ar_port), 0);
	INIT_WORK(&ar_port->work, ar_timerout_work);

	list_add_tail(&ar_port->list, &g_ar_dev.list);

	filp->private_data = ar_port;
	ctxhub_info("hismart:[%s]:line[%d] v1.4 enter\n", __func__, __LINE__);

	mutex_unlock(&g_ar_dev.lock);
	return ret;

ar_wakeup_register_ERR:
	data_buffer_exit(&ar_port->ar_buf);
ar_buffer_ERR:
	kfree(ar_port);
ar_open_ERR:
	mutex_unlock(&g_ar_dev.lock);
	return ret;
}

static bool is_ar_read_check_ok(char __user *buf, size_t count)
{
	if (buf == NULL) {
		ctxhub_err("%s, input is null\n", __func__);
		return false;
	}

	if ((int)count < g_support_activities_buf_len) {
		ctxhub_err("%s, input buf not enough\n", __func__);
		return false;
	}
	return true;
}

static ssize_t ar_read(struct file *file, char __user *buf, size_t count, loff_t *pos)
{
	ctxhub_info("%s, in\n", __func__);
	if (!is_ar_read_check_ok(buf, count))
		return 0;

	if (copy_to_user(buf, g_support_activities_buf, g_support_activities_buf_len) != 0) {
		ctxhub_err("%s, copy supported activities failed\n", __func__);
		return 0;
	}

	ctxhub_info("%s, out\n", __func__);
	return g_support_activities_buf_len;
}

/*lint -e438*/
/*
 * Device node close function
 */
static int ar_release(struct inode *inode, struct file *file)/*lint -e715*/
{
	struct ar_port_t *ar_port = NULL;
	struct list_head    *pos = NULL;
	struct ar_port_t      *port = NULL;
	int ret;

	if (file == NULL) {
		ctxhub_err("hismart: ar_release file is null\n");
		return -EINVAL;
	}
	ar_port = (struct ar_port_t *)file->private_data;
	ctxhub_info("hismart:[%s]:line[%d]\n", __func__, __LINE__);
	if (ar_port == NULL) {
		ctxhub_err("hismart:[%s]:line[%d] flp_close parameter error\n", __func__, __LINE__);
		return -EINVAL;
	}

	smart_softtimer_delete(&ar_port->sleep_timer);
	cancel_work_sync(&ar_port->work);
	wakeup_source_unregister(ar_port->wlock);
	ar_port->wlock = NULL;

	mutex_lock(&g_ar_dev.lock);
	list_del(&ar_port->list);
/* if andriod vm restart, apk doesnot send stop cmd,just adjust it */
	g_ar_dev.ar_devinfo.usr_cnt = 0;
	list_for_each(pos, &g_ar_dev.list) {
		port = container_of(pos, struct ar_port_t, list);
		if (port->channel_type & FLP_AR_DATA)
			g_ar_dev.ar_devinfo.usr_cnt++;
	}
	ret = ar_stop(0);
	if (ret)
		ctxhub_err("hismart:[%s]:line[%d] ar_stop ret [%d] error\n", __func__, __LINE__, ret);

	data_buffer_exit(&ar_port->ar_buf);
	kfree(ar_port);
	file->private_data = NULL;
	mutex_unlock(&g_ar_dev.lock);
	return 0;
}
/*lint -e785*/
static const struct file_operations ar_fops = {
	.owner =          THIS_MODULE,
	.llseek =         no_llseek,
	.unlocked_ioctl = ar_ioctl,
	.open       =     ar_open,
	.release    =     ar_release,
#if defined(CONFIG_COMPAT) && defined(CONFIG_EXT_INPUTHUB)
	.compat_ioctl  =  ar_compat_ioctl,
#endif
	.read = ar_read,
};

static struct miscdevice ar_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "ar",
	.fops =     &ar_fops,
};
/*lint +e785*/
/*lint -e826*/
/*lint !e715*/
/*lint +e826*/
static void get_contexthub_dts(void)
{
	g_dts_status = DTS_DISABLED;
	if (get_contexthub_dts_status() == 0)
		g_dts_status = SENSORHUB_DTS_ENABLED;
	else if (get_ext_contexthub_dts_status() == 0)
		g_dts_status = EXT_SENSORHUB_DTS_ENABLED;
}

static void get_support_activity_from_dts(void)
{
	struct device_node *ar_support_node = NULL;
	int dts_support_num = 0;
	int actual_support_num = 0;
	int i;
	int offset = sizeof(int); // buf head is a int, save string num.
	const char *support_activity = NULL;

	ar_support_node = of_find_compatible_node(NULL, NULL, AR_COMPATIBLE_NAME);
	if (ar_support_node == NULL) {
		ctxhub_err("find ar node failed\n");
		goto SET_SUPPORT_NUM;
	}

	if (of_property_read_s32(ar_support_node, "ar_support_num", &dts_support_num) != 0) {
		ctxhub_err("%s, read support num failed!\n", __func__);
		goto SET_SUPPORT_NUM;
	}

	if (dts_support_num < 0 || dts_support_num >= AR_STATE_BUTT) {
		ctxhub_err("support num %d invalid\n", dts_support_num);
		goto SET_SUPPORT_NUM;
	}

	for (i = 0; i < dts_support_num; i++) {
		if (of_property_read_string_index(ar_support_node, "ar_support_list", i, &support_activity) != 0) {
			ctxhub_err("support index %d invalid\n", i);
			continue;
		}

		if ((unsigned int)offset + strlen(support_activity) + 1 > AR_MAX_BUF_LEN) {
			ctxhub_err("i:%d, offset:%d + %u + 1 > 1024 err!\n", i, offset, (unsigned int)strlen(support_activity));
			break;
		}
		if (memcpy_s(g_support_activities_buf + offset, AR_MAX_BUF_LEN - offset,
					support_activity, strlen(support_activity) + 1) != 0) { // 1 is '\0'
			ctxhub_err("%s, memcpy failed!\n", __func__);
			continue;
		}

		actual_support_num++;
		offset += (int)(strlen(support_activity) + 1);
	}

SET_SUPPORT_NUM:
	ctxhub_info("%s, get ar support num %d", __func__, actual_support_num);
	*((int *)g_support_activities_buf) = actual_support_num;
	g_support_activities_buf_len = offset;
}

/*
 * init AR driver
 */
static int __init ar_init(void)
{
	int ret = 0;

	get_contexthub_dts();
	ctxhub_info("%s :line[%d] contexthub g_dts status is [%d]\n ", __func__, __LINE__, g_dts_status);
	if (g_dts_status == DTS_DISABLED)
		return ret;
	(void)memset_s((void *)&g_ar_dev, sizeof(g_ar_dev), 0, sizeof(g_ar_dev));

	ret = genl_register_family(&ar_genl_family);
	if (ret) {
		ctxhub_err("hismart:[%s]:line[%d] genl_register_family err[%d]\n", __func__, __LINE__, ret);
		return ret;
	}

	g_ar_dev.wq = create_freezable_workqueue("ar_wq");
	if (!g_ar_dev.wq) {
		ctxhub_err("[%s] create workqueue fail!\n", __func__);
		ret = -EFAULT;
		goto AR_PROBE_GEN;
	}

	ret = misc_register(&ar_miscdev);
	if (ret) {
		ctxhub_err("hismart:[%s]:line[%d] cannot register smart ar ret[%d]\n", __func__, __LINE__, ret);
		goto DESTROY_WQ;
	}

	get_support_activity_from_dts();
	INIT_LIST_HEAD(&g_ar_dev.list);
	mutex_init(&g_ar_dev.lock);
	mutex_init(&g_ar_dev.state_lock);
	init_completion(&g_ar_dev.get_complete);
#ifdef CONFIG_EXT_INPUTHUB
	if (g_dts_status == EXT_SENSORHUB_DTS_ENABLED)
		register_data_callback_from_1135();
#endif
	if (g_dts_status == SENSORHUB_DTS_ENABLED) {
		register_mcu_event_notifier(TAG_AR, CMD_DATA_REQ, process_ar_data_from_mcu);
		register_mcu_event_notifier(TAG_AR, SUB_CMD_FLP_AR_SHMEM_STATE_REQ, ar_state_shmem);
		register_iom3_recovery_notifier(&ar_reboot_notify);
	}
	return ret;

DESTROY_WQ:
	destroy_workqueue(g_ar_dev.wq);

AR_PROBE_GEN:
	genl_unregister_family(&ar_genl_family);
	return ret;
}

/*
 * release AR driver
 */
static void ar_exit(void)
{
	int ret = 0;

	destroy_workqueue(g_ar_dev.wq);
	misc_deregister(&ar_miscdev);
#ifdef CONFIG_EXT_INPUTHUB
	if (g_dts_status == EXT_SENSORHUB_DTS_ENABLED)
		unregister_data_callback_from_1135();
#endif
	if (g_dts_status == SENSORHUB_DTS_ENABLED) {
		ret += unregister_mcu_event_notifier(TAG_AR, CMD_DATA_REQ, process_ar_data_from_mcu);
		ret += unregister_mcu_event_notifier(TAG_AR, SUB_CMD_FLP_AR_SHMEM_STATE_REQ, ar_state_shmem);
	}
	if (ret)
		ctxhub_err("hismart:[%s]:line[%d] ret[%d]err\n", __func__, __LINE__, ret);
}
/*lint +e64*/

static int ar_pm_suspend(struct device *dev)
{
	return 0;
}

/*
 * driver resume callback
 */
static int ar_pm_resume(struct device *dev)
{
	struct list_head    *pos = NULL;
	struct ar_port_t      *ar_port = NULL;
	unsigned int cnt = 0;

	if (g_dts_status == DTS_DISABLED)
		return 0;

	mutex_lock(&g_ar_dev.lock);
	ctxhub_info("hismart:[%s]:line[%d] in\n", __func__, __LINE__);
	list_for_each(pos, &g_ar_dev.list) {
		ar_port = container_of(pos, struct ar_port_t, list);
		if (ar_port->need_awake) {
			ar_port->work_para = AR_GENL_CMD_AWAKE_RET;
			queue_work(system_power_efficient_wq, &ar_port->work);
			cnt++;
		}
	}
	ctxhub_info("hismart:[%s]:line[%d] cnt[%d]out\n", __func__, __LINE__, cnt);
	mutex_unlock(&g_ar_dev.lock);
	return 0;
}
/*lint -e785*/
const struct dev_pm_ops ar_pm_ops = {
#ifdef CONFIG_PM_SLEEP
	.suspend = ar_pm_suspend,
	.resume  = ar_pm_resume,
#endif
};

static int generic_ar_probe(struct platform_device *pdev)
{
	return ar_init();
}

static int generic_ar_remove(struct platform_device *pdev)
{
	ar_exit();
	return 0;
}

static const struct of_device_id generic_ar[] = {
	{ .compatible = AR_COMPATIBLE_NAME },
	{},
};
MODULE_DEVICE_TABLE(of, generic_ar);
/*lint -e64*/
static struct platform_driver generic_ar_platdrv = {
	.driver = {
		.name	= "smart-ar",
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(generic_ar),
		.pm = &ar_pm_ops,
	},
	.probe = generic_ar_probe,
	.remove  = generic_ar_remove,
};
/*lint +e64*/
/*lint +e785*/
/*lint -e64*/
static int __init smart_ar_init(void)
{
	return platform_driver_register(&generic_ar_platdrv);
}
/*lint +e64*/
static void __exit smart_ar_exit(void)
{
	platform_driver_unregister(&generic_ar_platdrv);
}
/*lint -e528 -esym(528,*)*/
late_initcall_sync(smart_ar_init);
module_exit(smart_ar_exit);
