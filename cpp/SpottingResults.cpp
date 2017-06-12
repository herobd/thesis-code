#include "SpottingResults.h"

#include <ctime>
#include <random>


atomic_ulong Spotting::_id;
atomic_ulong SpottingResults::_id;

SpottingResults::SpottingResults(string ngram, int contextPad) : 
    //instancesByScore(scoreCompById(&(this->instancesById),false)),
    ngram(ngram), contextPad(contextPad)
{
    id = ++_id;
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
    
    done=false;

    numComb=0;

    //pullFromScore=splitThreshold;
    momentum=1.2;
    lastDifPullFromScore=0;
    batchesSinceChange=0;

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

    
    for (auto iter=instancesByLocation.begin(); iter!=instancesByLocation.end(); iter++)
    {
        assert(instancesById.find(iter->id) != instancesById.end());
    }
    set<unsigned long> check;
    for (auto iter=instancesByScore.begin(); iter!=instancesByScore.end(); iter++)
    {
        auto iter2=instancesById.find(iter->second);
        assert(iter2 != instancesById.end());
        assert(check.find(iter->second) == check.end());
        check.insert(iter->second);
    }
    check.clear();
    for (auto iter=allInstancesByScoreQbE.begin(); iter!=allInstancesByScoreQbE.end(); iter++)
    {
        auto iter2=instancesById.find(iter->second);
        assert(iter2 != instancesById.end());
        assert(iter->first == iter2->second.scoreQbE);
        assert(check.find(iter->second) == check.end());
        check.insert(iter->second);
    }

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
        if (p.second.scoreQbE==p.second.scoreQbE && p.second.scoreQbE!=MAX_FLOAT)
        {
            auto range = allInstancesByScoreQbE.equal_range(p.second.scoreQbE);
            assert(range.first!=range.second);
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
            assert(present || starts.find(p.first)!=starts.end());
        }

    }
    //assert(maxS==maxScore() && minS==minScore());
}

