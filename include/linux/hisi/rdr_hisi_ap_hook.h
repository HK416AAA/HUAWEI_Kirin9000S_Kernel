/*
 *
 * AP side track hook header file.
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
#ifndef __RDR_DFX_AP_HOOK_H__
#define __RDR_DFX_AP_HOOK_H__

#include <linux/thread_info.h>
#include <platform_include/basicplatform/linux/rdr_types.h>
#include <platform_include/basicplatform/linux/rdr_pub.h>
#include <linux/version.h>

#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
#include <mm_ion.h>
#endif

#if (KERNEL_VERSION(4, 9, 76) <= LINUX_VERSION_CODE)
#include <linux/sched.h>
#endif

enum hook_mem_type {
	DFX_MEM_FREE = 0,
	DFX_MEM_ALLOC = 1,
};
/* Need to print more 2 workers before begin worker, and 1 worker includes 2 numbers */
#define AHEAD_BEGIN_WORKER 4

#define NS_INTERVAL 1000000000
#define SECOND_TICK_CNT 32764

typedef u64 (*arch_timer_func_ptr)(void);

enum hook_type {
	HK_IRQ = 0,
	HK_TASK,
	HK_CPUIDLE,
	HK_WORKER,
	HK_MEM_ALLOCATOR,
	HK_ION_ALLOCATOR,
	HK_TIME,
	HK_NET_TX_SOFTIRQ = 7, /* The value cannot be changed. It is used by "hck_dfx_bbox_softirq_hook" */
	HK_NET_RX_SOFTIRQ = 8, /* The value cannot be changed. It is used by "hck_dfx_bbox_softirq_hook" */
	HK_BLOCK_SOFTIRQ = 9, /* The value cannot be changed. It is used by "hck_dfx_bbox_softirq_hook" */
	HK_SCHED_SOFTIRQ = 10, /* The value cannot be changed. It is used by "hck_dfx_bbox_softirq_hook" */
	HK_RCU_SOFTIRQ = 11, /* The value cannot be changed. It is used by "hck_dfx_bbox_softirq_hook" */
	HK_PERCPU_TAG, /* The track of percpu is above HK_PERCPU_TAG */
	HK_CPU_ONOFF = HK_PERCPU_TAG,
	HK_SYSCALL,
	HK_HUNGTASK,
	HK_TASKLET,
	HK_DIAGINFO,
	HK_TIMEOUT_SOFTIRQ,
	HK_MAX
};

struct hook_irq_info {
	u64 clock;
	u64 jiff;
	u32 irq;
	u8 dir;
};

struct task_info {
	u64 clock;
	u64 stack;
	u32 pid;
	char comm[TASK_COMM_LEN];
};

struct cpuidle_info {
	u64 clock;
	u8 dir;
};

struct cpu_onoff_info {
	u64 clock;
	u8 cpu;
	u8 on;
};

struct system_call_info {
	u64 clock;
	u32 syscall;
	u8 cpu;
	u8 dir;
};

struct hung_task_info {
	u64 clock;
	u32 timeout;
	u32 pid;
	char comm[TASK_COMM_LEN];
};

struct tasklet_info {
	u64 clock;
	u64 action;
	u8 cpu;
	u8 dir;
};

struct worker_info {
	u64 clock;
	u64 action;
	u8 dir;
	u8 cpu;
	u16 resv;
	u32 pid;
};

struct mem_allocator_info {
	u64 clock;
	u32 pid;
	char comm[TASK_COMM_LEN];
	u64 caller;
	u8 operation;
	u64 va_addr;
	u64 phy_addr;
	u64 size;
};

struct ion_allocator_info {
	u64 clock;
	u32 pid;
	char comm[TASK_COMM_LEN];
	u8 operation;
	u64 va_addr;
	u64 phy_addr;
	u32 size;
};

struct time_info {
	u64 clock;
	u64 action;
	u8 dir;
};

struct softirq_info {
	u64 clock;
	u64 action;
	u8 dir;
};

struct percpu_buffer_info {
	unsigned char *buffer_addr;
	unsigned char *percpu_addr[NR_CPUS];
	unsigned int percpu_length[NR_CPUS];
	unsigned int buffer_size;
};

