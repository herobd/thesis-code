#include "MasterQueue.h"

#ifdef TEST_MODE
#include <csignal>
#endif




void MasterQueue::checkIncomplete()
{
    transcribeBatchQueue.checkIncomplete();    
    newExemplarsBatchQueue.checkIncomplete();    
    pthread_rwlock_rdlock(&semResults);
    if (GlobalK::knowledge()->CLUSTER)
    {
        for (auto ele : clusterBatchers)
        {
            
            sem_t* sem = ele.second.first;
            ClusterBatcher* res = ele.second.second;
            sem_wait(sem);
                
            bool incm = res->checkIncomplete();
            sem_post(sem);
            if (incm)
            {
                pthread_rwlock_wrlock(&semResultsQueue);
                clusterBatchersQueue[res->getId()] = ele.second;
                pthread_rwlock_unlock(&semResultsQueue);
            }
            
                

        }
    }
    else
    {
        for (auto ele : results)
        {
            
            sem_t* sem = ele.second.first;
            SpottingResults* res = ele.second.second;
            sem_wait(sem);
                
            bool incm = res->checkIncomplete();
            sem_post(sem);
            if (incm)
            {
                pthread_rwlock_wrlock(&semResultsQueue);
                resultsQueue[res->getId()] = ele.second;
                pthread_rwlock_unlock(&semResultsQueue);
            }
            
                

        }
    }
    pthread_rwlock_unlock(&semResults);
}

MasterQueue::MasterQueue(int contextPad, string savePre) : contextPad(contextPad)
{
    saveDir = savePre+"_extra/";
    if (GlobalK::knowledge()->PHOC_TRANS || GlobalK::knowledge()->CPV_TRANS)
        finish.store(true);
    else
        finish.store(false);
    //sem_init(&semResultsQueue,false,1);
    //sem_init(&semResults,false,1);
    pthread_rwlock_init(&semResultsQueue,NULL);
    pthread_rwlock_init(&semResults,NULL);
#if ROTATE
    pthread_rwlock_init(&semRotate,NULL);
#endif
    kill.store(false);

    transcribeBatchQueue.setContextPad(contextPad);

    //atID=0;
    
    ///testing
    
    /*
    //cv::Mat* &page = new cv::Mat();
    page = cv::imread("/home/brian/intel_index/data/gw_20p_wannot/2700270.tif");//,CV_LOAD_IMAGE_GRAYSCALE
    
    SpottingResults* r0 = new SpottingResults("an",-1,-1);//0.1,0.9);
    
    r0->add(Spotting(1416, 186, 1518, 225, 0, &page, "an", 0.5));
    r0->add(Spotting(258, 360, 342, 399, 0, &page, "an", 0.4));
    r0->add(Spotting(1626, 366, 1704, 396, 0, &page, "an", 0.45));
    
    r0->add(Spotting(801, 189, 927, 231, 0, &page, "an", 0.2));
    r0->add(Spotting(693, 618, 837, 657, 0, &page, "an", 0.3));
    r0->add(Spotting(1563, 612, 1677, 651, 0, &page, "an", 0.35));
    r0->add(Spotting(1260, 702, 1356, 735, 0, &page, "an", 0.41));
    
    r0->add(Spotting(1416, 186, 1518, 225, 0, &page, "an", 0.55));
    r0->add(Spotting(258, 360, 342, 399, 0, &page, "an", 0.44));
    r0->add(Spotting(1626, 366, 1704, 396, 0, &page, "an", 0.47));
    
    r0->add(Spotting(801, 189, 927, 231, 0, &page, "an", 0.22));
    r0->add(Spotting(693, 618, 837, 657, 0, &page, "an", 0.33));
    r0->add(Spotting(1563, 612, 1677, 651, 0, &page, "an", 0.42));
    r0->add(Spotting(1260, 702, 1356, 735, 0, &page, "an", 0.46));
    
    r0->add(Spotting(1416, 186, 1518, 225, 0, &page, "an", 0.7));
    r0->add(Spotting(258, 360, 342, 399, 0, &page, "an", 0.64));
    r0->add(Spotting(1626, 366, 1704, 396, 0, &page, "an", 0.445));
    
    r0->add(Spotting(801, 189, 927, 231, 0, &page, "an", 0.1));
    r0->add(Spotting(693, 618, 837, 657, 0, &page, "an", 0.15));
    r0->add(Spotting(1563, 612, 1677, 651, 0, &page, "an", 0.35));
    r0->add(Spotting(1260, 702, 1356, 735, 0, &page, "an", 0.433));
    
    r0->add(Spotting(1416, 186, 1518, 225, 0, &page, "an", 0.665));
    r0->add(Spotting(258, 360, 342, 399, 0, &page, "an", 0.477));
    r0->add(Spotting(1626, 366, 1704, 396, 0, &page, "an", 0.475));
    
    r0->add(Spotting(801, 189, 927, 231, 0, &page, "an", 0.19));
    r0->add(Spotting(693, 618, 837, 657, 0, &page, "an", 0.222));
    r0->add(Spotting(1563, 612, 1677, 651, 0, &page, "an", 0.399));
    r0->add(Spotting(1260, 702, 1356, 735, 0, &page, "an", 0.388));
    
    //boundary cases
    r0->add(Spotting(24, 3180, 72, 3234, 0, &page, "an", 0.401));
    r0->add(Spotting(1935, 951, 1992, 1005, 0, &page, "an", 0.402));
    
    //false
    r0->add(Spotting(1416, 186, 1518, 225, 0, &page, "an", 0.88));
    r0->add(Spotting(258, 360, 342, 399, 0, &page, "an", 0.423));
    r0->add(Spotting(1626, 366, 1704, 396, 0, &page, "an", 0.77));
    
    r0->add(Spotting(1416, 186, 1518, 225, 0, &page, "an", 0.555));
    r0->add(Spotting(258, 360, 342, 399, 0, &page, "an", 0.433));
    r0->add(Spotting(1626, 366, 1704, 396, 0, &page, "an", 0.444));
    
    r0->add(Spotting(1416, 186, 1518, 225, 0, &page, "an", 0.631));
    r0->add(Spotting(258, 360, 342, 399, 0, &page, "an", 0.51));
    r0->add(Spotting(1626, 366, 1704, 396, 0, &page, "an", 0.53));
    
    addSpottingResults(r0);*/
#if ROTATE 
    testIter=0;	
    rotate_pos=0;
#endif
    //addTestSpottings();
    //accuracyAvg= recallAvg= manualAvg= effortAvg= 0;
    //done=0;
    numCFalse=numCTrue=0;

#ifdef TEST_MODE
    forceNgram="";
#endif
    
    ///end testing
}



