/*
 * dm-row.c
 *
 * storage Redirection On Write device-mapper
 *
 * Copyright (c) 2012-2019 Huawei Technologies Co., Ltd.
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

#include <linux/blkdev.h>
#include <linux/genhd.h>
#include <linux/device-mapper.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/ctype.h>
#include <linux/blk_types.h>
#include <linux/version.h>
#include <securec.h>
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
#include <linux/dax.h>
#endif

#ifdef CONFIG_STORAGE_ROW_SKIP_PART
#include <chipset_common/storage_rofa/storage_rofa.h>
#endif

#define DM_MSG_PREFIX "row"

#define SECTORS_PER_PAGE_SHIFT  (PAGE_SHIFT - SECTOR_SHIFT)
#define SECTORS_PER_PAGE		(1 << SECTORS_PER_PAGE_SHIFT)

typedef blk_qc_t local_submit_bio(struct bio *);
struct block_device_operations blk_dev_op;
struct dm_row {
	struct rw_semaphore lock;

	struct dm_dev *origin;
	struct dm_dev *cow;

	struct dm_target *ti;

	/* List of rows per Origin */
	struct list_head list;

	/* You can't use a row if this is 0 (e.g. if full). */
	int valid;

	/* Origin writes don't trigger exceptions until this is set */
	int active;

	/* Size of data blocks saved - must be a power of 2 */
	unsigned int chunk_size; /* keep same type as bi_size */
	sector_t chunk_mask; /* keep same type as bi_sector */
	unsigned int chunk_shift;

	/* Hook flag for origin block device's mrfn */
	unsigned int submtiofn_hook;
#define HOOK_MAGIC	  0x4B4F4F48

	/* submit_bio of origin block device */
	local_submit_bio *submtiofn;

	/* Fast lookup bitmap for written blocks mapping */
	unsigned long *wr_bitmap;

	/* pointer to the DM device itself */
	struct block_device *bdev_self;
};

static LIST_HEAD(g_rows_list);
static DECLARE_RWSEM(g_rows_sem);

/*
 * find the dm_row with the input arg as origin device
 */
static struct dm_row *lookup_row(struct gendisk *origin)
{
	struct dm_row *s = NULL;

	down_read(&g_rows_sem);

	list_for_each_entry(s, &g_rows_list, list) {
		if (s->origin->bdev->bd_disk == origin) {
			up_read(&g_rows_sem);
			return s;
		}
	}

	up_read(&g_rows_sem);
	return NULL;
}

/*
 * On success, returns 0.
 * return -EEXIST for already existed dm_row.
 */
static int register_row(struct dm_row *row)
{
	struct dm_row *s = NULL;

	down_write(&g_rows_sem);

	list_for_each_entry(s, &g_rows_list, list) {
		if (s->origin->bdev == row->origin->bdev) {
			up_write(&g_rows_sem);
			return -EEXIST;
		}
	}

	list_add_tail(&row->list, &g_rows_list);

	up_write(&g_rows_sem);
	return 0;
}

static void unregister_row(struct dm_row *row)
{
	struct dm_row *s = NULL;

	down_write(&g_rows_sem);

	list_for_each_entry(s, &g_rows_list, list) {
		if (s == row)
			list_del(&s->list);
	}

	up_write(&g_rows_sem);
}

/*
 * The following implements a ROW (Redirect On Write) type Device-Mapper
 */

/*
 * Forward declaration and external reference
 */
static inline int dm_target_is_row_bio_target(struct target_type *tt);
static blk_qc_t row_submit_bio(struct bio *bio);

