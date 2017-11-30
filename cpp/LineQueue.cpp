#include "LineQueue.h"


LineQueue::LineQueue(int contextPad, Knowledge::Corpus* corpus) : contextPad(contextPad), corpus(corpus)
{
    int lineNum=0;
    for (Knowledge::Page* page : corpus->getPages())
    {
        for (Knowledge::Line* line : page->lines())
        {
            origins.push_back(new PsuedoWordBackPointer(lineNum++));
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
            batches.emplace_back(origins.back(), vector<string>(), page->getImg(), &spottings, tlx, tly, brx, bry, gt);
        }
    }
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
    TranscribeBatch& ret = batches.at((on++)%batches.size());
    ret.setWidth(width, contextPad);
    return new BatchWraperTranscription(&ret);
}


