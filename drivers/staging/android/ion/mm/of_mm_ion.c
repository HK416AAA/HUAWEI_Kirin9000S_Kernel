/*
 * Copyright(C) 2004-2020 Hisilicon Technologies Co., Ltd. All rights reserved.
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

#define pr_fmt(fmt) "Ion: " fmt

#include <linux/dma-buf.h>
#include <linux/err.h>
#include <linux/platform_drivers/lpcpu_idle_sleep.h>
#include <linux/ion/mm_ion.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/platform_device.h>
#include <linux/sched/mm.h>
#include <linux/sizes.h>
#include <linux/uaccess.h>

#include <asm/cacheflush.h>
#include <asm/cputype.h>

#include "mm_ion_priv.h"
#include "ion.h"
#include "sec_alloc.h"
#include "vltmm.h"
#ifdef CONFIG_MM_LB
#include <linux/mm_lb/mm_lb.h>
#endif
#ifdef CONFIG_ION_MEDIA_HEAP
#include "../ion_media_heap.h"
#endif

#include <linux/bg_dmabuf_reclaim/bg_dmabuf_ops.h>

#define MAX_D_TABLE_NUM  5
#define MAX_MM_ION_DYNAMIC_AREA_NAME_LEN  64
#define MM_ION_FLUSH_ALL_CPUS_CACHES_THRESHOLD  0x1000000 /* 16MB */
#ifdef CONFIG_ZONE_MEDIA_OPT
#define MAX_WATER_MARK (SZ_1M * 400) /* 400MB */
#else
#define MAX_WATER_MARK (SZ_1M * 236) /* 236MB */
#endif

#define LOCK_RECURSIVE 1
#define MAX_PRE_ALLOC_SIZE SZ_512M

struct mm_ion_dynamic_area {
	phys_addr_t    base;
	unsigned long  size;
	char           name[MAX_MM_ION_DYNAMIC_AREA_NAME_LEN];
};

struct mm_ion_type_table {
	const char *name;
	enum ion_heap_type type;
};

struct mm_ion_heap_creat_table {
	enum ion_heap_type type;
	struct ion_heap *(*heap_creat)(struct ion_platform_heap *heap_data);
};

struct mm_ion_heap_phys_table {
	enum ion_heap_type type;
	int (*secmem_heap_phys)(struct ion_heap *heap, struct ion_buffer *buffer,
			 phys_addr_t *addr, size_t *len);
};

static const struct mm_ion_type_table ion_type_table[] = {
	{"ion_system", ION_HEAP_TYPE_SYSTEM},
	{"ion_system_contig", ION_HEAP_TYPE_SYSTEM_CONTIG},
	{"ion_carveout", ION_HEAP_TYPE_CARVEOUT},
#ifdef CONFIG_ION_MM_CPA
	{"ion_cpa", ION_HEAP_TYPE_CPA},
#endif
#ifdef CONFIG_ION_MM_SECCM
	{"ion_sec", ION_HEAP_TYPE_SECCM},
#endif
#ifdef CONFIG_ION_MM_SECSG
	{"ion_sec", ION_HEAP_TYPE_SEC_CONTIG},
	{"ion_sec_sg", ION_HEAP_TYPE_SECSG},
#endif
#ifdef CONFIG_ION_MM_DMA_POOL
	{"ion_dma_pool", ION_HEAP_TYPE_DMA_POOL},
#endif
#ifdef CONFIG_ION_MM_CMA_HEAP
	{"ion_dma", ION_HEAP_TYPE_DMA},
#endif
};

static struct mm_ion_heap_creat_table g_heap_creat_tb[] = {
	{ION_HEAP_TYPE_SYSTEM, ion_system_heap_create},
	{ION_HEAP_TYPE_CARVEOUT, ion_carveout_heap_create},
	{ION_HEAP_TYPE_SECCM, ion_seccm_heap_create},
	{ION_HEAP_TYPE_SEC_CONTIG, ion_seccg_heap_create},
	{ION_HEAP_TYPE_SECSG, ion_secsg_heap_create},
#ifdef CONFIG_ION_MM_CPA
	{ION_HEAP_TYPE_CPA, ion_cpa_heap_create},
#endif
	{ION_HEAP_TYPE_DMA_POOL, ion_dma_pool_heap_create},
	{ION_HEAP_TYPE_DMA, ion_mm_cma_heap_create},
};