static int dm_exception_store_build(struct dm_target *ti,
	int argc, char **argv, struct dm_row *row, unsigned int *args_used)
{
	int r = 0;
	char persistent;
	unsigned int chunk_size;

	if (argc < 2) {
		ti->error = "Insufficient exception store arguments";
		DMERR("Insufficient exception store arguments");
		return -EINVAL;
	}

	persistent = toupper(*argv[0]);
	if (persistent != 'P' && persistent != 'N') {
		ti->error = "Persistent flag is not P or N";
		DMERR("Persistent flag is not P or N");
		r = -EINVAL;
		goto bad_type;
	}

	if (kstrtouint(argv[1], 10, &chunk_size)) {
		ti->error = "Invalid chunk size";
		DMERR("Invalid chunk size");
		r = -EINVAL;
		goto bad_type;
	}

	if (!chunk_size) {
		row->chunk_size = 0;
		row->chunk_mask = 0;
		row->chunk_shift = 0;
		goto common_out;
	}

	/* Check chunk_size is a power of 2 */
	if (!is_power_of_2(chunk_size)) {
		ti->error = "Chunk size is not a power of 2";
		DMERR("Chunk size is not a power of 2");
		r = -EINVAL;
		goto bad_type;
	}

	if (chunk_size > INT_MAX >> SECTOR_SHIFT) {
		ti->error = "Chunk size is big";
		DMERR("Chunk size is big");
		r = -EINVAL;
		goto bad_type;
	}

	row->chunk_size = chunk_size;
	row->chunk_mask = chunk_size - 1;
	row->chunk_shift = ffs(chunk_size) - 1;

common_out:
	*args_used = 2;
	return 0;

bad_type:
	return r;
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0))
static inline void bio_set_dev(struct bio *bio, struct block_device *bdev)
{
	bio->bi_bdev = bdev;
}
#endif

/*
 * Return the number of sectors in the device.
 */
static inline sector_t get_dev_size(struct block_device *bdev)
{
	return i_size_read(bdev->bd_inode) >> SECTOR_SHIFT;
}

static inline sector_t sector_to_chunk(struct dm_row *row, sector_t sector)
{
	return sector >> row->chunk_shift;
}

/*
 * Attach to the origin_dev request queue
 */
static void row_attach_queue(struct dm_row *row)
{
	struct gendisk* disk = row->origin->bdev->bd_disk;
	memset_s(&blk_dev_op, sizeof(blk_dev_op), 0, sizeof(blk_dev_op));
	blk_dev_op = (struct block_device_operations)(*(disk->fops));
	if (dm_target_is_row_bio_target(row->ti->type)) {
		if (row->submtiofn_hook == 0) {
			row->submtiofn = blk_dev_op.submit_bio;
			blk_dev_op.submit_bio = row_submit_bio;
			disk->fops = &blk_dev_op;
			row->submtiofn_hook = HOOK_MAGIC;
		}
	}
}

/*
 * Detach from the origin_dev request queue
 * stop mrfn entry and restore mrfn as the original mrfn.
 */
static void row_detach_queue(struct dm_row *row)
{
	struct gendisk* disk = row->origin->bdev->bd_disk;
	memset_s(&blk_dev_op, sizeof(blk_dev_op), 0, sizeof(blk_dev_op));
	blk_dev_op = (struct block_device_operations)(*(disk->fops));
	if (dm_target_is_row_bio_target(row->ti->type)) {
	if (row->submtiofn_hook == HOOK_MAGIC) {
			blk_dev_op.submit_bio = row->submtiofn;
			disk->fops = &blk_dev_op;
			row->submtiofn_hook = 0;
		}
	}
}

static void row_fix_bio_uieinfo(struct bio *bio, const struct bio *base_bio)
{
#ifdef CONFIG_F2FS_FS_ENCRYPTION
#if !defined(CONFIG_HUAWEI_STORAGE_ROFA_FOR_MTK) && !defined(CONFIG_BOOT_DETECTOR_QCOM)
	unsigned int bvec_off;
	sector_t sec_downrnd;

	bio->ci_key = base_bio->ci_key;
	bio->ci_key_len = base_bio->ci_key_len;
	bio->ci_key_index = base_bio->ci_key_index;
	bio->index = 0;
	bio->ci_metadata = base_bio->ci_metadata;

	if (base_bio->ci_key_len && base_bio->ci_key) {
		/* bio vector offset, bio ci index is page aligned */
		sec_downrnd = base_bio->bi_iter.bi_sector & ~0x07;
		bvec_off = (bio->bi_iter.bi_sector - sec_downrnd) >> 3;
		bio->index = base_bio->index + bvec_off;
	}
#endif
#endif
}

