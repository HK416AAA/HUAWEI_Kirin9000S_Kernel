/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2016-2022. All rights reserved.
 * Description: root monitor common
 * Create: 2016-04-01
 */

#ifndef _ROOT_COMMON_H_
#define _ROOT_COMMON_H_

#include <linux/module.h>
#include <linux/sched.h>
#include <securec.h>
#include "teek_client_type.h"
#include <chipset_common/security/hw_kernel_stp_interface.h>

#include "hw_rscan_interface.h"
#define SCAN_CYCLE (10 * 60 * 1000) /* 10mins */

#define AR_WAKELOCK_TIMEOUT 1000

#define RM_PRE_ALLOCATE_SIZE SZ_4K

#define ROOTAGENT_TAG "rootagent-ca"

enum {
	ROOTAGENT_DEBUG_ERROR   = 1U << 0,
	ROOTAGENT_DEBUG_AGENT   = 1U << 1,
	ROOTAGENT_DEBUG_API     = 1U << 2,
	ROOTAGENT_DEBUG_CRYPTO  = 1U << 3,
	ROOTAGENT_DEBUG_MEM     = 1U << 4,
	ROOTAGENT_DEBUG_DUMPHEX = 1U << 5,
	ROOTAGENT_DEBUG_DUMMY   = 1U << 6,
	DEBUG_NEED_2_DELETE     = 1U << 7,
};

