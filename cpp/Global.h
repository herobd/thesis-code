#ifndef GLOBAL_HEADER
#define GLOBAL_HEADER

#include <map>
#include <set>
#include <vector>
#include <fstream>
#include <iostream>
#include <algorithm>
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <atomic>
#include <mutex>
#include <time.h>
#include <sys/stat.h>
#include <sstream>
#include <limits>

#ifdef NO_NAN
#include "SubwordSpottingResult.h"
#endif

#if defined(TEST_MODE) || defined(NO_NAN)
#define OVERLAP_INSIDE_THRESH 0.65
#define OVERLAP_CONSUME_THRESH 1.8
#define OVERLAP_SIDE_THRESH 0.55
#define SIDE_NOT_INCLUDED_THRESH 0.80
#endif

#define SHOW_PROGRESS 0 

#define TRANS_DONT_WAIT 0
#define USE_QBE 1
#define NO_EXEMPLARS 1




using namespace std;

#define MAX_FLOAT numeric_limits<float>::max()
#define MIN_FLOAT numeric_limits<float>::min()


#if defined(TEST_MODE) || defined(NO_NAN)
class WordBound
{
public:
    WordBound(int tlx, int tly, int brx, int bry) :
        tlx(tlx), tly(tly), brx(brx), bry(bry) {}
    WordBound(string text, int tlx, int tly, int brx, int bry, vector<int> startBounds, vector<int> endBounds) :
        text(text), tlx(tlx), tly(tly), brx(brx), bry(bry), startBounds(startBounds), endBounds(endBounds) {}
    int tlx, tly, brx, bry;
    string text;
    vector<int> startBounds, endBounds;
};
class tlyComp
{
  bool reverse;
public:
  tlyComp(const bool& revparam=false)
    {reverse=revparam;}
  bool operator() (const WordBound& lhs, const WordBound& rhs) const
  {
      bool ret;
      if (lhs.tly == rhs.tly)
          ret=lhs.tlx < rhs.tlx;
      else
          ret = lhs.tly < rhs.tly;
    if (reverse) return !ret;
    else return ret;
  }
};
#endif

#define MIN_N 1
#define MAX_N 1
#define MAX_NGRAM_RANK 300
class GlobalK
{
    private:
        GlobalK();
        ~GlobalK();
        static GlobalK* _self;

        map<int, vector<string> > ngramRanks;
        //int contextPad;
#ifdef NO_NAN
        atomic_int transSent;
        atomic_int transBadBatch;
        atomic_int transBadNgram;
        atomic_int spotSent;
        atomic_int spotAccept;
        atomic_int spotReject;
        atomic_int spotAutoAccept;
        atomic_int spotAutoReject;
        atomic_int newExemplarSpotted;
        atomic_int badPrunes;
        ofstream trackFile;

        stringstream track;

        mutex spotMut;
        mutex accumResMut;
        string spottingFile;
        map<string, vector<float> > spottingAccums;
        map<string, vector<int> > spottingAccumDifs;
        map<string, vector<float> > spottingAccumAvgs;
        map<string, vector<float> > spottingAccumStds;
        map<string, vector<float> > spottingExemplars;
        map<string, vector<float> > spottingNormals;
        map<string, vector<float> > spottingOthers;
        
        map<string, vector<SubwordSpottingResult>*> accumRes;

        mutex xLock;
        const vector< vector<int> >* corpusXLetterStartBounds;
        const vector< vector<int> >* corpusXLetterEndBounds;
        const vector<string>* corpusSegWords;
#endif

    public:
        static GlobalK* knowledge();

        int getNgramRank(string ngram);
        //int getContextPad() {return contextPad;}
        //void setContextPad(int pad) {contextPad=pad;}

        static double otsuThresh(vector<int> histogram);
        static void saveImage(const cv::Mat& im, ofstream& out);
        static void loadImage(cv::Mat& im, ifstream& in);
        static string currentDateTime();

        static int numCombThresh(string ngram)
        {
            int l = ngram.length();
            if (l==1)
                return 30;
            else if (l==2)
                return 25;
            else
                return 20;
        }

#ifdef NO_NAN
        void setSimSave(string file);
        void sentSpottings();
        void sentTrans();
        void badTransBatch();
        void badTransNgram();
        void accepted();
        void rejected();
        void autoAccepted();
        void autoRejected();
        void badPrune(){badPrunes++;}
        void newExemplar();
        void saveTrack(float accTrans, float pWordsTrans, float pWords80_100, float pWords60_80, float pWords40_60, float pWords20_40, float pWords0_20, float pWords0, string misTrans,
                       float accTrans_IV, float pWordsTrans_IV, float pWords80_100_IV, float pWords60_80_IV, float pWords40_60_IV, float pWords20_40_IV, float pWords0_20_IV, float pWords0_IV, string misTrans_IV);
        void writeTrack();       

        void setCorpusXLetterBounds(const vector< vector<int> >* start, const vector< vector<int> >* end, const vector<string>* words)
        {
            corpusXLetterStartBounds=start;
            corpusXLetterEndBounds=end;
            corpusSegWords=words;
            xLock.unlock();
        }
        string getSegWord(int i)
        {
            return corpusSegWords->at(i);
        }
        const vector< vector<int> >* getCorpusXLetterStartBounds() {xLock.lock(); xLock.unlock(); return corpusXLetterStartBounds;}
        const vector< vector<int> >* getCorpusXLetterEndBounds() {xLock.lock(); xLock.unlock(); return corpusXLetterEndBounds;}
        void storeSpottingAccum(string ngram, float ap, int dif);
        void storeSpottingExemplar(string ngram, float ap);
        void storeSpottingNormal(string ngram, float ap);
        void storeSpottingOther(string ngram, float ap);
        vector<SubwordSpottingResult>* accumResFor(string ngram);
#endif
#if defined(TEST_MODE) || defined(NO_NAN)
        bool ngramAt(string ngram, int pageId, int tlx, int tly, int brx, int bry);
        map<int, multiset<WordBound,tlyComp> > wordBounds;//pageId -> set of words
        void addWordBound(string word, int pageId, int tlx, int tly, int brx, int bry, vector<int> startBounds, vector<int> endBounds);
#endif
        static int sgn(float val) {
            return (0 < val) - (val < 0);
        }
        bool SR_THRESH_NONE;
        bool SR_TAKE_FROM_TOP; 
        bool SR_OTSU_FIXED; 
        bool SR_FANCY_ONE;
        bool SR_FANCY_TWO;
        bool SR_GAUSSIAN_DRAW;

        float MIN_SPOTTING_AP;
        bool IDEAL_COMB;

        bool PHOC_TRANS;
};

#endif
