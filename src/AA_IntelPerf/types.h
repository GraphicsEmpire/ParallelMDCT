/* 
  Copyright 2009-2011 Intel Corporation All Rights Reserved.
 
  The source code, information and material ("Material") contained herein is 
  owned by Intel Corporation or its suppliers or licensors, and title to such 
  Material remains with Intel Corporation or its suppliers or licensors. The 
  Material contains proprietary information of Intel or its suppliers and 
  licensors. The Material is protected by worldwide copyright laws and treaty 
  provisions. No part of the Material may be used, copied, reproduced, 
  modified, published, uploaded, posted, transmitted, distributed or 
  disclosed in any way without Intel's prior express written permission. 
  No license under any patent, copyright or other intellectual property 
  rights in the Material is granted to or conferred upon you, either expressly, 
  by implication, inducement, estoppel or otherwise. Any license under such 
  intellectual property rights must be express and approved by Intel in 
  writing.
  Unless otherwise agreed by Intel in writing, you may not remove or alter 
  this notice or any other notice embedded in Materials by Intel or Intel's 
  suppliers or licensors in any way.
  Use of this material must comply with the 
  rights and restrictions set forth in the accompnied license terms set
  forth in file "license.txt"
*/
// written by Roman Dementiev
//

#ifndef CPUCounters_TYPES_H
#define CPUCounters_TYPES_H


/*!     \file types.h
        \brief Internal type and constant definitions
*/

#include <iostream>

typedef unsigned long long uint64;
typedef signed long long int64;
typedef unsigned int uint32;
typedef signed int int32;


/*
        MSR addreses from
        "Intel 64 and IA-32 Architectures Software Developers Manual Volume 3B:
        System Programming Guide, Part 2", Appendix A "PERFORMANCE-MONITORING EVENTS"
*/

#define INST_RETIRED_ANY_ADDR           (0x309)
#define CPU_CLK_UNHALTED_THREAD_ADDR    (0x30A)
#define CPU_CLK_UNHALTED_REF_ADDR       (0x30B)
#define IA32_CR_PERF_GLOBAL_CTRL        (0x38F)
#define IA32_CR_FIXED_CTR_CTRL          (0x38D)
#define IA32_PERFEVTSEL0_ADDR           (0x186)
#define IA32_PERFEVTSEL1_ADDR           (IA32_PERFEVTSEL0_ADDR + 1)
#define IA32_PERFEVTSEL2_ADDR           (IA32_PERFEVTSEL0_ADDR + 2)
#define IA32_PERFEVTSEL3_ADDR           (IA32_PERFEVTSEL0_ADDR + 3)

#define IA32_DEBUGCTL                   (0x1D9)

#define IA32_PMC0                       (0xC1)
#define IA32_PMC1                       (0xC1 + 1)
#define IA32_PMC2                       (0xC1 + 2)
#define IA32_PMC3                       (0xC1 + 3)

/* From Table B-5. of the above mentioned document */
#define PLATFORM_INFO_ADDR              (0xCE)

#define IA32_TIME_STAMP_COUNTER         (0x10)

// Event IDs

// Nehalem/Westmere on-core events
#define MEM_LOAD_RETIRED_L3_MISS_EVTNR  (0xCB)
#define MEM_LOAD_RETIRED_L3_MISS_UMASK  (0x10)

#define MEM_LOAD_RETIRED_L3_UNSHAREDHIT_EVTNR   (0xCB)
#define MEM_LOAD_RETIRED_L3_UNSHAREDHIT_UMASK   (0x04)

#define MEM_LOAD_RETIRED_L2_HITM_EVTNR  (0xCB)
#define MEM_LOAD_RETIRED_L2_HITM_UMASK  (0x08)

#define MEM_LOAD_RETIRED_L2_HIT_EVTNR   (0xCB)
#define MEM_LOAD_RETIRED_L2_HIT_UMASK   (0x02)

// Sandy Bridge on-core events