#ifdef CONFIG_STORAGE_ROW_SKIP_PART
static int should_skip_row(struct bio *bio)
{
	int i;
	int ret;
	int num;
	struct hd_struct *part = NULL;
	static const char * const skip_partitions[] = {
		"log",
		"misc",
		"rrecord",
		"splash2",
		"bootfail_info",
	};

	if (!is_bopd_row_skip_part())
		return 0;

	if ((bio->bi_opf & REQ_OP_WRITE) &&
		(bio->bi_opf & REQ_PREFLUSH) &&
		!bio->bi_iter.bi_size) {
		DMINFO("send flush to disk");
		return 1;
	}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	if (!bio->bi_partno && bio->bi_orig_partno)
		part = disk_get_part(bio->bi_disk, bio->bi_orig_partno);
	else
		part = disk_get_part(bio->bi_disk, bio->bi_partno);
#else
	part = bio->bi_bdev->bd_part;
#endif

	ret = 0;
	if (unlikely(!part || !part->info)) {
		DMERR("%s:get part of %s %d %d failed",
			__func__, bio->bi_disk->disk_name,
			bio->bi_partno, bio->bi_orig_partno);
		dump_stack();
		goto out;
	}

	num = ARRAY_SIZE(skip_partitions);
	for (i = 0; i < num; i++) {
		if (!strcmp(part->info->volname, skip_partitions[i])) {
			ret = 1;
			DMINFO("write to %s on disk", skip_partitions[i]);
			goto out;
		}
	}

out:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	disk_put_part(part);
#endif
	return ret;
}
#endif

/*
 * Origin device's make_request_fn replacer as bio transfer
 */
static blk_qc_t row_submit_bio(struct bio *bio)
{
	struct dm_row *s = NULL;
	struct gendisk* disk;

	/*
	 * if the bio has no sectors, blk_partition_remap does not do
	 * top partition remap, so we use bd_contains as top blkdev.
	 * while the bio has sectors, bi_sector and bi_bdev is top
	 * blkdev remapped.
	 */
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	s = lookup_row(bio->bi_disk);
#else
	s = lookup_row(bio->bi_bdev->bd_contains->bd_disk);
#endif
	if (s == NULL) {
		DMERR("Lookup failed");
		return BLK_QC_T_NONE; // lint !e501
	}

#ifdef CONFIG_STORAGE_ROW_SKIP_PART
	if (should_skip_row(bio)) {
		if (likely(s->submtiofn))
			s->submtiofn(bio);
		else
			DMERR("no mrfn\n");
		return BLK_QC_T_NONE;
	}
#endif

	disk = s->bdev_self->bd_disk;
	disk->fops->submit_bio(bio);

	return BLK_QC_T_NONE;
}

/*
 * non-exclusive get block_device
 */
static int dm_row_get_device(struct dm_target *ti,
	const char *path, fmode_t mode, struct dm_dev **result)
{
	dev_t dev;
	struct dm_dev *ddev = NULL;
	struct block_device *bdev = NULL;

	dev = dm_get_dev_t(path);
	if (!dev) {
		ti->error = "Get devt failed";
		DMERR("Get devt failed");
		return -ENODEV;
	}

	ddev = kmalloc(sizeof(*ddev), GFP_KERNEL);
	if (!ddev) {
		ti->error = "Alloc dm_dev failed";
		return -ENOMEM;
	}

	ddev->mode = mode;
	ddev->bdev = NULL;

	bdev = blkdev_get_by_dev(dev, ddev->mode, NULL);
	if (IS_ERR(bdev)) {
		ti->error = "Get blkdev by devt failed";
		DMERR("Get blkdev by devt failed");
		kfree(ddev);
		return PTR_ERR(bdev);
	}

	ddev->bdev = bdev;
	format_dev_t(ddev->name, dev);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	ddev->dax_dev = dax_get_by_host(bdev->bd_disk->disk_name);
#endif
	*result = ddev;
	return 0;
}