static struct mm_ion_heap_phys_table g_heap_phys_tb[] = {
	{ION_HEAP_TYPE_SECCM, ion_seccm_heap_phys},
	{ION_HEAP_TYPE_SEC_CONTIG, ion_seccg_heap_phys},
	{ION_HEAP_TYPE_SECSG, ion_secsg_heap_phys},
#ifdef CONFIG_ION_MM_CPA
	{ION_HEAP_TYPE_CPA, ion_cpa_heap_phys},
#endif
};

static int num_heaps;
struct platform_device *mm_ion_pdev;
static struct mm_ion_dynamic_area  d_table[MAX_D_TABLE_NUM];
static int ion_dynamic_area_count;

#ifdef CONFIG_ZONE_MEDIA
struct media_zone_rsvdmem media_zone_rsvdmem_sp[MAX_MEDIA_ZONE_RSVDMEM];
int num_mz_rsvdmem;
#endif

static int add_dynamic_area(phys_addr_t base,
				unsigned long  len,
				const char *name)
{
	int ret = 0;
	int i = ion_dynamic_area_count;

	if (i < MAX_D_TABLE_NUM) {
		d_table[i].base = base;
		d_table[i].size  = len;
		strncpy(d_table[i].name, name, /* unsafe_function_ignore: strncpy */
				MAX_MM_ION_DYNAMIC_AREA_NAME_LEN-1);
		d_table[i].name[MAX_MM_ION_DYNAMIC_AREA_NAME_LEN-1] = '\0';
		pr_err("insert heap-name %s\n", d_table[i].name);
		ion_dynamic_area_count++;
		return ret;
	}

	return -EFAULT;
}

static struct mm_ion_dynamic_area *find_dynamic_area_by_name(const char *name)
{
	int i = 0;

	if (!name)
		return NULL;

	for (; i < MAX_D_TABLE_NUM; i++) {
		pr_err("name = %s, table name =%s\n", name, d_table[i].name);
		if (!strcmp(name, d_table[i].name))
			return &d_table[i];
	}

	return NULL;
}

static int mm_ion_reserve_area(struct reserved_mem *rmem)
{
	char *status = NULL;
	int  namesize = 0;
	const char *heapname = NULL;

	status = (char *)of_get_flat_dt_prop(rmem->fdt_node, "status", NULL);
	if (status && (strncmp(status, "ok", strlen("ok")) != 0))
		return 0;

	heapname = of_get_flat_dt_prop(rmem->fdt_node, "heap-name", &namesize);
	if (!heapname || (namesize <= 0)) {
		pr_err("no 'heap-name' property namesize=%d\n", namesize);
		return -EFAULT;
	}

	pr_info("base 0x%llx, size is 0x%llx, node name %s, heap-name %s namesize %d,[%d][%d][%d][%d]\n",
			rmem->base, rmem->size, rmem->name, heapname, namesize,
			 heapname[0], heapname[1], heapname[2], heapname[3]);

	if (add_dynamic_area(rmem->base, rmem->size, heapname)) {
		pr_err("fail to add to dynamic area\n");
		return -EFAULT;
	}

	return 0;
}
RESERVEDMEM_OF_DECLARE(mm_ion, "mm_ion", mm_ion_reserve_area);

static int ion_secmem_heap_phys(struct ion_heap *heap,
				struct ion_buffer *buffer,
				phys_addr_t *addr, size_t *len)
{
	int ret = -EINVAL;
	int i;

	for (i = 0; i < ARRAY_SIZE(g_heap_phys_tb); i++) {
		if (g_heap_phys_tb[i].type == heap->type) {
			ret = g_heap_phys_tb[i].secmem_heap_phys(heap, buffer, addr, len);
			break;
		}
	}

	return ret;
}

int ion_secmem_get_phys(struct dma_buf *dmabuf,
		phys_addr_t *addr, size_t *len)
{
	struct ion_buffer *buffer = NULL;
	int ret;

	if (!dmabuf || !is_ion_dma_buf(dmabuf)) {
		pr_err("%s: dmabuf is not coherent with a ION buffer !\n",
			__func__);
		return -EINVAL;
	}

	if (!addr || !len) {
		pr_err("%s: invalid addr or len pointer\n", __func__);
		return -EINVAL;
	}

	buffer = dmabuf->priv;
	if (!buffer) {
		pr_err("%s: ION buffer pointer is NULL!\n", __func__);
		return -EINVAL;
	}

	ret = ion_secmem_heap_phys(buffer->heap, buffer, addr, len);

	return ret;
}