#define MEM_LOAD_UOPS_MISC_RETIRED_LLC_MISS_EVTNR (0xD4)
#define MEM_LOAD_UOPS_MISC_RETIRED_LLC_MISS_UMASK (0x02)

#define MEM_LOAD_UOPS_LLC_HIT_RETIRED_XSNP_NONE_EVTNR (0xD2)
#define MEM_LOAD_UOPS_LLC_HIT_RETIRED_XSNP_NONE_UMASK (0x08)

#define MEM_LOAD_UOPS_LLC_HIT_RETIRED_XSNP_HITM_EVTNR (0xD2)
#define MEM_LOAD_UOPS_LLC_HIT_RETIRED_XSNP_HITM_UMASK (0x04)

#define MEM_LOAD_UOPS_RETIRED_L2_HIT_EVTNR (0xD1)
#define MEM_LOAD_UOPS_RETIRED_L2_HIT_UMASK (0x02)

// architectural on-core events

#define ARCH_LLC_REFERENCE_EVTNR        (0x2E)
#define ARCH_LLC_REFERENCE_UMASK        (0x4F)

#define ARCH_LLC_MISS_EVTNR     (0x2E)
#define ARCH_LLC_MISS_UMASK     (0x41)

// Atom on-core events

#define ATOM_MEM_LOAD_RETIRED_L2_HIT_EVTNR   (0xCB)
#define ATOM_MEM_LOAD_RETIRED_L2_HIT_UMASK   (0x01)

#define ATOM_MEM_LOAD_RETIRED_L2_MISS_EVTNR   (0xCB)
#define ATOM_MEM_LOAD_RETIRED_L2_MISS_UMASK   (0x02)

/*
        From "Intel(r) Xeon(r) Processor 7500 Series Uncore Programming Guide"
*/
// Uncore msrs

#define MSR_UNCORE_PERF_GLOBAL_CTRL_ADDR        (0x391)

#define MSR_UNCORE_PERFEVTSEL0_ADDR             (0x3C0)
#define MSR_UNCORE_PERFEVTSEL1_ADDR             (MSR_UNCORE_PERFEVTSEL0_ADDR + 1)
#define MSR_UNCORE_PERFEVTSEL2_ADDR             (MSR_UNCORE_PERFEVTSEL0_ADDR + 2)
#define MSR_UNCORE_PERFEVTSEL3_ADDR             (MSR_UNCORE_PERFEVTSEL0_ADDR + 3)
#define MSR_UNCORE_PERFEVTSEL4_ADDR             (MSR_UNCORE_PERFEVTSEL0_ADDR + 4)
#define MSR_UNCORE_PERFEVTSEL5_ADDR             (MSR_UNCORE_PERFEVTSEL0_ADDR + 5)
#define MSR_UNCORE_PERFEVTSEL6_ADDR             (MSR_UNCORE_PERFEVTSEL0_ADDR + 6)
#define MSR_UNCORE_PERFEVTSEL7_ADDR             (MSR_UNCORE_PERFEVTSEL0_ADDR + 7)


#define MSR_UNCORE_PMC0                         (0x3B0)
#define MSR_UNCORE_PMC1                         (MSR_UNCORE_PMC0 + 1)
#define MSR_UNCORE_PMC2                         (MSR_UNCORE_PMC0 + 2)
#define MSR_UNCORE_PMC3                         (MSR_UNCORE_PMC0 + 3)
#define MSR_UNCORE_PMC4                         (MSR_UNCORE_PMC0 + 4)
#define MSR_UNCORE_PMC5                         (MSR_UNCORE_PMC0 + 5)
#define MSR_UNCORE_PMC6                         (MSR_UNCORE_PMC0 + 6)
#define MSR_UNCORE_PMC7                         (MSR_UNCORE_PMC0 + 7)

// Uncore event IDs

#define UNC_QMC_WRITES_FULL_ANY_EVTNR           (0x2F)
#define UNC_QMC_WRITES_FULL_ANY_UMASK           (0x07)

