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

template <size_t N, UINT64 init = (1 << N)/2 - 1>   // N < 64
class SaturatingCnt
{
    UINT64 val;
    public:
        SaturatingCnt() { reset(); }

        void increase() { if (val < (1 << N) - 1) val++; }
        void decrease() { if (val > 0) val--; }

        void reset() { val = init; }
        UINT64 getVal() { return val; }

        BOOL isTaken() { return (val > (1 << N)/2 - 1); }
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
        virtual BOOL predict(ADDRINT addr) { return FALSE; };
        virtual void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr) {};
};

BranchPredictor* BP;


/* ===================================================================== */
/* ������ʵ��2�ֶ�̬Ԥ�ⷽ��                                               */
/* ===================================================================== */
// 1. BHT-based branch predictor
template<size_t L>
class BHTPredictor: public BranchPredictor
{
    SaturatingCnt<2> counter[1 << L];
    
    public:
        BHTPredictor() { }

        BOOL predict(ADDRINT addr)
        {
            return counter[truncate(addr, L)].isTaken();
        }

        void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
        {
            UINT64 Tag = truncate(addr, L);

            // 10->01
            if(takenActually)
            {
                counter[Tag].increase();
            }
            else
            {
                counter[Tag].decrease();
            }

            // 10->00
            // if(takenPredicted)
            // {
            //     if(takenActually)
            //     {
            //         counter[Tag].increase();
            //     }
            //     else
            //     {
            //         counter[Tag].decrease();
            //         if(!counter[Tag].isTaken())
            //         {
            //             counter[Tag].decrease();
            //         }
            //     }
            // }
            // else
            // {
            //     if(takenActually)
            //     {
            //         counter[Tag].increase();
            //         if(counter[Tag].isTaken())
            //         {
            //             counter[Tag].increase();
            //         }
            //     }
            //     else
            //     {
            //         counter[Tag].decrease();
            //     }
            // }
        }
};

// 2. Global-history-based branch predictor
template<size_t L, size_t H, UINT64 BITS = 2>
class GlobalHistoryPredictor: public BranchPredictor
{
    SaturatingCnt<BITS> bhist[1 << L];  // PHT�еķ�֧��ʷ�ֶ�
    ShiftReg<H> GHR;
    
    public:
        GlobalHistoryPredictor() { }

        BOOL predict(ADDRINT addr)
        {
            UINT64 Tag = truncate(addr ^ GHR.getVal(), L);
            return bhist[Tag].isTaken();
        }

        void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
        {
            UINT64 Tag = truncate(addr ^ GHR.getVal(), L);

            GHR.shiftIn(takenActually);
            if(takenActually)
            {
                bhist[Tag].increase();
            }
            else
            {
                bhist[Tag].decrease();
            }
        }
};

// 3. Local-history-based branch predictor
template<size_t L, size_t H, size_t HL = 6, UINT64 BITS = 2>
class LocalHistoryPredictor: public BranchPredictor
{
    SaturatingCnt<BITS> bhist[1 << L];  // PHT�еķ�֧��ʷ�ֶ�
    ShiftReg<H> LHT[1 << HL];

    public:
    LocalHistoryPredictor() { }

    BOOL predict(ADDRINT addr)
    {
        UINT64 LH = LHT[truncate(addr, HL)].getVal();
        UINT64 Tag = truncate(addr ^ LH, L);
        // printf("truncate:%ld LH:%ld Tag:%ld isTaken:%d\n", truncate(addr, H), LH, Tag, bhist[Tag].isTaken());
        return bhist[Tag].isTaken();
    }

    void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
    {
        UINT64 LHTTag = truncate(addr, HL);
        UINT64 Tag = truncate(addr ^ LHT[LHTTag].getVal(), L);

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

        BOOL predict(ADDRINT addr)
        {
            UINT64 directionTag = truncate(addr ^ GHR.getVal(), L);
            if(choosePHT[truncate(addr, L)].isTaken())
            {
                return directionTPHT[directionTag].isTaken();
            }
            else
            {
                return directionNTPHT[directionTag].isTaken();
            }
        }

        void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
        {
            UINT64 directionTag = truncate(addr ^ GHR.getVal(), L);
            UINT64 chooseTag = truncate(addr, L);
            BOOL chooseTaken = choosePHT[chooseTag].isTaken();
            BOOL chooseUpdate = !(takenActually != chooseTaken && takenActually == takenPredicted);

            GHR.shiftIn(takenActually);
            // ����ѡ��ķ���PHT���£���ѡ��PHT�ͽ����һ�£�������PHT�ͽ����ͬ���򲻸���ѡ��PHT
            if(takenActually)
            {
                if(chooseTaken)
                {
                    directionTPHT[directionTag].increase();
                }
                else
                {
                    directionNTPHT[directionTag].increase();
                }
                if(chooseUpdate)
                {
                    choosePHT[chooseTag].increase();
                }
            }
            else
            {
                if(chooseTaken)
                {
                    directionTPHT[directionTag].decrease();
                }
                else
                {
                    directionNTPHT[directionTag].decrease();
                }
                if(chooseUpdate)
                {
                    choosePHT[chooseTag].decrease();
                }
            }
        }
};

/* ===================================================================== */
/* ������Ԥ������ѡ����ƿ���ȫ�ַ���ֲ���ʵ�֣���ѡһ����                   */
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

        BOOL predict(ADDRINT addr)
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

        void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
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

        BOOL predict(ADDRINT addr)
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

        void update(BOOL takenActually, BOOL takenPredicted, ADDRINT addr)
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
    BOOL prediction = BP->predict(pc);
    BP->update(direction, prediction, pc);
    if (prediction)
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
    
    cout << "takenCorrect: " << takenCorrect << endl
    	<< "takenIncorrect: " << takenIncorrect << endl
    	<< "notTakenCorrect: " << notTakenCorrect << endl
    	<< "nnotTakenIncorrect: " << notTakenIncorrect << endl
    	<< "Precision: " << precision << endl;
    
    OutFile.setf(ios::showbase);
    OutFile << "takenCorrect: " << takenCorrect << endl
    	<< "takenIncorrect: " << takenIncorrect << endl
    	<< "notTakenCorrect: " << notTakenCorrect << endl
    	<< "nnotTakenIncorrect: " << notTakenIncorrect << endl
    	<< "Precision: " << precision << endl;
    
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
    // TODO: New your Predictor below.
    BP = new BHTPredictor<19>();
    // BP = new GlobalHistoryPredictor<19, 19>();
    // ????????????????????????H&HL
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
