#include <iostream>
#include <fstream>
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include "pin.H"

using namespace std;

ofstream OutFile;

#define truncate(val, bits) ((val) & ((1 << (bits)) - 1))

static UINT64 takenCorrect = 0;
static UINT64 takenIncorrect = 0;
static UINT64 notTakenCorrect = 0;
static UINT64 notTakenIncorrect = 0;
static UINT64 addrCorrect = 0;
static UINT64 addrIncorrect = 0;
static UINT64 addrUnpredict = 0;

typedef struct predict_result
{
    BOOL taken;
    BOOL valid;
    ADDRINT targetAddr;
} PredictResult;

template <size_t N, UINT64 init = (1 << N)/2 - 1>   // N < 64
class SaturatingCnt
{
    UINT64 val;
    BOOL valid;
    ADDRINT targetAddr;

    public:
        SaturatingCnt() { reset();  valid = false; targetAddr = 0; }

        void increase() { if (val < (1 << N) - 1) val++; }
        void decrease() { if (val > 0) val--; }

        void reset() { val = init; }

        UINT64 getVal() { return val; }
        BOOL isTaken() { return (val > (1 << N)/2 - 1); }

        BOOL getValid() { return valid; }
        void setValid() { valid = true; }
        void resetValid() { valid = false; }

        ADDRINT getAddr() { return targetAddr; }
        void setAddr(ADDRINT addr) { targetAddr = addr; }
};

template<size_t N>      // N < 64
class ShiftReg
{
    UINT64 val;
    public:
        ShiftReg() { val = 0; }

        bool shiftIn(bool b)
        {
            bool ret = !!(val&(1<<(N-1)));
            val <<= 1;
            val |= b;
            val &= (1<<N)-1;
            return ret;
        }

        UINT64 getVal() { return val; }
};

class BranchPredictor
{
    public:
        BranchPredictor() { }
        virtual PredictResult predict(ADDRINT addr) 
        { 
            PredictResult re; 
            re.taken = false;  
            re.valid = false;
            re.targetAddr = 0;
            return re;
        };
        virtual void update(BOOL takenActually, ADDRINT addrActually, PredictResult predicted, ADDRINT addr) {};
};

BranchPredictor* BP;


/* ===================================================================== */
/* 至少需实现2种动态预测方法                                               */
/* ===================================================================== */
// 1. BHT-based branch predictor
template<size_t L>
class BHTPredictor: public BranchPredictor
{
    SaturatingCnt<2> counter[1 << L];

    public:
        BHTPredictor() { }

        PredictResult predict(ADDRINT addr)
        {
            PredictResult ret;
            UINT64 Tag = truncate(addr, L);

            ret.taken = counter[Tag].isTaken();
            ret.valid = counter[Tag].getValid();
            if(ret.valid)
            {
                ret.targetAddr = counter[Tag].getAddr();
            }
            else
            {
                ret.targetAddr = 0;
            }
            return ret;
        }

        void update(BOOL takenActually, ADDRINT addrActually, PredictResult predicted, ADDRINT addr)
        {
            UINT64 Tag = truncate(addr, L);
            BOOL valid = counter[Tag].getValid();
            ADDRINT addrPredict = counter[Tag].getAddr();

            // taken
            if(takenActually)
            {
                counter[Tag].increase();
            }
            else
            {
                counter[Tag].decrease();
            }     

            // 有效
            if(valid)
            {
                // 预测失败
                if(addrActually != addrPredict)
                {
                    counter[Tag].resetValid();
                }
            }
            // 无效
            else
            {
                // 变为有效
                if(takenActually)
                {
                    counter[Tag].setValid();
                    counter[Tag].reset();
                    counter[Tag].increase();
                    counter[Tag].setAddr(addrActually);
                }
            }
        }
};

// 2. Global-history-based branch predictor
template<size_t L, size_t H, UINT64 BITS = 2>
class GlobalHistoryPredictor: public BranchPredictor
{
    SaturatingCnt<BITS> bhist[1 << L];  // PHT中的分支历史字段
    ShiftReg<H> GHR;
    
    public:
        GlobalHistoryPredictor() { }

        PredictResult predict(ADDRINT addr) 
        {
            UINT64 Tag = truncate(addr ^ GHR.getVal(), L);
            PredictResult ret;
            
            ret.taken = bhist[Tag].isTaken();
            ret.valid = bhist[Tag].getValid();
            if(ret.valid)
            {
                ret.targetAddr = bhist[Tag].getAddr();
            }
            else
            {
                ret.targetAddr = 0;
            }
            
            return ret;
        }

