//Custom.hh

#ifndef __CPU_PRED_CUSTOM_BRANCH_PRED_HH__
#define __CPU_PRED_CUSTOM_BRANCH_PRED_HH__

#include <vector>
#include "base/sat_counter.hh"
#include "cpu/pred/bpred_unit.hh"
#include "base/types.hh"
#include "base/intmath.hh"
#include "params/CustomBP.hh"

class CustomBP : public BPredUnit
{
public:
    CustomBP(const CustomBPParams &params);
    void uncondBranch(ThreadID tid, Addr pc, void * &bp_history);
    void squash(ThreadID tid, void *bp_history);
    bool lookup(ThreadID tid, Addr branch_addr, void * &bp_history);
    void btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history);
    void update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                bool squashed, const StaticInstPtr & inst, Addr corrTarget);

private:
    struct BPHistory {
        unsigned index;
        unsigned globalHistoryReg;
        uint8_t counter;
    };

    unsigned PredictorSize;
    unsigned PHTCtrBits;
    unsigned globalHistoryBits;
    unsigned PHThistoryBits;
    unsigned bBranchAddressBits;
    unsigned BranchMask;
    unsigned globalHistoryMask;
    unsigned PHTThreshold;

    std::vector<SatCounter8> pht;
    unsigned globalHistoryReg;

    void updateGlobalHistReg(ThreadID tid, bool taken);
};

#endif // __CPU_PRED_CUSTOM_BRANCH_PRED_HH__
