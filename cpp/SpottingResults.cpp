#include "SpottingResults.h"

#include <ctime>
#include <random>


atomic_ulong Spotting::_id;
atomic_ulong SpottingResults::_id;

SpottingResults::SpottingResults(string ngram, int contextPad) : 
    //instancesByScore(scoreCompById(&(this->instancesById),false)),
    ngram(ngram), contextPad(contextPad)
{
    //id = ++_id;
    if (ngram.length()==1)
    {
        auto begin = GlobalK::knowledge()->unigrams.begin();
        auto end = GlobalK::knowledge()->unigrams.end();
        auto found = find(begin,end,ngram);
        if (found!=end)
            id = GlobalK::knowledge()->trigrams.size() + GlobalK::knowledge()->bigrams.size() + distance(begin,found);
        else
            id = ++_id;
    }
    else if (ngram.length()==2)
    {
        auto begin = GlobalK::knowledge()->bigrams.begin();
        auto end = GlobalK::knowledge()->bigrams.end();
        auto found = find(begin,end,ngram);
        if (found!=end)
            id = GlobalK::knowledge()->trigrams.size() + distance(begin,found);
        else
            id = ++_id;
    }
    else if (ngram.length()==3)
    {
        auto begin = GlobalK::knowledge()->trigrams.begin();
        auto end = GlobalK::knowledge()->trigrams.end();
        auto found = find(begin,end,ngram);
        if (found!=end)
            id = distance(begin,found);
        else
            id = ++_id;
    }
    //sem_init(&mutexSem,false,1);
    //numBatches=0;
    allBatchesSent=true;
    
    numberClassifiedTrue=0;
    numberClassifiedFalse=0;
    numberAccepted=0;
    numberRejected=0;
    maxScoreQbE=-999999;
    minScoreQbE=999999;
    maxScoreQbS=-999999;
    minScoreQbS=999999;
    
    useQbE=false;
    
    trueMean=minScore();//How to initailize?
    trueVariance=1.0;
    falseMean=maxScore();
    falseVariance=1.0;
    
    acceptThreshold=-1;
    rejectThreshold=-1;
    numLeftInRange=-1;

    distCheckCounter=0;
    
    done=false;

    numComb=0;

    //pullFromScore=splitThreshold;
    momentum=1.2;
    lastDifPullFromScore=0;
    batchesSinceChange=0;

    badInARow=0;

    numSpottingsQbEMax=-1;

#ifdef GRAPH_SPOTTING_RESULTS
    undoneGraphName="save/graph/graph_undone_"+ngram+".png";
    fullGraphName="save/graph/graph_full_"+ngram+".png";
    //cv::namedWindow(undoneGraphName);
#endif
#ifdef TEST_MODE
    histFile = "save/graph/hist_"+ngram+".csv";
    rtn=0;
    atn=0;
#endif //TEST_MODE
}

void SpottingResults::add(Spotting spotting) {
    assert(spotting.tlx!=-1);
    assert(!useQbE);
    //sem_wait(&mutexSem);
    allBatchesSent=false;
    instancesById[spotting.id]=spotting;
    assert(spotting.ngram.compare(ngram)==0);
    assert(spotting.score(useQbE)==spotting.score(useQbE) && spotting.pageId<100000);//currption checking
    if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
    {
        classById[spotting.id]=true;
        numberClassifiedTrue++;
#ifdef NO_NAN
        //GlobalK::knowledge()->accepted();
#endif
    }
    else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
    {
        classById[spotting.id]=false;
        numberClassifiedFalse++;
#ifdef NO_NAN
        //GlobalK::knowledge()->rejected();
#endif
    }
    else
    {
        instancesByScoreInsert(spotting.id);
    }
    instancesByLocation.insert(spotting);
    tracer = instancesByScore.begin();
    if (spotting.scoreQbS>maxScore())
    {
        if (falseMean==maxScore() && falseVariance==1.0)
            falseMean=spotting.scoreQbS;
        maxScoreQbS=spotting.scoreQbS;
    }
    if (spotting.scoreQbS<minScore())
    {
        if (trueMean==minScore() && trueVariance==1.0)
            trueMean=spotting.scoreQbS;
        minScoreQbS=spotting.scoreQbS;
    }
    //sem_post(&mutexSem);
    batchesSinceChange=0;
}
void SpottingResults::addTrueNoScore(const SpottingExemplar& spotting) {
    assert(spotting.tlx!=-1);
    //sem_wait(&mutexSem);
    instancesById[spotting.id]=spotting;
    instancesById.at(spotting.id).type=SPOTTING_TYPE_APPROVED;
    instancesByLocation.insert(spotting);
    classById[spotting.id]=true;
    //sem_post(&mutexSem);
}
/*SpottingsBatch* SpottingResults::getBatch(bool* done, unsigned int num, unsigned int maxWidth) {
    cout <<"getBatch, from:"<<pullFromScore<<endl;
    if (acceptThreshold==-1 && rejectThreshold==-1)
        EMThresholds(true);
    SpottingsBatch* ret = new SpottingsBatch(ngram,id);
    //sem_wait(&mutexSem);
    unsigned int toRet = ((((signed int)instancesByScore.size())-(signed int) num)>3)?num:instancesByScore.size();
    
    for (unsigned int i=0; i<toRet && !*done; i++) {
        SpottingImage tmp = getNextSpottingImage(done, maxWidth);
        ret->push_back(tmp);
        if (classById.size()<20)
            *done=false;
    }
    if (*done)
        allBatchesSent=true;
    //sem_post(&mutexSem);
    numBatches++;
    cout <<"["<<id<<"] sent batch of size "<<ret->size()<<", have "<<instancesByScore.size()<<" left"<<endl;
    cout <<"score: ";
    for (int i=0; i<ret->size(); i++)
    {
        cout<<ret->at(i).score<<"\t";
    }
    cout<<endl;
    return ret;
}*/

void SpottingResults::debugState() const
{
    //if (ngram.compare("th")==0)
    //    cout<<"[th] byId: "<<instancesById.size()<<"  byScore: "<<instancesByScore.size()<<" numLeftInRange: "<<numLeftInRange<<endl;

    
    //for (auto iter=instancesByLocation.begin(); iter!=instancesByLocation.end(); iter++)
    //{
    //    assert(instancesById.find(iter->id) != instancesById.end());
    //}
    //set<unsigned long> check;
    //for (auto iter=instancesByScore.begin(); iter!=instancesByScore.end(); iter++)
    //{
    //    auto iter2=instancesById.find(iter->second);
    //    assert(iter2 != instancesById.end());
    //    assert(check.find(iter->second) == check.end());
    //    check.insert(iter->second);
    //}
    //check.clear();
    //for (auto iter=allInstancesByScoreQbE.begin(); iter!=allInstancesByScoreQbE.end(); iter++)
    //{
    //    auto iter2=instancesById.find(iter->second);
    //    assert(iter2 != instancesById.end());
    //    assert(iter->first == iter2->second.scoreQbE);
    //    assert(check.find(iter->second) == check.end());
    //    check.insert(iter->second);
    //}

    //float maxS=-999999;
    //float minS=999999;
    for (auto p : instancesById)
    {
        assert(p.second.brx-p.second.tlx>0 &&p.second.bry-p.second.tly>0);
        //if (p.second.score(useQbE)==p.second.score(useQbE) && p.second.score(useQbE)!=MAX_FLOAT)
        //{
        //    if (p.second.score(useQbE)>maxS)
        //        maxS=p.second.score(useQbE);
        //    if (p.second.score(useQbE)<minS)
        //        minS=p.second.score(useQbE);
        //}
        //if (classById.find(p.first) == classById.end())
        //    assert(instancesByScoreContains(p.first) ||
        //           starts.find(p.first) != starts.end());

        //if (p.second.scoreQbE==p.second.scoreQbE && p.second.scoreQbE!=MAX_FLOAT)
        //{
        //    auto range = allInstancesByScoreQbE.equal_range(p.second.scoreQbE);
        //    assert(range.first!=range.second);
        //}

        unsigned long id = p.first;
        while (updateMap.find(id)!=updateMap.end())
        {
            //unsigned long oldId=id;
            id=updateMap.at(id);
            //instancesById.erase(oldId);
        }
        if (classById.find(p.first) == classById.end())
        {
            auto range = instancesByScore.equal_range(p.second.score(useQbE));
            bool present=false;
            for (auto iter=range.first; iter!=range.second; iter++)
                if (iter->second == p.first)
                {
                    present=true;
                    break;
                }
            if (!present)
                for (auto p : starts)
                {
                    unsigned long id = p.first;
                    while (updateMap.find(id)!=updateMap.end())
                    {
                        id=updateMap.at(id);
                    }
                    if (id==p.first)
                    {
                        present=true;
                        break;
                    }
                }


            assert(present || starts.find(p.first)!=starts.end());
        }

    }
    //assert(maxS==maxScore() && minS==minScore());
}

