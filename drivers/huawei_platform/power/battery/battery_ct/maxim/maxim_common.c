/*
 * maxim_common.c
 *
 * maxim universal function
 *
 * Copyright (c) 2012-2020 Huawei Technologies Co., Ltd.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 */

#include "maxim_common.h"

#define HWLOG_TAG HW_ONEWIRE_MAXIM
HWLOG_REGIST();

/* Never change dscrc_table for CRC8, which is maxim special. */
static const unsigned char dscrc_table[] = {
	0, 94, 188, 226, 97, 63, 221, 131,
	194, 156, 126, 32, 163, 253, 31, 65,
	157, 195, 33, 127, 252, 162, 64, 30,
	95, 1, 227, 189, 62, 96, 130, 220,
	35, 125, 159, 193, 66, 28, 254, 160,
	225, 191, 93, 3, 128, 222, 60, 98,
	190, 224, 2, 92, 223, 129, 99, 61,
	124, 34, 192, 158, 29, 67, 161, 255,
	70, 24, 250, 164, 39, 121, 155, 197,
	132, 218, 56, 102, 229, 187, 89, 7,
	219, 133, 103, 57, 186, 228, 6, 88,
	25, 71, 165, 251, 120, 38, 196, 154,
	101, 59, 217, 135, 4, 90, 184, 230,
	167, 249, 27, 69, 198, 152, 122, 36,
	248, 166, 68, 26, 153, 199, 37, 123,
	58, 100, 134, 216, 91, 5, 231, 185,
	140, 210, 48, 110, 237, 179, 81, 15,
	78, 16, 242, 172, 47, 113, 147, 205,
	17, 79, 173, 243, 112, 46, 204, 146,
	211, 141, 111, 49, 178, 236, 14, 80,
	175, 241, 19, 77, 206, 144, 114, 44,
	109, 51, 209, 143, 12, 82, 176, 238,
	50, 108, 142, 208, 83, 13, 239, 177,
	240, 174, 76, 18, 145, 207, 45, 115,
	202, 148, 118, 40, 171, 245, 23, 73,
	8, 86, 180, 234, 105, 55, 213, 139,
	87, 9, 235, 181, 54, 104, 138, 212,
	149, 203, 41, 119, 244, 170, 72, 22,
	233, 183, 85, 11, 136, 214, 52, 106,
	43, 117, 151, 201, 74, 20, 246, 168,
	116, 42, 200, 150, 21, 75, 169, 247,
	182, 232, 10, 84, 215, 137, 107, 53,
};

/* Never change oddparity for CRC16, which is maxim special. */
static const short oddparity[] = {
	0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0
};

/*
 * CRC8 Check
 * @data: data to check
 * @length: data length
 */
unsigned char check_crc8(unsigned char *data, int length)
{
	unsigned char crc8 = 0;
	int i;

	for (i = 0; i < length; i++)
		crc8 = dscrc_table[crc8 ^ data[i]];

	return crc8;
}

/*
 * CRC16 Check
 * @check_data: data to check
 * @length: data length
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * !!!!Never change digits inside this function!!!!
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 */
unsigned short check_crc16(unsigned char *check_data, int length)
{
	unsigned short crc16 = 0;
	unsigned short datatemp;
	int i;

	for (i = 0; i < length; i++) {
		datatemp = check_data[i];
		datatemp = (datatemp ^ (crc16 & 0xff)) & 0xff;
		crc16 >>= 8;

		if (oddparity[datatemp & 0xf] ^ oddparity[datatemp >> 4])
			crc16 ^= 0xc001;

		datatemp <<= 6;
		crc16 ^= datatemp;
		datatemp <<= 1;
		crc16 ^= datatemp;
	}

	return crc16;
}

/*
 * format and write array to  the character string str
 * @buf: destination
 * @array: source
 */
int snprintf_array(unsigned char *buf, int buf_len,
	unsigned char *array, int array_len)
{
	int i;
	int ret = 0;

	for (i = 0; i < array_len; i++) {
		ret += snprintf(buf + ret, buf_len - ret, "%02X ", array[i]);
		if ((ret + 1) >= buf_len)
			break;
	}

	return ret;
}

