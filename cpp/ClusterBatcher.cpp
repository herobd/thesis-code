#include "ClusterBatcher.h"


ClusterBatcher(string ngram, int contextPad, bool stepMode, const vector<Spotting>& massSpottingRes, const Mat& crossScores) : ngram(ngram), contextPad(contextPad), stepMode(stepMode), spottingRes(massSpottingRes), crossScores(crossScores), finished(false)
//vector<Spotting>* start(const vector<Spotting>& massSpottingRes, const Mat& crossScores)
{
    //Set up GT
    vector<bool> gt(massSpottingRes.size());
    //spottingRes = massSpottingRes;
    for (int i=0; i<massSpottingRes.size(); i++)
        gt[i] = massSpottingRes[i].gt==1;
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

}

SpottingsBatch* getBatch(bool* done, unsigned int maxWidth,int color,string prevNgram, bool need=true)
{

    int clusterToReturn-1;
    if (trueInstancesToSeed.size()==0)
    {
        for (int level=0; level<averageClusterSize.size(); level++)
        {
            if (averageClusterSize[level]>10)
            {
                curLevel=level;
                break;
            }
        }
        float bestAvgScore=-9999999;//SpottingLocs have high score as good
        //int bestClust=-1;
        for (int clusterI=0; clusterI<clusterLevels.at(curLevel).size(); clusterI++)
        {
            const list<int>& clust = clusterLevels.at(curLevel)[clusterI];
            float avgScore=0;
            int count=0;
            for (int i : clust)
            {
                if (spottingRes.at(i).type == SPOTTING_TYPE_NONE &&
                        starts.find(i+1)==starts.end())
                {
                    count++;
                    avgScore += spottingRes.at(i).scoreQbS;
                }
            }
            avgScore/=count;
            if (avgScore>bestAvgScore)
            {
                bestAvgScore=avgScore;
                clusterToReturn=clusterI;
            }
        }
    }
    else
    {
        int seedInstance = trueInstancesToSeed.front();
        trueInstancesToSeed.pop_front();
        maxClustMinDist=-999999;
        for (int clusterI=0; clusterI<clusterLevels.at(curLevel).size(); clusterI++)
        {
            const list<int>& clust = clusterLevels.at(curLevel)[clusterI];
            float minDist=9999999;
            for (int i : clust)
            {
                if (spottingRes.at(i).type == SPOTTING_TYPE_NONE &&  //only include unlabeled instances in cluster comparison
                        starts.find(i+1)==starts.end() && 
                        crossScores.at<float>(seedInstance,i) < minDist)
                    minDist=crossScores.at<float>(seedInstance,i);
            }
            if (minDist > maxClustMinDist)
            {
                clusterToReturn=clusterI;
                maxClustMinDist=minDist;
            }
        }
    }
    assert(clusterToReturn!=-1);

    //put batch together
    SpottingsBatch* ret = new SpottingsBatch(ngram,id);

    for (int inst : clusterLevels.at(curLevel).at(clusterToReturn))
    {
        if (spottingRes.at(inst).type == SPOTTING_TYPE_NONE && starts.find(inst+1)==starts.end()) 
        {
            ret.emplace_back(spottingRes.at(inst),maxWidth,contextPad,color,prevNgram);
        }
    }

    //remove cluster so it isn't sent again
    //this is tracked by the type and starts

    //is done?
    *done=finished;//we'll let this be determined in feedback


    for (int i=0; i<ret->size(); i++)
    {
        starts[ret->at(i).id] = chrono::system_clock::now();
    }

    return ret;
}