static void dm_row_put_device(struct dm_target *ti, struct dm_dev *d)
{
	(void)ti;

	if (!d->bdev)
		return;

	blkdev_put(d->bdev, d->mode);
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	put_dax(d->dax_dev);
	d->dax_dev = NULL;
#endif
	d->bdev = NULL;
	kfree(d);
}

/*
 * Construct a row mapping: <origin_dev> <COW-dev> <P/N> <chunk_size>
 */
static int row_ctr(struct dm_target *ti, unsigned int argc, char **argv)
{
	struct dm_row *s = NULL;
	char *origin_path = NULL;
	char *cow_path = NULL;
	dev_t origin_dev;
	dev_t cow_dev;
	unsigned int args_used;
	unsigned long bmp_len;
	fmode_t origin_mode = FMODE_READ;
	int err = -EINVAL;

	DMINFO("%s: Construct row mapped-device enter", __func__);

	if (argc != 4) {
		ti->error = "DM row requires exactly 4 arguments";
		DMERR("DM row requires exactly 4 arguments");
		return err;
	}

	s = kmalloc(sizeof(struct dm_row), GFP_KERNEL);
	if (!s) {
		ti->error = "Cannot allocate private dm_row structure";
		DMERR("Cannot allocate private dm_row structure");
		err = -ENOMEM;
		return err;
	}

	origin_path = argv[0];
	argv++;
	argc--;

	err = dm_row_get_device(ti, origin_path, origin_mode, &s->origin);
	if (err) {
		ti->error = "Cannot get origin device";
		DMERR("Cannot get origin device");
		goto bad_origin;
	}
	origin_dev = s->origin->bdev->bd_dev;

	cow_path = argv[0];
	argv++;
	argc--;

	cow_dev = dm_get_dev_t(cow_path);
	if (cow_dev && cow_dev == origin_dev) {
		ti->error = "COW device cannot be the same as origin device";
		DMERR("COW device cannot be the same as origin device");
		goto bad_cow;
	}

	err = dm_get_device(ti, cow_path, dm_table_get_mode(ti->table),
		&s->cow);
	if (err) {
		ti->error = "Cannot get COW device";
		DMERR("Cannot get COW device");
		goto bad_cow;
	}

	err = dm_exception_store_build(ti, argc, argv, s, &args_used);
	if (err) {
		ti->error = "Couldn't create exception store";
		DMERR("Couldn't create exception store");
		goto bad_mapinfo;
	}
	if (s->chunk_size != SECTORS_PER_PAGE) {
		ti->error = "Chunk size must equal to PAGE_SIZE";
		DMERR("Chunk size must equal to PAGE_SIZE");
		err = -EINVAL;
		goto bad_mapinfo;
	}

	argv += args_used;
	argc -= args_used;

	s->ti = ti;
	s->valid = 1;
	s->active = 0;
	init_rwsem(&s->lock);
	INIT_LIST_HEAD(&s->list);

	/* Allocate bitmap table for COW data */
	bmp_len = BITS_TO_LONGS(sector_to_chunk(s, ti->len));
	/*
	 * for a 16TB disk, needed bitmap size is 16TB / (4K * 8) = 512MB.
	 * use that as an upper limit
	 */
#define ROW_BITMAP_LEN (512 * 1024 * 1024UL)
	if (bmp_len > (ROW_BITMAP_LEN / sizeof(long))) {
		ti->error = "Origin disk size is too large";
		DMERR("Origin disk size is too large");
		err = -EINVAL;
		goto bad_mapinfo;
	}
	s->wr_bitmap = vzalloc(bmp_len * sizeof(long));
	if (!s->wr_bitmap) {
		ti->error = "Could not allocate block mapping bitmap";
		DMERR("Could not allocate block mapping bitmap");
		err = -ENOMEM;
		goto bad_mapinfo;
	}
	s->bdev_self = NULL;
	s->submtiofn_hook = 0;

	ti->private = s;
	ti->num_flush_bios = 1;
	ti->flush_supported = true;
	ti->num_discard_bios = 1;
	ti->discards_supported = false;
	ti->per_io_data_size = 0;

	/* Add row to the list of snapshots for this origin */
	err = register_row(s);
	if (err == -EEXIST) {
		ti->error = "Existing dm-row for origin device";
		DMERR("Existing dm-row for origin device");
		goto exist_origin;
	}

	err = dm_set_target_max_io_len(ti, s->chunk_size);
	if (err) {
		ti->error = "Set dm-row target max io len failed";
		DMERR("Set dm-row target max io len failed");
		goto invalid_chunk_size;
	}

	DMINFO("%s: Construct row mapped-device ok", __func__);
	return 0;

invalid_chunk_size:
	unregister_row(s);

exist_origin:
	vfree(s->wr_bitmap);

bad_mapinfo:
	dm_put_device(ti, s->cow);

bad_cow:
	dm_row_put_device(ti, s->origin);

bad_origin:
	kfree(s);
	return err;
}

