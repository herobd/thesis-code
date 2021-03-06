
#ifndef BATCH_WRAPER_TRANS
#define BATCH_WRAPER_TRANS
#ifndef NO_NAN
#include <nan.h>
#define BUFFERSIZE 65536
#include <b64/encode.h>
#endif
#include <iostream>
#include <assert.h>
#include "opencv2/highgui/highgui.hpp"
#include "batches.h"
#include "spotting.h"
#include "BatchWraper.h"

#ifndef NO_NAN
using namespace Nan;
using namespace v8;
#endif
using namespace std;
class BatchWraperTranscription: public BatchWraper
{
    protected:
        //output
        string batchId;
        string wordImgStr;
        string ngramImgStr;
        string wordIndex;
        string gt;
        double scale;
        vector<string> retPoss;
        vector<SpottingPoint> spottings;
        bool manual;

    public:
        BatchWraperTranscription(TranscribeBatch* batch);
        //~BatchWraperTranscription() {}
#ifndef NO_NAN
        virtual void doCallback(Callback* callback);
#else
        virtual int getType(){return TRANSCRIPTION;}
        virtual void getTranscription(string* batchId,int* wordIndex, vector<SpottingPoint>* spottings, vector<string>* poss, bool* manual, string* gt);
#endif
};
#endif
