#include "Web.h"


vector<Spotting>* Web::start(const vector<string>& ngrams_, const vector<SpottingLoc>& massSpottingRes, const Mat& crossScores)
{
    ngrams.insert(ngrams.end(),ngrams_.begin(),ngrams_.end());
    ngrams.push_front("");//the null node to allow spottings to receive to classification

    for (const SpottingLoc& r : massSpottingRes)
    {
        allSpottings[r.id]=r;
    }
    this->crossScores=crossScores;
    makeGraph();
    classifications = classify();
    vector<Spotting>* spottings = new vector<Spotting>();
    for (auto p : classifications)
    {
        if (p.second.length()>0)
        {
            spottings->push_back(corpus->wrapSpotting(allSpottings[p.first].imIdx,allSpottings[p.first].startX,allSpottings[p.first].endX,allSpottings[p.first].scores[p.second],p.second));
            //spottings.emplace_back( 
            //                        allSpottings[p.first].tlx,
            //                        allSpottings[p.first].tly,
            //                        allSpottings[p.first].brx,
            //                        allSpottings[p.first].bry,
            //                        allSpottings[p.first].pageId,
            //                        pageRef->getPageImg(allSpottings[p.first].pageId),
            //                        p.second
            //                      );
            sidToSlid[spottings->back().id]=p.first;
            slidToSid[p.first]=spottings->back().id;
        }
    }
    return spottings;
}

void Web::makeGraph()
{
                                                               //src/sink edges,     //edges to ngram nodes                //interconn edges
    graph = new GraphType(ngrams.size()+allSpottings.size(), 2*(ngrams.size()) + (ngrams.size())*allSpottings.size()+allSpottings.size()*allSpottings.size());
    //for (int i=0; i<ngrams.size()+allSpottings.size(); i++)
    graph->add_node(ngrams.size()+allSpottings.size());


    auto iter=ngrams.begin();
    for (int i=0; i<ngrams.size(); i++, iter++)
    {
        ngramToNode[*iter]=i;
    }


    int node1=ngrams.size();
    slidToNode[allSpottings.begin()->first]=node1;
    for (auto iter1=allSpottings.begin(); iter1!=allSpottings.end(); iter1++, node1++)
    {
        arc_id a1,a2;
        for (string ngram : ngrams)
        {
            float score;
            if (ngram.length()>0 || iter1->second.scores.find(ngram) != iter1->second.scores.end())
               score = iter1->second.scores[ngram];
            else
            {
                score=sum(crossScores(Rect(iter1->first,0,1,crossScores.rows)))[0];
                for (string ngram2 : ngrams)
                    if (ngram2.size()>0)
                        score += iter1->second.scores[ngram2];
                score /= ngrams.size()-1+allSpottings.size()-1;//no-class score is the average score
                iter1->second.scores[ngram]=score;
            }
            graph->add_edge(ngramToNode[ngram],node1,score,score,&a1,&a2);
            arcMap[ngram][iter1->first] = make_pair(a1,a2);
        }


        int node2 = node1+1;
        auto iter2=iter1;
        iter2++;
        for (; iter2!=allSpottings.end(); iter2++, node2++)
        {
            if (node1==ngrams.size())
                slidToNode[iter2->first]=node2;
            float score =crossScores.at<float>(iter1->first,iter2->first);

            graph->add_edge(node1,node2,score,score);
        }
    }

    reuse=false;

}

Web::Web(ifstream& in, Knowledge::Corpus* corpus) : corpus(corpus), graph(NULL)
{
    string line;
    getline(in,line);
    assert(line.compare("WEB")==0);
    getline(in,line);
    int sz = stoi(line);
    for (int i=0; i<sz; i++)
    {
        getline(in,line);
        ngrams.push_back(line);
    }

    getline(in,line);
    sz = stoi(line);
    for (int i=0; i<sz; i++)
    {
        SpottingLoc l;
        getline(in,line);
        l.imIdx = stoi(line);
        getline(in,line);
        l.startX = stoi(line);
        getline(in,line);
        l.endX = stoi(line);
        getline(in,line);
        l.id = stoul(line);
        allSpottings[l.id]=l;
        SpottingLoc& lr = allSpottings[l.id];
        getline(in,line);
        int sz2 = stoi(line);
        for (int j=0; j<sz2; j++)
        {
            string ngram;
            getline(in,ngram);
            getline(in,line);
            lr.scores[ngram]=stof(line);
        }
        getline(in,line);
        lr.numChar = stoi(line);
    }
    //read crossScores
    crossScores=GlobalK::readFloatMat(in);
    getline(in,line);
    sz = stoi(line);
    for (int i=0; i<sz; i++)
    {
        getline(in,line);
        unsigned long id = stoul(line);
        getline(in,line);
        classifications[id]=line;
    }
    makeGraph();
}