#ifdef CONFIG_TEE_ANTIROOT_CLIENT_ENG_DEBUG
#define antiroot_debug(fmt, args...) \
	printk_ratelimited("[debug]%s: %s: %d: " fmt "\n", \
		ROOTAGENT_TAG, __func__, __LINE__, ## args)
#else
#define antiroot_debug(fmt, args...)
#endif

#define antiroot_error(fmt, args...) pr_err("[error]%s: %s: %d: " fmt "\n", \
		ROOTAGENT_TAG, __func__, __LINE__, ## args)

#define antiroot_warning(fmt, args...) pr_warn("[warn]%s: %s: %d: " fmt "\n", \
		ROOTAGENT_TAG, __func__, __LINE__, ## args)

#define antiroot_info(fmt, args...) pr_info("[info]%s: %s: %d: " fmt "\n", \
		ROOTAGENT_TAG, __func__, __LINE__, ## args)

#define MAGIC   0x53525748
#define VERSION 1

#define KERNEL_CODE_LENGTH_SHA   HASH_DIGEST_SIZE
#define SYSTEM_CALL_LENGTH_SHA   HASH_DIGEST_SIZE
#define SELINUX_HOOKS_LENGTH_SHA HASH_DIGEST_SIZE
#define RO_DATA_LENGTH_SHA       HASH_DIGEST_SIZE

#define CHALLENGE_MAX_LENGTH     16
#define CHALLENGE_KEY_LENGTH     32
#define CHALLENGE_REQ_KEY_LENGTH CHALLENGE_KEY_LENGTH
#define RESPONSE_COUNTER_LENGTH  CHALLENGE_KEY_LENGTH
#define CHALLENGE_NOUNCE_LENGTH  CHALLENGE_KEY_LENGTH

#define HASH_LENGTH     32
#define FN_LENGTH       256
#define USECASE_LENGTH  20
#define RESERVED_LENGTH 24

/*
 * TEEK_InvokeCommand buffer (Max 4K):
 * IV + struct ragent_command + RPROCS
 * 16   max320                  max 3680
 */
/* MAX_RPROC_SIZE is defined in hw_rscan_interface.h or rscan_dummy.h */
#define ROOTAGENT_RPROCS_MAX_LENGTH  3680
#define IV_SIZE                      16
/* 320 is reserved for sizeof(struct ragent_command) */
#define ANTIROOT_SRC_LEN             (320 + ROOTAGENT_RPROCS_MAX_LENGTH)
#define ANTIROOT_DST_LEN             (IV_SIZE + ANTIROOT_SRC_LEN)

/*
 * @ingroup root agent cmd
 *
 * @brief: root agent command id
 *
 */
enum root_agent_cmd {
	ROOTAGENT_CONFIG_ID      = 0x01, /* set the white information */
	ROOTAGENT_CHALLENGE_ID   = 0x02, /* request a challenge */
	ROOTAGENT_RESPONSE_ID    = 0x03, /* return the scanning result */
	ROOTAGENT_KERNEL_ADDR_ID = 0x06,
	ROOTAGENT_PAUSE_ID       = 0x07,
	ROOTAGENT_RESUME_ID      = 0x08,
#ifdef CONFIG_TEE_ANTIROOT_CLIENT_ENG_DEBUG
	TEE_STATUS_TEST          = 0x09,
#endif
};

enum root_agent_opid {
	RAOPID_TERM = 0,
	RAOPID_KCODE,
	RAOPID_SYSCALL,
	RAOPID_RPROCS,
	RAOPID_SELINUX_STATUS,
	RAOPID_SELINUX_HOOK,
	RAOPID_SETID,
	RAOPID_RODATA,
	RAOPID_SELINUX_POLICY,
	RAOPID_HIDDENTHREAD,
	RAOPID_FRAMEWORK,
	RAOPID_SYSAPP,
	RAOPID_CPULOAD,
	RAOPID_BATTERY,
	RAOPID_CHARGING,
	RAOPID_TIME,
	RAOPID_NOOP,
	RAOPID_MAXID
};

enum {
	/* AntiRoot Response verify failed */
	TEE_ERROR_ANTIROOT_RSP_FAIL     = 0xFFFF9110,
	/* AntiRoot ERROR during invokecmd */
	TEE_ERROR_ANTIROOT_INVOKE_ERROR = 0xFFFF9111
};

#define REV_NOT_ROOT    0x0   /* TEE_SUCCESS */
#define REV_ROOTED      TEE_ERROR_ANTIROOT_RSP_FAIL

struct ragent_whitelist {
	/* build version of REE, 0 for ENG, other for USER */
	uint32_t dstatus;
	uint8_t kcodes[KERNEL_CODE_LENGTH_SHA];   /* kernel code hash */
	uint8_t syscalls[SYSTEM_CALL_LENGTH_SHA]; /* system call table hash */
	uint32_t selinux;                         /* SE-Linux state */
	/* struct security_operations hash */
	uint8_t sehooks[SELINUX_HOOKS_LENGTH_SHA];
	uint32_t proclength;
	uint32_t setid;
	uint8_t rodata[RO_DATA_LENGTH_SHA];  /* read only data section hash */
};

struct ragent_cipher_key {
	/* aes key cipher challenge request */
	uint8_t cha_req_key[CHALLENGE_REQ_KEY_LENGTH];
	/* aes key cipher challenge */
	uint8_t cha_key[CHALLENGE_KEY_LENGTH];
};

struct ragent_config {
	struct ragent_cipher_key cipher_key; /* cipher key */
	struct ragent_whitelist white_list; /* white listi */
};

#ifndef UINT16_MAX
#define UINT16_MAX ((uint16_t)~0U)
#endif

/*
 * @ingroup  ragent_challenge
 *
 * @brief: challenge information
 *
 */
struct ragent_challenge {
	/* CPU Usage 0~100 */
	uint32_t cpuload;
	/* remaining battery 0~100 */
	uint32_t battery;
	/* Charging or not 0~1 */
	uint32_t charging;
	/* Current time in UNIX Epoch time. same as the time() systemcall */
	uint32_t time;
	/*
	 * Timezone in minutes.
	 * see tz_minuteswest in gettimeofday().
	 * e.g. Beijing +8 is -480
	 */
	uint32_t timezone;
	/* nounce as key for responce */
	uint8_t nounce[CHALLENGE_NOUNCE_LENGTH];
	/* challenge association */
	uint32_t challengeid[CHALLENGE_MAX_LENGTH];
};

struct ragent_response {
	/* white information in runtime */
	struct ragent_whitelist runtime_white_list;
	/* integrated of process chain */
	uint32_t proc_integrated;
	/* Heartbeat */
	uint32_t noop;
};

/*
 * @ingroup  TEE_RM_CONTENT
 *
 * @brief: specific content
 *
 */
union ragent_content {
	struct ragent_config config; /* white information */
	struct ragent_challenge challenge; /* challenge generated by TEE */
	struct ragent_response response; /* response (the result of scanning) */
};

/*
 * @ingroup  TEE_RM_COMMAND
 *
 * @brief: root monitor command information.
 *
 */
struct ragent_command {
	uint32_t magic;   /* scanner identify */
	uint32_t version; /* version */
	/*
	 * interface type,
	 * corresponding to content
	 */
	uint32_t interface;
	/*
	 * specific content
	 * (configuration,
	 * challenge, response),
	 * depend on interface
	 */
	union ragent_content content;
};

/*
 * @ingroup  TEE_RM_ROOTPROC
 *
 * @brief: root process list
 *
 */
struct ragent_rootproc {
	uint32_t length;
	uint8_t *procid;
};
#endif
