#include "Web.h"


vector<Spotting>* start(const vector<string>& ngrams)
{
    for (string ngram : ngrams)
    {
        SpottingQuery query(ngram);
        //vector<Spotting>* res = corpus->runQuery(&query);
        vector<SpottingLoc> = spotter->massSpot(ngrams);
        for (SpottingLoc& r : *res)
        {
            allSpottings[r.id]=r;
            //TODO track scores
        }
    }
                                                               //src/sink edges,     //edges to ngram nodes                //interconn edges
    graph = new GraphType(1+ngrams.size()+allSpottings.size(), 2*(1+ngrams.size()) + (1+ngrams.size())*allSpottings.size()+allSpottings.size()*allSpottings.size());
    //for (int i=0; i<1+ngrams.size()+allSpottings.size(); i++)
    graph->add_node(1+ngrams.size()+allSpottings.size());


    for (int i=0; i<ngram.size(); i++)
    {
        ngramToNode[ngrams[i]]=i+1;
    }


    int node1=1+ngrams.size();
    slidToNode[allSpottings.begin()->first]=node1;
    for (auto iter1=allSpottings.begin(); iter1!=allSpottings.end(); iter1++, node1++)
    {
        arc* a1,a2;
        for (string ngram : ngrams)
        {
            float score = iter1->second.scores[ngram];
            graph->add_edge(ngramToNode[ngram],node1,score,score,&a1,&a2);
            arcMap[ngram][iter1->first] = make_pair(a1,a2);
        }

        float nullScore = ??;
        graph->add_edge(0,node1,nullScore,nullScore,&a1,&a2);
        arcMap[""][iter1->first] = make_pair(a1,a2);

        int node2 = node1+1;
        for (auto iter2=iter1+1; iter2!=allSpottings.end(); iter2++, node2++)
        {
            if (node1==1+ngrams.size())
                slidToNode[iter->first]=node2;
            float score = compare(??);
            crossScores[pair<iter1->first,iter2->first>]=score;

            graph->add_edge(node1,node2,score,score);
        }
    }

    classifications = classify(false);
    for (auto p : classifications)
    {
        if (p.second.length()>0)
        {
            spottings.emplace_back( 
                                    allSpottings[p.first].tlx,
                                    allSpottings[p.first].tly,
                                    allSpottings[p.first].brx,
                                    allSpottings[p.first].bry,
                                    allSpottings[p.first].pageId,
                                    corpus->getPagePnt(allSpottings[p.first].pageId),
                                    p.second
                                  );
            sidToSlid[spottings.back().id]=p.first;
            slidToSid[p.first]=spottings.back().id;
        }
    }
    return &spottings;
}

map<unsigned long, ngram> classify(bool reuse)
{
}

void redoCuts(vector<Spotting>* add, vector<pair<unsigned long,string> >* remove)
{
    toRemoveMut.lock();
    for (unsigned long sid : toRemove)
    {
        unsigned long slid = sidToSlid[sid];
        pair<arc*,arc*> arcs = arcMap[classifications[slid]][slid];
        graph->set_rcap(arc.first)=0;
        graph->set_rcap(arc.second)=0;
        graph->mark_node(slidToNode[slid]);
        graph->mark_node(ngramToNode[classifications[slid]]);

    }
    toRemove.clear();
    toRemoveMut.unlock();

    map<unsigned long, ngram> newClassifications = classify(true);
    for (auto p : newClassifications)
    {
        if (p.second.compare(classifications[p.first])!=0)
        {
            remove->emplace_back(slidToSid[p.first],classifications[p.first]);
            add->emplace_back(
                                allSpottings[p.first].tlx,
                                allSpottings[p.first].tly,
                                allSpottings[p.first].brx,
                                allSpottings[p.first].bry,
                                allSpottings[p.first].pageId,
                                corpus->getPagePnt(allSpottings[p.first].pageId),
                                p.second
                              );
            sidToSlid.erase(slidToSid[p.first]);
            sidToSlid[add.back().id]=p.first;
            slidToSid[p.first]=add.back().id;
        }
    }
    classifications = newClassifications;

}


void badSpotting(unsigned long sid, vector<Spotting>* add, vector<pair<unsigned long,string> >* remove)
{
    toRemoveMut.lock();
    toRemove.push_back(sid);
    toRemoveMut.unlock();
    if (runningMut.try_lock())
    {
        redoCuts(add,remove);
        runningMut.unlock();
        return;
    }
    else if (waitingMut.try_lock())
    {
        runningMut.lock();
        waitingMut.unlock();
        redoCuts(add,remove);
        runningMut.unlock();
        return;
    }
    else
    {
        return;
    }

    //get current classification of sid
    //get that weight to zero
    //recut
}