#ifdef TEST_MODE
int SpottingResults::setDebugInfo(SpottingsBatch* b)
{
    double precAtPull=0;
    int countAtPull=0;
    bool atPull=true;
    double precAcceptT=0;
    int countAcceptT=0;
    bool acceptT=true;
    double precRejectT=0;
    int countRejectT=0;
    bool rejectT=false;
    double precBetweenT=0;
    int countBetweenT=0;
    bool betweenT=false;

    int tBetBefore=0;
    int fBetBefore=0;
    int tBetAfter=0;
    int fBetAfter=0;
#ifdef GRAPH_SPOTTING_RESULTS
    /*
    if (undoneGraph.cols==0)
        undoneGraph = cv::Mat::zeros(1,instancesByScore.size()+3,CV_8UC3);
    if (undoneGraph.cols<instancesByScore.size()+3)
    {
        int dif = instancesByScore.size()+3-undoneGraph.cols;
        cv::hconcat(undoneGraph,cv::Mat::zeros(undoneGraph.rows,dif,CV_8UC3),undoneGraph);
    }

    cv::Mat newUndoneLine = cv::Mat::zeros(2,undoneGraph.cols,CV_8UC3);//the line we're building now
    int inUndone=0;*/
#endif
    for (auto iter=instancesByLocation.begin(); iter!=instancesByLocation.end(); iter++)
    {
        assert(instancesById.find(iter->id) != instancesById.end());
    }
    for (auto iter=instancesByScore.begin(); iter!=instancesByScore.end(); iter++)
    {
        assert(instancesById.find(iter->second) != instancesById.end());
        Spotting& spotting = instancesById.at(iter->second);
        float score=spotting.score(useQbE);
        int t=0;
        if (spotting.gt==1)
            t=1;
        else if (spotting.gt!=0)
        {
            t = GlobalK::knowledge()->ngramAt(ngram, spotting.pageId, spotting.tlx, spotting.tly, spotting.brx, spotting.bry);
            spotting.gt=t;
        }

        if (score >=pullFromScore && atPull)
        {
            atPull=false;
#ifdef GRAPH_SPOTTING_RESULTS

            //newUndoneLine.at<cv::Vec3b>(0,inUndone++) = cv::Vec3b(255,200,200);
#endif
        }
        if (score >=acceptThreshold && acceptT)
        {
            acceptT=false;
            betweenT=true;
#ifdef GRAPH_SPOTTING_RESULTS
            /*if (atn==0)
                newUndoneLine.at<cv::Vec3b>(0,inUndone++) = cv::Vec3b(255,0,0);
            else if (atn==1)
                newUndoneLine.at<cv::Vec3b>(0,inUndone++) = cv::Vec3b(255,200,0);
            else if (atn==2)
                newUndoneLine.at<cv::Vec3b>(0,inUndone++) = cv::Vec3b(255,0,200);
                */
#endif
        }
        if (score >rejectThreshold && betweenT)
        {
            rejectT=true;
            betweenT=false;
        }

        if (atPull)
        {
            countAtPull++;
            precAtPull+=t==1?1:0;
        }
        if (acceptT)
        {
            countAcceptT++;
            precAcceptT+=t==1?1:0;
        }
        if (rejectT)
        {
            countRejectT++;
            precRejectT+=t==1?1:0;
        }
        if (betweenT)
        {
            countBetweenT++;
            precBetweenT+=t==1?1:0;
        }
        if (!acceptT && atPull)
        {
            if (t==1)
                tBetBefore++;
            else if (t==0)
                fBetBefore++;
        }

        if (!rejectT && !atPull)
        {
            if (t==1)
                tBetAfter++;
            else if (t==0)
                fBetAfter++;
        }
#ifdef GRAPH_SPOTTING_RESULTS
        /*if (t)
            newUndoneLine.at<cv::Vec3b>(0,inUndone++) = cv::Vec3b(0,205,0);
        else
            newUndoneLine.at<cv::Vec3b>(0,inUndone++) = cv::Vec3b(0,0,205);
            */
#endif
    }
    cout<<"["<<ngram<<"] serving batch."<<endl;
    cout<<"("<<precAcceptT/countAcceptT<<", "<<(int)precAcceptT<<", "<<(int)(countAcceptT-precAcceptT)<<") A ";
    cout<<"("<<(0.0+tBetBefore)/(tBetBefore+fBetBefore)<<", "<<tBetBefore<<", "<<fBetBefore<<") P ";
    cout<<"("<<(0.0+tBetAfter)/(tBetAfter+fBetAfter)<<", "<<tBetAfter<<", "<<fBetAfter<<") R ";
    cout<<"("<<precRejectT/countRejectT<<", "<<(int)precRejectT<<", "<<(int)(countRejectT-precRejectT)<<")"<<endl;;

#ifdef GRAPH_SPOTTING_RESULTS
    /*undoneGraph.push_back(newUndoneLine);
    //if (undoneGraph.rows-1%4==0)
        cv::imwrite(undoneGraphName,undoneGraph);
    //cv::imshow(undoneGraphName,undoneGraph);
    //cv::waitKey(100);
    */
    atPull=true;
    acceptT=true;
    rejectT=false;
    betweenT=false;
    if (fullInstancesByScore.size() < instancesById.size())
    {
        fullInstancesByScore.clear();
        for (const auto& p : instancesById)
        {
            assert(p.second.brx-p.second.tlx>0 && p.second.bry-p.second.tly>0);
            fullInstancesByScore.emplace(p.second.score(useQbE),p.first);
        }
    }

    if (fullGraph.cols==0)
        fullGraph = cv::Mat::zeros(1,fullInstancesByScore.size()+3,CV_8UC3);
    if (fullGraph.cols<fullInstancesByScore.size()+3)
    {
        int dif = fullInstancesByScore.size()+3-fullGraph.cols;
        cv::hconcat(fullGraph,cv::Mat::zeros(fullGraph.rows,dif,CV_8UC3),fullGraph);
    }
    cv::Mat newFullLine = cv::Mat::zeros(2,fullGraph.cols,CV_8UC3);
    int inFull=0;
    for (auto iter=fullInstancesByScore.begin(); iter!=fullInstancesByScore.end(); iter++)
    {
        float score = iter->first;
        Spotting* spotting = &(instancesById.at(iter->second));
        bool t=false;
        if (spotting->gt==1)
            t=true;
        else if (spotting->gt!=0)
        {
            t = GlobalK::knowledge()->ngramAt(ngram, spotting->pageId, spotting->tlx, spotting->tly, spotting->brx, spotting->bry);
            spotting->gt=t;
        }

        int color=205;
        if (!instancesByScoreContains(spotting->id))
            color = 100;

        if (score >=pullFromScore && atPull)
        {
            atPull=false;
            newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(255,200,190);
        }
        if (score >=acceptThreshold && acceptT)
        {
            acceptT=false;
            betweenT=true;
            if (atn==0)
                newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(255,0,0);
            else if (atn==1)
                newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(255,200,0);
            else if (atn==2)
                newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(255,0,200);
        }
        if (score >rejectThreshold && betweenT)
        {
            rejectT=true;
            betweenT=false;
            if (rtn==0)
                newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(255,0,0);
            else if (rtn==1)
                newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(255,200,0);
            else if (rtn==2)
                newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(255,0,200);
        }
        if (t)
            newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(0,color,0);
        else
            newFullLine.at<cv::Vec3b>(0,inFull++) = cv::Vec3b(0,0,color);
    } 
    fullGraph.push_back(newFullLine);
    //if (fullGraph.rows-1%4==0)
        cv::imwrite(fullGraphName,fullGraph);
    cout<<"wrote "<<fullGraphName<<endl;
    //cv::imshow(fullGraphName,fullGraph);
    //cv::waitKey(100);
#endif

    precAtPull/=countAtPull;
    precAcceptT/=countAcceptT;
    precRejectT/=countRejectT;
    precBetweenT/=countBetweenT;

    if (b!=NULL)
        b->addDebugInfo(precAtPull,precAcceptT,precRejectT,precBetweenT, countAcceptT, countRejectT, countBetweenT);
    //cout<<"precAtPull: "<<precAtPull<<"\t
    return 1;
}


void SpottingResults::saveHistogram(float actualModelDif)
{
    int numBuckets=50;
    vector<int> bucketsT(numBuckets);
    vector<int> bucketsF(numBuckets);
    int count=0;
    for (auto& p : instancesById)
    {
        float score = p.second.score(useQbE);
        if (score!=score || score==MAX_FLOAT)
            continue;
        count++;
        if (useQbE)
        {
            auto range = allInstancesByScoreQbE.equal_range(score);
            assert(range.first != range.second);
        }

        int bucket = (numBuckets-1)*(score-minScore())/(maxScore()-minScore());
        assert(bucket>=0 && bucket<numBuckets);
        bool t=false;
        if (p.second.gt==1)
            t=true;
        else if (p.second.gt!=0)
        {
            t = GlobalK::knowledge()->ngramAt(ngram, p.second.pageId, p.second.tlx, p.second.tly, p.second.brx, p.second.bry);
            p.second.gt=t;
        }

        if (t)
            bucketsT[bucket]++;
        else
            bucketsF[bucket]++;
    }
    if (useQbE)
        assert(numSpottingsQbEMax>=count);
    ofstream file (histFile,ofstream::app);
    file<<ngram<<" "<<(useQbE?"QbE ":"QbS ")<<numComb<<"   actualModelDif:"<<actualModelDif<<"   tailEndScoreInit("<<tailEnd<<"):"<<tailEndScore<<"   trueFalseDivide:"<<trueFalseDivide<<"   accept:"<<acceptThreshold<<"   reject:"<<rejectThreshold<<endl;
    file<<"Score, ";
    vector<float> binScores(numBuckets);
    float binSize = (maxScore()-minScore())/numBuckets;
    for (int i=0; i<numBuckets; i++)
    {
        float score = minScore()+((i+0.5)*binSize);
        file<<score<<", ";
        binScores[i]=score;
    }
    file<<"\nTrue, ";
    int sumTrue=0;
    for (int n : bucketsT)
    {
        sumTrue+=n;
        file<<n<<", ";
    }
    file<<"\nFalse, ";
    int sumFalse=0;
    for (int n : bucketsF)
    {
        sumFalse+=n;
        file<<n<<", ";
    }

    
    //vector<float> binScores(numBuckets+1);
    //for (int bin=0; bin<numBuckets+1; bin++)
    //{
    //    float v = minScore() + ((bin+0.0)/numBuckets)*(maxScore()-minScore());
    //    binScores[bin]=v;
    //}
    

    //Scaling not right :(
    file<<"\nInitail, ";
    for (int bin=0; bin<numBuckets; bin++)
    {
        float scale = binSize*sumFalse*2.0; ///(useQbE?-1.0:2.0);
        file<<(int)(scale*NPD(binScores[bin],usedMean,usedStd*usedStd))<<", ";
    }
    file<<"\nest True, ";
    for (int bin=0; bin<numBuckets; bin++)
    { 
        float scale = binSize*sumTrue*2.0; ///(useQbE?-1.0:2.0);
        file<<(int)(scale*NPD(binScores[bin],trueMean,trueVariance))<<", ";
    }
    file<<"\nest False, ";
    for (int bin=0; bin<numBuckets; bin++)
    {
        float scale = binSize*sumFalse*2.0; ///(useQbE?-1.0:2.0);
        file<<(int)(scale*NPD(binScores[bin],falseMean,falseVariance))<<", ";
    }
    file<<"\n"<<endl;
    file.close();
}
#endif