void set_sched_affinity_to_current(void)
{
	long retval;
	int current_cpu;

	preempt_disable();
	current_cpu = smp_processor_id();
	preempt_enable();
	retval = sched_setaffinity(CURRENT_MAXIM_TASK, cpumask_of(current_cpu));
	if (retval)
		hwlog_info("Setting current cpu affinity failed %ld in %s\n",
			retval, __func__);
	else
		hwlog_info("Setting current cpu %d affinity in %s\n",
			current_cpu, __func__);
}

void set_sched_affinity_to_all(void)
{
	long retval;
	cpumask_t dstp;

	cpumask_setall(&dstp);
	retval = sched_setaffinity(CURRENT_MAXIM_TASK, &dstp);
	if (retval)
		hwlog_info("Setting all cpus affinity failed(%ld) in %s\n",
			   retval, __func__);
	else
		hwlog_info("Setting all cpus affinity in %s\n", __func__);
}

/* Check whether IC ROM ID conforms to format */
int maxim_check_rom_id_format(struct maxim_onewire_drv_data *drv_data)
{
	int i;
	struct battery_constraint bcons = drv_data->batt_cons;
	unsigned char *rom_id = get_rom_id(drv_data);
	int id_length = get_rom_id_len(drv_data);

	for (i = 0; i < id_length; i++)
		bcons.id_chk[i] = bcons.id_example[i] ^ rom_id[i];

	for (i = 0; i < id_length; i++)
		bcons.id_chk[i] &= bcons.id_mask[i];

	for (i = 0; i < id_length; i++) {
		if (bcons.id_chk[i]) {
			hwlog_err("IC id was unmatched at %dth byte\n", i);
			return MAXIM_FAIL;
		}
	}

	return MAXIM_SUCCESS;
}

/*
 * create a bunch of attribute groups
 * @drv_data:the devices driver data
 * @pdev:the devices
 * @attrs:Pointer to NULL terminated list of attributes.
 */
int maxim_dev_sys_node_init(struct maxim_onewire_drv_data *drv_data,
	struct platform_device *pdev, const struct attribute **attrs)
{
	drv_data->attr_group = devm_kzalloc(&pdev->dev,
		sizeof(struct attribute_group), GFP_KERNEL);
	drv_data->attr_groups =
		devm_kzalloc(&pdev->dev, 2 * sizeof(struct attribute_group *),
			GFP_KERNEL);
	if (!drv_data->attr_group || !drv_data->attr_groups)
		goto create_groups_fail;

	drv_data->attr_group->attrs = (struct attribute **)attrs;

	drv_data->attr_groups[0] = drv_data->attr_group;
	drv_data->attr_groups[1] = NULL;

	if (sysfs_create_groups(&pdev->dev.kobj, drv_data->attr_groups))
		goto create_groups_fail;

	return MAXIM_SUCCESS;

create_groups_fail:
	if (drv_data->attr_group) {
		devm_kfree(&pdev->dev, drv_data->attr_group);
		drv_data->attr_group = 0;
	}
	if (drv_data->attr_groups) {
		devm_kfree(&pdev->dev, drv_data->attr_groups);
		drv_data->attr_groups = 0;
	}

	return MAXIM_FAIL;
}

static void maxim_dev_sys_node_remove(struct maxim_onewire_drv_data *drv_data,
	struct platform_device *pdev)
{
	if (drv_data->attr_group && drv_data->attr_groups)
		sysfs_remove_groups(&pdev->dev.kobj, drv_data->attr_groups);

	if (drv_data->attr_groups) {
		devm_kfree(&pdev->dev, drv_data->attr_groups);
		drv_data->attr_groups = 0;
	}

	if (drv_data->attr_group) {
		devm_kfree(&pdev->dev, drv_data->attr_group);
		drv_data->attr_group = 0;
	}
}

static int maxim_sn_info_init(struct maxim_onewire_mem *mem,
	struct platform_device *pdev)
{
	int ret;
	struct device_node *batt_ic_np = pdev->dev.of_node;
	int sn_page_bits;

	mem->sn_length_bits = SN_LENGTH_BITS;
	ret = of_property_read_u32(batt_ic_np, "sn-offset-bits",
		&(mem->sn_offset_bits));
	if (ret) {
		hwlog_err("Read battery SN offset in bits failed\n");
		return MAXIM_FAIL;
	} else if (not_muti8(mem->sn_offset_bits)) {
		hwlog_err("Illegal SN offset(%u) found in %s\n",
			  mem->sn_offset_bits, __func__);
		return MAXIM_FAIL;
	}