#ifdef CONFIG_ION_MM_SECSG
static bool check_heap_type_secure(struct ion_heap *heap)
{
	switch (heap->type) {
	case ION_HEAP_TYPE_SECCM:
	case ION_HEAP_TYPE_SEC_CONTIG:
	case ION_HEAP_TYPE_SECSG:
	case ION_HEAP_TYPE_CPA:
		return true;
	default:
		return false;
	}
}

/**
 * secmem_get_buffer() - Map secure ION buffer's sharefd to buffid or sg_table
 * @fd: secure ION buffer's sharefd
 * @table: return table to caller when buffer is used by secure service.
 * @id: return buffid to caller when buffer is used by DRM service.
 * @type: return SEC_DRM_TEE when buffer is used by DRM, return SEC_SVC_MAX when
 *        buffer is used by secure service.
 *
 * When map success return 0, otherwise return errno.
 *
 * Attention: This function is only called by tzdriver, please do not use it
 * in other driver.
 */
int secmem_get_buffer(int fd, struct sg_table **table, u64 *id,
		      enum SEC_SVC *type)
{
	struct dma_buf *dmabuf = NULL;
	struct ion_buffer *buffer = NULL;
	struct ion_heap *heap = NULL;

	if (!table || !id || !type)
		return -EINVAL;

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("%s: invalid fd!\n", __func__);
		return -EINVAL;
	}

	if (!is_ion_dma_buf(dmabuf)) {
		pr_err("%s: dmabuf is no a ION buffer !\n", __func__);
		goto err;
	}

	buffer = dmabuf->priv;
	if (!buffer) {
		pr_err("%s: dmabuf has no ION buffer!\n", __func__);
		goto err;
	}

	heap = buffer->heap;
	if (!heap) {
		pr_err("%s: invalid ION buffer's heap!\n", __func__);
		goto err;
	}

	if (!check_heap_type_secure(heap)) {
		pr_err("%s: ION heap is not secure heap!\n", __func__);
		goto err;
	}

	if (buffer->id) {
		*id = buffer->id;
		*type = SEC_DRM_TEE;
	} else {
		*table = buffer->sg_table;
		*type = SEC_SVC_MAX;
	}

	dma_buf_put(dmabuf);

	return 0;
err:
	dma_buf_put(dmabuf);
	return -EINVAL;
}
#endif

/* return platform device */
struct platform_device *get_mm_ion_platform_device(void)
{
	return mm_ion_pdev;
}

static inline void artemis_flush_cache(unsigned long level)
{
	asm volatile("msr s1_1_c15_c14_0, %0" : : "r" (level));
	asm volatile("dsb sy");
	asm volatile("isb");
}

static inline void artemis_flush_cache_all(void)
{
	artemis_flush_cache(0); /* flush L1 cache */

	artemis_flush_cache(2); /* flush l2 cache */
}

#ifndef CONFIG_MM_LB_FULL_COHERENCY
static void mm_ion_flush_cache_all(void *dummy)
{
	unsigned int midr = read_cpuid_id();
	if (MIDR_PARTNUM(midr) == ARM_CPU_PART_CORTEX_A73) {
		artemis_flush_cache_all();
	} else {
		flush_cache_all();
#ifdef CONFIG_MM_LB_L3_EXTENSION
		lb_ci_cache_exclusive();
#endif
	}
}

void ion_flush_all_cpus_caches(void)
{
	int cpu;
	cpumask_t mask;
	unsigned int idle_cpus;

	cpumask_clear(&mask);

	preempt_disable();

	idle_cpus = lpcpu_get_idle_cpumask();
	for_each_online_cpu(cpu) { /*lint !e574 */
		if ((idle_cpus & BIT(cpu)) == 0)
			cpumask_set_cpu(cpu, &mask);
	}

	if ((idle_cpus & 0x0f) == 0x0f)
		cpumask_set_cpu(0, &mask);

	if ((idle_cpus & 0xf0) == 0xf0)
		cpumask_set_cpu(4, &mask);

	on_each_cpu_mask(&mask, mm_ion_flush_cache_all, NULL, 1);

	preempt_enable();
}

