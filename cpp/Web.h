
#ifndef WEB_H
#define WEB_H

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>
#include <set>
#include <list>
#include <map>
#include <chrono>

#include <iostream>
#include "batches.h"
#include "Global.h"
#include "PageRef.h"
#include "maxflow/graph.h"

typedef Graph<float,float,float> GraphType;

/*
   This class is inteded to handle most of the functionalilty of the interconnected/graph spotting approval.
   It recieves the top x% of QbE spottings, then spots these against eachother exhaustively.
   The QbE scores are normalized to be similar to the QbE scores.
   The graph is then created and cut.
   The classifications (cuts) returned [ and added to words via corpus->updateSpottings(...) ]
   Web gets transcription feedback, and adjusts, recuts, and returns spotting classftin changes.
*/

class Web
{
public:
    Web() : graph(NULL) {};
    ~Web()
    {
        if (graph!=NULL)
            delete graph;
    }
    vector<Spotting>* start(const vector<string>& ngrams);
    vector<pair<unsigned long,string> > badSpotting(unsigned long sid);

private:
    GraphType* graph;
    map<string,map<unsigned long,pair<arc*,arc*> > > arcMap;

    map<string,int> ngramToNode;
    map<unsigned long,int> slidToNode;
    map<unsigned long,unsigned long> sidToSlid;
    map<unsigned long, ngram> classifications;


    list<unsigned long> toRemove;
    mutex toRemoveMut, runnningMut, waitiningMut;
};

#endif