void Web::save (ofstream& out)
{
    out<<"WEB"<<endl;
    out<<ngrams.size()<<endl;
    for (string ngram : ngrams)
        cout<<ngram<<endl;
    out<<allSpottings.size()<<endl;
    for (auto p : allSpottings)
    {
        out<<p.second.imIdx<<"\n";
        out<<p.second.startX<<"\n";
        out<<p.second.endX<<"\n";
        out<<p.second.id<<"\n";
        out<<p.second.scores.size()<<"\n";
        for (auto p2 : p.second.scores)
        {
            out<<p2.first<<"\n";
            out<<p2.second<<"\n";
        }
        out<<p.second.numChar<<endl;
    }
    //save crossScores
    GlobalK::writeFloatMat(out,crossScores);
    out<<classifications.size()<<endl;
    for (auto p : classifications)
    {
        out<<p.first<<endl;
        out<<p.second<<endl;
    }
}

map<unsigned long, string> Web::classify()
{
    cout<<"Commencing multiway cut..."<<endl;
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
                if (graph->what_segment(slidToNode[iter1->first]) != graph->what_segment(ngramToNode[ngram]))
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
    cout<<"...finished multiway cut."<<endl;

    map<unsigned long, string> ret;
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

void Web::redoCuts(vector<Spotting>* add, vector<pair<unsigned long,string> >* remove)
{
    toRemoveMut.lock();
    for (unsigned long sid : toRemove)
    {
        unsigned long slid = sidToSlid[sid];
        string cls = classifications[slid];
        pair<arc_id,arc_id> arcs = arcMap[cls][slid];
        graph->set_rcap(arcs.first,REMOVE_SCORE);
        graph->set_rcap(arcs.second,REMOVE_SCORE);
        graph->mark_node(slidToNode[slid]);
        graph->mark_node(ngramToNode[classifications[slid]]);
        

        allSpottings[slid].scores[cls] = REMOVE_SCORE;//so we can recreate from save
    }
    //removed.insert(removed.end(),toRemove.begin(),toRemove.end());
    toRemove.clear();
    toRemoveMut.unlock();

    map<unsigned long, string> newClassifications = classify();
    for (auto p : newClassifications)
    {
        if (p.second.compare(classifications[p.first])!=0)
        {
            if (classifications[p.first].length()>0)
                remove->emplace_back(slidToSid[p.first],classifications[p.first]);
            if (p.second.length()>0)
                add->push_back(corpus->wrapSpotting(allSpottings[p.first].imIdx,allSpottings[p.first].startX,allSpottings[p.first].endX,allSpottings[p.first].scores[p.second],p.second));
                //add->emplace_back(
                //                    allSpottings[p.first].tlx,
                //                    allSpottings[p.first].tly,
                //                    allSpottings[p.first].brx,
                //                    allSpottings[p.first].bry,
                //                    allSpottings[p.first].pageId,
                //                    pageRef->getPageImg(allSpottings[p.first].pageId),
                //                    p.second
                //                  );
            sidToSlid.erase(slidToSid[p.first]);
            sidToSlid[add->back().id]=p.first;
            slidToSid[p.first]=add->back().id;
        }
    }
    classifications = newClassifications;

    cout<<"redoCuts(), add:"<<add->size()<<"   remove:"<<remove->size()<<endl;

}


void Web::badSpotting(unsigned long sid, vector<Spotting>* add, vector<pair<unsigned long,string> >* remove)
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