vector<Spotting>* feedback(int* done, const vector<string>& ids, const vector<int>& userClassifications, int resent=false, vector<pair<unsigned long,string> >* retRemove=NULL)
{
//ids one indexed
    //Evaluate purity and accuracy and update clusterLeve
    //Add seeds
    //Store results
    //Return results
    *done = false;
    int numTrue=0;
    int numFalse=0;

    vector<Spotting>* ret = new vector<Spotting>();
    for (unsigned int i=0; i< ids.size(); i++)
    {
        int id = stoi(ids[i]);
        int check = starts.erase(id);
        if (userClassifications[i]>0)
        {
            numTrue++;
#ifdef NO_NAN
            GlobalK::knowledge()->accepted();
#endif
            if (!resent || spottingRes.at(id-1).type!=SPOTTING_TYPE_APPROVED)
            {
                spottingRes.at(id-1).type=SPOTTING_TYPE_APPROVED;
                ret->push_back(spottingRes.at(id-1));
                if (stepMode)
                    trueInstancesToSeed.push_back(id-1);
            }
        }
        else if (userClassifications[i]==0)
        {
            numFalse++;
#ifdef NO_NAN
            GlobalK::knowledge()->rejected();
#endif
            if (!resent || spottingRes.at(id-1).type!=SPOTTING_TYPE_REJECTED)
            {
                spottingRes.at(id-1).type=SPOTTING_TYPE_REJECTED;
                if (retRemove!=NULL)
                    retRemove->push_back(spottingRes.at(id-1));
            }
        }
        //Cluster is "readded" if we dont set the type
        //if (!resent && userClassifications[i]==-1)
        //{
        //    readd cluster
        //}
    }

    float purity = 2* ((max(numTrue,numFalse)/(numTrue+numFalse))-0.5);
    windowPurity.push_back(purity);
    if (windowPurity.size()>RUNNING_PURITY_COUNT)
    {
        float popped = windowPurity.front();
        windowPurity.pop_front();
        runningPurity += (purity-popped)/RUNNING_PURITY_COUNT;
        windowPurity.push_back(purity);
    }
    else
    {
        runningPurity=0;
        for (float p : windowPurity)
            runningPurity+=1;
        runningPurity/=windowPurity.size();
    }

#ifdef TEST_MODE
    cout<<"["<<ngram<<"] Estimated purity: "<<runningPurity<<", Actual: "<<meanCPurity[curLevel]<<endl;
#endif
    if (abs(GOAL_PURITY-runningPurity) > PURITY_THRESHOLD)
    {
        if (runningPurity<GOAL_PURITY)
            curLevel--;
        else
            curLevel++;
    }


    float accuracy = numTrue/(numTrue+numFalse);
    windowAccuracy.push_back(accuracy);
    if (windowAccuracy.size()>RUNNING_ACCURACY_COUNT)
    {
        float popped = windowAccuracy.front();
        windowAccuracy.pop_front();
        runningAccuracy += (accuracy-popped)/RUNNING_ACCURACY_COUNT;
        windowAccuracy.push_back(accuracy);
    }
    else
    {
        runningAccuracy=0;
        for (float p : windowAccuracy)
            runningAccuracy+=1;
        runningAccuracy/=windowAccuracy.size();
    }

    if (windowAccuracy.size()==RUNNING_ACCURACY_COUNT && runningAccuracy<ACCURACY_STOP_THRESH)
    {
#ifdef TEST_MODE
        cout<<"["<<ngram<<"] running acc: "<<runningAccuracy<<" below thresh: "<<ACCURACY_STOP_THRESH<<endl;
#endif
        *done=finished?2:1;
        finished=true;
    }


    return ret;

}



void CL_cluster(vector< list<int> >& clusters, Mat& minSimilarity, int numClusters, const vector<bool>& gt)//, vector<float>& meanCPurity, vector<float>& medianCPurity, vector<float>& meanIPurity, vector<float>& medianIPurity, vector<float>& maxPurity, vector< vector< list<int> > >& clusterLevels)
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
        clusterLevels.push_back(clusters);


        //stats tracking
        float sumCPurity=0;
        float sumIPurity=0;
        float mPurity=0;
        set<float> sortCPurity, sortIPurity;
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
            sumIPurity += clust.size()*purity;
            sortCPurity.insert(purity);
            for (int i=0; i<clust.size(); i++)
                sortIPurity.insert(purity);
            if (purity > mPurity)
                mPurity=purity;
        }
        meanCPurity.push_back(sumCPurity/clusters.size());
        meanIPurity.push_back(sumIPurity/gt.size());
        maxPurity.push_back(mPurity);
        auto iter = sortCPurity.begin();
        for (int i=0; i<sortCPurity.size()/2; i++)
            iter++;
        medianCPurity.push_back(*iter);
        iter = sortIPurity.begin();
        for (int i=0; i<sortIPurity.size()/2; i++)
            iter++;
        medianIPurity.push_back(*iter);



        averageClusterSizes.push_back(sumSize/clusters.size());
        //if (sumSize/clusters.size()>5)//dont need to track for all levels
        //    minSimilarities[clusterLevels.size()-1]=minSimilarity;
    }
}