SpottingsBatch* SpottingResults::getBatch(int* done, unsigned int num, bool hard, unsigned int maxWidth, int color, string prevNgram, bool need) {
    //debugState();

    if (!need && (numLeftInRange<12 && numLeftInRange>=0) && starts.size()>1 && !takeFromTail)
#ifndef NO_NAN
        return NULL;
#else
    {}
#endif

    if (acceptThreshold==-1 && rejectThreshold==-1)
        EMThresholds();
    //sem_wait(&mutexSem);
    
    unsigned int toRet = ((hard&&instancesByScore.size()>=num)||((((signed int)instancesByScore.size())-(signed int) num)>3))?num:instancesByScore.size();
    if (toRet==0)
    {
        //This occurs in a race condition when the spotting results is queried before it can be removed from the MasterQueue
        return NULL;
    }
    SpottingsBatch* ret = new SpottingsBatch(ngram,id);
#ifdef TEST_MODE
    //setDebugInfo(ret);
#endif
    set<float> scoresToDraw;
    if (GlobalK::knowledge()->SR_TAKE_FROM_TOP)
    {
        auto iterScore = instancesByScore.begin();
        for (int i=0; i<min(toRet,(unsigned int)instancesByScore.size()); i++)
        {
            scoresToDraw.insert(iterScore->first);
            iterScore++;
        }
    }
    else if (GlobalK::knowledge()->SR_GAUSSIAN_DRAW)
    {
        normal_distribution<float> distribution(pullFromScore, takeStd);
        for (int i=0; i<min(toRet,(unsigned int)instancesByScore.size()); i++)
        {
            float v = distribution(generator);
            if (v<=acceptThreshold)
                v=2*acceptThreshold-v;
            if (v>=rejectThreshold)
                v=2*rejectThreshold-v;
            scoresToDraw.insert(v);
        }
    }
    else
    {
        uniform_real_distribution<float> distribution(acceptThreshold,rejectThreshold);
        for (int i=0; i<min(toRet,(unsigned int)instancesByScore.size()); i++)
        {
            float v = distribution(generator);
            if (v<=acceptThreshold)
                v=2*acceptThreshold-v;
            if (v>=rejectThreshold)
                v=2*rejectThreshold-v;
            scoresToDraw.insert(v);
        }
    }
#ifdef TEST_MODE
    cout <<"\n["<<ngram<<"] getBatch, from: ";
    for (float v : scoresToDraw)
        cout<<v<<", ";
    cout<<endl;
#endif

    /*auto iterR = instancesByScore.end();
    do
    {
        iterR--;
        assert(instancesById.find(iterR->second) != instancesById.end());
    } while(iterR!=instancesByScore.begin() && instancesById.at(iterR->second).score(useQbE) >rejectThreshold);
    for (float drawScore : scoresToDraw)
    {
        auto iter=iterR;
        assert(instancesById.find(iter->second) != instancesById.end());
        while (instancesById.at(iter->second).score(useQbE) >drawScore && iter!=instancesByScore.begin())
        {
            iter--;
        }
        assert(instancesById.find(iter->second) != instancesById.end());
        if (instancesById.at(iter->second).score(useQbE)<acceptThreshold && iter!=instancesByScore.end())
        {
            iter++;
            if (iter==instancesByScore.end())
                iter--;
        }
        if (iter==iterR)
        {
            if (iterR!=instancesByScore.begin())
            {
                iterR--;
            }
            else
            {
                iterR++;
            }
        }
    
        assert (updateMap.find(iter->second)==updateMap.end());
        assert(instancesById.find(iter->second) != instancesById.end());

        SpottingImage tmp(instancesById.at(iter->second),maxWidth,contextPad,color,prevNgram);
        ret->push_back(tmp);

        assert(ret->at(ret->size()-1).id == (iter->second));
        
        instancesByScore.erase(iter);
    }*/

    //scoresToDraw will be ordered by set<>, so we just need to increment the iterator along
    auto iterR = instancesByScore.begin();
    while(iterR!=instancesByScore.end() && instancesById.at(iterR->second).score(useQbE) < acceptThreshold)
    {
        iterR++;
    } 
    for (float drawScore : scoresToDraw)
    {
        while (iterR!=instancesByScore.end() && instancesById.at(iterR->second).score(useQbE) < drawScore)
        {
            iterR++;
        }
        if (iterR==instancesByScore.end() || (instancesById.at(iterR->second).score(useQbE)>rejectThreshold && iterR!=instancesByScore.begin()))
        {
            iterR--;
        }
    
        assert (updateMap.find(iterR->second)==updateMap.end());
        assert(instancesById.find(iterR->second) != instancesById.end());

        SpottingImage tmp(instancesById.at(iterR->second),maxWidth,contextPad,color,prevNgram);
        ret->push_back(tmp);

        assert(ret->at(ret->size()-1).id == (iterR->second));
        
        iterR = instancesByScore.erase(iterR);
        if (iterR == instancesByScore.end() && iterR != instancesByScore.begin())
            iterR--;
    }

    //check if done, that is we don't have spottings left between the thresholds
    *done=0;
    if (instancesByScore.size()<=1)
        *done=allBatchesSent?1:2;
    else
    {
        if (instancesById.at(iterR->second).score(useQbE) > rejectThreshold)
        {
            if (iterR==instancesByScore.begin())
                *done=allBatchesSent?1:2;
            else
            {
                iterR--;
                assert(instancesById.find(iterR->second) != instancesById.end());
                if (instancesById.at(iterR->second).score(useQbE) < acceptThreshold)
                    *done=allBatchesSent?1:2;
            }
        }
        else if (instancesById.at(iterR->second).score(useQbE) < acceptThreshold)
        {
            iterR++;
            if (iterR==instancesByScore.end())
                *done=allBatchesSent?1:2;
            else
            {
                assert(instancesById.find(iterR->second) != instancesById.end());
                if (instancesById.at(iterR->second).score(useQbE) > rejectThreshold)
                    *done=allBatchesSent?1:2;
            }
        }

        
        if (batchesSinceChange++ > CHECK_IF_BAD_SPOTTING_START)
        {
            if (numberClassifiedTrue/(numberClassifiedTrue+numberClassifiedFalse+0.0) < CHECK_IF_BAD_SPOTTING_THRESH)
                this->done=allBatchesSent?1:2;
        }
        
        if (!*done && takeFromTail && runningClassificationTrueAverage() > TAIL_CONTINUE_THRESH)
        {
            *done=0;//Just keep taking from the top until we aren't getting a lot of trues. This allows a bad thresholding to still work
#ifdef TEST_MODE
            cout<<"["<<ngram<<"] continuing past thresh due to running classification avg: "<<runningClassificationTrueAverage()<<endl;
#endif
        }
    }
    
    if (*done)
        allBatchesSent=true;
    
    //cout<<"batch is "<<ret->size()<<endl;
    //cout << "send: ";
    for (int i=0; i<ret->size(); i++)
    {   
        assert(instancesById.find(ret->at(i).id) != instancesById.end());
        starts[ret->at(i).id] = chrono::system_clock::now();
        
        //time_t tt= chrono::system_clock::to_time_t ( starts[ret->at(i).id] );
        //cout << ret->at(i).id<<", ";//" at time "<< ctime(&tt)<<endl;
    }
        //cout <<endl;
    //sem_post(&mutexSem);
    //numBatches++;
    
    //debugState();
    return ret;
}

BatchWraper* SpottingResults::getSpottingsAsBatch(int width, int color, string prevNgram, vector<unsigned long> spottingIds)
{
    SpottingsBatch* batch = new SpottingsBatch(ngram,id);
    for (unsigned long sid : spottingIds)
    {
        if (instancesById.find(sid)!=instancesById.end())
            batch->emplace_back(instancesById.at(sid),width,contextPad,color,prevNgram);
        else
            cout<<"ERROR "<<ngram<<"["<<id<<"]: no spotting by id "<<sid<<"  skipping"<<endl;
    }
    BatchWraperSpottings* ret = new BatchWraperSpottings(batch);
    return ret;
}

float SpottingResults::runningClassificationTrueAverage()
{
    if (runningClassifications.size()<RUNNING_CLASSIFICATIONS_COUNT)
        return 1;
    float ret=0;
    for (bool c : runningClassifications)
        ret += c?1:0;
    return ret/runningClassifications.size();
}

void SpottingResults::updateRunningClassifications(const vector<unsigned long>& ids, const vector<int>& newClassifications)
{
    for (int i=0; i<newClassifications.size(); i++)
    {
        auto iter = dontAddToRunningClassifications.find(ids[i]);
        if (iter == dontAddToRunningClassifications.end())
        {
            bool c = newClassifications[i];
            if (c==0 || c==1)
                runningClassifications.push_back((bool)c);
        }
        else
        {
            dontAddToRunningClassifications.erase(iter);
        }
    }
    while (runningClassifications.size()>RUNNING_CLASSIFICATIONS_COUNT)
        runningClassifications.pop_front();
}

void SpottingResults::resetRunningClassifications()
{
    for (auto p : starts)
        dontAddToRunningClassifications.insert(p.first);
    runningClassifications.clear();
}

vector<Spotting>* SpottingResults::feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent, vector<pair<unsigned long,string> >* retRemove, map<string,vector<Spotting> >* forAutoApproval)
{
    //debugState();
    /*cout << "fed: ";
    for (int i=0; i<ids.size(); i++)
    {   
        cout << ids[i]<<", ";
    }
    cout<<endl;*/
    vector<unsigned long> sids;
    for (string id : ids)
        sids.push_back(stoul(id));
    updateRunningClassifications(sids,userClassifications);
    if (runningClassificationTrueAverage()<0.5)
    {
        badInARow++;
    }
    else
    {
        badInARow=0;
    }

    int numTrue=0;
    int numFalse=0;
    
    vector<Spotting>* ret = new vector<Spotting>();
    int swing=0;
    for (unsigned int i=0; i< ids.size(); i++)
    {
        unsigned long id = sids[i];
        int check = starts.erase(id);
        //assert(check==1 || resent);

        //In the event this spotting has been updated
        while (updateMap.find(id)!=updateMap.end())
        {
            //unsigned long oldId=id;
            id=updateMap.at(id);
            //instancesById.erase(oldId);
        }

        auto iterClass = classById.find(id);
        if (!resent && iterClass!=classById.end())
            continue;//we'll skip processing if they waited to long and it got done. Unreliable

        if (resent)
        {
            if (iterClass->second)
                numberClassifiedTrue--;
            else
                numberClassifiedFalse--;
        }
        
        if (instancesById.find(id) == instancesById.end())
        {
            //This occurs when QbE accumulations have kicked the instance out.
            assert(kickedOut.find(id)!=kickedOut.end());
            continue;
        }
        instancesById.at(id).type=SPOTTING_TYPE_APPROVED;
        // adjust threshs
        if (userClassifications[i]>0)
        {
            swing++;
            numberClassifiedTrue++;
            numTrue++;
#ifdef NO_NAN
        GlobalK::knowledge()->accepted();
#endif
            if (!resent || !iterClass->second)
            {
                ret->push_back(instancesById.at(id)); //otherwise we've already sent it
            }
            classById[id]=true;

#ifdef AUTO_APPROVE
            for (int subLen=ngram.length()-1; subLen>0; subLen--)
            {
                for (int subPos=0; subPos<=ngram.length()-subLen; subPos++)
                {
                    string sub = ngram.substr(subPos,subLen);
                    int width = (instancesById.at(id).brx-instancesById.at(id).tlx+1)*subLen/(0.0+ngram.length());
                    int xStart = width*subPos + instancesById.at(id).tlx;
                    int xEnd = xStart+width-1;
                    (*forAutoApproval)[sub].emplace_back(instancesById.at(id).pageId,xStart,instancesById.at(id).tly,xEnd,instancesById.at(id).bry,instancesById.at(id).wordId);
                }
            }
#endif
        }
        else if (userClassifications[i]==0)
        {
            swing--;
            numberClassifiedFalse++;
            numFalse--;
#ifdef NO_NAN
        GlobalK::knowledge()->rejected();
#endif
            if (resent && (retRemove && iterClass->second))
            {
                retRemove->push_back(make_pair(id,instancesById.at(id).ngram));
            }
            classById[id]=false;
        }
        else if (!resent)//someone passed, so we need to add it again, unless this is resent, in whichcase we use the old.
        { 
            instancesByScoreInsert(id);
            tracer = instancesByScore.begin();
        }
    }

    if (batchesSinceChange > CHECK_IF_BAD_SPOTTING_START)
    {
        if (numberClassifiedTrue/(numberClassifiedTrue+numberClassifiedFalse+0.0) < CHECK_IF_BAD_SPOTTING_THRESH)
            this->done=true;
    }

    if (numTrue+numFalse>0)
        batchTracking.emplace_back(-1,numTrue/(0.0+numTrue+numFalse),numTrue+numFalse,0,runningClassificationTrueAverage());

    if (this->done)
    {
        *done=1;
        return ret;
    }
    bool allWereSent = allBatchesSent; 
    EMThresholds(swing);
  


#ifdef TEST_MODE
    cout<<"["<<ngram<<"]: all sent: "<<allBatchesSent<<", waiting for "<<starts.size()<<", num left "<<instancesByScore.size()<<endl;
    //for (auto i=instancesByScore.begin(); i!=instancesByScore.end(); i++)
    //    cout <<(**i).id<<"[tlx:"<<(**i).tlx<<" score:"<<(**i).score<<"]"<<endl;
#endif

    if (resent==0 && starts.size()==0 && allBatchesSent)
    {
        if (allWereSent)
            *done=1;
        else
            *done=2;
        this->done=true;
#ifdef TEST_MODE
        cout <<"["<<ngram<<"]: all batches sent, cleaning up"<<endl;
        int numAutoAccepted=0;
        int trueAutoAccepted=0;
        int numAutoRejected=0;
        int trueAutoRejected=0;
#endif
        tracer = instancesByScore.begin();
        while (tracer!=instancesByScore.end() && instancesById.at(tracer->second).score(useQbE) <= acceptThreshold)
        {
            assert(instancesById.find(tracer->second) != instancesById.end());

            instancesById.at(tracer->second).type=SPOTTING_TYPE_THRESHED;
            ret->push_back(instancesById.at(tracer->second));
            classById[instancesById.at(tracer->second).id]=true;
            numberAccepted++;
#ifdef NO_NAN
            GlobalK::knowledge()->autoAccepted();
#endif
#ifdef TEST_MODE
            numAutoAccepted++;
            trueAutoAccepted += GlobalK::knowledge()->ngramAt(ngram, instancesById.at(tracer->second).pageId, instancesById.at(tracer->second).tlx, instancesById.at(tracer->second).tly, instancesById.at(tracer->second).brx, instancesById.at(tracer->second).bry)?1:0;
#endif
#ifdef TEST_MODE_LONG
            cout <<" "<<instancesById.at(tracer->second).id<<"[tlx:"<<instancesById.at(tracer->second).tlx<<", score:"<<instancesById.at(tracer->second).score(useQbE)<<"]: true"<<endl;
#endif
            tracer++;
        }
        while (tracer!=instancesByScore.end())
        {
            assert(instancesById.find(tracer->second) != instancesById.end());

            instancesById.at(tracer->second).type=SPOTTING_TYPE_THRESHED;
            classById[instancesById.at(tracer->second).id]=false;
#ifdef NO_NAN
            GlobalK::knowledge()->autoRejected();
#endif
#ifdef TEST_MODE
            numAutoRejected++;
            trueAutoRejected += GlobalK::knowledge()->ngramAt(ngram, instancesById.at(tracer->second).pageId, instancesById.at(tracer->second).tlx, instancesById.at(tracer->second).tly, instancesById.at(tracer->second).brx, instancesById.at(tracer->second).bry)?1:0;
#endif
#ifdef TEST_MODE_LONG
            cout <<" "<<instancesById.at(tracer->second).id<<"[tlx:"<<instancesById.at(tracer->second).tlx<<", score:"<<instancesById.at(tracer->second).score(useQbE)<<"]: false"<<endl;
#endif
            tracer++;
        }
        //cout << "hit end "<<(tracer==instancesByScore.end())<<endl;
        numberRejected = distance(tracer,instancesByScore.end());
#ifdef TEST_MODE
        cout<<"["<<ngram<<"], AUTO accepted["<<numAutoAccepted<<"]: "<<(0.0+trueAutoAccepted)/numAutoAccepted<<"  rejected["<<numAutoRejected<<"]: "<<(0.0+trueAutoRejected)/numAutoRejected<<endl;
#endif        
        instancesByScore.clear();
        tracer = instancesByScore.begin();
    }
    else if (!allBatchesSent && allWereSent)
    {
        *done=-1;
#ifdef TEST_MODE
        cout<<"SpottingResults ["<<ngram<<"]: has more batches and will be re-enqueued"<<endl;
#endif
    }
    //debugState();
    return ret;
}

