#ifndef TRANS_BATCH_Q
#define TRANS_BATCH_Q

#include <deque>
#include <map>
#include <chrono>

#include <mutex>
#include <iostream>

#include "spotting.h"
#include "batches.h"
#include "WordBackPointer.h"
#include "CorpusRef.h"
#include "Global.h"

#define TBQ_NONE 0
#define TBQ_ERROR 1
#define TBQ_REMOVE 2
#define TBQ_RESULT 3

using namespace std;

class TranscribeBatchQueue
{
    public:
        TranscribeBatchQueue();
        ~TranscribeBatchQueue()
        {
            for (auto t : queue)
                delete t;
            for (auto t : returnMap)
                delete t.second;
        }
            
        void save(ofstream& out);
        void load(ifstream& in, CorpusRef* corpusRef);

        void enqueueAll(vector<TranscribeBatch*> batches, vector<unsigned long>* remove=NULL);

        TranscribeBatch* dequeue(unsigned int maxWidth, bool need=false);

        vector<Spotting*> feedback(unsigned long id, string transcription, vector<pair<unsigned long, string> >* toRemoveExemplars, unsigned long* badSpotting=NULL);

        void checkIncomplete();
        void setContextPad(int contextPad) {this->contextPad=contextPad;}
    private:
        deque<TranscribeBatch*> queue;
        map<unsigned long, TranscribeBatch*> returnMap;
        map<unsigned long, chrono::system_clock::time_point> timeMap;
        map<unsigned long, WordBackPointer*> doneMap;
#if TRANS_DONT_WAIT
        deque<TranscribeBatch*> lowQueue;
        map<unsigned long, TranscribeBatch*> lowReturnMap;
        map<unsigned long, chrono::system_clock::time_point> lowTimeMap;
        map<unsigned long, WordBackPointer*> lowDoneMap;
#endif
        mutex mutLock;
        int contextPad;
        void lock() { mutLock.lock(); }
        void unlock() { mutLock.unlock(); }
        int feedbackProcess(unsigned long id, string transcription, vector<pair<unsigned long, string> >* toRemoveExemplars, WordBackPointer* backPointer, bool resend, deque<TranscribeBatch*>& queue, map<unsigned long, TranscribeBatch*>& returnMap, map<unsigned long, chrono::system_clock::time_point>& timeMap, map<unsigned long, WordBackPointer*>& doneMap, vector<Spotting*>* newNgramExemplars, unsigned long* badSpotting=NULL);
};
#endif
