#include "Simulator.h"
#include <csignal>


//BENHTANM 
//spottingAverageMilli=16116
//spottingErrorProbConst=0.035
//spottingSkipProbConst=0.078


//NAMES 
//spottingAverageMilli=16783
//spottingErrorProbConst=0.058
//spottingSkipProbConst=0.029

Simulator::Simulator(string dataname, string mode, string segCSV) : cluster(mode[0]=='c' && mode[1]=='l'), spottingsUseTrues(false)
{
    //read in seg GT
    ifstream in (segCSV);
    assert(in.good());
    string line;
    //getline(in,line);//header
    while (getline(in,line))
    {
        string s;
        std::stringstream ss(line);
        getline(ss,s,',');
        corpusWord.push_back(s);
        getline(ss,s,',');
        corpusPage.push_back(stoi(s)+1); //+1 is correction from my creation of segmentation file
        getline(ss,s,',');//x1
        int x1=stoi(s);
        getline(ss,s,',');
        int y1=stoi(s);
        getline(ss,s,',');//x2
        getline(ss,s,',');
        int y2=stoi(s);
        corpusYBounds.push_back(make_pair(y1,y2));
        vector<int> lettersStart, lettersEnd;
        vector<int> lettersStartRel, lettersEndRel;

        while (getline(ss,s,','))
        {
            lettersStart.push_back(stoi(s));
            lettersStartRel.push_back(stoi(s)-x1);
            getline(ss,s,',');
            lettersEnd.push_back(stoi(s));
            lettersEndRel.push_back(stoi(s)-x1);
            //getline(ss,s,',');//conf
        }
        assert(lettersStart.size() == corpusWord.back().length());
        corpusXLetterStartBounds.push_back(lettersStart);
        corpusXLetterEndBounds.push_back(lettersEnd);
        corpusXLetterStartBoundsRel.push_back(lettersStartRel);
        corpusXLetterEndBoundsRel.push_back(lettersEndRel);
    }
    in.close();
    //sending corpus word is purely for error checking
    GlobalK::knowledge()->setCorpusXLetterBounds(&corpusXLetterStartBoundsRel,&corpusXLetterEndBoundsRel,&corpusWord);

    if (dataname.compare("test")==0)
    {
        cout<<"TEST MODE TIMINGS BEING USED"<<endl;
        spottingAverageMilli=50;
        spottingAverageMilli_prev=50;
        //spottingErrorProbConst=0.035;
        //spottingSkipProbConst=0.078;
        spottingErrorProb_m=0; //https://plot.ly/create/
        spottingErrorProb_b=0;
        spottingSkipProb_m=0;
        spottingSkipProb_b=0;


        transMilli_b=50;//position 0.07 (no -1)
        transMilli_m=0;
        transMilli_0=1;
        transMilli_notAvail=50;
        transErrorProbAvailError=0;
        transErrorProbAvailWord=0;
        transErrorProbNotAvail=0;
        //transSkipProb=0;

        manMilli_b=50;
        manMilli_m=3;
        manErrorProb=0;
    }
    else if (dataname.compare("BENTHAM")==0)
    {
        if (cluster)
        {
            spottingTime_m=704.818;//on total number
            spottingTime_b=3943.204;
#if NO_ERROR
            spottingErrorProb_m=0;
            spottingErrorProb_b=0;
#else
            spottingErrorProb_m=-0.003; //https://plot.ly/create/
            spottingErrorProb_b=0.065;
#endif
            spottingSkipProb_m=0.095;
            spottingSkipProb_b=-0.369;
        }
        else
        {
            
            spottingAverageMilli=8521.865;
            spottingAverageMilli_prev=-1685.935;
#if NO_ERROR
            spottingErrorProb_m=0;
            spottingErrorProb_b=0;
#else
            spottingErrorProb_m=-0.0041; //https://plot.ly/create/
            spottingErrorProb_b=0.061;
#endif
            spottingSkipProb_m=0.02652;
            spottingSkipProb_b=0.084055;
        }


        transMilli_b=2857.76;//1354.996;
        transMilli_m=252.42;//1021.526;
        transMilli_0=1763.635;
        transMilli_notAvail=5490.7;
        transErrorProbAvailError=0.01415;
        transErrorProbAvailWord=0.01179;
        transErrorProbNotAvail=1- 0.921;
        //transSkipProb=0.012;

        manMilli_b=0;//1734.123;
        manMilli_m=475.2370893;//260.813;
#if NO_ERROR
        manErrorProb=0;
#else
        manErrorProb=1-0.927;
#endif
    }
    else if (dataname.compare("NAMES")==0)
    {
        if (cluster)
        {
            spottingTime_m=620.8846;//on total number
            spottingTime_b=4308.497;
#if NO_ERROR
            spottingErrorProb_m=0;
            spottingErrorProb_b=0;
#else
            spottingErrorProb_m=0.003; //https://plot.ly/create/
            spottingErrorProb_b=-0.002;
#endif
            spottingSkipProb_m=-0.004;
            spottingSkipProb_b= 0.060;
        }
        else
        {
            spottingsUseTrues=true;
            
            spottingTime_m=1783.927;//on num true
            spottingTime_b=4909.408;
#if NO_ERROR
            spottingErrorProb_m=0;
            spottingErrorProb_b=0;
#else
            spottingErrorProb_m=0.018; //https://plot.ly/create/
            spottingErrorProb_b=0.011;
#endif
            spottingSkipProb_m=2.125;
            spottingSkipProb_b=-4.952;
        }

        transMilli_b=2603;//2543.413;
        transMilli_m=239.57;//345.995;
        transMilli_0=1996;
        transMilli_notAvail=5526;
        transErrorProbAvailError=0.01739;
        transErrorProbAvailWord=0.03043;
        transErrorProbNotAvail=1- 0.904;
        //transSkipProb=0.012;

        manMilli_b=0;//36.011;
        manMilli_m=518.6982641;//516.171;
#if NO_ERROR
        manErrorProb=0;
#else
        manErrorProb=1-0.840;
#endif


    }
    else
    {
        cout<<"ERROR, unknown dataname "<<dataname<<endl;
    }
}
//I need, for each word in the corpus"
//*GT
//*boundaries between letters