static void row_status(struct dm_target *ti, status_type_t type,
	unsigned int status_flags, char *result, unsigned int maxlen)
{
	struct dm_row *row = ti->private;
	unsigned int sz = 0;

	DMINFO("%s: Status query row mapped-device", __func__);

	switch (type) {
	case STATUSTYPE_INFO:
		down_read(&row->lock);

		if (!row->valid)
			DMEMIT("Invalid");
		else
			DMEMIT("Unknown");

		up_read(&row->lock);
		break;

	case STATUSTYPE_TABLE:
		/*
		 * kdevname returns a static pointer so we need
		 * to make private copies if the output is to
		 * make sense.
		 */
		down_read(&row->lock);

		DMEMIT("%s %s", row->origin->name, row->cow->name);
		DMEMIT(" N %llu", (unsigned long long)row->chunk_size);

		up_read(&row->lock);
		break;
	}
}

static int row_iterate_devices(struct dm_target *ti,
	iterate_devices_callout_fn fn, void *data)
{
	struct dm_row *row = ti->private;
	int r;

	r = fn(ti, row->origin, 0, ti->len, data);
	if (!r)
		r = fn(ti, row->cow, 0, get_dev_size(row->cow->bdev), data);

	return r;
}

static void row_resume(struct dm_target *ti)
{
	struct dm_row *s = ti->private;
	int err = 0;

	DMINFO("%s: Resume row mapped-device", __func__);

	/* Hold the dm device and its queue */
	down_write(&s->lock);

	s->active = 1;

	if (s->bdev_self != NULL)
		goto out_unlock;
	s->bdev_self = bdget_disk(dm_disk(dm_table_get_md(ti->table)), 0);
	if (s->bdev_self == NULL) {
		DMERR("Get dm-row device blockdev failed");
		goto out_unlock;
	}
	err = blkdev_get(s->bdev_self, FMODE_READ | FMODE_WRITE, NULL);
	if (err) {
		DMERR("Open dm-row device itself failed");
		/* bd has been put in __blkdev_get if failed */
		s->bdev_self = NULL;
		goto out_unlock;
	}

out_unlock:
	up_write(&s->lock);
}

/*
 * Release the row dm itself before suspend
 */
static void row_presuspend(struct dm_target *ti)
{
	struct dm_row *s = ti->private;

	DMINFO("%s: Presuspend row mapped-device", __func__);

	down_write(&s->lock);

	s->active = 0;

	if (s->bdev_self) {
		blkdev_put(s->bdev_self, FMODE_READ | FMODE_WRITE);
		s->bdev_self = NULL;
	}

	up_write(&s->lock);
}

