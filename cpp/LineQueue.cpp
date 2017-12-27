#include "LineQueue.h"

#include <algorithm>


LineQueue::LineQueue(int contextPad, Knowledge::Corpus* corpus) : contextPad(contextPad), corpus(corpus)
{
    int lineNum=0;
    totalWords=0;
    for (Knowledge::Page* page : corpus->getPages())
    {
        for (Knowledge::Line* line : page->lines())
        {
            int tlx=999999;
            int brx=-99999;
            int tly, bry;
            vector<Knowledge::Word*> words = line->wordsAndBounds(&tly, &bry);
            map<int,string> gts;
            for (Knowledge::Word* word : words)
            {
                int word_tlx, word_tly, word_brx, word_bry;
                bool isDone;
                string word_gt;
                word->getBoundsAndDoneAndGT(&word_tlx, &word_tly, &word_brx, &word_bry, &isDone, &word_gt);
                assert(gts.find(word_tlx)==gts.end());
                gts[word_tlx]=word_gt;
                if (word_tlx<tlx)
                    tlx=word_tlx;
                if (word_brx>brx)
                    brx=word_brx;
            }
            auto iter = gts.begin();
            string gt=iter->second;
            iter++;
            for (; iter!=gts.end(); iter++)
                gt+=" "+iter->second;
            origins.push_back(new PsuedoWordBackPointer(lineNum++,gt,gts.size()));
            totalWords += gts.size();
            batches.emplace_back(origins.back(), vector<string>(), page->getImg(), &spottings, tlx, tly, brx, bry, gt);
            oPointer[batches.back().getId()]=origins.back();
        }
    }
#ifndef NO_NAN
    random_shuffle(batches.begin(),batches.end());
#endif
    on=0;
}
LineQueue::~LineQueue()
{
    for (auto pt : origins)
        delete pt;
}

LineQueue::LineQueue(ifstream& in, int contextPad, Knowledge::Corpus* corpus) : LineQueue(contextPad,corpus)
{
    string line;
    getline(in,line);
    on=stoi(line);
}

void LineQueue::save(ofstream& out)
{
    out<<on<<endl;
}

BatchWraper* LineQueue::getBatch(int width)
{
#ifdef NO_NAN
    mutLock.lock();
    if (on>=batches.size())
    {
        cout<<"Manual finished."<<endl;
        cout<<"Total time in minutes: "<<GlobalK::knowledge()->time_spent/(1000.0*60)<<endl;
        cout<<"comp./day: "<<1.0/(GlobalK::knowledge()->time_spent/(1000.0*60*60*24))<<endl;
        return new BatchWraperRanOut();
    }
#endif
    TranscribeBatch& ret = batches.at((on++)%batches.size());
#ifdef NO_NAN
    mutLock.unlock();
#endif
    ret.setWidth(width, contextPad);
    return new BatchWraperTranscription(&ret);
}

#ifdef NO_NAN
void LineQueue::feedback(string trans, unsigned long id)
{
    PsuedoWordBackPointer* origin = oPointer.at(id);
    string gt = origin->gt;
    //score
    float acc;

    mutLock.lock();
    sumAcc += acc*origin->numWords;
    wordsDone += origin->numWords;
    mutLock.unlock();
}

void LineQueue::getStats(float* accTrans, float* pWordsTrans)
{
    *accTrans = sumAcc/wordsDone;
    *pWordsTrans = wordsDone/(0.0+totalWords);
}
#endif

