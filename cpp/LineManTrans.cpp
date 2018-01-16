#include "LineManTrans.h"

void saveSleeper(LineManTrans* lineManTrans)
{
    //This is the lowest priority of the systems threads
    nice(3);
    while(lineManTrans->cont()) {
        //cout <<"begin sleep"<<endl;
        this_thread::sleep_for(chrono::minutes(CHECK_SAVE_TIME_LINE));
        if (lineManTrans->cont())
        {
            lineManTrans->save();
        }
    }
}

LineManTrans::LineManTrans(
                string pageImageDir, 
                string segmentationFile, 
                string savePrefix,
                int contextPad
                ) : savePrefix(savePrefix)
{
    cont_A=true;
    ifstream in (savePrefix+"_LineManTrans.sav");
    if (in.good())
    {
        cout<<"Load file found."<<endl;
        corpus = new Knowledge::Corpus(in);
        lineQueue = new LineQueue(in,contextPad,corpus);

        string line;
        getline(in,line);
        cout<<"Loaded. Begins from time: "<<line<<endl;
        in.close();
    }
    else
    {
        corpus = new Knowledge::Corpus(contextPad, "none");
        corpus->addWordSegmentaionAndGT(pageImageDir, segmentationFile);
        lineQueue = new LineQueue(contextPad,corpus);
    }
    saveThread = new thread(saveSleeper,this);
    saveThread->detach();
}

BatchWraper* LineManTrans::getBatch(int width, int index)
{
#if !defined(TEST_MODE) && !defined(NO_NAN)
    try
    {
#endif

        BatchWraper* ret= lineQueue->getBatch(width,index);
        if (ret==NULL)
        {
            return new BatchWraperBlank();
        }
        return ret;
#if !defined(TEST_MODE) && !defined(NO_NAN)
    }
    catch (exception& e)
    {
        cout <<"Exception in CATTSS::getBatch(), "<<e.what()<<endl;
    }
    catch (...)
    {
        cout <<"Exception in CATTSS::getBatch(), UNKNOWN"<<endl;
    }
#endif
    return new BatchWraperBlank();
}

void LineManTrans::save()
{
    if (savePrefix.length()>0)
    {

        time_t timeSec;
        time(&timeSec);

        string saveName = savePrefix+"_LineManTrans.sav";
        //In the event of a crash while saveing, keep a backup of the last save
        rename( saveName.c_str() , (saveName+".bck").c_str() );

        ofstream out (saveName);

        corpus->save(out);
        lineQueue->save(out);
        out<<timeSec<<"\n";

        out.close();
    }
}