        void update(BOOL takenActually, ADDRINT addrActually, PredictResult predicted, ADDRINT addr)
        {
            UINT64 Tag = truncate(addr ^ GHR.getVal(), L);
            BOOL valid = bhist[Tag].getValid();
            ADDRINT addrPredict = bhist[Tag].getAddr();

            // GHR
            GHR.shiftIn(takenActually);

            // taken
            if(takenActually)
            {
                bhist[Tag].increase();
            }
            else
            {
                bhist[Tag].decrease();
            }

            // addr
            if(valid)
            {
                if(addrActually != addrPredict)
                {
                    bhist[Tag].resetValid();
                }
            }
            else
            {   
                // 变为有效
                if(takenActually)
                {
                    bhist[Tag].setValid();
                    bhist[Tag].reset();
                    bhist[Tag].increase();
                    bhist[Tag].setAddr(addrActually);
                }
            }
        }
};

// 3. Local-history-based branch predictor
template<size_t L, size_t H, size_t HL = 6, UINT64 BITS = 2>
class LocalHistoryPredictor: public BranchPredictor
{
    SaturatingCnt<BITS> bhist[1 << L];  // PHT中的分支历史字段
    ShiftReg<H> LHT[1 << HL];

    public:
    LocalHistoryPredictor() { }

    PredictResult predict(ADDRINT addr) 
    {
        UINT64 Tag = truncate(addr ^ LHT[truncate(addr, HL)].getVal(), L);
        PredictResult ret;

        ret.taken = bhist[Tag].isTaken();
        ret.valid = bhist[Tag].getValid();
        if(ret.valid)
        {
            ret.targetAddr = bhist[Tag].getAddr();
        }
        else
        {
            ret.targetAddr = 0;
        }
        
        return ret;
    }

    void update(BOOL takenActually, ADDRINT addrActually, PredictResult predicted, ADDRINT addr)
    {
        UINT64 LHTTag = truncate(addr, HL);
        UINT64 Tag = truncate(addr ^ LHT[LHTTag].getVal(), L);
        BOOL valid = bhist[Tag].getValid();
        ADDRINT addrPredict = bhist[Tag].getAddr();

        // LHT update
        LHT[LHTTag].shiftIn(takenActually);

        // PHT update
        if(takenActually)
        {
            bhist[Tag].increase();
        }
        else
        {
            bhist[Tag].decrease();
        }

        // addr
        if(valid)
        {
            if(addrActually != addrPredict)
            {
                bhist[Tag].resetValid();
            }
        }
        else
        {
            if(takenActually)
            {
                bhist[Tag].setValid();
                bhist[Tag].reset();
                bhist[Tag].increase();
                bhist[Tag].setAddr(addrActually);
            }
        }
    }
};

// 4. Bi-Mode branch predictor
template<size_t L, size_t H, UINT64 BITS = 2>
class BiModeHistoryPredictor: public BranchPredictor
{
    SaturatingCnt<BITS> directionTPHT[1 << L];
    SaturatingCnt<BITS> directionNTPHT[1 << L];
    SaturatingCnt<BITS> choosePHT[1 << L];
    ShiftReg<H> GHR;

    public:
        BiModeHistoryPredictor() {}

        PredictResult predict(ADDRINT addr) 
        {
            UINT64 directionTag = truncate(addr ^ GHR.getVal(), L);
            PredictResult ret;
            // 选择方向表1
            if(choosePHT[truncate(addr, L)].isTaken())
            {
                ret.taken = directionTPHT[directionTag].isTaken();
                ret.valid = directionTPHT[directionTag].getValid();
                if(ret.valid)
                {
                    ret.targetAddr = directionTPHT[directionTag].getAddr();
                }
                else
                {
                    ret.targetAddr = 0;
                }
            }
            // 选择方向表2
            else
            {
                ret.taken = directionNTPHT[directionTag].isTaken();
                ret.valid = directionNTPHT[directionTag].getValid();
                if(ret.valid)
                {
                    ret.targetAddr = directionNTPHT[directionTag].getAddr();
                }
                else
                {
                    ret.targetAddr = 0;
                }
            }
            return ret;
        }

