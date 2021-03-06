#ifndef SPOTTING_Q_H
#define SPOTTING_Q_H

#include "MasterQueue.h"
#include "Knowledge.h"
#include <atomic>
#include <map>
#include <list>
#include <semaphore.h>
#include <pthread.h>
#include <queue>

#include "SpottingQuery.h"

using namespace std;

class sqcomparison
{
public:
  sqcomparison()
    {}
  bool operator() (const SpottingQuery* lhs, const SpottingQuery* rhs) const
  {
    return (lhs->getType()==SPOTTING_TYPE_EXEMPLAR && rhs->getType()!=SPOTTING_TYPE_EXEMPLAR);
  }
};

class SpottingQueue
{
    public:
    SpottingQueue(MasterQueue* masterQueue, Knowledge::Corpus* corpus);
    SpottingQueue(ifstream& in, MasterQueue* masterQueue, Knowledge::Corpus* corpus);
    void save(ofstream& out);
    ~SpottingQueue();
    void run(int numThreads);
    void spottingLoop();
    void stop();

    void addQueries(vector<SpottingExemplar*>& exemplars);
    void addQueries(vector<Spotting*>& exemplars);
    void addQueries(vector<Spotting>& exemplars);
    void addQueries(const vector<string>& ngrams);
    void removeQueries(vector<pair<unsigned long,string> >* toRemove);
    bool isRunning() {return inProgress.size()>0 || onDeck.size()>0;}


    protected:
    MasterQueue* masterQueue;
    Knowledge::Corpus* corpus;
    atomic_char cont;
    vector<thread*> spottingThreads;
    map<thread::id,SpottingQuery*> inProgress;
    //map<thread::id,mutex> progLock;
    mutex progLock;

    sem_t semLock;
    mutex mutLock;
    mutex emLock;//"emergency" lock

    //There are two queues, the onDeck on having on instance of each ngram, thus forcing rotations
    list<SpottingQuery*> onDeck;
    map<string,priority_queue<SpottingQuery*,vector<SpottingQuery*>,sqcomparison> > ngramQueues;
    set<unsigned long> emList; //List of "emergency" halts to cancel a spotting that's in progress
    
    void enqueue(SpottingQuery* q);

    SpottingQuery* dequeue();
};

#endif
