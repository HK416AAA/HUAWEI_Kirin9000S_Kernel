// SPDX-License-Identifier: GPL-2.0-only
/*
 * Debugfs support for hosts and cards
 *
 * Copyright (C) 2008 Atmel Corporation
 */
#include <linux/moduleparam.h>
#include <linux/export.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/stat.h>
#include <linux/fault-inject.h>

#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <platform_include/basicplatform/linux/mmc/dw_mmc.h>
#include <linux/mmc/sdhci_zodiac_mmc.h>

#include "core.h"
#include "card.h"
#include "host.h"
#include "mmc_ops.h"
#include "../host/dw_mmc.h"
#include "../host/sdhci.h"
#include "../host/sdhci-pltfm.h"

#ifdef CONFIG_EMMC_FAULT_INJECT
#include <platform_include/basicplatform/linux/mmc/emmc_fault_inject.h>
#endif

#ifdef CONFIG_FAIL_MMC_REQUEST

static DECLARE_FAULT_ATTR(fail_default_attr);
static char *fail_request;
module_param(fail_request, charp, 0);

#endif /* CONFIG_FAIL_MMC_REQUEST */
/* Enum of power state */
enum sd_type {
	SDHC = 0,
	SDXC,
};

/* The debugfs functions are optimized away when CONFIG_DEBUG_FS isn't set. */
static int mmc_ios_show(struct seq_file *s, void *data)
{
	static const char *vdd_str[] = {
		[8]	= "2.0",
		[9]	= "2.1",
		[10]	= "2.2",
		[11]	= "2.3",
		[12]	= "2.4",
		[13]	= "2.5",
		[14]	= "2.6",
		[15]	= "2.7",
		[16]	= "2.8",
		[17]	= "2.9",
		[18]	= "3.0",
		[19]	= "3.1",
		[20]	= "3.2",
		[21]	= "3.3",
		[22]	= "3.4",
		[23]	= "3.5",
		[24]	= "3.6",
	};
	struct mmc_host	*host = s->private;
	struct mmc_ios	*ios = &host->ios;
	const char *str;

	seq_printf(s, "clock:\t\t%u Hz\n", ios->clock);
	if (host->actual_clock)
		seq_printf(s, "actual clock:\t%u Hz\n", host->actual_clock);
	seq_printf(s, "vdd:\t\t%u ", ios->vdd);
	if ((1 << ios->vdd) & MMC_VDD_165_195)
		seq_printf(s, "(1.65 - 1.95 V)\n");
	else if (ios->vdd < (ARRAY_SIZE(vdd_str) - 1)
			&& vdd_str[ios->vdd] && vdd_str[ios->vdd + 1])
		seq_printf(s, "(%s ~ %s V)\n", vdd_str[ios->vdd],
				vdd_str[ios->vdd + 1]);
	else
		seq_printf(s, "(invalid)\n");

	switch (ios->bus_mode) {
	case MMC_BUSMODE_OPENDRAIN:
		str = "open drain";
		break;
	case MMC_BUSMODE_PUSHPULL:
		str = "push-pull";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "bus mode:\t%u (%s)\n", ios->bus_mode, str);

	switch (ios->chip_select) {
	case MMC_CS_DONTCARE:
		str = "don't care";
		break;
	case MMC_CS_HIGH:
		str = "active high";
		break;
	case MMC_CS_LOW:
		str = "active low";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "chip select:\t%u (%s)\n", ios->chip_select, str);

	switch (ios->power_mode) {
	case MMC_POWER_OFF:
		str = "off";
		break;
	case MMC_POWER_UP:
		str = "up";
		break;
	case MMC_POWER_ON:
		str = "on";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "power mode:\t%u (%s)\n", ios->power_mode, str);
	seq_printf(s, "bus width:\t%u (%u bits)\n",
			ios->bus_width, 1 << ios->bus_width);

	switch (ios->timing) {
	case MMC_TIMING_LEGACY:
		str = "legacy";
		break;
	case MMC_TIMING_MMC_HS:
		str = "mmc high-speed";
		break;
	case MMC_TIMING_SD_HS:
		str = "sd high-speed";
		break;
	case MMC_TIMING_UHS_SDR12:
		str = "sd uhs SDR12";
		break;
	case MMC_TIMING_UHS_SDR25:
		str = "sd uhs SDR25";
		break;
	case MMC_TIMING_UHS_SDR50:
		str = "sd uhs SDR50";
		break;
	case MMC_TIMING_UHS_SDR104:
		str = "sd uhs SDR104";
		break;
	case MMC_TIMING_UHS_DDR50:
		str = "sd uhs DDR50";
		break;
	case MMC_TIMING_MMC_DDR52:
		str = "mmc DDR52";
		break;
	case MMC_TIMING_MMC_HS200:
		str = "mmc HS200";
		break;
	case MMC_TIMING_MMC_HS400:
		str = mmc_card_hs400es(host->card) ?
			"mmc HS400 enhanced strobe" : "mmc HS400";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "timing spec:\t%u (%s)\n", ios->timing, str);

	switch (ios->signal_voltage) {
	case MMC_SIGNAL_VOLTAGE_330:
		str = "3.30 V";
		break;
	case MMC_SIGNAL_VOLTAGE_180:
		str = "1.80 V";
		break;
	case MMC_SIGNAL_VOLTAGE_120:
		str = "1.20 V";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "signal voltage:\t%u (%s)\n", ios->signal_voltage, str);

	switch (ios->drv_type) {
	case MMC_SET_DRIVER_TYPE_A:
		str = "driver type A";
		break;
	case MMC_SET_DRIVER_TYPE_B:
		str = "driver type B";
		break;
	case MMC_SET_DRIVER_TYPE_C:
		str = "driver type C";
		break;
	case MMC_SET_DRIVER_TYPE_D:
		str = "driver type D";
		break;
	default:
		str = "invalid";
		break;
	}
	seq_printf(s, "driver type:\t%u (%s)\n", ios->drv_type, str);

	return 0;
}
DEFINE_SHOW_ATTRIBUTE(mmc_ios);

