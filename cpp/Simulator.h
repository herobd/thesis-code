#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <vector>
#include "BatchWraper.h"
#include "batches.h"
#include <thread>

/*
   Spotting timing ought to take into consideration:
   -Ngram
   -if prev ngram same
   -Whether word contains ngram (gt shows this)

   Transcription timing ought to consider
   -Length if word
   -position of correct
   -ngram error
   -none error

   Manual timing ought to consider
   -length of word


Spotting:
Does it overlap with an ngram?
No -> reject with X% prob
Yes ->
    If spot inside ngram:
        Overlap >65% of ngram accept with Y% prob, else reject
    else If spot consumes ngram:
        Spot <145% of ngram accept with Z% prob, else reject
    else If spot sided on ngram
        >55% overlap and spotting outside ngram is <90% of the overlap: accept with W% prob, else reject
*/
#define OVERLAP_INSIDE_THRESH 0.65
#define OVERLAP_CONSUME_THRESH 1.8
#define OVERLAP_SIDE_THRESH 0.55
#define SIDE_NOT_INCLUDED_THRESH 0.80

#define OVERLAP_INSIDE_THRESH_STRICT 0.85
#define OVERLAP_CONSUME_THRESH_STRICT 1.35
#define OVERLAP_SIDE_THRESH_STRICT 0.85
#define SIDE_NOT_INCLUDED_THRESH_STRICT 0.50

#define RAND_PROB (static_cast <float> (rand()) / static_cast <float> (RAND_MAX))

class Simulator
{
    public:
    Simulator(string dataname, string mode, string segCSV);
    vector<int> spottings(string ngram, vector<Location> locs, vector<string> gt, string prevNgram);
    vector<int> newExemplars(vector<string> ngrams, vector<Location> locs, string prevNgram);
    string transcription(int wordIndex, vector<SpottingPoint> spottings, vector<string> poss, string gt, bool lastWasTrans);
    string manual(int wordIndex, vector<string> poss, string gt, bool lastWasMan);

    private:
    int skipAndError(vector<int>& labels);
    //network_t *spotNet;
    vector<string> corpusWord; //The strings of the corpus
    vector<int> corpusPage;
    vector< vector<int> > corpusXLetterStartBounds; //The starting x boundary of a letter. Page relative
    vector< vector<int> > corpusXLetterEndBounds; //The ending x boudary of a letter
    vector< vector<int> > corpusXLetterStartBoundsRel; //The starting x boundary of a letter. Word relative
    vector< vector<int> > corpusXLetterEndBoundsRel; //The ending x boudary of a letter
    vector< pair<int, int> > corpusYBounds; //the verticle boundaries of each word

    //spotting probs
    //float isInSkipProb, falseNegativeProb, falsePositiveProb, notInSkipProb, notInFalsePositiveProb;

    float spottingAverageMilli;
    float spottingAverageMilli_prev;
    float spottingErrorProb_m; //https://plot.ly/create/
    float spottingErrorProb_b;
    float spottingSkipProb_m;
    float spottingSkipProb_b;


    float transMilli_b;
    float transMilli_m;
    float transMilli_0;
    float transMilli_notAvail;
    float transErrorProbAvailError;//specific type of error where user said it wasn't present when it was
    float transErrorProbAvailWord;//user picked wrong word
    float transErrorProbNotAvail;

    float manMilli_b;
    float manMilli_m;
    float manErrorProb;

    float spottingTime_b;
    float spottingTime_m;
    bool spottingsUseTrues;
    bool cluster;
  
    int getSpottingLabel(string ngram, Location loc, bool strict=false); 
};
#endif