/*void MasterQueue::addTestSpottings()
{
    string pageLocation = "/home/brian/intel_index/data/gw_20p_wannot/";
    
    map<string,SpottingResults*> spottingResults;
    
    
    ifstream in("./data/GW_agSpottings_fold1_0.100000.csv");
    assert(in.is_open());
    string line;
    
    //std::getline(in,line);
    //float initSplit=0;//stof(line);//-0.52284769;
    
    while(std::getline(in,line))
    {
        vector<string> strV;
        //split(line,',',strV);
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, ',')) {
            strV.push_back(item);
        }
        
        
        
        string ngram = strV[0];
        string spottingId = strV[0]+strV[1]+":"+to_string(testIter);
        if (spottingResults.find(spottingId)==spottingResults.end())
        {
            spottingResults[spottingId] = new SpottingResults(ngram);
        }
        
        string page = strV[2];
        size_t startpos = page.find_first_not_of(" \t");
        if( string::npos != startpos )
        {
            page = page.substr( startpos );
        }
        if (pages.find(page)==pages.end())
        {
            pages[page] = cv::imread(pageLocation+page);
            assert(pages[page].cols!=0);
        }
        
        int tlx=stoi(strV[3]);
        int tly=stoi(strV[4]);
        int brx=stoi(strV[5]);
        int bry=stoi(strV[6]);
        
        float score=-1*stof(strV[7]);
        bool truth = strV[8].find("true")!= string::npos?true:false;
        
        Spotting spotting(tlx, tly, brx, bry, stoi(page), &pages[page], ngram, score);
        spottingResults[spottingId]->add(spotting);
        test_groundTruth[spottingResults[spottingId]->getId()][spotting.id]=truth;
        test_total[spottingResults[spottingId]->getId()]++;
        if (truth)
            test_totalPos[spottingResults[spottingId]->getId()]++;
    }
    
    for (auto p : spottingResults)
    {
        addSpottingResults(p.second);
        //cout << "added "<<p.first<<endl;
    }
    
    
    testIter++;
}

bool MasterQueue::test_autoBatch()
{
    SpottingsBatch* b = getSpottingsBatch(5, false, 300,0,"");
    if (b==NULL)
        return false;
    vector<string> ids;
    vector<int> userClassifications;
    assert(b->size()>0);
    //cout<<b->ngram<<": ";
    
    for (int i=0; i<b->size(); i++)
    {
        
        unsigned long id = b->at(i).id;
        ids.push_back(to_string(id));
        if (test_groundTruth[b->spottingResultsId][id])
            userClassifications.push_back(1);
        else
            userClassifications.push_back(0);
        
        //cout << b->at(i).score <<" "<<test_groundTruth[b->spottingResultsId][id]<< ", ";
    }
    //cout<<endl;
    vector<Spotting>* tmp = test_feedback(b->spottingResultsId, ids, userClassifications);
    if (tmp!=NULL)
        delete tmp;
    return true;
}*/


