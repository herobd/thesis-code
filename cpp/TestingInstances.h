#ifndef TESTING_INST
#define TESTING_INST

#include <mutex>
#include <map>
#include "BatchWraperSpottings.h"
#include "BatchWraperTranscription.h"
#include "spotting.h"
#include "batches.h"
#include "SpecialInstances.h"
#include "Knowledge.h"

class TestingInstances: public SpecialInstances
{
    public:
        TestingInstances(const Knowledge::Corpus* corpus);
        ~TestingInstances()
        {
            for (auto p: spottings)
            {
                for (Spotting* s : p.second)
                    delete s;
            }
            for (TranscribeBatch* t : trans)
                delete t;
            for (TranscribeBatch* t : manTrans)
                delete t;
        }
        BatchWraper* getBatch(int width, int color, string prevNgram, int testingNum);
        void addSpotting(string ngram, bool label, int pageId, int tlx, int tly, int brx, int bry);
        void addTrans(string label, vector<string> poss, multimap<string,Location> spots, int wordIdx, bool manual);
        void allLoaded();

        
    private:
        Knowledge::Word* dummyWord;
        vector<char> testNumType;

        vector<string> ngramList;

        map<int, vector<bool> > ngramsUsed;
        map<int, mutex> ngramsMut;


        map<string,vector<Spotting*> > spottings;
        map<string,vector<bool> > spottingsUsed;
        map<string,mutex> spottingsMut;

        vector<TranscribeBatch*> trans;
        vector<bool> transUsed;
        mutex transMut;
        vector<TranscribeBatch*> manTrans;
        vector<bool> manTransUsed;
        mutex manTransMut;

        const Knowledge::Corpus* corpus;

        int getNextIndex(vector<bool>& setUsed, mutex& mutLock);
        BatchWraper* getSpottingsBatch(string ngram, int width, int color, string prevNgram);
        BatchWraper* getManTransBatch(int width);
        BatchWraper* getTransBatch(int width);
        BatchWraper* makeInstance(int testingNum, int width,int color, string prevNgram);
};
#endif
