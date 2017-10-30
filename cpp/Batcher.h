#ifndef BATCHER_H
#define BATCHER_H

#include "BatchWraper.h"
#include "batches.h"
#include "spotting.h"
#include <vector>

using namespace std;

class Batcher
{
    public:


    virtual SpottingsBatch* getBatch(int* done, unsigned int num, bool hard, unsigned int maxWidth,int color,string prevNgram, bool need=true) =0;

    virtual vector<Spotting>* feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent=false, vector<pair<unsigned long,string> >* retRemove=NULL, map<string,vector<Spotting> >* forAutoApproval=NULL) =0;

    virtual void autoApprove(vector<Spotting> toApprove, vector<Spotting>* ret) =0;
    virtual BatchWraper* getSpottingsAsBatch(int width, int color, string prevNgram, vector<unsigned long> spottingIds) =0;
    
    virtual bool checkIncomplete() =0;

    string ngram;
};

#endif