#ifdef CONFIG_MM_ION_FLUSH_CACHE_ALL
void ion_flush_all_cpus_caches_raw(void)
{
	int cpu;
	cpumask_t mask;

	cpumask_clear(&mask);

	preempt_disable();

	for_each_online_cpu(cpu)
		cpumask_set_cpu(cpu, &mask);

	on_each_cpu_mask(&mask, mm_ion_flush_cache_all, NULL, 1);

	preempt_enable();
}
#endif
#endif

#ifdef CONFIG_MM_LB_FULL_COHERENCY
void ion_ci_sgt(struct sg_table *table)
{
	struct scatterlist *sg = NULL;
	u32 i;

	if (!table || !table->sgl)
		return;

	for_each_sg(table->sgl, sg, table->nents, i) {
		if(PageLB(sg_page(sg)))
			__inval_dcache_area(lb_page_to_virt(sg_page(sg)), sg->length);
		else
			__inval_dcache_area(page_address(sg_page(sg)), sg->length);
	}
}

void ion_ci_pages(struct page *start_pg, u64 size)
{
	if(!start_pg)
		return;
	if(PageLB(start_pg))
		__inval_dcache_area(lb_page_to_virt(start_pg), size);
	else
		__inval_dcache_area(page_address(start_pg), size);
}
#endif

int mm_ion_mutex_lock_recursive(struct mutex *lock)
{
	int is_lock_recursive = 0;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	if (mutex_owner(lock) == current)
#else
	if (__mutex_owner(lock) == current)
#endif
		is_lock_recursive = LOCK_RECURSIVE;
	else
		mutex_lock(lock);

	return is_lock_recursive;
}

void mm_ion_mutex_unlock_recursive(struct mutex *lock, int is_lock_recursive)
{
	if (is_lock_recursive == LOCK_RECURSIVE)
		return;

	mutex_unlock(lock);
}


