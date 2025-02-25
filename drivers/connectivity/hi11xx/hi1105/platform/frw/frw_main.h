/**
 * Copyright (c) @CompanyNameMagicTag 2021-2023. All rights reserved.
 *
 * Description: frw_main.c header file
 * Author: @CompanyNameTag
 */

#ifndef FRW_MAIN_H
#define FRW_MAIN_H

/* 其他头文件包含 */
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "frw_subsys_context.h"

#undef THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_FRW_MAIN_H

/* 宏定义 */
#define frw_err_log(_uc_vap_id, _puc_string)
#define frw_err_log1(_uc_vap_id, _puc_string, _l_para1)
#define frw_err_log2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define frw_err_log3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define frw_err_log4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define frw_err_var(_uc_vap_id, _c_fmt, ...)

#define frw_warning_log(_uc_vap_id, _puc_string)
#define frw_warning_log1(_uc_vap_id, _puc_string, _l_para1)
#define frw_warning_log2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define frw_warning_log3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define frw_warning_log4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define frw_warning_var(_uc_vap_id, _c_fmt, ...)

#define frw_info_log(_uc_vap_id, _puc_string)
#define frw_info_log1(_uc_vap_id, _puc_string, _l_para1)
#define frw_info_log2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define frw_info_log3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define frw_info_log4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define frw_info_var(_uc_vap_id, _c_fmt, ...)

#define frw_event_internal(_puc_macaddr, _uc_vap_id, en_event_type, output_data, data_len)

#define FRW_TIMER_DEFAULT_TIME 10

/* 函数声明 */
int32_t frw_main_init(void);
void frw_main_exit(void);
void frw_set_init_state_ctx(uint16_t en_init_state, frw_subsys_context *ctx);
uint16_t frw_get_init_state_ctx(frw_subsys_context *ctx);

#endif /* end of frw_main */
