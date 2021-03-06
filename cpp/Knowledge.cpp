#include "Knowledge.h"
#include <time.h>

int Knowledge::Page::_id=0;
atomic_uint Knowledge::Word::_id(0);

Knowledge::Corpus::Corpus(int contextPad, string ngramWWFile, set<string>* ngrams) : ngramWWFile(ngramWWFile)  //int averageCharWidth) 
{
    pthread_rwlock_init(&pagesLock,NULL);
    pthread_rwlock_init(&spottingsMapLock,NULL);
    //this->averageCharWidth=averageCharWidth;
    if (ngramWWFile.compare("none")!=0)
    {
        ifstream widths(ngramWWFile);
        assert(widths.good());
        string line;
        float sum=0;
        int count=0;
        maxImageWidth=0;
        while (getline(widths,line))
        {
            string ngram = GlobalK::lowercaseAndStrip(line);
            if (ngrams!=NULL)
                ngrams->insert(ngram);
            getline(widths,line);
            int w=stoi(line);
            sum+=w;
            maxImageWidth = max(maxImageWidth,w);
            count+=ngram.length();
            getline(widths,line);
        }
        maxImageWidth*=1.2;
        widths.close();
        averageCharWidth=sum/count;
#ifdef TEST_MODE
        cout<<"Average character width: "<<averageCharWidth<<endl;
#endif
    }
    else
    {
        averageCharWidth=-1;
        maxImageWidth=-1;
    }
    countCharWidth=0;
    threshScoring= 1.0;
    manQueue.setContextPad(contextPad);
    minWordImageLen=99999;
    maxWordImageLen=0;
    name="UNINITIALIZED";
}
void Knowledge::Corpus::loadSpotter(string modelPrefix)
{
    //spotter = new AlmazanSpotter(this,modelPrefix);
    spotter = new NetSpotter(this,modelPrefix,ngramWWFile);

    //This is bad, it shouldn't be coming from here, but it prevents code dup.
    //averageCharWidth = spotter->getAverageCharWidth();

}
vector<TranscribeBatch*> Knowledge::Corpus::addSpotting(Spotting s,vector<Spotting*>* newExemplars)
{
    vector<TranscribeBatch*> ret;
#if DONT_ASSUME_PAGE_SEG
    pthread_rwlock_rdlock(&pagesLock);
    Page* page = pages[s.pageId];
    pthread_rwlock_unlock(&pagesLock);
    if (page==NULL)
    {
        /*page = new Page();
        pthread_rwlock_wrlock(&pagesLock);
        pages[s.pageId] = page;
        pthread_rwlock_unlock(&pagesLock);*/
        assert(false && "ERROR, page not present");
    }

    
    addSpottingToPage(s,page,ret,newExemplars);
#else

    assert(s.wordId >= 0);
    Word* word = getWord(s.wordId);
    assert(word!=NULL);
    TranscribeBatch* newBatch = word->addSpotting(s,newExemplars);
    pthread_rwlock_wrlock(&spottingsMapLock);
    spottingsToWords[s.id].push_back(word);
    pthread_rwlock_unlock(&spottingsMapLock);
    if (newBatch != NULL)
    {
        ret.push_back(newBatch);
    }
#endif
    
    return ret;
}

vector<TranscribeBatch*> Knowledge::Corpus::phocTrans(float keep)
{
    spotter->addLexicon(Lexicon::instance()->get());
    vector< multimap<float,string> > trans = spotter->transcribeCorpus();
    multimap<float, pair<int, multimap<float,string>*> > transByAvgScore;
    for (int i=0; i<size(); i++)
    {
        float sum=0;
        auto iter=trans.at(i).begin();
        for (int j=0; j<PHOC_TRANS_TOP; j++, iter++)
        {
            sum += iter->first;
        }
        transByAvgScore.emplace(sum/PHOC_TRANS_TOP, make_pair(i,&(trans.at(i))));
    }
    vector<TranscribeBatch*> ret(size()*keep);
    auto transIter = transByAvgScore.begin();
    for (int j=0; j<(int)(size()*keep); j++, transIter++)
    {
        multimap<float,string> words;
        int i = (transIter->second).first;
        auto iter=(transIter->second).second->begin();
        for (int jj=0; jj<PHOC_TRANS_TOP; jj++, iter++)
        {
            words.emplace(iter->first,iter->second);
        }
        int tlx,tly,brx,bry;
        string gt;
        bool toss;
        getWord(i)->getBoundsAndDoneAndGT(&tlx,&tly,&brx,&bry,&toss,&gt);
        TranscribeBatch* newBatch = new TranscribeBatch(getWord(i),words,getWord(i)->getPage(),NULL,tlx,tly,brx,bry,gt);
        ret.at(j) = newBatch;
    }
#ifdef TIMING_TEST
    cout<<"shuffle timing test"<<endl;
    random_shuffle(ret.begin(),ret.end());
#endif
    return ret;
}

#ifdef CTC
Mat drawCPV(Mat cpv, Mat image, float netScale)
{
    double minV, maxV;
    minMaxLoc(cpv,&minV,&maxV);
    //cout<<"min: "<<minV<<"  max: "<<maxV<<endl;

    float meanV = mean(cpv)[0];
    //float newMaxV = 0.9*minV+0.1*meanV;

    int letterSize=40;

    Mat draw = Mat::zeros(image.rows+26*letterSize,letterSize+std::max((int)image.cols,(int)(cpv.cols/netScale)),CV_8UC3);
    image.copyTo(draw(Rect(std::max(0.0f,letterSize-(image.cols-(cpv.cols/netScale))/2),0,image.cols,image.rows)));
    for (int i=0; i<cpv.rows; i++)
    {
        string letter=" ";
        letter[0]=i+'a';

        cv::putText(draw,letter,cv::Point(1,image.rows+i*letterSize+17),cv::FONT_HERSHEY_COMPLEX_SMALL,0.75,cv::Scalar(250,250,255));
        for (int c=0; c<cpv.cols; c++)
        {
            float v = cpv.at<float>(i,c);
            float height = (letterSize-1)*(v-minV)/(maxV-minV);
            //float height = (letterSize-1)*(1-(std::min(newMaxV,v)-minV)/(newMaxV-minV));
            //float color = 510-510*(v+10)/11;
            float color = (510)*(v-minV)/(maxV-minV);
            color = std::min(510.0f,std::max(0.0f,color));
            Vec3b colorP(0,(color<255?color:255),(color<255?255:510-color));
            if (v==(float)maxV)
                colorP=Vec3b(255,0,0);
            for (int h=0; h<height; h++)
                for (int cc=c/netScale; cc<(c+1)/netScale; cc++)
                    draw.at<Vec3b>(image.rows+letterSize*i+(letterSize-1)-h,letterSize+cc)=colorP;
        }
    }
    return draw;
    //imwrite(outFileName+"/"+to_string(wordIndex)+"_"+word+".png",draw);
}


vector<TranscribeBatch*> Knowledge::Corpus::cpvTransCTC(float keep, const vector<string>& ngrams)
{
#ifdef TEST_MODE
    ofstream dout(DEBUG_DIR+string("/topTrans.txt"));
#endif
    spotter->setNgrams(ngrams);
    //join
    int maxFeatureLen=1+maxWordImageLen/4;
    int maxLexLen=0;
    vector<string> lex = Lexicon::instance()->get();
    for (const string& word : lex)
        if (word.length() > maxLexLen)
            maxLexLen=word.length();

    CTCWrapper ctcWrapper(maxFeatureLen, maxLexLen);
    float top5Acc=0;
    multimap<float, pair<int,multimap<float,string> > > byAvgScore;
    for (int i=0; i<size(); i++)
    {
        int tlx,tly,brx,bry;
        string gt;
        bool toss;
        getWord(i)->getBoundsAndDoneAndGT(&tlx,&tly,&brx,&bry,&toss,&gt);
        int imLen = brx-tlx +1;
        int minWordLen = round(0.6*imLen/(0.0+averageCharWidth));
        int maxWordLen = round(1.4*imLen/(0.0+averageCharWidth));
        Mat cpv = spotter->cpv(i);
        /*
        int pad = 0.25*averageCharWidth;
        Mat cpv = Mat::zeros(26,gt.length()*pad,CV_32F) + 0.0001;
        for (int c=0; c<gt.length(); c++)
        {
            for (int ii=0; ii<pad; ii++)
                cpv.at<float>(gt[c]-'a',pad*c+ii) = 1-26*0.0001;
        }
        */
#ifdef TEST_MODE
        Mat draw = drawCPV(cpv,getWord(i)->getImg(),0.25);
        imwrite(DEBUG_DIR+string("/cpv")+to_string(i)+".png",draw);
        imshow("cpv",draw);
        cout<<i<<": "<<gt<<endl;
#endif
        multimap<float,string> topMatches;// = Lexicon::instance()->ctc(cpv,THRESH_SCORING_COUNT);
        //multimap<float,string> ret;
        for (string word : lex)
        {
            if (word.length()<minWordLen || word.length()>maxWordLen)
                continue;
            float loss = ctcWrapper.loss(cpv,word);
            topMatches.emplace(loss,word);
        }

#ifdef TEST_MODE
        dout<<i<<": ";
#endif
        auto iter = topMatches.begin();
        for (int j=0; j<THRESH_SCORING_COUNT; j++)
        {
            if (iter->second.compare(gt)==0)
            {
                top5Acc+=1;
#ifdef TEST_MODE
                dout<<"["<<iter->second<<"]:"<<iter->first<<",";
            }
            else
            {
                dout<<iter->second<<":"<<iter->first<<",";
#endif
            }
            iter++;
        }
#ifdef TEST_MODE
        dout<<endl;
        if (i>50)
        {
            cout<<"test mode stop early"<<endl;
            break;
        }
#endif
        topMatches.erase(iter,topMatches.end());
        //return ret;
        
        float weight=1.0;
        float score=0;
        for (auto p : topMatches)
        {
            score+=weight*p.first;
            weight*=0.5;
        }
        //score /= topMatches.size();
        byAvgScore.emplace(score, make_pair(i,topMatches));

        ///debug
        /*
        cout<<"GT: "<<gt<<endl;
        for (auto p : topMatches)
        {
            cout<<p.second<<" : "<<p.first<<endl;
        }
        waitKey();
        //*/
        ///////
    }
    top5Acc /= size();
    cout<<"Top "<<THRESH_SCORING_COUNT<<" acc for CTC trans: "<<top5Acc<<endl;
#ifdef TEST_MODE
    dout<<"Top "<<THRESH_SCORING_COUNT<<" acc for CTC trans: "<<top5Acc<<endl;
    dout.close();
#endif
    vector<TranscribeBatch*> ret(size()*keep);
    auto transIter = byAvgScore.begin();
    for (int i=0; i<(int)(size()*keep); i++, transIter++)
    {
        int wordIdx = transIter->second.first;
        multimap<float,string>& words = transIter->second.second;
        int tlx,tly,brx,bry;
        string gt;
        bool toss;
        getWord(wordIdx)->getBoundsAndDoneAndGT(&tlx,&tly,&brx,&bry,&toss,&gt);
        TranscribeBatch* newBatch = new TranscribeBatch(getWord(wordIdx),words,getWord(wordIdx)->getPage(),NULL,tlx,tly,brx,bry,gt);
        ret.at(i) = newBatch;
    }
    return ret;
}
#endif
/*
vector<TranscribeBatch*> Knowledge::Corpus::phocNgramTrans(float keep)
{
    spotter->addLexicon(Lexicon::instance()->get());
    vector< multimap<float,string> > trans = spotter->transcribeCorpusNgram();
    multimap<float, pair<int, multimap<float,string>*> > transByAvgScore;
    for (int i=0; i<size(); i++)
    {
        float sum=0;
        auto iter=trans.at(i).begin();
        for (int j=0; j<THRESH_SCORING_COUNT; j++, iter++)
        {
            sum += iter->first;
        }
        transByAvgScore.emplace(sum/THRESH_SCORING_COUNT, make_pair(i,&(trans.at(i))));
    }
    vector<TranscribeBatch*> ret(size()*keep);
    auto transIter = transByAvgScore.begin();
    for (int j=0; j<(int)(size()*keep); j++, transIter++)
    {
        multimap<float,string> words;
        int i = (transIter->second).first;
        auto iter=(transIter->second).second->begin();
        for (int jj=0; jj<THRESH_SCORING_COUNT; jj++, iter++)
        {
            words.emplace(iter->first,iter->second);
        }
        int tlx,tly,brx,bry;
        string gt;
        bool toss;
        getWord(i)->getBoundsAndDoneAndGT(&tlx,&tly,&brx,&bry,&toss,&gt);
        TranscribeBatch* newBatch = new TranscribeBatch(getWord(i),words,getWord(i)->getPage(),NULL,tlx,tly,brx,bry,gt);
        ret.at(j) = newBatch;
    }
    return ret;
}*/
/*
vector<TranscribeBatch*> Knowledge::Corpus::npvTransRegex(const vector<string>& ngrams)
{
    spotter->npvPrep(ngrams);
    vector<TranscribeBatch*> ret(size());
    for (int i=0; i<size(); i++)
    {
        Mat npv = spotter->npv(i);
        int estimatedLen = 
        multimap<float,string> words;
        auto iter=trans.at(i).begin();
        for (int j=0; j<THRESH_SCORING_COUNT; j++,iter++)
        {
            words.emplace(iter->first,iter->second);
        }
        int tlx,tly,brx,bry;
        string gt;
        bool toss;
        getWord(i)->getBoundsAndDoneAndGT(&tlx,&tly,&brx,&bry,&toss,&gt);
        TranscribeBatch* newBatch = new TranscribeBatch(getWord(i),words,getWord(i)->getPage(),NULL,tlx,tly,brx,bry,gt);
        ret.at(i) = newBatch;
    }
}


vector<TranscribeBatch*> Knowledge::Corpus::npvTransDirect(const vector< Mat >* trans)
{
    spotter->npvPrep(ngrams);
    //spawn thread?
    map<string,Mat> lexicon = Lexicon::instance()->getNPV();
    int lexSize = lexicon.begin()->second.cols;
    //join
    multimap<float, pair<i,multimap<float,string> > > byAvgScore;
    for (int i=0; i<size(); i++)
    {
        Mat npv = spotter->npv(i);
        resize(npv,npv,Size(npv.rows,lexSize));
        multimap<float,string> words;
        for (auto p : lexicon)
        {
            float score=0;
            for (int c=0; c<lexSize; c++)
                score += -1*p.second.col(c).dot(npv.col(c));
            words.emplace(score,p.first);
        }
        auto iter=words.begin();
        for (int j=0; j<THRESH_SCORING_COUNT; j++,iter++)
        {
        }
        words.erase(iter,words.end());
        byAvgScore.emplace(score, make_pair(i,words));
    }
    vector<TranscribeBatch*> ret(size()*keep);
    auto transIter = byAvgScore.begin();
    for (int i=0; i<(int)(size()*keep); i++, transIter++)
    {
        int i = transIter->second.first;
        multimap<float,string>& words = transIter->second.second;
        int tlx,tly,brx,bry;
        string gt;
        bool toss;
        getWord(i)->getBoundsAndDoneAndGT(&tlx,&tly,&brx,&bry,&toss,&gt);
        TranscribeBatch* newBatch = new TranscribeBatch(getWord(i),words,getWord(i)->getPage(),NULL,tlx,tly,brx,bry,gt);
        ret.at(i) = newBatch;
    }
}*/


vector<TranscribeBatch*> Knowledge::Corpus::updateSpottings(vector<Spotting>* spottings, vector<pair<unsigned long,string> >* removeSpottings, vector<unsigned long>* toRemoveBatches, vector<Spotting*>* newExemplars, vector<pair<unsigned long, string> >* toRemoveExemplars)
{
    //cout <<"addSpottings"<<endl;
    vector<TranscribeBatch*> ret;
#if DONT_ASSUME_PAGE_SEG
    vector<Page*> thesePages;
    pthread_rwlock_rdlock(&pagesLock);
    //cout <<"addSpottings: got lock"<<endl;
    //bool writing=false;
    for (const Spotting& s : *spottings)
    {
        Page* page;
        if (pages.find(s.pageId)==pages.end())
        {

            assert(false && "ERROR, page not present");
            /*if (!writing)
            {
                pthread_rwlock_unlock(&pagesLock);
                cout <<"addSpottings: release lock"<<endl;
                pthread_rwlock_wrlock(&pagesLock);
                cout <<"addSpottings: got write lock"<<endl;
                writing=true;
            }
            page = new Page();
            pages[s.pageId] = page;*/
        }
        else
        {
            page = pages[s.pageId];
        }
        thesePages.push_back(page);
    }
    pthread_rwlock_unlock(&pagesLock);
    //cout <<"addSpottings: release lock"<<endl;
    
    for (int i=0; i<spottings->size(); i++)
        addSpottingToPage(spottings->at(i),thesePages[i],ret,newExemplars);
#else
    for (const Spotting& s : *spottings)
    {
        assert(s.wordId >= 0);
        Word* word = getWord(s.wordId);
        assert(word!=NULL);
        TranscribeBatch* newBatch = word->addSpotting(s,newExemplars);
        pthread_rwlock_wrlock(&spottingsMapLock);
        spottingsToWords[s.id].push_back(word);
        pthread_rwlock_unlock(&spottingsMapLock);
        if (newBatch != NULL)
        {
            ret.push_back(newBatch);
        }
    }
#endif

    //Removing spottings
    map<unsigned long, vector<Word*> > wordsForIds;
    if (removeSpottings)
    {
        pthread_rwlock_wrlock(&spottingsMapLock);
        for (auto sid : *removeSpottings)
        {
            wordsForIds[sid.first] = spottingsToWords[sid.first];
            spottingsToWords[sid.first].clear(); 
        }
        pthread_rwlock_unlock(&spottingsMapLock);
        for (auto sid : *removeSpottings)
        {
            vector<Word*> wordsForId = wordsForIds[sid.first];
            for (Word* word : wordsForId)
            {
                
                unsigned long retractId=0;  
                TranscribeBatch* newBatch = word->removeSpotting(sid.first,0,false,&retractId,newExemplars,toRemoveExemplars);
                if (retractId!=0 && newBatch==NULL)
                {
                    //retract the batch
                    if (toRemoveBatches)
                        toRemoveBatches->push_back(retractId);
                }
                else if (newBatch != NULL)
                {
                    //modify batch
                    ret.push_back(newBatch);
                }
            }
        }
    }
    //cout <<"END addSpottings"<<endl;
    return ret;
}


