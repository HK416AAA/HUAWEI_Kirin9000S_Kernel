#ifndef FREQDUMP_PLAT_COMMON_H
#define FREQDUMP_PLAT_COMMON_H 
#define bit_value(i,n) ((i) & (1 << (n)))
enum MODULE_TEMP {
 LITTLE_CLUSTER_TEMP = 0,
 MIDDLE_CLUSTER_TEMP,
 BIG_CLUSTER_TEMP,
 GPU_TEMP,
 PERI_TEMP,
 LITE_TEMP,
 MAX_MODULE_TEMP,
};
enum GPU_STATUS {
 GPU_STATUS_OFF = 0,
 GPU_STATUS_ON,
 GPU_STATUS_MAX,
};
enum FREQDUMP_MODULE {
 FREQ_CLOCK = 0x1,
 FREQ_CPU = 0x2,
 FREQ_DDR = 0x4,
 FREQ_L3 = 0x8,
 FREQ_PERI = 0x10,
 FREQ_NPU = 0x20,
 FREQ_GPU = 0x40,
 FREQ_TEMP = 0x80,
 FREQ_NULL = 0x0,
};
enum FREQ_SMC_SUB_ID {
 FREQ_READ_DATA,
 NODE_DUMP_CLUSTER0,
 NODE_DUMP_CLUSTER1,
 NODE_DUMP_CLUSTER2,
 NODE_DUMP_CLUSTER3,
 NODE_DUMP_CLUSTER4,
 NODE_DUMP_CLUSTER5,
 NODE_DUMP_GPU,
 NODE_DUMP_L3,
 NODE_DUMP_DDR,
 NODE_DUMP_NPU,
 NODE_DUMP_PERI,
};
enum FREQ_PLAT_FLAG {
 FREQ_ES = 0,
 FREQ_CS = 1,
};
#define MAX_PERI_VOTE_TYPE 6
#define MAX_PERI_MEM_VOTE_TYPE 5
#define POWER_STATE 16
#define bit_count(n) (((((n) - (((n) >> 1) & 033333333333) - \
        (((n) >> 2) & 011111111111)) + \
        (((n) - (((n) >> 1) & 033333333333) - \
        (((n) >> 2) & 011111111111)) >> 3)) & 030707070707) % 63)
#define MAX_SENSOR_NUM 32
#define MONITOR_STAT_NUM 3
#define SHARED_MEMORY_OFFSET 0x1e00
#define SHARED_MEMORY_SIZE (1024 * 4)
#define DEFAULT_SAMPLES 1
#define LOADMONITOR_AO_TICK 'a'
struct ip_status {
 unsigned int idle;
 unsigned int busy_norm_volt;
 unsigned int buys_low_volt;
};
struct ips_info {
 char name;
 unsigned int flag;
 unsigned int mask;
 unsigned int freq;
 unsigned int samples;
 struct ip_status ip[MAX_SENSOR_NUM];
};
#define DDR_BANDWIDTH_CALC_IN_ATF 
struct loadmonitor_ddr {
 unsigned int wr_bandwidth;
 unsigned int rd_bandwidth;
};
#endif
