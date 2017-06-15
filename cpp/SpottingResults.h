
#ifndef SPOTTINGRESULTS_H
#define SPOTTINGRESULTS_H

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

#define TAIL_DENSITY_TRUE_THRESHOLD 0.0139
#define GOOD_TAIL_SCORE -0.01 //Hueristic

#define ACCEPT_THRESH 2 //number of false instances acceptable above accept threshold
#define TAIL_THRESH 20 //approximate number of instances in the "tail"

#define CVT_MARGIN 0.05

#define UPDATE_OVERLAP_THRESH 0.4
#define UPDATE_OVERLAP_THRESH_TIGHT 0.7

#define CHECK_IF_BAD_SPOTTING_START 50
#define CHECK_IF_BAD_SPOTTING_THRESH 0.035

#define TAIL_CONTINUE_THRESH 0.5

#define RUNNING_CLASSIFICATIONS_COUNT 20

#define MAX_REJECT_THRESHOLD_FROM_FALSE 1.1
#define MAX_ACCEPT_THRESHOLD_FROM_FALSE 1.6

using namespace std;





class scoreCompById
{
  const map<unsigned long,Spotting>* instancesById;
  bool useQbE;
public:
  scoreCompById(const map<unsigned long,Spotting>* instancesById=NULL, bool useQbE=false) : instancesById(instancesById), useQbE(useQbE)
    {}
  bool operator() (unsigned long lhs, unsigned long rhs) const
  {
      if (useQbE)
        return (instancesById->at(lhs).scoreQbE<instancesById->at(rhs).scoreQbE);
      else
        return (instancesById->at(lhs).scoreQbS<instancesById->at(rhs).scoreQbS);
  }
};
/*class scoreComp
{
  bool reverse;
public:
  scoreComp(const bool& revparam=false)
    {reverse=revparam;}
  bool operator() (const Spotting* lhs, const Spotting* rhs) const
  {
    if (reverse) return (lhs->score>rhs->score);
    else return (lhs->score<rhs->score);
  }
};
class tlCompById
{
  const map<unsigned long,Spotting>* instancesById;
public:
  tlCompById(const map<unsigned long,Spotting>* instancesById=NULL) : instancesById(instancesById)
    {}
  bool operator() (unsigned long lhs, unsigned long rhs) const
  {
      const Spotting& r = instancesById->at(rhs);
      const Spotting& l = instancesById->at(lhs);
      if (l.pageId == r.pageId)
      {
          if (l.tlx == r.tlx)
              return l.tly < r.tly;
          else
              return l.tlx < r.tlx;
      }
      else
          return l.pageId < r.pageId;
  }
};*/

class tlComp
{
  bool reverse;
public:
  tlComp(const bool& revparam=false)
    {reverse=revparam;}
  bool operator() (const Spotting& lhs, const Spotting& rhs) const
  {
      bool ret;
      if (lhs.pageId == rhs.pageId)
      {
          if (lhs.tlx == rhs.tlx)
              ret=lhs.tly < rhs.tly;
          else
              ret = lhs.tlx < rhs.tlx;
      }
      else
          ret = lhs.pageId < rhs.pageId;
    if (reverse) return !ret;
    else return ret;
  }
};


/* This class holds all of the results of a single spotting query.
 * It orders these by score. It tracks an accept and reject 
 * threshold, indicating the score we are confident we can either
 * accept a score at, or reject a score at. These thresholds are
 * adjusted as user-classifications are returned to it. The
 * thresholds are initailly set by a global criteria, which this
 * returns to when it is finished.
 * It also facilitates generating a batch from the results. This
 * is done by detirming the midpoint between the accept threshold
 * and the reject threshold, and taking the X number of spottings
 * from that point in ordered results.
 */
class SpottingResults {
public:
    SpottingResults(string ngram, int contextPad);
    SpottingResults(ifstream& in, PageRef* pageRef);
    void save(ofstream& out);

    string ngram;
    
    ~SpottingResults() 
    {
        cout <<"results["<<id<<"]:"<<ngram<<", numberClassifiedTrue: "<<numberClassifiedTrue<<", numberClassifiedFalse: "<<numberClassifiedFalse<<", numberAccepted: "<<numberAccepted<<", numberRejected: "<<numberRejected<<endl;
        float effortR=numberAccepted/(0.0+numberClassifiedTrue+numberClassifiedFalse);
        cout<<"* effort reduction: "<<effortR<<endl;
        //assert(effortR>0);
    }
    
    //sem_t mutexSem;
    