static void row_dtr(struct dm_target *ti)
{
	struct dm_row *s = ti->private;

	DMINFO("%s: Deconstruct row mapped-device", __func__);

	/* Invalidate the device first */
	down_write(&s->lock);
	s->valid = 0;
	up_write(&s->lock);

	/* Prevent further origin writes from using this row. */
	unregister_row(s);

	/*
	 * Ensure instructions in mempool_destroy aren't reordered
	 * before atomic_read.
	 */
	smp_mb();

	vfree(s->wr_bitmap);

	dm_put_device(ti, s->cow);

	/*
	 * detach the original device queue when destructed
	 * during row and original device put. have no other chance
	 * to get the detach_queue message.
	 */
	row_detach_queue(s);

	dm_row_put_device(ti, s->origin);

	kfree(s);
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
static int row_message(struct dm_target *ti, unsigned int argc, char **argv, char *result, unsigned maxlen)
#else
static int row_message(struct dm_target *ti, unsigned int argc, char **argv)
#endif
{
	struct dm_row *s = ti->private;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 19, 0))
	(void *)(result);
	(void)(maxlen);
#endif
	DMINFO("%s: Message row mapped-device", __func__);

	if (argc != 1) {
		DMERR("%s: Invalid message count : %u", __func__, argc);
		return -EINVAL;
	}

	if (strcasecmp(argv[0], "attach_queue") == 0) {
		if (s->active == 0) {
			DMERR("Device inactive for message '%s'", argv[0]);
			return -EINVAL;
		}
		row_attach_queue(s);
		return 0;
	} else if (strcasecmp(argv[0], "detach_queue") == 0) {
		/* shall know what you are doing when using this message */
		if (s->active == 0) {
			DMERR("Device inactive for message '%s'", argv[0]);
			return -EINVAL;
		}
		row_detach_queue(s);
		return 0;
	}

	DMERR("%s: Unsupported message '%s'", __func__, argv[0]);
	return -EINVAL;
}

/*
 * A copy version of same routine in bio.c
 * Even we call mrfn directly, we can not sure the lower does not
 * call generic_make_request.
 * The safe way is to call generic_make_request and use bi_end_io callback
 */
struct submit_bio_ret {
	struct completion event;
	int error;
};

static void submit_bio_wait_endio(struct bio *bio)
{
	struct submit_bio_ret *ret = bio->bi_private;

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	ret->error = bio->bi_status;
#else
	ret->error = bio->bi_error;
#endif
	complete(&ret->event);
}

static int submit_bio_mkreq_wait(local_submit_bio *mkreq_fn, struct bio *bio)
{
	struct submit_bio_ret ret;

	init_completion(&ret.event);
	bio->bi_private = &ret;
	bio->bi_end_io = submit_bio_wait_endio;
	mkreq_fn(bio);
	wait_for_completion(&ret.event);

	return ret.error;
}

/*
 * bio_alloc_pages - allocates a single page for each bvec in a bio
 * @bio: bio to allocate pages for
 * @page_nr: number of pages to add, may less than @bio->bi_max_vecs
 *
 * Allocates pages and update @bio->bi_vcnt.
 *
 * Returns 0 on success, On failure, any allocated pages are freed.
 */
static int bio_add_pages(struct bio *bio, unsigned int pg_num)
{
	struct page *page = NULL;
	struct bio_vec *bvec = NULL;
	int i;
	int err = 0;
	struct bvec_iter_all iter_all;

	for (i = 0; i < pg_num; ++i) {
		page = alloc_page(GFP_KERNEL);
		if (!page) {
			err = -ENOMEM;
			break;
		}

		if (!bio_add_page(bio, page, PAGE_SIZE, 0)) {
			__free_page(page);
			err = -EIO;
			break;
		}
	}

	if (err) {
		bio_for_each_segment_all(bvec, bio, iter_all)
			__free_page(bvec->bv_page);
	}

	return err;
}

/*
 * A copy version of the same routine in bio.c
 */
static void bio_delete_pages(struct bio *bio)
{
	struct bio_vec *bvec = NULL;
	struct bvec_iter_all iter_all;

	bio_for_each_segment_all(bvec, bio, iter_all)
		__free_page(bvec->bv_page);
}