float SpottingResults::NPD(float x, float mean, float var)
{
    ///if (useQbE)
    ///{
    ///    return (1/x)*(1/sqrt(2*CV_PI*var))*exp(-0.5*pow(logCvt(x)-mean,2)/var);
    ///}
    ///else
        return (1/sqrt(2*CV_PI*var))*exp(-0.5*pow(x-mean,2)/var);
}
    
float SpottingResults::PHI(float x) //cumulative distribution function of normal distribution
{
    float sum=x;
    float value =x;
    for (int i=1; i<100; i++)
    {
        value=(value*x*x/(2*i+1));
        sum+=value;
    }
    return 0.5+(sum/sqrt(2*CV_PI))*exp(-(x*x)/2);
}
float SpottingResults::PHI(float x, float mean, float std) //cumulative distribution function of normal distribution
{
    float nX;
    ///if (useQbE)
    ///    nX = (logCvt(x)-mean)/std;
    ///else
        nX = (x-mean)/std;
    return PHI(nX);
}
float SpottingResults::logCvt(float x)
{
    return log( -1*(min(x,cvtMax)-cvtMax) + CVT_MARGIN);
}

float SpottingResults::expCvt(float x)
{
    return cvtMax-1*(exp(x)-CVT_MARGIN);
}

void SpottingResults::EM_none()
{
    acceptThreshold=minScore();
    rejectThreshold=maxScore();
    pullFromScore = (acceptThreshold+rejectThreshold)/2.0;// -sqrt(trueVariance);
}

void SpottingResults::EM_top(bool init)
{
    acceptThreshold=minScore();
    if (runningClassificationTrueAverage() > 0.3)
    {
        rejectThreshold=maxScore();
    }
    else
    {
        rejectThreshold=minScore();
    }
    pullFromScore = (acceptThreshold+rejectThreshold)/2.0;// -sqrt(trueVariance);
}
void SpottingResults::EM_otsuFixed(bool init)
{
    if (init)
    {
        //we initailize our split with Otsu, as we are assuming two distinct distributions.
        //make histogram
        vector<int> histogram(40);
        //int total = 0;//instancesById.size();
        for (auto p : instancesById)
        {
            if (p.second.score(useQbE)==p.second.score(useQbE) && p.second.score(useQbE)!=MAX_FLOAT)
            {
                //total++;
                unsigned long id = p.first;
                
                int bin = 39*(instancesById.at(id).score(useQbE)-minScore())/(maxScore()-minScore());
                if (bin<0) bin=0;
                if (bin>histogram.size()-1) bin=histogram.size()-1;
                histogram[bin]++;
            }
        }
        
        
        double thresh = GlobalK::otsuThresh(histogram);
        acceptThreshold=minScore();
        rejectThreshold=thresh;
    }
}
// void SpottingReuslts::EM_otsuAdapt(bool init)
void SpottingResults::EM_takeGuass(bool init)
{
    if (init)
    {
        acceptThreshold=minScore();
        //int oneQuarter = instancesByScore.size()/4;
        //auto scoreIter = instanceByScore.begin();
        //for (int i=0; i<oneQuarter; i++)
        //    scoreIter++;
        pullFromScore=minScore() + (maxScore()-minScore())*0.25;
        //int half = instancesByScore.size()/2;
        //for (int i=oneQuarter; i<half; i++)
        //    scoreIter++;
        rejectThreshold=minScore() + (maxScore()-minScore())*0.5;
        takeStd = (pullFromScore-minScore())/2;
    }

    if (runningClassifications.size()==RUNNING_CLASSIFICATIONS_COUNT)
    {
        float shift = 0.5-runningClassificationTrueAverage();
        if (shift<0)
        {
            shift*=-1;
            pullFromScore = minScore() + (pullFromScore-minScore())*(1-shift);
        }
        else
        {
            pullFromScore = pullFromScore + (pullFromScore-minScore())*(shift/0.5);
        }
        takeStd = (pullFromScore-minScore())/2;
        rejectThreshold = pullFromScore+2*takeStd;

    }
}

