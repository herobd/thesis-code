#ifndef CTCWRAPPER_H
#define CTCWRAPPER_H

#ifdef CTC

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
//#include "ctc.h"
#include "ddynamicprogramming.h"
#include <iostream>

using namespace std;
using namespace cv;

class CTCWrapper
{
private:
    int alphaSize, maxInputLen, maxLabelLen;
    //char* workspace;
    double* probs;
    //float* grads;
    //int* labels;
    double* labelData;
    //ctcOptions options;

public:
    CTCWrapper(int maxInputLen, int maxLabelLen);
    ~CTCWrapper()
    {
        //delete[] workspace;//free(workspace);
        delete[] probs;
        //delete[] grads;
        //delete[] labels;
        delete[] labelData;
    }

    float loss(Mat cpv, string label);
};

#endif
#endif
