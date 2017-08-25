
#ifndef WEB_H
#define WEB_H

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>
#include <set>
#include <list>
#include <map>
#include <mutex>

#include <iostream>
#include "spotting.h"
#include "Global.h"
#include "PageRef.h"
#include "maxflow/graph.h"
#include "Knowledge.h"

using namespace std;
using namespace cv;

typedef Graph<float,float,float> GraphType;
typedef Graph<float, float, float>::arc_id arc_id;

/*
   This class is inteded to handle most of the functionalilty of the interconnected/graph spotting approval.
   It recieves the top x% of QbS spottings and their cross scorings (QbE).
   (The QbS scores are assumed to be normalized to be similar to the QbE scores.)
   The graph is then created and cut.
   The classifications (cuts) are returned as Spotting objects
   Web gets transcription feedback, and adjusts, recuts, and returns spotting classftin changes.
*/

#define REMOVE_SCORE 0

class Web
{
public:
    Web(Knowledge::Corpus* corpus) : corpus(corpus), graph(NULL) {};
    Web(ifstream& in, Knowledge::Corpus* corpus);
    void save (ofstream& out);
    ~Web()
    {
        if (graph!=NULL)
            delete graph;
    }
    vector<Spotting>* start(const vector<string>& ngrams, const vector<SpottingLoc>& massSpottingRes, const Mat& crossScores);
    void badSpotting(unsigned long sid, vector<Spotting>* add, vector<pair<unsigned long,string> >* remove);

private:
    Knowledge::Corpus* corpus;
    list<string> ngrams;
    map<unsigned long, SpottingLoc> allSpottings;
    Mat crossScores;

    //filled by makeGraph()
    GraphType* graph;
    map<string,map<unsigned long,pair<arc_id,arc_id> > > arcMap;
    map<string,int> ngramToNode;
    map<unsigned long,int> slidToNode;
    map<unsigned long,unsigned long> sidToSlid;
    map<unsigned long,unsigned long> slidToSid;
    bool reuse;//should reuse comp

    map<unsigned long, string> classifications;


    list<unsigned long> toRemove;
    //list<unsigned long> removed;
    mutex toRemoveMut, runningMut, waitingMut;

    void makeGraph();//sets up the graph object using ngrams, allSpottings, and crossScores
    map<unsigned long, string> classify();//performs the multiway-cut on the graph, return which ngram each SpottingLoc was classified as
    void redoCuts(vector<Spotting>* add, vector<pair<unsigned long,string> >* remove);//
};

#endif
