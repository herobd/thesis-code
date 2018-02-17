#include <atomic>

#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "Simulator.h"
#include "CATTSS.h"

#include "CTCWrapper.h"

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
        BatchWraper* batch;
        if (GlobalK::knowledge()->MANUAL_LINES)
        {
            batch= cattss->getLineBatch(500);
        }
        else
            batch = cattss->getBatch(5,500,0,prevNgram);
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
                float pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, pWordsBad;
                string misTrans_IV;
                float accTrans_IV,pWordsTrans_IV;
                float pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV;
                cattss->getStats(&accTrans,&pWordsTrans, &pWords80_100, &pWords60_80, &pWords40_60, &pWords20_40, &pWords0_20, &pWords0, &pWordsBad, &misTrans,
                            &accTrans_IV,&pWordsTrans_IV, &pWords80_100_IV, &pWords60_80_IV, &pWords40_60_IV, &pWords20_40_IV, &pWords0_20_IV, &pWords0_IV, &misTrans_IV);
                misTrans="[FINAL/MANUAL] "+misTrans;
                GlobalK::knowledge()->saveTrack(accTrans,pWordsTrans, pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, pWordsBad, misTrans,
                                            accTrans_IV,pWordsTrans_IV, pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV, misTrans_IV);
                GlobalK::knowledge()->writeTrack();
            }
            else
            {
                cout<<"ran out, so manual finish."<<endl;
                cattss->misc("manualFinish");
            }
        }
        else if (batch->getType()==CONTINUE_WORKING)
        {
            this_thread::sleep_for(chrono::minutes(1));
            slept+=1;
            bool spotterRunning, taskQueueEmpty;
            batch->getDebug(&spotterRunning,&taskQueueEmpty);
            cout<<"continue working: ";
            if (spotterRunning)
                cout<<"spotter running ";
            else
                cout<<"spotter finished ";
            cout <<"taskQueue ";
            if (!taskQueueEmpty)
                cout<<"not ";
            cout<<" empty."<<endl;
        }
        else
        {
            cout<<"Blank batch given to sim"<<endl;
            if (prevNgram.compare("---")==0)
            {
                cattss->misc("stopSpotting");
                cont->store(false);

                string misTrans;
                float accTrans,pWordsTrans;
                float pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, pWordsBad;
                string misTrans_IV;
                float accTrans_IV,pWordsTrans_IV;
                float pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV;
                cattss->getStats(&accTrans,&pWordsTrans, &pWords80_100, &pWords60_80, &pWords40_60, &pWords20_40, &pWords0_20, &pWords0, &pWordsBad, &misTrans,
                            &accTrans_IV,&pWordsTrans_IV, &pWords80_100_IV, &pWords60_80_IV, &pWords40_60_IV, &pWords20_40_IV, &pWords0_20_IV, &pWords0_IV, &misTrans_IV);
                misTrans="[FINAL/BLANK DONE] "+misTrans;
                GlobalK::knowledge()->saveTrack(accTrans,pWordsTrans, pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, pWordsBad, misTrans,
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
    //Mat test(45036,45036,CV_32F);
    /*
    cout<<"1"<<endl;
    CTCWrapper ctcWrapper(28, 10);
    Mat cpv = Mat::zeros(27,14,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(1,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(2,i)=1;
    float loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cout<<"2"<<endl;
    cpv = Mat::zeros(27,21,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(1,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(2,i)=1;
    for (int i=14; i<21; i++)
        cpv.at<float>(2,i)=1;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cout<<"3"<<endl;
    cpv = Mat::zeros(27,14,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(2,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(1,i)=1;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cpv = Mat::zeros(27,14,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(3,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(4,i)=1;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cpv = Mat::zeros(27,14,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(4,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(3,i)=1;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cpv = Mat::zeros(27,14,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(0,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(1,i)=1;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cpv = Mat::zeros(27,14,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(0,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(0,i)=1;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cout<<"8"<<endl;
    cpv = Mat::zeros(27,6,CV_32F);
    for (int i=0; i<3; i++)
        cpv.at<float>(1,i)=1;
    for (int i=3; i<6; i++)
        cpv.at<float>(2,i)=1;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;

    cout<<"9"<<endl;
    cpv = Mat::zeros(27,14,CV_32F);
    for (int i=0; i<7; i++)
        cpv.at<float>(1,i)=1;
    for (int i=7; i<14; i++)
        cpv.at<float>(2,i)=1;
    cpv.at<float>(1,4)=0.5;
    cpv.at<float>(3,4)=0.5;
    loss = ctcWrapper.loss(cpv,"ab");
    cout<<loss<<" "<<loss/cpv.cols<<endl;
    return 3;
    //*/


    int numSpottingThreads = 1;//CNNSPPSpotter will use the same network object
    int numSimThreads=1;
    //set<int> nsOfInterest;

    string dataname="BENTHAM";
    string lexiconFile = "/home/brian/intel_index/data/wordsEnWithNames.txt";
    string pageImageDir = "/home/brian/intel_index/data/bentham/BenthamDatasetR0-Images/Images/Pages";
    string segmentationFile = "/home/brian/intel_index/data/bentham/ben_cattss_c_corpus.gtp";
    string ngramWWFile = "/home/brian/intel_index/data/bentham/customWidths.txt";
    string charSegFile = "/home/brian/intel_index/data/bentham/manual_segmentations.csv";
    string spottingModelPrefix = "/home/brian/intel_index/data/bentham/network/phocnet_msf";//"model/CATTSS_BENTHAM";
    string savePrefix = "save/sim_net_BENTHAM";
    string SR_mode="fancy";
    if (argc==1)
    {
        cout<<"usage: "<<argv[0]<<" savePrefix simSave.csv [numSimThreads OR -FLAGS] [fancy,take_from_top,otsu_fixed,none_take_from_top,none,gaussian_draw,fancy_one,fancy_two, two_walk, phoc_trans,cpv_trans,web_trans,cluster_step,cluster_top,manual] lexiconFile.txt pageImageDir segmentationFile.gtp ngramWWFile.txt charSegFile.csv spottingModelPrefix [i (ideal)]"<<endl;
        return 0;
    }

    if (argc>1)
        savePrefix=argv[1];

    if (savePrefix.find("NAMES")!=string::npos)
        dataname="NAMES";
    if (savePrefix.find("noQbS")!=string::npos || savePrefix.find("NoQbS")!=string::npos)
         GlobalK::knowledge()->USE_QBE=false;
    else
        GlobalK::knowledge()->USE_QBE=true;

    if (argc>2)
        GlobalK::knowledge()->setSimSave(argv[2]);
    else
        GlobalK::knowledge()->setSimSave("save/simulationTracking_net_BENTHAM.csv");
    if (argc>3)
    {
        if (argv[3][0]=='-')
        {
            numSimThreads=0;
            /* FLAGS
             * e = QbS only
             * a# = auto-trans at len #
             * aa = no auto-approve
             * w = no wait
             */
            int i=1;
            while (argv[3][i]!='\0')
            {
                if (argv[3][i]=='e')
                {
                    GlobalK::knowledge()->USE_QBE=false;
                    cout<<"No QbE"<<endl;
                }
                else if (argv[3][i]=='a')
                {
                    i++;
                    if (argv[3][i]=='a')
                    {
                        GlobalK::knowledge()->AUTO_APPROVE=false;
                        cout<<"No auto approving spottings"<<endl;
                    }
                    else
                    {
                        GlobalK::knowledge()->AUTO_TRANS_LEN_THRESH=argv[3][i] - '0';
                        cout<<"Auto transcribe words less than "<<GlobalK::knowledge()->AUTO_TRANS_LEN_THRESH<<endl;
                    }
                }
                /*else if (argv[3][i]=='w')
                {
                    GlobalK::knowledge()->TRANS_DONT_WAIT=true;
                    cout<<"No waiting for transc"<<endl;
                }*/
                i++;
            }

        }
        else
            numSimThreads=atoi(argv[3]);
    }
    if (argc>4)
        SR_mode=argv[4];
    if (argc>5)
        lexiconFile=argv[5];
    if (argc>6)
        pageImageDir=argv[6];
    if (argc>7)
        segmentationFile=argv[7];
    if (argc>8)
        ngramWWFile=argv[8];
    if (argc>9)
        charSegFile=argv[9];
    if (argc>10)
        spottingModelPrefix=argv[10];

    if (argc>11)
    {
        for (int i=11; i<argc; i++)
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
                cout<<"WARNING, n-grams specified in width file"<<endl;
                //nsOfInterest.insert(atoi(argv[i]));
        }
    }
    //else
        //nsOfInterest.insert(2);

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
    else if (SR_mode.compare("two_walk")==0)
    {
        GlobalK::knowledge()->SR_TWO_WALK=true;
    }
    else if (SR_mode.substr(0,10).compare("phoc_trans")==0)
    {
        GlobalK::knowledge()->PHOC_TRANS=true;
        numSpottingThreads = 100;
        if (SR_mode.length()>10)
        {
            numSpottingThreads = stoi(SR_mode.substr(10));
            cout<<"trans top "<<numSpottingThreads<<"%"<<endl;
        }
    }
    else if (SR_mode.substr(0,9).compare("cpv_trans")==0)
    {
        GlobalK::knowledge()->CPV_TRANS=true;
        numSpottingThreads = 100;
        if (SR_mode.length()>9)
        {
            numSpottingThreads = stoi(SR_mode.substr(9));
            cout<<"trans top "<<numSpottingThreads<<"%"<<endl;
        }
    }
    else if (SR_mode.compare("web_trans")==0)
    {
        GlobalK::knowledge()->WEB_TRANS=true;
    }
    else if (SR_mode.compare("cluster_step")==0)
    {
        GlobalK::knowledge()->CLUSTER=true;
        numSpottingThreads = 1;
    }
    else if (SR_mode.compare("cluster_top")==0)
    {
        GlobalK::knowledge()->CLUSTER=true;
        numSpottingThreads = 0;
    } 
    else if (SR_mode.compare("manual")==0)
    {
        GlobalK::knowledge()->MANUAL_LINES=true;
        numSpottingThreads = 0;
    } 
    else if (SR_mode.compare("fancy")!=0)
    {
        cout<<"Error, unknown SpottingResults mode: "<<SR_mode<<endl;
        cout<<SR_mode.substr(0,10)<<endl;
        return 0;
    }

#ifndef TEST_MODE
//#ifndef DEBUG_AUTO
    Simulator sim(dataname,SR_mode,charSegFile);
//#else
//    Simulator sim("test",charSegFile);
//#endif
#else
    Simulator sim("test",SR_mode,charSegFile);
#endif
    /*int avgCharWidth=-1;
    if (dataname.compare("BENTHAM")==0)
        avgCharWidth=37;
    else if (dataname.compare("NAMES")==0)
        avgCharWidth=20;
    else if (dataname.compare("GW")==0)
        avgCharWidth=38;
    assert(avgCharWidth>0);*/

    int numTaskThreads = 7;
    int height = 1000;
    int width = 2500;
    int milli = 7000;
    CATTSS* cattss = new CATTSS(lexiconFile,
                        pageImageDir,
                        segmentationFile,
                        spottingModelPrefix,
                        savePrefix,
                        //nsOfInterest,
                        ngramWWFile,//avgCharWidth,
                        numSpottingThreads,
                        numTaskThreads,
                        height,
                        width,
                        milli,
                        0,//pad
                        numSimThreads==0
                        );
    if (GlobalK::knowledge()->MANUAL_LINES)
        cattss->initLines(0);
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
    cattss->save();
    //delete cattss;
    for (int i=0; i<numSimThreads; i++)
        delete taskThreads[i];
    this_thread::sleep_for(chrono::seconds(40));
    delete cattss;
}