BatchWraper* MasterQueue::getBatch(unsigned int numberOfInstances, bool hard, unsigned int maxWidth, int color, string prevNgram)
{
    assert(!GlobalK::knowledge()->MANUAL_LINES);

    int ngramQueueCount;
    pthread_rwlock_rdlock(&semResultsQueue);
    ngramQueueCount=(GlobalK::knowledge()->CLUSTER)?clusterBatchersQueue.size():resultsQueue.size();
    pthread_rwlock_unlock(&semResultsQueue);
    if (((GlobalK::knowledge()->CLUSTER)?clusterBatchers.size():results.size())==0 && !finish.load())
    {
        //we haven't even begun
        return NULL;
    }

    //for setting up, just do some spottings
    if (prevNgram.compare("~")==0)
    {
            SpottingsBatch* batch = getSpottingsBatch(numberOfInstances,hard,maxWidth,color,prevNgram,true);
            if (batch!=NULL)
            {
#ifdef NO_NAN
                GlobalK::knowledge()->sentSpottings();
#endif
                return new BatchWraperSpottings(batch);
            }
    }

    BatchWraper* ret=NULL;
    if (finish.load())
    {
        TranscribeBatch* batch = transcribeBatchQueue.dequeue(maxWidth);
        if (batch!=NULL) 
            ret = new BatchWraperTranscription(batch);
    }
    else
    {

        if (ngramQueueCount < NGRAM_Q_COUNT_THRESH_NEW)
        {
            NewExemplarsBatch* batch=newExemplarsBatchQueue.dequeue(numberOfInstances,maxWidth,color);
            if (batch!=NULL)
                ret = new BatchWraperNewExemplars(batch);
        }
        if (ret==NULL && (ngramQueueCount < NGRAM_Q_COUNT_THRESH_WORD))// || transcribeBatchQueue.size()>TRANS_READY_THRESH)
        {
            TranscribeBatch* batch = transcribeBatchQueue.dequeue(maxWidth);
            if (batch!=NULL) {
                ret = new BatchWraperTranscription(batch);
            }
        }
        if (ret==NULL)
        {
            SpottingsBatch* batch = getSpottingsBatch(numberOfInstances,hard,maxWidth,color,prevNgram,false);
            if (batch!=NULL)
                ret = new BatchWraperSpottings(batch);
        }
        //a second pass without conditions
        if (ret==NULL)
        {
            TranscribeBatch* batch = transcribeBatchQueue.dequeue(maxWidth,true);
            if (batch!=NULL)
                ret = new BatchWraperTranscription(batch);
        }
        if (ret==NULL)
        {
            SpottingsBatch* batch = getSpottingsBatch(numberOfInstances,hard,maxWidth,color,prevNgram,true);
            if (batch!=NULL)
                ret = new BatchWraperSpottings(batch);
        }
        if (ret == NULL)
        {
            NewExemplarsBatch* batch=newExemplarsBatchQueue.dequeue(numberOfInstances,maxWidth,color,true);
            if (batch!=NULL)
                ret = new BatchWraperNewExemplars(batch);
        }
    }

#ifdef NO_NAN
    if (ret==NULL && !finish.load()) 
        ret = new BatchWraperRanOut();
    else if (ret!=NULL)
    {
        if (ret->getType()==SPOTTINGS)
            GlobalK::knowledge()->sentSpottings();
        else if (ret->getType()==TRANSCRIPTION)
            GlobalK::knowledge()->sentTrans();
    }

#endif

    return ret;
}

BatchWraper* MasterQueue::getSpottingsAsBatch(int width, int color, string prevNgram, unsigned long batcherId, vector<unsigned long> spottingIds, string ngram)
{
    if (GlobalK::knowledge()->CLUSTER)
        return _getSpottingsAsBatch(clusterBatchers,width,color,prevNgram,batcherId,spottingIds,ngram);
    else
        return _getSpottingsAsBatch(results,width,color,prevNgram,batcherId,spottingIds,ngram);
}
template <class T>
BatchWraper* MasterQueue::_getSpottingsAsBatch(map<unsigned long, pair<sem_t*,T*> >& batchers, int width, int color, string prevNgram, unsigned long batcherId, vector<unsigned long> spottingIds, string ngram)
{
    BatchWraper* ret;
    pthread_rwlock_rdlock(&semResults);
    if (batchers.find(batcherId)!=batchers.end())
    {
        sem_t* sem=batchers.at(batcherId).first;
        T* res = batchers.at(batcherId).second;
        pthread_rwlock_unlock(&semResults);
        assert(res->ngram.compare(ngram)==0);
        sem_wait(sem);
        ret = res->getSpottingsAsBatch(width,color,prevNgram,spottingIds);
       
        sem_post(sem);
    }
    else
    {
        cout <<"Results not found for: "<<batcherId<<endl;
        pthread_rwlock_unlock(&semResults);
    }
    return ret;
}