/*
 * Only used to check the cloned split bio which has only one
 * bio_vec and is no more than PAGE_SZIE
 */
static inline int row_bio_full_chunk(struct dm_row *row, struct bio *bio)
{
	unsigned int chunk_size = row->chunk_size << SECTOR_SHIFT;

	/* redundant check condition */
	if (bio->bi_iter.bi_size == chunk_size &&
		bio_iter_len(bio, bio->bi_iter) == chunk_size &&
		bio_offset(bio) == 0)
		return 1;
	else
		return 0;
}

/*
 * Read the whole chunk from original device and fill the cow device.
 * just for special case of some superblock write and real vfat fs write.
 * eg. sb may not page aligned and only has 2 sectors.
 * Because the sectors of chunk is ensured same as SECTORS_PER_PAGE,
 * the cieinfo fix is safe.
 */
static int row_read_fill_chunk(struct dm_row *s, struct bio *ref)
{
	struct bio *bio = NULL;
	sector_t sector;
	unsigned int pg_cnt;
	struct request_queue *q = NULL;
	int err = 0;

	pg_cnt = s->chunk_size >> SECTORS_PER_PAGE_SHIFT;

	bio = bio_alloc(GFP_KERNEL, pg_cnt);
	if (!bio) {
		DMERR("%s: Allocate bio failed", __func__);
		return -ENOMEM;
	}

	/*
	 * bi_bdev and bi_iter.bi_sector must have values for
	 * linux version < 4.4
	 */
	sector = ref->bi_iter.bi_sector & ~s->chunk_mask;
	bio_set_dev(bio, s->origin->bdev);
	bio->bi_opf = REQ_OP_READ | REQ_SYNC;
	bio->bi_iter.bi_sector = sector;

	err = bio_add_pages(bio, pg_cnt);
	if (err) {
		DMERR("%s: Allocate or add bio pages failed", __func__);
		goto free_bio;
	}

	row_fix_bio_uieinfo(bio, ref);

	/* read from original device */
	q = bdev_get_queue(s->origin->bdev);
	err = submit_bio_mkreq_wait(s->submtiofn, bio);
	if (err) {
		DMERR("%s: Queue bio[RD] to origin dev failed", __func__);
		goto free_pages;
	}

	/* write to cow device */
	bio_reset(bio);
	bio_set_dev(bio, s->cow->bdev);
#ifdef CONFIG_HUAWEI_STORAGE_ROFA_FOR_MTK
	bio->bi_opf = REQ_OP_WRITE | REQ_SYNC;
#else
	bio->bi_opf = REQ_OP_WRITE | REQ_SYNC;
#endif
	bio->bi_iter.bi_sector = sector;
	bio->bi_iter.bi_size = pg_cnt * PAGE_SIZE;

	/* reassign bi_vcnt reset by bio_reset */
	bio->bi_vcnt = pg_cnt;

	row_fix_bio_uieinfo(bio, ref);

	/*
	 * invoke the dest target mrfn directly rather than
	 * call generic_make_request which put the request to
	 * current->bio_list and only return to wait
	 * when the current request is blocked
	 */
	struct gendisk* disk = s->cow->bdev->bd_disk;
	err = submit_bio_mkreq_wait(disk->fops->submit_bio, bio);
	if (err)
		DMERR("%s: Queue bio[WR] to cow dev failed", __func__);

free_pages:
	bio_delete_pages(bio);

free_bio:
	bio_put(bio);
	return err;
}