int Simulator::skipAndError(vector<int>& labels)
{
    float T=0;
    for (int c : labels)
        T+=c;
    if (cluster)
        T=labels.size();
    float e = T*spottingErrorProb_m + spottingErrorProb_b; //
    float s = T*spottingSkipProb_m + spottingSkipProb_b; //
    if (RAND_PROB < e)
    {
        int i = rand()%labels.size();
        labels[i]=!labels[i];
    }
    if (RAND_PROB < s)
    {
        int i = rand()%labels.size();
        labels[i]=-1;
    }
    return T;
}

vector<int> Simulator::spottings(string ngram, vector<Location> locs, vector<string> gt, string prevNgram)
{
    //int numSkip=0;
    //int numT=0;
    //int numF=0;
    //int numObv=0;
    vector<int> labels(locs.size());
    for (int i=0; i<locs.size(); i++)
    {
        if (gt[i].length()>2)
            labels[i] = getSpottingLabel(ngram,locs[i]);
        else
            labels[i] = stoi(gt[i]);
    }
    int numTrues=skipAndError(labels);
    /*for (int i=0; i<locs.size(); i++)
    {
        //labels[i] = getSpottingLabel(ngram,locs[i]);
        if (labels[i]==0)
            numF++;
        else if (labels[i]==1)
            numT++;
        else
            numSkip++;
        if (gt[i].compare("0")==0)
            numObv++;
    }*/
    
    /*float nIn[nInSIze];
    for (int i=0; i<2*N_LETTERS; i++)
        nIn[i]=0;
    nIn[ngram[0]-'a']=1;
    nIn[N_LETTERS+ngram[1]-'a']=1;
    nIn[2*N_LETTERS+0]=(numSkip/5.0)*2.0-1.0;
    nIn[2*N_LETTERS+1]=(numT/5.0)*2.0-1.0;
    nIn[2*N_LETTERS+2]=(numF/5.0)*2.0-1.0;
    nIn[2*N_LETTERS+3]=(prevNgram.compare(ngram)==0)?1:0;
    //nIn[2*N_LETTERS+4]=(numObv/5.0)*2.0-1.0;
    nIn[2*N_LETTERS+4]=2*error-1.0;

    float nOut;
    net_compute (spotNet, nIn, &nOut);
    //int milli = MINSPOT_MILLI+MAX_SPOT_MILLI*(nOut+1)/2.0;
    int milli = nOut*2*MILLI_STD + MILLI_MEAN;*/
    //int same = ngram.compare(prevNgram)==0?1:0;
    //int milli = c + numT*t  + same*p + (1.0-error)*a;
    bool prev =ngram.compare(prevNgram);
    int milli = (labels.size()/5.0)*spottingAverageMilli + (prev?(labels.size()/5.0)*spottingAverageMilli_prev:0);
    if (cluster)
        milli = labels.size()*spottingTime_m + spottingTime_b;
    if (spottingsUseTrues)
        milli = numTrues*spottingTime_m + spottingTime_b;

    this_thread::sleep_for(chrono::milliseconds(milli));
    GlobalK::knowledge()->timeSpentSpot+=milli;

    return labels;
}

