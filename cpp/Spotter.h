#ifndef SPOTTER_H
#define SPOTTER_H

#include "SpottingQuery.h"
#include "SubwordSpottingResult.h"
#include "opencv2/core/core.hpp"

struct SpottingResult {
    int imIdx;
    float score;
    int startX;
    int endX;
    SpottingResult(int imIdx, float score, int startX, int endX) : 
        imIdx(imIdx), score(score), startX(startX), endX(endX)
    {
    }
    SpottingResult() : 
        imIdx(-1), score(0), startX(-1), endX(-1)
    {
    }
};

class Spotter
{
    public:
    virtual vector<SpottingResult> runQuery(SpottingQuery* query) =0;
    virtual float score(string text, const cv::Mat& image) const =0;
    virtual float score(string text, int wordIndex) const =0;
    //virtual double getAverageCharWidth() const =0;
    virtual ~Spotter() {}
    virtual void addLexicon(const vector<string>& lexicon) =0;
    virtual vector< multimap<float,string> > transcribeCorpus() =0;
    virtual void setNgrams(const vector<string>& ngrams) =0;
    virtual cv::Mat cpv(int wordIndex) =0;
    virtual vector<SpottingLoc> massSpot(const vector<string>& ngrams, cv::Mat& crossScores) =0;
};
#endif