#ifdef TEST_MODE
void SpottingResults::setDebugInfo(SpottingsBatch* b)
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
        bool t=false;
        if (spotting.gt==1)
            t=true;
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
            precAtPull+=t?1:0;
        }
        if (acceptT)
        {
            countAcceptT++;
            precAcceptT+=t?1:0;
        }
        if (rejectT)
        {
            countRejectT++;
            precRejectT+=t?1:0;
        }
        if (betweenT)
        {
            countBetweenT++;
            precBetweenT+=t?1:0;
        }
        if (!acceptT && atPull)
        {
            if (t)
                tBetBefore++;
            else
                fBetBefore++;
        }

        if (!rejectT && !atPull)
        {
            if (t)
                tBetAfter++;
            else
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
    cout<<"["<<id<<"]:"<<ngram<<" serving batch."<<endl;
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

    b->addDebugInfo(precAtPull,precAcceptT,precRejectT,precBetweenT, countAcceptT, countRejectT, countBetweenT);
    //cout<<"precAtPull: "<<precAtPull<<"\t
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

SpottingsBatch* SpottingResults::getBatch(bool* done, unsigned int num, bool hard, unsigned int maxWidth, int color, string prevNgram, bool need) {
    debugState();

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
    setDebugInfo(ret);
#endif
    set<float> scoresToDraw;
    //normal_distribution<float> distribution(pullFromScore,((pullFromScore-acceptThreshold)+(rejectThreshold-pullFromScore))/4.0);
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
#ifdef TEST_MODE
    cout <<"\ngetBatch, from: ";
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
    if (instancesByScore.size()<=1)
        *done=true;
    else
    {
        if (instancesById.at(iterR->second).score(useQbE) > rejectThreshold)
        {
            if (iterR==instancesByScore.begin())
                *done=true;
            else
            {
                iterR--;
                assert(instancesById.find(iterR->second) != instancesById.end());
                if (instancesById.at(iterR->second).score(useQbE) < acceptThreshold)
                    *done=true;
            }
        }
        else if (instancesById.at(iterR->second).score(useQbE) < acceptThreshold)
        {
            iterR++;
            if (iterR==instancesByScore.end())
                *done=true;
            else
            {
                assert(instancesById.find(iterR->second) != instancesById.end());
                if (instancesById.at(iterR->second).score(useQbE) > rejectThreshold)
                    *done=true;
            }
        }

        
        if (batchesSinceChange++ > CHECK_IF_BAD_SPOTTING_START)
        {
            if (numberClassifiedTrue/(numberClassifiedTrue+numberClassifiedFalse+0.0) < CHECK_IF_BAD_SPOTTING_THRESH)
                this->done=true;
        }
        
        if (takeFromTail && runningClassificationTrueAverage() > TAIL_CONTINUE_THRESH)
            *done=false;//Just keep taking from the top until we aren't getting a lot of trues
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
    
    debugState();
    return ret;
}

float SpottingResults::runningClassificationTrueAverage()
{
    float ret=0;
    for (bool c : runningClassifications)
        ret += c?1:0;
    return ret/runningClassifications.size();
}

void SpottingResults::updateRunningClassifications(const vector<int>& newClassifications)
{
    for (int c : newClassifications)
        if (c==0 || c==1)
            runningClassifications.push_back((bool)c);
    while (runningClassifications.size()>RUNNING_CLASSIFICATIONS_COUNT)
        runningClassifications.pop_front();
}

vector<Spotting>* SpottingResults::feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent, vector<pair<unsigned long,string> >* retRemove)
{
    debugState();
    /*cout << "fed: ";
    for (int i=0; i<ids.size(); i++)
    {   
        cout << ids[i]<<", ";
    }
    cout<<endl;*/
    updateRunningClassifications(userClassifications);
    
    vector<Spotting>* ret = new vector<Spotting>();
    int swing=0;
    for (unsigned int i=0; i< ids.size(); i++)
    {
        unsigned long id = stoul(ids[i]);
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
#ifdef NO_NAN
        GlobalK::knowledge()->accepted();
#endif
            if (!resent || !iterClass->second)
            {
                ret->push_back(instancesById.at(id)); //otherwise we've already sent it
            }
            classById[id]=true;
        }
        else if (userClassifications[i]==0)
        {
            swing--;
            numberClassifiedFalse++;
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

    if (this->done)
    {
        *done=true;
        return ret;
    }
    bool allWereSent = allBatchesSent; 
    EMThresholds(swing);
  


#ifdef TEST_MODE
    cout<<"["<<id<<"]:"<<ngram<<", all sent: "<<allBatchesSent<<", waiting for "<<starts.size()<<", num left "<<instancesByScore.size()<<endl;
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
        cout <<"["<<id<<"]:"<<ngram<<", all batches sent, cleaning up"<<endl;
        int numAutoAccepted=0;
        int trueAutoAccepted=0;
        int numAutoRejected=0;
        int trueAutoRejected=0;
#endif
        tracer = instancesByScore.begin();
        assert(instancesById.find(tracer->second) != instancesById.end());
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
        cout<<"["<<id<<"]:"<<ngram<<" AUTO accepted["<<numAutoAccepted<<"]: "<<(0.0+trueAutoAccepted)/numAutoAccepted<<"  rejected["<<numAutoRejected<<"]: "<<(0.0+trueAutoRejected)/numAutoRejected<<endl;
#endif        
        instancesByScore.clear();
        tracer = instancesByScore.begin();
    }
    else if (!allBatchesSent && allWereSent)
    {
        *done=-1;
#ifdef TEST_MODE
        cout<<"SpottingResults ["<<id<<"]:"<<ngram<<" has more batches and will be re-enqueued"<<endl;
#endif
    }
    debugState();
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

    

bool SpottingResults::EMThresholds(int swing)
{
    
    debugState();
    assert(instancesById.size()>1);
    assert(maxScore()!=-999999);
    bool init = acceptThreshold==-1 && rejectThreshold==-1;
    float oldMidScore = acceptThreshold + (rejectThreshold-acceptThreshold)/2.0;
    /*This will likely predict very narrow and distinct distributions
     *initailly. This should be fine as we sample from the middle of
     *the thresholds outward.
     */
    
#ifdef TEST_MODE 
    //test
    int displayLen=90;
    vector<int> histogramCP(displayLen);
    vector<int> histogramCN(displayLen);
    vector<int> histogramGP(displayLen);
    vector<int> histogramGN(displayLen);
    //test
    float actualModelDif=-99999;
#endif
    trueFalseDivide=99999;
    
    if (init)
    {
        /*
        */
        float usedSum=0;
        vector<float> usedValues;
        vector<float> allValues;
        for (auto p : instancesById)
        {
            if (p.second.score(useQbE)!=p.second.score(useQbE) || p.second.score(useQbE)==MAX_FLOAT)
                continue;
            float score = p.second.score(useQbE); ///useQbE?logCvt(p.second.score(useQbE)):p.second.score(useQbE);
            allValues.push_back(score); 
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
        for (float v : allValues)
            if (v<tailEndScore) ///if ( (!useQbE&&v<tailEndScore) || (useQbE&&v>tailEndScore) )
                countTail++;
        //This represent the density that the model says should be after we have any examples. Given more examples (not threshing the spotting results) this would be in our data
        float cutOffDensity = (1-PHI((maxScore()-usedMean)/usedStd)); ///useQbE?1:(1-PHI((maxScore()-usedMean)/usedStd));

        //We then estimate how many instances we would have if this part wern't cut off
        float estTotalCount = allValues.size() + allValues.size()*(1-cutOffDensity);
        assert(estTotalCount >= allValues.size());

        //We use this to compute what the density of the tail region is
        float actualDensity = countTail/estTotalCount;

        //Compute expected (model) density of tail
        float modelDensity = PHI(tailEnd);

        //If difference is above a threshold, we have a good True distribution
#ifdef TEST_MODE 
        actualModelDif=actualDensity-modelDensity;
        assert(actualModelDif==actualModelDif);
#endif
        if (actualDensity-modelDensity > TAIL_DENSITY_TRUE_THRESHOLD)
        {
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
            takeFromTail=true;
        }

    }
    
    //expectation

    /*
    bool strictPredict=classById.size()/(0.0+numLeftInRange)>0.1;
#if defined(TEST_MODE) && defined(GRAPH_SPOTTING_RESULTS)
    if (strictPredict && init)
    {
        cv::Mat undoneW(1,undoneGraph.cols,CV_8UC3);
        undoneW = cv::Scalar(255,0,0);
        undoneGraph.push_back(undoneW);
        cv::Mat fullW(1,fullGraph.cols,CV_8UC3);
        fullW = cv::Scalar(255,0,0);
        fullGraph.push_back(fullW);
    }
#endif
    */
    //bool initV=init;
    //while (1)//this terminates unless we are initailizing and strictPredict, in which case it will go again with strictPredict off
    //{
#ifdef TEST_MODE
        for (int i=0; i<displayLen; i++)
        {
            histogramCP[i]=0;
            histogramGP[i]=0;
            histogramCN[i]=0;
            histogramGN[i]=0;;
        }
#endif        
        
        //map<unsigned long, bool> expected;
        vector<float> expectedTrue;
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
                if (classById[id])
                {
                    expectedTrue.push_back(score);
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
                double trueProb = NPD(instancesById.at(id).score(useQbE),trueMean,trueVariance);//exp(-1*pow(score - trueMean,2)/(2*trueVariance));
                double falseProb = NPD(instancesById.at(id).score(useQbE),falseMean,falseVariance);//exp(-1*pow(score - falseMean,2)/(2*falseVariance));
                if (takeFromTail)
                {
                    trueProb=0;
                    falseProb=1;
                }
                else if (init)
                {
                    trueProb = score<trueFalseDivide;
                    falseProb = score>trueFalseDivide;
                }
                if (trueProb>falseProb)
                {
                    sumTrue+=score;
                    expectedTrue.push_back(score);
                    
#ifdef TEST_MODE
                    //test
                    //if (bin>=0) histogramGP.at(bin)++;
#endif        
                }
                else
                {
                    expectedFalse.push_back(score);
                    sumFalse+=score;
                    
#ifdef TEST_MODE
                    //test
                    //if (bin>=0) histogramGN.at(bin)++;
#endif        
                }
            }
        }
        
        //maximization
        if (expectedTrue.size()!=0)
            trueMean=sumTrue/expectedTrue.size();
        if (expectedFalse.size()!=0)
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
        }
        else
        {
            //just take region of intersection between distribitons
            float delta = (maxScore()-minScore())/1000;
            float x1=minScore();
            while (1)
            {
                float density = PHI(x1,falseMean,falseStd)*expectedFalse.size();
                if (density>ACCEPT_THRESH)
                {
                    x1-=delta;
                    break;
                }
                x1+=delta;
                assert(x1<maxScore());
            }
            float x2=x1+delta;
            float constOverlap = PHI(x1,trueMean,trueStd)*expectedTrue.size() - PHI(x1,falseMean,falseStd)*expectedFalse.size();
            while (1)
            {
                float dif = PHI(x2,falseMean,falseStd)*expectedFalse.size() - PHI(x2,trueMean,trueStd)*expectedTrue.size() + constOverlap;
                if (dif > 0)
                {
                    x2-=delta;
                    break;
                }
                x2+=delta;
                assert(x2<maxScore());
            }

            acceptThreshold = x1;
            rejectThreshold = x2;

            /*

            //set new thresholds
            float numStdDevs = 1;// + ((1.0+min((int)instancesById.size(),50))/(1.0+min((int)classById.size(),50)));//Use less as more are classified, we are more confident. Capped to reduce effect of many returned instances bogging us down.
            float acceptThreshold1 = falseMean-numStdDevs*falseStd;
            float rejectThreshold1 = trueMean+numStdDevs*trueStd;
            float acceptThreshold2 = trueMean-numStdDevs*trueStd;
            float rejectThreshold2 = falseMean-numStdDevs*falseStd;
            //float prevAcceptThreshold=acceptThreshold;
            //float prevRejectThreshold=rejectThreshold;
            if (falseVariance!=0 && trueVariance!=0)
            {
                acceptThreshold = max( min(acceptThreshold1,acceptThreshold2), (useQbE?logCvt(minScore()):minScore()));
                //rejectThreshold = min( max(rejectThreshold1,rejectThreshold2), maxScore());
                rejectThreshold = min( rejectThreshold2, (useQbE?logCvt(maxScore()):maxScore()));//This seems to work better
            }
            else if (falseVariance==0)
            {
                acceptThreshold = acceptThreshold2;
                rejectThreshold = trueMean+2*numStdDevs*trueStd;
            }
            else
            {
                rejectThreshold = rejectThreshold2;
                acceptThreshold = falseMean-2*numStdDevs*falseStd;
            }
#ifdef TEST_MODE
            
            if (rejectThreshold==rejectThreshold1)
                rtn=1;
            else if (rejectThreshold==rejectThreshold2)
                rtn=2;
            else
                rtn=0;
            
            if (acceptThreshold==acceptThreshold1)
                atn=1;
            else if (acceptThreshold==acceptThreshold2)
                atn=2;
            else
                atn=0;
#endif //TEST_MODE
            if (useQbE)
          {
              acceptThreshold=exp(acceptThreshold);
              rejectThreshold=exp(rejectThreshold);
          }
            */
        }


        assert(rejectThreshold==rejectThreshold && acceptThreshold==acceptThreshold);
#ifdef TEST_MODE
        if (init)
            saveHistogram(actualModelDif);
#endif //TEST_MODE

        /*if (!init)
        {
            float difAcceptThreshold = acceptThreshold-prevAcceptThreshold;
            if ( (difAcceptThreshold>0 && lastDifAcceptThreshold>0) || (difAcceptThreshold<0 && lastDifAcceptThreshold<0) )
            {
                acceptThreshold+=momentum*lastDifAcceptThreshold;
                difAcceptThreshold = acceptThreshold-prevAcceptThreshold;
            }
            lastDifAcceptThreshold=difAcceptThreshold;

            float difRejectThreshold = rejectThreshold-prevRejectThreshold;
            if ( (difRejectThreshold>0 && lastDifRejectThreshold>0) || (difRejectThreshold<0 && lastDifRejectThreshold<0) )
            {
                rejectThreshold+=momentum*lastDifRejectThreshold;
                difRejectThreshold = rejectThreshold-prevRejectThreshold;
            }
            lastDifRejectThreshold=difRejectThreshold;
        }*/
#ifdef TEST_MODE
        cout <<ngram<<": adjusted threshs, now "<<acceptThreshold<<" <> "<<rejectThreshold<<"    computed with std devs of: f:"<<sqrt(falseVariance)<<", t:"<<sqrt(trueVariance)<<endl;
#endif        
        /*if (!init || !initV)
            break;
        
        if (fabs(pullFromScore-trueMean)<0.05)
            break;
        else
            initV=false;
        pullFromScore = trueMean;*/

        //if (!init || !strictPredict)
        //    break;
        //strictPredict=false;
    //}
    float prevPullFromScore = pullFromScore;
    pullFromScore = (acceptThreshold+rejectThreshold)/2.0;// -sqrt(trueVariance);
    //pullFromScore = trueMean-sqrt(trueVariance);
    /*if (!init)
    {
        float difPullFromScore = pullFromScore-prevPullFromScore;
#ifdef TEST_MODE
        cout<<"orig pull dif="<<difPullFromScore<<",  ";
#endif
        //if ( (pull>0 && lastDifPullFromScore>0) || (difPullFromScore<0 && lastDifPullFromScore<0) )
        //{
        //    pullFromScore+=momentum*lastDifPullFromScore;
        //    difPullFromScore = pullFromScore-prevPullFromScore;
        //}
        if ((swing>0 && difPullFromScore<0) || (swing<0 && difPullFromScore>0))
            pullFromScore=prevPullFromScore;
        if ((swing>0 && lastDifPullFromScore>0) || (swing<0 && lastDifPullFromScore<0))
        {
            pullFromScore+=momentum*lastDifPullFromScore;
        }
        else 
        {
            pullFromScore+= (swing/10.0)*fabs(difPullFromScore+lastDifPullFromScore);
        }
        difPullFromScore = pullFromScore-prevPullFromScore;
#ifdef TEST_MODE
        cout<<"final pull="<<pullFromScore<<" dif="<<difPullFromScore<<",  swing="<<swing<<",  lastDif="<<lastDifPullFromScore<<endl;
#endif
        lastDifPullFromScore=difPullFromScore;
    }*/

    //safe gaurd
    if (pullFromScore>rejectThreshold)
        pullFromScore=rejectThreshold;
    else if (pullFromScore<acceptThreshold)
        pullFromScore=acceptThreshold;

//cout <<"true mean "<<trueMean<<" true var "<<trueVariance<<endl;
//cout <<"false mean "<<falseMean<<" false var "<<falseVariance<<endl;


//cout <<"adjusted threshs, now "<<acceptThreshold<<" <> "<<rejectThreshold<<"    computed with std dev of: "<<numStdDevs<<endl;
/*//a historgram visualization

int falseMeanBin=(displayLen-1)*(falseMean-minScore())/(maxScore()-minScore());
int falseVarianceBin1=(displayLen-1)*(falseMean-sqrt(falseVariance)-minScore())/(maxScore()-minScore());
int falseVarianceBin2=(displayLen-1)*(falseMean+sqrt(falseVariance)-minScore())/(maxScore()-minScore());
int trueMeanBin=(displayLen-1)*(trueMean-minScore())/(maxScore()-minScore());
int trueVarianceBin1=(displayLen-1)*(trueMean-sqrt(trueVariance)-minScore())/(maxScore()-minScore());
int trueVarianceBin2=(displayLen-1)*(trueMean+sqrt(trueVariance)-minScore())/(maxScore()-minScore());
int acceptThresholdBin=(displayLen-1)*(acceptThreshold-minScore())/(maxScore()-minScore());
int rejectThresholdBin=(displayLen-1)*(rejectThreshold-minScore())/(maxScore()-minScore());
int pullFromScoreBin=(displayLen-1)*(pullFromScore-minScore())/(maxScore()-minScore());
for (int bin=0; bin<displayLen; bin++)
{
    if (falseMeanBin==bin)
        cout <<"F";
    else
        cout <<" ";
    if (falseVarianceBin1==bin || falseVarianceBin2==bin)
        cout <<"f";
        else
            cout <<" ";
        if (trueMeanBin==bin)
            cout <<"T";
        else
            cout <<" ";
        if (trueVarianceBin1==bin || trueVarianceBin2==bin)
            cout <<"t";
        else
            cout <<" ";
        if (rejectThresholdBin==bin)
            cout <<"R";
        else
            cout <<" ";
        if (acceptThresholdBin==bin)
            cout <<"A";
        else
            cout <<" ";
        if (pullFromScoreBin==bin)
            cout <<"P";
        else
            cout <<" ";
        cout <<":";
        for (int c=0; c<histogramCP[bin]; c++)
            cout<<"#";
        for (int c=0; c<histogramGP[bin]; c++)
            cout<<"+";
        for (int c=0; c<histogramCN[bin]; c++)
            cout<<"=";
        for (int c=0; c<histogramGN[bin]; c++)
            cout<<"-";
        cout<<endl;
    }
    cout <<"oooooooooooooooooooooooooooooooooooooooooo"<<endl;
    //*/
    
    
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
    debugState();
    return allBatchesSent;
}


bool SpottingResults::checkIncomplete()
{
    debugState();
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
    debugState();
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
    debugState();
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
    debugState();
}

//combMin
#if USE_QBE
bool SpottingResults::updateSpottings(vector<Spotting>* spottings)
{
#ifdef GRAPH_SPOTTING_RESULTS
    fullInstancesByScore.clear();
#endif
    debugState();
    if (++numComb>GlobalK::numCombThresh(ngram) && !useQbE)
    {
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
#if defined(TEST_MODE) && defined(GRAPH_SPOTTING_RESULTS)
        cv::Mat undoneW(1,undoneGraph.cols,CV_8UC3);
        undoneW = cv::Scalar(255,255,255);
        undoneGraph.push_back(undoneW);
        cv::Mat fullW(1,fullGraph.cols,CV_8UC3);
        fullW = cv::Scalar(255,255,255);
        fullGraph.push_back(fullW);
#endif
    debugState();

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
    assert(instancesByScore.size()>= initSize);
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
    assert(instancesByScore.size()>= initSize);
                    }
                    else if (best.type==SPOTTING_TYPE_THRESHED && classById.at(best.id)==false)
                    {
                        //This occurs after being resurrected. We'll give a better score another chance.

                        //cout<<"{} replaced false spotting "<<spotting.scoreQbE<<endl;
                        //best.type=SPOTTING_TYPE_NONE;
                        if (classById.at(best.id))
                            numberClassifiedTrue--;
                        else
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
                        //It's been classified, correct that classification if needed
                        if (!classById.at(best.id))
                        {
                            numberClassifiedFalse--;
                            numberClassifiedTrue++;
                        }
                        classById.erase(best.id);
                        classById[spotting.id]=true;
                    }
                    else if (spotting.type==SPOTTING_TYPE_TRANS_FALSE)
                    {
                        //It's been classified, correct that classification if needed
                        if (classById.at(best.id))
                        {
                            numberClassifiedTrue--;
                            numberClassifiedFalse++;
                        }
                        classById.erase(best.id);
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
    assert(instancesByScore.size()>= initSize);
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
                    else if (spotting.type==SPOTTING_TYPE_TRANS_TRUE)
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
    //debugState();
    assert(instancesByScore.size()>= initSize);
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
    assert(instancesByScore.size()>= initSize);
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
    assert(instancesByScore.size()>= initSize);
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
    assert(instancesByScore.size()>= initSize);
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
            check_didRemove=true;
            if (!useQbE)
                kickedOut.insert(iid);
        }
        auto check = allInstancesByScoreQbE.erase(worstIter);
        assert(check == allInstancesByScoreQbE.end());
        for (auto p : allInstancesByScoreQbE)
            assert(p.second!=iid);

    }
    auto worstIter = allInstancesByScoreQbE.end();
    worstIter--;
    maxScoreQbE=worstIter->first;
    if (useQbE)
    {
        bool allWereSent = allBatchesSent;
        EMThresholds();
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
    debugState();
    assert(instancesByScore.size()>= initSize || check_didRemove);
    delete spottings;
    return false;
}
#endif

void SpottingResults::save(ofstream& out)
{
    debugState();
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
    out<<lastDifPullFromScore<<"\n"<<momentum<<"\n";
    out<<pullFromScore<<"\n";
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
            if (updateMap.find(sid) != updateMap.end())
                sid=updateMap.at(sid);
            else
            {
                //for (auto& p : instancesById)
                //{
                //    if (&(instancesById[p.first]) == s)
                //        assert(false && "SpottingResults::save ran into non-existant spotting id, though pointer exists in instancesByScore.");
                //}
                assert(false && "SpottingResults::save ran into non-existant spotting id (in instancesByScore)");
            }
        }
        out<<sid<<"\n";
        out.flush();
    }
    for (auto& p : starts)
    {
        unsigned long sid = p.first;
        if (instancesById.find(sid) == instancesById.end())
        {
            if (updateMap.find(sid) != updateMap.end())
                sid=updateMap.at(p.first);
            else
                assert(false && "SpottingResults::save ran into non-existant spotting id (in starts)");
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

    //skip updateMap as no feedback will be recieved.
    //skip tracer as we will just refind it
    out<<contextPad<<"\n";
    debugState();
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
    getline(in,line);
    acceptThreshold = stod(line);
    getline(in,line);
    rejectThreshold = stod(line);
    getline(in,line);
    allBatchesSent = stoi(line);
    getline(in,line);
    done = stoi(line);
    getline(in,line);
    useQbE = stoi(line);
    getline(in,line);
    numComb = stoi(line);
    getline(in,line);
    trueMean = stod(line);
    getline(in,line);
    trueVariance = stod(line);
    getline(in,line);
    falseMean = stod(line);
    getline(in,line);
    falseVariance = stod(line);
    getline(in,line);
    lastDifPullFromScore = stod(line);
    getline(in,line);
    momentum = stod(line);
    getline(in,line);
    pullFromScore = stod(line);
    getline(in,line);
    maxScoreQbE = stod(line);
    getline(in,line);
    minScoreQbE = stod(line);
    getline(in,line);
    maxScoreQbS = stod(line);
    getline(in,line);
    minScoreQbS = stod(line);
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
        unsigned long sid = stoul(line);
        if (!instancesByScoreContains(sid))
            instancesByScoreInsert(sid);
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
    debugState();
    
}
