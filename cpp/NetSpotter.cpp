#include "NetSpotter.h"

NetSpotter::NetSpotter(const Dataset* corpus, string modelPrefix, string ngramWWFile) 
{
    //spotter = new EmbAttSpotter(modelPrefix+"_emb",true);
    int gpu=-1;
    bool adapt=modelPrefix.find("dapt")!=string::npos;
    spotter = new CNNSPPSpotter(modelPrefix+"_featurizer.prototxt", modelPrefix+"_embedder.prototxt", modelPrefix+".caffemodel", adapt, ngramWWFile, gpu, true, 0.25, 4, modelPrefix, GlobalK::knowledge()->IDEAL_COMB);
    
    spotter->setCorpus_dataset(corpus);
}

vector<SpottingResult> NetSpotter::runQuery(SpottingQuery* query)
{
    vector< SubwordSpottingResult > res;
#if CUT_OFF_SPOTTING_RESULTS
    float refinePortion=0.25;
    float refinePortionQbE=0.25;
    if (query->getNgram().length()==1)
    {
        refinePortion=0.75;
        refinePortionQbE=0.75;
    }
    else if (query->getNgram().length()>2)
    {
        refinePortion=0.15;
        refinePortionQbE=0.15;
    }
#else
    float refinePortion=0.98;//get rid of bad outliers
    float refinePortionQbE=0.98;
#endif

#ifdef NO_NAN
    float ap=-1;
    float accumAP=-1;
    int initAccumResSize = GlobalK::knowledge()->accumResFor(query->getNgram())->size();
#endif
    if (query->getImg().cols==0)
    {
#ifdef NO_NAN
           res = spotter->subwordSpot_eval(query->getNgram(),refinePortion, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, NULL, &resLock);
#else
           res = spotter->subwordSpot(query->getNgram(), refinePortion);
#endif
    }
    else
    {
        if (query->getWordId()>=0)
        {
            //this assumes this was spotted from a word, so we give the spotter a reference to it so we don't need to embed it again
#ifdef NO_NAN
           res = spotter->subwordSpot_eval(query->getWordId(), query->getX0(), query->getNgram(), refinePortionQbE, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, &accumAP, &resLock, GlobalK::knowledge()->MIN_SPOTTING_AP);
           
#else
           res = spotter->subwordSpot(query->getNgram(),query->getWordId(), query->getX0(), refinePortionQbE);
#endif
            
        }
        else
        {
            if (query->getImg().channels()==1)
            {
#ifdef NO_NAN
               res = spotter->subwordSpot_eval(query->getImg(), query->getNgram(), refinePortionQbE, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, &accumAP, &resLock, GlobalK::knowledge()->MIN_SPOTTING_AP);
               
#else
               res = spotter->subwordSpot(query->getNgram(),query->getImg(), refinePortion);
#endif
            }
            else
            {
                cv::Mat gray;
                cv::cvtColor(query->getImg(),gray,CV_RGB2GRAY);
#ifdef NO_NAN
                res = spotter->subwordSpot_eval(gray, query->getNgram(), refinePortionQbE, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, &accumAP, &resLock, GlobalK::knowledge()->MIN_SPOTTING_AP);
                
#else
                res = spotter->subwordSpot(query->getNgram(),gray, refinePortion);
#endif
            }
        }
    }
#ifdef NO_NAN
    int accumDif = GlobalK::knowledge()->accumResFor(query->getNgram())->size() - initAccumResSize;
    GlobalK::knowledge()->storeSpottingAccum(query->getNgram(),accumAP,accumDif);
    if (query->getType()==SPOTTING_TYPE_EXEMPLAR)
        GlobalK::knowledge()->storeSpottingExemplar(query->getNgram(),ap);
    else if (query->getType()==SPOTTING_TYPE_APPROVED)
        GlobalK::knowledge()->storeSpottingNormal(query->getNgram(),ap);
    else 
        GlobalK::knowledge()->storeSpottingOther(query->getNgram(),ap);
#endif

    vector <SpottingResult> ret;
    //for (int i=0; i<res.size(); i++)
    for (auto r : res)
    {
        //cout<<"s:"<<i<<", "<<res.size()<<endl;
        //cout<<((i<res.size())?"true":"false")<<endl;
        //assert(i<res.size());
        if (r.endX-r.startX>0)
            ret.emplace_back(r.imIdx,
                             r.score,
                             r.startX,
                             r.endX
                            );
        //cout<<"e:"<<i<<endl;
    }
    return ret;
}

float NetSpotter::score(string text, const cv::Mat& image) const
{
    if (image.channels() == 1)
        return spotter->compare(text,image);
    else
    {
        cv::Mat gray;
        cv::cvtColor(image,gray,CV_RGB2GRAY);
        return spotter->compare(text,gray);
    }
}
float NetSpotter::score(string text, int wordIndex) const
{
    return spotter->compare(text,wordIndex);
}

void NetSpotter::addLexicon(const vector<string>& lexicon)
{
    spotter->addLexicon(lexicon);
}
vector< multimap<float,string> > NetSpotter::transcribeCorpus()
{
    return spotter->transcribeCorpus();
}
vector<SpottingLoc> NetSpotter::massSpot(const vector<string>& ngrams, cv::Mat& crossScores)
{
    return spotter->massSpot(ngrams,crossScores);
}