        void update(BOOL takenActually, ADDRINT addrActually, PredictResult predicted, ADDRINT addr)
        {
            UINT64 directionTag = truncate(addr ^ GHR.getVal(), L);
            UINT64 chooseTag = truncate(addr, L);
            BOOL chooseTaken = choosePHT[chooseTag].isTaken();
            BOOL chooseUpdate = !(takenActually != chooseTaken && takenActually == takenPredicted);

            // GHR
            GHR.shiftIn(takenActually);

            // 仅被选择的方向PHT更新，若选择PHT和结果不一致，而方向PHT和结果相同，则不更新选择PHT
            if(takenActually)
            {
                // taken
                if(chooseTaken)
                {
                    directionTPHT[directionTag].increase();
                }
                else
                {
                    directionNTPHT[directionTag].increase();
                }
                // choose
                if(chooseUpdate)
                {
                    choosePHT[chooseTag].increase();
                }
            }
            else
            {
                // taken
                if(chooseTaken)
                {
                    directionTPHT[directionTag].decrease();
                }
                else
                {
                    directionNTPHT[directionTag].decrease();
                }
                // choose
                if(chooseUpdate)
                {
                    choosePHT[chooseTag].decrease();
                }
            }

            // addr
            if(chooseUpdate)
            {
                if(directionTPHT[directionTag].getValid())
                {
                    if(addrActually != directionTPHT[directionTag].getAddr())
                    {
                        directionTPHT[directionTag].resetValid();
                    }
                }
                else
                {
                    if(takenActually)
                    {
                        directionTPHT[directionTag].setValid();
                        directionTPHT[directionTag].reset();
                        directionTPHT[directionTag].increase();
                        directionTPHT[directionTag].setAddr(addrActually);
                    }
                }
            }
            else
            {
                if(directionNTPHT[directionTag].getValid())
                {
                    if(addrActually != directionNTPHT[directionTag].getAddr())
                    {
                        directionNTPHT[directionTag].resetValid();
                    }
                }
                else
                {
                    if(takenActually)
                    {
                        directionNTPHT[directionTag].setValid();
                        directionNTPHT[directionTag].reset();
                        directionNTPHT[directionTag].increase();
                        directionNTPHT[directionTag].setAddr(addrActually);
                    }
                }
            }
            
        }
};

/* ===================================================================== */
/* 锦标赛预测器的选择机制可用全局法或局部法实现，二选一即可                   */
/* ===================================================================== */
// 1. Tournament predictor: Select output by global selection history
template<UINT64 BITS = 2>
class TournamentPredictor_GSH: public BranchPredictor
{
    SaturatingCnt<BITS> GSHR;
    BranchPredictor* BPs[2];

    public:
        TournamentPredictor_GSH(BranchPredictor* BP0, BranchPredictor* BP1)
        {
            BPs[0] = BP0;
            BPs[1] = BP1;
        }

        PredictResult predict(ADDRINT addr) 
        {
            if(!GSHR.isTaken())
            {
                return BPs[0]->predict(addr);
            }
            else
            {
                return BPs[1]->predict(addr);
            }
        }

        void update(BOOL takenActually, ADDRINT addrActually, PredictResult predicted, ADDRINT addr)
        {
            BOOL result[2];
            result[0] = BPs[0]->predict(addr);
            result[1] = BPs[1]->predict(addr);

            // BPs update
            BPs[0]->update(takenActually, result[0], addr);
            BPs[1]->update(takenActually, result[1], addr);

            // GSHR update
            if(result[0] == takenActually && result[1] != takenActually)
            {
                GSHR.decrease();
            }
            else if(result[0] != takenActually && result[1] == takenActually)
            {
                GSHR.increase();
            }
        }
};

// 2. Tournament predictor: Select output by local selection history
template<size_t L, UINT64 BITS = 2>
class TournamentPredictor_LSH: public BranchPredictor
{
    SaturatingCnt<BITS> LSHT[1 << L];
    BranchPredictor* BPs[2];

    public:
        TournamentPredictor_LSH(BranchPredictor* BP0, BranchPredictor* BP1)
        {
            BPs[0] = BP0;
            BPs[1] = BP1;
        }

        PredictResult predict(ADDRINT addr) 
        {
            UINT64 Tag = truncate(addr, L);
            if(!LSHT[Tag].isTaken())
            {
                return BPs[0]->predict(addr);
            }
            else
            {
                return BPs[1]->predict(addr);
            }
        }

        void update(BOOL takenActually, ADDRINT addrActually, PredictResult predicted, ADDRINT addr)
        {
            UINT64 Tag = truncate(addr, L);
            BOOL result[2];
            result[0] = BPs[0]->predict(addr);
            result[1] = BPs[1]->predict(addr);

            // BPs update
            BPs[0]->update(takenActually, result[0], addr);
            BPs[1]->update(takenActually, result[1], addr);

            // LSHT update
            if(result[0] == takenActually && result[1] != takenActually)
            {
                LSHT[Tag].decrease();
            }
            else if(result[0] != takenActually && result[1] == takenActually)
            {
                LSHT[Tag].increase();
            }
        }
};

