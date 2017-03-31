
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
    NetSpotter(const Dataset* corpus, string modelDir, int charWidth);
    ~NetSpotter()
    {
        delete spotter;
        //delete dataset;
    }
    vector<SpottingResult> runQuery(SpottingQuery* query);
    float score(string text, const cv::Mat& image) const;
    float score(string text, int wordIndex) const;
    //double getAverageCharWidth() const { return spotter->getAverageCharWidth(); }
};

#endif