void SpottingResults::EM_fancy(bool init)
{
    float oldMidScore = acceptThreshold + (rejectThreshold-acceptThreshold)/2.0;
    /*This will likely predict very narrow and distinct distributions
     *initailly. This should be fine as we sample from the middle of
     *the thresholds outward.
     */
    
#ifdef TEST_MODE 
    //test
    float actualModelDif=-99999;
    bool swaped=false;
#endif
    trueFalseDivide=99999;
    distCheckCounter++;



    if (init || distCheckCounter%5==0)
    {
        if (badInARow>2 and !GlobalK::knowledge()->SR_FANCY_ONE and !GlobalK::knowledge()->SR_FANCY_TWO)
        {
            if (!takeFromTail)
                badInARow=0;
            takeFromTail=true;//a hueristic, as unigram spotting is worse and doesn't usually have a pure true section.
        }
        else
        {
            /*
            */
            float usedSum=0;
            vector<float> usedValues;
            vector<float> notFalseValues;
            int allCount=0;
            for (auto p : instancesById)
            {
                if (p.second.score(useQbE)!=p.second.score(useQbE) || p.second.score(useQbE)==MAX_FLOAT)
                    continue;
                float score = p.second.score(useQbE); ///useQbE?logCvt(p.second.score(useQbE)):p.second.score(useQbE);
                allCount++;
                if (classById.find(p.first)==classById.end() || classById.at(p.first))
                    notFalseValues.push_back(score);
                if (classById.find(p.first)!=classById.end() && classById.at(p.first))
                    continue;//we've labeled it as true, so we can atleast prune this
                usedSum += score;
                usedValues.push_back(score);
            }
            usedMean = maxScore(); ///useQbE?usedSum/usedValues.size():maxScore(); //The QbS false distributions are always cut off, so we just assinge the max as the mean
            usedStd=0;
            for (float v : usedValues)
                usedStd += pow(v-usedMean,2);
            usedStd = sqrt(usedStd/usedValues.size());

            //Do we have outliers on our tail?   ('tail' being the trailing low scores that positive instances should lie in).

            float delta = (maxScore()-minScore())/1000;
            tailEndScore=minScore();
            while (1)
            {
                float density = PHI(tailEndScore,usedMean,usedStd)*usedValues.size();
                if (density>TAIL_THRESH)
                {
                    tailEndScore-=delta;
                    break;
                }
                tailEndScore+=delta;
                assert(tailEndScore<maxScore());
            }
            tailEnd = (tailEndScore-usedMean)/usedStd;
            //tailEnd=-1.8; ///useQbE?2.5:-1.8;
            //tailEndScore=usedMean + usedStd*tailEnd;
            //hack
            //if (!useQbE)
            //{
            //    tailEndScore = (0.7*tailEndScore+0.3*GOOD_TAIL_SCORE);//Lean towards a 'good' tail score to prevent grevious errors
            //    tailEnd = (tailEndScore-usedMean)/usedStd;
            //}
            //if (useQbE)
            //    tailEndScore = expCvt(tailEndScore);
            //Count instances in the tail
            int countTail=0;
            for (float v : notFalseValues)
                if (v<tailEndScore) ///if ( (!useQbE&&v<tailEndScore) || (useQbE&&v>tailEndScore) )
                    countTail++;
            //This represent the density that the model says should be after we have any examples. Given more examples (not threshing the spotting results) this would be in our data
            float cutOffDensity = (1-PHI((maxScore()-usedMean)/usedStd)); ///useQbE?1:(1-PHI((maxScore()-usedMean)/usedStd));

            //We then estimate how many instances we would have if this part wern't cut off
            float estTotalCount = allCount + allCount*(1-cutOffDensity);
            assert(estTotalCount >= allCount);

            //We use this to compute what the density of the tail region is
            float actualDensity = countTail/estTotalCount;

            //Compute expected (model) density of tail
            float modelDensity = PHI(tailEnd);

            //If difference is above a threshold, we have a good True distribution
#ifdef TEST_MODE 
            actualModelDif=actualDensity-modelDensity;
            assert(actualModelDif==actualModelDif);
#endif
            bool useTwoDist = actualDensity-modelDensity > TAIL_DENSITY_TRUE_THRESHOLD && ngram.length()!=1;
            if (GlobalK::knowledge()->SR_FANCY_ONE)
                useTwoDist=false;
            else if (GlobalK::knowledge()->SR_FANCY_TWO)
                useTwoDist=true;

            if (useTwoDist)
            {
                if (!init && takeFromTail)
                {
#ifdef TEST_MODE 
                    cout<<"["<<ngram<<"] swaped to two distrubution mode."<<endl;
                    swaped=true;
#endif
                    init=true; //For the case of a correction at a check
                    
                }
                if (takeFromTail)
                {
                    badInARow=0;
                    resetRunningClassifications();
                }
                takeFromTail=false;

                //we initailize our split with Otsu, as we are assuming two distinct distributions.
                //make histogram
                vector<int> histogram(40);
                //int total = 0;//instancesById.size();
                for (auto p : instancesById)
                {
                    if (p.second.score(useQbE)==p.second.score(useQbE) && p.second.score(useQbE)!=MAX_FLOAT)
                    {
                        //total++;
                        unsigned long id = p.first;
                        
                        int bin = 39*(instancesById.at(id).score(useQbE)-minScore())/(maxScore()-minScore());
                        if (bin<0) bin=0;
                        if (bin>histogram.size()-1) bin=histogram.size()-1;
                        histogram[bin]++;
                    }
                }
                
                
                double thresh = GlobalK::otsuThresh(histogram);
                float trueFalseDivideCandidate = (thresh/40)*(maxScore()-minScore())+minScore();
                //float estSplit = usedMean-1.6*usedStd;
                trueFalseDivide = min(trueFalseDivideCandidate,tailEndScore);
                ///if (useQbE)
                ///    trueFalseDivide=logCvt(trueFalseDivide);
            }
            else
            {
                if (!init && !takeFromTail)
                {
#ifdef TEST_MODE 
                    cout<<"["<<ngram<<"] swaped to takeFromTail mode."<<endl;
                    swaped=true;
#endif
                    init=true;
                }
                if (!takeFromTail)
                {
                    badInARow=0;
                    resetRunningClassifications();
                }
                takeFromTail=true;
            }
            distCheckCounter=0;
        }
    }
        
        //map<unsigned long, bool> expected;
        multiset<float> expectedTrue;
        float sumTrue=0;
        vector<float> expectedFalse;
        float sumFalse=0;
        
        
        
        numLeftInRange=0;
        for (auto p : instancesById)
        {
            if (p.second.score(useQbE)!=p.second.score(useQbE) || p.second.score(useQbE)==MAX_FLOAT)
                continue; 
            unsigned long id = p.first;
            float score = instancesById.at(id).score(useQbE);
            ///if (useQbE)
            ///    score=logCvt(score);
            
#ifdef TEST_MODE
            //test
            //int bin=(displayLen-1)*(score-minScore())/(maxScore()-minScore());
#endif        
            
            if (classById.find(id)!=classById.end())
            {
                if (classById[id] && score<rejectThreshold+0.5*sqrt(falseVariance)) //Added thing to try and keep true meaa on low side
                {
                    expectedTrue.insert(score);
                    //expectedTrue.push_back(instancesById.at(id).score);
                    sumTrue+=1*score;
                    
#ifdef TEST_MODE
                    //test
                    //if (bin>=0) histogramCP.at(bin)++;
#endif        
                }
                else
                {
                    expectedFalse.push_back(score);
                    //expectedFalse.push_back(instancesById.at(id).score);
                    sumFalse+=1*score;
                    
#ifdef TEST_MODE
                    //test
                    //if (bin>=0) histogramCN.at(bin)++;
#endif        
                }
            }
            else
            {
                bool inRange = instancesById.at(id).score(useQbE) >acceptThreshold && instancesById.at(id).score(useQbE)<rejectThreshold;
                if (inRange)
                    numLeftInRange++;
                //if (strictPredict && init)
                //    continue; //skip, but we'll repeat once we've drawn mean and var from labeled examples
                if (takeFromTail)
                {
                    expectedFalse.push_back(score);
                    sumFalse+=score;
                }
                else if (init)
                {
                    if (score<trueFalseDivide)
                    {
                        sumTrue+=score;
                        expectedTrue.insert(score);
                    }
                    else
                    {
                        expectedFalse.push_back(score);
                        sumFalse+=score;
                    }
                }
                else
                {
                    double trueProb = NPD(instancesById.at(id).score(useQbE),trueMean,trueVariance);//exp(-1*pow(score - trueMean,2)/(2*trueVariance));
                    double falseProb = NPD(instancesById.at(id).score(useQbE),falseMean,falseVariance);//exp(-1*pow(score - falseMean,2)/(2*falseVariance));
                    falseProb *= falseProbWeight;
                    double normTerm = falseProb+trueProb;
                    falseProb /= normTerm;
                    trueProb /= normTerm;
//DEBUG
                    /*if (instancesById.at(id).gt >=0)
                    {
                        if ((double)rand() / RAND_MAX > 0.5)
                        {
                            falseProb = instancesById.at(id).gt?0:1;
                            trueProb = instancesById.at(id).gt?1:0;
                        }
                    }*/
//DEBUG
                    if ((double)rand() / RAND_MAX <= trueProb)
                    {
                        sumTrue+=score;
                        expectedTrue.insert(score);
                    }
                    else
                    {
                        expectedFalse.push_back(score);
                        sumFalse+=score;
                    }
                }
            }
        }

        if (expectedFalse.size()==0) //something is off. Esimate using all false, and use takeFromTail
        {
            assert(expectedTrue.size()>0);
            takeFromTail=true;
            sumFalse=sumTrue;
            sumTrue=0;
            expectedFalse.insert(expectedFalse.end(),expectedTrue.begin(),expectedTrue.end());
            expectedTrue.clear();
        }
        
        //maximization
        if (expectedTrue.size()!=0)
        {
            float newTrueMean=sumTrue/expectedTrue.size();
            while (newTrueMean > tailEndScore && expectedTrue.size()>1)
            {
                auto iter = expectedTrue.end();
                sumTrue -= *(--iter);
                expectedTrue.erase(iter);
                newTrueMean=sumTrue/expectedTrue.size();
            }
            trueMean = min(newTrueMean, tailEndScore);
        }
        //if (expectedFalse.size()!=0)
        falseMean=maxScore(); ///useQbE?(sumFalse/expectedFalse.size()):maxScore();
        trueVariance=0;
        for (float score : expectedTrue)
            trueVariance += (score-trueMean)*(score-trueMean);
        if (expectedTrue.size()!=0)
            trueVariance/=expectedTrue.size();
        falseVariance=0;
        for (float score : expectedFalse)
            falseVariance += (score-falseMean)*(score-falseMean);
        if (expectedFalse.size()!=0)
            falseVariance/=expectedFalse.size();

        falseProbWeight = expectedFalse.size()/(expectedTrue.size()+1);

        /*if (badInARow>0)
        {
            trueMean *= 1.3*badInARow;
            trueVariance *= 0.8*badInARow;
            falseVariance *= 1.3*badInARow;
        }*/


        float falseStd = sqrt(falseVariance);
        float trueStd = sqrt(trueVariance);

        assert(falseStd==falseStd);
        assert(trueStd==trueStd);
        
        if (takeFromTail)
        {
            acceptThreshold = minScore();
            rejectThreshold = falseMean + falseStd*tailEnd;
            //if (useQbE)
            //    rejectThreshold=exp(rejectThreshold);
            float shift = (0.5f-min(runningClassificationTrueAverage(),0.5f))/0.5f;
            float minRejectThreshold = 0.8*minScore() + 0.2*rejectThreshold;
            //auto iterScore = instancesByScore.begin();

            float rejectThreshold = (shift)*minRejectThreshold + (1-shift)*rejectThreshold;
        }
        else
        {
            //just take region of intersection between distribitons
            float delta = (maxScore()-minScore())/200;
            float maxAcceptThreshold = falseMean-MAX_ACCEPT_THRESHOLD_FROM_FALSE*falseStd;
            float maxRejectThreshold = falseMean-MAX_REJECT_THRESHOLD_FROM_FALSE*falseStd;
            float newAcceptT=minScore();
            while (newAcceptT < maxAcceptThreshold)
            {
                float density = PHI(newAcceptT,falseMean,falseStd)*expectedFalse.size();
                if (density>ACCEPT_THRESH)
                {
                    newAcceptT-=delta;
                    break;
                }
                newAcceptT+=delta;
                if (newAcceptT>maxScore() && instancesByScore.size()>0)
                {
                    int counter=0;
                    auto scoreIter = instancesByScore.begin();
                    while (scoreIter != instancesByScore.end() && counter++<25)
                    {
                        newAcceptT=scoreIter->first;
                    }
                }
            }
            float newRejectT=newAcceptT+delta;
            //phi_false(reject)-phi_false(accept) = phi_true(reject)-phi_true(accept) #the midpoint
            float constOverlap = PHI(newAcceptT,trueMean,trueStd)*expectedTrue.size() - PHI(newAcceptT,falseMean,falseStd)*expectedFalse.size();
            float lastDif = PHI(newRejectT,falseMean,falseStd)*expectedFalse.size() - PHI(newRejectT,trueMean,trueStd)*expectedTrue.size() + constOverlap;//this allows us to have either dist be bigger
            newRejectT+=delta;
            while (newRejectT < maxRejectThreshold)
            {
                float dif = PHI(newRejectT,falseMean,falseStd)*expectedFalse.size() - PHI(newRejectT,trueMean,trueStd)*expectedTrue.size() + constOverlap;
                if (GlobalK::sgn(dif) != GlobalK::sgn(lastDif))
                {
                    newRejectT-=delta;
                    break;
                }
                newRejectT+=delta;
                assert(newRejectT<maxScore());
                lastDif=dif;
            }

            if ((newRejectT-falseMean)/falseStd > -1.0)
                newRejectT = falseMean-falseStd;

            acceptThreshold = newAcceptT;
            rejectThreshold = 0.45*newAcceptT + 0.55*newRejectT;//This is a hueristic, as it tends to poorly model the true distribution causing the reject threshold to be too large
            //assert(rejectThreshold>acceptThreshold);
            float shift = (0.5f-min(runningClassificationTrueAverage(),0.5f))/0.5f;
            float newAcceptThreshold = (shift)*minScore() + (1-shift)*acceptThreshold;
            rejectThreshold -= acceptThreshold-newAcceptThreshold;
            acceptThreshold= newAcceptThreshold;
        }



        assert(rejectThreshold==rejectThreshold && acceptThreshold==acceptThreshold);
        if (rejectThreshold<=acceptThreshold)
        {
            acceptThreshold=minScore();
            auto iterScore = instancesByScore.begin();
            iterScore++;
            for (int ii=1; ii<min(RUNNING_CLASSIFICATIONS_COUNT*10*runningClassificationTrueAverage(),(float)instancesByScore.size()); ii++)
                iterScore++;
            rejectThreshold = iterScore->first;
        }
        //assert(rejectThreshold>acceptThreshold);
#ifdef TEST_MODE
#ifdef NO_NAN
        if (init)
            saveHistogram(actualModelDif);
#endif
#ifdef GRAPH_SPOTTING_RESULTS
        if (swaped)
        {
            cv::Mat undoneW(1,undoneGraph.cols,CV_8UC3);
            undoneW = cv::Scalar(155,155,155);
            undoneGraph.push_back(undoneW);
            cv::Mat fullW(1,fullGraph.cols,CV_8UC3);
            fullW = cv::Scalar(155,155,155);
            fullGraph.push_back(fullW);
            setDebugInfo(NULL);
        }
#endif
        cout <<"["<<ngram<<"]: adjusted threshs, now "<<acceptThreshold<<" <> "<<rejectThreshold<<"    computed with mean/std devs of: f:"<<falseMean<<"/"<<sqrt(falseVariance)<<", t:"<<trueMean<<"/"<<sqrt(trueVariance)<<endl;
#endif        
    float prevPullFromScore = pullFromScore;
    pullFromScore = (acceptThreshold+rejectThreshold)/2.0;// -sqrt(trueVariance);
}
    

