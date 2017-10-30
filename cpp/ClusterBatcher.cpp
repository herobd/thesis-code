#include "ClusterBatcher.h"

atomic_ulong ClusterBatcher::_id;

ClusterBatcher::ClusterBatcher(string ngram, int contextPad, bool stepMode, const vector<Spotting>& massSpottingRes, const Mat& crossScores, string saveDir) : ngram(ngram), contextPad(contextPad), stepMode(stepMode), spottingRes(massSpottingRes), crossScores(crossScores), finished(false), curLevel(-1)
//vector<Spotting>* start(const vector<Spotting>& massSpottingRes, const Mat& crossScores)
{
    for (int i=0; i<spottingRes.size(); i++)
        spottingIdToIndex[spottingRes[i].id]=i;
    id = ++_id;
    //Set up GT
    vector<bool> gt(massSpottingRes.size());
    //spottingRes = massSpottingRes;
    for (int i=0; i<massSpottingRes.size(); i++)
    {
        gt[i] = (massSpottingRes[i].gt==1 || (massSpottingRes[i].gt==UNKNOWN_GT && i%2==0));
    }
    //this->crossScores=crossScores;
    assert(clusterLevels.size()==0);
    //now cluster!
    //using hierarchical clustering
    //vector< vector< list<int> > > clustersAtLevels;
    //level,clusters,instances
    vector< list<int> > clusters(massSpottingRes.size());
    Mat minSimilarity = crossScores.clone();//minimum pair-wise link for each cluster
    //instanceToCluster.resize(1);
    for (int r=0; r<minSimilarity.rows; r++)
        minSimilarity.at<float>(r,r)=-99999;
    for (int i=0; i<massSpottingRes.size(); i++)
    {
        clusters[i].push_back(i);
        //instanceToCluster[0][i]=i;
    }

    //For testing purposes
    //vector<float> meanCPurity,  medianCPurity,  meanIPurity,  medianIPurity,  maxPurity;

    //vector< vector< list<int> > > clusterLevels;
    //vector<float> averageClusterSize;
    //map<int,Mat> minSimilarities;
    CL_cluster(clusters,minSimilarity,10, gt);//, meanCPurity,  medianCPurity,  meanIPurity,  medianIPurity,  maxPurity, clusterLevels, averageClusterSize);
    batchesOut=0;

    incompleteCluster=-1;

    //Cut down the clusters!
    //We're going to prune away the first few layers of clusters to improve memory efficiency

    //at what point should the average batch size be 2?
    auto meanIter = meanCPurity.begin();
    auto clustIter = clusterLevels.begin();
    auto sizeIter = averageClusterSize.begin();
    while (*sizeIter < 2)
    {
        meanIter++;
        clustIter++;
        sizeIter++;
    }
    meanCPurity.erase(meanCPurity.begin(),meanIter);
    clusterLevels.erase(clusterLevels.begin(),clustIter);
    averageClusterSize.erase(averageClusterSize.begin(),sizeIter);

    //to speed things up, were going to save the crossScores matrix once.
    mkdir(saveDir.c_str(),S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    ofstream out(saveDir+"/crossScores_"+ngram+".dat");
    GlobalK::writeFloatMat(out,crossScores);
    out.close();

}

SpottingsBatch* ClusterBatcher::getBatch(int* done, unsigned int num, bool hard, unsigned int maxWidth,int color,string prevNgram, bool need)
{
    if(finished)
    {
        cout<<"["<<ngram<<"] got getBatch() called when finished."<<endl;
    }
    if (((finished || batchesOut>=MAX_BATCHES_OUT_PER_NGRAM) && !need) || batchesOut>=HARD_MAX_BATCHES_OUT_PER_NGRAM)
    {
        *done=finished?1:0;
        return NULL;
    }
    else if (batchesOut>=MAX_BATCHES_OUT_PER_NGRAM)
    {
        cout<<"["<<ngram<<"] need making me send out extra batch: "<<batchesOut+1<<endl;
    }

    int clusterToReturn=incompleteCluster;
    if (clusterToReturn!=-1)
    {
        int checkCountInClust=0;//in case the level change has banished the cluster
        for (int i : clusterLevels.at(curLevel)[clusterToReturn])
        {
            if (spottingRes.at(i).type == SPOTTING_TYPE_NONE &&
                    starts.find(spottingRes.at(i).id)==starts.end())
                if (++checkCountInClust>=2)
                    break;
        }
        if (checkCountInClust<2)
            clusterToReturn=-1;
    }

    if (clusterToReturn==-1)
    {
        if (trueInstancesToSeed.size()==0)
        {
            if (curLevel=-1)
            {
                for (int level=0; level<averageClusterSize.size(); level++)
                {
                    if (averageClusterSize[level]>10)
                    {
                        curLevel=level;
                        break;
                    }
                }
            }
            float bestAvgScore=-9999999;//SpottingLocs have high score as good
            //int bestClust=-1;
            for (int clusterI=0; clusterI<clusterLevels.at(curLevel).size(); clusterI++)
            {
                //const list<int>& clust = clusterLevels.at(curLevel)[clusterI];
                //assert(clust.size()>=0);
                
                float avgScore=0;
                int count=0;
                for (int i : clusterLevels.at(curLevel)[clusterI])
                {
                    if (spottingRes.at(i).type == SPOTTING_TYPE_NONE &&
                            starts.find(spottingRes.at(i).id)==starts.end())
                    {
                        count++;
                        avgScore += spottingRes.at(i).scoreQbS;
                        assert(avgScore==avgScore);
                    }
                    else
                        assert(starts.size()>0 || spottingRes.at(i).type != SPOTTING_TYPE_NONE);
                }
                if (count>0)
                {
                    avgScore/=count;
                    if (avgScore>bestAvgScore)
                    {
                        bestAvgScore=avgScore;
                        clusterToReturn=clusterI;
                    }
                    assert(clusterToReturn!=-1);
                }
            }
        //assert(clusterToReturn!=-1);
        }
        else
        {
            int seedInstance = trueInstancesToSeed.front();
            trueInstancesToSeed.pop_front();
            float maxClustMinDist=-999999;
            for (int clusterI=0; clusterI<clusterLevels.at(curLevel).size(); clusterI++)
            {
                const vector<int>& clust = clusterLevels.at(curLevel)[clusterI];
                float minDist=9999999;
                for (int i : clust)
                {
                    if (spottingRes.at(i).type == SPOTTING_TYPE_NONE &&  //only include unlabeled instances in cluster comparison
                            starts.find(spottingRes.at(i).id)==starts.end() && 
                            crossScores.at<float>(seedInstance,i) < minDist)
                        minDist=crossScores.at<float>(seedInstance,i);
                }
                if (minDist!=9999999 && minDist > maxClustMinDist)
                {
                    clusterToReturn=clusterI;
                    maxClustMinDist=minDist;
                }
            }
        //assert(clusterToReturn!=-1);
        }
    }

    if (clusterToReturn==-1)
    {
        *done=finished?1:2;
        finished=true;
        return NULL;
    }

    //is done?
    *done=finished?1:0;//we'll let this be determined in feedback
    //put batch together
    SpottingsBatch* ret = new SpottingsBatch(ngram,id);

    incompleteCluster=-1;
    for (int inst : clusterLevels.at(curLevel).at(clusterToReturn))
    {
        if (spottingRes.at(inst).type == SPOTTING_TYPE_NONE && starts.find(spottingRes.at(inst).id)==starts.end()) 
        {
            ret->emplace_back(spottingRes.at(inst),maxWidth,contextPad,color,prevNgram);
        }
        else
            assert(starts.size()>0 || spottingRes.at(inst).type != SPOTTING_TYPE_NONE);
        if (ret->size() >= MAX_BATCH_SIZE)
        {
            if (clusterLevels.at(curLevel).at(clusterToReturn).size()-ret->size()>1)
                incompleteCluster=clusterToReturn;//finish cluster on next batch. Only if there are atleast 2 instances left
            break;
        }
    }
    assert(ret->size()>0);

    //remove cluster so it isn't sent again
    //this is tracked by the type and starts



#ifdef TEST_MODE
    cout<<"["<<ngram<<"] sending cluster/batch of size "<<ret->size()<<" from level "<<curLevel<<", ids: ";
#endif 
    for (int i=0; i<ret->size(); i++)
    {
        starts[ret->at(i).id] = chrono::system_clock::now();
#ifdef TEST_MODE
        cout<<ret->at(i).id<<", ";
#endif 
    }
#ifdef TEST_MODE
    cout<<endl;
#endif 


    batchesOut++;

    return ret;
}

BatchWraper* ClusterBatcher::getSpottingsAsBatch(int width, int color, string prevNgram, vector<unsigned long> spottingIds)
{
    SpottingsBatch* batch = new SpottingsBatch(ngram,id);
    for (unsigned long sid : spottingIds)
    {
        int inst = spottingIdToIndex[sid];
        batch->emplace_back(spottingRes.at(inst),width,contextPad,color,prevNgram);
    }
    BatchWraperSpottings* ret = new BatchWraperSpottings(batch);
    return ret;
}

vector<Spotting>* ClusterBatcher::feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent, vector<pair<unsigned long,string> >* retRemove, map<string,vector<Spotting> >* forAutoApproval)
{
    batchesOut--;
    //Evaluate purity and accuracy and update clusterLeve
    //Add seeds
    //Store results
    //Return results
    *done = false;
    int numTrue=0;
    int numFalse=0;

#ifdef TEST_MODE
    cout<<"["<<ngram<<"] revieved feedback: ";
#endif

    vector<Spotting>* ret = new vector<Spotting>();
    assert(ids.size()<spottingRes.size());
    for (unsigned int i=0; i< ids.size(); i++)
    {
#ifdef TEST_MODE
        cout<<userClassifications[i]<<", ";
#endif
        int sid = stoi(ids[i]);
        int sindex = spottingIdToIndex[sid];
        int check = starts.erase(sid);
        if (userClassifications[i]>0)
        {
            numTrue++;
#ifdef NO_NAN
            GlobalK::knowledge()->accepted();
#endif
            if (spottingRes.at(sindex).type!=SPOTTING_TYPE_APPROVED)
            {
                if (spottingRes.at(sindex).type!=SPOTTING_TYPE_AUTO_APPROVED)
                {
                    ret->push_back(spottingRes.at(sindex));
#ifdef AUTO_APPROVE
                    for (int subLen=ngram.length()-1; subLen>0; subLen--)
                    {
                        for (int subPos=0; subPos<=ngram.length()-subLen; subPos++)
                        {
                            string sub = ngram.substr(subPos,subLen);
                            int width = (spottingRes.at(sindex).brx-spottingRes.at(sindex).tlx+1)*subLen/(0.0+ngram.length());
                            int xStart = width*subPos + spottingRes.at(sindex).tlx;
                            int xEnd = xStart+width-1;
                            (*forAutoApproval)[sub].emplace_back(spottingRes.at(sindex).pageId,xStart,spottingRes.at(sindex).tly,xEnd,spottingRes.at(sindex).bry,spottingRes.at(sindex).wordId);
                        }
                    }
#endif
                }
                spottingRes.at(sindex).type=SPOTTING_TYPE_APPROVED;
#if NO_ERROR
                assert(spottingRes.at(sindex).gt==1 || spottingRes.at(sindex).gt==UNKNOWN_GT);
#endif
                if (stepMode && trueInstancesToSeed.size()<1000)
                    trueInstancesToSeed.push_back(sindex);
            }
        }
        else if (userClassifications[i]==0)
        {
            numFalse++;
#ifdef NO_NAN
            GlobalK::knowledge()->rejected();
#endif
            if (spottingRes.at(sindex).type!=SPOTTING_TYPE_REJECTED)
            {
                spottingRes.at(sindex).type=SPOTTING_TYPE_REJECTED;
#if NO_ERROR
                assert(spottingRes.at(sindex).gt==0 || spottingRes.at(sindex).gt==UNKNOWN_GT);
#endif
                if (resent&&retRemove!=NULL)
                    retRemove->emplace_back(spottingRes.at(sindex).id,ngram);
            }
        }
        //Cluster is "readded" if we dont set the type
        //if (!resent && userClassifications[i]==-1)
        //{
        //    readd cluster
        //}
    }
#ifdef TEST_MODE
    cout<<endl;
#endif

    if (numTrue+numFalse==0)
        return ret;

    float purity = 2* ((max(numTrue,numFalse)/(0.0+numTrue+numFalse))-0.5);
    assert(purity>=0);
    windowPurity.push_back(purity);
    if (windowPurity.size()>RUNNING_PURITY_COUNT)
    {
    //    float popped = windowPurity.front();
        windowPurity.pop_front();
    //    runningPurity += (purity-popped)/RUNNING_PURITY_COUNT;
    }
    //else
    //{
        runningPurity=0;
        for (float p : windowPurity)
            runningPurity+=p;
        runningPurity/=windowPurity.size();
    //}
    assert(runningPurity>=0);
    assert(windowPurity.size()<=RUNNING_PURITY_COUNT);

#ifdef TEST_MODE
    cout<<"["<<ngram<<"] Estimated purity: "<<runningPurity<<", Actual: "<<meanCPurity[curLevel];
#endif
    if (abs(GOAL_PURITY-runningPurity) > PURITY_THRESHOLD)
    {
        if (runningPurity<GOAL_PURITY)
            if (curLevel>0)
                curLevel--;
        else if (curLevel<clusterLevels.size()-1)
            curLevel++;
        //assert(curLevel>=0 && curLevel<clusterLevels.size());
    }


    float accuracy = numTrue/(0.0+numTrue+numFalse);
    windowAccuracy.push_back(accuracy);
    if (windowAccuracy.size()>RUNNING_ACCURACY_COUNT)
    {
        float popped = windowAccuracy.front();
        windowAccuracy.pop_front();
        runningAccuracy += (accuracy-popped)/RUNNING_ACCURACY_COUNT;
    }
    else
    {
        runningAccuracy=0;
        for (float p : windowAccuracy)
            runningAccuracy+=p;
        runningAccuracy/=windowAccuracy.size();
    }
    assert(windowAccuracy.size()<=RUNNING_ACCURACY_COUNT);

#ifdef TEST_MODE
    cout<<", Window acc: "<<runningAccuracy<<", batch acc: "<<accuracy<<endl;
#endif

    if (windowAccuracy.size()>=RUNNING_ACCURACY_COUNT && runningAccuracy<ACCURACY_STOP_THRESH)
    {
#ifdef TEST_MODE
        cout<<"["<<ngram<<"] running acc: "<<runningAccuracy<<" below thresh: "<<ACCURACY_STOP_THRESH<<endl;
#endif
        *done=finished?1:2;//-1 resurrect, 0 not done, 1 done, 2 just finished
        finished=true;
    }

    //Tracking for debugging
    assert(numTrue+numFalse<spottingRes.size());
    batchTracking.emplace_back(purity,accuracy,numTrue+numFalse,runningPurity,runningAccuracy);


    return ret;

}