SpottingsBatch* MasterQueue::getSpottingsBatch(unsigned int numberOfInstances, bool hard, unsigned int maxWidth, int color, string prevNgram, bool need) 
{
    if (GlobalK::knowledge()->CLUSTER)
    {
        //we don't enforce using the same ngram as previous
        return _getSpottingsBatch<ClusterBatcher>(clusterBatchersQueue,numberOfInstances,hard,maxWidth,color,".",need);
    }
    else
    {
        return _getSpottingsBatch<SpottingResults>(resultsQueue,numberOfInstances,hard,maxWidth,color,prevNgram,need);
    }
}
template <class T>
SpottingsBatch* MasterQueue::_getSpottingsBatch(map<unsigned long, pair<sem_t*,T*> >& batcherQueue,unsigned int numberOfInstances, bool hard, unsigned int maxWidth, int color, string prevNgram, bool need) 
{
    SpottingsBatch* batch=NULL;
    //cout<<"getting rw lock"<<endl;
    pthread_rwlock_rdlock(&semResultsQueue);
    //cout<<"got rw lock"<<endl;
    if (batcherQueue.size()==0)
    {
        pthread_rwlock_unlock(&semResultsQueue);
        return NULL;
    }

    auto iter = batcherQueue.begin();
    int indexHolder=0;
    //for (auto ele : batcherQueue)
    /*for (; iter!=batcherQueue.end(); iter++, indexHolder++)
    {
        T*  res = iter->second.second;
        if (prevNgram.compare(res->ngram)==0)
        {
            break;
        }

#ifdef TEST_MODE
        if (!need && forceNgram.size()>0 && forceNgram.compare(res->ngram)==0)
        {
            cout<<"Forcing ngram : "<<forceNgram<<endl;
            break;
        }
#endif
    }


    if (iter==batcherQueue.end())*/
    {
        iter = batcherQueue.begin();
        indexHolder=0;
#if ROTATE
        int test_loc=0;
        pthread_rwlock_rdlock(&semRotate);
        int use_rotate_pos=rotate_pos;
        if (rotate_pos>BEFORE_ROT*batcherQueue.size())
            use_rotate_pos=0;//in case elements are removed from the queue
        //for (; iter!=batcherQueue.end(); iter++,indexHolder++)
        //    if (++test_loc>use_rotate_pos/2) //TODO add var to control
        //        break;

        //rotate_pos actually holds position in an extended rot space
        
        for (int use=0; use<use_rotate_pos/BEFORE_ROT; use++)
        {
            iter++;
            indexHolder++;
        }
        ///MasterQueue.cpp:457: SpottingsBatch* MasterQueue::_getSpottingsBatch(std::map<long unsigned int, std::pair<sem_t*, T*> >&, unsigned int, bool, unsigned int, int, std::string, bool) [with T = SpottingResults; std::string = std::basic_string<char>]: Assertion `iter!=batcherQueue.end()' failed.
        //assert(iter!=batcherQueue.end());
        if (iter==batcherQueue.end())
        {
            
            iter=batcherQueue.begin();
            
        }
        pthread_rwlock_unlock(&semRotate);
        if (iter==batcherQueue.end())
        {
            pthread_rwlock_unlock(&semResultsQueue);
            return NULL;
        }
#endif
    }

    auto iterStart=iter;
    int startIndexHolder=indexHolder;
    int failsafe=1000;
    do
    {
        
        sem_t* sem = iter->second.first;
        T* res = iter->second.second;

        bool succ = 0==sem_trywait(sem);
        if (succ)
        {
            
            
            //test
#if ROTATE
            int qSize = batcherQueue.size();
            pthread_rwlock_wrlock(&semRotate);
#if LARGE_ROT
            if ((1+rotate_pos)%BEFORE_ROT==0)
            {
                int largeStep=BEFORE_ROT*qSize/5;
                while ((BEFORE_ROT*qSize)%largeStep==0 && largeStep!=1)
                    largeStep--;
                rotate_pos=(rotate_pos+largeStep-1)%(BEFORE_ROT*qSize);
                //ensure +1%BEFORE==0
                rotate_pos -= (rotate_pos+1)%BEFORE_ROT;
            }
#endif
            if (++rotate_pos>BEFORE_ROT*qSize)
                rotate_pos=0;
            //cout<<"rotated to "<<rotate_pos<<endl;
            pthread_rwlock_unlock(&semRotate);
#endif
            //test

            pthread_rwlock_unlock(&semResultsQueue);//I'm going to break out of the loop, so I'll release control
            
            int done=0;
            //cout << "getBatch   prev:"<<prevNgram<<endl;
            batch = res->getBatch(&done,numberOfInstances,hard,maxWidth,color,prevNgram,need);
           
            unsigned long id = res->getId(); 
            sem_post(sem);
            if (done==2 || done==1) //2 when this first runs out. 1 should only occur in race condition as a finised batcher should not be in the queue still...
            {   //cout <<"done in queue "<<endl;
                
                pthread_rwlock_wrlock(&semResultsQueue);
                batcherQueue.erase(id);
                //batcherQueue.erase(iter);//NO, iter is invalid as we released control
                
                pthread_rwlock_unlock(&semResultsQueue);
                
                ///test
                //test_finish();
                ///test
            }
            if (batch!=NULL)
                break;
            else
            {
                pthread_rwlock_rdlock(&semResultsQueue);
                if (batcherQueue.size()==0)
                    break;
                //reset iter as results queue may have changed (race)
                iter=batcherQueue.begin();
                for (int i=0; i<std::min(indexHolder,(int)batcherQueue.size()); i++)
                    iter++;
                iterStart = batcherQueue.begin();
                startIndexHolder = std::min(startIndexHolder,(int)batcherQueue.size());
                for (int i=0; i<startIndexHolder; i++)
                    iterStart++;
            }
            
        }
        //else
        //{
        //    cout <<"couldn't get lock"<<endl;
        //}
        iter++;
        indexHolder++;
        if (iter==batcherQueue.end())
        {
            iter=batcherQueue.begin();
            indexHolder=0;
        }
    } while (iter!=iterStart && failsafe-->0);
    if (batch==NULL)
    {
        pthread_rwlock_unlock(&semResultsQueue);
#ifdef TEST_MODE
        cout<<"no spotting batch from MasterQueue, need:"<<need<<endl;
#endif
    }
    return batch;
}