void Knowledge::Corpus::addSpottingToPage(Spotting& s, Page* page, vector<TranscribeBatch*>& ret, vector<Spotting*>* newExemplars)
{
    vector<Line*> possibleLines;
    //pthread_rwlock_rdlock(&page->linesLock);
    vector<Line*> lines = page->lines();
    bool oneLine=false;
    for (Line* line : lines)
    {
        int line_ty, line_by;
        vector<Word*> wordsForLine = line->wordsAndBounds(&line_ty,&line_by);
        int overlap = min(s.bry,line_by) - max(s.tly,line_ty);
        float overlapPortion = overlap/(0.0+s.bry-s.tly);
        if (overlapPortion > OVERLAP_LINE_THRESH)
        {
            oneLine=true;
            //pthread_rwlock_rdlock(&line->wordsLock);
            bool oneWord=false;
            for (Word* word : wordsForLine)
            {
                int word_tlx, word_tly, word_brx, word_bry;
                bool isDone;
                word->getBoundsAndDone(&word_tlx,&word_tly,&word_brx,&word_bry,&isDone);
                if (!isDone)
                {
                    int overlap = (max(0,min(s.bry,word_bry) - max(s.tly,word_tly))) * (max(0,min(s.brx,word_brx) - max(s.tlx,word_tlx)));
                    float overlapPortion = overlap/(0.0+(s.bry-s.tly)*(s.brx-s.tlx));
                    //cout <<"Overlap for word "<<word<<": overlapPortion="<<overlapPortion<<" ="<<overlap<<"/"<<(0.0+(s.bry-s.tly)*(s.brx-s.tlx))<<endl;
                    TranscribeBatch* newBatch=NULL;
                    if (overlapPortion > OVERLAP_WORD_THRESH)
                    {
                        oneWord=true;
                        //possibleWords.push_back(word);
                        newBatch = word->addSpotting(s,newExemplars);
                        pthread_rwlock_wrlock(&spottingsMapLock);
                        spottingsToWords[s.id].push_back(word);
                        pthread_rwlock_unlock(&spottingsMapLock);
                    }
                    
                    if (newBatch != NULL)
                    {
                        /*submit/update batch
                        cout<<"Batch Possibilities: ";
                        for (string pos : newBatch->getPossibilities())
                        {
                            cout << pos <<", ";
                        }
                        cout <<endl;
                        cv::imshow("highligh",newBatch->getImage());
                        cv::waitKey();*/
                        ret.push_back(newBatch);
                    }
                }
            }
            
            if (!oneWord)
            {
                //for now, do nothing as we know where all words are
                //TODO
                //make a new word?
                //assert(false);
                //line->addWord(s);
            }
            
            //pthread_rwlock_unlock(&line->wordsLock);
            
            /*if (possibleWords.size()>0)
            {
                for (Word* word : possibleWords)
                {
                    word->addSpottings(s);
                    spottingsToWords[s.id].push_back(word);
                }
            }
            else
            {
                //make a new word
                assert(false);
            }*/
        }
    }
    if (!oneLine)
    {
        //Addline
        //I'm assumming most lines are added prior with a preprocessing step
        page->addLine(s,newExemplars);
        recreateDatasetVectors(true);
    }
}

/*void Knowledge::Corpus::removeSpotting(unsigned long sid)
{

    pthread_rwlock_wrlock(&spottingsMapLock);
    vector<Word*> words = spottingsToWords[sid];
    spottingsToWords[sid].clear(); 
    pthread_rwlock_unlock(&spottingsMapLock);
    for (Word* word : words)
    {
        
        unsigned long retractId=0;
        TranscribeBatch* newBatch = word->removeSpotting(sid,&retractId);
        if (retractId!=0 && newBatch==NULL)
        {
            //TODO retract the batch
        }
        else if (newBatch != NULL)
        {
            //TODO modify batch
        }
    }
}*/

void Knowledge::Word::reAddSpottings(unsigned long batchId, vector<Spotting*>* newExemplars)
{
    auto s = removedSpottings.find(batchId);
    if (s != removedSpottings.end())
    {
        for (Spotting& spotting : s->second)
            addSpotting(spotting,newExemplars, false,false);
        removedSpottings.erase(s);
    }
}

vector<Spotting*> Knowledge::Word::result(string selected, unsigned long batchId, bool resend, vector< pair<unsigned long, string> >* toRemoveExemplars)
{
    vector<Spotting*> ret;
#ifdef TEST_MODE
        //cout<<"[write] "<<gt<<" ("<<tlx<<","<<tly<<") result"<<endl;
#endif
    pthread_rwlock_wrlock(&lock);
    if (resend)
    {
        reAddSpottings(batchId, &ret);
    }
    if (!done)
    {
        done=true;
        transcription=selected;
        ret = harvest();
#if TRANS_DONT_WAIT
        sentPoss.clear();
#endif
    }
    else
    {
        //this is a resubmission
        if (transcription.compare(selected)!=0)
        {
            //Retract harvested ngrams
            toRemoveExemplars->insert(toRemoveExemplars->end(),harvested.begin(),harvested.end());
            transcription=selected;
            ret= harvest();
        }
    }
    pthread_rwlock_unlock(&lock);
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") result"<<endl;
#endif
    return ret;
    //Harvested ngrams should be approved before spotting with them
}

TranscribeBatch* Knowledge::Word::error(unsigned long batchId, bool resend, vector<Spotting*>* newExemplars, vector< pair<unsigned long, string> >* toRemoveExemplars)
{
    TranscribeBatch* newBatch=NULL;
#ifdef TEST_MODE
        //cout<<"[write] "<<gt<<" ("<<tlx<<","<<tly<<") error"<<endl;
#endif
    pthread_rwlock_wrlock(&lock);
#if TRANS_DONT_WAIT
    //rejectedTrans.insert(sentPoss.begin(), sentPoss.end());
    for (auto p : sentPoss)
        rejectedTrans.insert(p.second);
    sentPoss.clear();

    if (notSent.size()>0)//send this low priority batch which still may have the correct transcription
    {
        int numToSend = std::min((int) notSent.size(),(int) THRESH_SCORING_COUNT);
        auto iterNotSent = notSent.begin();
        for (int i=0; i< numToSend; i++)
            iterNotSent++;
        sentPoss = multimap<float,string>(notSent.begin(), iterNotSent);
        notSent = multimap<float,string>(iterNotSent, notSent.end());
        newBatch = new TranscribeBatch(this,sentPoss,pagePnt,&spottings,tlx,tly,brx,bry,gt,sentBatchId,true);//low priority
        pthread_rwlock_unlock(&lock);
        return newBatch;
    }
#endif

    if (!loose)
    {
        //Given most errors are caused by misalgined ngrams and bad char width predictions
        //loosening the regex matching might allow the correct word in
        loose=true;
        //With the loosened constriants, we still may be able to produce a batch
        newBatch=queryForBatch(newExemplars);
    }
    else
    {
        bool removedSpotting = removeWorstSpotting(batchId);
        if (removedSpotting)
        {
            newBatch=queryForBatch(newExemplars);
        }
        else
        {
            //This code should never run, now that removeWorstSpotting() removes a spotting each call
            if (resend)
            {
                //This is a shortened reAddSpottings(), as we know we don't need to do anything else;
                auto s = removedSpottings.find(batchId);
                if (s != removedSpottings.end())
                {
                    for (Spotting& spotting : s->second)
                        spottings.insert(make_pair(spotting.tlx,spotting));//multimap, so they are properly ordered
                    removedSpottings.erase(s);
                }

            }
            vector<Spotting> removed;
            for (auto p : spottings)
            {
                removed.push_back(p.second);
            }
            removedSpottings[batchId]=removed;
            spottings.clear();
            toRemoveExemplars->insert(toRemoveExemplars->end(),harvested.begin(),harvested.end());
            harvested.clear();
        }
    }
    pthread_rwlock_unlock(&lock);
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") error"<<endl;
#endif
    if (newBatch==NULL)
        sentBatchId=0;
    return newBatch;
}

TranscribeBatch* Knowledge::Word::addSpotting(Spotting s, vector<Spotting*>* newExemplars, bool findBatch, bool doLock)
{
#ifdef TEST_MODE
        //cout<<"[write] "<<gt<<" ("<<tlx<<","<<tly<<") addSpotting"<<endl;
#endif
    if (doLock)
    {
        pthread_rwlock_wrlock(&lock);
    }
#if NO_ERROR
    assert (gt.find(s.ngram) != string::npos);
#endif
    if (done && (s.type==SPOTTING_TYPE_TRANS_FALSE || transcription.find(s.ngram) == string::npos))
    {
        if (doLock)
        {
            pthread_rwlock_unlock(&lock);
        }
        return NULL;
    }
    //decide if it should be merge with another
    int width = s.brx-s.tlx;
    bool merged=false;
    for (auto otherS : spottings)
    {
        if (otherS.second.ngram.compare(s.ngram)==0)
        {
            int overlap = min(otherS.second.brx,s.brx)-max(otherS.second.tlx,s.tlx);
            if (overlap > width/2)
            {
                //These are probably the same and should be merged
                otherS.second.tlx = (otherS.second.tlx+s.tlx)/2;
                otherS.second.brx = (otherS.second.brx+s.brx)/2;
                merged=true;
                //cout <<"merge"<<endl;
                break;
            }
        }
    }
    if (!merged)
    {
#if TRANS_DONT_WAIT
        notSent.clear();
#endif
        spottings.insert(make_pair(s.tlx,s));//multimap, so they are properly ordered
    }
    TranscribeBatch* ret=NULL;
    if (findBatch && !done)
        ret=queryForBatch(newExemplars);//This has an id matching the sent batch (if it exists)
    if (doLock)
    {
        pthread_rwlock_unlock(&lock);
    }
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") addSpotting"<<endl;
#endif
    return ret;
}

TranscribeBatch* Knowledge::Word::queryForBatch(vector<Spotting*>* newExemplars)
{
    //if (sentBatchId!=0)
    //    return NULL;
    string newQuery = generateQuery(spottings.end());
    set<unsigned long> removedSpottings;
    TranscribeBatch* ret=NULL;
    if (query.compare(newQuery) !=0)
    {
        query=newQuery;
#if TRANS_DONT_WAIT
        vector<string> matches = Lexicon::instance()->search(query,meta,rejectedTrans);
#else
        vector<string> matches = Lexicon::instance()->search(query,meta);
#endif
        if (matches.size() == 0)
        {
            //First see if removing non-overlapping spottings helps
            for (auto iter=spottings.begin(); iter!=spottings.end(); iter++)
            {
                if (!iter->second.overlap)
                {
                    string removedQuery = generateQuery(iter);
                    if (query.compare(removedQuery) !=0)
                    {
#if TRANS_DONT_WAIT
                        vector<string> matchesR = Lexicon::instance()->search(removedQuery,meta,rejectedTrans);
#else
                        vector<string> matchesR = Lexicon::instance()->search(removedQuery,meta);
#endif
                        if (matchesR.size()>0)
                        {
                            removedSpottings.insert(iter->second.id);
                            matches.insert(matches.begin(),matchesR.begin(),matchesR.end());
                        }
                    }
                }

            }

            if (matches.size() == 0)
            {
                //Then see if removing the other spottings helps
                for (auto iter=spottings.begin(); iter!=spottings.end(); iter++)
                {
                    if (iter->second.overlap)
                    {
                        string removedQuery = generateQuery(iter);
                        if (query.compare(removedQuery) !=0)
                        {
#if TRANS_DONT_WAIT
                            vector<string> matchesR = Lexicon::instance()->search(removedQuery,meta,rejectedTrans);
#else
                            vector<string> matchesR = Lexicon::instance()->search(removedQuery,meta);
#endif
                            if (matchesR.size()>0)
                            {
                                removedSpottings.insert(iter->second.id);
                                matches.insert(matches.begin(),matchesR.begin(),matchesR.end());
                            }
                        }
                    }

                }
            }
        }
        /*
#if TRANS_DONT_WAIT
        //filter out matches we've previously rejected
        auto iter = matches.begin();
        while (iter != matches.end())
        {
            if (rejectedTrans.find(*iter) != rejectedTrans.end())
                iter = matches.erase(iter);
            else
                iter++;
        }
#endif*/
        if (matches.size() == 1 && matches[0].length()<=GlobalK::knowledge()->AUTO_TRANS_LEN_THRESH)
        {
            transcription=matches[0];
            done=true;
            if (newExemplars!=NULL)
            {
                vector<Spotting*> newE = harvest();
                newExemplars->insert(newExemplars->end(),newE.begin(),newE.end());
            }
        }
        else if (matches.size() < THRESH_LEXICON_LOOKUP_COUNT)
        {
            multimap<float,string> scored = scoreAndThresh(matches);//,*threshScoring);
#if TRANS_DONT_WAIT
            if (scored.size() > 0)
            {
                int numToSend = min((int)scored.size(),(int)THRESH_SCORING_COUNT);
                auto iterScored = scored.begin();
                for (int i=0; i<numToSend; i++)
                    iterScored++;
                sentPoss = multimap<float,string>(scored.begin(), iterScored);
                notSent = multimap<float,string>(iterScored, scored.end());
                ret = new TranscribeBatch(this,sentPoss,pagePnt,&spottings,tlx,tly,brx,bry,gt,sentBatchId);
            }
#else
            if (scored.size()>0 && scored.size()<THRESH_SCORING_COUNT)
            {
                //ret= createBatch(scored);
                ret = new TranscribeBatch(this,scored,pagePnt,&spottings,tlx,tly,brx,bry,gt,sentBatchId);
                removeSpottings(removedSpottings);
            }
#endif
        }
        else if (matches.size()==0 && !loose)
        {
            if (!loose)
            {
                loose=true;
                return queryForBatch(newExemplars);
            }
            /*else removed this as the new remove-and-check method is a more exhaustive way to this
            {
                bool removedSpotting = removeWorstSpotting();
                if (removedSpotting)
                    return queryForBatch(newExemplars);
                //OoV or something is wrong. Return as manual batch.
                //But we'll do it later to keep the state of the Corpus's queue good.
                //ret= new TranscribeBatch(this,vector<string>(),pagePnt,&spottings,tlx,tly,brx,bry,sentBatchId);
            }*/
        }
    }
    if (ret!=NULL)
        sentBatchId=ret->getId();
    return ret;
}

bool Knowledge::Word::removeWorstSpotting(unsigned long batchId)
{

    if (spottings.size()==0)
        return false;
    //curretly removes auto-accepted spottings, and then the highest scored spotting
    //float maxScoreQbE=-9999;
    //auto maxIterQbE = spottings.begin();
    //float maxScoreQbS=-9999;
    //auto maxIterQbS = spottings.begin();
    multimap<float,unsigned long> scoresQbE, scoresQbS;
    auto iter = spottings.begin();
    while (iter!=spottings.end())
    {
        if (iter->second.type==SPOTTING_TYPE_THRESHED)
        {
            if (batchId!=0)
                removedSpottings[batchId].push_back(iter->second);
            iter=spottings.erase(iter);
            return true;//only do first
        }
        else
        {
            /*if (iter->second.scoreQbE>maxScoreQbE)
            {
                maxScoreQbE = iter->second.scoreQbE;
                maxIterQbE=iter;
            }
            if (iter->second.scoreQbE>maxScoreQbE)
            {
                maxScoreQbE = iter->second.scoreQbE;
                maxIterQbE=iter;
            }*/
            if (iter->second.scoreQbE==iter->second.scoreQbE)
                scoresQbE.emplace(iter->second.scoreQbE,iter->second.id);
            else
                scoresQbE.emplace(MAX_FLOAT,iter->second.id);
            if (iter->second.scoreQbS==iter->second.scoreQbS)
                scoresQbS.emplace(iter->second.scoreQbS,iter->second.id);
            else
                scoresQbS.emplace(MAX_FLOAT,iter->second.id);
            iter++;
        }
    }
    map<unsigned long, int> ranks;
    auto iterScores=scoresQbE.begin();
    for (int i=0; i<scoresQbE.size(); i++, iterScores++)
    {
        ranks[iterScores->second]+=i;
    }
    iterScores=scoresQbS.begin();
    for (int i=0; i<scoresQbS.size(); i++, iterScores++)
    {
        ranks[iterScores->second]+=i;
    }
    float maxRank=-99999;
    unsigned long maxId;
    for (auto r : ranks)
    {
        if (r.second>maxRank)
        {
            maxRank=r.second;
            maxId=r.first;
        }
    }
    multimap<int,Spotting>::iterator iterMax=spottings.begin();
    while (iterMax->second.id != maxId)
        iterMax++;


    if (batchId!=0)
        removedSpottings[batchId].push_back(iterMax->second);
        //removedSpottings[batchId].push_back(maxIter->second);
    spottings.erase(iterMax);
    //spottings.erase(maxIter);

    //can we fix spotting boundaries? loose should do this to some degree...
    return true;
}

void Knowledge::Word::removeSpottings(const set<unsigned long>& toRemove)
{
    auto iter = spottings.begin();
    while (iter!=spottings.end())
    {
        if (toRemove.find(iter->second.id) != toRemove.end())
        {
            //I dont think we need to readd
            //removedSpottings[batchId].push_back(iter->second);
            iter=spottings.erase(iter);
        }
        else
            iter++;
    }
}


vector<string> Knowledge::Word::getRestrictedLexicon(int max)
{
#ifdef TEST_MODE
        //cout<<"[read] "<<gt<<" ("<<tlx<<","<<tly<<") getRestrictedLexicon"<<endl;
#endif
    pthread_rwlock_rdlock(&lock);
    string newQuery = generateQuery(spottings.end());
    pthread_rwlock_unlock(&lock);
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") getRestrictedLexicon"<<endl;
#endif
    SearchMeta nmeta(meta);
    nmeta.max=max;
    return Lexicon::instance()->search(query,nmeta);
}

TranscribeBatch* Knowledge::Word::removeSpotting(unsigned long sid, unsigned long batchId, bool resend, unsigned long* sentBatchId, vector<Spotting*>* newExemplars, vector< pair<unsigned long, string> >* toRemoveExemplars)
{
#ifdef TEST_MODE
        //cout<<"[write] "<<gt<<" ("<<tlx<<","<<tly<<") removeSpotting"<<endl;
#endif
    pthread_rwlock_wrlock(&lock);
    if (resend)
    {
        reAddSpottings(batchId,newExemplars);
    }
    if (sentBatchId!=NULL)
        *sentBatchId = this->sentBatchId;

    toRemoveExemplars->insert(toRemoveExemplars->end(),harvested.begin(),harvested.end());
    for (auto iter= spottings.begin(); iter!=spottings.end(); iter++)
    {
        if (iter->second.id == sid)
        {
            if (batchId!=0 && sentBatchId==NULL)
            {
                vector<Spotting> aSpot={iter->second};
                removedSpottings[batchId]=aSpot;
            }
            spottings.erase(iter);
            break;
        }
    }
    TranscribeBatch* ret=NULL;
    if (spottings.size()>0)
        ret=queryForBatch(newExemplars);
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") removeSpotting"<<endl;
#endif
    if (ret==NULL)
        this->sentBatchId=0;
    pthread_rwlock_unlock(&lock);
    return ret;
}

