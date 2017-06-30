#ifndef BATCH_WRAPER
#define BATCH_WRAPER

#include "Location.h"

#ifndef NO_NAN
#include <nan.h>
using namespace Nan;
using namespace v8;
#else
#include "opencv2/core/core.hpp"
using namespace cv;
#define SPOTTINGS 1
#define NEW_EXEMPLARS 2
#define TRANSCRIPTION 3
#define RAN_OUT 4
#include "batches.h"
#endif
using namespace std;


class BatchWraper
{
    public:
        virtual ~BatchWraper() {}
        //virtual BatchWraper(Batch* batch)=0;
#ifndef NO_NAN
        virtual void doCallback(Callback* callback)=0;
#else
        virtual int getType()=0;
        virtual void getSpottings(string* resId,string* ngram, vector<string>* ids, vector<Location>* locs, vector<string>* gt) {}
        virtual void getNewExemplars(string* batchId,vector<string>* ngrams, vector<Location>* locs) {}
        virtual void getTranscription(string* batchId,int* wordIndex, vector<SpottingPoint>* spottings, vector<string>* poss, bool* manual, string* gt) {}
        virtual vector<Mat> getImages()
        {
            return images;
        }
    protected:
        vector<Mat> images;
#endif
};

class BatchWraperBlank : public BatchWraper
{
    public:
#ifndef NO_NAN
        virtual void doCallback(Callback* callback)
        {

            Nan:: HandleScope scope;
            Local<Value> argv[] = {
                Nan::Null(),
                Nan::Null(),
                Nan::Null(),
                Nan::Null(),
                Nan::Null(),
                Nan::Null(),
                Nan::Null(),
                Nan::Null()
            };

            callback->Call(8, argv);

        }
#else
        virtual int getType(){return 0;}
#endif
        
};

#ifdef NO_NAN
class BatchWraperRanOut : public BatchWraper
{
    public:
        virtual int getType(){return RAN_OUT;}
        
};
#endif

#endif
