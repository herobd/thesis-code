#include "Global.h"
GlobalK* GlobalK::_self=NULL;
GlobalK::GlobalK()
{
    string filePaths[2] = {"./data/top500_bigrams_in_freq_order.txt" , "./data/top500_trigrams_in_freq_order.txt"};
    for (int i=0; i<1+(MAX_N-MIN_N); i++)
    {
        ifstream in(filePaths[i]);
        string ngram;
        while(getline(in,ngram))
        {   
            transform(ngram.begin(), ngram.end(), ngram.begin(), ::tolower);
            ngramRanks[i+MIN_N].push_back(ngram);
        }
        in.close();
    }
    if (MIN_N==1)
    {
            vector<string> orderedAlpha={"e","t","a","o","i","n","s","h","r","d","l","c","u","m","w","f","g","y","p","b","v","k","j","x","q","z"};
            ngramRanks[1]=orderedAlpha;
    }
#ifdef NO_NAN
    xLock.lock();
#endif
    badPrunes=transBadBatch=transBadNgram=transSent=spotSent=spotAccept=spotReject=spotAutoAccept=spotAutoReject=newExemplarSpotted=0;
    SR_THRESH_NONE=false;
    SR_TAKE_FROM_TOP=false; 
    SR_OTSU_FIXED=false; 
    SR_FANCY_ONE=false;
    SR_FANCY_TWO=false;
    SR_GAUSSIAN_DRAW=false;

    MIN_SPOTTING_AP=-1;
    IDEAL_COMB=false;

    PHOC_TRANS=false;
    CPV_TRANS=false;
}

GlobalK::~GlobalK()
{
#ifdef NO_NAN
    if (trackFile.good())
        trackFile.close();
    for (auto p : accumRes)
        delete p.second;
#endif
}

GlobalK* GlobalK::knowledge()
{
    if (_self==NULL)
        _self=new GlobalK();
    return _self;
}

#ifdef NO_NAN
void GlobalK::setSimSave(string file)
{
    spottingFile = file+".spots";
    transBadBatch=transBadNgram=transSent=spotSent=spotAccept=spotReject=spotAutoAccept=spotAutoReject=newExemplarSpotted=0;
    struct stat buffer;
    bool appending = (stat (file.c_str(), &buffer) == 0);
    trackFile.open(file,ofstream::app|ofstream::out);
    if (!appending)
    {
        trackFile<<"time,accuracyTrans,pWordsTrans,pWords80_100,pWords60_80,pWords40_60,pWords20_40,pWords0_20,pWords0,transSent,badTransBatchs,badTransNgram,spotSent,spotAccept,spotReject,spotAutoAccept,spotAutoReject,newExemplarsSpotted,badPrunes,";
        trackFile<<"accuracyTrans_IV,pWordsTrans_IV,pWords80_100_IV,pWords60_80_IV,pWords40_60_IV,pWords20_40_IV,pWords0_20_IV,pWords0_IV,misTrans"<<endl;
    }

    ifstream in (spottingFile);
    if (!in.good())
        in.open(spottingFile+".bck");
    if (in.good())
    {
        spotMut.lock();
        string line;
        getline(in,line);
        assert(line.compare("[accums]")==0);
        getline(in,line);
        int num = stoi(line);
        for (int i=0; i<num; i++)
        {
            getline(in,line);
            int ind = line.find(':');
            string ngram = line.substr(0,ind-1);
            string scores = line.substr(ind+2);
            stringstream ss(scores);
            while (getline(ss,line,','))
            {
                int bp = line.find('[');
                int bp2 = line.find(']');
                int avb = line.find('(');
                int avb2 = line.find(')');
                int stb = line.find('{');
                int stb2 = line.find('}');
                string map = line.substr(0,bp);
                string dif = line.substr(bp+1,bp2-bp -1);
                string avg = line.substr(avb+1,avb2-avb -1);
                string std = line.substr(stb+1,stb2-stb -1);
                spottingAccumDifs[ngram].push_back(stoi(dif));
                spottingAccumAvgs[ngram].push_back(stoi(avg));
                spottingAccumStds[ngram].push_back(stoi(std));
                spottingAccums[ngram].push_back(stof(map));
            }
        }

        getline(in,line);
        assert(line.compare("[exemplars]")==0);
        getline(in,line);
        num = stoi(line);
        for (int i=0; i<num; i++)
        {
            getline(in,line);
            int ind = line.find(':');
            string ngram = line.substr(0,ind-1);
            string scores = line.substr(ind+2);
            stringstream ss(scores);
            while (getline(ss,line,','))
            {
                spottingExemplars[ngram].push_back(stof(line));
            }
        }

        getline(in,line);
        assert(line.compare("[normals]")==0);
        getline(in,line);
        num = stoi(line);
        for (int i=0; i<num; i++)
        {
            getline(in,line);
            int ind = line.find(':');
            string ngram = line.substr(0,ind-1);
            string scores = line.substr(ind+2);
            stringstream ss(scores);
            while (getline(ss,line,','))
            {
                spottingNormals[ngram].push_back(stof(line));
            }
        }

        getline(in,line);
        assert(line.compare("[others]")==0);
        getline(in,line);
        num = stoi(line);
        for (int i=0; i<num; i++)
        {
            getline(in,line);
            int ind = line.find(':');
            string ngram = line.substr(0,ind-1);
            string scores = line.substr(ind+2);
            stringstream ss(scores);
            while (getline(ss,line,','))
            {
                spottingOthers[ngram].push_back(stof(line));
            }
        }
        spotMut.unlock();
        in.close();
    }
}
#endif

