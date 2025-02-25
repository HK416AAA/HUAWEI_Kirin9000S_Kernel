/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2020-2020. All rights reserved.
 *
 * pmic.h
 *
 * Header file for device driver PMIC
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

#ifndef	__PMIC_CORE_H__
#define	__PMIC_CORE_H__

#include <linux/irqdomain.h>
#include <linux/irq.h>
#include <linux/regulator/consumer.h>

#define PMIC_REGS_ENA_PROTECT_TIME	0 /* in microseconds */
#define PMIC_ECO_MODE_ENABLE		1
#define PMIC_ECO_MODE_DISABLE		0

#define PMIC_DIEID_BUF		100
#define PMIC_DIEID_TEM_SAVE_BUF	4

#define PMU_VERSION_ADDR	0
#define VERSION_LEN	6
#define VERSION_HIGH	8

#define PMIC_IRQ_NAME_SIZE		20

#define SPMI_PMIC_COMP		"pmic-spmi"
#define SPMI_SUB_PMIC_COMP	"sub-pmic-spmi"
#define SPMI_MMW_PMIC_COMP	"mmw-pmic-spmi"
#define SPMI_COMMON_PMIC_COMP	"common-pmic-spmi"

typedef int (*pmic_ocp_callback)(char *);

struct bit_info {
	int addr;
	int bit;
};

struct vbus_irq_info {
	unsigned int addr;
	unsigned int shift;
	unsigned int mask;
};

struct write_lock {
	int addr;
	int val;
};

struct vendor_pmic {
	struct resource *res;
	struct device *dev;
	void __iomem *regs;
	spinlock_t lock;
	struct irq_domain *domain;
	struct regmap *regmap;
	int irq;
	int gpio;
	unsigned int *irqs;
	int irqnum;
	int irqarray;
	struct write_lock normal_lock;
	struct write_lock debug_lock;
	char *dieid_name;
	unsigned int dieid_reg_num;
	unsigned int *dieid_regs;
	int *irq_mask_addr_arry;
	int *irq_addr_arry;
	char irq_name[PMIC_IRQ_NAME_SIZE];
	char chip_irq_name[PMIC_IRQ_NAME_SIZE];
	struct irq_chip irq_chip;
	int powerkey_irq_down_up;
};

/* 0:disable; 1:enable */
unsigned int get_uv_mntn_status(void);
void clear_uv_mntn_resered_reg_bit(void);
void set_uv_mntn_resered_reg_bit(void);

#if defined(CONFIG_PMIC_SPMI) || defined(CONFIG_PMIC_VM_DRV)
unsigned int pmic_read_reg(int addr);
void pmic_write_reg(int addr, int val);
#else
static inline unsigned int pmic_read_reg(int addr)
{
	return 0;
}
static inline void pmic_write_reg(int addr, int val)
{
}
#endif

#ifdef CONFIG_PMIC_SPMI
/* Register Access Helpers */
u32 main_pmic_read(struct vendor_pmic *pmic, int reg);
void main_pmic_write(struct vendor_pmic *pmic, int reg, u32 val);
void pmic_rmw(struct vendor_pmic *pmic, int reg, u32 mask, u32 bits);
void pmic_reg_write_lock(int addr, int val);
int pmic_array_read(int addr, char *buff, unsigned int len);
int pmic_array_write(int addr, const char *buff, unsigned int len);
extern int pmic_get_irq_byname(unsigned int pmic_irq_list);
extern int pmic_get_vbus_status(void);
void pmic_vbus_irq_mask(int enable);
void pmic_clear_vbus_irq(int enable);
extern int pmic_special_ocp_register(
	char *power_name, pmic_ocp_callback handler);
unsigned short pmic_get_main_pmu_version(void);

#ifdef CONFIG_PMIC_SUB_PMU_SPMI
u32 sub_pmic_read(struct vendor_pmic *pmic, int reg);
void sub_pmic_write(struct vendor_pmic *pmic, int reg, u32 val);
int pmic_subpmu_get_dieid(char *dieid, unsigned int len);
unsigned int sub_pmic_reg_read(int addr);
void sub_pmic_reg_write(int addr, int val);
unsigned int mmw_pmic_reg_read(int addr);
void mmw_pmic_reg_write(int addr, int val);
#else
static inline unsigned int sub_pmic_reg_read(int addr)
{
	return 0;
}
static inline void sub_pmic_reg_write(int addr, int val)
{
}
static inline unsigned int mmw_pmic_reg_read(int addr)
{
	return 0;
}
static inline void mmw_pmic_reg_write(int addr, int val)
{
}
#endif

u32 pmic_read_sub_pmu(u8 sid, int reg);
void pmic_write_sub_pmu(u8 sid, int reg, u32 val);
int pmic_get_dieid(char *dieid, unsigned int len);
#if defined(CONFIG_SUBPMU) && defined(CONFIG_PLATFORM_DIEID)
int subpmu_get_dieid(char *dieid, unsigned int len);
#endif