    //stats
    int numberClassifiedTrue;
    int numberClassifiedFalse;
    int numberAccepted;
    int numberRejected;
    
    unsigned long getId()
    {
        return id;
    }
    
    //For use when creating SpottingResult
    void add(Spotting spotting);

    //For use when creating SpottingResult Adds a spotting which is a new exemplar. We just want to prevent a future redundant classification of it.
    void addTrueNoScore(const SpottingExemplar& spotting);

    //This will either replace the spottings or add new ones if it can't find any close enough. spottings is consumed.
    //The return value merely indicates whether this needs "resurrected" (Put back into the MasterQueue).
    bool updateSpottings(vector<Spotting>* spottings);
    
    //This accpets a spotting which is a new exemplar. We just want to prevent a future redundant classification of it.
    void updateSpottingTrueNoScore(const SpottingExemplar& spotting);

    SpottingsBatch* getBatch(bool* done, unsigned int num, bool hard, unsigned int maxWidth,int color,string prevNgram, bool need=true);
    
    vector<Spotting>* feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent=false, vector<pair<unsigned long,string> >* retRemove=NULL);
    
    bool checkIncomplete();

#ifdef TEST_MODE
    int setDebugInfo(SpottingsBatch* b);
    void saveHistogram(float actualModelDif);
#endif
    void debugState() const;
    
private:
    static atomic_ulong _id;
    
    unsigned long id;
    double acceptThreshold;
    double rejectThreshold;
    //int numBatches;
    bool allBatchesSent;
    bool done;
    bool useQbE; //This indicates whether enough exemplars have been combined to make QbE reliable
    float cvtMax; //A max value taken when switching to QbE mode. It must remain constant
    int numComb;
    
    float trueMean;
    float trueVariance;

    //When QbE is on, these are assumed to be the log version of the parameters
    //When QbS, the mean is fixed to (near) the max score
    float falseMean;
    float falseVariance;

    //This is here for testing purposes
    float usedMean, usedStd, trueFalseDivide;

    bool takeFromTail; //This means we don't have good enough spotting to try and auto approve any, and modelling the true distribution will be difficult
    float tailEnd;
    float tailEndScore;

    double falseProbWeight;

    int distCheckCounter;

    //float lastDifAcceptThreshold;
    //float lastDifRejectThreshold;
    float lastDifPullFromScore; //The delta for the monentum
    float momentum;
    
    float pullFromScore; //The choosen score from which batches are pulled (around the score, alternatively grt than and less than).
    
    float maxScoreQbE, maxScoreQbS;
    float minScoreQbE, minScoreQbS;
    float& maxScore()
    {
        if (useQbE)
            return maxScoreQbE;
        else
            return maxScoreQbS;
    }
    float& minScore()
    {
        if (useQbE)
            return minScoreQbE;
        else
            return minScoreQbS;
    }
    float maxScore() const
    {
        if (useQbE)
            return maxScoreQbE;
        else
            return maxScoreQbS;
    }
    float minScore() const
    {
        if (useQbE)
            return minScoreQbE;
        else
            return minScoreQbS;
    }

    int numLeftInRange; //This actually has the count from the round previous, for efficency


    //This multiset orders the spotting results to ease the extraction of batches
    //multiset<Spotting*,scoreComp> instancesByScore; //This holds Spottings yet to be classified
    //multiset<unsigned long,scoreCompById> instancesByScore; //This holds Spottings yet to be classified
    multimap<float,unsigned long> instancesByScore; //This holds Spottings yet to be classified
    multiset<Spotting,tlComp> instancesByLocation; //This is a convienince holder of all Spottings
    map<unsigned long,Spotting> instancesById; //This is all the Spottings
    map<unsigned long,bool> classById; //This is the classifications of Spottings

    int numSpottingsQbEMax; //This sets a cap on how many instances we hang on to when combining with QbE.
    multimap<float,unsigned long> allInstancesByScoreQbE;//for pruning the number of instances to a set size. When we combine, we accumulate instances
    //vector<Spotting> instancesToAddQbE; //These are instances without QbS scores, and thus cann't be added to instancesByScore.
    multiset<Spotting,tlComp> instancesToAddQbEByLocation;//For checking for overlaps/duplicates when still in QbS mode
    set<unsigned long> kickedOut; //Just debugging, which instances QbE accumulation kicked out durting QbS mode

    bool instancesByScoreContains(unsigned long id) const
    {
        float score = instancesById.at(id).score(useQbE);
        auto range = instancesByScore.equal_range(score);
        for (auto iter=range.first; iter!=range.second; iter++)
            if (iter->second==id)
                return true;
        return false;
    }
    bool instancesByScoreErase(unsigned long id)
    {
        float score = instancesById.at(id).score(useQbE);
        auto range = instancesByScore.equal_range(score);
        for (auto iter=range.first; iter!=range.second; iter++)
            if (iter->second==id)
            {
                instancesByScore.erase(iter);
                return true;
            }
        return false;
    }
    bool allInstancesByScoreQbEErase(unsigned long id)
    {
        float score = instancesById.at(id).scoreQbE;
        auto range = allInstancesByScoreQbE.equal_range(score);
        for (auto iter=range.first; iter!=range.second; iter++)
            if (iter->second==id)
            {
                allInstancesByScoreQbE.erase(iter);
                return true;
            }
        return false;
    }
    void instancesByScoreInsert(unsigned long id)
    {
        float score = instancesById.at(id).score(useQbE);
        assert(score==score);
        instancesByScore.emplace(score,id);
    }
    
    bool instancesByLocationErase(unsigned long id)
    {
        //const Spotting& s = instancesById.at(id);
        auto range = instancesByLocation.equal_range(instancesById.at(id));//Spotting(s.tlx, s.tly, s.brx, s.bry,s.pageId));
        for (auto iter=range.first; iter!=range.second; iter++)
            if (iter->id==id)
            {
                instancesByLocation.erase(iter);
                return true;
            }
        return false;
    }

    map<unsigned long, chrono::system_clock::time_point > starts;
   
    //This provides a mapping of ids to allow a feedback of a spotting to be properly mapped if it was updated
    map<unsigned long, unsigned long> updateMap;
