//#include "LineBatchRetrieveWorker.h"

#include <nan.h>
#include <iostream>
#include <assert.h>

#include "LineManTrans.h"
#include "BatchWraper.h"
using namespace Nan;
using namespace std;
using namespace v8;

class LineBatchRetrieveWorker : public AsyncWorker {
    public:
        LineBatchRetrieveWorker(Callback *callback, LineManTrans* lineManTrans, int width)
        : AsyncWorker(callback), width(width), lineManTrans(lineManTrans) {}


        ~LineBatchRetrieveWorker() {}


        void Execute () {
            batch = lineManTrans->getBatch(width);
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
        LineManTrans* lineManTrans;
        
        
};