int GlobalK::getNgramRank(string ngram)
{
    transform(ngram.begin(), ngram.end(), ngram.begin(), ::tolower);
    const vector<string>& ranks = ngramRanks[ngram.length()];
    for (int i=0; i<ranks.size(); i++)
    {
        if (ranks[i].compare(ngram)==0)
            return i+1;
    }
    return ranks.size()+1;
}


double GlobalK::otsuThresh(vector<int> histogram)
{
    double sum =0;
    int total=0;
    for (int i = 1; i < histogram.size(); ++i)
    {
        total+=histogram[i];
        sum += i * histogram[i];
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
    for (int i = 0; i < histogram.size(); ++i)
    {
        wB += histogram[i];
        if (wB == 0)
            continue;
        wF = total - wB;
        if (wF == 0)
            break;
        sumB += i * histogram[i];
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

    return ( threshold1 + threshold2 ) / 2.0;
}
void GlobalK::saveImage(const cv::Mat& im, ofstream& out)
{
    if (im.cols==0)
    {
        out<<"X"<<endl;
    }
    else
    {
        bool color=im.channels()==3;
        vector<unsigned char> encoded;
        cv::imencode(".png",im,encoded);
        out<<color<<"\n";
        out<<encoded.size()<<"\n";
        for (unsigned char c : encoded)
            out<<c;
        out<<"\n";
    }
}
void GlobalK::loadImage(cv::Mat& im, ifstream& in)
{
    string line;
    getline(in,line);
    if (line[0]!='X')
    {
        bool color=stoi(line);
        getline(in,line);
        int size = stoi(line);
        vector<unsigned char> encoded(size);
        for (int i=0; i<size; i++)
        {
            encoded[i]=in.get();
        }
        assert('\n'==in.get());
        im=cv::imdecode(encoded,color?CV_LOAD_IMAGE_COLOR:CV_LOAD_IMAGE_GRAYSCALE);
    }
}

void GlobalK::writeFloatMat(ofstream& dst, const cv::Mat& m)
{
    assert(m.type()==CV_32F);
    dst << m.rows<<"\n"<<m.cols<<"\n";
    std::streamsize p = dst.precision();
    dst << setprecision(9);
    for (int r=0; r<m.rows; r++)
        for (int c=0; c<m.cols; c++)
        {
            assert(m.at<float>(r,c)==m.at<float>(r,c));
            dst << m.at<float>(r,c) << "\n";
        }
    dst << setprecision(p);
}

cv::Mat GlobalK::readFloatMat(ifstream& src)
{
    int rows, cols;
    string rS;
    string cS;
    getline(src,rS);
    getline(src,cS);
    rows = stoi(rS);
    cols = stoi(cS);
    cv::Mat ret(rows,cols,CV_32F);
    for (int r=0; r<rows; r++)
        for (int c=0; c<cols; c++)
        {
            getline(src,rS);
            ret.at<float>(r,c) = stof(rS);
            //src >> ret.at<float>(r,c);
        }
    return ret;
}

string GlobalK::currentDateTime() //from http://stackoverflow.com/a/10467633/1018830
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    // Visit http://en.cppreference.com/w/cpp/chrono/c/strftime
    // for more information about date/time format
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

    return buf;
}
#ifdef NO_NAN

void GlobalK::sentSpottings()
{
    spotSent++;
}
void GlobalK::sentTrans()
{
    transSent++;
}
void GlobalK::badTransBatch()
{
    transBadBatch++;
}
void GlobalK::badTransNgram()
{
    transBadNgram++;
}
void GlobalK::accepted()
{
    spotAccept++;
}
void GlobalK::rejected()
{
    spotReject++;
}
void GlobalK::autoAccepted()
{
    spotAutoAccept++;
}
void GlobalK::autoRejected()
{
    spotAutoReject++;
}
void GlobalK::newExemplar()
{
    newExemplarSpotted++;
}
void GlobalK::saveTrack(float accTrans, float pWordsTrans, float pWords80_100, float pWords60_80, float pWords40_60, float pWords20_40, float pWords0_20, float pWords0, string misTrans,
                       float accTrans_IV, float pWordsTrans_IV, float pWords80_100_IV, float pWords60_80_IV, float pWords40_60_IV, float pWords20_40_IV, float pWords0_20_IV, float pWords0_IV, string misTrans_IV)
{
    track<<currentDateTime()<<","<<accTrans<<","<<pWordsTrans<<","<<pWords80_100<<","<<pWords60_80<<","<<pWords40_60<<","<<pWords20_40<<","<<pWords0_20<<","<<pWords0<<",";
    track<<transSent<<","<<transBadBatch<<","<<transBadNgram<<","<<spotSent<<","<<spotAccept<<","<<spotReject<<","<<spotAutoAccept<<","<<spotAutoReject<<","<<newExemplarSpotted<<","<<badPrunes<<",";
    track << accTrans_IV<<","<<pWordsTrans_IV<<","<<pWords80_100_IV<<","<<pWords60_80_IV<<","<<pWords40_60_IV<<","<<pWords20_40_IV<<","<<pWords0_20_IV<<","<<pWords0_IV<<","<<misTrans<<endl;//","<<misTrans_IV<<endl;
    //track+=currentDateTime()+","+accTrans+","+pWordsTrans+","+pWords80_100+","+pWords60_80+","+pWords40_60+","+pWords20_40+","+pWords0_20+","+pWords0+","+transSent+","+spotSent+","+spotAccept+","+spotReject+","+spotAutoAccept+","+spotAutoReject+","+newExemplarSpotted+"\n";
    transBadBatch=transBadNgram=transSent=spotSent=spotAccept=spotReject=spotAutoAccept=spotAutoReject=newExemplarSpotted=0;

}

void GlobalK::writeTrack()
{
    trackFile<<track.rdbuf()<<flush;
    track.str(string());
    track.clear();

    rename( spottingFile.c_str() , (spottingFile+".bck").c_str() );
    ofstream out(spottingFile);
    spotMut.lock();
    out<<"[accums]\n"<<spottingAccums.size()<<endl;
    for (auto aps : spottingAccums)
    {
        out<<aps.first<<" : ";
        for (int i =0; i<aps.second.size(); i++)
        {
            float ap = aps.second[i];
            int dif = spottingAccumDifs[aps.first][i];
            float avg = spottingAccumAvgs[aps.first][i];
            float std = spottingAccumStds[aps.first][i];
            out<<ap<<"["<<dif<<"]("<<avg<<"){"<<std<<"},";
        }
        out<<endl;
    }
    out<<"[exemplars]\n"<<spottingExemplars.size()<<endl;
    for (auto aps : spottingExemplars)
    {
        out<<aps.first<<" : ";
        for (float ap : aps.second)
            out<<ap<<",";
        out<<endl;
    }
    out<<"[normals]\n"<<spottingNormals.size()<<endl;
    for (auto aps : spottingNormals)
    {
        out<<aps.first<<" : ";
        for (float ap : aps.second)
            out<<ap<<",";
        out<<endl;
    }
    out<<"[others]\n"<<spottingOthers.size()<<endl;
    for (auto aps : spottingOthers)
    {
        out<<aps.first<<" : ";
        for (float ap : aps.second)
            out<<ap<<",";
        out<<endl;
    }
    spotMut.unlock();
    out.close();
}

void GlobalK::storeSpottingAccum(string ngram, float ap, int dif)
{
    accumResMut.lock();

    if (accumRes.find(ngram) == accumRes.end())
        accumRes[ngram] = new vector<SubwordSpottingResult>();
    vector<SubwordSpottingResult>* ar= accumRes.at(ngram);

    float avg=0;
    for (const SubwordSpottingResult& s : *ar)
    {
        avg+=s.score;
    }
    if (ar->size()>0)
        avg/=ar->size();
    float std=0;
    for (const SubwordSpottingResult& s : *ar)
    {
        std += pow(s.score-avg,2);
    }
    if (ar->size()>0)
        std = sqrt(std/ar->size());

    accumResMut.unlock();


    spotMut.lock();
    spottingAccums[ngram].push_back(ap);
    spottingAccumDifs[ngram].push_back(dif);
    spottingAccumAvgs[ngram].push_back(avg);
    spottingAccumStds[ngram].push_back(std);
    spotMut.unlock();


}
void GlobalK::storeSpottingExemplar(string ngram, float ap)
{
    spotMut.lock();
    spottingExemplars[ngram].push_back(ap);
    spotMut.unlock();
}
void GlobalK::storeSpottingNormal(string ngram, float ap)
{
    spotMut.lock();
    spottingNormals[ngram].push_back(ap);
    spotMut.unlock();
}
void GlobalK::storeSpottingOther(string ngram, float ap)
{
    spotMut.lock();
    spottingOthers[ngram].push_back(ap);
    spotMut.unlock();
}

vector<SubwordSpottingResult>* GlobalK::accumResFor(string ngram)
{
    accumResMut.lock();
    if (accumRes.find(ngram) == accumRes.end())
        accumRes[ngram] = new vector<SubwordSpottingResult>();
    vector<SubwordSpottingResult>* toRet= accumRes.at(ngram);
    accumResMut.unlock();
    return toRet;
}
#endif

#if defined(TEST_MODE) || defined(NO_NAN)
bool GlobalK::ngramAt(string ngram, int pageId, int tlx, int tly, int brx, int bry)
{
    float overlap_insides_thresh = OVERLAP_INSIDE_THRESH;
    float overlap_consume_thresh = OVERLAP_CONSUME_THRESH;
    float overlap_size_thresh = OVERLAP_SIDE_THRESH;
    float size_note_included_thresh = SIDE_NOT_INCLUDED_THRESH;


    WordBound searchL(tlx-5,tly-5,brx,bry);
    WordBound searchU(brx,bry,brx+5,bry+5);
    auto iterL = wordBounds[pageId].lower_bound(searchL);
    auto iterU = wordBounds[pageId].upper_bound(searchU);

    int bestOverlap = 0;
    const WordBound* best=NULL;
    auto iterBest = iterL;
    while (iterL != iterU)
    {
        int overlap = ( min(iterL->brx, brx)-max(iterL->tlx,tlx) ) *
            ( min(iterL->bry, bry)-max(iterL->tly,tly) );
        //assert(overlap != bestOverlap || bestOverlap==0); can occur with overlapping word images
        if (overlap > bestOverlap)
        {
            bestOverlap = overlap;
            best = &(*iterL);
            iterBest = iterL;
        }
        iterL++;
    }
    assert(best!=NULL);
    int loc = best->text.find(ngram);
    if (loc == string::npos)
        return false;

    //int ngramOverlap = min(best->startBounds[loc],tlx) - max(best->endBounds, brx);
    if (
            (tlx>=best->startBounds[loc] && brx<=best->endBounds[loc+ngram.size()-1] && (brx-tlx)/(best->endBounds[loc+ngram.size()-1]-best->startBounds[loc]+0.0) > overlap_insides_thresh) ||
            (tlx<=best->startBounds[loc] && brx>=best->endBounds[loc+ngram.size()-1] && (brx-tlx)/(best->endBounds[loc+ngram.size()-1]-best->startBounds[loc]+0.0) < overlap_consume_thresh) ||
            ((min(brx,best->endBounds[loc+ngram.size()-1])-max(tlx,best->startBounds[loc]))/(best->endBounds[loc+ngram.size()-1]-best->startBounds[loc]+0.0) > overlap_size_thresh && max(max(tlx,best->startBounds[loc])-min(tlx,best->startBounds[loc]),max(brx,best->endBounds[loc+ngram.size()-1])-min(brx,best->endBounds[loc+ngram.size()-1]))/(min(brx,best->endBounds[loc+ngram.size()-1])-max(tlx,best->startBounds[loc])+0.0) < size_note_included_thresh)
       )
    {
        return true;
    }
    
    return false;
}

void GlobalK::addWordBound(string word, int pageId, int tlx, int tly, int brx, int bry, vector<int> startBounds, vector<int> endBounds)
{
    wordBounds[pageId].emplace(word,tlx,tly,brx,bry,startBounds,endBounds);
}
#endif
