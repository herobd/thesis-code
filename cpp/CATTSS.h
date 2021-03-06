#ifndef CATTSS_H
#define CATTSS_H

#include "MasterQueue.h"
#include "Knowledge.h"
#include "SpottingQueue.h"
//#include "ClusterBatcher.h"
#include "spotting.h"
#include "Lexicon.h"
#include "BatchWraper.h"
#include "Web.h"
#include "opencv2/core/core.hpp"
#include <exception>
#include <pthread.h>

#include <unistd.h>
//#include "ctpl_stl.h"
#ifdef NO_NAN
#include "tester.h"
#endif
#include "LineQueue.h"

using namespace std;

#define CHECK_SAVE_TIME 6

#define NEW_EXEMPLAR_TASK 1
#define TRANSCRIPTION_TASK 2
#define SPOTTINGS_TASK 3
struct UpdateTask
{
    int type;
    string id;
    vector<int> labels;
    int resent_manual_bool;
    vector<string> strings;
    UpdateTask(string batchId,  vector<int>& labels, int resent) : type(NEW_EXEMPLAR_TASK), id(batchId), labels(labels), resent_manual_bool(resent) {} 
    UpdateTask(string id, string transcription, bool manual) : type(TRANSCRIPTION_TASK), id(id),  resent_manual_bool(manual) {strings.push_back(transcription);}
    UpdateTask(string resultsId, vector<string>& ids, vector<int>& labels, int resent) : type(SPOTTINGS_TASK), id(resultsId), labels(labels), resent_manual_bool(resent), strings(ids) {} 

    UpdateTask(ifstream& in)
    {
        string line;
        getline(in,line);
        type = stoi(line);
        getline(in,id);
        getline(in,line);
        int size = stoi(line);
        labels.resize(size);
        for (int i=0; i<size; i++)
        {
            getline(in,line);
            labels.at(i)=stoi(line);
        }
        getline(in,line);
        resent_manual_bool = stoi(line);
        getline(in,line);
        size = stoi(line);
        strings.resize(size);
        for (int i=0; i<size; i++)
        {
            getline(in,strings.at(i));
        }
    }
    void save(ofstream& out)
    {
        out<<type<<"\n";
        out<<id<<"\n";
        out<<labels.size()<<"\n";
        for (int i : labels)
        {
            out<<i<<"\n";
        }
        out<<resent_manual_bool<<"\n";
        out<<strings.size()<<"\n";
        for (string s : strings)
        {
            out<<s<<"\n";
        }
    }
};

class CATTSS
{
    private:
    MasterQueue* masterQueue;
    SpottingQueue* spottingQueue;
    Knowledge::Corpus* corpus;
    Web* web;
    LineQueue* lineQueue;
    thread* incompleteChecker;
    thread* showChecker;

    //ctpl::thread_pool* pool;
    //Thread pool stuff
    vector<thread*> taskThreads;
    atomic_char cont;
    deque<UpdateTask*> taskQueue;
    sem_t semLock;
    mutex taskQueueLock;

    string savePrefix;//file prefix for its regular saving.

    UpdateTask* dequeue();
    void enqueue(UpdateTask* task);
    void run(int numThreads);
    void stop();

    set<int> nsOfInterest;
    bool noManual;

    public:
    CATTSS( string lexiconFile,
            string pageImageDir, 
            string segmentationFile, 
            string spottingModelPrefix,
            string savePrefix,
            //set<int> nsOfInterest, //ngrams we will be spotting
            string ngramWWFile, //This is a file with three lines for each ngram: the ngram, the estimated width, the clustered width (for preprocessing embeddings) //int avgCharWidth,
            int numSpottingThreads,
            int numTaskThreads,
            int showHeight,     //Height of showProgress image
            int showWidth,      //Width of showProgress image
            int showMilli,      //How frequently to save showProgress
            int contextPad,     //how many pixels to arbitrarly pad to the bottom of images sent to users (for NAMES)
            bool noManual=false);//For testing, prevent manual batches from being sent
    ~CATTSS()
    {
        delete incompleteChecker;
        delete showChecker;
        delete masterQueue;
        delete corpus;
        delete spottingQueue;
        for (thread* t : taskThreads)
            delete t;
        for (UpdateTask* t : taskQueue)
            delete t;
        if (GlobalK::knowledge()->WEB_TRANS && web!=NULL)
            delete web;
        if (lineQueue!=NULL)
            delete lineQueue;
    }
   //this is aux for extracting data from save file 
    CATTSS(     string save,
                string outCompleted="",
                string outIncomplete="");
    void save();

    BatchWraper* getBatch(int num, int width, int color, string prevNgram);
    BatchWraper* getSpottingsAsBatch(int width, int color, string prevNgram, unsigned long batcherId, vector<unsigned long> spottingIds, string ngram);
    void updateSpottings(string resultsId, vector<string> ids, vector<int> labels, int resent);
    void updateTranscription(string id, string transcription, bool manual);
    void updateNewExemplars(string resultsId,  vector<int> labels, int resent);
    void misc(string task);
    const Knowledge::Corpus* getCorpus() const {return corpus;}

    const cv::Mat* imgForPageId(int id) const {return corpus->imgForPageId(id);}
    void threadLoop();
    bool getCont() {return cont.load();}
#ifdef NO_NAN
    friend class Tester;
#endif

    //For data collection, when I deleted all my trans... :(
    void resetAllWords_();
    void getStats(float* accTrans, float* pWordsTrans, float* pWords80_100, float* pWords60_80, float* pWords40_60, float* pWords20_40, float* pWords0_20, float* pWords0, float* pWordsBad, string* misTrans,
                          float* accTrans_IV, float* pWordsTrans_IV, float* pWords80_100_IV, float* pWords60_80_IV, float* pWords40_60_IV, float* pWords20_40_IV, float* pWords0_20_IV, float* pWords0_IV, string* misTrans_IV)
    {
        corpus->getStats(accTrans,pWordsTrans,pWords80_100,pWords60_80,pWords40_60,pWords20_40,pWords0_20,pWords0,pWordsBad,misTrans,accTrans_IV,pWordsTrans_IV,pWords80_100_IV,pWords60_80_IV,pWords40_60_IV,pWords20_40_IV,pWords0_20_IV,pWords0_IV,misTrans_IV);
    }
    void printFinalStats();
    void printBatchStats(string ngram, string file);
    int getMaxImageWidth() {return corpus->getMaxImageWidth();}

    void initLines(int contextPad);
    BatchWraper* getLineBatch(int width, int index=-1);
    BatchWraper* getManualBatch(int width);
};
#endif