static int check_vaddr_bounds(struct mm_struct *mm,
	int fd, unsigned long start, unsigned long length)
{
	struct dma_buf *dmabuf = NULL;
	struct vm_area_struct *vma = NULL;

	if (start >= start + length) {
		pr_err("%s,addr is overflow!\n", __func__);
		return -EINVAL;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	if (!access_ok(start, length)) {
#else
	if (!access_ok(VERIFY_WRITE, start, length)) {
#endif
		pr_err("%s,addr can not access!\n", __func__);
		return -EINVAL;
	}

	if (!PAGE_ALIGNED(start) || !PAGE_ALIGNED(length)) {
		pr_err("%s,PAGE_ALIGNED!\n", __func__);
		return -EINVAL;
	}

	vma = find_vma(mm, start);
	if (!vma) {
		pr_err("%s,vma is null!\n", __func__);
		return -EINVAL;
	}

	if (start + length > vma->vm_end) {
		pr_err("%s,start + length > vma->vm_end(0x%lx)!\n",
			__func__, vma->vm_end);
		return -EINVAL;
	}

	dmabuf = dma_buf_get(fd);
	if (IS_ERR_OR_NULL(dmabuf)) {
		pr_err("%s, not dma buf!\n", __func__);
		return -EINVAL;
	}
	if (!vma->vm_file || file_to_dma_buf(vma->vm_file) != dmabuf) {
		pr_err("%s, not dma buf's va!\n", __func__);
		dma_buf_put(dmabuf);
		return -EINVAL;
	}
	dma_buf_put(dmabuf);

	return 0;
}

int ion_do_cache_op(int fd, unsigned long uaddr,
	unsigned long length, unsigned int cmd)
{
	int ret = 0;
	unsigned long flags = 0;
	struct dma_buf *buf = NULL;
	struct ion_buffer *buffer = NULL;

	/* Get dma_buf by fd */
	buf = dma_buf_get(fd);
	if (IS_ERR(buf)) {
		pr_err("%s get dma_buf by fd, error.\n", __func__);
		return -EFAULT;
	}

	/* Get dma_buf by fd */
	if (!is_ion_dma_buf(buf)) {
		dma_buf_put(buf);
		pr_err("%s is not ion buffer.\n", __func__);
		return -EFAULT;
	}

	/* Get flags from dma_buf.priv, which is ion_buffer */
	buffer = (struct ion_buffer *)buf->priv;
	flags = buffer->flags;

	if (!ION_IS_CACHED(flags)) {
		dma_buf_put(buf);
		pr_err("%s ION is noncached!\n", __func__);
		return 0;
	}

	switch (cmd) {
	case ION_MM_CLEAN_CACHES:
		uaccess_ttbr0_enable();
		__dma_map_area((void *)(uintptr_t)uaddr, length, DMA_BIDIRECTIONAL);
		uaccess_ttbr0_disable();
		break;

	case ION_MM_INV_CACHES:
		uaccess_ttbr0_enable();
		__dma_unmap_area((void *)(uintptr_t)uaddr, length, DMA_FROM_DEVICE);
		uaccess_ttbr0_disable();
		break;

	default:
		pr_info("%s: Invalidate CMD(0x%x)\n", __func__, cmd);
		ret = -EINVAL;
	}
	dma_buf_put(buf);
	return ret;
}

int mm_ion_cache_operate(int fd, unsigned long uaddr,
	unsigned long offset, unsigned long length, unsigned int cmd)
{
	int ret = 0;
	struct mm_struct *mm = NULL;
	unsigned long start = 0;

#ifndef CONFIG_MM_LB_FULL_COHERENCY
	if (length >= MM_ION_FLUSH_ALL_CPUS_CACHES_THRESHOLD) {
		ion_flush_all_cpus_caches();
		return 0;
	}
#endif

	start = uaddr + offset;
	if (uaddr > start) {
		pr_err("%s:  overflow start:0x%lx!\n", __func__, start);
		return -EINVAL;
	}

	mm = get_task_mm(current);
	if (!mm) {
		pr_err("%s: Invalid thread: %d\n", __func__, fd);
		return  -ENOMEM;
	}
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	down_read(&mm->mmap_lock);
#else
	down_read(&mm->mmap_sem);
#endif
	if (check_vaddr_bounds(mm, fd, start, length)) {
		pr_err("%s: invalid virt 0x%lx 0x%lx\n",
			__func__, start, length);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
		up_read(&mm->mmap_lock);
#else
		up_read(&mm->mmap_sem);
#endif
		mmput(mm);
		return -EINVAL;
	}

	ret = ion_do_cache_op(fd, start, length, cmd);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(5,10,0))
	up_read(&mm->mmap_lock);
#else
	up_read(&mm->mmap_sem);
#endif
	mmput(mm);
	return ret;
}

static int get_type_by_name(const char *name, enum ion_heap_type *type)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(ion_type_table); i++) {
		if (strcmp(name, ion_type_table[i].name))
			continue;
		*type = ion_type_table[i].type;
		return 0;
	}
	return -EINVAL;
}

static char *get_data_from_phandle_node(struct device_node *phandle_node,
			const char *status, struct device_node *np)
{
	int len, ret;
	struct property *prop = NULL;

	ret = of_property_read_string(phandle_node,
			"status", &status);
	if (!ret) {
		if (strncmp("ok", status, strlen("ok")))
			return NULL;
	}

	prop = of_find_property(phandle_node, "heap-name",
							&len);
	if (!prop) {
		pr_err("no heap-name in phandle of node %s\n",
		np->name);
		return NULL;
	}

	if (!prop->value || !prop->length) {
		pr_err("%s %s %d, node %s, invalid phandle, value=%pK,length=%d\n",
				 __FILE__, __func__, __LINE__,
				 np->name, prop->value,
				 prop->length);
		return NULL;
	}

	return prop->value;
}

static int get_property_need(const char *heap_name, struct device_node *np,
					struct platform_device *pdev, struct ion_platform_heap *p_data)
{
	int ret;
	unsigned int base = 0;
	unsigned int size = 0;
	unsigned int id = 0;
	const char *type_name = NULL;
	enum ion_heap_type type = 0;

	ret = of_property_read_u32(np, "heap-id", &id);
	if (ret < 0) {
		pr_err("check the id %s\n", np->name);
		return -1;
	}

	ret = of_property_read_u32(np, "heap-base", &base);
	if (ret < 0) {
		pr_err("check the base of node %s\n", np->name);
		return -1;
	}

	ret = of_property_read_u32(np, "heap-size", &size);
	if (ret < 0) {
		pr_err("check the size of node %s\n", np->name);
		return -1;
	}