bool SpottingResults::EMThresholds(int swing)
{
    
    //debugState();
    assert(instancesById.size()>1);
    assert(maxScore()!=-999999);

    bool init = acceptThreshold==-1 && rejectThreshold==-1;
if (GlobalK::knowledge()->SR_THRESH_NONE)
    EM_none();
else if (GlobalK::knowledge()->SR_TAKE_FROM_TOP)
    EM_top(init);
else if (GlobalK::knowledge()->SR_OTSU_FIXED)
    EM_otsuFixed(init);
//else if (GlobalK::knowledge()->SR_OTSU_ADAPT)
//    EM_otsuAdapt(init);
else if (GlobalK::knowledge()->SR_GAUSSIAN_DRAW)
    EM_takeGuass(init);
else
    EM_fancy(init);

    //safe gaurd
    if (pullFromScore>rejectThreshold)
        pullFromScore=rejectThreshold;
    else if (pullFromScore<acceptThreshold)
        pullFromScore=acceptThreshold;

    
    if(acceptThreshold>rejectThreshold) {//This is the case where the distributions are so far apart they "don't overlap"
        if (instancesById.size()/3 < classById.size()){//Be sure we aren't hitting this too early
            float mid = rejectThreshold + (acceptThreshold-rejectThreshold)/2.0;
            acceptThreshold = rejectThreshold = mid;//We finish here, by accepting and rejecting everything left.
            cout<<"cross threshed, finisheing"<<endl;
        } 
        else {
            acceptThreshold = trueMean;//This is an overcorrection, allowing a alater batch to fix us.
            rejectThreshold = falseMean;
            cout<<"cross threshed, correcting"<<endl;
        }
    }
    
    /*if (!init)
    {
        int ns=0;
        float innerTrue;
        float innerFalse;
        do
        {
            innerTrue=trueMean+ns*sqrt(trueVariance);
            innerFalse=falseMean-ns*sqrt(falseVariance);
            ns++;
        } while (innerTrue<innerFalse);
        innerTrue=trueMean+(ns-1)*sqrt(trueVariance);
        innerFalse=falseMean-(ns-1)*sqrt(falseVariance);
        float newMidScore = innerTrue+ (innerFalse-innerTrue)/2.0;
        pullFromScore = trueMean;//newMidScore;
        //cout << "pullFromScore: "<<pullFromScore<<endl;
    }*/
    if (init) {
        allBatchesSent=false;
        numLeftInRange=instancesByScore.size();
    }
    else
    {
        allBatchesSent=true;
        auto iter = instancesByScore.begin();
        while (iter!=instancesByScore.end() && instancesById.at(iter->second).score(useQbE) < rejectThreshold)
        {
            assert(instancesById.find(iter->second) != instancesById.end());
            
            if (instancesById.at(iter->second).score(useQbE) > acceptThreshold && instancesById.at(iter->second).score(useQbE) < rejectThreshold)
            {
                allBatchesSent=false;
                break;
            }
            iter++;
        } 
    }
    //debugState();
    return allBatchesSent;
}


bool SpottingResults::checkIncomplete()
{
    //debugState();
    bool incomp=false;
    //cout <<"checkIncomplete, starts is "<<starts.size()<<endl;
    //vector<unsigned long> toRemove;
    for (auto iter=starts.begin(); iter!=starts.end(); iter++)
    {
        auto start = *iter;
        chrono::system_clock::duration d = chrono::system_clock::now()-start.second;
        chrono::minutes pass = chrono::duration_cast<chrono::minutes> (d);
        //cout<<pass.count()<<" minutes has past for "<<(start.first)<<endl;
        if (pass.count() > 20) //if 20 mins has passed
        {
            unsigned long restartId=start.first;
            if (instancesById.find(restartId) == instancesById.end())
            {
                if (updateMap.find(restartId) != updateMap.end())
                    restartId = updateMap.at(start.first);
                else
                    assert(false && "SpottingResults::checkIncomplete() is trying to restart a non-existant spotting id");
            }
            instancesByScoreInsert(restartId);
            tracer = instancesByScore.begin();
            incomp=true;
            //toRemove.push_back(start.first);
#ifdef TEST_MODE
            cout<<"Timeout ("<<pass.count()<<") on batch "<<start.first<<endl;
#endif     
            iter = starts.erase(iter);
            if (iter!=starts.begin())
                iter--;
            if (iter==starts.end())
                break;
        }
    }
    //for (unsigned long id : toRemove)
    //    starts.erase(id);
    if (incomp && allBatchesSent)
    {
        allBatchesSent=false;
        return true;
    }
    //debugState();
    return false;
}


multiset<Spotting,tlComp>::iterator SpottingResults::findOverlap(const Spotting& spotting, float* ratioOff) const
{
    bool updated=false;
    int width = spotting.brx-spotting.tlx;
    int height = spotting.bry-spotting.tly;
    //Spotting* bestSoFar=NULL;
    auto bestSoFarIter = instancesByLocation.end();
    int bestOverlap=0;
    double spottingArea = (spotting.brx-spotting.tlx)*(spotting.bry-spotting.tly);
    for (int tlx=spotting.tlx-width*(1-UPDATE_OVERLAP_THRESH); tlx<spotting.tlx+width*(1-UPDATE_OVERLAP_THRESH); tlx++)
    {
        //Find all spottings for given tlx
        Spotting lb(spotting.pageId,tlx,spotting.tly-height*(1-UPDATE_OVERLAP_THRESH));
        Spotting ub(spotting.pageId,tlx,spotting.tly+height*(1-UPDATE_OVERLAP_THRESH));
        auto itLow = instancesByLocation.lower_bound(lb);
        auto itHigh = instancesByLocation.upper_bound(ub);


        for (;itLow!=itHigh; itLow++)
        {
            int overlapArea = ( min(spotting.brx,itLow->brx) - max(spotting.tlx,itLow->tlx) ) * ( min(spotting.bry,itLow->bry) - max(spotting.tly,itLow->tly) );
            double thresh = UPDATE_OVERLAP_THRESH;

            assert(instancesById.find(itLow->id) != instancesById.end());

            bool updateWhenInBatch = itLow->type!=SPOTTING_TYPE_THRESHED && !instancesByScoreContains(itLow->id);
            if (updateWhenInBatch)
                thresh=UPDATE_OVERLAP_THRESH_TIGHT;
            double ratio = overlapArea/max(spottingArea,1.0*(itLow->brx-itLow->tlx)*(itLow->bry-itLow->tly));
            if (ratio > thresh)
            {
                if (overlapArea > bestOverlap)
                {
                    bestOverlap=overlapArea;
                    //bestSoFar=*itLow;
                    bestSoFarIter=itLow;
                    if (ratioOff!=NULL)
                        *ratioOff = 1.0 - (ratio-thresh)/(1.0-thresh);
                }
                else
                {
                    tlx=spotting.tlx+width*(1-UPDATE_OVERLAP_THRESH);
                    break;
                }


                
            }
        }
        if (!useQbE) //we may have the instances, just not queued in
        {
            Spotting lb(spotting.pageId,tlx,spotting.tly-height*(1-UPDATE_OVERLAP_THRESH));
            Spotting ub(spotting.pageId,tlx,spotting.tly+height*(1-UPDATE_OVERLAP_THRESH));
            auto itLow = instancesToAddQbEByLocation.lower_bound(lb);
            auto itHigh = instancesToAddQbEByLocation.upper_bound(ub);


            for (;itLow!=itHigh; itLow++)
            {
                int overlapArea = ( min(spotting.brx,itLow->brx) - max(spotting.tlx,itLow->tlx) ) * ( min(spotting.bry,itLow->bry) - max(spotting.tly,itLow->tly) );
                double thresh = UPDATE_OVERLAP_THRESH;

                double ratio = overlapArea/max(spottingArea,1.0*(itLow->brx-itLow->tlx)*(itLow->bry-itLow->tly));
                if (ratio > thresh)
                {
                    if (overlapArea > bestOverlap)
                    {
                        bestOverlap=overlapArea;
                        //bestSoFar=*itLow;
                        bestSoFarIter=itLow;
                        if (ratioOff!=NULL)
                            *ratioOff = 1.0 - (ratio-thresh)/(1.0-thresh);
                    }
                    else
                    {
                        tlx=spotting.tlx+width*(1-UPDATE_OVERLAP_THRESH);
                        break;
                    }


                    
                }
            }
        }
    }
    return bestSoFarIter;
}

//This method is to check to see if we actually have this exemplar already and then prevent is from being re-approved
void SpottingResults::updateSpottingTrueNoScore(const SpottingExemplar& spotting)
{
#ifdef GRAPH_SPOTTING_RESULTS
    fullInstancesByScore.clear();
#endif
    //debugState();
    assert(spotting.tlx!=-1);
    assert(spotting.scoreQbE != spotting.scoreQbE);
    assert(spotting.scoreQbS != spotting.scoreQbS);

    //Scan for possibly overlapping (the same) spottings
    auto bestIter = findOverlap(spotting,NULL);
    if (bestIter != instancesByLocation.end() && instancesById.find(bestIter->id)!=instancesById.end())
    {
        Spotting best=*bestIter;

        //bool updateWhenInBatch = (best)->type!=SPOTTING_TYPE_THRESHED && instancesByScore.find(best->id)==instancesByScore.end();
        //if (updateWhenInBatch)
            updateMap[best.id]=spotting.id;
#ifdef TEST_MODE
        //else
        //    testUpdateMap[best.id]=spotting.id;
#endif
        //Add this spotting
        assert(spotting.pageId>=0);
        instancesById[spotting.id]=spotting;
        instancesById.at(spotting.id).type=SPOTTING_TYPE_APPROVED;
        instancesById.at(spotting.id).scoreQbE=best.scoreQbE;//we replace the score so that we contribute to the thresholding
        instancesById.at(spotting.id).scoreQbS=best.scoreQbS;//we replace the score so that we contribute to the thresholding
        instancesByLocation.insert(instancesById.at(spotting.id));
        classById[spotting.id]=true;
        instancesByLocation.erase(bestIter); //erase by iterator

        //remove the old one
        bool removed = instancesByScoreErase(best.id); //erase by value (pointer)
        if (removed)
        {
            tracer = instancesByScore.begin();
        }
        allInstancesByScoreQbEErase(best.id);
        instancesById.erase(best.id);
    }
    else
    {
        addTrueNoScore(spotting);
    }
    //debugState();
}

