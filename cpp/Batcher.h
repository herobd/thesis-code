#ifndef BATCHER_H
#define BATCHER_H

#include "batches.h"
#include "spotting.h"
#include <vector>

using namespace std;

class Batcher
{
    public:


    virtual SpottingsBatch* getBatch(bool* done, unsigned int num, bool hard, unsigned int maxWidth,int color,string prevNgram, bool need=true) =0;

    virtual vector<Spotting>* feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent=false, vector<pair<unsigned long,string> >* retRemove=NULL) =0;
    
    virtual bool checkIncomplete() =0;

    string ngram;
};

#endif
