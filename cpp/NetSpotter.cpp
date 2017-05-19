#include "NetSpotter.h"

NetSpotter::NetSpotter(const Dataset* corpus, string modelPrefix, int charWidth, set<int> ngrams) 
{
    //spotter = new EmbAttSpotter(modelPrefix+"_emb",true);
    spotter = new CNNSPPSpotter(modelPrefix+"_featurizer.prototxt", modelPrefix+"_embedder.prototxt", modelPrefix+".caffemodel", ngrams, true, 0.25, charWidth, 4, modelPrefix+"_cnnsppspotter");
    
    spotter->setCorpus_dataset(corpus,false);
}

vector<SpottingResult> NetSpotter::runQuery(SpottingQuery* query)
{
    vector< SubwordSpottingResult > res;
    float refinePortion=0.20;
#ifdef NO_NAN
    float ap, accumAP;
    int initAccumResSize = GlobalK::knowledge()->accumResFor(query->getNgram())->size();
#endif
    if (query->getImg().cols==0)
    {
        refinePortion=0.25;
//#ifdef NO_NAN
//           res = spotter->subwordSpot_eval(query->getNgram(),refinePortion, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, &accumAP, &resLock);
//#else
           res = spotter->subwordSpot(query->getNgram(), refinePortion);
//#endif
    }
    else
    {
        if (query->getWordId()>=0)
        {
            //this assumes this was spotted from a word, so we give the spotter a reference to it so we don't need to embed it again
#ifdef NO_NAN
           res = spotter->subwordSpot_eval(query->getWordId(), query->getX0(), query->getNgram(), refinePortion, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, &accumAP, &resLock);
           
#else
           res = spotter->subwordSpot(query->getNgram().length(),query->getWordId(), query->getX0(), refinePortion);
#endif
            
        }
        else
        {
            if (query->getImg().channels()==1)
            {
#ifdef NO_NAN
               res = spotter->subwordSpot_eval(query->getImg(), query->getNgram(), refinePortion, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, &accumAP, &resLock);
               
#else
               res = spotter->subwordSpot(query->getNgram().length(),query->getImg(), refinePortion);
#endif
            }
            else
            {
                cv::Mat gray;
                cv::cvtColor(query->getImg(),gray,CV_RGB2GRAY);
#ifdef NO_NAN
                res = spotter->subwordSpot_eval(gray, query->getNgram(), refinePortion, GlobalK::knowledge()->accumResFor(query->getNgram()), GlobalK::knowledge()->getCorpusXLetterStartBounds(), GlobalK::knowledge()->getCorpusXLetterEndBounds(), &ap, &accumAP, &resLock);
                
#else
                res = spotter->subwordSpot(query->getNgram().length(),gray, refinePortion);
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

    vector <SpottingResult> ret(res.size());
    for (int i=0; i<res.size(); i++)
        ret[i] = SpottingResult(res[i].imIdx,
                                res[i].score,
                                res[i].startX,
                                res[i].endX
                               );
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
