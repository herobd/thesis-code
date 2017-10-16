
#ifndef NETSPOTTER_H
#define NETSPOTTER_H

#include "opencv2/core/core.hpp"
#include "Spotter.h"
#include "cnnspp_spotter.h"
#include "dataset.h"


class NetSpotter : public Spotter
{
    private:
    CNNSPPSpotter* spotter;
    mutex resLock;
    //AlmazanDataset* dataset;

    public:
    NetSpotter(const Dataset* corpus, string modelDir, string ngramWWFile);
    ~NetSpotter()
    {
        delete spotter;
        //delete dataset;
    }
    vector<SpottingResult> runQuery(SpottingQuery* query);
    float score(string text, const cv::Mat& image) const;
    float score(string text, int wordIndex) const;
    //double getAverageCharWidth() const { return spotter->getAverageCharWidth(); }
    void addLexicon(const vector<string>& lexicon);
    vector< multimap<float,string> > transcribeCorpus();
    void setNgrams(const vector<string>& ngrams) {spotter->cpvPrep(ngrams);}
    Mat cpv(int wordIndex) {return spotter->cpv(wordIndex);}
    vector<SpottingLoc> massSpot(const vector<string>& ngrams, cv::Mat& crossScores);
};

#endif