int MasterQueue::checkLocks()
{
    int c=0;
    for (auto iter=clusterBatchersQueue.begin(); iter!=clusterBatchersQueue.end(); iter++)
    {
        sem_t* sem = iter->second.first;
        ClusterBatcher* res = iter->second.second;

        bool succ = 0==sem_trywait(sem);
        if (!succ)
        {
            c++;
            cout<<res->getId()<<": "<<res->ngram<<" locked"<<endl;
        }
    }
    return c;
}

/*void MasterQueue::test_finish()
{
    if (resultsQueue.size()==0)
    {
        
        test_numDone.clear();
        test_totalPos.clear();
        test_numTruePos.clear();
        test_numFalsePos.clear();
        addTestSpottings();
    }
    
}
void MasterQueue::test_showResults(unsigned long id,string ngram)
{
    cout << "*for "<<id<<" "<<ngram<<endl;
    cout << "* accuracy: "<<(0.0+test_numTruePos[id])/(test_numTruePos[id]+test_numFalsePos[id])<<endl;
    cout << "* recall: "<<(0.0+test_numTruePos[id])/(test_totalPos[id])<<endl;
    cout << "* manual: "<<test_numDone[id]/(0.0+test_total[id])<<endl;
    cout << "* effort: "<<(0.0+test_numTruePos[id])/test_numDone[id]<<endl;
    cout << "* true pos: "<<test_numTruePos[id]<<" false pos: "<<test_numFalsePos[id]<<" total true: "<<test_totalPos[id]<<" total all: "<<test_total[id]<<endl;
    
    accuracyAvg+=(0.0+test_numTruePos[id])/(test_numTruePos[id]+test_numFalsePos[id]);
    recallAvg+=(0.0+test_numTruePos[id])/(test_totalPos[id]);
    manualAvg+=test_numDone[id]/(0.0+test_total[id]);
    effortAvg+=(0.0+test_numTruePos[id])/test_numDone[id];
    done++;
}

//not thread safe
vector<Spotting>* MasterQueue::test_feedback(unsigned long id, const vector<string>& ids, const vector<int>& userClassifications)
{
    assert(ids.size()>0 && userClassifications.size()>0);
    for (int c : userClassifications)
    {
        if (c)
            numCTrue++;
        else
            numCFalse++;
    }
    test_numDone[id]+=ids.size();
    vector<pair<unsigned long,string> > toRemoveSpottings;
    vector<Spotting>* res = feedback(id, ids, userClassifications,0,&toRemoveSpottings);
    for (Spotting s : *res)
    {
        
        if (test_groundTruth[id][s.id])
        {
            test_numTruePos[id]++;
        }
        else
        {
            test_numFalsePos[id]++;
        }
    }
    
    if (results.find(id)==results.end())
        test_showResults(id,"");
    return res;
}*/