multimap<float,string> Knowledge::Word::scoreAndThresh(vector<string> match) const //, float thresh)
{
    multimap<float,string> scores;
    //swan threads?
    float min=999999;
    float max=-999999;
#ifdef TEST_MODE
    //cout<<"Scoring poss  ";
#endif
    for (string word : match)
    {
        const Mat im=getWordImg();
        assert(*spotter != NULL);
        float score = (*spotter)->score(word,id);
        //float score = (*spotter)->score(word,spottingIndex);
        //float score = (*spotter)->score(word,im);
        scores.insert(make_pair(score,word));
        if (score<min)
            min=score;
        if (score>max)
            max=score;

#ifdef TEST_MODE
        //cout<<word<<":"<<score<<", ";
#endif
    }
#ifdef TEST_MODE
    //cout<<endl;
#endif
    if (min==max)
        return scores;

    vector<int> histogram(100);
    for (auto p : scores)
    {
        int bin = 99*(p.first-min)/(max-min);
        
        histogram.at(bin)++;
    }
    float thresh = (GlobalK::otsuThresh(histogram)/100)*(max-min)+min;
    
    if (match.size()>LOW_COUNT_PRUNE_THRESH || min+(max-min)*LOW_COUNT_SCORE_THRESH<thresh)
    {
        multimap<float,string> ret(scores.begin(),scores.lower_bound(thresh));
        //for (string m : match)
        //    ret.insert(make_pair(0.1,m));
#ifdef TEST_MODE
        for (auto iter =scores.lower_bound(thresh); iter!=scores.end(); iter++)
            if (iter->second.compare(gt)==0)
            {
                cout<<"[!]Pruning discarded correct: "<<gt<<endl;
                for (auto iter2=scores.begin(); iter2!=scores.lower_bound(thresh); iter2++)
                    cout<<"+ "<<iter2->first<<": "<<iter2->second<<endl;
                for (auto iter2=scores.lower_bound(thresh); iter2!=iter; iter2++)
                    cout<<"- "<<iter2->first<<": "<<iter2->second<<endl;
                cout<<"- "<<iter->first<<": "<<iter->second<<endl;
            }

#endif
#ifdef NO_NAN
        for (auto iter =scores.lower_bound(thresh); iter!=scores.end(); iter++)
            if (iter->second.compare(gt)==0)
                GlobalK::knowledge()->badPrune();
#endif
        return ret;
    }
    else
        return scores;
}

/*TranscribeBatch* Knowledge::Word::createBatch(multimap<float,string> scored)
{
    vector<string> pos;
    for (auto p : scored)
    {
        pos.push_back(p.second);
    }    
    return new TranscribeBatch(pos,cv::Mat(),cv::Mat());
}*/

/*
bool getOverlap(int overlap, string ret, string& ngramToAdd)
{
    //recursive
    if (placeO>=overlap || placeO>=ngramToAdd.length())
    {
        //We remove overlapping characters, this removes duplicates
        if (setOverlap)
        {
            (--spot)->second.overlap=true;//prev spotting
            (++spot)->second.overlap=true;//back to current
        }


        if (overlap==1)
            ret+="?";//condition the matching character
        else if (overlap>=ngramToAdd.length())
            ngramToAdd="";
        else
            ngramToAdd=ngramToAdd.substr(overlap);
        return true;
    }

    assert((overlap-placeO)<=ret.length());
    if ((placeO-overlap)+1<0 && ret[ret.length()-(overlap-placeO)+1]=='?')
    {
        //dont skip
        string ngramToAddInclude(ngramToAdd);
        string retInclude = ret.substr(0,ret.length()-(overlap-placeO)+1)+ret.substr(ret.length()-(overlap-placeO)+2);
        bool included = getOverlap(placeO, retInclude, ngramToAddInclude, setOverlap);
        if (included)
        {
            ret=retInclude;
            ngramToAdd=ngramToAddInclude;
            return true;
        }

        //skip
        string ngramToAddSkip(ngramToAdd);
        string retSkip = ret.substr(0,ret.length()-(overlap-placeO))+ret.substr(ret.length()-(overlap-placeO)+2);
        bool skipped = getOverlap(placeO, retSkip, ngramToAddSkip, setOverlap);
        if (skipped)
        {
            ret=retSkip;
            ngramToAdd=ngramToAddSkip;
            return true;
        }

    }
    else
    {
        if (ret[ret.length()-(overlap-placeO)] != ngramToAdd[placeO])
        {
            return false;
        }
        return getOverlap(placeO+1, ret, ngramToAdd, setOverlap);
    }
}*/

void Knowledge::Word::getPossibleStrings(string s, set<string>& ret)
{
    bool found=false;
    for (int i=0; i<s.length(); i++)
    {
        if (s[i]=='?')
        {
            getPossibleStrings(s.substr(0,i-1)+s.substr(i+1));//remove char
            getPossibleStrings(s.substr(0,i)+s.substr(i+1));//keep char
            found=true;
        }
    }
    if (!found)
        ret.insert(s);

}

set<string> Knowledge::Word::getPossibleStrings(string s)
{
    set<string> ret;
    getPossibleStrings(s,ret);
    return ret;
}

/*
string Knowledge::Word::generateQuery(multimap<int,Spotting>::iterator skip)
{
    bool setOverlap = skip==spottings.end();
    auto spot = spottings.begin();
    int pos = tlx;
    string ret = "";
    float estTotalChars = (brx-tlx)/(0.0+*averageCharWidth);
    while (spot != spottings.end())
    {
        if (spot == skip)
        {
            spot++;
            continue;//SKIP!
        }
        
        if (skip==spottings.end())
            spot->second.overlap=false;
        string addToEnd="";
        int dif = spot->second.tlx-pos;
        float numChars = dif/(0.0+*averageCharWidth);
        //cout <<"pos: "<<pos<<" str: "<<spot->second.tlx<<endl;
        //cout <<"num chars: "<<numChars<<endl;
        string ngramToAdd = spot->second.ngram;
        
        
        if (numChars>0) //There are probably chars between the spottings
        {
            int least = floor(numChars);
            int most = ceil(numChars);
            
            if (numChars-least < THRESH_UNKNOWN_EST)
                least-=1;
            if (most-numChars < THRESH_UNKNOWN_EST && spot!=spottings.begin())
                most+=1;

            if (spot==spottings.begin()) //This character (particularly capitol) is often larger
            {
                least-=1;
                most = std::max(0,(int)ceil(numChars - 0.5*std::min(1.0f,1.0f/estTotalChars)));
            }
            if (loose)
            {
                least-=1;
                most+=1;
            }

            least = max(0,least);
            if (numChars<0.5 && ret.length()>0 && ret[ret.length()-1]==ngramToAdd[0])//as we always put "?" at the end of the previous char, this works (we don't compare to "?")
            {
                if (setOverlap)
                {
                    (--spot)->second.overlap=true;//prev spotting
                    (++spot)->second.overlap=true;//back to current
                }
                if (ngramToAdd[0]=='l' || ngramToAdd[0]=='s' || ngramToAdd[0]=='e')
                    ret+="?";//condition the matching character
                else
                    ngramToAdd=ngramToAdd.substr(1);
            }
            else if (most>0)
                ret += "[a-zA-Z0-9]{"+to_string(least)+","+to_string(most)+"}";
        }
        else
        {
            set<string> rets = getPossibleStrings(ret);
            for (int overlap=std::min((int)ceil(-1*numChars)+1,(int)ret.length()); overlap>0; overlap--) //The "+1" is a safety
            {
                for (string retPoss : rets)
                {
                    bool didOverlap = true;//getOverlap(overlap, retPoss, ngramToAdd);
                    for (int placeO=0; placeO<overlap && placeO<ngramToAdd.length(); placeO++)
                    {
                        assert((overlap-placeO)<=ret.length());
                        if (ret[ret.length()-(overlap-placeO)] != ngramToAdd[placeO])
                        {
                            didOverlap=false;
                            break;
                        }
                    }
                    if (didOverlap)
                    {
                        if (setOverlap)
                        {
                            (--spot)->second.overlap=true;//prev spotting
                            (++spot)->second.overlap=true;//back to current
                        }
                        ret=retPoss;
                        if (overlap==1 && (ngramToAdd[0]=='l' || ngramToAdd[0]=='s' || ngramToAdd[0]=='e')) //l,s,e most common doubles
                            ret+="?";//condition the matching character
                        else if (overlap>=ngramToAdd.length())
                            ngramToAdd="";
                        else
                            ngramToAdd=ngramToAdd.substr(overlap);
                        overlap=0;
                        break;
                    }
                }
            }


            //Because spottings often are large, we allow a character to occur between overlaping ones
            //if (-1*numChars<THRESH_UNKNOWN_EST/2.0 || loose)
            //    ret += "[a-zA-Z0-9]?";
        }

        //Add the spotting
        ret += ngramToAdd+addToEnd;

        //Move the currrent position to the end of the furthest spotting processed
        pos = std::max(pos,spot->second.brx);
        spot++;
    }
    
    int dif = brx-pos;
    float numChars = dif/(0.0+*averageCharWidth);
    //cout <<"pos: "<<pos<<" end: "<<brx<<endl;
    //cout <<"E num chars: "<<numChars<<endl;
    if (numChars>0)
    {
        int least = floor(numChars);
        int most = std::max(0,(int)ceil(numChars - 0.5*std::min(1.0f,1.0f/estTotalChars)));
        
        if (numChars-least < THRESH_UNKNOWN_EST)
            least-=1;
        //if (most-numChars < THRESH_UNKNOWN_EST)
        //    most+=1;

        //The last character sometimes has a trail making it larger
        least-=1;
        if (loose)
        {
            least-=1;
            most+=1;
        }
        least = max(0,least);
        if(most!=0)
            ret += "[a-zA-Z0-9]{"+to_string(least)+","+to_string(most)+"}";
    }
    //cout << "query : "<<ret<<endl;
    return ret;
}*/

string Knowledge::Word::generateQuery(multimap<int,Spotting>::iterator skip)
{
    auto spot = spottings.begin();
    int pos = tlxBetter();
    string ret = "";
    float estTotalChars = (brxBetter()-tlxBetter())/(0.0+*averageCharWidth);
    while (spot != spottings.end())
    {
        if (spot == skip)
        {
            spot++;
            continue;//SKIP!
        }
        
        if (skip==spottings.end())
            spot->second.overlap=false;
        string addToEnd="";
        int dif = spot->second.tlx-pos;
        float numChars = dif/(0.0+*averageCharWidth);
        //cout <<"pos: "<<pos<<" str: "<<spot->second.tlx<<endl;
        //cout <<"num chars: "<<numChars<<endl;
        string ngramToAdd = spot->second.ngram;
        
        
        if (numChars>0) //There are probably chars between the spottings
        {
            int least = floor(numChars);
            int most = ceil(numChars);
            
            if (numChars-least < THRESH_UNKNOWN_EST)
                least-=1;
            if (most-numChars < THRESH_UNKNOWN_EST && spot!=spottings.begin())
                most+=1;

            if (spot==spottings.begin()) //This character (particularly capitol) is often larger
            {
                least-=1;
                most = std::max(0,(int)ceil(numChars - 0.5*std::min(1.0f,1.0f/estTotalChars)));
            }
            if (loose)
            {
                least-=1;
                most+=1;
            }

            least = max(0,least);
            if (numChars<0.5 && ret.length()>0 && ret[ret.length()-1]==ngramToAdd[0])
            {
                if (skip==spottings.end())
                {
                    (--spot)->second.overlap=true;//prev spotting
                    (++spot)->second.overlap=true;//back to current
                }
                
                ngramToAdd=ngramToAdd.substr(1);
            }
            else if (most>0)
                ret += "[a-zA-Z0-9]{"+to_string(least)+","+to_string(most)+"}";
        }
        else
        {
            for (unsigned int overlap=std::min((int)ceil(-1*numChars)+1,(int)ret.length()); overlap>0; overlap--) //The "+1" is a safety
            {
                bool isOverlap=true;
                for (int placeO=0; placeO<overlap && placeO<ngramToAdd.length(); placeO++)
                {
                    assert((overlap-placeO)<=ret.length());
                    if (ret[ret.length()-(overlap-placeO)] != ngramToAdd[placeO])
                    {
                        isOverlap=false;
                        break;
                    }
                }
                if (isOverlap)
                {
                    //We remove overlapping characters, this removes duplicates
                    if (skip==spottings.end())
                    {
                        (--spot)->second.overlap=true;//prev spotting
                        (++spot)->second.overlap=true;//back to current
                    }

                    if (overlap>=ngramToAdd.length())
                        ngramToAdd="";
                    else
                        ngramToAdd=ngramToAdd.substr(overlap);

                    break;
                }
            }


            //Because spottings often are large, we allow a character to occur between overlaping ones
            //if (-1*numChars<THRESH_UNKNOWN_EST/2.0 || loose)
            //    ret += "[a-zA-Z0-9]?";
        }

        //Add the spotting
        ret += ngramToAdd+addToEnd;

        //Move the currrent position to the end of the furthest spotting processed
        pos = std::max(pos,spot->second.brx);
        spot++;
    }
    
    int dif = brxBetter()-pos;
    float numChars = dif/(0.0+*averageCharWidth);
    //cout <<"pos: "<<pos<<" end: "<<brx<<endl;
    //cout <<"E num chars: "<<numChars<<endl;
    if (numChars>0)
    {
        int least = floor(numChars);
        int most = std::max(0,(int)ceil(numChars - 0.5*std::min(1.0f,1.0f/estTotalChars)));
        
        if (numChars-least < THRESH_UNKNOWN_EST)
            least-=1;
        //if (most-numChars < THRESH_UNKNOWN_EST)
        //    most+=1;

        //The last character sometimes has a trail making it larger
        least-=1;
        if (loose)
        {
            least-=1;
            most+=1;
        }
        least = max(0,least);
        if(most!=0)
            ret += "[a-zA-Z0-9]{"+to_string(least)+","+to_string(most)+"}";
    }
    return ret;
}

int Knowledge::Word::tlxBetter()
{
    if (_tlxBetter==-1)
        refineHorzBoundaries();
    return _tlxBetter;
}
int Knowledge::Word::brxBetter()
{
    if (_brxBetter==-1)
        refineHorzBoundaries();
    return _brxBetter;
}

void Knowledge::Word::refineHorzBoundaries()
{
    Mat wordIm, bin;
    getWordImgAndBin(wordIm, bin);
    Mat profile;
    reduce(bin,profile,0,CV_REDUCE_SUM,CV_32S);
    int x=0;
    while (profile.at<int>(0,x)<=255)
        x++;
    assert(x<profile.cols);
    _tlxBetter=tlx+x;
    x=0;
    while (profile.at<int>(0,profile.cols-(x+1))<=255)
        x++;
    assert(x<profile.cols);
    _brxBetter=brx-x;

    assert(_tlxBetter>=tlx && _tlxBetter<_brxBetter &&  _brxBetter<=brx);
}

/*
string Knowledge::Word::generateQuery()
{
    auto spot = spottings.begin();
    int pos = tlx;
    string ret = "";
    while (spot != spottings.end())
    {
        string addToEnd="";
        int dif = spot->second.tlx-pos;
        float numChars = dif/(0.0+*averageCharWidth);
        //cout <<"pos: "<<pos<<" str: "<<spot->second.tlx<<endl;
        //cout <<"num chars: "<<numChars<<endl;
        
        
        if (numChars>0) //There are probably chars between the spottings
        {
            int least = floor(numChars);
            int most = ceil(numChars);
            
            if (numChars-least < THRESH_UNKNOWN_EST)
                least-=1;
            if (most-numChars < THRESH_UNKNOWN_EST)
                most+=1;

            if (spot==spottings.begin()) //This character (particularly capitol) is often larger
                least-=1;
            if (loose)
            {
                least-=1;
                most+=1;
            }

            least = max(0,least);
            ret += "[a-zA-Z0-9]{"+to_string(least)+","+to_string(most)+"}";
        }
        else
        {
            //There may be doubles or overlap
            //we allow both

            //check for overlap starting at full overlap
            for (unsigned int overlap=std::min(spot->second.ngram.length()+(int)floor(0.5-1*numChars),ret.length()); overlap>0; overlap--)
            {
                bool isOverlap=true;
                for (int placeO=0; placeO<overlap && placeO<spot->second.ngram.length(); placeO++)
                {
                    assert((overlap-placeO)<=ret.length());
                    if (ret[ret.length()-(overlap-placeO)] != spot->second.ngram[placeO])
                    {
                        isOverlap=false;
                        break;
                    }
                }
                if (isOverlap)
                {
                    //set a ? around the overlapping region to allow for double or overlap
                    if (spot->second.ngram.length()-overlap < 0)
                    {
                        assert(ret.length()-overlap+spot->second.ngram.length() < ret.length());
                        addToEnd = ret.substr(ret.length()-overlap+spot->second.ngram.length());
                    }
                    assert(0+ret.length()-overlap<=ret.length());
                    assert(ret.length()-overlap<ret.length());
                    assert(ret.length()-overlap + std::min(overlap,(unsigned int) (spot->second.ngram.length())) <=ret.length());
                    ret = ret.substr(0,ret.length()-overlap) + "(" + ret.substr(ret.length()-overlap,std::min(overlap,(unsigned int) (spot->second.ngram.length()))) + ")?";
                    break;
                }
            }

            //Because spottings often are large, we allow a character to occur between overlaping ones
            if (-1*numChars<THRESH_UNKNOWN_EST/2.0 || loose)
                ret += "[a-zA-Z0-9]?";
        }

        //Add the spotting
        ret += spot->second.ngram+addToEnd;

        //Move the currrent position to the end of the furthest spotting processed
        pos = std::max(pos,spot->second.brx);
        spot++;
    }
    
    int dif = brx-pos;
    float numChars = dif/(0.0+*averageCharWidth);
    //cout <<"pos: "<<pos<<" end: "<<brx<<endl;
    //cout <<"E num chars: "<<numChars<<endl;
    if (numChars>0)
    {
        int least = floor(numChars);
        int most = ceil(numChars);
        
        if (numChars-least < THRESH_UNKNOWN_EST)
            least-=1;
        if (most-numChars < THRESH_UNKNOWN_EST)
            most+=1;

        //The last character sometimes has a trail making it larger
        least-=1;
        if (loose)
        {
            least-=1;
            most+=1;
        }
        least = max(0,least);
        ret += "[a-zA-Z0-9]{"+to_string(least)+","+to_string(most)+"}";
    }
    //cout << "query : "<<ret<<endl;
    return ret;
}*/

const cv::Mat Knowledge::Word::getWordImg() const
{
    return (*pagePnt)(cv::Rect(tlx,tly,brx-tlx+1,bry-tly+1));
}
const cv::Mat Knowledge::Word::getImg()// const
{
#ifdef TEST_MODE
        //cout<<"[read] "<<gt<<" ("<<tlx<<","<<tly<<") getImg"<<endl;
#endif
    pthread_rwlock_rdlock(&lock);
    const cv::Mat ret = getWordImg();
    pthread_rwlock_unlock(&lock);
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") getImg"<<endl;
#endif
    return ret;
}
void Knowledge::Word::getWordImgAndBin(cv::Mat& wordImg, cv::Mat& b)
{
    //cv::Mat b;
    int blockSize = (1+bry-tly)/2;
    if (blockSize%2==0)
        blockSize++;
    assert(blockSize>1);
    wordImg = getWordImg();
    //wordImg = (*pagePnt)(cv::Rect(tlx,tly,brx-tlx+1,bry-tly+1));
    if (wordImg.type()==CV_8UC3)
        cv::cvtColor(wordImg,wordImg,CV_RGB2GRAY);
    cv::adaptiveThreshold(wordImg, b, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, blockSize, 10);
}