void ClusterBatcher::CL_cluster(vector< list<int> >& clusters, Mat& minSimilarity, int numClusters, const vector<bool>& gt)//, vector<float>& meanCPurity, vector<float>& medianCPurity, vector<float>& meanIPurity, vector<float>& medianIPurity, vector<float>& maxPurity, vector< vector< list<int> > >& clusterLevels)
{
    while (clusters.size()>numClusters)
    {
        for (int r=0; r<minSimilarity.rows; r++)
            for (int c=0; c<minSimilarity.cols; c++)
                assert(minSimilarity.at<float>(r,c) == minSimilarity.at<float>(r,c));

        Point maxLink;
        minMaxLoc(minSimilarity,NULL,NULL,NULL,&maxLink);//get cluster with strongest link
        int clust1 = std::min(maxLink.x,maxLink.y);
        int clust2 = std::max(maxLink.x,maxLink.y);
        //instancesToCluster.push_back(instancesToCluster.back());
        //for (int inst : clusters[clust2])
        //    instancesToCluster.back()[inst]=clust1;
        clusters[clust1].insert(clusters[clust1].end(), clusters[clust2].begin(), clusters[clust2].end());
        clusters.erase(clusters.begin()+clust2);
        for (int r=0; r<minSimilarity.rows; r++)
        {
            if (r!=clust1 && r!=clust2)
            {
                minSimilarity.at<float>(clust1,r) = minSimilarity.at<float>(r,clust1) = min(minSimilarity.at<float>(clust1,r),minSimilarity.at<float>(clust2,r));
            }
        }
        Mat newMinS(minSimilarity.rows-1,minSimilarity.cols-1,CV_32F);
        minSimilarity(Rect(0,0,clust2,clust2)).copyTo(newMinS(Rect(0,0,clust2,clust2)));
        if (minSimilarity.cols-(clust2+1)>0)
        {
            minSimilarity(Rect(clust2+1,0,minSimilarity.cols-(clust2+1),clust2)).copyTo(newMinS(Rect(clust2,0,minSimilarity.cols-(clust2+1),clust2)));
            minSimilarity(Rect(0,clust2+1,clust2,minSimilarity.rows-(clust2+1))).copyTo(newMinS(Rect(0,clust2,clust2,minSimilarity.rows-(clust2+1))));
            minSimilarity(Rect(clust2+1,clust2+1,minSimilarity.cols-(clust2+1),minSimilarity.rows-(clust2+1))).copyTo(newMinS(Rect(clust2,clust2,minSimilarity.cols-(clust2+1),minSimilarity.rows-(clust2+1))));
        }
        minSimilarity=newMinS;

        //check//
        for (auto clust : clusters)
            assert(clust.size()>0);
        //    //


        //stats tracking
        float sumCPurity=0;
        //float sumIPurity=0;
        //float mPurity=0;
        //set<float> sortCPurity, sortIPurity;
        float sumSize=0;
        for (auto clust : clusters)
        {
            sumSize+=clust.size();
            float numF = 0;
            float numT = 0;
            for (int i : clust)
            {
                if (gt[i])
                    numT++;
                else
                    numF++;
            }
            float purity = 2* ((max(numT,numF)/(numT+numF))-0.5);
            sumCPurity += purity;
            //sumIPurity += clust.size()*purity;
            //sortCPurity.insert(purity);
            //for (int i=0; i<clust.size(); i++)
            //    sortIPurity.insert(purity);
            //if (purity > mPurity)
            //    mPurity=purity;
        }
        //meanIPurity.push_back(sumIPurity/gt.size());
        //maxPurity.push_back(mPurity);
        //auto iter = sortCPurity.begin();
        //for (int i=0; i<sortCPurity.size()/2; i++)
        //    iter++;
        //medianCPurity.push_back(*iter);
        //iter = sortIPurity.begin();
        //for (int i=0; i<sortIPurity.size()/2; i++)
        //    iter++;
        //medianIPurity.push_back(*iter);

        int meanClusterSize = sumSize/clusters.size();
        if (meanClusterSize>5 && (meanClusterSize>10 || clusters.size()%2==0))//dont need to track for all levels
        {
            //clusterLevels.push_back(clusters);
            vector< vector<int> > clustersNew;
            clustersNew.reserve(clusters.size());
            for (auto clust : clusters)
                clustersNew.emplace_back(clust.begin(), clust.end());
            clusterLevels.emplace_back(clustersNew);
            meanCPurity.push_back(sumCPurity/clusters.size());
            averageClusterSize.push_back(meanClusterSize);
        }
        //    minSimilarities[clusterLevels.size()-1]=minSimilarity;
    }
}