#define UNC_QMC_NORMAL_READS_ANY_EVTNR          (0x2C)
#define UNC_QMC_NORMAL_READS_ANY_UMASK          (0x07)

#define UNC_QHL_REQUESTS_EVTNR                  (0x20)

#define UNC_QHL_REQUESTS_IOH_READS_UMASK        (0x01)
#define UNC_QHL_REQUESTS_IOH_WRITES_UMASK       (0x02)
#define UNC_QHL_REQUESTS_REMOTE_READS_UMASK     (0x04)
#define UNC_QHL_REQUESTS_REMOTE_WRITES_UMASK    (0x08)
#define UNC_QHL_REQUESTS_LOCAL_READS_UMASK      (0x10)
#define UNC_QHL_REQUESTS_LOCAL_WRITES_UMASK     (0x20)


// Beckton uncore event IDs

#define U_MSR_PMON_GLOBAL_CTL                   (0x0C00)

#define MB0_MSR_PERF_GLOBAL_CTL                 (0x0CA0)
#define MB0_MSR_PMU_CNT_0                       (0x0CB1)
#define MB0_MSR_PMU_CNT_CTL_0                   (0x0CB0)
#define MB0_MSR_PMU_CNT_1                       (0x0CB3)
#define MB0_MSR_PMU_CNT_CTL_1                   (0x0CB2)
#define MB0_MSR_PMU_ZDP_CTL_FVC                 (0x0CAB)


#define MB1_MSR_PERF_GLOBAL_CTL                 (0x0CE0)
#define MB1_MSR_PMU_CNT_0                       (0x0CF1)
#define MB1_MSR_PMU_CNT_CTL_0                   (0x0CF0)
#define MB1_MSR_PMU_CNT_1                       (0x0CF3)
#define MB1_MSR_PMU_CNT_CTL_1                   (0x0CF2)
#define MB1_MSR_PMU_ZDP_CTL_FVC                 (0x0CEB)

#define BB0_MSR_PERF_GLOBAL_CTL                 (0x0C20)
#define BB0_MSR_PERF_CNT_1                      (0x0C33)
#define BB0_MSR_PERF_CNT_CTL_1                  (0x0C32)

#define BB1_MSR_PERF_GLOBAL_CTL                 (0x0C60)
#define BB1_MSR_PERF_CNT_1                      (0x0C73)
#define BB1_MSR_PERF_CNT_CTL_1                  (0x0C72)

#define R_MSR_PMON_CTL0 (0x0E10)
#define R_MSR_PMON_CTR0 (0x0E11)
#define R_MSR_PMON_CTL1 (0x0E12)
#define R_MSR_PMON_CTR1 (0x0E13)
#define R_MSR_PMON_CTL2 (0x0E14)
#define R_MSR_PMON_CTR2 (0x0E15)
#define R_MSR_PMON_CTL3 (0x0E16)
#define R_MSR_PMON_CTR3 (0x0E17)
#define R_MSR_PMON_CTL4 (0x0E18)
#define R_MSR_PMON_CTR4 (0x0E19)
#define R_MSR_PMON_CTL5 (0x0E1A)
#define R_MSR_PMON_CTR5 (0x0E1B)
#define R_MSR_PMON_CTL6 (0x0E1C)
#define R_MSR_PMON_CTR6 (0x0E1D)
#define R_MSR_PMON_CTL7 (0x0E1E)
#define R_MSR_PMON_CTR7 (0x0E1F)
#define R_MSR_PMON_CTL8 (0x0E30)
#define R_MSR_PMON_CTR8 (0x0E31)
#define R_MSR_PMON_CTL9 (0x0E32)
#define R_MSR_PMON_CTR9 (0x0E33)
#define R_MSR_PMON_CTL10 (0x0E34)
#define R_MSR_PMON_CTR10 (0x0E35)
#define R_MSR_PMON_CTL11 (0x0E36)
#define R_MSR_PMON_CTR11 (0x0E37)
#define R_MSR_PMON_CTL12 (0x0E38)
#define R_MSR_PMON_CTR12 (0x0E39)
#define R_MSR_PMON_CTL13 (0x0E3A)
#define R_MSR_PMON_CTR13 (0x0E3B)
#define R_MSR_PMON_CTL14 (0x0E3C)
#define R_MSR_PMON_CTR14 (0x0E3D)
#define R_MSR_PMON_CTL15 (0x0E3E)
#define R_MSR_PMON_CTR15 (0x0E3F)

