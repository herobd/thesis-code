//#include "BatchRetrieveWorker.h"

#include <nan.h>
#include <iostream>
#include <assert.h>
#include "opencv2/highgui/highgui.hpp"

#include "CATTSS.h"
#include "BatchWraper.h"
using namespace Nan;
using namespace std;
using namespace v8;

class BatchRetrieveWorker : public AsyncWorker {
    public:
        BatchRetrieveWorker(Callback *callback, CATTSS* cattss, int width, int color, string prevNgram, int num)
        : AsyncWorker(callback), width(width), color(color), prevNgram(prevNgram), num(num), cattss(cattss), batch(NULL) {}

        BatchRetrieveWorker(Callback *callback, CATTSS* cattss, int width, int color, string prevNgram, unsigned long batcherId, vector<unsigned long> spottingIds, string ngram)
        : AsyncWorker(callback), width(width), color(color), prevNgram(prevNgram), num(-1), cattss(cattss), batch(NULL), batcherId(batcherId), spottingIds(spottingIds), ngram(ngram) {}

        ~BatchRetrieveWorker() {}


        void Execute () {
            if (spottingIds.size()==0)
                batch = cattss->getBatch(num,width,color,prevNgram);
            else
                batch = cattss->getSpottingsAsBatch(width,color,prevNgram,batcherId,spottingIds,ngram);
        }

        // We have the results, and we're back in the event loop.
        void HandleOKCallback () {
            batch->doCallback(callback);
            delete batch;
        }
    private:
        //output
        BatchWraper* batch;
        //input
        int width;
        int color;
        string prevNgram;
        int num;
        CATTSS* cattss;
        unsigned long batcherId;
        vector<unsigned long> spottingIds;
        string ngram;
        
        
};