vector<Spotting*> Knowledge::Word::harvest()
{
#if NO_EXEMPLARS
    return vector<Spotting*>();
#endif
    assert(false && "harvest() not implemented for variable ngrams");

#ifdef TEST_MODE
    //cout<<"harvesting: "<<transcription<<endl;
#endif
    assert (transcription[0]!='$' || transcription[transcription.length()-1]!='$');

    cv::Mat wordImg, b;
    vector<Spotting*> ret;
    string unspotted = transcription;
    multimap<int,Spotting> spottingsByIndex;
    int curI =0;
    for (auto p : spottings)
    {
        bool found = false;
        for (; curI<transcription.length(); curI++)
        {
            if (transcription.substr(curI,p.second.ngram.length()).compare(p.second.ngram)==0)
            {
                found=true;
                spottingsByIndex.insert(make_pair(curI,p.second));
                for (int i=curI; i<curI+p.second.ngram.length(); i++)
                    unspotted[i]='$';
#ifdef TEST_MODE
                //cout <<"["<<curI<<"]:"<<p.second.ngram<<", ";
#endif
                break;
            }
        }
        if (!found)
        {
#ifdef TEST_MODE
            cout<<"ERROR, Word::harvest(): failed to find ngram ("<<p.second.ngram<<") in transcription ("<<transcription<<")"<<endl;
#endif
            return ret;
        }
    }
#ifdef TEST_MODE
    //cout<<endl<<"unspotted: "<<unspotted<<endl;;
#endif
    for (int i=0; i< 1+transcription.length()-MIN_N; i++)
    {
        for (int n=MIN_N; n<=MAX_N; n++)
        {
            //if we have anchors on either side
            if ((i==0|| (unspotted[i-1]=='$' || unspotted[i]=='$')) && (i==transcription.length()-n || (i<transcription.length()-n && (unspotted[i+n]=='$' || unspotted[i+n-1]=='$'))))
            {
                bool isNew=true;
                string ngram = transcription.substr(i,n);
                /*for (int offset=0; offset<n; offset++)
                {
                    if (unspotted[i+offset]!='$')
                    {
                        isNew=true;
                        break;
                    }
                }*/
                auto startAndEnd = spottingsByIndex.equal_range(i);
                for (auto iter=startAndEnd.first; iter!=startAndEnd.second; iter++)
                {
#ifdef TEST_MODE
                        //cout<<"comparing, ["<<ngram<<"] to ["<<iter->second.ngram<<"] at "<<i<<endl;
#endif
                    if (iter->second.ngram.compare(ngram)==0)
                    {
                        isNew=false;
#ifdef TEST_MODE
                        //cout<<"Nope, ["<<ngram<<"] already there. "<<i<<endl;
#endif
                        break;
                    }
                }
                if (isNew)
                {
#ifdef TEST_MODE
                    //cout<<"New ngram: "<<ngram<<", at "<<i<<endl;
#endif
                    int rank = GlobalK::knowledge()->getNgramRank(ngram);
                    if ( rank<=MAX_NGRAM_RANK )
                    {
                        //getting left x location
                        int leftLeftBound =0;
                        int rightLeftBound=0;
                        int bc=0;
                        //start from first char of new exemplar, working backwards
                        for (int si=i; si>=0; si--)
                        {
                            startAndEnd = spottingsByIndex.equal_range(si);
                            for (auto iter=startAndEnd.first; iter!=startAndEnd.second; iter++)
                            {
                                if (iter->second.ngram.length()+si==i)
                                {
                                    assert(iter->second.ngram[iter->second.ngram.length()-1]==transcription[i-1]);
                                    //bounds from end
                                    leftLeftBound  += iter->second.brx - (iter->second.brx-iter->second.tlx)/8;
                                    rightLeftBound += iter->second.brx + (iter->second.brx-iter->second.tlx)/8;
                                    bc++;
                                }
                                else if (iter->second.ngram.length()+si>i)
                                {
                                    //bounds from middle
                                    int numOverlap = iter->second.ngram.length()+si-i;
                                    for (int oo=0; oo<numOverlap; oo++)
                                        assert( transcription[i+oo]==ngram[oo] );
                                    int per = (iter->second.brx-iter->second.tlx)/iter->second.ngram.length();
                                    int loc = per * (iter->second.ngram.length()-numOverlap) + iter->second.tlx;
                                    leftLeftBound  += loc - (iter->second.brx-iter->second.tlx)/5;
                                    rightLeftBound += loc + (iter->second.brx-iter->second.tlx)/5;
                                    bc++;
                                }
                            }
                        }
                        if (bc>0)
                        {
                            leftLeftBound /=bc;
                            rightLeftBound/=bc;
                            //etlx = getBreakPoint(lowerBound,tly,upperBound,bry,pagePnt);
                        }
                        else
                        {
                            assert(i==0);
                            //etlx=tlx;
                            leftLeftBound=tlx; 
                            rightLeftBound=tlx; 
                        }
                        //getting right x location
                        int leftRightBound =0;
                        int rightRightBound=0;
                        bc=0;
                        //search from one char into the new exemplar, working forwards
                        for (int si=i+1; si<=i+n; si++)
                        {
                            auto startAndEnd = spottingsByIndex.equal_range(si);
                            for (auto iter=startAndEnd.first; iter!=startAndEnd.second; iter++)
                            {
                                if (si==i+n)
                                {
                                    assert(iter->second.ngram[0]==transcription[i+n]);
                                    //bounds from front
                                    leftRightBound  += iter->second.tlx - (iter->second.brx-iter->second.tlx)/7;
                                    rightRightBound += iter->second.tlx + (iter->second.brx-iter->second.tlx)/7;
                                    bc++;
                                }
                                else if (si<i+n)
                                {
                                    //bounds from middle
                                    int numTo = (i+n)-si;
                                    int per = (iter->second.brx-iter->second.tlx)/iter->second.ngram.length();
                                    int loc = per * (numTo) + iter->second.tlx;
                                    leftRightBound += loc - (iter->second.brx-iter->second.tlx)/5.5;
                                    rightRightBound += loc + (iter->second.brx-iter->second.tlx)/5.5;
                                    //int nloc = getBreakPoint(leftRightBound,tly,rightRightBound,bry,pagePnt);
                                    //leftRightBound += (nloc-loc)/2;
                                    //rightRightBound += (nloc-loc)/2;
                                    bc++;
                                }
                            }
                        }
                        if (bc>0)
                        {
                            leftRightBound/=bc;
                            rightRightBound/=bc;
                            //ebrx = getBreakPoint(lowerBound,tly,upperBound,bry,pagePnt);
                        }
                        else
                        {
                            assert(i==transcription.length()-n);
                            //ebrx=brx;
                            leftRightBound=brx;
                            rightRightBound=brx;
                        }
                        if (rightLeftBound >= leftRightBound)
                        {
                            int mid=(rightLeftBound+leftRightBound)/2;
                            rightLeftBound=mid-2;
                            leftRightBound=mid+2;
                        }
                        if (wordImg.cols==0)
                            getWordImgAndBin(wordImg,b);
                        SpottingExemplar* toRet = extractExemplar(leftLeftBound,rightLeftBound,leftRightBound,rightRightBound,ngram,wordImg,b);
                        if (toRet != NULL)
                        {
                            //Spotting toRet(etlx,tly,ebrx,bry,pageId,pagePnt,ngram,0.0);
                            //toRet.type=SPOTTING_TYPE_EXEMPLAR;
                            toRet->ngramRank=rank;
                            //toRet.setHarvested();
                            ret.push_back(toRet);
                            //ret.emplace(ngram,(*pagePnt)(cv::Rect(etlx,etly,ebrx-etlx+1,bry-tly+1));
#ifdef TEST_MODE
#if SHOW
                            cout <<"EXAMINE harvested: "<<ngram<<endl;
                            cv::imshow("harvested",toRet->ngramImg());
                            cv::imshow("boundary image",toRet->img());
#ifdef TEST_MODE_LONG
                            cv::waitKey(100);
#else
                            cv::waitKey();
#endif
#endif
#endif
                        }
                    }
                }
            }
        }
    }
    
    harvested.clear();
    for (Spotting* s : ret)
        harvested.insert(make_pair(s->id,s->ngram));
    /*
    //Also harvest char width
    if (wordImg.cols==0)
        getWordImgAndBin(wordImg,b);
    vector<int> vHist(wordImg.cols);
    map<int,int> hist;
    int maxV=0;
    int total=0;
    for (int c=0; c<wordImg.cols; c++) {
        for (int r=0; r<wordImg.rows; r++)
            vHist[c]+=wordImg.at<unsigned char>(r,c);
        hist[vHist[c]]+=1;
        total++;
        if (vHist[c]>maxV)
            maxV=vHist[c];
    }

    
    //sparse GlobalK::otsuThresh
    double sum =0;
    for (int i = 1; i <= maxV; ++i) {
        if (hist.find(i)!=hist.end())
            sum += i * hist[i];
    }
    double sumB = 0;
    double wB = 0;
    double wF = 0;
    double mB;
    double mF;
    double max = 0.0;
    double between = 0.0;
    double threshold1 = 0.0;
    double threshold2 = 0.0;
    for (int i = 0; i < maxV; ++i)
    {
        if (hist.find(i)!=hist.end())
            wB += hist[i];
        if (wB == 0)
            continue;
        wF = total - wB;
        if (wF == 0)
            break;
        if (hist.find(i)!=hist.end())
            sumB += i * hist[i];
        mB = sumB / (wB*1.0);
        mF = (sum - sumB) / (wF*1.0);
        between = wB * wF * pow(mB - mF, 2);
        if ( between >= max )
        {
            threshold1 = i;
            if ( between > max )
            {
                threshold2 = i;
            }
            max = between; 
        }
    }
    
    double thresh = ( threshold1 + threshold2 ) / 2.0;
    int newLeft, newRight;
    for (newLeft=0; newLeft<vHist.size(); newLeft++)
        if (vHist[newLeft]>thresh)
            break;
    newLeft--;
    for (newRight=vHist.size()-1; newRight>=0; newRight--)
        if (vHist[newRight]>thresh)
            break;
    newRight++;
    float charWidth = (newRight-newLeft+0.0f)/transcription.length();
    *countCharWidth++;
    *averageCharWidth = ((*countCharWidth-1.0)/(*countCharWidth))*(*averageCharWidth) + (1.0/(*countCharWidth))*charWidth;
#ifdef TEST_MODE
    cout <<"harvested char width: "<<charWidth<<endl;
    cout <<"averageCharWidth now: "<<*averageCharWidth<<endl;
#endif
    */
    return ret;

}

inline void setEdge(int x1, int y1, int x2, int y2, GraphType* g, const cv::Mat &img)
{
    assert(x1>=0 && x1<img.cols && y1>=0 && y1<img.rows && x2>=0 && x2<img.cols && y2>=0 && y2<img.rows);
    float w = ((255-img.at<unsigned char>(y1,x1))+(255-img.at<unsigned char>(y2,x2)))/(255.0+255.0);
    //w = 1/(1+exp(-2*w));
    w = w*w*w;
    g -> add_edge(x1+y1*img.cols, x2+y2*img.cols,w,w);
    //cout << w <<endl;
}

#ifdef TEST_MODE
void Knowledge::Word::emergencyAnchor(cv::Mat& b, GraphType* g,int startX, int endX, float sum_anchor, float goal_sum, bool word, cv::Mat& showA)
#else
void Knowledge::Word::emergencyAnchor(cv::Mat& b, GraphType* g,int startX, int endX, float sum_anchor, float goal_sum, bool word)
#endif
{
    float baselineH = (botBaseline-topBaseline)/2.0;
    int c=startX;
    float inc=.5;
    float addStr=1;
    //float init = sum_anchor;
    //float sum_anchor=0;
    while (sum_anchor < goal_sum)
    {

        for (int r=topBaseline+1; r<botBaseline; r++)
        {
            float str= (baselineH-fabs((r-topBaseline)-baselineH))/baselineH;
            if (b.at<unsigned char>(wordCord(r,c)))
            {
                g -> add_tweights(wordIndex(r,c), ANCHOR_CONST*addStr*(word?1:0),ANCHOR_CONST*addStr*(word?0:1));
                sum_anchor+=addStr;
#ifdef TEST_MODE
                showA.at<cv::Vec3b>(wordCord(r,c))[0]=(word?1:0)*max(.40*ANCHOR_CONST*addStr,255.0);
                showA.at<cv::Vec3b>(wordCord(r,c))[1]=(word?0:1)*max(.40*ANCHOR_CONST*addStr,255.0);
                showA.at<cv::Vec3b>(wordCord(r,c))[2]=0;
#endif
            }
            //else
            //    g -> add_tweights(wordIndex(r,c), 0,NGRAM_GRAPH_BIAS);
        }
        if (endX>startX)
        {
            if (++c > endX) 
            {
                c=startX;
                endX+=2;
                //addStr+=inc;
            }
        }
        else
        {
            if (--c < endX) 
            {
                c=startX;
                endX-=2;
                //addStr+=inc;
            }
        }
    }
}

SpottingExemplar* Knowledge::Word::extractExemplar(int leftLeftBound, int rightLeftBound, int leftRightBound, int rightRightBound, string newNgram, cv::Mat& wordImg, cv::Mat& b)
{
    //assert(leftLeftBound<=rightLeftBound && rightLeftBound<leftRightBound && leftRightBound<=rightRightBound);
    if (!(leftLeftBound<=rightLeftBound && rightLeftBound<leftRightBound && leftRightBound<=rightRightBound))
    {
        cout<<"WARNING: extractExemplar given mixed up data ("<<leftLeftBound<<", "<<rightLeftBound<<", "<<leftRightBound<<", "<<rightRightBound<<"). Not extracting."<<endl;
        return NULL;
    }

    if (topBaseline==-1 || botBaseline==-1)
        findBaselines(wordImg,b);

    int width = (brx+1)-tlx;
    int height = (bry+1)-tly;

    GraphType *g = new GraphType(width*height, 2*(width)*(height)-(height+width));

    for (int i=0; i<width*height; i++)
    {
        g->add_node();
    }
#ifdef TEST_MODE
    cv::Mat showA;
    cv::cvtColor(wordImg,showA,CV_GRAY2RGB);
#endif    

    //Add anchors
    float baselineH = (botBaseline-topBaseline)/2.0;
    float sum_anchor_wordFront=0;
    float sum_anchor_wordBack=0;
    float sum_anchor_ngram=0;
    for (int c=tlx; c<=brx; c++)
    {
        float anchor_word=0;
        float anchor_ngram=0;
        if (c<leftLeftBound || c>rightRightBound)
        {
            anchor_word=ANCHOR_CONST;
            /*for (int r=tly; r<topBaseline; r++)
                if (b.at<unsigned char>(wordCord(r,c)))
                    g -> add_tweights(wordIndex(r,c), NGRAM_GRAPH_BIAS,0);
            for (int r=botBaseline+1; r<bry; r++)
                if (b.at<unsigned char>(wordCord(r,c)))
                    g -> add_tweights(wordIndex(r,c), NGRAM_GRAPH_BIAS,0);*/
        }
        else if (c> rightLeftBound && c<leftRightBound)
        {
            anchor_ngram=ANCHOR_CONST;
        }
        /*else if (c>=leftLeftBound && c<leftLeftBound+0.3*(rightLeftBound-leftLeftBound))
        {
            anchor_word = ((0.3*(rightLeftBound-leftLeftBound))-(c-leftLeftBound))/(0.3*(rightLeftBound-leftLeftBound));
        }
        else if (c<=rightRightBound && c>rightRightBound-0.3*(rightRightBound-leftRightBound))
        {
            anchor_ngram = ((0.3*(rightRightBound-leftRightBound))-(rightRightBound-c))/(0.3*(rightRightBound-leftRightBound));
        }*/
        for (int r=topBaseline+1; r<botBaseline; r++)
        {
            float str= (baselineH-fabs((r-topBaseline)-baselineH))/baselineH;
            if (b.at<unsigned char>(wordCord(r,c)))
            {
                g -> add_tweights(wordIndex(r,c), anchor_word*str,anchor_ngram*str);
                if (c<leftLeftBound)
                    sum_anchor_wordFront+=str;
                else if (c>rightRightBound)
                    sum_anchor_wordBack+=str;
                else if (c> rightLeftBound && c<leftRightBound)
                    sum_anchor_ngram+=str;
#ifdef TEST_MODE
                showA.at<cv::Vec3b>(wordCord(r,c))[0]=.40*anchor_word*str;
                showA.at<cv::Vec3b>(wordCord(r,c))[1]=.40*anchor_ngram*str;
                showA.at<cv::Vec3b>(wordCord(r,c))[2]=0;
#endif
            }
            //else
            //    g -> add_tweights(wordIndex(r,c), 0,NGRAM_GRAPH_BIAS);
        }
        /*for (int r=tly; r<topBaseline; r++)
            if (!b.at<unsigned char>(wordCord(r,c)))
                g -> add_tweights(wordIndex(r,c), 0,NGRAM_GRAPH_BIAS);
        for (int r=botBaseline+1; r<bry; r++)
            if (!b.at<unsigned char>(wordCord(r,c)))
                g -> add_tweights(wordIndex(r,c), 0,NGRAM_GRAPH_BIAS);*/
    }

    //Ensure some anchor gets placed
    if (leftLeftBound!=rightLeftBound &&sum_anchor_wordFront < min(sum_anchor_wordBack,sum_anchor_ngram)/2)
    {
        emergencyAnchor(b,g,tlx, leftLeftBound, sum_anchor_wordFront, min(sum_anchor_wordBack,sum_anchor_ngram)/2, true
#ifdef TEST_MODE
               , showA
#endif
               );
    }
    if (leftRightBound!=rightRightBound && sum_anchor_wordBack < min(sum_anchor_wordFront,sum_anchor_ngram)/2)
    {
        emergencyAnchor(b,g,brx, rightRightBound, sum_anchor_wordBack, min(sum_anchor_wordFront,sum_anchor_ngram)/2, true
#ifdef TEST_MODE
               , showA
#endif
               );
    }
    if (sum_anchor_ngram < min(sum_anchor_wordBack,sum_anchor_wordFront)/2)
    {
        emergencyAnchor(b,g,rightLeftBound, leftRightBound, sum_anchor_ngram, min(sum_anchor_wordBack,sum_anchor_wordFront)/2, false
#ifdef TEST_MODE
               , showA
#endif
               );
    }



#ifdef TEST_MODE
#if SHOW
    //cv::resize(showA,showA,cv::Size(),2,2,cv::INTER_NEAREST);
    cv::imshow("anchors",showA);
#ifdef TEST_MODE_LONG
    //if (newNgram.compare("ex")==0)
    //{
    //    cv::waitKey();
    //}
#endif
#endif
#endif

    //set up graph
    for (int i=0; i<width; i++)
    {
        for (int j=0; j<height; j++)
        {
            if (i+1<width)
            {
                setEdge(i,j,i+1,j,g,wordImg);
            }

            if (j+1<height)
            {
                setEdge(i,j,i,j+1,g,wordImg);
            }

            /*if (j>0 && i<width-1)
            {
                setEdge(i,j,i+1,j-1,g,wordImg);
            }

            if (j<height-1 && i<width-1)
            {
                setEdge(i,j,i+1,j+1,g,wordImg);
            }*/
        }
    }
    g -> maxflow();
    
    cv::Mat mask(b.rows,b.cols,CV_8U);
    int etlx=99999;
    int etly=99999;
    int ebrx=-99999;
    int ebry=-99999;

    for (int i=0; i<width; i++)
    {
        for (int j=0; j<height; j++)
        {
            int index=i+j*width;
            if (g->what_segment(index) == GraphType::SOURCE)
            {
                mask.at<unsigned char>(j,i) = 0;
            }
            else
            {
                mask.at<unsigned char>(j,i) = 1;
                if (b.at<unsigned char>(j,i))
                {
                    if (i<etlx)
                        etlx=i;
                    if (j<etly)
                        etly=j;
                    if (i>ebrx)
                        ebrx=i;
                    if (j>ebry)
                        ebry=j;
        
                }
            }

        }
    }
    //pad
    int pad=2;
    if (etlx-pad>=0)
        etlx-=pad;
    if (etly-pad>=0)
        etly-=pad;
    if (ebrx+pad<width)
        ebrx+=pad;
    if (ebry+pad<height)
        ebry+=pad;

    //Saftey
    if (
            (etlx<0 || etlx>=width) ||
            (etly<0 || etly>=height) ||
            (ebrx<0 || ebrx>=width) ||
            (ebry<0 || ebry>=height) ||
            (ebrx-etlx>2.5*(rightRightBound-leftLeftBound)) //heuristic
       )
    {
        delete g;
        return NULL;
    }

#ifndef NO_NAN
    cv::Mat exe = inpainting(wordImg(cv::Rect(etlx,etly,1+ebrx-etlx,1+ebry-etly)),mask(cv::Rect(etlx,etly,1+ebrx-etlx,1+ebry-etly)));
#else
    cv::Mat exe = wordImg(cv::Rect(etlx,etly,1+ebrx-etlx,1+ebry-etly));
#endif
    SpottingExemplar* ret = new SpottingExemplar(tlx+etlx,tly+etly,tlx+ebrx,tly+ebry,pageId,pagePnt,newNgram,exe);
#ifdef TEST_MODE
#if SHOW
    cv::Mat show;
    cv::cvtColor(wordImg,show,CV_GRAY2RGB);
    
    
    for (int i=0; i<width; i++)
    {
        for (int j=0; j<height; j++)
        {
            int index=i+j*width;
            if (g->what_segment(index) == GraphType::SOURCE)
            {
                show.at<cv::Vec3b>(j,i)[0] = 0;
                show.at<cv::Vec3b>(j,i)[1] = 255;
            }
            else
            {
                show.at<cv::Vec3b>(j,i)[1] = 0;
                show.at<cv::Vec3b>(j,i)[0] = 255;
            }
        }
    }
    cv::imshow("seg results",show);
    //cv::imshow("exe",ret->ngramImg());
    //cv::imshow("fromPage",ret->img());
    //cv::waitKey();
#endif
#endif
    delete g;
    return ret;
}

