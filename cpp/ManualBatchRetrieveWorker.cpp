//#include "ManualBatchRetrieveWorker.h"

#include <nan.h>
#include <iostream>
#include <assert.h>

#include "CATTSS.h"
#include "BatchWraper.h"
using namespace Nan;
using namespace std;
using namespace v8;

class ManualBatchRetrieveWorker : public AsyncWorker {
    public:
        ManualBatchRetrieveWorker(Callback *callback, CATTSS* cattss, int width, int line)
        : AsyncWorker(callback), width(width), cattss(cattss), line(line) {}


        ~ManualBatchRetrieveWorker() {}


        void Execute () {
            if (line>=0)
                batch = cattss->getLineBatch(width,line);
            else
                batch = cattss->getManualBatch(width);
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
        CATTSS* cattss;
        int line;
        
};


