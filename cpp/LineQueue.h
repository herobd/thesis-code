#ifndef LINEQUEUE
#define LINEQUEUE

#include "batches.h"
#include "PsuedoWordBackPointer.h"
#include "Knowledge.h"
#include "BatchWraperTranscription.h"

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

    public:
    LineQueue(int contextPad, Knowledge::Corpus* corpus);
    LineQueue(ifstream& in, int contextPad, Knowledge::Corpus* corpus);
    ~LineQueue();
    void save(ofstream& out);
    BatchWraper* getBatch(int width);
};

#endif