void Knowledge::Word::getBaselines(int* top, int* bot)
{
#ifdef TEST_MODE
        //cout<<"[read] "<<gt<<" ("<<tlx<<","<<tly<<") getBaselines"<<endl;
#endif
    pthread_rwlock_rdlock(&lock);
    if (topBaseline<0 || botBaseline<0)
    {
        pthread_rwlock_unlock(&lock);
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") getBaselines"<<endl;
#endif
#ifdef TEST_MODE
        //cout<<"[write] "<<gt<<" ("<<tlx<<","<<tly<<") getBaselines"<<endl;
#endif
        pthread_rwlock_wrlock(&lock);
        
        cv::Mat wordImg, b;
        getWordImgAndBin(wordImg,b);
        findBaselines(wordImg,b);

        *top=topBaseline;
        *bot=botBaseline;
        pthread_rwlock_unlock(&lock);
    }
    else
    {
    
        *top=topBaseline;
        *bot=botBaseline;
        pthread_rwlock_unlock(&lock);
    }
#ifdef TEST_MODE
        //cout<<"[unlock] "<<gt<<" ("<<tlx<<","<<tly<<") getBaselines"<<endl;
#endif
}

void Knowledge::Word::findBaselines(const cv::Mat& gray, const cv::Mat& bin)
{
    assert(gray.cols==bin.cols && gray.rows==bin.rows); 
    //top and botBaseline should have page cords.
    int avgWhite=0;
    int countWhite=0;
    cv::Mat hist = cv::Mat::zeros(gray.rows,1,CV_32F);
    map<int,int> topPixCounts, botPixCounts;
    for (int c=0; c<gray.cols; c++)
    {
        int topPix=-1;
        int lastSeen=-1;
        for (int r=0; r<gray.rows; r++)
        {
            if (bin.at<unsigned char>(r,c))
            {
                if (topPix==-1)
                {
                    topPix=r;
            }
                lastSeen=r;
            }
            else
            {
                avgWhite+=gray.at<unsigned char>(r,c);
                countWhite++;
            }
            hist.at<float>(r,0)+=gray.at<unsigned char>(r,c);
        }
        if (topPix!=-1)
            topPixCounts[topPix]++;
        if (lastSeen!=-1)
            botPixCounts[lastSeen]++;
    }
    avgWhite /= countWhite;

    int maxTop=-1;
    int maxTopCount=-1;
    for (auto c : topPixCounts)
    {
        if (c.second > maxTopCount)
        {
            maxTopCount=c.second;
            maxTop=c.first;
        }
    }
    int maxBot=-1;
    int maxBotCount=-1;
    for (auto c : botPixCounts)
    {
        if (c.second > maxBotCount)
        {
            maxBotCount=c.second;
            maxBot=c.first;
        }
    }

    //cv::Mat kernel = cv::Mat::ones( 5, 1, CV_32F )/ (float)(5);
    //cv::filter2D(hist, hist, -1 , kernel );
    cv::Mat edges;
    int pad=5;
    cv::Mat paddedHist = cv::Mat::ones(hist.rows+2*pad,1,hist.type());
    double avg=0;
    double maxHist=-99999;
    double minHist=99999;
    for (int r=0; r<hist.rows; r++)
    {
        avg+=hist.at<float>(r,0);
        if (hist.at<float>(r,0)>maxHist)
            maxHist=hist.at<float>(r,0);
        if (hist.at<float>(r,0)<minHist)
            minHist=hist.at<float>(r,0);
    }
    avg/=hist.rows;
    paddedHist *= avg;
    hist.copyTo(paddedHist(cv::Rect(0,pad,1,hist.rows)));
    float kernelData[11] = {1,1,1,1,.5,0,-.5,-1,-1,-1,-1};
    cv::Mat kernel = cv::Mat(11,1,CV_32F,kernelData);
    cv::filter2D(paddedHist, edges, -1 , kernel );//, Point(-1,-1), 0 ,BORDER_AVERAGE);
    float kernelData2[11] = {.1,.1,.1,.1,.1,.1,.1,.1,.1,.1,.1};
    cv::Mat kernel2 = cv::Mat(11,1,CV_32F,kernelData2);
    cv::Mat blurred;
    cv::filter2D(hist, blurred, -1 , kernel2 );//, Point(-1,-1), 0 ,BORDER_AVERAGE);
    topBaseline=-1;
    float maxEdge=-9999999;
    botBaseline=-1;
    float minEdge=9999999;
    float minPeak=9999999;
    float center=-1;
    for (int r=pad; r<gray.rows+pad; r++)
    {
        assert(r<edges.rows);
        float v = edges.at<float>(r,0);
        if (v>maxEdge)
        {
            maxEdge=v;
            topBaseline=r-pad;
        }
        if (v<minEdge)
        {
            minEdge=v;
            botBaseline=r-pad;
        }

        assert(r-pad<blurred.rows);
        if (blurred.at<float>(r-pad,0) < minPeak) {
            center=r-pad;
            minPeak=blurred.at<float>(r-pad,0);
        }
    }
    if (topBaseline>center)
    {
        if (maxTop < center)
            topBaseline=maxTop;
        else
        {
            maxEdge=-999999;
            for (int r=pad; r<center+pad; r++)
            {
                assert(edges.rows>r);
                float v = edges.at<float>(r,0);
                if (v>maxEdge)
                {
                    maxEdge=v;
                    topBaseline=r-pad;
                }
            }
        }
    }
    if (botBaseline<center)
    {
        if (maxBot > center)
            botBaseline=maxBot;
        else
        {
            minEdge=999999;
            for (int r=center+1; r<gray.rows+pad; r++)
            {
                assert(edges.rows>r);
                float v = edges.at<float>(r,0);
                if (v<minEdge)
                {
                    minEdge=v;
                    botBaseline=r-pad;
                }
            }
        }
    }
    if (botBaseline < topBaseline)//If they fail this drastically, the others won't be much better.
    {
        topBaseline=maxTop;
        botBaseline=maxBot;
    }
    topBaseline += tly;
    botBaseline += tly;
}


int Knowledge::getBreakPoint(int lxBound, int ty, int rxBound, int by, const cv::Mat* pagePnt)
{
    int retX;
    cv::Mat b;// = GlobalK::otsuThresh(*t.pagePnt);
    int blockSize = max((rxBound-lxBound)/2,(by-ty)/2);
    if (blockSize%2==0)
        blockSize++;
    assert(blockSize>1);
    cv::Mat orig = (*pagePnt)(cv::Rect(lxBound,ty,rxBound-lxBound+1,by-ty+1));
    if (orig.type()==CV_8UC3)
        cv::cvtColor(orig,orig,CV_RGB2GRAY);
    cv::adaptiveThreshold(orig, b, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, blockSize, 10);
    
    cv::Mat hist(b.cols,1,CV_32F);
    cv::Mat tb(b.cols,1,CV_32F);
    //vector<int> top(b.cols,9999);
    //vector<int> bot(b.cols,-9999);
    for (int c=0; c<b.cols; c++)
    {
        int top=9999;
        int bot=-9999;
        bool hitTop=false;
        for (int r=0; r<b.rows; r++)
        {
            assert(c<hist.rows && r<orig.rows && c<orig.cols);
            hist.at<float>(c,0)+=orig.at<unsigned char>(r,c);
            if (b.at<unsigned char>(r,c))
            {
                if (!hitTop)
                {
                    top=r;
                    hitTop=true;
                }
                bot=r;
            }
        }
        
        tb.at<float>(c,0)=bot-top;
    }
    float kernelData[5] = {.1,.5,.7,.5,.1};
    cv::Mat kernel = cv::Mat(5,1,CV_32F,kernelData);
    cv::filter2D(hist, hist, -1 , kernel );
    cv::filter2D(tb, tb, -1 , kernel );
    float minHist=99999;
    float maxHist=-99999;
    float minTb=99999;
    float maxTb=-99999;
    for (int i=2; i<b.cols-2; i++)
    {
        float vHist = hist.at<float>(i,0);
        float vTb = tb.at<float>(i,0);
        if (vHist>maxHist)
            maxHist=vHist;
        if (vHist<minHist)
            minHist=vHist;
        if (vTb>maxTb)
            maxTb=vTb;
        if (vTb<minTb)
            minTb=vTb;
    }
    hist = (hist-minHist)/maxHist;
    tb = (tb-minTb)/maxTb;
    
    float min=99999;
    for (int i=2; i<b.cols-2; i++)
    {
        float v = hist.at<float>(i,0) + tb.at<float>(i,0);
        if (v<min)
        {
            min=v;
            retX=i;
        }
    }

#ifdef TEST_MODE
#if SHOW
    orig=orig.clone();
    for (int r=0; r<orig.rows; r++)
        orig.at<unsigned char>(r,retX)=0;
    cv::imshow("break point",orig);
    cv::waitKey();
#endif
#endif

    return retX+lxBound;
}
/*void Knowledge::computeInverseDistanceMap(const cv::Mat &src, int* out)
{
    int maxDist=0;
    int g[src.cols*src.rows];
    for (int x=0; x<src.cols; x++)
    {
        if (src.pixel(x,0))
        {
            g[x+0*src.cols]=0;
        }
        else
        {
            g[x+0*src.cols]=INT_POS_INFINITY;//src.cols*src.rows;
        }
        
        for (int y=1; y<src.rows; y++)
        {
            if (src.pixel(x,y))
            {
                g[x+y*src.cols]=0;
            }
            else
            {
                if (g[x+(y-1)*src.cols] != INT_POS_INFINITY)
                    g[x+y*src.cols]=1+g[x+(y-1)*src.cols];
                else
                    g[x+y*src.cols] = INT_POS_INFINITY;
            }
        }
        
        for (int y=src.rows-2; y>=0; y--)
        {
            if (g[x+(y+1)*src.cols]<g[x+y*src.cols])
            {
                if (g[x+(y+1)*src.cols] != INT_POS_INFINITY)
                    g[x+y*src.cols]=1+g[x+(y+1)*src.cols];
                else
                    g[x+y*src.cols] = INT_POS_INFINITY;
            }
        }
        
    }
    
    int q;
    int s[src.cols];
    int t[src.cols];
    int w;
    for (int y=0; y<src.rows; y++)
    {
        q=0;
        s[0]=0;
        t[0]=0;
        for (int u=1; u<src.cols;u++)
        {
            while (q>=0 && f(t[q],s[q],y,src.cols,g) > f(t[q],u,y,src.cols,g))
            {
                q--;
            }
            
            if (q<0)
            {
                q=0;
                s[0]=u;
            }
            else
            {
                w = SepPlusOne(s[q],u,y,src.cols,g);
                if (w<src.cols)
                {
                    q++;
                    s[q]=u;
                    t[q]=w;
                }
            }
        }
        
        for (int u=src.cols-1; u>=0; u--)
        {
            out[u+y*src.cols]= f(u,s[q],y,src.cols,g);
            if (out[u+y*src.cols] > maxDist)
                maxDist = out[u+y*src.cols];
            if (u==t[q])
                q--;
        }
    }
    
    
    //    QImage debug(src.cols,src.rows,src.format());
    //    debug.setColorTable(src.colorTable());
    //    for (int i=0; i<debug.width(); i++)
    //    {
    //        for (int j=0; j<debug.height(); j++)
    //            debug.setPixel(i,j,(int)((pow(out[i+j*debug.width()],.2)/((double)pow(maxDist,.2)))*255));
            
    //    }
    //    debug.save("./reg_dist_map.pgm");
    //    printf("image format:%d\n",debug.format());
    
    //invert
//    printf("maxDist=%d\n",maxDist);
    maxDist++;
//    double normalizer = (25.0/maxDist);
    double e = 10;
    double b = 25;
    double m = 2000;
    double a = INV_A;
    int max_cc_size=500;
    
//    double normalizer = (b/m);
    BImage mark = src.makeImage();
    QVector<QPoint> workingStack;
    QVector<QPoint> growingComponent;
    
    
    int newmax = 0;
    int newmax2 = 0;
    int newmin = INT_MAX;
    for (int q = 0; q < src.cols*src.rows; q++)
    {   
        //out[q] = pow(6,24-out[q]*normalizer)/pow(6,20);
        if (src.pixel(q%src.cols,q/src.cols) && mark.pixel(q%src.cols,q/src.cols))
        {
            //fill bias
            QPoint p(q%src.cols,q/src.cols);
            workingStack.push_back(p);
            mark.setPixel(p,false);
            while (!workingStack.isEmpty())
            {   
                QPoint cur = workingStack.back();
                workingStack.pop_back();
                growingComponent.append(cur);
                
                
                
                
                if (cur.x()>0 && mark.pixel(cur.x()-1,cur.y()))
                {
                    QPoint pp(cur.x()-1,cur.y());
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                }
                
                
                if (cur.x()<mark.width()-1 && mark.pixel(cur.x()+1,cur.y()))
                {
                    QPoint pp(cur.x()+1,cur.y());
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                    
                }
                if (cur.y()<mark.height()-1 && mark.pixel(cur.x(),cur.y()+1))
                {
                    QPoint pp(cur.x(),cur.y()+1);
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                }
                if (cur.y()>0 && mark.pixel(cur.x(),cur.y()-1))
                {
                    QPoint pp(cur.x(),cur.y()-1);
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                }
                //diagonals
                if (cur.x()>0 && cur.y()>0 && mark.pixel(cur.x()-1,cur.y()-1))
                {
                    QPoint pp(cur.x()-1,cur.y()-1);
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                }
                
                
                if (cur.x()<mark.width()-1 && cur.y()>0 && mark.pixel(cur.x()+1,cur.y()-1))
                {
                    QPoint pp(cur.x()+1,cur.y()-1);
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                    
                }
                if (cur.x()>0 && cur.y()<mark.height()-1 && mark.pixel(cur.x()-1,cur.y()+1))
                {
                    QPoint pp(cur.x()-1,cur.y()+1);
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                }
                if (cur.x()<mark.width()-1 && cur.y()>0 && mark.pixel(cur.x()+1,cur.y()-1))
                {
                    QPoint pp(cur.x()+1,cur.y()-1);
                    workingStack.push_back(pp);
                    mark.setPixel(pp,false);
                }
            }
            int cc_size = growingComponent.size();
            while (!growingComponent.isEmpty())
            {
                QPoint cur = growingComponent.back();
                growingComponent.pop_back();
                int index = cur.x()+src.cols*cur.y();
                out[index] = pow(b-std::min(out[index]*(b/m),b),e)*a/(pow(b,e)) + std::min(cc_size,max_cc_size) + 1;
                
                if (out[index]>newmax)
                    newmax=out[index];
                
                if (out[index]>newmax2 && out[index]<newmax)
                    newmax2=out[index];
                
                if (out[index]<newmin)
                    newmin=out[index];
            }
        }
        else if (!src.pixel(q%src.cols,q/src.cols))
        {
            out[q] = pow(b-std::min(out[q]*(b/m),b),e)*a/(pow(b,e)) + 1;
        }

        if (out[q]>newmax)
            newmax=out[q];
        if (out[q]>newmax2 && out[q]<newmax)
            newmax2=out[q];
        if (out[q]<newmin)
            newmin=out[q];
    }
    
}*/
cv::Mat Knowledge::inpainting(const cv::Mat& src, const cv::Mat& mask, double* avg, double* std, bool show)
{
    assert(src.rows == mask.rows && src.cols==mask.cols);
    int x_start[4] = {0,0,mask.cols-1,mask.cols-1};
    int x_end[4] = {mask.cols,mask.cols,-1,-1};
    int y_start[4] = {0,mask.rows-1,0,mask.rows-1};
    int y_end[4] = {mask.rows,-1,mask.rows,-1};
    cv::Mat dst = src.clone();
    cv::Mat P[4];
    cv::Mat I = src.clone();
    for (int i=0; i<4; i++)
    {
        P[i] = (cv::Mat_<unsigned char>(mask.rows,mask.cols));
        cv::Mat M = mask.clone();
        int yStep = y_end[i]>y_start[i]?1:-1;
        int xStep = x_end[i]>x_start[i]?1:-1;
        for (int y=y_start[i]; y!=y_end[i]; y+=yStep)
            for (int x=x_start[i]; x!=x_end[i]; x+=xStep)
            {
                
                if (M.at<unsigned char>(y,x) == 0)
                {
                    
                    int denom = (x-1>=0?M.at<unsigned char>(y,x-1):0) + 
                                (y-1>=0?M.at<unsigned char>(y-1,x):0) +
                                (x+1<I.cols?M.at<unsigned char>(y,x+1):0) + 
                                (y+1<I.rows?M.at<unsigned char>(y+1,x):0);
                    if (denom !=0)
                    {
                        P[i].at<unsigned char>(y,x) = (
                                                        (x-1>=0?I.at<unsigned char>(y,x-1)*M.at<unsigned char>(y,x-1):0) +
                                                        (y-1>=0?I.at<unsigned char>(y-1,x)*M.at<unsigned char>(y-1,x):0) + 
                                                        (x+1<I.cols?I.at<unsigned char>(y,x+1)*M.at<unsigned char>(y,x+1):0) +
                                                        (y+1<I.rows?I.at<unsigned char>(y+1,x)*M.at<unsigned char>(y+1,x):0)
                                                      )/denom;
                        //if (P[i].at<unsigned char>(y,x)!=0)
                        //{
                            if (avg!=NULL && std!=NULL)
                            {
                                if (P[i].at<unsigned char>(y,x)>*avg)
                                {
                                    double dif = std::min(P[i].at<unsigned char>(y,x)-(*avg), 3*(*std));
                                    P[i].at<unsigned char>(y,x) -= 8.5*dif/(*std);
                                }
                                else if (P[i].at<unsigned char>(y,x)<*avg)
                                {
                                    double dif = std::min((*avg)-P[i].at<unsigned char>(y,x), 3*(*std));
                                    P[i].at<unsigned char>(y,x) += 8.5*dif/(*std);
                                }
                            }
                            I.at<unsigned char>(y,x) = P[i].at<unsigned char>(y,x);
                            M.at<unsigned char>(y,x) = 1;
                        //}
                        //else
                        //    P[i].at<unsigned char>(y,x) = 255;
                    }
                    else
                        P[i].at<unsigned char>(y,x) = 255;
                }
                else
                    P[i].at<unsigned char>(y,x) = I.at<unsigned char>(y,x);
            }
    }
    if (show)
    {
        imshow("P[0]",P[0]); cv::waitKey();
        imshow("P[1]",P[1]); cv::waitKey();
        imshow("P[2]",P[2]); cv::waitKey();
        imshow("P[3]",P[3]); cv::waitKey();
    }
    
    for (int y=0; y<mask.rows; y++)
        for (int x=0; x<mask.cols; x++)
        {
            if (mask.at<unsigned char>(y,x) == 0)
            {
                dst.at<unsigned char>(y,x) = std::min( std::min(P[0].at<unsigned char>(y,x),
                                                                P[1].at<unsigned char>(y,x))
                                                       ,
                                                       std::min(P[2].at<unsigned char>(y,x),
                                                                P[3].at<unsigned char>(y,x))
                                                     );
            }
                                                 
        }
    return dst;
}

