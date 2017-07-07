#include <atomic>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "Simulator.h"
#include "CATTSS.h"

using namespace std;
using namespace cv;

void controlLoop(CATTSS* cattss, atomic_bool* cont)
{
    while(1)
    {
        cout<<"CONTROL:"<<endl<<": quit"<<endl<<": show"<<endl<<": manual"<<endl<<": save"<<endl;;
        string line;
        getline(cin, line);
        if (line.compare("quit")==0)
        {
            cattss->misc("stopSpotting");
            cont->store(false);
            break;
        }
        else if (line.compare("show")==0)
        {
            cattss->misc("showCorpus");
        }
        else if (line.compare("manual")==0)
        {
            cattss->misc("manualFinish");
        }
        else if (line.compare("save")==0)
        {
            cattss->misc("save");
        }
    }
}

void threadLoop(CATTSS* cattss, Simulator* sim, atomic_bool* cont, bool noManual)
{
    string prevNgram="";
    int slept=0;
    thread::id thread_id = this_thread::get_id();
    stringstream ss;
    ss<<thread_id;
    string thread = ss.str();
    while (cont->load())
    {
        BatchWraper* batch = cattss->getBatch(5,500,0,prevNgram);
        if (batch->getType()==SPOTTINGS)
        {
            string id;
            vector<string> ids;
            string ngram;
            vector<Location> locs;
            vector<string> gt;
            batch->getSpottings(&id,&ngram,&ids,&locs,&gt);
            vector<int> labels = sim->spottings(ngram,locs,gt,prevNgram);

#ifdef DEBUG_AUTO
            vector<Mat> images = batch->getImages();
            for (int i=0; i<labels.size(); i++)
            {
                if (gt[i].length()>2)
                {
                    string name = thread;
                    if (labels[i])
                        name+="_TRUE";
                    else 
                        name+="_FALSE";
                    imshow(name,images.at(i));
                    do
                    {
                        cout<<name<<"> ["<<ngram<<"] Loc page:"<<locs[i].pageId<<", bb: "<<locs[i].x1<<" "<<locs[i].y1<<" "<<locs[i].x2<<" "<<locs[i].y2<<endl;
                    }
                    while(waitKey() != 10);//enter
                }
            }
#endif

            cattss->updateSpottings(id,ids,labels,0);
            prevNgram = ngram;
        }
        else if (batch->getType()==NEW_EXEMPLARS)
        {
            string id;
            vector<string> ngrams;
            vector<Location> locs;
            batch->getNewExemplars(&id,&ngrams,&locs);
            vector<int> labels = sim->newExemplars(ngrams,locs,prevNgram);
#ifdef DEBUG_AUTO
            vector<Mat> images = batch->getImages();
            for (int i=0; i<labels.size(); i++)
            {
                string name = thread;
                if (labels[i])
                    name+="_TRUE";
                else 
                    name+="_FALSE";
                imshow(name,images[i]);
                do
                {
                    cout<<name<<"> ["<<ngrams[i]<<"] Loc page:"<<locs[i].pageId<<", bb: "<<locs[i].x1<<" "<<locs[i].y1<<" "<<locs[i].x2<<" "<<locs[i].y2<<endl;
                }
                while(waitKey() != 10);//enter
            }
#endif
            cattss-> updateNewExemplars(id,labels,0);
            prevNgram=ngrams.back();
        }
        else if (batch->getType()==TRANSCRIPTION)
        {
            string batchId;
            int wordIndex;
            vector<SpottingPoint> spottings;
            vector<string> poss;
            bool manual;
            string gt;
            batch->getTranscription(&batchId,&wordIndex,&spottings,&poss,&manual,&gt);
            string trans;
            if (manual)
            {
                trans=sim->manual(wordIndex,poss,gt,prevNgram.compare("#")==0);
                prevNgram="#";
            }
            else
            {
                trans=sim->transcription(wordIndex,spottings,poss,gt,prevNgram.compare("!")==0);
                prevNgram="!";
            }
#ifdef DEBUG_AUTO
#endif
            cattss->updateTranscription(batchId,trans,manual);
        }
        else if (batch->getType()==RAN_OUT)
        {
            //this_thread::sleep_for(chrono::minutes(15));
            //slept+=15;
            prevNgram="_";
            if (noManual)
            {
                cout<<"manual hit, finishing"<<endl;
                cattss->misc("stopSpotting");
                cont->store(false);

                string misTrans;
                float accTrans,pWordsTrans;
                float pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0;
                string misTrans_IV;
                float accTrans_IV,pWordsTrans_IV;
                float pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV;
                cattss->getStats(&accTrans,&pWordsTrans, &pWords80_100, &pWords60_80, &pWords40_60, &pWords20_40, &pWords0_20, &pWords0, &misTrans,
                            &accTrans_IV,&pWordsTrans_IV, &pWords80_100_IV, &pWords60_80_IV, &pWords40_60_IV, &pWords20_40_IV, &pWords0_20_IV, &pWords0_IV, &misTrans_IV);
                misTrans="[FINAL/MANUAL] "+misTrans;
                GlobalK::knowledge()->saveTrack(accTrans,pWordsTrans, pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, misTrans,
                                            accTrans_IV,pWordsTrans_IV, pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV, misTrans_IV);
                GlobalK::knowledge()->writeTrack();
            }
            else
            {
                cout<<"ran out, so manual finish."<<endl;
                cattss->misc("manualFinish");
            }
        }
        else
        {
            cout<<"Blank batch given to sim"<<endl;
            if (prevNgram.compare("-----")==0)
            {
                cattss->misc("stopSpotting");
                cont->store(false);

                string misTrans;
                float accTrans,pWordsTrans;
                float pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0;
                string misTrans_IV;
                float accTrans_IV,pWordsTrans_IV;
                float pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV;
                cattss->getStats(&accTrans,&pWordsTrans, &pWords80_100, &pWords60_80, &pWords40_60, &pWords20_40, &pWords0_20, &pWords0, &misTrans,
                            &accTrans_IV,&pWordsTrans_IV, &pWords80_100_IV, &pWords60_80_IV, &pWords40_60_IV, &pWords20_40_IV, &pWords0_20_IV, &pWords0_IV, &misTrans_IV);
                misTrans="[FINAL/BLANK DONE] "+misTrans;
                GlobalK::knowledge()->saveTrack(accTrans,pWordsTrans, pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, misTrans,
                                            accTrans_IV,pWordsTrans_IV, pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV, misTrans_IV);
                GlobalK::knowledge()->writeTrack();
            }
            else
            {
                this_thread::sleep_for(chrono::minutes(1));
                slept+=1;
                if (prevNgram[0]=='-')
                    prevNgram+="-";
                else
                    prevNgram="-";
            }

        }

        delete batch;
        
    }
    cout<<"Thread slept: "<<slept<<endl;
}




