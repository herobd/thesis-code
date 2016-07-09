
#ifndef NE_BATCH_Q
#define NE_BATCH_Q

#include <queue>
#include <map>
#include <chrono>

#include <mutex>
#include <iostream>

//#include "Global.h"
#include "Knowledge.h"
#include "SpottingResults.h"

using namespace std;

class NewExemplarsBatch {
public:
    
    NewExemplarsBatch(const vector<Spotting*>& exes, unsigned int maxWidth, int color)
    {
        batchId = _batchId++;
        for (Spotting* s : exes)
        {
            instances.push_back(SpottingImage(*s,maxWidth,color));
            delete s;
        }

    }
    
    
    SpottingImage operator [](int i) const    {return instances[i];}
    SpottingImage & operator [](int i) {return instances[i];}
    SpottingImage at(int i) const    {return instances.at(i);}
    SpottingImage & at(int i) {return instances.at(i);}
    unsigned int size() const { return instances.size();}
    unsigned long getId() {return batchId;}
    
private:
    static unsigned long _batchId;
    unsigned long batchId;
    vector<SpottingImage> instances;
};

class pcomparison
{
    bool reverse;
    public:
    pcomparison()
        {}
    bool operator() (Spotting* lhs, Spotting* rhs) const
    {
        if (lhs->ngramRank==-1)
            lhs->ngramRank = Global::knowledge()->getNgramRank(lhs->ngram);
        if (rhs->ngramRank==-1)
            rhs->ngramRank = Global::knowledge()->getNgramRank(rhs->ngram);
        return (lhs->ngramRank>rhs->ngramRank);
    }
};

class NewExemplarsBatchQueue
{
    public:
        NewExemplarsBatchQueue();
        void enqueue(const vector<Spotting*>& batch);

        NewExemplarsBatch* dequeue(int batchSize, unsigned int maxWidth, int color);

        vector<SpottingExemplar*> feedback(unsigned long id, const vector<int>& userClassifications, vector<pair<unsigned long, string> >* toRemoveExemplars);

        void checkIncomplete();
        void lock() { mutLock.lock(); }
        void unlock() { mutLock.unlock(); }
    private:
        priority_queue<Spotting*, vector<Spotting*>, pcomparison> queue;
        map<unsigned long, NewExemplarsBatch*> returnMap;
        map<unsigned long, chrono::system_clock::time_point> timeMap;
        map<unsigned long, NewExemplarsBatch*> doneMap;
        map<unsigned long, chrono::system_clock::time_point> timeDoneMap;
        mutex mutLock;
};
#endif