void Knowledge::findPotentailWordBoundraies(Spotting s, int* tlx, int* tly, int* brx, int* bry)
{
    //Set it to bounding box of connected component which intersects the given spottings  
    
    //int centerY = s.tly+(s.bry-s.tly)/2.0;
    cv::Mat b;// = GlobalK::otsuThresh(*t.pagePnt);
    int blockSize = max(s.brx-s.tlx,s.bry-s.tly);
    if (blockSize%2==0)
        blockSize++;
    assert(blockSize>1);
    cv::Mat orig = *s.pagePnt;
    if (orig.type()==CV_8UC3)
        cv::cvtColor(orig,orig,CV_RGB2GRAY);
    cv::adaptiveThreshold(orig, b, 255, cv::ADAPTIVE_THRESH_GAUSSIAN_C, cv::THRESH_BINARY_INV, blockSize, 10);
    //cv::imshow("ff", b);
    //cv::waitKey();
    cv::Mat ele = cv::Mat::ones(3,5,CV_32F);//(cv::Mat_<float>(3,5) << 1, 1, 1, 1, 1, 1, 1, 1, 1);
    cv::dilate(b, b, ele);
    //cv::imshow("ff", b);
    //cv::waitKey();
    vector<cv::Point> ps;
    for (int r=s.tly; r<=s.bry; r++)
        for (int c=s.tlx; c<=s.brx; c++)
        {
            if (b.at<unsigned char>(r,c) >0)
            {
                //startX=c;
                //startY=r;
                ps.push_back(cv::Point(c,r));
                b.at<unsigned char>(r,c)=0;
                //r= s.bry+1;
                //break;
            }
        }
    if (ps.size()>0)
    {
        int minX = ps[0].x;
        int maxX = ps[0].x;
        int minY = ps[0].y;
        int maxY = ps[0].y;
        vector<cv::Point> rel = {cv::Point(0,1),cv::Point(0,-1),cv::Point(1,0),cv::Point(-1,0)};
        while (ps.size()>0)
        {
            cv::Point c = ps.back();
            ps.pop_back();
            for (cv::Point r : rel)
            {
                cv::Point n(r.x+c.x,r.y+c.y);
                if (b.at<unsigned char>(n) >0)
                {
                    b.at<unsigned char>(n)=0;
                    if (n.x<minX)
                        minX=n.x;
                    if (n.x>maxX)
                        maxX=n.x;
                    if (n.y<minY)
                        minY=n.y;
                    if (n.y>maxY)
                        maxY=n.y;
                    ps.push_back(n);
                }
            }
        }
       // cout<<"findPotentailWordBoundraies: "<<minX<<", "<<minY<<", "<<maxX<<", "<<maxY<<endl;
        *tlx=min(s.tlx,minX);
        *tly=min(s.tly,minY);
        *brx=max(s.brx,maxX);
        *bry=max(s.bry,maxY);
    }
    else
    {   //no word?
        cout<<"findPotentailWordBoundraies: No word?"<<endl;
        *tlx=s.tlx;
        *tly=s.tly;
        *brx=s.brx;
        *bry=s.bry;
    }
}

void Knowledge::Corpus::show()
{
    map<const cv::Mat*,cv::Mat> draw;
    pthread_rwlock_rdlock(&pagesLock);
    
    for (auto p : pages)
    {
        Page* page = p.second;
        vector<Line*> lines = page->lines();
        for (Line* line : lines)
        {
            int line_ty, line_by;
            vector<Word*> wordsForLine = line->wordsAndBounds(&line_ty,&line_by);
            for (Word* word : wordsForLine)
            {
                int tlx,tly,brx,bry;
                bool done;
                word->getBoundsAndDone(&tlx, &tly, &brx, &bry, &done);
                if (done)
                {
                    if (draw.find(word->getPage()) == draw.end())
                    {
                        if (word->getPage()->type() == CV_8UC3)
                            draw[word->getPage()] = word->getPage()->clone();
                        else
                        {
                            draw[word->getPage()] = cv::Mat();
                            cv::cvtColor(*word->getPage(),draw[word->getPage()],CV_GRAY2BGR);
                        }
                    }
                    cv::putText(draw[word->getPage()],word->getTranscription(),cv::Point(tlx+(brx-tlx)/2,tly+(bry-tly)/2),cv::FONT_HERSHEY_TRIPLEX,2.0,cv::Scalar(50,50,255));
                }
                //else
                //    cout<<"word not done at "<<tlx<<", "<<tly<<endl;
            }
        }
    }
    for (auto p : draw)
    {
        int h =1500;
        int w=((h+0.0)/p.second.rows)*p.second.cols;
        cv::resize(p.second, p.second, cv::Size(w,h));
        cv::imshow("a page",p.second);
        cv::waitKey();
    }
    cout<<"All pages."<<endl;
    cv::waitKey();

    pthread_rwlock_unlock(&pagesLock);
}


void Knowledge::Corpus::mouseCallBackFunc(int event, int x, int y, int flags, void* page_p)
{
    x*=2;
    y*=2;
     Page* page = (Page*) page_p;
     if  ( event == cv::EVENT_LBUTTONDOWN )
     {
          //cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;

        vector<Line*> lines = page->lines();
        for (Line* line : lines)
        {
            int line_ty, line_by;
            vector<Word*> wordsForLine = line->wordsAndBounds(&line_ty,&line_by);
            for (Word* word : wordsForLine)
            {
                int tlx,tly,brx,bry;
                bool done;
                word->getBoundsAndDone(&tlx, &tly, &brx, &bry, &done);
                if (tlx<=x && x<=brx && tly<=y && y<=bry)
                {
                    string query, gt;
                    word->getDoneAndGTAndQuery(&done, &gt, &query);
                    cout<<"WORD: "<<gt<<"  query: "<<query<<endl;
                    vector<Spotting> spots = word->getSpottings();
                    cout<<"Spots: ";
                    for (const Spotting & s : spots)
                    {
                        cout<<s.ngram<<", ";
                    }
                    cout<<endl<<"Poss trans: ";
                    vector<string> poss = word->getRestrictedLexicon(100);
                    for (string p : poss)
                    {
                        cout<<p<<", ";
                    }
                    cout<<endl;
                }
            }
        }
     }
     else if  ( event == cv::EVENT_RBUTTONDOWN )
     {
          //cout << "Right button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }
     else if  ( event == cv::EVENT_MBUTTONDOWN )
     {
          //cout << "Middle button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
     }
     else if ( event == cv::EVENT_MOUSEMOVE )
     {
          //cout << "Mouse move over the window - position (" << x << ", " << y << ")" << endl;

     }
}

void Knowledge::Corpus::showInteractive(int pageId)
{
    cv::Mat draw;
    pthread_rwlock_rdlock(&pagesLock);
    
    Page* page = pages[pageId];
    vector<Line*> lines = page->lines();
    for (Line* line : lines)
    {
        int line_ty, line_by;
        vector<Word*> wordsForLine = line->wordsAndBounds(&line_ty,&line_by);
        for (Word* word : wordsForLine)
        {
            if (draw.cols<2)
            {
                if (word->getPage()->type() == CV_8UC3)
                    draw = word->getPage()->clone();
                else
                {
                    cv::cvtColor(*word->getPage(),draw,CV_GRAY2BGR);
                }
            }
            vector<Spotting> sps = word->getSpottings();
            for (Spotting& s : sps)
            {
                int color = (s.ngram.length()-1)%3;
                for (int x=s.tlx; x<=s.brx; x++)
                    for (int y=s.tly; y<=s.bry; y++)
                    {
                        assert(x<draw.cols && y<draw.rows);
                        draw.at<cv::Vec3b>(y,x)[color] = ((x==s.tlx || x==s.brx || y==s.tly || y==s.bry)?0:0.8)*draw.at<cv::Vec3b>(y,x)[color];
                    }
            }
            int tlx,tly,brx,bry;
            bool done;
            word->getBoundsAndDone(&tlx, &tly, &brx, &bry, &done);
            if (done)
            {
                //cv::putText(draw,word->getTranscription(),cv::Point(tlx+(brx-tlx)/2,tly+(bry-tly)/2),cv::FONT_HERSHEY_COMPLEX_SMALL,1.0,cv::Scalar(50,50,255));
                cv::putText(draw,word->getTranscription(),cv::Point(tlx+(brx-tlx)/5,bry-3),cv::FONT_HERSHEY_COMPLEX_SMALL,1.5,cv::Scalar(5,5,105));
            }
            //else
            //    cout<<"word not done at "<<tlx<<", "<<tly<<endl;
        }
    }
    //int h =1500;
    //int w=((h+0.0)/p.second.rows)*p.second.cols;
    cv::resize(draw,draw, cv::Size(), 0.5,0.5);
    cv::namedWindow("page");
    cv::setMouseCallback("page", mouseCallBackFunc, page);
    cv::imshow("page",draw);
    cv::waitKey();

    pthread_rwlock_unlock(&pagesLock);
}


void Knowledge::Corpus::showProgress(int height, int width)
{
    pthread_rwlock_rdlock(&pagesLock);
    int pageH=0;//pages.begin()->second->getImg()->rows;
    int pageW=0;//pages.begin()->second->getImg()->cols;
    for (auto p : pages)
    {
        Page* page = p.second;
        pageH = max(pageH,page->getImg()->rows);
        pageW = max(pageW,page->getImg()->cols);
    }
    float resizeScale=.001;
    int nAcross, nDown;
    for (float scale=0.5; scale>.001; scale-=.001)
    {
        nAcross=floor(width/(pageW*scale));
        nDown=floor(height/(pageH*scale));
        if (nAcross*nDown>=pages.size())
        {
            resizeScale=scale;
            break;
        }
    }
    cv::Mat draw = cv::Mat::zeros(height,width,CV_8UC3);
    draw = cv::Scalar(100,100,100);
    int xPos=0;
    int yPos=0;
    int across=0;
    for (auto p : pages)
    {
        Page* page = p.second;
        cv::Mat workingIm;
        if (page->getImg()->channels()==1)
            cv::cvtColor(*(page->getImg()), workingIm, CV_GRAY2RGB);
        else if (page->getImg()->channels()==3)
            workingIm = page->getImg()->clone();
        else
            assert(false);
        vector<Line*> lines = page->lines();
        for (Line* line : lines)
        {
            int line_ty, line_by;
            vector<Word*> wordsForLine = line->wordsAndBounds(&line_ty,&line_by);
            for (Word* word : wordsForLine)
            {
                int tlx,tly,brx,bry;
                bool done;
                word->getBoundsAndDone(&tlx, &tly, &brx, &bry, &done);
                if (done)
                {
                    for (int x=tlx; x<=brx; x++)
                        for (int y=tly; y<=bry; y++)
                        {
                            assert(x<workingIm.cols && y<workingIm.rows);
                            workingIm.at<cv::Vec3b>(y,x)[0] = 0.5*workingIm.at<cv::Vec3b>(y,x)[0];
                            workingIm.at<cv::Vec3b>(y,x)[1] = min(255,workingIm.at<cv::Vec3b>(y,x)[1]+120);
                            workingIm.at<cv::Vec3b>(y,x)[2] = 0.5*workingIm.at<cv::Vec3b>(y,x)[2];
                        }
                }
                else
                {
                    vector<Spotting> sps = word->getSpottings();
                    for (Spotting& s : sps)
                        for (int x=s.tlx; x<=s.brx; x++)
                            for (int y=s.tly; y<=s.bry; y++)
                            {
                                assert(x<workingIm.cols && y<workingIm.rows);
                                workingIm.at<cv::Vec3b>(y,x)[0] = 0.5*workingIm.at<cv::Vec3b>(y,x)[0];
                                workingIm.at<cv::Vec3b>(y,x)[2] = min(255,workingIm.at<cv::Vec3b>(y,x)[2]+120);
                                workingIm.at<cv::Vec3b>(y,x)[1] = 0.5*workingIm.at<cv::Vec3b>(y,x)[1];
                            }
                }
            }
        }
        cv::resize(workingIm,workingIm,cv::Size(workingIm.cols*resizeScale,workingIm.rows*resizeScale));
        //cout <<"page dims: "<<workingIm.rows<<", "<<workingIm.cols<<"  at: "<<xPos<<", "<<yPos<<endl;
        //cv::imshow("test",workingIm);
        //cv::waitKey();
        assert(xPos>=0 && yPos>=0 && xPos+workingIm.cols<draw.cols && yPos+workingIm.rows<draw.rows);
        workingIm.copyTo(draw(cv::Rect(xPos,yPos,workingIm.cols,workingIm.rows)));
        xPos+=workingIm.cols;
        if (++across >= nAcross)
        {
            xPos=0;
            yPos+=workingIm.rows;
            across=0;
        }
    }
    //cv::imshow("progress",draw);
    //cv::waitKey(2000);
    cv::imwrite("progress/show.jpg",draw);

    pthread_rwlock_unlock(&pagesLock);
}



void Knowledge::Corpus::addWordSegmentaionAndGT(string imageLoc, string queriesFile)
{
    if (queriesFile.find_last_of('/') != string::npos)
        name=queriesFile.substr(queriesFile.find_last_of('/')+1);
    else
        name=queriesFile;
    ifstream in(queriesFile);
    if (!in.good())
        cout<<"failed to open "<<queriesFile<<endl;
    assert(in.is_open());
    string line;
    
    //std::getline(in,line);
    //float initSplit=0;//stof(line);//-0.52284769;
    //regex nonNum ("[^\\d]");
    numWordsReadIn=0;
    pthread_rwlock_wrlock(&pagesLock);
    while(std::getline(in,line))
    {
        if (line.length()==0)
            continue;
        vector<string> strV;
        //split(line,',',strV);
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, ' ')) {
            strV.push_back(item);
        }
        
        
        string imageFile = strV[0];
        string pageName = (strV[0]);
        string gt = strV[5];
#ifdef NO_NAN
        assert(gt.compare(GlobalK::knowledge()->getSegWord(numWordsReadIn))==0);
#endif
        int tlx=stoi(strV[1]);
        int tly=stoi(strV[2]);
        int brx=stoi(strV[3]);
        int bry=stoi(strV[4]);
        
        //pageName = regex_replace (pageName,nonNum,"");
        //int pageId = stoi(pageName);
        if (pageIdMap.find(pageName)==pageIdMap.end())
        {
            int newId = pageIdMap.size()+1;
            pageIdMap[pageName]=newId;
        }
        int pageId = pageIdMap.at(pageName);
        Page* page;
        if (pages.find(pageId)==pages.end())
        {
            /*if (!writing)
            {
                pthread_rwlock_unlock(&pagesLock);
                pthread_rwlock_wrlock(&pagesLock);
                writing=true;
            }*/
            page = new Page(&spotter,imageLoc+"/"+imageFile,&averageCharWidth,&countCharWidth,pageId);
            pages[page->getId()] = page;
            //cout<<"new page "<<pageId<<endl;
        }
        else
        {
            page = pages[pageId];
        }
        if (tlx<0)
            tlx=0;
        if (tly<0)
            tly=0;
        if (brx >= page->getImg()->cols)
            brx=page->getImg()->cols-1;
        if (bry >= page->getImg()->rows)
            bry=page->getImg()->rows-1;
        if (tlx>brx || tly>bry)
        {
            cout<<"ERROR, fliped box: "+line<<endl;
            exit(1);
        }
        vector<Line*> lines = page->lines();
        bool oneLine=false;
        float bestOverlap=0;
        Line* bestLine=NULL;
        for (Line* line : lines)
        {
            int line_ty, line_by;
            line->wordsAndBounds(&line_ty,&line_by);
            int overlap = min(bry,line_by) - max(tly,line_ty);
            float overlapPortion = overlap/(0.0+bry-tly);
            //cout <<"overlap: "<<overlapPortion<<endl;
            if (overlapPortion > OVERLAP_LINE_THRESH)
            {
                oneLine=true;
                //line->addWord(tlx,tly,brx,bry,gt);
                if (overlapPortion>bestOverlap)
                {
                    bestOverlap=overlapPortion;
                    bestLine=line;
                }
                
            }
        }
        if (oneLine)
            bestLine->addWord(tlx,tly,brx,bry,gt);
        if (!oneLine)
        {
            page->addWord(tlx,tly,brx,bry,gt);
        }
        numWordsReadIn++;

        int len = brx-tlx+1;
        if (len<minWordImageLen)
            minWordImageLen=len;
        if (len>maxWordImageLen)
            maxWordImageLen=len;
    }
    
    /*double heightAvg=0;
    int count=0;

    for (auto pp : pages)
    {
        Page* page = pp.second;
        for (Line* line : page->lines())
        {
            for (Word* word : line->wordsAndBounds(NULL,NULL))
            {
                int topBaseline, botBaseline;
                word->getBaselines(&topBaseline,&botBaseline);
                heightAvg += botBaseline-topBaseline;
                count++;
            }
        }
    }
    heightAvg/=count;
    averageCharWidth=CHAR_ASPECT_RATIO*heightAvg;
    countCharWidth=10;
#ifdef TEST_MODE
    cout<<"avg char height: "<<heightAvg<<endl;
    cout<<"avg char width : "<<averageCharWidth<<endl;
#endif*/
    recreateDatasetVectors(false);
    pthread_rwlock_unlock(&pagesLock);

    in.close();

    //TODO
    //averageCharWidth = getAverageCharWidth();
}