int Simulator::getSpottingLabel(string ngram, Location loc, bool strict)
{
    float overlap_insides_thresh = OVERLAP_INSIDE_THRESH;
    float overlap_consume_thresh = OVERLAP_CONSUME_THRESH;
    float overlap_size_thresh = OVERLAP_SIDE_THRESH;
    float size_note_included_thresh = SIDE_NOT_INCLUDED_THRESH;
    if (strict)
    {
        overlap_insides_thresh = OVERLAP_INSIDE_THRESH_STRICT;
        overlap_consume_thresh = OVERLAP_CONSUME_THRESH_STRICT;
        overlap_size_thresh = OVERLAP_SIDE_THRESH_STRICT;
        size_note_included_thresh = SIDE_NOT_INCLUDED_THRESH_STRICT;
    }
    int maxOverlap=0;
    int w=-1;
    for (int i=0; i<corpusWord.size(); i++)
    {
        if (    loc.pageId == corpusPage[i] &&
                loc.y1<corpusYBounds[i].second &&
                loc.y2>corpusYBounds[i].first &&
                loc.x1<corpusXLetterEndBounds[i].back() &&
                loc.x2>corpusXLetterStartBounds[i].front()
           )
        {
            int overlap = ( min(loc.y2,corpusYBounds[i].second)-max(loc.y1,corpusYBounds[i].first) ) *
                          ( min(loc.x2,corpusXLetterEndBounds[i].back())-max(loc.x1,corpusXLetterStartBounds[i].front()) );
            if (overlap>maxOverlap)
            {
                maxOverlap=overlap;
                w=i;
            }
        }
    }
    if (w>=0)
    {
        //cout<<corpusWord[w]<<" : "<<ngram<<endl;
        //int l1 = corpusWord[w].find(ngram[0]);
        //int l2 = corpusWord[w].find(ngram[1]);
        for (int l2=ngram.length()-1; l2<corpusWord[w].length(); l2++)
        {
            int l1=l2-(ngram.length()-1);
            if (ngram.compare(corpusWord[w].substr(l1,ngram.length()))!=0)
                continue;
            //if (RAND_PROB<isInSkipProb)
            //    ret.push_back( -1 );
            //else
            {
                /*cout<<"page: "<<loc.pageId<<"  y1:"<<loc.y1<<endl;
                cout<<loc.x1<<"\t"<<loc.x2<<"\t"<<endl;
                cout<<corpusXLetterStartBounds[w][l1]<<"\t"<<corpusXLetterEndBounds[w][l2]<<endl;
                cout<<"inside["<<((loc.x1>=corpusXLetterStartBounds[w][l1] && loc.x2<=corpusXLetterEndBounds[w][l2])?1:0)<<"]: "<<(loc.x2-loc.x1)/(corpusXLetterEndBounds[w][l2]-corpusXLetterStartBounds[w][l1]+0.0)<<" ?> "<<overlap_insides_thresh<<endl;
                cout<<"consume["<<((loc.x1<=corpusXLetterStartBounds[w][l1] && loc.x2>=corpusXLetterEndBounds[w][l2])?1:0)<<"]: "<<(loc.x2-loc.x1)/(corpusXLetterEndBounds[w][l2]-corpusXLetterStartBounds[w][l1]+0.0)<<" ?< "<<overlap_consume_thresh<<endl;
                cout<<"("<<endl<<"side: "<<(min(loc.x2,corpusXLetterEndBounds[w][l2])-max(loc.x1,corpusXLetterStartBounds[w][l1]))/(corpusXLetterEndBounds[w][l2]+corpusXLetterStartBounds[w][l1]+0.0)<<" ?> "<<overlap_size_thresh<<endl;
                cout<<"not inc: "<<max(max(loc.x1,corpusXLetterStartBounds[w][l1])-min(loc.x1,corpusXLetterStartBounds[w][l1]),max(loc.x2,corpusXLetterEndBounds[w][l2])-min(loc.x2,corpusXLetterEndBounds[w][l2]))/(min(loc.x2,corpusXLetterEndBounds[w][l2])-max(loc.x1,corpusXLetterStartBounds[w][l1])+0.0)<<" ?< "<<size_note_included_thresh<<endl<<")"<<endl;*/
                if (
                        (loc.x1>=corpusXLetterStartBounds[w][l1] && loc.x2<=corpusXLetterEndBounds[w][l2] && (loc.x2-loc.x1)/(corpusXLetterEndBounds[w][l2]-corpusXLetterStartBounds[w][l1]+0.0) > overlap_insides_thresh) ||
                        (loc.x1<=corpusXLetterStartBounds[w][l1] && loc.x2>=corpusXLetterEndBounds[w][l2] && (loc.x2-loc.x1)/(corpusXLetterEndBounds[w][l2]-corpusXLetterStartBounds[w][l1]+0.0) < overlap_consume_thresh) ||
                        ((min(loc.x2,corpusXLetterEndBounds[w][l2])-max(loc.x1,corpusXLetterStartBounds[w][l1]))/(corpusXLetterEndBounds[w][l2]-corpusXLetterStartBounds[w][l1]+0.0) > overlap_size_thresh && max(max(loc.x1,corpusXLetterStartBounds[w][l1])-min(loc.x1,corpusXLetterStartBounds[w][l1]),max(loc.x2,corpusXLetterEndBounds[w][l2])-min(loc.x2,corpusXLetterEndBounds[w][l2]))/(min(loc.x2,corpusXLetterEndBounds[w][l2])-max(loc.x1,corpusXLetterStartBounds[w][l1])+0.0) < size_note_included_thresh)
                   )
                {
                    //if  (RAND_PROB < falseNegativeProb)
                    //{
                    //    return 0;
                    //    *error+=1.0/locs.size();
                    //}
                    //else

                    //cout<<"alignment good"<<endl;
                        return 1;
                }
                else
                {
                    //if (RAND_PROB < falsePositiveProb)
                    //{
                    //    return 1;
                    //    *error+=1.0/locs.size();
                    //}
                    //else

                    //cout<<"alignment bad"<<endl;
                        return 0;
                }

            }
            
        }
        //else if we didn't find the ngram
        {
            //if (RAND_PROB<notInSkipProb)
            //    ret.push_back( -1 );
            //else
            {
                //if (RAND_PROB < notInFalsePositiveProb)
                //{
                //    return 1;
                //    *error+=1.0/locs.size();
                //}
                //else

                //cout<<"letters bad"<<endl;
                    return 0;
            }
        }
    }
    else
    {
        cout<<"Error, spotting did not match word"<<endl;
        cout<<"   Page:"<<loc.pageId<<" bb: "<<loc.x1<<" "<<loc.y1<<" "<<loc.x2<<" "<<loc.y2<<endl;
        //raise(SIGINT);
        return 0;
    }

}