int main(int argc, char** argv)
{
    int numSimThreads=1;
    set<int> nsOfInterest;

    string dataname="BENTHAM";
    string lexiconFile = "/home/brian/intel_index/data/wordsEnWithNames.txt";
    string pageImageDir = "/home/brian/intel_index/data/bentham/BenthamDatasetR0-Images/Images/Pages";
    string segmentationFile = "/home/brian/intel_index/data/bentham/ben_cattss_c_corpus.gtp";
    string charSegFile = "/home/brian/intel_index/data/bentham/manual_segmentations.csv";
    string spottingModelPrefix = "/home/brian/intel_index/data/bentham/network/phocnet_msf";//"model/CATTSS_BENTHAM";
    string savePrefix = "save/sim_net_BENTHAM";
    string SR_mode="fancy";
    if (argc==1)
    {
        cout<<"usage: "<<argv[0]<<" savePrefix simSave.csv numSimThreads [fancy,take_from_top,otsu_fixed,none_take_from_top,none,gaussian_draw,fancy_one,fancy_two] lexiconFile.txt pageImageDir segmentationFile.gtp charSegFile.csv spottingModelPrefix (ngram list)"<<endl;
        return 0;
    }

    if (argc>1)
        savePrefix=argv[1];
    if (argc>2)
        GlobalK::knowledge()->setSimSave(argv[2]);
    else
        GlobalK::knowledge()->setSimSave("save/simulationTracking_net_BENTHAM.csv");
    if (argc>3)
        numSimThreads=atoi(argv[3]);
    if (argc>4)
        SR_mode=argv[4];
    if (argc>5)
        lexiconFile=argv[5];
    if (argc>6)
        pageImageDir=argv[6];
    if (argc>7)
        segmentationFile=argv[7];
    if (argc>8)
        charSegFile=argv[8];
    if (argc>9)
        spottingModelPrefix=argv[9];

    if (argc>10)
    {
        for (int i=10; i<argc; i++)
        {
            if (argv[i][0]=='i')
            {
                GlobalK::knowledge()->IDEAL_COMB=true;
            }
            else if (argv[i][0]=='0' || argv[i][0]=='.')
            {
                GlobalK::knowledge()->MIN_SPOTTING_AP=atof(argv[i]);
            }
            else
                nsOfInterest.insert(atoi(argv[i]));
        }
    }
    else
        nsOfInterest.insert(2);

    if (SR_mode.compare("take_from_top")==0)
    {
        GlobalK::knowledge()->SR_TAKE_FROM_TOP=true;
    }
    else if (SR_mode.compare("otsu_fixed")==0)
    {
        GlobalK::knowledge()->SR_OTSU_FIXED=true;
    }
    else if (SR_mode.compare("none_take_from_top")==0)
    {
        GlobalK::knowledge()->SR_TAKE_FROM_TOP=true;
        GlobalK::knowledge()->SR_THRESH_NONE=true;
    }
    else if (SR_mode.compare("none")==0)
    {
        GlobalK::knowledge()->SR_THRESH_NONE=true;
    }
    else if (SR_mode.compare("gaussian_draw")==0)
    {
        GlobalK::knowledge()->SR_GAUSSIAN_DRAW=true;
    }
    else if (SR_mode.compare("fancy_one")==0)
    {
        GlobalK::knowledge()->SR_FANCY_ONE=true;
    }
    else if (SR_mode.compare("fancy_two")==0)
    {
        GlobalK::knowledge()->SR_FANCY_TWO=true;
    }
    else if (SR_mode.compare("phoc_trans")==0)
    {
        GlobalK::knowledge()->PHOC_TRANS=true;
    }
    else if (SR_mode.compare("fancy")!=0)
    {
        cout<<"Error, unknown SpottingResults mode: "<<SR_mode<<endl;
        return 0;
    }


//#ifndef DEBUG_AUTO
    Simulator sim(dataname,charSegFile);
//#else
//    Simulator sim("test",charSegFile);
//#endif
    int avgCharWidth=-1;
    if (dataname.compare("BENTHAM")==0)
        avgCharWidth=37;
    else if (dataname.compare("NAMES")==0)
        avgCharWidth=20;
    else if (dataname.compare("GW")==0)
        avgCharWidth=38;
    assert(avgCharWidth>0);

    int numSpottingThreads = 1;//CNNSPPSpotter will use the same network object
    int numTaskThreads = 3;
    int height = 1000;
    int width = 2500;
    int milli = 7000;
    CATTSS* cattss = new CATTSS(lexiconFile,
                        pageImageDir,
                        segmentationFile,
                        spottingModelPrefix,
                        savePrefix,
                        nsOfInterest,
                        avgCharWidth,
                        numSpottingThreads,
                        numTaskThreads,
                        height,
                        width,
                        milli,
                        0,//pad
                        numSimThreads==0
                        );
    atomic_bool cont(true);
    vector<thread*> taskThreads(numSimThreads);
    //string line;
    //cout<<"WAITING FOR ENTRY BEFORE BEGINNING SIM"<<endl;
    //getline(cin, line);
    cout<<"SIMULATION STARTED"<<endl;
    for (int i=0; i<numSimThreads; i++)
    {
        taskThreads[i] = new thread(threadLoop,cattss,&sim,&cont,false);
        taskThreads[i]->detach();
        
    }
    if (numSimThreads==0)
    {
        cout<<"Non-interactive mode. Will terminate on manual."<<endl;
        threadLoop(cattss,&sim,&cont,true);
    }
    else
        controlLoop(cattss,&cont);

    cout<<"---DONE---"<<endl;
    //delete cattss;
    for (int i=0; i<numSimThreads; i++)
        delete taskThreads[i];
    this_thread::sleep_for(chrono::seconds(40));
    delete cattss;
}