static inline int sub_pmu_pmic_dieid(char *dieid, unsigned int len)
{
	int ret = 0;
#if defined(CONFIG_SUBPMU) && defined(CONFIG_PLATFORM_DIEID)
	ret = subpmu_get_dieid(dieid, len);
#elif defined(CONFIG_PMIC_SUB_PMU_SPMI)
	ret = pmic_subpmu_get_dieid(dieid, len);
#endif
	return ret;
}
#else
static inline u32 main_pmic_read(struct vendor_pmic *pmic, int reg)
{
	return 0;
}
static inline void main_pmic_write(struct vendor_pmic *pmic, int reg, u32 val)
{
}
static inline void pmic_rmw(
	struct vendor_pmic *pmic, int reg, u32 mask, u32 bits)
{
}
static inline unsigned int sub_pmic_reg_read(int addr)
{
	return 0;
}
static inline void sub_pmic_reg_write(int addr, int val)
{
}
static inline unsigned int mmw_pmic_reg_read(int addr)
{
	return 0;
}
static inline void mmw_pmic_reg_write(int addr, int val)
{
}
static inline void pmic_reg_write_lock(int addr, int val)
{
}
static inline int pmic_array_read(int addr, char *buff, unsigned int len)
{
	return 0;
}
static inline int pmic_array_write(
	int addr, const char *buff, unsigned int len)
{
	return 0;
}
static inline int pmic_get_irq_byname(unsigned int pmic_irq_list)
{
	return -1;
}
static inline int pmic_get_vbus_status(void)
{
	return 1;
}
static inline void pmic_vbus_irq_mask(int enable)
{
}
static inline void pmic_clear_vbus_irq(int enable)
{
}
static inline int pmic_special_ocp_register(
	char *power_name, pmic_ocp_callback handler)
{
	return 0;
}
static inline u32 pmic_read_sub_pmu(u8 sid, int reg)
{
	return 0;
}
static inline void pmic_write_sub_pmu(u8 sid, int reg, u32 val)
{
}
static inline int pmic_get_dieid(char *dieid, unsigned int len)
{
	return 0;
}
static inline int sub_pmu_pmic_dieid(char *dieid, unsigned int len)
{
	return 0;
}
#endif

/* for modem */
#ifndef CONFIG_AB_APCP_NEW_INTERFACE
#ifdef CONFIG_PMIC_SPMI
unsigned int hisi_pmic_reg_read(int addr);
void hisi_pmic_reg_write(int addr, int val);
#else
static inline unsigned int hisi_pmic_reg_read(int addr)
{
	return -1;
}
static inline void hisi_pmic_reg_write(int addr, int val)
{
}
#endif

#ifdef CONFIG_PMIC_SUB_PMU_SPMI
unsigned int hisi_sub_pmic_reg_read(int addr);
void hisi_sub_pmic_reg_write(int addr, int val);
#else
static inline unsigned int hisi_sub_pmic_reg_read(int addr)
{
	return -1;
}
static inline void hisi_sub_pmic_reg_write(int addr, int val)
{
}
#endif

#ifdef CONFIG_PMIC_MNTN_SPMI
extern int hisi_pmic_special_ocp_register(char *power_name, pmic_ocp_callback handler);
#else
static inline int hisi_pmic_special_ocp_register(char *power_name, pmic_ocp_callback handler)
{
	return -1;
}
#endif
#endif

#ifdef CONFIG_PMIC_21V500_PMU
enum pmic_irq_list {
	POR_D45MR = 0,
	VBUS_CONNECT,
	VBUS_DISCONNECT,
	ALARMON_R,
	HOLD_6S,
	HOLD_1S,
	POWERKEY_UP,
	POWERKEY_DOWN,
	OCP_SCP_R,
	COUL_R,
	VSYS_OV,
	VSYS_UV,
	VSYS_PWROFF_ABS,
	VSYS_PWROFF_DEB,
	THSD_OTMP140,
	THSD_OTMP125,
	HRESETN,
	SIM0_HPD_R = 24,
	SIM0_HPD_F,
	SIM0_HPD_H,
	SIM0_HPD_L,
	SIM1_HPD_R,
	SIM1_HPD_F,
	SIM1_HPD_H,
	SIM1_HPD_L,
	PMIC_IRQ_LIST_MAX,
};
#else
enum pmic_irq_list {
	OTMP = 0,
	VBUS_CONNECT,
	VBUS_DISCONNECT,
	ALARMON_R,
	HOLD_6S,
	HOLD_1S,
	POWERKEY_UP,
	POWERKEY_DOWN,
	OCP_SCP_R,
	COUL_R,
	SIM0_HPD_R,
	SIM0_HPD_F,
	SIM1_HPD_R,
	SIM1_HPD_F,
	PMIC_IRQ_LIST_MAX,
};
#endif

#ifdef CONFIG_DRV_REGULATOR_DEBUG
void get_ip_regulator_state(void);
#else
static inline void get_ip_regulator_state(void)
{
}
#endif
#endif /* __PMIC_CORE_H__ */