const cv::Mat* Knowledge::Corpus::imgForPageId(int pageId) const
{

    const Page* page = pages.at(pageId);
    return page->getImg();
}

TranscribeBatch* Knowledge::Corpus::makeManualBatch(int maxWidth, bool noSpottings)
{
    TranscribeBatch* ret=NULL;
    for (auto p : _words)
    {
        Word* word = p.second;
        int tlx,tly,brx,bry;
        bool done;
        bool sent;
        string gt;
        word->getBoundsAndDoneAndSentAndGT(&tlx, &tly, &brx, &bry, &done, &sent, &gt);
        //cout << "at word, "<<done<<", "<<sent<<endl;
        if (!done && !sent && (!noSpottings || word->getSpottings().size()==0))
        {
            vector<string> prunedDictionary = word->getRestrictedLexicon(PRUNED_LEXICON_MAX_SIZE);
            if (prunedDictionary.size()>PRUNED_LEXICON_MAX_SIZE)
                prunedDictionary.clear();
            ret = new TranscribeBatch(word,prunedDictionary,word->getPage(),word->getSpottingsPointer(),tlx,tly,brx,bry,gt);
            word->sent(ret->getId());
            //enqueue and dequeue to keep the queue's state good.
            vector<TranscribeBatch*> theBatch = {ret};
            manQueue.enqueueAll(theBatch);
            ret=manQueue.dequeue(maxWidth);
            break;
        }
    }
    return ret;
}

TranscribeBatch* Knowledge::Corpus::getManualBatch(int maxWidth)
{
    TranscribeBatch* ret=manQueue.dequeue(maxWidth);
    if (ret==NULL)
        ret=makeManualBatch(maxWidth,true);
    if (ret==NULL)
        ret=makeManualBatch(maxWidth,false);
    return ret;
}

void Knowledge::Corpus::checkIncomplete()
{
    manQueue.checkIncomplete();
}
vector<Spotting*> Knowledge::Corpus::transcriptionFeedback(unsigned long id, string transcription, vector<pair<unsigned long, string> >* toRemoveExemplars)
{
    if (transcription.find('\n') != string::npos)
        transcription="$PASS$";
    return manQueue.feedback(id,transcription,toRemoveExemplars);
}

const vector<string>& Knowledge::Corpus::labels() const
{
    return _gt;
}
int Knowledge::Corpus::size() const
{
    return _gt.size();
}
const Mat Knowledge::Corpus::image(unsigned int i) const
{
    return 255-_wordImgs.at(i);
}
unsigned int Knowledge::Corpus::wordId(unsigned int i) const
{
    //return _words.at(i)->getId();
    return i;
}
Knowledge::Word* Knowledge::Corpus::getWord(unsigned int i) const
{
    return _words.at(i);
}

void Knowledge::Corpus::recreateDatasetVectors(bool lockPages)
{
    _words.clear();
    _wordImgs.clear();
    _gt.clear();
    if (lockPages)
        pthread_rwlock_rdlock(&pagesLock);
    for (auto p : pages)
    {
        Page* page = p.second;
        vector<Line*> lines = page->lines();
        for (Line* line : lines)
        {
            int line_ty, line_by;
            vector<Word*> wordsInLine = line->wordsAndBounds(&line_ty,&line_by);
            for (Word* word : wordsInLine)
            {
                //word->setSpottingIndex(_words.size());
                //_words.push_back(word);
                //_wordImgs.push_back(word->getImg());
                //_gt.push_back(word->getGT());
                _words[word->getId()]=word;

#ifndef DONT_LOAD
                _wordImgs[word->getId()]=word->getImg();
#endif
                //_gt[word->getId()]=word->getGT();

            }

            //We assume no more words are added at the moment
        }
    }
    assert(_words.size()==numWordsReadIn);
    for (auto p : _words)
    {
        assert(p.second->getId() == _gt.size());
        _gt.push_back(p.second->getGT());
    }
    if (lockPages)
        pthread_rwlock_unlock(&pagesLock);
}

vector<Spotting>* Knowledge::Corpus::runQuery(SpottingQuery* query)// const
{
    vector<SpottingResult> res = spotter->runQuery(query);
#ifdef TEST_MODE
    cout<<"Spotting returned ("<<query->getNgram()<<")."<<endl;
#endif
    vector<Spotting>* ret = new vector<Spotting>(res.size());
    for (int i=0; i<res.size(); i++)
    {
        Knowledge::Word* w = getWord(res[i].imIdx);
        int tlx, tly, brx, bry;
        bool done;
        int gt=0;//UNKNOWN_GT;
        string wordGT;
        w->getBoundsAndDoneAndGT(&tlx, &tly, &brx, &bry, &done, &wordGT);
        //int posN = wordGT.find_first_of(query->getNgram());
        //if (posN ==  string::npos && wordGT.length()>0)
        //    gt=0;
        int endPos=wordGT.length()-(query->getNgram().length());
        for (int c=0; c<= endPos; c++)
        {
            assert(c+query->getNgram().length() <= wordGT.length());
            if (query->getNgram().compare( wordGT.substr(c,query->getNgram().length()) ) == 0)
            {
                //cout <<"found ["<<query->getNgram()<<"] in: "<<wordGT<<endl;
                gt=UNKNOWN_GT;
                break;
            }
        }

        ret->at(i) = Spotting(res[i].startX+tlx, tly, res[i].endX+tlx, bry, w->getPageId(), w->getPage(), query->getNgram(), gt, res[i].imIdx, res[i].startX);
        if (query->getImg().cols==0)//QbS
            ret->at(i).scoreQbS=res[i].score;
        else
            ret->at(i).scoreQbE=res[i].score;
        assert(i==0 || ret->at(i).id != ret->at(i-1).id);
        if (done)
            w->preapproveSpotting(&ret->at(i));
        /*if (word.done)
        {
            if (word->hasNgram(&ret->at(i)))
                ret->at(i).type=SPOTTING_TYPE_TRUE;
            else
                ret->at(i).type=SPOTTING_TYPE_FALSE;
        }*/
    }
    return ret;
}

Spotting Knowledge::Corpus::wrapSpotting(int imIdx, int startX, int endX, float score, string ngram)
{
    Knowledge::Word* w = getWord(imIdx);
    int tlx, tly, brx, bry;
    bool done;
    int gt=0;//UNKNOWN_GT;
    string wordGT;
    w->getBoundsAndDoneAndGT(&tlx, &tly, &brx, &bry, &done, &wordGT);
    int endPos=wordGT.length()-(ngram.length());
    for (int c=0; c<= endPos; c++)
    {
        assert(c+ngram.length() <= wordGT.length());
        if (ngram.compare( wordGT.substr(c,ngram.length()) ) == 0)
        {
            gt=UNKNOWN_GT;
            break;
        }
    }

    Spotting ret(startX+tlx, tly, endX+tlx, bry, w->getPageId(), w->getPage(), ngram, gt, imIdx, startX);
    ret.scoreQbS=score;
    //ret->at(i).scoreQbE=score;
    if (done)
        w->preapproveSpotting(&ret);
    return ret;
}

void Knowledge::Word::preapproveSpotting(Spotting* spotting)
{
    pthread_rwlock_rdlock(&lock);
    int posN = transcription.find_first_of(spotting->ngram);
    if (posN !=  string::npos)
    {
        auto spot = spottings.begin();
        int pos = tlx;
        string ret = "";
        int posWord=0;
        while (spot != spottings.end())
        {
            int posC = transcription.find_first_of(spot->second.ngram);
            if (posC==posN)
            {
                //it will be combine with the one alread in the SpottingResult
                break;
            }
            else if (posN < posC)
            {
                //int numCharInGap=posC-posWord;
                int charsIn = posN-posWord;
                //int gapWidth = spot->second.tlx-pos;
                int pixIn = (spotting->tlx + (spotting->brx-spotting->tlx)/2) - pos;
                int idealPixIn = charsIn*(*averageCharWidth);
                if (fabs(pixIn-idealPixIn)<(*averageCharWidth)*1.75)
                    spotting->type=SPOTTING_TYPE_TRANS_TRUE;
                else
                    spotting->type=SPOTTING_TYPE_TRANS_FALSE;
                break;
            }

            pos = spot->second.brx;
            posWord = posC+spot->second.ngram.length();
            spot++;
        }

        if (spot==spottings.end())
        {
            //int numCharInGap=transcription.length()-posWord;
            int charsIn = posN-posWord;
            //int gapWidth = spot->second.tlx-pos;
            int pixIn = (spotting->tlx + (spotting->brx-spotting->tlx)/2) - pos;
            int idealPixIn = charsIn*(*averageCharWidth);
            if (fabs(pixIn-idealPixIn)<(*averageCharWidth)*1.75)
                spotting->type=SPOTTING_TYPE_TRANS_TRUE;
            else
                spotting->type=SPOTTING_TYPE_TRANS_FALSE;
            
        }
    }
    else
        spotting->type=SPOTTING_TYPE_TRANS_FALSE;

    pthread_rwlock_unlock(&lock);
}

void Knowledge::Corpus::save(ofstream& out)
{
    //string saveName = savePrefix+"_Corpus.sav";
    //ofstream out(saveName);
    out<<"CORPUS"<<endl;
    out<<name<<endl;
    out<<averageCharWidth<<"\n"<<countCharWidth<<"\n"<<threshScoring<<"\n";
    pthread_rwlock_rdlock(&pagesLock);
    out<<"minmax\n"<<minWordImageLen<<"\n"<<maxWordImageLen<<endl;
    out<<pages.size()<<"\n";
    for (auto p : pages)
    {
        out<<p.first<<"\n";
        p.second->save(out);
    }
    out<<pageIdMap.size()<<"\n";
    for (auto p : pageIdMap)
    {
        out<<p.first<<"\n"<<p.second<<"\n";
    }
    out<<numWordsReadIn<<"\n";
    pthread_rwlock_unlock(&pagesLock);

    pthread_rwlock_rdlock(&spottingsMapLock);
    out<<spottingsToWords.size()<<"\n";
    for (auto p : spottingsToWords)
    {
        out<<p.first<<"\n"<<p.second.size()<<"\n";
        for (Word* w : p.second)
            out<<w->getSpottingIndex()<<"\n";
    }
    pthread_rwlock_unlock(&spottingsMapLock);

    //spotter->save(out); use normal load
    manQueue.save(out);
    /*out<<_gt.size()<<"\n";
    for (string g : _gt)
        out<<g<<"\n";
    out<<_words.size()<<"\n";
    for (auto w : _words)
        out<<w->getSpottingIndex()<<"\n";
    out<<_wordImgs.size()<<"\n";
    for (const Mat& m : _wordImages)*/
    //just call recreateDatasetVectors
    //out.close();
    out<<ngramWWFile<<endl;
    out<<maxImageWidth<<endl;
}

Knowledge::Corpus::Corpus(ifstream& in)
{
    //string loadName = loadPrefix+"_Corpus.sav";
    //ifstream in(loadName);

    pthread_rwlock_init(&pagesLock,NULL);
    pthread_rwlock_init(&spottingsMapLock,NULL);

    string line;
    getline(in,line);
    assert(line.compare("CORPUS")==0);
    getline(in,name);
    getline(in,line);
    averageCharWidth = stof(line);
    getline(in,line);
    countCharWidth = stoi(line);
    getline(in,line);
    threshScoring = stof(line);

    minWordImageLen=99999;
    maxWordImageLen=0;

    getline(in,line);

    if (line[0]=='m')
    {
        getline(in,line);
        minWordImageLen = stoi(line);
        getline(in,line);
        maxWordImageLen = stoi(line);
        getline(in,line);
    }

    int pagesSize = stoi(line);
    for (int i=0; i<pagesSize; i++)
    {
        getline(in,line);
        int pageId = stoi(line);
        Page* page = new Page(in,&spotter,&averageCharWidth,&countCharWidth);
        pages[pageId]=page;
    }
    getline(in,line);
    pagesSize = stoi(line);
    for (int i=0; i<pagesSize; i++)
    {
        string s;
        getline(in,s);
        getline(in,line);
        int pageId = stoi(line);
        pageIdMap[s]=pageId;
    }
    getline(in,line);
    numWordsReadIn=stoi(line);
    recreateDatasetVectors(false);

    getline(in,line);
    int spottingsToWordsSize=stoi(line);
    for (int i=0; i<spottingsToWordsSize; i++)
    {
        getline(in,line);
        unsigned long sid=stoul(line);
        getline(in,line);
        int wordLen=stoi(line);
        for (int j=0; j<wordLen; j++)
        {
            getline(in,line);
            int spottingIndex=stoi(line);
            spottingsToWords[sid].push_back(_words.at(spottingIndex));
        }
    }
    CorpusRef* cr = getCorpusRef();
    manQueue.load(in,cr);
    delete cr;

    getline(in,ngramWWFile);
    getline(in,line);
    maxImageWidth=stoi(line);

    //in.close();
}

CorpusRef* Knowledge::Corpus::getCorpusRef()
{
    CorpusRef* ret = new CorpusRef();
    for (int i=0; i<_words.size(); i++)
    {
        ret->addWord(i,_words.at(i),_words.at(i)->getPage(),_words.at(i)->getSpottingsPointer());
        int x1,y1,x2,y2;
        bool toss;
        _words.at(i)->getBoundsAndDone(&x1,&y1,&x2,&y2,&toss);
        ret->addLoc(Location(_words.at(i)->getPageId(),x1,y1,x2,y2));
    }
    return ret;
}
PageRef* Knowledge::Corpus::getPageRef()
{
    PageRef* ret = new PageRef();
    for (auto p : pages)
    {
        ret->addPage(p.first,p.second->getImg());
    }

    //For debugging
    for (int i=0; i<_words.size(); i++)
    {
        int x1,y1,x2,y2;
        bool toss;
        _words.at(i)->getBoundsAndDone(&x1,&y1,&x2,&y2,&toss);
        ret->addWord(Location(_words.at(i)->getPageId(),x1,y1,x2,y2));
    }

    return ret;
}

void Knowledge::Page::save(ofstream& out)
{
    out<<"PAGE"<<endl;
    vector<Line*> ls = lines();
    out<<ls.size()<<"\n";
    out<<pageImgLoc<<"\n";
    for (Line* l : ls)
        l->save(out);
    out<<_id<<"\n";
    out<<id<<"\n";
}

Knowledge::Page::Page(ifstream& in, const Spotter* const* spotter, float* averageCharWidth, int* countCharWidth) : spotter(spotter), averageCharWidth(averageCharWidth), countCharWidth(countCharWidth)
{
    pthread_rwlock_init(&lock,NULL);
    string line;
    getline(in,line);
    assert(line.compare("PAGE")==0);
    getline(in,line);
    int sizeLines = stoi(line);
    getline(in,pageImgLoc);
#ifndef DONT_LOAD
    pageImg = cv::imread(pageImgLoc);
    assert(pageImg.cols*pageImg.rows > 1);
#endif
    _lines.resize(sizeLines);
    for (int i=0; i<sizeLines; i++)
    {
        _lines.at(i) = new Line(in,&pageImg,spotter,averageCharWidth,countCharWidth);
    }
    getline(in,line);
    _id=stoul(line);
    getline(in,line);
    id=stoul(line);
}

void Knowledge::Line::save(ofstream& out)
{
    out<<"LINE"<<endl;
    int cty, cby;
    vector<Word*> ws = wordsAndBounds(&cty,&cby);
    out<<ws.size()<<"\n";
    for (Word* w : ws)
    {
        w->save(out);
    }
    out<<cty<<"\n"<<cby<<"\n";
    out<<pageId<<"\n";
}

Knowledge::Line::Line(ifstream& in, const cv::Mat* pagePnt, const Spotter* const* spotter, float* averageCharWidth, int* countCharWidth) : pagePnt(pagePnt), spotter(spotter), averageCharWidth(averageCharWidth), countCharWidth(countCharWidth)
{
    pthread_rwlock_init(&lock,NULL);
    string line;
    getline(in,line);
    assert(line.compare("LINE")==0);
    getline(in,line);
    int wordSize = stoi(line);
    _words.resize(wordSize);
    for (int i=0; i<wordSize; i++)
    {
        _words.at(i)=new Word(in, pagePnt, spotter, averageCharWidth, countCharWidth);
    }
    getline(in,line);
    ty = stoi(line);
    getline(in,line);
    by = stoi(line);
    getline(in,line);
    pageId = stoi(line);
}

