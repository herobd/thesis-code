#ifndef LINEQUEUE
#define LINEQUEUE

#include "batches.h"
#include "PsuedoWordBackPointer.h"
#include "Knowledge.h"
#include "BatchWraperTranscription.h"
#include <mutex>

using namespace std;

class LineQueue
{
    private:
    Knowledge::Corpus* corpus;
    vector<TranscribeBatch> batches;
    atomic_uint on;
    vector<PsuedoWordBackPointer*> origins;
    multimap<int,Spotting> spottings;
    int contextPad;
    int totalWords;
    int wordsDone;
    float sumAcc;
#ifdef NO_NAN
    mutex mutLock;
#endif

    public:
    LineQueue(int contextPad, Knowledge::Corpus* corpus);
    LineQueue(ifstream& in, int contextPad, Knowledge::Corpus* corpus);
    ~LineQueue();
    void save(ofstream& out);
    BatchWraper* getBatch(int width);
    void feedback(string trans, int line);
    void getStats(float* accTrans, float* pWordsTrans);
};

#endif
