#include "cpu/pred/custom.hh"
#include "base/bitfield.hh"
#include "base/intmath.hh"

CustomBP::CustomBP(const CustomBPParams &params)
    : BPredUnit(params),
      PredictorSize(params.PredictorSize),
      PHTCtrBits(params.PHTCtrBits),
      globalHistoryBits(params.globalHistoryBits),
      pht(PredictorSize, SatCounter8(PHTCtrBits)),
      globalHistoryReg(0)
{
    assert(isPowerOf2(PredictorSize));

    // Initialize derived parameters
    PHThistoryBits = log2(PredictorSize);
    bBranchAddressBits = PHThistoryBits;
    BranchMask = (1 << bBranchAddressBits) - 1;
    globalHistoryMask = (1 << globalHistoryBits) - 1;
    PHTThreshold = (1 << (PHTCtrBits - 1)) - 1;
}

void CustomBP::uncondBranch(ThreadID tid, Addr pc, void * &bp_history) {
    BPHistory *history = new BPHistory;
    history->globalHistoryReg = globalHistoryReg;
    history->counter = 3; // Setting prediction as strong "taken"
    bp_history = static_cast<void*>(history);

    // Update global history register as "taken"
    updateGlobalHistReg(tid, true);
}

void CustomBP::squash(ThreadID tid, void *bp_history) {
    BPHistory *history = static_cast<BPHistory*>(bp_history);

    // Restore the global history register to its previous state
    globalHistoryReg = history->globalHistoryReg;
    delete history; // Clean up allocated memory for history
}

bool CustomBP::lookup(ThreadID tid, Addr branch_addr, void * &bp_history) {
    unsigned historyBits = globalHistoryReg & globalHistoryMask; // n bits of history
    unsigned branchBits = branch_addr & BranchMask; // m bits of branch address
    unsigned index = ~(branchBits & historyBits) & (PredictorSize - 1); // NAND operation

    BPHistory *history = new BPHistory;
    history->index = index;
    history->globalHistoryReg = globalHistoryReg;
    history->counter = pht[index];

    bp_history = static_cast<void*>(history);
    return pht[index] >> (PHTCtrBits - 1); // MSB of counter gives prediction
}

void CustomBP::btbUpdate(ThreadID tid, Addr branch_addr, void * &bp_history) {
    // Mask the global history to the required number of bits
    globalHistoryReg &= globalHistoryMask;
}

void CustomBP::update(ThreadID tid, Addr branch_addr, bool taken, void *bp_history,
                      bool squashed, const StaticInstPtr &inst, Addr corrTarget) {
    BPHistory *history = static_cast<BPHistory*>(bp_history);

    if (squashed) {
        globalHistoryReg = history->globalHistoryReg; // Restore global history
        delete history; // Clean up
        return;
    }

    unsigned index = history->index;
    if (taken && pht[index] < 3) {
        pht[index]++; // Increment counter if taken
    } else if (!taken && pht[index] > 0) {
        pht[index]--; // Decrement counter if not taken
    }

    updateGlobalHistReg(tid, taken);
    delete history;
}

void CustomBP::updateGlobalHistReg(ThreadID tid, bool taken) {
    globalHistoryReg = (globalHistoryReg << 1) | taken;
    globalHistoryReg &= globalHistoryMask; // Keep only the specified number of bits
}