void ClusterBatcher::autoApprove(vector<Spotting> toApprove, vector<Spotting>* ret)
{
    for (Spotting& s : spottingRes)
    {
        for (auto bounds=toApprove.begin(); bounds!=toApprove.end();)
        {
            if (s.type == SPOTTING_TYPE_NONE &&
                    s.wordId==bounds->wordId && 
                    (min(s.brx,bounds->brx)-max(s.tlx,bounds->tlx))/(0.0+s.brx-s.tlx) > AUTO_APPROVE_THRESH)
            {
                //approve(s);
                s.type=SPOTTING_TYPE_AUTO_APPROVED;
                ret->push_back(s);
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

bool ClusterBatcher::checkIncomplete()
{
    bool incomp=false;
    for (auto iter=starts.begin(); iter!=starts.end(); iter++)
    {
        auto start = *iter;
        chrono::system_clock::duration d = chrono::system_clock::now()-start.second;
        chrono::minutes pass = chrono::duration_cast<chrono::minutes> (d);
        //cout<<pass.count()<<" minutes has past for "<<(start.first)<<endl;
        if (pass.count() > 20) //if 20 mins has passed
        {
            incomp=true;
#ifdef TEST_MODE
            cout<<"["<<ngram<<"] Timeout ("<<pass.count()<<") on batch "<<start.first<<endl;
#endif
            iter = starts.erase(iter);
            if (iter!=starts.begin())
                iter--;
            if (iter==starts.end())
                break;
        }
    }
    if (incomp && finished)
    {
        finished=false;
        return true;
    }
    return false;

}

void ClusterBatcher::save(ofstream& out)
{
    out<<"CLUSTERBATCHER"<<endl;
    out<<ngram<<"\n";
    out<<id<<"\n"<<contextPad<<"\n"<<(stepMode?1:0)<<"\n"<<(finished?1:0)<<endl;
    assert(meanCPurity.size()==clusterLevels.size() && meanCPurity.size()==averageClusterSize.size());
    //num cluster levels
    out<<meanCPurity.size()<<"\n";
    //num instances
    out<<spottingRes.size()<<"\n";

    for (float f : meanCPurity)
        out<<f<<"\n";

    for (auto clusters : clusterLevels)
    {
        out<<clusters.size()<<"\n";
        for (auto clust : clusters)
        {
            out<<clust.size()<<"\n";
            for (int i : clust)
                out<<i<<"\n";
        }
    }
    
    //for (auto instances : instanceToCluster)
    //{
    //    assert(instances.size() == spottingRes.size());
    //    for (int clust : instances)
    //        out<<clust<<"\n";
    //}

    for (float sz : averageClusterSize)
        out<<sz<<"\n";


    for (auto& spotting : spottingRes)
        spotting.save(out);

    out<<trueInstancesToSeed.size()<<"\n";
    for (int i : trueInstancesToSeed)
        out<<i<<"\n";

    out<<runningPurity<<"\n";
    out<<windowPurity.size()<<"\n";
    for (float f : windowPurity)
        out<<f<<"\n";
    out<<runningAccuracy<<"\n";
    out<<windowAccuracy.size()<<"\n";
    for (float f : windowAccuracy)
        out<<f<<"\n";
    out<<curLevel<<endl;

    out<<batchTracking.size()<<endl;
    for (auto t : batchTracking)
    {
        out<<get<0>(t)<<"\n"<<get<1>(t)<<"\n"<<get<2>(t)<<"\n"<<get<3>(t)<<"\n"<<get<4>(t)<<endl;
    }

    out<<incompleteCluster<<endl;
}

ClusterBatcher::ClusterBatcher(ifstream& in, PageRef* pageRef, string saveDir)
{
    string line;
    getline(in,line);
    assert(line.compare("CLUSTERBATCHER")==0);
    getline(in,ngram);
    getline(in,line);
    id = stoul(line);
    getline(in,line);
    contextPad = stoi(line);
    getline(in,line);
    stepMode = stoi(line);
    getline(in,line);
    finished = stoi(line);
    getline(in,line);
    int numLevels = stoi(line);
    getline(in,line);
    int numInstances = stoi(line);
    meanCPurity.resize(numLevels);
    for (int i=0; i<numLevels; i++)
    {
        in >> meanCPurity[i];
        getline(in,line);
        //meanCPurity[i] = stof(line);
    }

    clusterLevels.resize(numLevels);
    for (int i=0; i<numLevels; i++)
    {
        getline(in,line);
        int numClusters = stoi(line);
        clusterLevels[i].resize(numClusters);
        for (int j=0; j<numClusters; j++)
        {
            getline(in,line);
            int numIn = stoi(line);
            clusterLevels[i][j].reserve(numIn);
            for (int k=0; k<numIn; k++)
            {
                getline(in,line);
                clusterLevels[i][j].push_back(stoi(line));
            }
        }
    }

    //instanceToCluster.resize(numLevels);
    //for (int i=0; i<numLevels; i++)
    //{
    //    instanceToCluster[i].resize(numInstances);
    //    for (int j=0; j<numInstances; j++)
    //    {
    //        getline(in,line);
    //        instanceToCluster[i][j]=stoi(line);
    //    }
    //}

    averageClusterSize.resize(numLevels);
    for (int i=0; i<numLevels; i++)
    {
        in >> averageClusterSize[i];
        getline(in,line);
        //averageClusterSize[i]=stof(line);
    }
//
    string crossFile = saveDir+"/crossScores_"+ngram+".dat";
    ifstream crossIn(crossFile);
    assert(crossIn.good());
    crossScores = GlobalK::readFloatMat(crossIn);
    crossIn.close();
//
    //spottingRes.resize(numInstances);
    for (int i=0; i<numInstances; i++)
    {
        spottingRes.emplace_back(in,pageRef);
    }
    for (int i=0; i<spottingRes.size(); i++)
        spottingIdToIndex[spottingRes[i].id]=i;

    getline(in,line);
    int sz = stoi(line);
    for (int i=0; i<sz; i++)
    {
        getline(in,line);
        trueInstancesToSeed.push_back(stoi(line));
    }

    in >> runningPurity;
    getline(in,line);
    //runningPurity = stof(line);
    getline(in,line);
    sz = stoi(line);
    for (int i=0; i<sz; i++)
    {
        getline(in,line);
        windowPurity.push_back(stoi(line));
    }
    in >> runningAccuracy;
    getline(in,line);
    //runningAccuracy = stof(line);
    getline(in,line);
    sz = stoi(line);
    for (int i=0; i<sz; i++)
    {
        getline(in,line);
        windowAccuracy.push_back(stoi(line));
    }
    getline(in,line);
    curLevel = stoi(line);


    getline(in,line);
    sz = stoi(line);
    batchTracking.reserve(sz);
    for (int i=0; i<sz; i++)
    {
        float p,a,rp,ra;
        int s;
        in >> p >> a >> s >> rp >> ra;
        getline(in,line);
        batchTracking.emplace_back(p,a,s,rp,ra);
    }

    getline(in,line);
    incompleteCluster=stoi(line);
}