// This function is called every time a control-flow instruction is encountered
void predictBranch(ADDRINT pc, BOOL direction)
{
    ADDRINT targetAddr = IARG_BRANCH_TARGET_ADDR;
    PredictResult prediction = BP->predict(pc);
    BP->update(direction, targetAddr, prediction, pc);

    if (prediction.taken)
    {
        if (direction)
            takenCorrect++;
        else
            takenIncorrect++;
    }
    else
    {
        if (direction)
            notTakenIncorrect++;
        else
            notTakenCorrect++;
    }

    if(prediction.valid == false)
    {
        addrUnpredict++;
    }
    else if(targetAddr == prediction.targetAddr)
    {
        addrCorrect++;
    }
    else
    {
        addrIncorrect++;
    }
}

// Pin calls this function every time a new instruction is encountered
void Instruction(INS ins, void * v)
{
    if (INS_IsControlFlow(ins) && INS_HasFallThrough(ins))
    {
        INS_InsertCall(ins, IPOINT_TAKEN_BRANCH, (AFUNPTR)predictBranch,
                        IARG_INST_PTR, IARG_BOOL, TRUE, IARG_END);

        INS_InsertCall(ins, IPOINT_AFTER, (AFUNPTR)predictBranch,
                        IARG_INST_PTR, IARG_BOOL, FALSE, IARG_END);
    }
}

// This knob sets the output file name
KNOB<string> KnobOutputFile(KNOB_MODE_WRITEONCE, "pintool", "o", "brchPredict.txt", "specify the output file name");

// This function is called when the application exits
VOID Fini(int, VOID * v)
{
	double precision = 100 * double(takenCorrect + notTakenCorrect) / (takenCorrect + notTakenCorrect + takenIncorrect + notTakenIncorrect);
    double addrPrecision = 100 * double(addrCorrect) / (addrCorrect + addrIncorrect + addrUnpredict);

    cout << "takenCorrect: " << takenCorrect << endl
    	<< "takenIncorrect: " << takenIncorrect << endl
    	<< "notTakenCorrect: " << notTakenCorrect << endl
    	<< "nnotTakenIncorrect: " << notTakenIncorrect << endl
    	<< "Precision: " << precision << endl
        << "addrCorrect: " << addrCorrect << endl
        << "addrIncorrect: " << addrIncorrect << endl
        << "addrUnpredict: " << addrUnpredict << endl
        << "addrPrecision: " << addrPrecision << endl;
    
    OutFile.setf(ios::showbase);
    OutFile << "takenCorrect: " << takenCorrect << endl
    	<< "takenIncorrect: " << takenIncorrect << endl
    	<< "notTakenCorrect: " << notTakenCorrect << endl
    	<< "nnotTakenIncorrect: " << notTakenIncorrect << endl
    	<< "Precision: " << precision << endl
        << "addrCorrect: " << addrCorrect << endl
        << "addrIncorrect: " << addrIncorrect << endl
        << "addrUnpredict: " << addrUnpredict << endl
        << "addrPrecision: " << addrPrecision << endl;
    
    OutFile.close();
}

/* ===================================================================== */
/* Print Help Message                                                    */
/* ===================================================================== */

INT32 Usage()
{
    cerr << "This tool counts the number of dynamic instructions executed" << endl;
    cerr << endl << KNOB_BASE::StringKnobSummary() << endl;
    return -1;
}

/* ===================================================================== */
/* Main                                                                  */
/* ===================================================================== */
/*   argc, argv are the entire command line: pin -t <toolname> -- ...    */
/* ===================================================================== */

int main(int argc, char * argv[])
{
    // 19?????!!!!!!!!!!!!
    // TODO: New your Predictor below.
    BP = new BHTPredictor<19>();
    // BP = new GlobalHistoryPredictor<19, 19>();
    // BP = new LocalHistoryPredictor<19, 19>();
    // BP = new BiModeHistoryPredictor<19, 19>();

    // Tournament predictor: Select output by global selection history
    // BranchPredictor* BP0 = new BHTPredictor<19>();
    // BranchPredictor* BP0 = new LocalHistoryPredictor<19, 19>();
    // BranchPredictor* BP1 = new GlobalHistoryPredictor<19, 19>();
    // BP = new TournamentPredictor_GSH<>(BP0, BP1);

    // Tournament predictor: Select output by local selection history
    // BranchPredictor* BP0 = new BHTPredictor<19>();
    // BranchPredictor* BP0 = new LocalHistoryPredictor<19, 19>();
    // BranchPredictor* BP1 = new GlobalHistoryPredictor<19, 19>();
    // BP = new TournamentPredictor_LSH<19>(BP0, BP1);

    // Initialize pin
    if (PIN_Init(argc, argv)) return Usage();
    
    OutFile.open(KnobOutputFile.Value().c_str());

    // Register Instruction to be called to instrument instructions
    INS_AddInstrumentFunction(Instruction, 0);

    // Register Fini to be called when the application exits
    PIN_AddFiniFunction(Fini, 0);

    // Start the program, never returns
    PIN_StartProgram();

    return 0;
}