static int mmc_clock_opt_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	*val = host->ios.clock;

	return 0;
}

static int mmc_clock_opt_set(void *data, u64 val)
{
	struct mmc_host *host = data;

	/* We need this check due to input value is u64 */
	if (val != 0 && (val > host->f_max || val < host->f_min))
		return -EINVAL;

	mmc_claim_host(host);
	mmc_set_clock(host, (unsigned int) val);
	mmc_release_host(host);

	return 0;
}

DEFINE_DEBUGFS_ATTRIBUTE(mmc_clock_fops, mmc_clock_opt_get, mmc_clock_opt_set,
	"%llu\n");

static int mmc_sdxc_opt_get(void *data, u64 *val)
{
	struct mmc_card	*card = data;

	if (mmc_card_ext_capacity(card)) {
		*val = SDXC;
		printk(KERN_INFO "sd card SDXC type is detected\n");
		return 0;
	}
	*val = SDHC;
	printk(KERN_INFO "sd card SDHC type is detected\n");

	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(mmc_sdxc_fops, mmc_sdxc_opt_get,
			NULL, "%llu\n");

#ifdef CONFIG_DFX_DEBUG_FS
static int mmc_sim_remove_sd_get(void *data, u64 *val)
{
	struct mmc_host *host = data;

	pr_err("%s %d sim_remove_sd: %lu\n",
			__func__, __LINE__, host->sim_remove_sd);
	*val = host->sim_remove_sd;

	return 0;
}

extern int mmc_schedule_delayed_work(struct delayed_work *work,
						 unsigned long delay);
static int mmc_sim_remove_sd_set(void *data, u64 val)
{
	struct mmc_host *host = data;

	if (val == host->sim_remove_sd) {
		pr_err("%s %d Nothing changed!\n", __func__, __LINE__);
		return 0;
	}

	if (!host->card || mmc_card_sd(host->card)) {
		host->sim_remove_sd = val;
		mmc_schedule_delayed_work(&host->detect, 0);
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(sim_remove_sd_fops, mmc_sim_remove_sd_get,
			mmc_sim_remove_sd_set, "%llu\n");

static int mmc_nano_sd_remove_get(void *data, u64 *val)
{
	struct mmc_host *mmc = data;

	pr_err("%s %d sim_remove_nano: %lu\n",
			__func__, __LINE__, mmc->sim_remove_nano);
	*val = mmc->sim_remove_nano;

	return 0;
}

static int mmc_nano_sd_remove_set(void *data, u64 val)
{
	struct mmc_host *mmc = data;
#ifdef CONFIG_MMC_SDHCI_MUX_SDSIM
	struct sdhci_host *host = mmc_priv(mmc);
	struct sdhci_pltfm_host *pltfm_host = sdhci_priv(host);
	struct sdhci_zodiac_data *sdhci_zodiac = pltfm_host->priv;
#else
	struct dw_mci_slot *slot = mmc_priv(mmc);
	struct dw_mci *dw_mci_host = slot->host;
#endif

	if (val == mmc->sim_remove_nano) {
		pr_err("%s %d Nothing changed!\n", __func__, __LINE__);
		return 0;
	}

	pr_err("%s nano sd status set val: %lu\n", __func__, val);
	if (!mmc->card || mmc_card_mmc(mmc->card)) {
		mmc->sim_remove_nano = val;
#ifdef CONFIG_MMC_SDHCI_MUX_SDSIM
		queue_work(sdhci_zodiac->card_workqueue, &sdhci_zodiac->card_work);
#else
		queue_work(dw_mci_host->card_workqueue, &dw_mci_host->card_work);
#endif
	}

	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(nano_sd_remove_fops, mmc_nano_sd_remove_get,
			mmc_nano_sd_remove_set, "%llu\n");
#endif

void mmc_add_host_debugfs(struct mmc_host *host)
{
	struct dentry *root;

	root = debugfs_create_dir(mmc_hostname(host), NULL);
	host->debugfs_root = root;

	debugfs_create_file("ios", S_IRUSR, root, host, &mmc_ios_fops);
	debugfs_create_x32("caps", S_IRUSR, root, &host->caps);
	debugfs_create_x32("caps2", S_IRUSR, root, &host->caps2);
	debugfs_create_file_unsafe("clock", S_IRUSR | S_IWUSR, root, host,
				   &mmc_clock_fops);

#ifdef CONFIG_DFX_DEBUG_FS
	if (host->index == 1) {
		debugfs_create_file_unsafe("sim_remove_sd", S_IRUSR | S_IWUSR, root, host,
				&sim_remove_sd_fops);

		debugfs_create_file_unsafe("sim_remove_nano", S_IRUSR | S_IWUSR, root, host,
				&nano_sd_remove_fops);
	}
#endif

#ifdef CONFIG_FAIL_MMC_REQUEST
	if (fail_request)
		setup_fault_attr(&fail_default_attr, fail_request);
	host->fail_mmc_request = fail_default_attr;
	fault_create_debugfs_attr("fail_mmc_request", root,
				  &host->fail_mmc_request);
#endif

#ifdef CONFIG_EMMC_FAULT_INJECT
	mmc_fault_inject_fs_setup();
#endif
}

void mmc_remove_host_debugfs(struct mmc_host *host)
{
	debugfs_remove_recursive(host->debugfs_root);
}

#ifdef CONFIG_HW_MMC_TEST
static int mmc_card_addr_open(struct inode *inode, struct file *filp)
{
	struct mmc_card *card = inode->i_private;
	filp->private_data = card;

	return 0;
}

static ssize_t mmc_card_addr_read(struct file *filp, char __user *ubuf,
				     size_t cnt, loff_t *ppos)
{
    char buf[64] = {0};
    struct mmc_card *card = filp->private_data;
    long card_addr = (long)card;

    card_addr = (long)(card_addr ^ CARD_ADDR_MAGIC);
    snprintf(buf, sizeof(buf), "%ld", card_addr);

    return simple_read_from_buffer(ubuf, cnt, ppos,
            buf, sizeof(buf));
}

static const struct file_operations mmc_dbg_card_addr_fops = {
	.open		= mmc_card_addr_open,
	.read		= mmc_card_addr_read,
    .llseek     = default_llseek,
};

static int mmc_test_st_open(struct inode *inode, struct file *filp)
{
	struct mmc_card *card = inode->i_private;

	filp->private_data = card;

	return 0;
}

static ssize_t mmc_test_st_read(struct file *filp, char __user *ubuf,
				     size_t cnt, loff_t *ppos)
{
    char buf[64] = {0};
	struct mmc_card *card = filp->private_data;

	if (!card)
		return cnt;

    snprintf(buf, sizeof(buf), "%d", card->host->test_status);

    return simple_read_from_buffer(ubuf, cnt, ppos,
            buf, sizeof(buf));

}

static ssize_t mmc_test_st_write(struct file *filp,
				      const char __user *ubuf, size_t cnt,
				      loff_t *ppos)
{
	struct mmc_card *card = filp->private_data;
	int value;

	if (!card){
        return cnt;
    }

	sscanf(ubuf, "%d", &value);
    card->host->test_status = value;

	return cnt;
}

static const struct file_operations mmc_dbg_test_st_fops = {
	.open		= mmc_test_st_open,
	.read		= mmc_test_st_read,
	.write		= mmc_test_st_write,
};
#endif

void mmc_add_card_debugfs(struct mmc_card *card)
{
	struct mmc_host	*host = card->host;
	struct dentry *root  = NULL;
	struct dentry *sdxc_root = NULL;

	if (!host->debugfs_root)
		return;

	sdxc_root = debugfs_create_dir("sdxc_root", host->debugfs_root);
	if (IS_ERR(sdxc_root))
		return;

	if (!sdxc_root)
		goto err;

	root = debugfs_create_dir(mmc_card_id(card), host->debugfs_root);
	card->debugfs_sdxc = sdxc_root;
	card->debugfs_root = root;

	debugfs_create_x32("state", S_IRUSR, root, &card->state);

#ifdef CONFIG_ZODIAC_MMC_FLUSH_REDUCE
	debugfs_create_x8("flush_reduce_en", S_IRUSR, root,
		&(card->host->mmc_flush_reduce_enable));
#endif

	if (mmc_card_sd(card))
		if (!debugfs_create_file("sdxc", S_IRUSR, sdxc_root, card,
				&mmc_sdxc_fops))
			goto err;

#ifdef CONFIG_HW_MMC_TEST
	if (mmc_card_mmc(card))
		if (!debugfs_create_file("card_addr", S_IRUSR, root, card,
				 &mmc_dbg_card_addr_fops))
			goto err;

	if (mmc_card_mmc(card))
		if (!debugfs_create_file("test_st", S_IRUSR, root, card,
				 &mmc_dbg_test_st_fops))
			goto err;
#endif

		return;

err:
	debugfs_remove_recursive(root);
	debugfs_remove_recursive(sdxc_root);
	card->debugfs_root = NULL;
	card->debugfs_sdxc = NULL;
	dev_err(&card->dev, "failed to initialize debugfs\n");
}

void mmc_remove_card_debugfs(struct mmc_card *card)
{
	debugfs_remove_recursive(card->debugfs_root);
	debugfs_remove_recursive(card->debugfs_sdxc);
	card->debugfs_root = NULL;
	card->debugfs_sdxc = NULL;
}