int register_arch_timer_func_ptr(arch_timer_func_ptr func);
int irq_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int task_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int cpuidle_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int cpu_onoff_buffer_init(unsigned char **addr, unsigned int size);
int syscall_buffer_init(unsigned char **addr, unsigned int size);
int hung_task_buffer_init(unsigned char **addr, unsigned int size);
int worker_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int tasklet_buffer_init(unsigned char **addr, unsigned int size);
int diaginfo_buffer_init(unsigned char **addr, unsigned int size);
int mem_alloc_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int ion_alloc_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int net_tx_softirq_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int net_rx_softirq_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int block_softirq_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int sched_softirq_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int rcu_softirq_init(struct percpu_buffer_info *buffer_info, unsigned char *addr, unsigned int size);
int timeout_softirq_buffer_init(unsigned char **addr, unsigned int size);
int time_buffer_init(struct percpu_buffer_info *buffer_info, unsigned char *addr,
				unsigned int size);

#ifdef CONFIG_PM
void hook_percpu_buffer_backup(void);
void hook_percpu_buffer_restore(void);
#endif

void worker_recent_search(u8 cpu, u64 basetime, u64 delta_time);

#ifdef CONFIG_DFX_BB
void irq_trace_hook(unsigned int dir, unsigned int old_vec, unsigned int new_vec);
void task_switch_hook(const void *pre_task, void *next_task);
void cpuidle_stat_hook(u32 dir);
void cpu_on_off_hook(u32 cpu, u32 on);
void syscall_hook(u32 syscall_num, u32 dir);
void hung_task_hook(void *tsk, u32 timeout);
void tasklet_hook(u64 address, u32 dir);
void worker_hook(u64 address, u32 dir);
void softirq_hook(unsigned int softirq_type, u64 address, u32 dir);
void softirq_timeout_hook(u64 address, u32 dir);
#ifdef CONFIG_DFX_MEM_TRACE
void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order);
void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size);
void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size);
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle);
#endif
void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size);
#else
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order) {}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size) {}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size) {}
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
static inline void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle) {}
#endif
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size) {}
#endif
#else
static inline void irq_trace_hook(unsigned int dir, unsigned int old_vec, unsigned int new_vec) {}
static inline void task_switch_hook(const void *pre_task, void *next_task) {}
static inline void cpuidle_stat_hook(u32 dir) {}
static inline void cpu_on_off_hook(u32 cpu, u32 on) {}
static inline void syscall_hook(u32 syscall_num, u32 dir) {}
static inline void hung_task_hook(void *tsk, u32 timeout) {}
static inline void get_current_last_irq(unsigned int cpu) {}
static inline void tasklet_hook(u64 address, u32 dir) {}
static inline void worker_hook(u64 address, u32 dir) {}
static inline void softirq_hook(unsigned int softirq_type, u64 address, u32 dir) {}
static inline void softirq_timeout_hook(u64 address, u32 dir) {}
static inline void page_trace_hook(gfp_t gfp_flag, u8 action, u64 caller, struct page *page, u32 order) {}
static inline void kmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, u64 phy_addr, u32 size) {}
static inline void vmalloc_trace_hook(u8 action, u64 caller, u64 va_addr, struct page *page, u64 size) {}
#if (KERNEL_VERSION(4, 14, 0) > LINUX_VERSION_CODE)
static inline void ion_trace_hook(u8 action, struct ion_client *client, struct ion_handle *handle) {}
#endif
static inline void smmu_trace_hook(u8 action, u64 va_addr, u64 phy_addr, u32 size) {}
#endif

#ifdef CONFIG_DFX_TIME_HOOK
void time_hook(u64 address, u32 dir);
#else
static inline void time_hook(u64 address, u32 dir) {}
#endif

void dfx_ap_defopen_hook_install(void);
int dfx_ap_hook_install(enum hook_type hk);
int dfx_ap_hook_uninstall(enum hook_type hk);
arch_timer_func_ptr get_timer_func_ptr(void);
#endif