#ifdef TEST_MODE
    map<unsigned long, unsigned long> testUpdateMap;
    int atn, rtn;

    string histFile;
#endif

    //This acts as a pointer to where we last extracted a batch to speed up searching for the correct score area to extract a batch from
    //multiset<unsigned long,scoreCompById>::iterator tracer;
    multimap<float,unsigned long>::iterator tracer;
    default_random_engine generator;
    
    //SpottingImage getNextSpottingImage(bool* done, int maxWidth,int color,string prevNgram);
   
    //This uses expectation maximization (one iteration each call) to produce new appect/reject thresholds. It assumes a bimodal gaussian distribution, one for positive spottings and one for negative.
    //returns (and sets allBatchesSent) whether we are now done (all spottings lie outside the thresholds)
    //It uses an Otsu threshold to be used to estimate some initail parameters on the first run.
    bool EMThresholds(int swing=0);

    list<bool> runningClassifications;
    float runningClassificationTrueAverage();
    void updateRunningClassifications(const vector<int>& newClassifications);

    float NPD(float x, float mean, float var); //normal probability distribution function
    float PHI(float x); //cumulative distribution function of normal distribution
    float PHI(float x, float mean, float std);
    float logCvt(float x);
    float expCvt(float x);

    //This returns the iterator of instancesByLocation for the spotting which overlaps (spatailly) the one given
    //It returns instancesByLocation.end() if none is found.
    //It sets ratioOff to indicate how far off the overlap is, 1 beng the max allowed by the threshold, 0 being perfectly alligned
    multiset<Spotting,tlComp>::iterator findOverlap(const Spotting& spotting, float* ratioOff) const;

    //How much to pad (top and bottom) images sent to users
    int contextPad;
    int batchesSinceChange;
#ifdef GRAPH_SPOTTING_RESULTS
    string undoneGraphName;
    cv::Mat undoneGraph;
    string fullGraphName;
    cv::Mat fullGraph;
    multimap<float,unsigned long> fullInstancesByScore;
#endif
    
    /*bool tlComp (unsigned long lhs, unsigned long rhs) const
    {
        const Spotting& r = instancesById->at(rhs);
        const Spotting& l = instancesById->at(lhs);
        if (l.pageId == r.pageId)
        {
            if (l.tlx == r.tlx)
                return l.tly < r.tly;
            else
                return l.tlx < r.tlx;
        }
        else
            return l.pageId < r.pageId;
    }
    bool tlToAddComp (unsigned long lhs, unsigned long rhs) const
    {
        const Spotting& r = instancesToAddQbE->at(rhs);
        const Spotting& l = instancesToAddQbE->at(lhs);
        if (l.pageId == r.pageId)
        {
            if (l.tlx == r.tlx)
                return l.tly < r.tly;
            else
                return l.tlx < r.tlx;
        }
        else
            return l.pageId < r.pageId;
    }*/
};

#endif