//combMin
#if USE_QBE
bool SpottingResults::updateSpottings(vector<Spotting>* spottings)
{
    bool fresh=false;
#ifdef GRAPH_SPOTTING_RESULTS
    fullInstancesByScore.clear();
#endif
    //debugState();
    if (++numComb>GlobalK::numCombThresh(ngram) && !useQbE)
    {
        fresh=true;
        cout<<"["<<ngram<<"] now using combine QbE scores."<<endl;
        useQbE=true;
        acceptThreshold=-1;//re-init the thresholds
        rejectThreshold=-1;
        multimap<float,unsigned long> tmpI=instancesByScore;
        instancesByScore.clear();
        for (auto p : tmpI)
            instancesByScoreInsert(p.second);
        for (auto& p : instancesById)
        {
            if (p.second.scoreQbE==p.second.scoreQbE && p.second.scoreQbE!=MAX_FLOAT &&
                    p.second.scoreQbS==p.second.scoreQbS)
            {
                instancesByLocationErase(p.first);
                p.second.tlx = get<0>(p.second.boxQbE);
                p.second.tly = get<1>(p.second.boxQbE);
                p.second.brx = get<2>(p.second.boxQbE);
                p.second.bry = get<3>(p.second.boxQbE);
                assert(p.second.brx-p.second.tlx>0 && p.second.bry-p.second.tly>0);
                instancesByLocation.insert(instancesById.at(p.first));
            }
        }
        for (auto& spotting : instancesToAddQbEByLocation)
        {
            instancesById[spotting.id]=spotting;
            instancesByLocation.insert(spotting);
            if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
            {
                classById[spotting.id]=true;
                numberClassifiedTrue++;
            }
            else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
            {
                classById[spotting.id]=false;
                numberClassifiedFalse++;
            }
            else
            {
                instancesByScoreInsert(spotting.id);
            }
            allInstancesByScoreQbE.emplace(spotting.scoreQbE,spotting.id);
        }
        instancesToAddQbEByLocation.clear();
        //instancesToAddQbE.clear();
        tracer = instancesByScore.begin();
    //debugState();

        cvtMax = maxScoreQbE;
    }

    //For debugging
    int initSize = instancesByScore.size();

    if (numSpottingsQbEMax == -1)
        numSpottingsQbEMax = spottings->size();
    for (Spotting& spotting : *spottings)
    {
        assert(spotting.tlx!=-1 && spotting.scoreQbE!=MAX_FLOAT && spotting.scoreQbE==spotting.scoreQbE);
        //Update max and min
        if (spotting.scoreQbE>maxScoreQbE)
        {
            if (falseMean==maxScoreQbE && falseVariance==1.0)
                falseMean=spotting.scoreQbE;
            maxScoreQbE=spotting.scoreQbE;
        }
        if (spotting.scoreQbE<minScoreQbE)
        {
            if (trueMean==minScoreQbE && trueVariance==1.0)
                trueMean=spotting.scoreQbE;
            minScoreQbE=spotting.scoreQbE;
        }


        //Scan for possibly overlapping (the same) spottings
        //Spotting* best=NULL;
        float ratioOff;//1 at threshold, 0 exactly the same
        auto bestIter = findOverlap(spotting, &ratioOff);
        //if (bestIter != instancesByLocation.end())
        //    if (instancesById.find(bestIter->id)!=instancesById.end())
        //        best=&instancesById.at(bestIter->id);
        //    else

        if (bestIter != instancesByLocation.end())
        {
            //We found matching spotting, so we need to merge them.

            Spotting best = *bestIter; //bestIter comes from instancesByLocation, which are copies already
            //Zagoris et al. A Framework for Efficient Transcription of Historical Documents Using Keyword Spotting would indicate that taking the worse (max) score would yield the best combintation results.
            
            //We can choose to do a weighted averaging based on how far spatially the spottings are from one another.
            //If they are percisely on the same location, we take the max.
            //If they are maximally off (as allowed by threshold), the min score (selected spotting) is used.
            //Interpolate between
            float bestScore = min(spotting.scoreQbE, best.scoreQbE); 
            //float worseScore = max(spotting.scoreQbE, (best)->scoreQbE);
            //float combScore = (1.0f-ratioOff)*worseScore + (ratioOff)*bestScore;


            //Averaging seems to do better with some expriementation with bigrams, shorter
            //float combScore = (spotting.scoreQbE + best.scoreQbE)/2.0f;
            
            //This is most principled. Each ngram has multiple "prototypes," and we want the score of the exemplar closest to the "correct prototype." Also prevents bad exemplars from tugging scores around too much.
            float combScore = bestScore;

            

            //prevent worse score for already approved spottings and prevent better score for rejected ones
            auto iii = classById.find(best.id);
            if (iii != classById.end())
            {
                if ( (iii->second && combScore>best.scoreQbE) ||
                        ((!iii->second) && combScore<best.scoreQbE)
                   )
                {
                    combScore = best.scoreQbE;
                }
            }
            if (combScore != combScore)
                combScore = spotting.scoreQbE;

            assert(combScore==combScore);

            if (useQbE)
            {

                if (spotting.scoreQbE < best.scoreQbE)//then replace the spotting, this happens to skip NaN in the case of a harvested exemplar
                {
                    //Replace spotting.

                    spotting.scoreQbE = combScore;
                    //bool updateWhenInBatch = best.type!=SPOTTING_TYPE_THRESHED && instancesByScore.find(best.id)==instancesByScore.end();
                    //if (updateWhenInBatch)
                        updateMap[best.id]=spotting.id;
#ifdef TEST_MODE
                    //else
                    //    testUpdateMap[best.id]=spotting.id;
#endif
                    //Add this spotting
                    assert(spotting.pageId>=0);
                    instancesById[spotting.id]=spotting;
                    allInstancesByScoreQbE.emplace(spotting.scoreQbE,spotting.id);
                    instancesByLocation.insert(spotting);
                    instancesByLocation.erase(bestIter); //erase by iterator

                    //remove the old one
    assert(((int)instancesByScore.size())>= initSize);
                    allInstancesByScoreQbEErase(best.id);
                    bool removed = instancesByScoreErase(best.id); 
                    if (removed)
                    {
                        //This was queued, we need to readd it
                        
                        if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                        {
                            classById[spotting.id]=true;
                            numberClassifiedTrue++;
                            initSize--;
                        }
                        else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                        {
                            classById[spotting.id]=false;
                            numberClassifiedFalse++;
                            initSize--;
                        }
                        else
                        {
                            instancesByScoreInsert(spotting.id);
                        }
                        tracer = instancesByScore.begin();
    assert(((int)instancesByScore.size())>= initSize);
                    }
                    else if (best.type==SPOTTING_TYPE_THRESHED && classById.at(best.id)==false)
                    {
                        //This occurs after being resurrected. We'll give a better score another chance.

                        //cout<<"{} replaced false spotting "<<spotting.scoreQbE<<endl;
                        //best.type=SPOTTING_TYPE_NONE;
                        //if (classById.at(best.id))
                        //    numberClassifiedTrue--;
                        //else
                            numberClassifiedFalse--;

                        classById.erase(best.id);
                        if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                        {
                            classById[spotting.id]=true;
                            numberClassifiedTrue++;
                        }
                        else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                        {
                            classById[spotting.id]=false;
                            numberClassifiedFalse++;
                        }
                        else
                        {
                            instancesByScoreInsert(spotting.id);
                        }
                        tracer = instancesByScore.begin();
                    }
                    else if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                    {
                        auto iter = classById.find(best.id);
                        //If it's been classified, correct that classification if needed
                        if (iter!=classById.end())
                        {
                            if (!iter->second)
                            {
                                numberClassifiedFalse--;
                                numberClassifiedTrue++;
                            }
                            classById.erase(iter);
                        }
                        classById[spotting.id]=true;
                    }
                    else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                    {
                        auto iter = classById.find(best.id);
                        //If it's been classified, correct that classification if needed
                        if (iter!=classById.end())
                        {
                            if (iter->second)
                            {
                                numberClassifiedFalse++;
                                numberClassifiedTrue--;
                            }
                            classById.erase(iter);
                        }
                        classById[spotting.id]=false;
                    }
                    else
                    {
                        auto iter = classById.find(best.id);
                        //assert(iter!=classById.end()); not true if currently sent to user
                        if (iter!=classById.end())
                        {
                            classById[spotting.id]=iter->second;
                            classById.erase(iter);
                        }
                    }


                    instancesById.erase(best.id);
    //debugState();
    assert(((int)instancesByScore.size())>= initSize);
                }
                else
                {
                    //Use original spotting, just merge score.

                    //becuase we're changing what its indexed by, we need to readd it
                    //auto iter = instancesByScore.find(best.id);
                    int removed = instancesByScoreErase(best.id);
                    instancesById.at(best.id).scoreQbE = combScore;
                    if (removed)
                    {
                        if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                        {
                            classById[best.id]=true;
                            numberClassifiedTrue++;
                            initSize--;
                        }
                        else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                        {
                            classById[best.id]=false;
                            numberClassifiedFalse++;
                            initSize--;
                        }
                        else
                        {
                            instancesByScoreInsert(best.id);
                        }
                    }
                    else if (starts.find(best.id) == starts.end())
                    {
                        if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                        {
                            if (!classById.at(best.id))
                            {
                                numberClassifiedFalse--;
                                numberClassifiedTrue++;
                                classById[best.id]=true;
                            }
                        }
                        else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                        {
                            if (classById.at(best.id))
                            {
                                numberClassifiedTrue--;
                                classById[best.id]=false;
                                numberClassifiedFalse++;
                            }
                        }
                    }
    //debugState();
    assert(((int)instancesByScore.size())>= initSize);
                }
            }
            else
            {
                //QbS mode

                //because the 'best' comes from instancesByLocation, which are copies, not references to isntanceById
                if (instancesById.find(best.id) != instancesById.end())
                {
                    //This is a QbS instance, we just save the new BB and merge scores.

                    Spotting& bestSpot = instancesById.at(best.id);
                    if (combScore<best.scoreQbE || best.scoreQbE!=best.scoreQbE)
                    {
                        bestSpot.boxQbE=make_tuple(spotting.tlx, spotting.tly, spotting.brx, spotting.bry);
                    }
                    allInstancesByScoreQbEErase(bestSpot.id);

                    bestSpot.scoreQbE = combScore;
                    allInstancesByScoreQbE.emplace(bestSpot.scoreQbE,bestSpot.id);
                    if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                    {
                        if(instancesByScoreErase(bestSpot.id))
                            initSize--;
                        auto iter = classById.find(best.id);
                        if (iter != classById.end())
                        {
                            if (!iter->second)
                            {
                                iter->second=true;
                                numberClassifiedFalse--;
                                numberClassifiedTrue++;
                            }
                        }
                        else
                        {
                            classById[bestSpot.id]=true;
                            numberClassifiedTrue++;
                        }
                    }
                    else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                    {
                        if(instancesByScoreErase(bestSpot.id))
                            initSize--;
                        auto iter = classById.find(best.id);
                        if (iter != classById.end())
                        {
                            if (iter->second)
                            {
                                iter->second=false;
                                numberClassifiedFalse++;
                                numberClassifiedTrue--;
                            }
                        }
                        else
                        {
                            classById[bestSpot.id]=false;
                            numberClassifiedFalse++;
                        }
                    }
                    //else
                    //{
                    //    instancesByScoreInsert(bestSpot.id);
                    //}
    //debugState();
    assert(((int)instancesByScore.size())>= initSize);
                }
                else
                {
                    //This is an unqueued QbE instance. Decide to replace
                    pair<multiset<Spotting,tlComp>::iterator,multiset<Spotting,tlComp>::iterator> range = instancesToAddQbEByLocation.equal_range(best);
                    for (multiset<Spotting,tlComp>::iterator iter=range.first; iter!=range.second; iter++)
                        if (iter->id==best.id)
                        {
                            if (combScore<best.scoreQbE || best.scoreQbE!=best.scoreQbE)
                            {
                                //Keep original, just merge score.

                                //iter->scoreQbE = combScore; wont compile
                                //so hack
                                Spotting hack=*iter;
                                instancesToAddQbEByLocation.erase(iter);
                                hack.scoreQbE = combScore;
                                if (spotting.type==SPOTTING_TYPE_TRANS_FALSE || spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                                    hack.type = spotting.type;
                                instancesToAddQbEByLocation.insert(hack);
                            }
                            else
                            {
                                //Replace the original one.
                                instancesToAddQbEByLocation.erase(iter);
                                spotting.scoreQbE = combScore;
                                instancesToAddQbEByLocation.insert(spotting);
                            }

                            break;
                        }


    //debugState();
    assert(((int)instancesByScore.size())>= initSize);
                }
            }
        }
        else
        {
            //This is a new spotting.
            if (useQbE)
            {
                instancesById[spotting.id]=spotting;
                instancesByLocation.insert(instancesById.at(spotting.id));
                allInstancesByScoreQbE.emplace(spotting.scoreQbE,spotting.id);
                if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
                {
                    classById[spotting.id]=true;
                    numberClassifiedTrue++;
                }
                else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                {
                    classById[spotting.id]=false;
                    numberClassifiedFalse++;
                }
                else
                {
                    instancesByScoreInsert(spotting.id);
                }
    //debugState();
    assert(((int)instancesByScore.size())>= initSize);
            }
            else
            {
                instancesToAddQbEByLocation.insert(spotting);
            }
        }

    }
    bool check_didRemove=false;
    //debugState();
    while (allInstancesByScoreQbE.size() > numSpottingsQbEMax)
    {
        auto worstIter = allInstancesByScoreQbE.end();
        worstIter--;
        unsigned long iid=worstIter->second;
        if (useQbE || instancesById.find(iid)!=instancesById.end())
        {
            instancesByScoreErase(iid);
            instancesByLocationErase(iid);
            instancesById.erase(iid);
            classById.erase(iid);
            check_didRemove=true;
            kickedOut.insert(iid);
        }
        auto check = allInstancesByScoreQbE.erase(worstIter);
        assert(check == allInstancesByScoreQbE.end());

    }
    auto worstIter = allInstancesByScoreQbE.end();
    worstIter--;
    maxScoreQbE=worstIter->first;
    if (useQbE)
    {
        bool allWereSent = allBatchesSent;
        EMThresholds();
#if defined(TEST_MODE) && defined(GRAPH_SPOTTING_RESULTS)
        if (fresh)
        {
            cv::Mat undoneW(1,undoneGraph.cols,CV_8UC3);
            undoneW = cv::Scalar(255,255,255);
            undoneGraph.push_back(undoneW);
            cv::Mat fullW(1,fullGraph.cols,CV_8UC3);
            fullW = cv::Scalar(255,255,255);
            fullGraph.push_back(fullW);
            setDebugInfo(NULL);
            fullW= cv::Mat(1,fullGraph.cols,CV_8UC3);
            fullW = cv::Scalar(255,255,255);
            fullGraph.push_back(fullW);
        }
#endif
        if (allWereSent && !allBatchesSent)
        {
            //RESURRECT!
            allBatchesSent=false;
            return true;
        }
    }
        //Go through spottings
        //If intersection, use previous label and label it instantly, but use the given score, dont put in "queue" (byScores)
        //Else, add it in normally
        //All non-intersected old ones need to have their score not count anymore (but save in case of later resurrection)
    //debugState();
    assert(((int)instancesByScore.size())>= initSize || check_didRemove);
    delete spottings;
    return false;
}
#endif


void SpottingResults::autoApprove(vector<Spotting> toApprove, vector<Spotting>* ret)
{
    for (auto iter=instancesByScore.begin(); iter!=instancesByScore.end(); iter++)
    {
        Spotting& s = instancesById[iter->second];
        for (auto bounds=toApprove.begin(); bounds!=toApprove.end();)
        {
            if (s.type == SPOTTING_TYPE_NONE &&
                    s.wordId==bounds->wordId && 
                    (min(s.brx,bounds->brx)-max(s.tlx,bounds->tlx))/(s.brx-s.tlx) > AUTO_APPROVE_THRESH)
            {
                //approve(s);
                s.type=SPOTTING_TYPE_AUTO_APPROVED;
                ret->push_back(s);
                instancesByScore.erase(iter);
                classById[s.id]=true;
                //
                bounds=toApprove.erase(bounds);
            }
            else
                bounds++;
        }
        if (toApprove.size()==0)
            break;
    }
}

void SpottingResults::save(ofstream& out)
{
    //debugState();
    out<<"SPOTTINGRESULTS"<<endl;
    out<<ngram<<"\n";
    out<<batchesSinceChange<<"\n";
    out<<numberClassifiedTrue<<"\n";
    out<<numberClassifiedFalse<<"\n";
    out<<numberAccepted<<"\n";
    out<<numberRejected<<"\n";
    out<<_id.load()<<"\n";
    out<<id<<"\n";
    out<<acceptThreshold<<"\n"<<rejectThreshold<<"\n";
    out<<allBatchesSent<<"\n"<<done<<"\n";
    out<<useQbE<<"\n"<<numComb<<endl;
    out<<trueMean<<"\n"<<trueVariance<<"\n";
    out<<falseMean<<"\n"<<falseVariance<<"\n";
    out<<takeFromTail<<endl;
    out<<tailEnd<<"\n"<<tailEndScore<<endl;
    out<<falseProbWeight<<endl;
    out<<distCheckCounter<<endl;
    out<<lastDifPullFromScore<<"\n"<<momentum<<"\n";
    if (pullFromScore!=pullFromScore)
        pullFromScore = (acceptThreshold+rejectThreshold)/2.0;
    out<<pullFromScore<<"\n"<<takeStd<<"\n";
    out<<maxScoreQbE<<"\n"<<minScoreQbE<<"\n";
    out<<maxScoreQbS<<"\n"<<minScoreQbS<<"\n";
    out<<numLeftInRange<<"\n";

    out<<instancesById.size()<<"\n";
    for (auto& p : instancesById)
    {
        assert(p.second.tlx!=-1);
        p.second.save(out);
    }

    out<<instancesByScore.size()+starts.size()<<"\n";
    out.flush();
    for (auto p : instancesByScore)
    {
        unsigned long sid = p.second;
        //unsigned long sid = s->id;
        if (instancesById.find(sid) == instancesById.end())
        {
            if (kickedOut.find(sid)!=kickedOut.end())
            {
                out<<"k\n";
                continue;
            }
            while (updateMap.find(sid) != updateMap.end())
                sid=updateMap.at(sid);
            if (kickedOut.find(sid)!=kickedOut.end())
            {
                out<<"k\n";
                continue;
            }
            assert(instancesById.find(sid) != instancesById.end());
        }
        out<<sid<<"\n";
        out.flush();
    }
    for (auto& p : starts)
    {
        unsigned long sid = p.first;
        if (instancesById.find(sid) == instancesById.end())
        {
            if (kickedOut.find(sid)!=kickedOut.end())
            {
                out<<"k\n";
                continue;
            }
            while (updateMap.find(sid) != updateMap.end())
                sid=updateMap.at(sid);
            if (kickedOut.find(sid)!=kickedOut.end())
            {
                out<<"k\n";
                continue;
            }
            assert(instancesById.find(sid) != instancesById.end());
        }
        out<<sid<<"\n";
    }

    out<<classById.size()<<"\n";
    for (auto p : classById)
    {
        out<<p.first<<"\n";
        out<<p.second<<"\n";
    }



    out<<instancesToAddQbEByLocation.size()<<endl;
    for (const Spotting& s : instancesToAddQbEByLocation)
    {
        s.save(out);
    }
    out<<numSpottingsQbEMax<<endl;

    out<<runningClassifications.size()<<endl;
    for (bool c : runningClassifications)
        out<<(c?"1":"0")<<endl;
    out<<badInARow<<endl;

    //skip updateMap as no feedback will be recieved.
    //skip tracer as we will just refind it
    out<<contextPad<<"\n";

    out<<batchTracking.size()<<endl;
    for (auto t : batchTracking)
    {
        out<<get<0>(t)<<"\n"<<get<1>(t)<<"\n"<<get<2>(t)<<"\n"<<get<3>(t)<<"\n"<<get<4>(t)<<endl;
    }
    //debugState();
}

SpottingResults::SpottingResults(ifstream& in, PageRef* pageRef) 
{
    string line;
    getline(in,line);
    assert(line.compare("SPOTTINGRESULTS")==0);
    getline(in,ngram);
    getline(in,line);
    batchesSinceChange = stoi(line);
    getline(in,line);
    numberClassifiedTrue = stoi(line);
    getline(in,line);
    numberClassifiedFalse = stoi(line);
    getline(in,line);
    numberAccepted = stoi(line);
    getline(in,line);
    numberRejected = stoi(line);
    getline(in,line);
    _id.store(stoul(line));
    getline(in,line);
    id = stoul(line);
    in >> acceptThreshold;
    getline(in,line);
    in >> rejectThreshold;
    getline(in,line);
    getline(in,line);
    allBatchesSent = stoi(line);
    getline(in,line);
    done = stoi(line);
    getline(in,line);
    useQbE = stoi(line);
    getline(in,line);
    numComb = stoi(line);
    in >> trueMean;
    getline(in,line);
    in >> trueVariance;
    getline(in,line);
    in >> falseMean;
    getline(in,line);
    in >> falseVariance;
    getline(in,line);
    getline(in,line);
    takeFromTail = stoi(line);
    in >> tailEnd;
    getline(in,line);
    in >> tailEndScore;
    getline(in,line);
    in >> falseProbWeight;
    getline(in,line);
    getline(in,line);
    distCheckCounter = stoi(line);
    in >> lastDifPullFromScore;
    getline(in,line);
    in >> momentum;
    getline(in,line);
    in >> pullFromScore;
    getline(in,line);
    in >> takeStd;
    getline(in,line);
    in >> maxScoreQbE;
    getline(in,line);
    in >> minScoreQbE;
    getline(in,line);
    in >> maxScoreQbS;
    getline(in,line);
    in >> minScoreQbS;
    getline(in,line);
    getline(in,line);
    numLeftInRange = stoi(line);

    getline(in,line);
    int size = stoi(line);
    for (int i=0; i<size; i++)
    {
        Spotting s(in,pageRef);
        assert(pageRef->verify(s.pageId,s.tlx,s.tly,s.brx,s.bry));
        instancesById[s.id]=s;
        instancesByLocation.insert(instancesById.at(s.id));
        if (s.scoreQbE==s.scoreQbE && s.scoreQbE!=MAX_FLOAT)
            allInstancesByScoreQbE.emplace(s.scoreQbE,s.id);
    }
    getline(in,line);
    size = stoi(line);
    for (int i=0; i<size; i++)
    {
        getline(in,line);
        if (line[0] != 'k')
        {
            unsigned long sid = stoul(line);
            if (!instancesByScoreContains(sid))
                instancesByScoreInsert(sid);
        }
    }
    getline(in,line);
    size = stoi(line);
    for (int i=0; i<size; i++)
    {
        getline(in,line);
        unsigned long sid = stoul(line);
        getline(in,line);
        classById[sid] = stoi(line);
    }
    tracer = instancesByScore.begin();


    getline(in,line);
    size = stoi(line);
    for (int i=0; i<size; i++)
    {
        Spotting s(in,pageRef);
        //instancesToAddQbE.push_back(s);
        instancesToAddQbEByLocation.insert(s);
    }
    getline(in,line);
    numSpottingsQbEMax = stoi(line);

    getline(in,line);
    size = stoi(line);
    for (int i=0; i<size; i++)
    {
        getline(in,line);
        runningClassifications.push_back(stoi(line));
    }
    getline(in,line);
    badInARow = stoi(line);

    getline(in,line);
    contextPad = stoi(line);
#ifdef GRAPH_SPOTTING_RESULTS
    undoneGraphName="save/graph/graph_undone_"+ngram+".png";
    //cv::namedWindow(undoneGraphName);
    fullGraphName="save/graph/graph_full_"+ngram+".png";
#endif
#ifdef TEST_MODE
    rtn=0;
    atn=0;
#endif //TEST_MODE
    getline(in,line);
    size = stoi(line);
    batchTracking.reserve(size);
    for (int i=0; i<size; i++)
    {
        float p,a,rp,ra;
        int s;
        in >> p >> a >> s >> rp >> ra;
        getline(in,line);
        batchTracking.emplace_back(p,a,s,rp,ra);
    }
    //debugState();
    
}