vector<Spotting>* MasterQueue::feedback(unsigned long id, const vector<string>& ids, const vector<int>& userClassifications, int resent, vector<pair<unsigned long, string> >* remove)
{
    if (GlobalK::knowledge()->CLUSTER)
    {
        return _feedback<ClusterBatcher>(clusterBatchers,clusterBatchersQueue,id,ids,userClassifications,resent, remove);
    }
    else
    {
        return _feedback<SpottingResults>(results,resultsQueue,id,ids,userClassifications,resent, remove);
    }
}
template <class T>
vector<Spotting>* MasterQueue::_feedback(map<unsigned long, pair<sem_t*,T*> >& batchers, map<unsigned long, pair<sem_t*,T*> >& batchersQueue, unsigned long id, const vector<string>& ids, const vector<int>& userClassifications, int resent, vector<pair<unsigned long, string> >* remove)
{
    //cout <<"got feedback for: "<<id<<endl;
    vector<Spotting>* ret=NULL;
    pthread_rwlock_rdlock(&semResults);
    if (batchers.find(id)!=batchers.end())
    {
        sem_t* sem=batchers.at(id).first;
        T* res = batchers.at(id).second;
        pthread_rwlock_unlock(&semResults);
        sem_wait(sem);
        int done=0;
        //cout <<"res feedback"<<endl;
        if (GlobalK::knowledge()->AUTO_APPROVE)
        {
            map<string,vector<Spotting> > forAutoApproval;
            ret = res->feedback(&done,ids,userClassifications,resent,remove,&forAutoApproval);
            autoApprove(batchers,forAutoApproval,ret);
        }
        else
            ret = res->feedback(&done,ids,userClassifications,resent,remove);
        //cout <<"END res feedback"<<endl;
        sem_post(sem);
        
        if (done==-1)
        {
            pthread_rwlock_wrlock(&semResultsQueue);
            batchersQueue[id] = make_pair(sem,res);
            pthread_rwlock_unlock(&semResultsQueue);
            
        }
        else if (done==2)
        {
            pthread_rwlock_wrlock(&semResultsQueue);
            int check = batchersQueue.erase(id);
            //assert(check==1);
            
            pthread_rwlock_unlock(&semResultsQueue);
        }
    }
    else
    {
        cout <<"Results not found for: "<<id<<endl;
        pthread_rwlock_unlock(&semResults);
    }
    return ret;
}

template <class T>
void MasterQueue::autoApprove(map<unsigned long, pair<sem_t*,T*> >& batchers, map<string,vector<Spotting> >& forAutoApproval, vector<Spotting>* ret)
{
    map<string, pair<sem_t*,T*> > neededBatchers;

    pthread_rwlock_rdlock(&semResults);
    for (auto p : batchers)
    {
        for (auto& p2 : forAutoApproval)
        {
            if (p2.first.compare(p.second.second->ngram)==0)
            {
                neededBatchers[p2.first]=p.second;
                break;
            }
        }
    }
    pthread_rwlock_unlock(&semResults);

    for (auto& p2 : forAutoApproval)
    {
        if (neededBatchers.find(p2.first) != neededBatchers.end())
        {
            sem_t* sem=neededBatchers[p2.first].first;
            T* res = neededBatchers[p2.first].second;
            sem_wait(sem);
            res->autoApprove(p2.second,ret);
            sem_post(sem);
        }
    }
}

/*void MasterQueue::addSpottingResults(SpottingResults* res, bool hasSemResults, bool toQueue)
{
    sem_t* sem = new sem_t();
    sem_init(sem,false,1);
    auto p = make_pair(sem,res);
    //This may be a race condition, but would require someone to get and finish a batch between here...
    if (!hasSemResults)
        pthread_rwlock_wrlock(&semResults);
    if (toQueue)
    {
        pthread_rwlock_wrlock(&semResultsQueue);
        resultsQueue[res->getId()] = p;
        pthread_rwlock_unlock(&semResultsQueue);
    }
    results[res->getId()] = p;
    if (!hasSemResults)
        pthread_rwlock_unlock(&semResults);
}*/


void MasterQueue::updateSpottingsMix(const vector< SpottingExemplar*>* spottings)
{

#ifdef TEST_MODE_LONG
    cout<<"updateSpottingsMix: ";
    for (auto s : *spottings)
        cout <<s->ngram<<", ";
    cout<<endl;
#endif
    map<SpottingExemplar*,pair<sem_t*,SpottingResults*> > toUpdateSpottingTrueNoScore;
    map<SpottingExemplar*,pair<sem_t*,SpottingResults*> > toAddTrueNoScore;

    pthread_rwlock_wrlock(&semResults);
    for (SpottingExemplar* spotting : *spottings)
    {
        bool found=false;
        for (auto p : results)
        {
            
            SpottingResults* res = p.second.second;
            if (res->ngram.compare(spotting->ngram) == 0)
            {
                found=true;
                toUpdateSpottingTrueNoScore[spotting] = p.second;
            }

            
        }
        if (!found)
        {
            //if no id, no matching ngram, this will frequently be the case
#ifdef TEST_MODE
            cout <<"Creating SpottingResults for "<<spotting->ngram<<endl;
#endif
            SpottingResults *n = new SpottingResults(spotting->ngram,contextPad);
            sem_t* sem = new sem_t();
            sem_init(sem,false,0);
            auto p = make_pair(sem,n);

            results[n->getId()] = p;
            toAddTrueNoScore[spotting]=p;
        }

    }
    pthread_rwlock_unlock(&semResults);

    for (auto p : toUpdateSpottingTrueNoScore)
    {
        sem_wait(p.second.first);
#ifdef TEST_MODE_LONG
        cout<<"update for new ex: "<<spotting->ngram<<endl;
#endif
        p.second.second->updateSpottingTrueNoScore(*(p.first));
        sem_post(p.second.first);
    }

    for (auto p : toAddTrueNoScore)
    {
        p.second.second->addTrueNoScore(*(p.first));
        sem_post(p.second.first);
    }

}