static int row_map(struct dm_target *ti, struct bio *bio)
{
	struct dm_row *s = ti->private;
	int err = DM_MAPIO_REMAPPED;
	sector_t chunk;

	down_write(&s->lock);

	/* To get here the table must believe so s->active is always set */
	if (!s->valid || !s->active) {
		DMERR("%s: Device dm-row is NOT valid or active", __func__);
		err = -EIO;
		goto out_unlock;
	}

	/*
	 * zero count sectors condition includes condition of
	 * bio->bi_iter.bi_sector == (sector_t)-1
	 * has no data, ensured in __send_empty_flush @ dm.c
	 */
	if (bio_sectors(bio) == 0 || (bio->bi_opf & REQ_PREFLUSH))
		goto out_endio;

	chunk = sector_to_chunk(s, bio->bi_iter.bi_sector);

	if (unlikely(bio_op(bio) == REQ_OP_DISCARD)) {
		sector_t chunk_end;

		/*
		 * redirect discarded sectors chunk to cow dev with sector
		 * range as large as possible. no need to split bio because
		 * no sectors payload.
		 */
		chunk_end = sector_to_chunk(s, bio_end_sector(bio));
		while (chunk < chunk_end) {
			set_bit(chunk, s->wr_bitmap);
			++chunk;
		}

		bio_set_dev(bio, s->cow->bdev);
		goto out_remapped;
	}

	/* Fix and add the inline cryption key info HERE to the cloned bio */
	row_fix_bio_uieinfo(bio, dm_get_tio_bio(bio));

	/* If the block is already remapped - use that, or else remap it */
	if (test_bit(chunk, s->wr_bitmap)) {
		bio_set_dev(bio, s->cow->bdev);
		goto out_remapped;
	}
	/*
	 * the above is that if bitmap set, read/write data from/to cow.
	 * or else, set bitmap and write to cow device, read from
	 * original device for read request
	 */

	if (bio_data_dir(bio) == WRITE) {
		if (unlikely(row_bio_full_chunk(s, bio) == 0)) {
			int res;

			DMINFO("%s: Recv bio[WR] NOT chunk aligned", __func__);
			res = row_read_fill_chunk(s, bio);
			if (res) {
				DMERR("%s: Overlap chunk failed", __func__);
				err = res;
				goto out_unlock;
			}
		}

		set_bit(chunk, s->wr_bitmap);
		bio_set_dev(bio, s->cow->bdev);
		goto out_remapped;
	} else {
		bio_set_dev(bio, s->origin->bdev);
		/*
		 * always SUBMITTED for avoid infinite remapping loop.
		 * we can do some duplicate check both in mrfn redirection pipe
		 * and in this map handler. that is not a problem,
		 * but we want to simply use the mrfn pipe implementation
		 */
		s->submtiofn(bio);
		goto out_submitted;
	}

out_endio:
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0))
	if (bio->bi_disk == NULL)
		bio_set_dev(bio, s->bdev_self);
#endif
	bio_endio(bio);
out_submitted:
	err = DM_MAPIO_SUBMITTED;
	goto out_unlock;

out_remapped:
	/*
	 * this out_remapped is remapped out to dm cow in fact.
	 * these small cloned BIOs including discard bios have not once
	 * entered the queue and can pass the generic_make_request_checks.
	 */
	err = DM_MAPIO_REMAPPED;

out_unlock:
	up_write(&s->lock);
	return err;
}

static struct target_type row_target = {
	.name	= "row",
	.version = {1, 0, 0},
	.module  = THIS_MODULE,
	.ctr	 = row_ctr,
	.dtr	 = row_dtr,
	.map	 = row_map,
	.presuspend = row_presuspend,
	.message = row_message,
	.resume  = row_resume,
	.status  = row_status,
	.iterate_devices = row_iterate_devices,
};

static inline int dm_target_is_row_bio_target(struct target_type *tt)
{
	return (tt == &row_target);
}

static int __init dm_row_init(void)
{
	int r;

	r = dm_register_target(&row_target);
	if (r < 0) {
		DMERR("DM row target register failed, err %d", r);
		return r;
	}

	return 0;
}

static void __exit dm_row_exit(void)
{
	dm_unregister_target(&row_target);
}

/* Module hooks */
module_init(dm_row_init);
module_exit(dm_row_exit);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION(DM_NAME " row target driver");
MODULE_AUTHOR("Huawei Technologies Co., Ltd.");
MODULE_ALIAS("dm-row");
