#ifndef CLUSTER_BATCHER_H
#define CLUSTER_BATCHER_H

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include <vector>
#include <set>
#include <list>
#include <map>
#include <chrono>
#include <deque>
#include <iostream>
#include "Batcher.h"
#include "Global.h"
#include "PageRef.h"

using namespace std;
using namespace cv;

/*
   This class is intended to replace the SpottingResults class.
   It maintains user classifications and distributes batches similarly.
   Unlike SpottingResults, it does not recieve incremental spotting 
   results, but all at once.

   This class heirchically clusters the results (QbE-wise) and feeds the 
   most promising (highest average QbS score) as the next batch. When
   feedback is recieved, it evaluates the purity of the sent batch and
   computes a runningowed average. This is used to adjust the heirarchy 
   level the clusters are taken from.
*/

#define RUNNING_PURITY_COUNT 7
#define GOAL_PURITY 0.9
#define PURITY_THRESHOLD 0.1

#define RUNNING_ACCURACY_COUNT 10
#define ACCURACY_STOP_THRESH 0.4


class ClusterBatcher : public Batcher
{
public:
    ClusterBatcher(string ngram, int contextPad, bool stepMode, const vector<Spotting>& massSpottingRes, const Mat& crossScores);
    //vector<Spotting>* start(const vector<SpottingLoc>& massSpottingRes, const Mat& crossScores);
    ClusterBatcher(ifstream& in, PageRef* pageRef);
    void save(ofstream& out);

    SpottingsBatch* getBatch(bool* done, unsigned int num, bool hard, unsigned int maxWidth,int color,string prevNgram, bool need=true);
    
    vector<Spotting>* feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent=false, vector<pair<unsigned long,string> >* retRemove=NULL);
    
    bool checkIncomplete();

    unsigned long getId() {return id;}

    string ngram;

    //bool checkIncomplete();

private:
    static atomic_ulong _id;

    unsigned long id;
    int contextPad;
    bool stepMode; //Whether to progress through batches by finding nearest clusters to approved examples, or simply rely on mean QbS of clusters

    bool finished; //Whether we've hit our threshold for accuracy in clusters batched out

    //For testing purposes
    vector<float> meanCPurity;//,  medianCPurity,  meanIPurity,  medianIPurity,  maxPurity;

    vector< vector< list<int> > > clusterLevels;
    //vector< vector< int > > instanceToCluster;
    vector<float> averageClusterSize;
    //map<int,Mat> minSimilarities;
    Mat crossScores;

    vector<Spotting> spottingRes;
    deque<int> trueInstancesToSeed;

    map<unsigned long, chrono::system_clock::time_point > starts; //For tracking sent batches

    float runningPurity;//Current mean batch purity over a runningow
    deque<float> windowPurity;//runningow
    float runningAccuracy;//Current mean batch accuracy over a runningow
    deque<float> windowAccuracy;//runningow

    int curLevel; //Current cluster level we are drawing from. Adjusted to maintain purity


    void CL_cluster(vector< list<int> >& clusters, Mat& minSimilarity, int numClusters, const vector<bool>& gt);
};

#endif