void Knowledge::Word::save(ofstream& out)
{
    out<<"WORD"<<endl;
    pthread_rwlock_rdlock(&lock);
    out<<id<<"\n";
    out<<tlx<<"\n"<<tly<<"\n"<<brx<<"\n"<<bry<<"\n";
    out<<query<<"\n"<<gt<<"\n";
    meta.save(out);
    out<<pageId<<"\n";//<<spottingIndex<<"\n";
    out<<topBaseline<<"\n"<<botBaseline<<"\n";
    out<<spottings.size()<<"\n";
    for (auto p : spottings)
    {
        out<<p.first<<"\n";
        p.second.save(out);
    }
    out<<done<<"\n"<<loose<<"\n";
    //?? out<<sentBatchId<<"\n";
    out<<harvested.size()<<"\n";
    for (auto p : harvested)
    {
        out<<p.first<<"\n";
        out<<p.second<<"\n";
    }
    out << transcription<<"\n";
    out<<removedSpottings.size()<<"\n";
    for (auto p : removedSpottings)
    {
        out<<p.first<<"\n";
        out<<p.second.size()<<"\n";
        for (Spotting& s : p.second)
        {
            s.save(out);
        }
    }
    pthread_rwlock_unlock(&lock);
}
Knowledge::Word::Word(ifstream& in, const cv::Mat* pagePnt, const Spotter* const* spotter, float* averageCharWidth, int* countCharWidth) : pagePnt(pagePnt), spotter(spotter), averageCharWidth(averageCharWidth), countCharWidth(countCharWidth), sentBatchId(0), _tlxBetter(-1), _brxBetter(-1)
{
    pthread_rwlock_init(&lock,NULL);
    string line;
    getline(in,line);
    assert(line.compare("WORD")==0);
    getline(in,line);
    id = stoi(line);
    getline(in,line);
    tlx = stoi(line);
    getline(in,line);
    tly = stoi(line);
    getline(in,line);
    brx = stoi(line);
    getline(in,line);
    bry = stoi(line);
    getline(in,query);
    getline(in,gt);
    meta = SearchMeta(in);
    getline(in,line);
    pageId = stoi(line);
    //getline(in,line);
    //spottingIndex = stoi(line);
    getline(in,line);
    topBaseline = stoi(line);
    getline(in,line);
    botBaseline = stoi(line);
    getline(in,line);
    int sSize = stoi(line);
    for (int i=0; i<sSize; i++)
    {
        getline(in,line);
        int x=stoi(line);
        spottings.insert(make_pair(x,Spotting(pagePnt,in)));
    }
    getline(in,line);
    done= stoi(line);
    getline(in,line);
    loose= stoi(line);
    //getline(in,line);
    //sentBatchId= stoul(line);
    getline(in,line);
    sSize= stoi(line);
    for (int i=0; i<sSize; i++)
    {
        getline(in,line);
        unsigned long sid=stoul(line);
        string ngram;
        getline(in,ngram);
        harvested.insert(make_pair(sid,ngram));
    }
    getline(in,transcription);
    getline(in,line);
    sSize=stoi(line);
    for (int i=0; i<sSize; i++)
    {
        getline(in,line);
        unsigned long sid=stoul(line);
        getline(in,line);
        int sSize2=stoi(line);
        removedSpottings[sid].resize(sSize2);
        for (int j=0; j<sSize2; j++)
        {
            removedSpottings.at(sid).at(j)=Spotting(pagePnt,in);
        }
    }
#ifdef NO_NAN
#ifndef DONT_LOAD
    //assert(gt.compare(GlobalK::knowledge()->getSegWord(id))==0);
#endif
#endif
}

//For data collection, when I deleted all my trans... :(
vector<TranscribeBatch*> Knowledge::Corpus::resetAllWords_()
{
    vector<TranscribeBatch*> ret;
    vector<Spotting*> newExemplars;
    for (auto p : _words)
    {
        Word* w = p.second;
        TranscribeBatch* b = w->reset_(&newExemplars);
        if (b!=NULL)
            ret.push_back(b);
    }
    return ret;
}

TranscribeBatch* Knowledge::Word::reset_(vector<Spotting*>* newExemplars)
{
    done=false;
    loose=false;
    transcription="";
    query="";
    return queryForBatch(newExemplars);
}

void meanStd(vector<int> vs, float* mean, float* std)
{
    *mean = 0;
    for (int v : vs)
        *mean+=v;
    *mean/=vs.size();


    *std=0;
    for (int v : vs)
        *std+=(*mean-v)*(*mean-v);
    *std=sqrt(*std/vs.size());
}

void Knowledge::Corpus::getStats(float* accTrans, float* pWordsTrans, float* pWords80_100, float* pWords60_80, float* pWords40_60, float* pWords20_40, float* pWords0_20, float* pWords0, float* pWordsBad, string* misTrans,
                                 float* accTrans_IV, float* pWordsTrans_IV, float* pWords80_100_IV, float* pWords60_80_IV, float* pWords40_60_IV, float* pWords20_40_IV, float* pWords0_20_IV, float* pWords0_IV, string* misTrans_IV,
                                 //additional
                                 float* wordsTrans80_100, float* wordsTrans60_80, float* wordsTrans40_60, float* wordsTrans20_40, float* wordsTrans0_20, float* wordsTrans0, float* wordsTransBad,
                                 float* meanLenPWordsTrans, float* meanLenPWords80_100, float* meanLenPWords60_80, float* meanLenPWords40_60, float* meanLenPWords20_40, float* meanLenPWords0_20, float* meanLenPWords0, float* meanLenPWordsBad,
                                 float* stdLenPWordsTrans, float* stdLenPWords80_100, float* stdLenPWords60_80, float* stdLenPWords40_60, float* stdLenPWords20_40, float* stdLenPWords0_20, float* stdLenPWords0, float* stdLenPWordsBad,
                                 float* meanLenWordsTrans80_100, float* meanLenWordsTrans60_80, float* meanLenWordsTrans40_60, float* meanLenWordsTrans20_40, float* meanLenWordsTrans0_20, float* meanLenWordsTrans0, float* meanLenWordsTransBad,
                                 float* stdLenWordsTrans80_100, float* stdLenWordsTrans60_80, float* stdLenWordsTrans40_60, float* stdLenWordsTrans20_40, float* stdLenWordsTrans0_20, float* stdLenWordsTrans0, float* stdLenWordsTransBad,
                                 map<char,float>* charDistDone, map<char,float>* charDistUndone,
                                 vector<string>* badNgrams,
                                 map<int,float>* meanUnigramsSpottedByLen, map<int,float>* meanBigramsSpottedByLen, map<int,float>* meanTrigramsSpottedByLen)
{
    int trueTrans, cTrans, c80_100, c60_80, c40_60, c20_40, c0_20, c0, cBad, cTrans80_100, cTrans60_80, cTrans40_60, cTrans20_40, cTrans0_20, cTrans0, cTransBad;
    trueTrans= cTrans= c80_100= c60_80= c40_60= c20_40= c0_20= c0= cBad= cTrans80_100= cTrans60_80= cTrans40_60= cTrans20_40= cTrans0_20= cTrans0= cTransBad=0;
    *misTrans="";
    vector<int> lensCTrans, lensC80_100, lensC60_80, lensC40_60, lensC20_40, lensC0_20, lensC0, lensCBad;
    vector<int> lensTransC80_100, lensTransC60_80, lensTransC40_60, lensTransC20_40, lensTransC0_20, lensTransC0, lensTransCBad;
    int numCharsDone=0;
    int numCharsUndone=0;
    map<int,vector<float> > unigramsSpottedByLen, bigramsSpottedByLen, trigramsSpottedByLen;
    bool more = meanTrigramsSpottedByLen!=NULL && wordsTrans80_100!=NULL;

    //IV is In-Vocabulary
    int trueTrans_IV, cTrans_IV, c80_100_IV, c60_80_IV, c40_60_IV, c20_40_IV, c0_20_IV, c0_IV;
    trueTrans_IV= cTrans_IV= c80_100_IV= c60_80_IV= c40_60_IV= c20_40_IV= c0_20_IV= c0_IV=0;
    *misTrans_IV="";
    
    int numIV=0;
    for (auto p : _words)
    {
        Word* w = p.second;
        bool done;
        string gt, query;
        w->getDoneAndGTAndQuery(&done,&gt,&query);
        for (int i=0; i<gt.length(); i++)
            gt[i]=tolower(gt[i]);
        bool inVocab = Lexicon::instance()->inVocab(gt);
        if (inVocab)
            numIV++;
        if (more && !done)
        {
            for (int i=0; i<gt.length(); i++)
                (*charDistUndone)[gt[i]]+=1;
            numCharsUndone+=gt.length();
        }
        if (done)
        {
            cTrans++;
            if (inVocab)
                cTrans_IV++;
            string trans = w->getTranscription();
            for (int i=0; i<trans.length(); i++)
                trans[i] = tolower(trans[i]);
            if (gt.compare(trans)==0)
                trueTrans++;
            else
            {
                *misTrans+=trans+"("+gt+") ";
                if (inVocab)
                    *misTrans_IV+=trans+"("+gt+") ";
            }

            if (more)
            {
                for (int i=0; i<gt.length(); i++)
                    (*charDistDone)[gt[i]]+=1;
                numCharsDone+=gt.length();
                lensCTrans.push_back(gt.length());
                bool bad=false;
                int numMatch=0;
                int posGT=0;
                bool skip=false;
                char last='.';
                for (int i=0; i<query.length(); i++)
                {
                    if (skip)
                    {
                        if (query[i]==']')
                            skip=false;
                    }
                    else
                    {
                        if (query[i]=='[')
                        {
                            skip=true;
                            last='.';
                        }
                        else if (query[i]>='a' && query[i]<='z')
                        {
                            if (query[i]==last)
                            {
                                if (query[i]==gt[posGT])
                                {
                                    posGT++;
                                    numMatch++;
                                }
                            }
                            else
                            {
                                bool match=false;
                                for (;posGT<gt.length(); posGT++)
                                    if (gt[posGT]==query[i])
                                    {
                                        match=true;
                                        numMatch++;
                                        posGT++;
                                        break;
                                    }
                                if (!match)
                                {
                                    bad=true;
                                    //cout<<"["<<w->getId()<<"]BadT: "<<gt<<", q: "<<query<<endl;
                                    if (badNgrams!=NULL)
                                        badNgrams->push_back("["+to_string(w->getId())+"]BadT: "+gt+", q: "+query);
                                }
                            }
                            last=query[i];
                        }
                    }
                }
                float p = numMatch/(0.0+gt.length());
                if (bad)
                {
                    cTransBad++;
                    lensTransCBad.push_back(gt.length());
                }
                else if (p>.8)
                {
                    cTrans80_100++;
                    lensTransC80_100.push_back(gt.length());
                }
                else if (p>.6)
                {
                    cTrans60_80++;
                    lensTransC60_80.push_back(gt.length());
                }
                else if (p>.4)
                {
                    cTrans40_60++;
                    lensTransC40_60.push_back(gt.length());
                }
                else if (p>.2)
                {
                    cTrans20_40++;
                    lensTransC20_40.push_back(gt.length());
                }
                else if (p>0)
                {
                    cTrans0_20++;
                    lensTransC0_20.push_back(gt.length());
                }
                else
                {
                    cTrans0++;
                    lensTransC0.push_back(gt.length());
                }
            }
        }
        else if (query.length()==0)
        {
            c0++;
            if (inVocab)
                c0_IV++;
            if (more)
                lensC0.push_back(gt.length());
        }
        else
        {
            bool bad=false;
            int numMatch=0;
            int posGT=0;
            bool skip=false;
            char last='.';
            for (int i=0; i<query.length(); i++)
            {
                if (skip)
                {
                    if (query[i]==']')
                        skip=false;
                }
                else
                {
                    if (query[i]=='[')
                    {
                        skip=true;
                        last='.';
                    }
                    else if (query[i]>='a' && query[i]<='z')
                    {
                        if (query[i]==last && posGT<gt.length())
                        {
                            if (query[i]==gt[posGT])
                            {
                                posGT++;
                                numMatch++;
                            }
                        }
                        else
                        {
                            bool match=false;
                            for (;posGT<gt.length(); posGT++)
                                if (gt[posGT]==query[i])
                                {
                                    match=true;
                                    numMatch++;
                                    posGT++;
                                    break;
                                }
                            if (!match)
                            {
                                bad=true;
                                //cout<<"["<<w->getId()<<"]Bad1: "<<gt<<", q: "<<query<<endl;
                                if (badNgrams!=NULL)
                                    badNgrams->push_back("["+to_string(w->getId())+"]Bad1: "+gt+", q: "+query);
                            }
                        }
                        last=query[i];
                    }
                }
                //if (i<query.length()-1 && posGT>=gt.length())
                //{
                //    bad=true;
                //    cout<<"["<<w->getId()<<"]Bad2: "<<gt<<", q: "<<query<<endl;
                //}
            }
            float p = numMatch/(0.0+gt.length());
            if (bad)
            {
                cBad++;
                if (more)
                    lensCBad.push_back(gt.length());
            }
            else if (p>.8)
            {
                c80_100++;
                if (inVocab)
                    c80_100_IV++;
                if (more)
                    lensC80_100.push_back(gt.length());
            }
            else if (p>.6)
            {
                c60_80++;
                if (inVocab)
                    c60_80_IV++;
                if (more)
                    lensC60_80.push_back(gt.length());
            }
            else if (p>.4)
            {
                c40_60++;
                if (inVocab)
                    c40_60_IV++;
                if (more)
                    lensC40_60.push_back(gt.length());
            }
            else if (p>.2)
            {
                c20_40++;
                if (inVocab)
                    c20_40_IV++;
                if (more)
                    lensC20_40.push_back(gt.length());
            }
            else if (p>0)
            {
                c0_20++;
                if (inVocab)
                    c0_20_IV++;
                if (more)
                    lensC0_20.push_back(gt.length());
            }
            else
            {
                c0++;
                if (inVocab)
                    c0_IV++;
                if (more)
                    lensC0.push_back(gt.length());
            }
        }

        if (more)
        {
            int numPossibleUnis=0;
            for (int ii=0; ii<gt.length(); ii++)
            {
                if (find(GlobalK::knowledge()->unigrams.begin(),GlobalK::knowledge()->unigrams.end(),gt.substr(ii,1))!=GlobalK::knowledge()->unigrams.end())
                    numPossibleUnis++;
            }
            int numPossibleBis=0;
            for (int ii=0; ii<((int)gt.length())-1; ii++)
            {
                if (find(GlobalK::knowledge()->bigrams.begin(),GlobalK::knowledge()->bigrams.end(),gt.substr(ii,2))!=GlobalK::knowledge()->bigrams.end())
                    numPossibleBis++;
            }
            int numPossibleTris=0;
            for (int ii=0; ii<((int)gt.length())-2; ii++)
            {
                if (find(GlobalK::knowledge()->trigrams.begin(),GlobalK::knowledge()->trigrams.end(),gt.substr(ii,3))!=GlobalK::knowledge()->trigrams.end())
                    numPossibleTris++;
            }
            int numUnis=0;
            int numBis=0;
            int numTris=0;
            for (auto spotting : w->getSpottings())
            {
                int l = spotting.ngram.length();
                switch (l)
                {
                    case 1:
                        numUnis++;
                        break;
                    case 2:
                        numBis++;
                        break;
                    case 3:
                        numTris++;
                        break;
                }
            }
            if (numPossibleUnis>0)
                unigramsSpottedByLen[gt.length()].push_back(numUnis/(0.0+numPossibleUnis));
            if (numPossibleBis>0)
                bigramsSpottedByLen[gt.length()].push_back(numBis/(0.0+numPossibleBis));
            if (numPossibleTris>0)
                trigramsSpottedByLen[gt.length()].push_back(numTris/(0.0+numPossibleTris));
                    
        }
    }
    if (cTrans>0)
        *accTrans= trueTrans/(0.0+cTrans);
    else
        *accTrans=0;
    *pWordsTrans= cTrans/(0.0+_words.size());
    *pWords80_100= c80_100/(0.0+_words.size());
    *pWords60_80= c60_80/(0.0+_words.size());
    *pWords40_60= c40_60/(0.0+_words.size());
    *pWords20_40= c20_40/(0.0+_words.size());
    *pWords0_20= c0_20/(0.0+_words.size());
    *pWords0= c0/(0.0+_words.size());
    *pWordsBad= cBad/(0.0+_words.size());

    if (cTrans_IV>0)
        *accTrans_IV= trueTrans_IV/(0.0+cTrans_IV);
    else
        *accTrans_IV=0;
    *pWordsTrans_IV= cTrans_IV/(0.0+numIV);
    *pWords80_100_IV= c80_100_IV/(0.0+numIV);
    *pWords60_80_IV= c60_80_IV/(0.0+numIV);
    *pWords40_60_IV= c40_60_IV/(0.0+numIV);
    *pWords20_40_IV= c20_40_IV/(0.0+numIV);
    *pWords0_20_IV= c0_20_IV/(0.0+numIV);
    *pWords0_IV= c0_IV/(0.0+numIV);

    if (more)
    {
        *wordsTrans80_100= cTrans80_100/(0.0+cTrans);
        *wordsTrans60_80= cTrans60_80/(0.0+cTrans);
        *wordsTrans40_60= cTrans40_60/(0.0+cTrans);
        *wordsTrans20_40= cTrans20_40/(0.0+cTrans);
        *wordsTrans0_20= cTrans0_20/(0.0+cTrans);
        *wordsTrans0= cTrans0/(0.0+cTrans);
        *wordsTransBad= cTransBad/(0.0+cTrans);

        meanStd(lensCTrans ,meanLenPWordsTrans ,stdLenPWordsTrans );
        meanStd(lensC80_100,meanLenPWords80_100,stdLenPWords80_100);
        meanStd(lensC60_80 ,meanLenPWords60_80 ,stdLenPWords60_80 );
        meanStd(lensC40_60 ,meanLenPWords40_60 ,stdLenPWords40_60 );
        meanStd(lensC20_40 ,meanLenPWords20_40 ,stdLenPWords20_40 );
        meanStd(lensC0_20  ,meanLenPWords0_20  ,stdLenPWords0_20  );
        meanStd(lensC0     ,meanLenPWords0     ,stdLenPWords0     );
        meanStd(lensCBad   ,meanLenPWordsBad   ,stdLenPWordsBad   );

        meanStd(lensTransC80_100,meanLenWordsTrans80_100,stdLenWordsTrans80_100);
        meanStd(lensTransC60_80 ,meanLenWordsTrans60_80 ,stdLenWordsTrans60_80 );
        meanStd(lensTransC40_60 ,meanLenWordsTrans40_60 ,stdLenWordsTrans40_60 );
        meanStd(lensTransC20_40 ,meanLenWordsTrans20_40 ,stdLenWordsTrans20_40 );
        meanStd(lensTransC0_20  ,meanLenWordsTrans0_20  ,stdLenWordsTrans0_20  );
        meanStd(lensTransC0     ,meanLenWordsTrans0     ,stdLenWordsTrans0     );
        meanStd(lensTransCBad   ,meanLenWordsTransBad   ,stdLenWordsTransBad   );

        for (auto& p : *charDistDone)
            p.second/=numCharsDone;
        for (auto& p : *charDistUndone)
            p.second/=numCharsUndone;

        for (auto p : unigramsSpottedByLen)
        {
            float sum = 0;
            for (float v : p.second)
                sum+=v;
            assert(sum==sum);
            assert(sum==0 || p.second.size()>0);
            //*(meanUnigramsSpottedByLen)[p.first] = sum/p.second.size();
            meanUnigramsSpottedByLen->emplace(p.first,sum/p.second.size());
        }
        for (auto p : bigramsSpottedByLen)
        {
            float sum = 0;
            for (float v : p.second)
                sum+=v;
            assert(sum==sum);
            assert(sum==0 || p.second.size()>0);
            //*(meanBigramsSpottedByLen)[p.first] = sum/p.second.size();
            meanBigramsSpottedByLen->emplace(p.first,sum/p.second.size());
        }
        for (auto p : trigramsSpottedByLen)
        {
            float sum = 0;
            for (float v : p.second)
                sum+=v;
            //*(meanTrigramsSpottedByLen)[p.first] = sum/p.second.size();
            meanTrigramsSpottedByLen->emplace(p.first,sum/p.second.size());
        }
    }
}

vector<SpottingLoc> Knowledge::Corpus::massSpot(const vector<string>& ngrams, Mat& crossScores)
{
    return spotter->massSpot(ngrams,crossScores);
}

