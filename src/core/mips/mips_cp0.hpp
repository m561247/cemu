#ifndef MIPS_CP0
#define MIPS_CP0

#include "mips_common.hpp"
#include "mips_mmu.hpp"
#include <cstdint>
#include <cassert>
#include <cstdio>
template <uint32_t nr_tlb_entry = 8>
class mips_cp0 {
public:
    mips_cp0(uint32_t &pc, bool &bd, mips_mmu<nr_tlb_entry> &mmu):pc(pc),bd(bd),mmu(mmu) {
        reset();
    }
    void reset() {
        index = 0;
        random = nr_tlb_entry - 1;
        entrylo0 = 0;
        entrylo1 = 0;
        context = 0;
        pagemask = 0;
        wired = 0;
        badva = 0;
        count = 0;
        entryhi = 0;
        compare = 0;
        status = 0;
        cp0_status *status_reg = (cp0_status*)&status;
        status_reg->BEV = 1;
        cause = 0;
        epc = 0;
        ebase = 0x8000000u;
        config0 = 0;
        cp0_config0 *config0_reg = (cp0_config0*)&config0;
        config0_reg->k0 = 3;
        config0_reg->MT = 1;
        config0_reg->M = 1;
        config1 = 0;
        cp0_config1 *config1_reg = (cp0_config1*)&config1;
        config1_reg->DA = 1; // Dcache associativity: 1: 2-way
        config1_reg->DL = 4; // Dcache line size: 4: 32bytes
        config1_reg->DS = 1; // Dcache sets per way: 1: 128
        config1_reg->IA = 1; // Icache associativity: 1: 2-way
        config1_reg->IL = 4; // Icache line size: 4: 32bytes
        config1_reg->IS = 1; // Icache sets per way: 1: 128
        config1_reg->MS = nr_tlb_entry - 1;
        taglo = 0;
        taghi = 0;
        errorepc = 0;
        cur_need_trap = false;
    }
    uint32_t mfc0(uint32_t reg, uint32_t sel) {
        switch (reg) {
            case RD_INDEX:
                assert(sel == 0);
                return index;
            case RD_RANDOM:
                assert(sel == 0);
                return random;
            case RD_ENTRYLO0:
                assert(sel == 0);
                return entrylo0;
            case RD_ENTRYLO1:
                assert(sel == 0);
                return entrylo1;
            case RD_CONTEXT:
                assert(sel == 0);
                return context;
            case RD_PAGEMASK:
                assert(sel == 0);
                return pagemask;
            case RD_WIRED:
                assert(sel == 0);
                return wired;
            case RD_BADVA:
                assert(sel == 0);
                return badva;
            case RD_COUNT:
                assert(sel == 0);
                return count;
            case RD_ENTRYHI:
                assert(sel == 0);
                return entryhi;
            case RD_COMPARE: 
                assert(sel == 0);
                return compare;
            case RD_STATUS:
                assert(sel == 0);
                return status;
            case RD_CAUSE:
                assert(sel == 0);
                return cause;
            case RD_EPC:
                assert(sel == 0);
                return epc;
            case RD_PRID_EBASE: {
                switch (sel) {
                    case 0:
                        return prid;
                    case 1:
                        return ebase;
                    default:
                        assert(false);
                        return 0;
                }
            }
            case RD_CONFIG: {
                switch (sel) {
                    case 0:
                        return config0;
                    case 1: 
                        return config1;
                    default:
                        assert(false);
                        return 0;
                }
            }
            case RD_TAGLO: {
                switch (sel) {
                    case 0:
                        return taglo;
                    case 2:
                        return taglo;
                    default:
                        assert(false);
                        return 0;
                }
            }
            case RD_TAGHI: {
                switch (sel) {
                    case 0:
                        return taghi;
                    case 2:
                        return taghi;
                    default:
                        assert(false);
                        return 0;
                }
            }
            case RD_ERREPC:
                assert(sel == 0);
                return errorepc;
            default:
                return 0;
        }
    }
    void mtc0(uint32_t reg, uint32_t sel, uint32_t value) {
        switch (reg) {
            case RD_INDEX:
                index = (index & 0x80000000u) | (value & (nr_tlb_entry - 1));
                assert(sel == 0);
                break;
            case RD_ENTRYLO0:
                entrylo0 = value & 0x0fffffffu; // mask higher 32bit PFN
                break;
            case RD_ENTRYLO1:
                entrylo1 = value & 0x0fffffffu; // mask higher 32bit PFN
                break;
            case RD_CONTEXT: {
                cp0_context *context_reg = (cp0_context*)&context;
                cp0_context *context_new = (cp0_context*)&value;
                context_reg->PTEBase = context_new->PTEBase;
                break;
            }
            case RD_PAGEMASK:
                // only 4KB implemented, always zero.
                break;
            case RD_WIRED: {
                wired = value & (nr_tlb_entry - 1);
                random = nr_tlb_entry - 1;
                break;
            }
            case RD_COUNT:
                count = value;
                assert(sel == 0);
                break;
            case RD_ENTRYHI: {
                cp0_entryhi *entryhi_reg = (cp0_entryhi*)&entryhi;
                cp0_entryhi *entryhi_new = (cp0_entryhi*)&value;
                entryhi_reg->ASID = entryhi_new->ASID;
                entryhi_reg->VPN2 = entryhi_new->VPN2;
                break;
            }
            case RD_COMPARE: {
                compare = value;
                cp0_cause *cause_reg = (cp0_cause*)&cause;
                cause_reg->IP &= 0x7f; // clear IP[7s]
                assert(sel == 0);
                break;
            }
            case RD_STATUS: {
                cp0_status *status_reg = (cp0_status*)&status;
                cp0_status *status_new = (cp0_status*)&value;
                status_reg->IE = status_new->IE;
                status_reg->EXL = status_new->EXL;
                status_reg->ERL = status_new->ERL;
                status_reg->KSU = status_new->KSU == USER_MODE ? USER_MODE : KERNEL_MODE;
                status_reg->IM = status_new->IM;
                if (status_reg->BEV != status_new->BEV) {
                    printf("bev changed at pc %x to %x\n",pc,status_new->BEV);
                }
                status_reg->BEV = status_new->BEV;
                assert(sel == 0);
                break;
            }
            case RD_CAUSE: {
                cp0_cause *cause_reg = (cp0_cause*)&cause;
                cp0_cause *cause_new = (cp0_cause*)&value;
                cause_reg->IP = (cause_reg->IP & 0b11111100u) | (cause_new->IP & 0b11);
                cause_reg->IV = cause_new->IV;
                assert(sel == 0);
                break;
            }
            case RD_EPC:
                epc = value;
                assert(sel == 0);
                break;
            case RD_PRID_EBASE: {
                // only ebase writeable
                ebase = 0x80000000u | (value & 0x3fff0000u);
                assert(sel == 1);
                break;
            }
            case RD_CONFIG: {
                assert(sel == 0);
                cp0_config0 *config0_reg = (cp0_config0*)&config0;
                cp0_config0 *config0_new = (cp0_config0*)&value;
                config0_reg->k0 = config0_new->k0;
                break;
            }
            case RD_TAGLO:
                taglo = value;
                break;
            case RD_TAGHI:
                taghi = value;
                break;
            case RD_ERREPC:
                errorepc = value;
                break;
            default: {
                break;
            }
        }
    }
    void pre_exec(unsigned int ext_int) {
        cur_need_trap = false;
        count = (count + 1llu) &0xfffffffflu;
        random = random == wired ? (nr_tlb_entry - 1) : random - 1;
        cp0_cause *cause_reg = (cp0_cause*)&cause;
        cause_reg->IP = (cause_reg->IP & 0b10000011u) | ( (ext_int & 0b11111u) << 2);
        if (count == compare) cause_reg->IP |= 1u << 7;
        check_and_raise_int();
    }
    bool need_trap() {
        return cur_need_trap;
    }
    uint32_t get_trap_pc() {
        return trap_pc;
    }
    void raise_trap(mips32_exccode exc, uint32_t badva_val = 0) {
        cur_need_trap = true;
        cp0_status *status_reg = (cp0_status*)&status;
        cp0_cause *cause_reg = (cp0_cause*)&cause;
        // Note: If multicore implemented, ebase should clear lower 10bits (which is CPUNum) before used.
        uint32_t trap_base = (status_reg->BEV ? 0xbfc00200u : ebase);
        if (!status_reg->EXL) {
            epc = bd ? pc - 4 : pc;
            cause_reg->BD = bd;
            trap_pc = trap_base + ((exc == EXC_TLBL || exc == EXC_TLBS) ? 0x000 : (exc == EXC_INT && cause_reg->IV && !status_reg->BEV) ? 0x200 : 0x180);
        }
        else trap_pc = trap_base + 0x180u;
        cause_reg->exccode = exc;
        status_reg->EXL = 1;
        if (exc == EXC_ADEL || exc == EXC_ADES || exc == EXC_TLBL || exc == EXC_TLBS || exc == EXC_MOD) {
            badva = badva_val;
        }
        if (exc == EXC_TLBL || exc == EXC_TLBS || exc == EXC_MOD) {
            cp0_context *context_reg = (cp0_context*)&context;
            context_reg->badvpn2 = badva_val >> 13;
        }
    }
    void eret() {
        cur_need_trap = true;
        cp0_status *status_reg = (cp0_status*)&status;
        trap_pc = status_reg->ERL ? errorepc : epc;
        if (status_reg->ERL) status_reg->ERL = 0;
        else status_reg->EXL = 0;
    }
    mips32_ksu get_ksu() {
        cp0_status *status_def = (cp0_status*)&status;
        return (status_def->EXL || status_def->ERL || status_def->KSU == KERNEL_MODE) ? KERNEL_MODE : USER_MODE;
    }
private:
    void check_and_raise_int() {
        cp0_status *status_def = (cp0_status*)&status;
        cp0_cause *cause_def = (cp0_cause*)&cause;
        if (status_def->IE && status_def->EXL == 0 && status_def->ERL == 0 && (status_def->IM & cause_def->IP) != 0) {
            raise_trap(EXC_INT);
        }
    }
    mips_mmu<nr_tlb_entry> &mmu;

    uint32_t &pc;
    bool &bd;

    bool cur_need_trap;
    uint32_t trap_pc;
    
    uint32_t index; // Note: index[31] will be write by TLBP
    uint32_t random;
    uint32_t entrylo0;
    uint32_t entrylo1;
    uint32_t context;
    uint32_t pagemask;
    uint32_t wired;
    uint32_t badva;
    uint32_t count;
    uint32_t entryhi; // entryhi VPN2 will be updated by TLB exception and TLBR
    // entryhi ASID will be updated by TLBR
    uint32_t compare;
    uint32_t status;
    uint32_t cause;
    uint32_t epc;
    uint32_t prid;
    uint32_t ebase;
    uint32_t config0;
    uint32_t config1;
    uint32_t taglo;
    uint32_t taghi;
    uint32_t errorepc;
};

#endif