	ret = of_property_read_string(np, "heap-type", &type_name);
	if (ret < 0) {
		pr_err("check the type of node %s\n", np->name);
		return -1;
	}
	ret = get_type_by_name(type_name, &type);
	if (ret < 0) {
		pr_err("type name error %s!\n", type_name);
		return -1;
	}

	p_data->name = heap_name;
	p_data->base = base;
	p_data->size = size;
	p_data->id = id;
	p_data->type = type;
	p_data->priv = (void *)&pdev->dev;
	return 0;
}

static void set_platform_heaps_value(struct ion_platform_heap *p_data,
										int index)
{
	if (!p_data->base && !p_data->size) {
		struct mm_ion_dynamic_area *area = NULL;

		pr_err("[%d] heap [%s] base =0, try to find dynamic area\n",
			index, p_data->name);
		area = find_dynamic_area_by_name(p_data->name);
		if (area) {
			p_data->base = area->base;
			p_data->size = area->size;
			pr_err("have found heap name %s base = %lx, size %zu\n",
				 p_data->name,
				 (unsigned long)p_data->base,
				 p_data->size);
		}
	}
}

static int mm_ion_set_platform_data(struct platform_device *pdev,
	const struct  device_node *dt_node,
	struct ion_platform_heap **platform_heaps)
{
	const char *heap_name = NULL;
	const char *status = NULL;
	int ret = 0;
	struct device_node *np = NULL;
	struct device_node *phandle_node = NULL;
	struct ion_platform_heap *p_data = NULL;
	int index = 0;

	pr_err("[node--           node_name--           heap_name--id--type-      base       --       size       .\n");
	for_each_child_of_node(dt_node, np) {
		ret = of_property_read_string(np, "status", &status);
		if (!ret) {
			if (strncmp("ok", status, strlen("ok")))
				continue;
		}

		phandle_node = of_parse_phandle(np, "heap-name", 0);
		if (phandle_node) {
			heap_name = get_data_from_phandle_node(phandle_node, status, np);
			if (!heap_name)
				continue;
		} else {
			ret = of_property_read_string(np, "heap-name",
					&heap_name);
			if (ret < 0) {
				pr_err("invalid heap-name in node [%s].\n",
					np->name);
				continue;
			}
		}
		p_data = devm_kzalloc(&pdev->dev,
					sizeof(struct ion_platform_heap),
					GFP_KERNEL);
		if (!p_data)
			return -ENOMEM;

		ret = get_property_need(heap_name, np, pdev, p_data);
		if (ret) {
			devm_kfree(&pdev->dev, p_data);
			continue;
		}

		set_platform_heaps_value(p_data, index);
		platform_heaps[index] = p_data;
		pr_err("%2d:%20s--%20s--%2d--%2d--[%16lx]--[%16lx]\n",
			index, np->name, heap_name, p_data->id, p_data->type,
			(unsigned long)platform_heaps[index]->base,
			platform_heaps[index]->size);
		index++;
	}
	num_heaps = index;
	return 0;
}

static struct ion_heap *ion_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_heap *heap = NULL;
	int i;

	if (heap_data == NULL) {
		pr_err("\t\t%s: Invalid heap \n", __func__);
		return ERR_PTR(-EINVAL);
	}

	for (i = 0; i < ARRAY_SIZE(g_heap_creat_tb); i++) {
		if (g_heap_creat_tb[i].type == heap_data->type) {
			heap = g_heap_creat_tb[i].heap_creat(heap_data);
			break;
		}
	}

	if (IS_ERR_OR_NULL(heap)) {
		pr_err("%s: error creating heap %s type %d base %lx size %lx\n",
				__func__,
				heap_data->name,
				heap_data->type,
				(unsigned long)heap_data->base,
				(unsigned long)heap_data->size);
		return ERR_PTR(-EINVAL);
	}

	heap->name = heap_data->name;
	heap->id = heap_data->id;
	return heap;
}

static ssize_t sys_pool_watermark_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret)
		return ret;

	if (val > MAX_WATER_MARK)
		val = MAX_WATER_MARK;

	pr_err("sys pool watermark store: %lu", val);

#ifdef CONFIG_ION_MEDIA_HEAP
	set_media_heap_mode(val);
#else
	set_sys_pool_watermark(val);
#endif

	return size;
}
static DEVICE_ATTR_WO(sys_pool_watermark);