#define R_MSR_PORT0_IPERF_CFG0 (0x0E04)
#define R_MSR_PORT1_IPERF_CFG0 (0x0E05)
#define R_MSR_PORT2_IPERF_CFG0 (0x0E06)
#define R_MSR_PORT3_IPERF_CFG0 (0x0E07)
#define R_MSR_PORT4_IPERF_CFG0 (0x0E08)
#define R_MSR_PORT5_IPERF_CFG0 (0x0E09)
#define R_MSR_PORT6_IPERF_CFG0 (0x0E0A)
#define R_MSR_PORT7_IPERF_CFG0 (0x0E0B)

#define R_MSR_PORT0_IPERF_CFG1 (0x0E24)
#define R_MSR_PORT1_IPERF_CFG1 (0x0E25)
#define R_MSR_PORT2_IPERF_CFG1 (0x0E26)
#define R_MSR_PORT3_IPERF_CFG1 (0x0E27)
#define R_MSR_PORT4_IPERF_CFG1 (0x0E28)
#define R_MSR_PORT5_IPERF_CFG1 (0x0E29)
#define R_MSR_PORT6_IPERF_CFG1 (0x0E2A)
#define R_MSR_PORT7_IPERF_CFG1 (0x0E2B)

#define R_MSR_PMON_GLOBAL_CTL_7_0 (0x0E00)
#define R_MSR_PMON_GLOBAL_CTL_15_8 (0x0E20)

#define W_MSR_PMON_GLOBAL_CTL    (0xC80)
#define W_MSR_PMON_FIXED_CTR_CTL (0x395)
#define W_MSR_PMON_FIXED_CTR     (0x394)

/* \brief Event Select Register format

        According to
        "Intel 64 and IA-32 Architectures Software Developers Manual Volume 3B:
        System Programming Guide, Part 2", Figure 30-6. Layout of IA32_PERFEVTSELx
        MSRs Supporting Architectural Performance Monitoring Version 3
*/
struct EventSelectRegister
{
    union
    {
        struct
        {
            uint64 event_select : 8;
            uint64 umask : 8;
            uint64 usr : 1;
            uint64 os : 1;
            uint64 edge : 1;
            uint64 pin_control : 1;
            uint64 apic_int : 1;
            uint64 any_thread : 1;
            uint64 enable : 1;
            uint64 invert : 1;
            uint64 cmask : 8;
            uint64 reserved1 : 32;
        } fields;
        uint64 value;
    };
};


/* \brief Fixed Event Control Register format

        According to
        "Intel 64 and IA-32 Architectures Software Developers Manual Volume 3B:
        System Programming Guide, Part 2", Figure 30-7. Layout of
        IA32_FIXED_CTR_CTRL MSR Supporting Architectural Performance Monitoring Version 3
*/
struct FixedEventControlRegister
{
    union
    {
        struct
        {
            // CTR0
            uint64 os0 : 1;
            uint64 usr0 : 1;
            uint64 any_thread0 : 1;
            uint64 enable_pmi0 : 1;
            // CTR1
            uint64 os1 : 1;
            uint64 usr1 : 1;
            uint64 any_thread1 : 1;
            uint64 enable_pmi1 : 1;
            // CTR2
            uint64 os2 : 1;
            uint64 usr2 : 1;
            uint64 any_thread2 : 1;
            uint64 enable_pmi2 : 1;