vector<int> Simulator::newExemplars(vector<string> ngrams, vector<Location> locs, string prevNgram)
{
    vector<int> labels(locs.size());
    for (int i=0; i<locs.size(); i++)
       labels[i] = getSpottingLabel(ngrams[i],locs[i],true);
    skipAndError(labels);
    
    int milli = (labels.size()/5.0)*spottingAverageMilli;
    this_thread::sleep_for(chrono::milliseconds(milli));

    return labels;
}

string Simulator::transcription(int wordIndex, vector<SpottingPoint> spottings, vector<string> poss, string gt, bool lastWasTrans)
{
    string ret ="";
    int corIdx=-1;
    int milli;
    transform(gt.begin(), gt.end(), gt.begin(), ::tolower);
    for (int i=0; i< poss.size(); i++)
    {
        if (gt.compare(poss[i])==0)
        {
            ret=poss[i];
            corIdx=i;
            milli=transMilli_b + transMilli_m*i;
            if (i==0)
                milli=transMilli_0;
        }
    }
    if (ret.length()==0)
    {
        milli=transMilli_notAvail;

        for (SpottingPoint sp : spottings)
        {
            if (0==getSpottingLabel(sp.getNgram(),Location(sp.page,sp.x1,sp.y1,sp.x2,sp.y2)))
            {
                ret="$REMOVE:"+sp.getId()+"$";
                break;
            }
        }

        if (ret.length()==0)
        {
            if (RAND_PROB < transErrorProbNotAvail)
                ret=poss[0];//just pick first trans
            else
                ret="$ERROR$";
        }
        else
        {
            if (RAND_PROB < transErrorProbNotAvail)
                ret="$ERROR$";//Assume that user misses that there ngram is wrong
        }
    }
    else
    {
        float rp = RAND_PROB;
        if (rp < transErrorProbAvailError)
            ret="$ERROR$";//user didn't see correct trans
        else if (rp < transErrorProbAvailError + transErrorProbAvailWord)
            ret=poss[(corIdx+1)%poss.size()];//wrong word selected
    }
            
    this_thread::sleep_for(chrono::milliseconds(milli));
    GlobalK::knowledge()->timeSpentTrans+=milli;

    return ret;
}

string Simulator::manual(int wordIndex, vector<string> poss, string gt, bool lastWasMan)
{
    int milli = manMilli_b + manMilli_m*gt.length();
    string ret=gt;
    if (RAND_PROB < manErrorProb)
        ret[0]='!';
    if (GlobalK::knowledge()->MANUAL_LINES)
        GlobalK::knowledge()->time_spent += milli;
    //else
    this_thread::sleep_for(chrono::milliseconds(milli));
    return ret;
}