unsigned long MasterQueue::updateSpottingResults(vector<Spotting>* spottings, unsigned long id)
{
#ifdef TEST_MODE
    //cout<<"updateSpottingResults called from MasterQueue. "<<spottings->front().ngram<<endl;
#endif
    if (GlobalK::knowledge()->USE_QBE)
    {
        pthread_rwlock_rdlock(&semResults);
        if (id>0)
        {
            if (results.find(id)!=results.end())
            {
                sem_t* sem=results.at(id).first;
                SpottingResults* res = results.at(id).second;
                pthread_rwlock_unlock(&semResults);
                sem_wait(sem);
                bool resurrect = res->updateSpottings(spottings);
                sem_post(sem);
                if (resurrect)
                {
                    pthread_rwlock_wrlock(&semResultsQueue);
                    resultsQueue[id] = results.at(id);
                    pthread_rwlock_unlock(&semResultsQueue);
                }
                return id;
            }
            else
            {
                cout <<"Resultss not found for: "<<id<<endl;
                //pthread_rwlock_unlock(&semResults);
            }
        }
        
        pthread_rwlock_unlock(&semResults);
        pthread_rwlock_wrlock(&semResults);//we obtain a write-lock now in case we need to add a SpottingResults
        for (auto p : results)
        {
            
            sem_t* sem=p.second.first;
            SpottingResults* res = p.second.second;
            if (res->ngram.compare(spottings->front().ngram) == 0)
            {
                sem_wait(sem);
                pthread_rwlock_unlock(&semResults);
                bool resurrect = res->updateSpottings(spottings);
                unsigned long rid = res->getId();
                sem_post(sem);
                if (resurrect)
                {
#ifdef TEST_MODE
                    cout<<"Resurrect "<<res->ngram<<endl;
#endif
                    pthread_rwlock_wrlock(&semResultsQueue);
                    resultsQueue[rid] = make_pair(sem,res);
                    pthread_rwlock_unlock(&semResultsQueue);
                }
                return rid;
                
            }
        }
    }
    else
    {
        assert(id<=0);
        pthread_rwlock_wrlock(&semResults);
        for (auto p : results)
        {
            assert(p.second.second->ngram.compare(spottings->front().ngram) != 0);
        }
    }
        
    //if no id, no matching ngram, or if somethign goes wrong
#ifdef TEST_MODE
    cout <<"Creating SpottingResults for "<<spottings->front().ngram<<endl;
#endif
    SpottingResults *n = new SpottingResults(spottings->front().ngram,contextPad);
    sem_t* sem = new sem_t();
    sem_init(sem,false,0);
    auto p = make_pair(sem,n);
    results[n->getId()] = p;
    pthread_rwlock_unlock(&semResults);
    unsigned long ret = n->getId();

    assert(spottings->size()>1);
    for (Spotting& s : *spottings)
    {
        n->add(s);
        //cout <<"added spotting : "<<s.id<<endl;
    }
#ifdef TEST_MODE
    n->debugState();
#endif
    sem_post(sem);
    delete spottings;
    pthread_rwlock_wrlock(&semResultsQueue);
    resultsQueue[n->getId()] = p;
    pthread_rwlock_unlock(&semResultsQueue);
    return ret;
}


void MasterQueue::insertClusterBatcher(string ngram, int contextPad, bool stepMode, const vector<Spotting>& spottings, Mat& crossScores)
{
    ClusterBatcher* batcher = new ClusterBatcher(ngram,contextPad,stepMode,spottings,crossScores, saveDir);
    sem_t* sem = new sem_t();
    sem_init(sem,false,0);
    auto p = make_pair(sem,batcher);
    pthread_rwlock_wrlock(&semResults);
    clusterBatchers[batcher->getId()] = p;
    pthread_rwlock_unlock(&semResults);
    sem_post(sem);
    pthread_rwlock_wrlock(&semResultsQueue);
    clusterBatchersQueue[batcher->getId()] = p;
    pthread_rwlock_unlock(&semResultsQueue);
}

void MasterQueue::transcriptionFeedback(unsigned long id, string transcription, vector<pair<unsigned long, string> >* toRemoveExemplars, unsigned long* badSpotting) 
{
    if (transcription.find('\n') != string::npos)
        transcription="$PASS$";
    vector<Spotting*> newExemplars = transcribeBatchQueue.feedback(id, transcription, toRemoveExemplars,badSpotting);

    //enqueue these for approval
    //if (newExemplars.size()>0)
        newExemplarsBatchQueue.enqueue(newExemplars,toRemoveExemplars);
    //delete newExemplars;
}
void MasterQueue::enqueueNewExemplars(const vector<Spotting*>& newExemplars, vector<pair<unsigned long, string> >* toRemoveExemplars)
{
    newExemplarsBatchQueue.enqueue(newExemplars,toRemoveExemplars);
}

