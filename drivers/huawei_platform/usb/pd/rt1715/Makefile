subdir-ccflags-y += -Wall -Werror -DCONFIG_RT_REGMAP
ifeq ($(USE_BISHENG_CLANG),true)
    subdir-ccflags-y += -Wno-error=unused-but-set-variable
endif

obj-y	+= tcpc_rt1715.o

tcpc_class-objs			+= tcpci_core.o tcpci_typec.o tcpci_timer.o \
		tcpm.o tcpci_alert.o rt-regmap.o tcpci_dual_role.o \
		pd_dbg_info.o huawei/tcpci.o

ifdef CONFIG_USB_POWER_DELIVERY
tcpc_class-objs			+= tcpci_event.o \
		pd_core.o pd_policy_engine.o pd_process_evt.o \
		pd_dpm_core.o pd_dpm_uvdm.o pd_dpm_alt_mode_dp.o pd_dpm_pdo_select.o\
		pd_dpm_reaction.o \
		pd_process_evt_snk.o pd_process_evt_src.o pd_process_evt_vdm.o \
		pd_process_evt_drs.o pd_process_evt_prs.o pd_process_evt_vcs.o \
		pd_process_evt_dbg.o pd_process_evt_tcp.o pd_process_evt_com.o \
		pd_policy_engine_src.o pd_policy_engine_snk.o pd_policy_engine_ufp.o pd_policy_engine_vcs.o \
		pd_policy_engine_dfp.o pd_policy_engine_dr.o pd_policy_engine_drs.o pd_policy_engine_prs.o \
		pd_policy_engine_dbg.o pd_policy_engine_com.o
endif

obj-$(CONFIG_TCPC_CLASS)	+= tcpc_class.o rt_pd_manager.o

ifdef CONFIG_KERNEL_CODEMERGE
KBUILD_CFLAGS += -fno-profile-use
endif