	sn_page_bits = mem->sn_length_bits + mem->sn_offset_bits;
	if ((mem->sn_length_bits > bytes2bits(mem->page_size)) ||
	    (mem->sn_offset_bits > bytes2bits(mem->page_size)) ||
	    (sn_page_bits > bytes2bits(mem->page_size)))
		hwlog_err("Battery SN length(%u) or offset(%u) is illegal\n",
			  mem->sn_length_bits, mem->sn_offset_bits);
	ret = of_property_read_u32(batt_ic_np, "sn-page", &(mem->sn_page));
	if (ret || (mem->sn_page >= mem->page_number))
		hwlog_err("Illegal battery SN page(%u) readed by %s\n",
			  mem->sn_page, __func__);

	return MAXIM_SUCCESS;
}

static int maxim_memory_init(struct maxim_onewire_mem *mem,
	struct platform_device *pdev)
{
	int ret;
	struct device_node *batt_ic_np = pdev->dev.of_node;

	memset(mem, 0, sizeof(*mem));
	/* init maxim_onewire_mem lock */
	mutex_init(&mem->lock);

	/* Get battery ic's memory description */
	ret = of_property_read_u32(batt_ic_np, "rom-id-length",
		&(mem->rom_id_length));
	if (ret) {
		hwlog_err("DTS:rom-id-length needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np,
		"page-number", &(mem->page_number));
	if (ret) {
		hwlog_err("DTS:page-number needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np, "page-size", &(mem->page_size));
	if (ret) {
		hwlog_err("DTS:page-size needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np, "mac-length",
		&(mem->mac_length));
	if (ret) {
		hwlog_err("DTS:mac-length needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np,
		"block-number", &(mem->block_number));
	if (ret) {
		hwlog_err("DTS:block-number needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np,
		"block-size", &(mem->block_size));
	if (ret) {
		hwlog_err("DTS:block-size needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}

	/* Allocate memory for battery memory */
	mem->rom_id = devm_kzalloc(&pdev->dev,
		mem->rom_id_length + 1, GFP_KERNEL);
	if (!mem->rom_id)
		return MAXIM_FAIL;
	mem->block_status = devm_kzalloc(&pdev->dev,
		mem->block_number + 1, GFP_KERNEL);
	if (!mem->block_status)
		return MAXIM_FAIL;
	mem->pages = devm_kzalloc(&pdev->dev,
		mem->page_number * (mem->page_size + 1), GFP_KERNEL);
	if (!mem->pages)
		return MAXIM_FAIL;

	mem->mac = devm_kzalloc(&pdev->dev, mem->mac_length + 1, GFP_KERNEL);
	if (!mem->mac)
		return MAXIM_FAIL;
	mem->sn = devm_kzalloc(&pdev->dev, PRINTABLE_SN_SIZE, GFP_KERNEL);
	if (!mem->sn)
		return MAXIM_FAIL;
	mem->mac_datum = devm_kzalloc(&pdev->dev,
		MAX_MAC_SOURCE_SIZE, GFP_KERNEL);
	if (!mem->mac_datum)
		return MAXIM_FAIL;

	return maxim_sn_info_init(mem, pdev);
}

static void maxim_destroy_maxim_mem(struct maxim_onewire_mem *mem,
	struct platform_device *pdev)
{
	if (mem->block_status) {
		devm_kfree(&pdev->dev, mem->block_status);
		mem->block_status = NULL;
	}
	if (mem->rom_id) {
		devm_kfree(&pdev->dev, mem->rom_id);
		mem->rom_id = NULL;
	}
	if (mem->pages) {
		devm_kfree(&pdev->dev, mem->pages);
		mem->pages = NULL;
	}
	if (mem->mac) {
		devm_kfree(&pdev->dev, mem->mac);
		mem->mac = NULL;
	}
	if (mem->sn) {
		devm_kfree(&pdev->dev, mem->sn);
		mem->sn = NULL;
	}
	if (mem->mac_datum) {
		devm_kfree(&pdev->dev, mem->mac_datum);
		mem->mac_datum = NULL;
	}
	mutex_destroy(&mem->lock);
}

static int maxim_phy_time_rq_init(struct maxim_onewire_des *ic_des,
	struct platform_device *pdev)
{
	int ret;
	struct device_node *batt_ic_np = pdev->dev.of_node;
	struct ow_treq *owtrq = &ic_des->trq.ow_trq;

	/* get 1-wire ic time requests */
	ret = of_property_read_u32_index(batt_ic_np, "reset-time-request",
		FIRST_TIME_PROPERTY, &(owtrq->reset_init_low_ns));
	if (ret) {
		hwlog_err("DTS:reset_init_low_ns needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32_index(batt_ic_np, "reset-time-request",
		SECOND_TIME_PROPERTY, &(owtrq->reset_slave_response_delay_ns));
	if (ret) {
		hwlog_err("DTS:reset_slave_response_delay_ns needed in %s\n",
			  __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32_index(batt_ic_np, "reset-time-request",
		THIRD_TIME_PROPERTY, &(owtrq->reset_hold_high_ns));
	if (ret) {
		hwlog_err("DTS:reset_hold_high_ns needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32_index(batt_ic_np, "write-time-request",
		FIRST_TIME_PROPERTY, &(owtrq->write_init_low_ns));
	if (ret) {
		hwlog_err("DTS:write_init_low_ns needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32_index(batt_ic_np, "write-time-request",
		SECOND_TIME_PROPERTY, &(owtrq->write_hold_ns));
	if (ret) {
		hwlog_err("DTS:write_hold_ns needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32_index(batt_ic_np, "write-time-request",
		THIRD_TIME_PROPERTY, &(owtrq->write_residual_ns));
	if (ret) {
		hwlog_err("DTS:write_residual_ns needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32_index(batt_ic_np, "read-time-request",
		FIRST_TIME_PROPERTY, &(owtrq->read_init_low_ns));
	if (ret) {
		hwlog_err("DTS:read_init_low_ns needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32_index(batt_ic_np, "read-time-request",
		SECOND_TIME_PROPERTY, &(owtrq->read_residual_ns));
	if (ret) {
		hwlog_err("DTS:read_residual_ns needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}

	owtrq->reset_hold_high_cycles = ns2cycles(owtrq->reset_hold_high_ns);
	owtrq->reset_init_low_cycles = ns2cycles(owtrq->reset_init_low_ns);
	owtrq->reset_slave_response_delay_cycles =
	    ns2cycles(owtrq->reset_slave_response_delay_ns);
	owtrq->write_init_low_cycles = ns2cycles(owtrq->write_init_low_ns);
	owtrq->write_hold_cycles = ns2cycles(owtrq->write_hold_ns);
	owtrq->write_residual_cycles = ns2cycles(owtrq->write_residual_ns);
	owtrq->read_init_low_cycles = ns2cycles(owtrq->read_init_low_ns);
	owtrq->read_residual_cycles = ns2cycles(owtrq->read_residual_ns);
	hwlog_info("Timer Frequence is %u, Hz is %u, Loops per jiffy is %lu\n",
		   arch_timer_get_cntfrq(), HZ, loops_per_jiffy);
	hwlog_info("%ucycles(%uns), %ucycles(%uns), %ucycles(%uns)",
		   owtrq->reset_init_low_cycles, owtrq->reset_init_low_ns,
		   owtrq->reset_slave_response_delay_cycles,
		   owtrq->reset_slave_response_delay_ns,
		   owtrq->reset_hold_high_cycles, owtrq->reset_hold_high_ns);
	hwlog_info("%ucycles(%uns), %ucycles(%uns), %ucycles(%uns)\n",
		   owtrq->write_init_low_cycles, owtrq->write_init_low_ns,
		   owtrq->write_hold_cycles, owtrq->write_hold_ns,
		   owtrq->write_residual_cycles, owtrq->write_residual_ns);
	hwlog_info("%ucycles(%uns), %ucycles(%uns)\n",
		   owtrq->read_init_low_cycles, owtrq->read_init_low_ns,
		   owtrq->read_residual_cycles, owtrq->read_residual_ns);

	owtrq->read_wait_slave_ns = 0;
	owtrq->read_wait_slave_cycles = 0;
	owtrq->transport_bit_order = 0;
	return MAXIM_SUCCESS;
}

static int maxim_protocol_time_rq_init(struct maxim_onewire_des *ic_des,
	struct platform_device *pdev)
{
	int ret;
	struct device_node *batt_ic_np = pdev->dev.of_node;

	ret = of_property_read_u32(batt_ic_np,
		"read-page-time", &(ic_des->trq.t_read));
	if (ret) {
		hwlog_err("DTS:read-page-time needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np,
		"programe-time", &(ic_des->trq.t_write_memory));
	if (ret) {
		hwlog_err("DTS:programe-time needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np,
		"write-status-time", &(ic_des->trq.t_write_status));
	if (ret) {
		hwlog_err("DTS:write-status-time needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u32(batt_ic_np,
		"compute-mac-time", &(ic_des->trq.t_compute));
	if (ret) {
		hwlog_err("DTS:compute-mac-time needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}

	hwlog_info("t_read = %ums, t_write_memory = %ums\n",
		ic_des->trq.t_read, ic_des->trq.t_write_memory);
	hwlog_info("t_write_status = %ums, t_compute = %ums\n",
		ic_des->trq.t_write_status, ic_des->trq.t_compute);

	return MAXIM_SUCCESS;
}

static int maxim_des_init(struct maxim_onewire_des *ic_des,
	struct platform_device *pdev)
{
	int ret;

	/* init memory */
	ret = maxim_memory_init(&ic_des->memory, pdev);
	if (ret) {
		hwlog_err("Maxim memory init failed %d in %s\n",
			ret, __func__);
		return ret;
	}

	/* init physical time request */
	ret = maxim_phy_time_rq_init(ic_des, pdev);
	if (ret) {
		hwlog_err("Maxim phy time request init failed %d in %s\n",
			ret, __func__);
		return ret;
	}

	/* init protocol time request */
	ret = maxim_protocol_time_rq_init(ic_des, pdev);
	if (ret) {
		hwlog_err("Maxim protocol time request init failed %d in %s\n",
			ret, __func__);
		return ret;
	}

	return MAXIM_SUCCESS;
}

/* Battery constraints initialization */
static int maxim_batt_cons_init(struct maxim_onewire_drv_data *drv_data,
	struct platform_device *pdev)
{
	int ret;

	/* Allocate memory for battery constraint information */
	drv_data->batt_cons.id_mask = devm_kzalloc(&pdev->dev,
		get_rom_id_len(drv_data), GFP_KERNEL);
	if (!drv_data->batt_cons.id_mask)
		return MAXIM_FAIL;
	drv_data->batt_cons.id_example = devm_kzalloc(&pdev->dev,
		get_rom_id_len(drv_data), GFP_KERNEL);
	if (!drv_data->batt_cons.id_example)
		return MAXIM_FAIL;
	drv_data->batt_cons.id_chk = devm_kzalloc(&pdev->dev,
		get_rom_id_len(drv_data), GFP_KERNEL);
	if (!drv_data->batt_cons.id_chk)
		return MAXIM_FAIL;

	/* Get battery id mask & id example */
	ret = of_property_read_u8_array(pdev->dev.of_node, "id-mask",
		drv_data->batt_cons.id_mask, get_rom_id_len(drv_data));
	if (ret) {
		hwlog_err("DTS:id-mask needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}
	ret = of_property_read_u8_array(pdev->dev.of_node, "id-example",
		drv_data->batt_cons.id_example, get_rom_id_len(drv_data));
	if (ret) {
		hwlog_err("DTS:id-example needed in %s\n", __func__);
		return MAXIM_ONEWIRE_DTS_ERROR;
	}

	return MAXIM_SUCCESS;
}

static void maxim_destroy_batt_cons_mem(struct battery_constraint *bcons,
	struct platform_device *pdev)
{
	if (bcons->id_mask) {
		devm_kfree(&pdev->dev, bcons->id_mask);
		bcons->id_mask = 0;
	}

	if (bcons->id_example) {
		devm_kfree(&pdev->dev, bcons->id_example);
		bcons->id_example = 0;
	}

	if (bcons->id_chk) {
		devm_kfree(&pdev->dev, bcons->id_chk);
		bcons->id_chk = 0;
	}
}

static int maxim_com_stat_init(struct maxim_onewire_com_stat *coms,
	struct platform_device *pdev, int cmds, const char * const *cmd_str)
{
	coms->totals = devm_kzalloc(&pdev->dev, cmds * sizeof(int), GFP_KERNEL);
	if (!coms->totals)
		return MAXIM_FAIL;
	coms->errs = devm_kzalloc(&pdev->dev, cmds * sizeof(int), GFP_KERNEL);
	if (!coms->errs)
		return MAXIM_FAIL;

	coms->cmds = cmds;
	coms->cmd_str = cmd_str;

	return MAXIM_SUCCESS;
}

static void maxim_destroy_com_stat_mem(struct maxim_onewire_com_stat *coms,
	struct platform_device *pdev)
{
	if (coms->totals) {
		devm_kfree(&pdev->dev, coms->totals);
		coms->totals = NULL;
	}

	if (coms->errs) {
		devm_kfree(&pdev->dev, coms->errs);
		coms->errs = NULL;
	}
	coms->cmds = 0;
	coms->cmd_str = NULL;
}

/*
 * Initialize Maxim Driver Data
 * @drv_data:driver data to Initialize
 * @pdev:carrier of driver data
 * @cmds:maxim protocol commands to state Txs and Errors
 * @cmd_str:maxim protocol commands description
 */
int maxim_drv_data_init(struct maxim_onewire_drv_data *drv_data,
	struct platform_device *pdev, int cmds, const char * const cmd_str[])
{
	drv_data->write_lock = power_wakeup_source_register(&pdev->dev, pdev->name);
	mutex_init(&drv_data->batt_safe_info_lock);

	if (maxim_des_init(&drv_data->ic_des, pdev)) {
		hwlog_err("maxim description initial failed in %s\n", __func__);
		goto des_mem_init_fail;
	}

	if (maxim_batt_cons_init(drv_data, pdev)) {
		hwlog_err("maxim constraint init failed in %s\n", __func__);
		goto batt_cons_init_fail;
	}

	if (maxim_com_stat_init(&drv_data->com_stat, pdev, cmds, cmd_str)) {
		hwlog_err("maxim com stat mem init failed in %s\n", __func__);
		goto com_stat_init_fail;
	}

	return MAXIM_SUCCESS;

com_stat_init_fail:
	maxim_destroy_com_stat_mem(&drv_data->com_stat, pdev);
batt_cons_init_fail:
	maxim_destroy_batt_cons_mem(&drv_data->batt_cons, pdev);
des_mem_init_fail:
	maxim_destroy_maxim_mem(&drv_data->ic_des.memory, pdev);
	power_wakeup_source_unregister(drv_data->write_lock);
	mutex_destroy(&drv_data->batt_safe_info_lock);

	return MAXIM_FAIL;
}

/*
 * Destroy Maxim Driver Data
 * @drv_data:driver data to Initialize
 * @pdev:carrier of driver data
 */
void maxim_destroy_drv_data(struct maxim_onewire_drv_data *drv_data,
	struct platform_device *pdev)
{
	if (!drv_data)
		return;

	maxim_dev_sys_node_remove(drv_data, pdev);
	maxim_destroy_maxim_mem(&(drv_data->ic_des.memory), pdev);
	maxim_destroy_batt_cons_mem(&(drv_data->batt_cons), pdev);
	maxim_destroy_com_stat_mem(&(drv_data->com_stat), pdev);

	power_wakeup_source_unregister(drv_data->write_lock);

	mutex_destroy(&drv_data->batt_safe_info_lock);
}

/*
 * Convert Sn page to printable form
 * @drv_data:the sn page data pointer
 * @sn_offset_bits:sn offset in page data
 * @sn:output
 */
void maxim_parise_printable_sn(unsigned char *page, unsigned int sn_offset_bits,
	unsigned char *sn)
{
	int i;
	char hex_print;
	char *sn_to_print = page + sn_offset_bits / BYTEBITS;

	for (i = 0; i < SN_CHAR_PRINT_SIZE; i++)
		sn[i] = sn_to_print[i];

	sn_to_print += SN_CHAR_PRINT_SIZE;
	for (i = 0; i < SN_HEX_PRINT_SIZE; i++) {
		if (is_odd(i)) /* get low 4 bits */
			hex_print = (sn_to_print[i / 2] & 0x0f);
		else /* get high 4 bits */
			hex_print = ((sn_to_print[i / 2] & 0xf0) >> 4) & 0x0f;

		sprintf(sn + (i + SN_CHAR_PRINT_SIZE), "%X", hex_print);
	}
}