static ssize_t sys_pool_dynamic_watermark_bytes_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t size)
{
	unsigned long val;
	int ret;

	ret = kstrtoul(buf, 0, &val);
	if (ret)
		return ret;

	if (val > MAX_PRE_ALLOC_SIZE)
		return -EINVAL;

	fill_system_heap(val);

	pr_info("fill system heap store: %lu", val);

	return size;
}
static DEVICE_ATTR_WO(sys_pool_dynamic_watermark_bytes);

static struct attribute *sys_pool_attrs[] = {
	&dev_attr_sys_pool_watermark.attr,
	&dev_attr_sys_pool_dynamic_watermark_bytes.attr,
	NULL,
};

static const struct attribute_group sys_pool_attr_group = {
	.name = NULL,
	.attrs = sys_pool_attrs,
};

static int mm_ion_probe(struct platform_device *pdev)
{
	int index;
	int err;
	static struct ion_platform_heap **pheaps;
	struct device_node *np = NULL;
	const struct device_node *dt_node = pdev->dev.of_node;
	struct ion_heap *new_ion_heap = NULL;

	err = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(64)); /*lint !e598 !e648 */
	if (err)
		pr_err("dma_set_mask_and_coherent error!\n");

	mm_ion_pdev = pdev;
	pr_info("%s set dma_mask to 64bit , dma_mask =%lx\n", __func__,
			 (unsigned long)*mm_ion_pdev->dev.dma_mask);

	/* Get heap node from pdev; */
	for_each_child_of_node(dt_node, np)
		num_heaps++;

	/* allocate ion_platform_heap buffers; */
	pheaps = devm_kzalloc(&pdev->dev,
				 sizeof(struct ion_platform_heap *) *
				 num_heaps,
				 GFP_KERNEL);
	if (!pheaps)
		return -ENOMEM;

	/* Get and set heap info to ion_pheaps; */
	err = mm_ion_set_platform_data(pdev, dt_node, pheaps);
	if (err) {
		devm_kfree(&pdev->dev, pheaps);
		pr_err("ion set platform data error!\n");
		return err;
	}
	pr_err("get platform heap info done!\n");
	/* Scan mm heaps to ION; */
	for (index = 0; index < num_heaps; index++) {
		new_ion_heap = ion_heap_create(pheaps[index]);
		if (IS_ERR_OR_NULL(new_ion_heap)) {
			pr_err("[%2d]err create[%20s]--[id:%d]--[type:%d]--[base:%lx @ size:%lx]\n",
					index,
					pheaps[index]->name,
					pheaps[index]->id,
					pheaps[index]->type,
					(unsigned long)pheaps[index]->base,
					(unsigned long)pheaps[index]->size);
			continue;
		}
		pr_info("[%2d] Create [%20s]--[id:%d]--[type:%d]--[base:%lx @ size:%lx]\n",
		 index,
		 pheaps[index]->name,
		 pheaps[index]->id,
		 pheaps[index]->type,
		 (unsigned long)pheaps[index]->base,
		 (unsigned long)pheaps[index]->size);

		ion_device_add_heap(new_ion_heap);

		pr_info("[%2d] Add    [%20s]--[id:%d]--[type:%d]--[base:%lx @ size:%lx]\n",
		 index,
		 new_ion_heap->name,
		 new_ion_heap->id,
		 new_ion_heap->type,
		 (unsigned long)pheaps[index]->base,
		 (unsigned long)pheaps[index]->size);
	}

	sec_alloc_init(pdev);

#ifdef CONFIG_MM_VLTMM
	smemheaps_init();
#endif

	err = sysfs_create_group(&pdev->dev.kobj, &sys_pool_attr_group);
	if (err)
		pr_err("create sys_pool attr fail!\n");

	register_bg_buffer_operation(&pdev->dev.kobj);

	return 0;
}

static const struct of_device_id mm_ion_match_table[] = {
	{.compatible = "hisilicon,mm-ion"},
	{},
};

static struct platform_driver mm_ion_driver = {
	.probe = mm_ion_probe,
	.driver = {
		.name = "ion-mm",
		.of_match_table = mm_ion_match_table,
	},
};

static int __init mm_ion_init(void)
{
	int ret;

	ret = platform_driver_register(&mm_ion_driver);

	return ret;
}

subsys_initcall(mm_ion_init);

