#ifndef LINEMANTRANS
#define LINEMANTRANS

#include "LineQueue.h"
#include "Knowledge.h"
#include "BatchWraper.h"
#include <exception>
#include <pthread.h>
#include <thread>
#include <iostream>
#include <fstream>

#include <unistd.h>

#define CHECK_SAVE_TIME_LINE 4
using namespace std;

class LineManTrans
{
    private:
    LineQueue* lineQueue;
    Knowledge::Corpus* corpus;
    thread* saveThread;

    string savePrefix;

    atomic_char cont_A;

    public:

    LineManTrans(
                string pageImageDir, 
                string segmentationFile, 
                string savePrefix,
                int contextPad
                );
    ~LineManTrans()
    {
        delete lineQueue;
        delete corpus;
        delete saveThread;
    }
    BatchWraper* getBatch(int width);


    void save();

    void stop()
    {
        cont_A=false;
    }
    bool cont()
    {
        return cont_A;
    }
};


#endif