void MasterQueue::needExemplar(string ngram)
{
    newExemplarsBatchQueue.needExemplar(ngram);
}


void MasterQueue::save(ofstream& out)
{
    //ofstream out(savePrefix);

    //This is a costly lockdown, but bad things might happen if the queue is changed between writing the two
    pthread_rwlock_rdlock(&semResults);
    pthread_rwlock_rdlock(&semResultsQueue);
    out<<"MASTERQUEUE"<<endl;
    out<<results.size()<<"\n";
    for (auto p : results)
    {
        out<<p.first<<"\n";
        sem_wait(p.second.first);
        p.second.second->save(out);
        sem_post(p.second.first);
    }

    out<<resultsQueue.size()<<"\n";
    for (auto p : resultsQueue)
    {
        out<<p.first<<"\n";
    }


    out<<clusterBatchers.size()<<"\n";
    for (auto p : clusterBatchers)
    {
        out<<p.first<<"\n";
        sem_wait(p.second.first);
        p.second.second->save(out);
        sem_post(p.second.first);
    }

    out<<clusterBatchersQueue.size()<<"\n";
    for (auto p : clusterBatchersQueue)
    {
        out<<p.first<<"\n";
    }
    pthread_rwlock_unlock(&semResultsQueue);
    pthread_rwlock_unlock(&semResults);
        
    transcribeBatchQueue.save(out);
    newExemplarsBatchQueue.save(out);

    out<<finish.load()<<"\n";
    out<<numCTrue<<"\n"<<numCFalse<<"\n";
    out<<contextPad<<"\n";
    //out.close();    
}
MasterQueue::MasterQueue(ifstream& in, CorpusRef* corpusRef, PageRef* pageRef, string savePre)
{
    saveDir = savePre+"_extra/";
    finish.store(false);
    pthread_rwlock_init(&semResultsQueue,NULL);
    pthread_rwlock_init(&semResults,NULL);
#if ROTATE
    pthread_rwlock_init(&semRotate,NULL);
    testIter=0;	
    rotate_pos=0;
#endif
    kill.store(false);
    
    //ifstream in(loadPrefix);

    string line;
    getline(in,line);
    assert(line.compare("MASTERQUEUE")==0);
    getline(in,line);
    int rSize = stoi(line);
    for (int i=0; i<rSize; i++)
    {
        getline(in,line);
        unsigned long id = stoul(line);
        //assert(id==i+1);
        SpottingResults* res = new SpottingResults(in,pageRef);
        assert(id==res->getId());
        sem_t* sem = new sem_t();
        sem_init(sem,false,1);
        auto p = make_pair(sem,res);
        results[res->getId()] = p;
    }
    getline(in,line);
    rSize = stoi(line);
    for (int i=0; i<rSize; i++)
    {
        getline(in,line);
        unsigned long id = stoul(line);
        resultsQueue[results[id].second->getId()] = results[id];
    }

    getline(in,line);
    rSize = stoi(line);
    for (int i=0; i<rSize; i++)
    {
        getline(in,line);
        unsigned long id = stoul(line);
        //assert(id==i+1);
        ClusterBatcher* res = new ClusterBatcher(in,pageRef, saveDir);
        assert(id==res->getId());
        sem_t* sem = new sem_t();
        sem_init(sem,false,1);
        auto p = make_pair(sem,res);
        clusterBatchers[res->getId()] = p;
    }
    getline(in,line);
    rSize = stoi(line);
    for (int i=0; i<rSize; i++)
    {
        getline(in,line);
        unsigned long id = stoul(line);
        clusterBatchersQueue[clusterBatchers[id].second->getId()] = clusterBatchers[id];
    }


    transcribeBatchQueue.load(in,corpusRef);
    newExemplarsBatchQueue.load(in,pageRef);

    getline(in,line);
    finish.store(stoi(line));
 
    getline(in,line);
    numCTrue = stoi(line);
    getline(in,line);
    numCFalse = stoi(line);

    getline(in,line);
    contextPad = stoi(line);
    //in.close();
#ifdef TEST_MODE
    forceNgram="";
#endif
}

map<string, vector< tuple<float,float,int,float,float> > > MasterQueue::getBatchTracking()
{
    map<string, vector< tuple<float,float,int,float,float> > > ret;
    pthread_rwlock_rdlock(&semResults);
    //if (GlobalK::knowledge()->CLUSTER)
    //{
        for (auto& p : clusterBatchers)
        {
            sem_t* sem=p.second.first;
            ClusterBatcher* res = p.second.second;
            sem_wait(sem);
            ret[res->ngram] = res->getBatchTracking();
            sem_post(sem);
        }
    //}
    //else
    //{
        for (auto& p : results)
        {
            sem_t* sem=p.second.first;
            SpottingResults* res = p.second.second;
            sem_wait(sem);
            ret[res->ngram] = res->getBatchTracking();
            sem_post(sem);
        }
    //}
    pthread_rwlock_unlock(&semResults);
    return ret;
}
