#include "Web.h"


vector<Spotting>* start(const vector<string>& ngrams_)
{
    ngrams.insert(ngrams_.begin(),ngrams_.end());
    ngrams.push_front("");//the null node to allow spottings to receive to classification

    Mat crossScores;
    vector<SpottingLoc> = spotter->massSpot(ngrams_,crossScores);//expects QbS scores to be adjusted
    for (SpottingLoc& r : *res)
    {
        allSpottings[r.id]=r;
        //TODO track scores
    }
                                                               //src/sink edges,     //edges to ngram nodes                //interconn edges
    graph = new GraphType(ngrams.size()+allSpottings.size(), 2*(ngrams.size()) + (ngrams.size())*allSpottings.size()+allSpottings.size()*allSpottings.size());
    //for (int i=0; i<ngrams.size()+allSpottings.size(); i++)
    graph->add_node(ngrams.size()+allSpottings.size());


    for (int i=0; i<ngram.size(); i++)
    {
        ngramToNode[ngrams[i]]=i;
    }


    int node1=ngrams.size();
    slidToNode[allSpottings.begin()->first]=node1;
    for (auto iter1=allSpottings.begin(); iter1!=allSpottings.end(); iter1++, node1++)
    {
        arc* a1,a2;
        for (string ngram : ngrams)
        {
            float score;
            if (ngram.length()>0)
               score = iter1->second.scores[ngram];
            else
            {
                score=sum(crossScores(Rect(iter1->first,0,1,crossScores.rows)))[0];
                for (string ngram2 : ngrams_)
                    score += iter1->second.scores[ngram2];
                score /= ngrams_.size()+allSpottings.size()-1;//no-class score is the average score
            }
            graph->add_edge(ngramToNode[ngram],node1,score,score,&a1,&a2);
            arcMap[ngram][iter1->first] = make_pair(a1,a2);
        }


        int node2 = node1+1;
        for (auto iter2=iter1+1; iter2!=allSpottings.end(); iter2++, node2++)
        {
            if (node1==ngrams.size())
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
    map<float, set< pair<unsigned long, unsigned long> > > isoCuts;
    map<float, map< string,set<unsigned long> > > isoCutHeads;
    for (auto ngramAndArcs : arcMap)
    {
        
        for (string ngram : ngrams)
        {
            int node = ngramToNode[ngram];
            if (ngram.compare(ngramAndArcs.first)==0)
            {
                if (reuse)
                {
                    graph->set_trcap(node,MAX_FLOAT);
                    graph->mark_node(node);
                }
                else
                    graph->add_tweights(node,MAX_FLOAT,0);
            }
            else
            {
                if (reuse)
                {
                    graph->set_trcap(node,-MAX_FLOAT);
                    graph->mark_node(node);
                }
                else
                    graph->add_tweights(node,0,MAX_FLOAT);
            }
        }

        float score = graph->maxflow(reuse);
        reuse=true;

        /*
        set< pair<unsigned long, unsigned long> > cut;
        int node1=0;
        for (auto iter1=allSpottings.begin(); iter1!=allSpottings.end(); iter1++, node1++)
        {
            int node2=0;
            for (auto iter2=iter1+1; iter2!=allSpottings.end(); iter2++, node2++)
            {
                if (graph->what_segment(node1) != graph->what_segment(node2))
                    cut.emplace(iter1->first,node2->first);
            }
        }
        isoCuts.emplace(score,cut);
        */
        map<string,set<unsigned long> > cutHead;
        for (string ngram : ngrams)
        {
            for (auto iter1=allSpottings.begin(); iter1!=allSpottings.end(); iter1++)
            {
                if (graph->what_segment(node1) != graph->what_segment(ngramToNode[ngram]))
                    cutHead[ngram].insert(iter1->first);
            }
        }
        isoCutHeads.emplace(score,cutHead);

    }

    /*
    set< pair<unsigned long, unsigned long> > unionCuts;
    for (auto iter = ++isoCuts.begin(); iter!=isoCuts.end(); iter++)
    {
        unionCuts.insert(iter->second.begin(),iter->second.end());
    }
    */
    map< string,set<unsigned long> > unionCutHeads;
    for (auto iter = ++isoCutHeads.begin(); iter!=isoCutHeads.end(); iter++)
    {
        for (string ngram : ngrams)
        {
            unionCutHeads[ngram].insert(iter->second[ngram].begin(),iter->second[ngram].end());
        }
    }

    map<unsigned long, ngram> ret;
    set<unsigned long> noClass;
    for (auto iter1=allSpottings.begin(); iter1!=allSpottings.end(); iter1++)
    {
        string cls="XXXXX";
        for (string ngram : ngrams)
        {
            if (unionCutHeads[ngram].find(iter1->first)==unionCutHeads[ngram].end())
            {
                assert(cls.length()==5);//ensure only one class
                cls=ngram;
                if (ngram.length()>0)
                    ret[iter1->first]=ngram;
                //break;
            }
        }
        if (cls.length()==5)
            noClass.insert(iter1->first);
    }

    if (noClass.size()>0)
        cout<<"[!] "<<noClass.size()<<" instances missed by cut."<<endl;

    return ret;
    
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