            uint64 reserved1 : 52;
        } fields;
        uint64 value;
    };
};

inline std::ostream & operator << (std::ostream & o, const FixedEventControlRegister & reg)
{
    o << "os0\t\t" << reg.fields.os0 << std::endl;
    o << "usr0\t\t" << reg.fields.usr0 << std::endl;
    o << "any_thread0\t" << reg.fields.any_thread0 << std::endl;
    o << "enable_pmi0\t" << reg.fields.enable_pmi0 << std::endl;

    o << "os1\t\t" << reg.fields.os1 << std::endl;
    o << "usr1\t\t" << reg.fields.usr1 << std::endl;
    o << "any_thread1\t" << reg.fields.any_thread1 << std::endl;
    o << "enable_pmi10\t" << reg.fields.enable_pmi1 << std::endl;

    o << "os2\t\t" << reg.fields.os2 << std::endl;
    o << "usr2\t\t" << reg.fields.usr2 << std::endl;
    o << "any_thread2\t" << reg.fields.any_thread2 << std::endl;
    o << "enable_pmi2\t" << reg.fields.enable_pmi2 << std::endl;

    o << "reserved1\t" << reg.fields.reserved1 << std::endl;
    return o;
}

// UNCORE COUNTER CONTROL

/* \brief Uncore Event Select Register Register format

        According to
        "Intel 64 and IA-32 Architectures Software Developers Manual Volume 3B:
        System Programming Guide, Part 2", Figure 30-20. Layout of MSR_UNCORE_PERFEVTSELx MSRs
*/
struct UncoreEventSelectRegister
{
    union
    {
        struct
        {
            uint64 event_select : 8;
            uint64 umask : 8;
            uint64 reserved1 : 1;
            uint64 occ_ctr_rst : 1;
            uint64 edge : 1;
            uint64 reserved2 : 1;
            uint64 enable_pmi : 1;
            uint64 reserved3 : 1;
            uint64 enable : 1;
            uint64 invert : 1;
            uint64 cmask : 8;
            uint64 reservedx : 32;
        } fields;
        uint64 value;
    };
};

/* \brief Beckton Uncore PMU ZDP FVC Control Register format

        From "Intel(r) Xeon(r) Processor 7500 Series Uncore Programming Guide"
        Table 2-80. M_MSR_PMU_ZDP_CTL_FVC Register - Field Definitions
*/
struct BecktonUncorePMUZDPCTLFVCRegister
{
    union
    {
        struct
        {
            uint64 fvid : 5;
            uint64 bcmd : 3;
            uint64 resp : 3;
            uint64 evnt0 : 3;
            uint64 evnt1 : 3;
            uint64 evnt2 : 3;
            uint64 evnt3 : 3;
            uint64 pbox_init_err : 1;
        } fields; // nehalem-ex version
        struct
        {
            uint64 fvid : 6;
            uint64 bcmd : 3;
            uint64 resp : 3;
            uint64 evnt0 : 3;
            uint64 evnt1 : 3;
            uint64 evnt2 : 3;
            uint64 evnt3 : 3;
            uint64 pbox_init_err : 1;
        } fields_wsm; // westmere-ex version
        uint64 value;
    };
};

/* \brief Beckton Uncore PMU Counter Control Register format

        From "Intel(r) Xeon(r) Processor 7500 Series Uncore Programming Guide"
        Table 2-67. M_MSR_PMU_CNT_CTL{5-0} Register - Field Definitions
*/
struct BecktonUncorePMUCNTCTLRegister
{
    union
    {
        struct
        {
            uint64 en : 1;
            uint64 pmi_en : 1;
            uint64 count_mode : 2;
            uint64 storage_mode : 2;
            uint64 wrap_mode : 1;
            uint64 flag_mode : 1;
            uint64 rsv1 : 1;
            uint64 inc_sel : 5;
            uint64 rsv2 : 5;
            uint64 set_flag_sel : 3;
        } fields;
        uint64 value;
    };
};

#endif
