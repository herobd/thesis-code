#include "CATTSS.h"
//

void checkIncompleteSleeper(CATTSS* cattss, MasterQueue* q, Knowledge::Corpus* c)
{
    //This is the lowest priority of the systems threads
    nice(3);
    //this_thread::sleep_for(chrono::hours(1));
    //cout <<"kill is "<<q->kill.load()<<endl;
#ifdef NO_NAN
    int count=0;
#endif
    while(!q->kill.load() && cattss->getCont()) {
        //cout <<"begin sleep"<<endl;
        this_thread::sleep_for(chrono::minutes(CHECK_SAVE_TIME));
        if (!q->kill.load() && cattss->getCont())
        {
            cattss->save();
#ifdef NO_NAN
            GlobalK::knowledge()->writeTrack();
            if (++count % 39 == 0)
            {
                q->checkIncomplete();
                c->checkIncomplete();
            }
#else
            q->checkIncomplete();
            c->checkIncomplete();
#endif
        }
    }
}
void showSleeper(CATTSS* cattss, MasterQueue* q, Knowledge::Corpus* c, int height, int width, int milli)
{
    //This is the lowest priority of the systems threads
    nice(3);
#ifdef NO_NAN
    int count=0;
#endif
    while(!q->kill.load() && cattss->getCont()) {
        this_thread::sleep_for(chrono::milliseconds(milli));
        if (!q->kill.load() && cattss->getCont())
        {
#if SHOW_PROGRESS
            c->showProgress(height,width);
#endif
#ifdef NO_NAN
            if (count++ % 5==0) //every 20 seconds
            {
                string misTrans;
                float accTrans,pWordsTrans;
                float pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, pWordsBad;
                string misTrans_IV;
                float accTrans_IV,pWordsTrans_IV;
                float pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV;
                c->getStats(&accTrans,&pWordsTrans, &pWords80_100, &pWords60_80, &pWords40_60, &pWords20_40, &pWords0_20, &pWords0, &pWordsBad, &misTrans,
                            &accTrans_IV,&pWordsTrans_IV, &pWords80_100_IV, &pWords60_80_IV, &pWords40_60_IV, &pWords20_40_IV, &pWords0_20_IV, &pWords0_IV, &misTrans_IV);
                GlobalK::knowledge()->saveTrack(accTrans,pWordsTrans, pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, pWordsBad, misTrans,
                                                accTrans_IV,pWordsTrans_IV, pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV, misTrans_IV);
            }
#endif
        }
    }
}

//for checking stuff
CATTSS::CATTSS(  
                string save,
                string outCompleted,
                string outIncomplete)
{

    ifstream in (save);
    if (in.good())
    {
        cout<<"Load file found."<<endl;
        Lexicon::instance()->load(in);
        corpus = new Knowledge::Corpus(in);
        CorpusRef* corpusRef = corpus->getCorpusRef();
        PageRef* pageRef = corpus->getPageRef();
        masterQueue = new MasterQueue(in,corpusRef,pageRef,save);
        in.close();

        if (outCompleted.length()>0 && outIncomplete.length()>0)
        {
            ofstream oC(outCompleted);
            ofstream oI(outIncomplete);
            for (int i=0; i<corpus->size(); i++)
            {
                bool done;
                string gt, toss;
                corpus->getWord(i)->getDoneAndGTAndQuery(&done, &gt, &toss);
                if (done)
                    oC<<i<<","<<gt<<endl;
                else
                    oI<<i<<","<<gt<<endl;
            }
            oC.close();
            oI.close();
        }
    }
    else
        cout<<"error, could not read "<<save<<endl;
}


CATTSS::CATTSS( string lexiconFile, 
                string pageImageDir, 
                string segmentationFile, 
                string spottingModelPrefix,
                string savePrefix,
                //set<string> ngramsOfInterest,
                string ngramWWFile, //int avgCharWidth,
                int numSpottingThreads,
                int numTaskThreads,
                int showHeight,
                int showWidth,
                int showMilli,
                int contextPad,
                bool noManual) : savePrefix(savePrefix), noManual(noManual) //nsOfInterest(nsOfInterest), 
{
    cont.store(1);
    sem_init(&semLock, 0, 0);

    ifstream in (savePrefix+"_CATTSS.sav");
    if (in.good())
    {
        cout<<"Load file found."<<endl;
        Lexicon::instance()->load(in);
        corpus = new Knowledge::Corpus(in);
        corpus->loadSpotter(spottingModelPrefix);
        CorpusRef* corpusRef = corpus->getCorpusRef();
        PageRef* pageRef = corpus->getPageRef();
        masterQueue = new MasterQueue(in,corpusRef,pageRef,savePrefix);
        spottingQueue = new SpottingQueue(in,masterQueue,corpus);

        if (GlobalK::knowledge()->WEB_TRANS)
        {
            web = new Web(in,corpus);
        }
        delete corpusRef;
        delete pageRef;

        string line;
        getline(in,line);
        int tSize = stoi(line);
        for (int i=0; i<tSize; i++)
        {

            UpdateTask burn(in); //we simply burn these as we don't save the appropriate data in other objects
            //taskQueue.push_back(new UpdateTask(in));
            //sem_post(&semLock);
        }
        getline(in,line);
        SpottingsBatch::setIdCounter(stoul(line));
        getline(in,line);
        NewExemplarsBatch::setIdCounter(stoul(line));
        getline(in,line);
        TranscribeBatch::setIdCounter(stoul(line));
        getline(in,line);
        cout<<"Loaded. Begins from time: "<<line<<endl;
        in.close();
    }
    else
    {
    
        masterQueue = new MasterQueue(contextPad,savePrefix);
        Lexicon::instance()->readIn(lexiconFile);
        vector<string> ngrams;
        corpus = new Knowledge::Corpus(contextPad, ngramWWFile, &ngrams);//the ngrams happen to be read in, so we just get them as a convience
        corpus->addWordSegmentaionAndGT(pageImageDir, segmentationFile);
        corpus->loadSpotter(spottingModelPrefix);
        spottingQueue = new SpottingQueue(masterQueue,corpus);
        if (GlobalK::knowledge()->WEB_TRANS)
        {
            web = new Web(corpus);
        }

#ifdef TEST_MODE_LONG
        int pageId=0;

        //Spotting er1 (433,14,472,36,pageId,corpus->imgForPageId(pageId),"er",0.0);//[1]
        //vector<Spotting > init = {er1};
        //masterQueue->updateSpottingResults(new vector<Spotting>(init));
        Spotting* er1 = new Spotting(811,18,842,40,pageId,corpus->imgForPageId(pageId),"er",0);//[1]
        vector<Spotting* > init = {er1};
        spottingQueue->addQueries(init);
#else
        if (GlobalK::knowledge()->PHOC_TRANS)
        {
            float transKeep = numSpottingThreads/100.0;
            vector<TranscribeBatch*> newBatches = corpus->phocTrans(transKeep);
            masterQueue->enqueueTranscriptionBatches(newBatches,NULL);
        }
        else
        {
            /*we read in ngrams now
            if (nsOfInterest.find(3)!=nsOfInterest.end())
            {
                //vector<string> top100Trigrams={"the","and","ing","ion","tio","ent","ati","for","her","ter","hat","tha","ere","ate","his","con","res","ver","all","ons","nce","men","ith","ted","ers","pro","thi","wit","are","ess","not","ive","was","ect","rea","com","eve","per","int","est","sta","cti","ica","ist","ear","ain","one","our","iti","rat","nte","tin","ine","der","ome","man","pre","rom","tra","whi","ave","str","act","ill","ure","ide","ove","cal","ble","out","sti","tic","oun","enc","ore","ant","ity","fro","art","tur","par","red","oth","eri","hic","ies","ste","ght","ich","igh","und","you","ort","era","wer","nti","oul","nde","ind","tho"};
                //vector<string> top300Trigrams={"the","and","ing","ion","tio","ent","ati","for","her","ter","hat","tha","ere","ate","his","con","res","ver","all","ons","nce","men","ith","ted","ers","pro","thi","wit","are","ess","not","ive","was","ect","rea","com","eve","per","int","est","sta","cti","ica","ist","ear","ain","one","our","iti","rat","nte","tin","ine","der","ome","man","pre","rom","tra","whi","ave","str","act","ill","ure","ide","ove","cal","ble","out","sti","tic","oun","enc","ore","ant","ity","fro","art","tur","par","red","oth","eri","hic","ies","ste","ght","ich","igh","und","you","ort","era","wer","nti","oul","nde","ind","tho","hou","nal","but","hav","uld","use","han","hin","een","ces","cou","lat","tor","ese","age","ame","rin","anc","ten","hen","min","eas","can","lit","cha","ous","eat","end","ssi","ial","les","ren","tiv","nts","whe","tat","abl","dis","ran","wor","rou","lin","had","sed","ont","ple","ugh","inc","sio","din","ral","ust","tan","nat","ins","ass","pla","ven","ell","she","ose","ite","lly","rec","lan","ard","hey","rie","pos","eme","mor","den","oug","tte","ned","rit","ime","sin","ast","any","orm","ndi","ona","spe","ene","hei","ric","ice","ord","omp","nes","sen","tim","tri","ern","tes","por","app","lar","ntr","eir","sho","son","cat","lle","ner","hes","who","mat","ase","kin","ost","ber","its","nin","lea","ina","mpl","sto","ari","pri","own","ali","ree","ish","des","ead","nst","sit","ses","ans","has","gre","ong","als","fic","ual","ien","gen","ser","unt","eco","nta","ace","chi","fer","tal","low","ach","ire","ang","sse","gra","mon","ffe","rac","sel","uni","ake","ary","wil","led","ded","som","owe","har","ini","ope","nge","uch","rel","che","ade","att","cia","exp","mer","lic","hem","ery","nsi","ond","rti","duc","how","ert","see","now","imp","abo","pec","cen","ris","mar","ens","tai","ely","omm","sur","hea"};
                if (GlobalK::knowledge()->CPV_TRANS || GlobalK::knowledge()->WEB_TRANS || GlobalK::knowledge()->CLUSTER)
                {
                    //ngrams.insert(ngrams.begin(),top300Trigrams.begin(),top300Trigrams.end());
                    ngrams.insert(ngrams.begin(),GlobalK::knowledge()->trigrams.begin(),GlobalK::knowledge()->trigrams.end());
                }
                else
                    spottingQueue->addQueries(GlobalK::knowledge()->trigrams);
            }
            if (nsOfInterest.find(2)!=nsOfInterest.end())
            {
                //vector<string> top100Bigrams={"th","he","in","er","an","re","on","at","en","nd","ti","es","or","te","of","ed","is","it","al","ar","st","to","nt","ng","se","ha","as","ou","io","le","ve","co","me","de","hi","ri","ro","ic","ne","ea","ra","ce","li","ch","ll","be","ma","si","om","ur","ca","el","ta","la","ns","di","fo","ho","pe","ec","pr","no","ct","us","ac","ot","il","tr","ly","nc","et","ut","ss","so","rs","un","lo","wa","ge","ie","wh","ee","wi","em","ad","ol","rt","po","we","na","ul","ni","ts","mo","ow","pa","im","mi","ai","sh"};
                if (GlobalK::knowledge()->CPV_TRANS || GlobalK::knowledge()->WEB_TRANS || GlobalK::knowledge()->CLUSTER)
                {
                    //ngrams.insert(ngrams.begin(),top100Bigrams.begin(),top100Bigrams.end());
                    ngrams.insert(ngrams.begin(),GlobalK::knowledge()->bigrams.begin(),GlobalK::knowledge()->bigrams.end());
                }
                else
                    spottingQueue->addQueries(GlobalK::knowledge()->bigrams);
            }
            if (nsOfInterest.find(1)!=nsOfInterest.end())
            {
                //vector<string> orderedAlpha={"e","t","a","o","i","n","s","h","r","d","l","c","u","m","w","f","g","y","p","b","v","k","j","x","q","z"};
                if (GlobalK::knowledge()->CPV_TRANS || GlobalK::knowledge()->WEB_TRANS || GlobalK::knowledge()->CLUSTER)
                {
                    //ngrams.insert(ngrams.begin(),orderedAlpha.begin(),orderedAlpha.end());
                    ngrams.insert(ngrams.begin(),GlobalK::knowledge()->unigrams.begin(),GlobalK::knowledge()->unigrams.end());
                }
                else
                    spottingQueue->addQueries(GlobalK::knowledge()->unigrams);
            }*/

            if (GlobalK::knowledge()->CPV_TRANS)
            {
#ifdef CTC
                float transKeep = numSpottingThreads/100.0;
                cout<<"Commencing CPV-CTC transcription."<<endl;
                vector<TranscribeBatch*> newBatches = corpus->cpvTransCTC(transKeep,ngrams);
                cout<<"Finished CPV-CTC transcription."<<endl;
                masterQueue->enqueueTranscriptionBatches(newBatches,NULL);
#else
                assert(false && "CTC comp flag not set");
#endif
                /*
                if (regex)
                   newBatches = corpus->npvTransRegex(ngrams);
                else
                   newBatches = corpus->npvTransDirect(ngrams);
                   */
            }
            else if (GlobalK::knowledge()->WEB_TRANS)
            {
//#ifdef WEB_TRANS
                //float transKeep = numSpottingThreads/100.0;
                cout<<"Commencing WEB transcription."<<endl;
                Mat crossScores;
                cout<<"Commencing massSpot..."<<endl;
                vector<SpottingLoc> massSpottingRes = corpus->massSpot(ngrams,crossScores);//expects QbS scores to be adjusted
                cout<<"...finished massSpot."<<endl;
                vector<Spotting>* toAdd = web->start(ngrams,massSpottingRes,crossScores);
                vector<TranscribeBatch*> newBatches = corpus->updateSpottings(toAdd,NULL,NULL,NULL,NULL);
                cout<<"Finished WEB transcription."<<endl;
                masterQueue->enqueueTranscriptionBatches(newBatches,NULL);
                //orderedTranscribeQueue->enqueue(newBatches);
//#else
//                assert(false && "WEB_TRANS comp flag not set");
//#endif
            }
            else if (GlobalK::knowledge()->CLUSTER)
            {
                for (string ngram : ngrams)
                {
                    Mat crossScores;
                    vector<string> n = {ngram};
                    vector<SpottingLoc> massSpottingRes = corpus->massSpot(n,crossScores);
                    vector<Spotting> spottings;
                    int tC=0;
                    for (const SpottingLoc& s : massSpottingRes)
                    {
                        Knowledge::Word* word = corpus->getWord(s.imIdx);
                        int tlx,tly,brx,bry;
                        bool done;
                        string label;
                        int gt=0;
                        word->getBoundsAndDoneAndGT(&tlx,&tly,&brx,&bry,&done,&label);
                        brx=std::min(tlx+s.endX,brx);
                        tlx+=s.startX;
                        if (label.find(ngram) != string::npos)
                        {
#if defined(TEST_MODE) || defined(NO_NAN)
                            gt = GlobalK::knowledge()->ngramAt_word(ngram,s.imIdx,s.startX,s.endX);
#else
                            gt = -1;
#endif
                        }
                        if (gt>0)
                            tC++;
                       
                        spottings.emplace_back(tlx,tly,brx,bry,word->getPageId(),word->getPage(),ngram,gt,s.imIdx,s.startX);
                        spottings.back().scoreQbS=s.score();
                        //NOO! spottings.back().id=spottings.size();//override here for simplicity
                    }
                    //assert(tC>0); 'you' is not in the corpus
                    bool stepMode = numSpottingThreads;
                    masterQueue->insertClusterBatcher(ngram,contextPad,stepMode,spottings,crossScores);
                }
            }
            else
            {
                spottingQueue->addQueries(ngrams);
            }
        }

#endif
    }

#ifdef TEST_MODE
    in.open(segmentationFile+".char");
    string line;
    //getline(in,line);//header
    while (getline(in,line))
    {
        string s;
        std::stringstream ss(line);
        getline(ss,s,',');
        string word=s;
        getline(ss,s,',');
        int pageId = (stoi(s)+1); //+1 is correction from my creation of segmentation file
        getline(ss,s,',');//x1
        int x1=stoi(s);
        getline(ss,s,',');
        int y1=stoi(s);
        getline(ss,s,',');//x2
        int x2=stoi(s);
        getline(ss,s,',');
        int y2=stoi(s);
        vector<int> lettersStart, lettersEnd;
        //vector<int> lettersStartRel, lettersEndRel;

        while (getline(ss,s,','))
        {
            lettersStart.push_back(stoi(s));
            //lettersStartRel.push_back(stoi(s)-x1);
            getline(ss,s,',');
            lettersEnd.push_back(stoi(s));
            //lettersEndRel.push_back(stoi(s)-x1);
            //getline(ss,s,',');//conf
        }
        GlobalK::knowledge()->addWordBound(word,pageId,x1,y1,x2,y2,lettersStart,lettersEnd);
    }
    in.close();
#endif
    
   
    //should be priority 77 
    incompleteChecker = new thread(checkIncompleteSleeper,this,masterQueue,corpus);//This could be done by a thread for each sr
    incompleteChecker->detach();
    showChecker = new thread(showSleeper,this,masterQueue,corpus,showHeight,showWidth,showMilli);
    showChecker->detach();
//#ifndef GRAPH_SPOTTING_RESULTS
    spottingQueue->run(numSpottingThreads);
//#endif
    run(numTaskThreads);
    //test
    /*
        Spotting s1(1000, 1458, 1154, 1497, 2720272, corpus->imgForPageId(2720272), "ma", 0.01);
        Spotting s2(1196, 1429, 1288, 1491, 2720272, corpus->imgForPageId(2720272), "ch", 0.01);
        Spotting s3(1114, 1465, 1182, 1496, 2720272, corpus->imgForPageId(2720272), "ar", 0.01);
        Spotting s4(345, 956, 415, 986, 2720272, corpus->imgForPageId(2720272), "or", 0.01);
        Spotting s5(472, 957, 530, 987, 2720272, corpus->imgForPageId(2720272), "er", 0.01);
        Spotting s6(535, 943, 634, 986, 2720272, corpus->imgForPageId(2720272), "ed", 0.01);
        Spotting s7(355, 1046, 455, 1071, 2720272, corpus->imgForPageId(2720272), "un", 0.01);
        Spotting s8(492, 1030, 553, 1069, 2720272, corpus->imgForPageId(2720272), "it", 0.01);
        Spotting s9(439, 1024, 507, 1096, 2720272, corpus->imgForPageId(2720272), "fi", 0.01);
        vector<Spotting> toAdd={s1,s2,s3,s4,s5,s6,s7,s8,s9};
        vector<TranscribeBatch*> newBatches = corpus->updateSpottings(&toAdd,NULL,NULL);
        assert(newBatches.size()>0);
        masterQueue->enqueueTranscriptionBatches(newBatches);
        if (toAdd.size()>9)
        {
            cout <<"harvested (init):"<<endl;
            for (int i=9; i<toAdd.size(); i++)
                imshow("har: "+toAdd[i].ngram,toAdd[i].img());
            waitKey();
        }

        vector<unsigned long> toRemoveSpottings={s7.id};
        vector<unsigned long> toRemoveBatches;        
        toAdd.clear();
        Spotting s10(356, 780, 423, 816, 2720272, corpus->imgForPageId(2720272), "on", 0.01);
        Spotting s11(429, 765, 494, 864, 2720272, corpus->imgForPageId(2720272), "ly", 0.01);
        toAdd.push_back(s10); toAdd.push_back(s11);
        newBatches = corpus->updateSpottings(&toAdd,&toRemoveSpottings,&toRemoveBatches);
        if (toAdd.size()>2)
        {
            cout <<"harvested (init):"<<endl;
            for (int i=9; i<toAdd.size(); i++)
                imshow("har: "+toAdd[i].ngram,toAdd[i].img());
            waitKey();
        }
        //vector<TranscribeBatch*> modBatches = corpus->removeSpottings(toRemoveSpottings,toRemoveBatches);
        masterQueue->enqueueTranscriptionBatches(newBatches,&toRemoveBatches);
        cout <<"Enqueued "<<newBatches.size()<<" new trans batches"<<endl;            
        if (toRemoveBatches.size()>0)
            cout <<"Removed "<<toRemoveBatches.size()<<" trans batches"<<endl;            
    /**/
    //test

}
BatchWraper* CATTSS::getBatch(int num, int width, int color, string prevNgram)
{
#if !defined(TEST_MODE) && !defined(NO_NAN)
    try
    {
#else
        //cout<<"getBatch, color:"<<color<<", prev:"<<prevNgram<<endl;
#endif

#if MANUAL_ONLY
        TranscribeBatch* batSp = corpus->getManualBatch(width);
        if (batSp!=NULL)
            return new BatchWraperTranscription(batSp);
        else
            return new BatchWraperBlank();
#endif

        bool hard=true;
        if (num==-1) {
            num=5;
            hard=false;

        }
        BatchWraper* ret= masterQueue->getBatch(num,hard,width,color,prevNgram);
        if (ret==NULL && !noManual)
        {
            TranscribeBatch* bat = corpus->getManualBatch(width);
            if (bat!=NULL)
                ret = new BatchWraperTranscription(bat);
        }
#ifdef TEST_MODE_C
        return ret;
#endif
        if (ret!=NULL)
            return ret;
        else
        {
            return new BatchWraperBlank();
        }
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


void CATTSS::updateSpottings(string resultsId, vector<string> ids, vector<int> labels, int resent)
{
    enqueue(new UpdateTask(resultsId,ids,labels,resent));
}

void CATTSS::updateTranscription(string id, string transcription, bool manual)
{
    enqueue(new UpdateTask(id,transcription,manual));
}

void CATTSS::updateNewExemplars(string batchId,  vector<int> labels, int resent)
{
    enqueue(new UpdateTask(batchId,labels,resent));
}

void CATTSS::misc(string task)
{
#if !defined(TEST_MODE) && !defined(NO_NAN)
    try
    {
#endif
        if (task.compare("showCorpus")==0)
        {
            corpus->show();
        }
        else if (task.length()>16 && task.substr(0,16).compare("showInteractive:")==0)
        {
            int pageNum = stoi(task.substr(16));
            corpus->showInteractive(pageNum);
        }
/*        else if (task.length()>13 && task.substr(0,13).compare("showProgress:")==0)
        {
            cout <<"CATTSS::showProgress()"<<endl;
            string dims = task.substr(13);
            int split = dims.find_first_of(',');
            int split2 = dims.find_last_of(',');
            int height = stoi(dims.substr(0,split));
            int width = stoi(dims.substr(1+split,split2));
            int milli = stoi(dims.substr(1+split2));
            showChecker = new thread(showSleeper,masterQueue,corpus,height,width,milli);
            showChecker->detach();
        }*/
        else if (task.compare("stopSpotting")==0)
        {
            spottingQueue->stop();
            stop();
        }
        else if (task.compare("manualFinish")==0)
        {
            masterQueue->setFinish(true);
            cout<<"Manual Finish engaged."<<endl;
        }
        else if (task.compare("save")==0)
        {
            save();
        }
#ifdef TEST_MODE
        else if (task.length()>11 && task.substr(0,11).compare("forceNgram:")==0)
        {
            masterQueue->forceNgram = task.substr(11);
        }
        else if (task.compare("unforce")==0 || task.compare("unforceNgram")==0)
        {
            masterQueue->forceNgram="";
        }
#endif
        /*else if (task.length()>14 && task.substr(0,14).compare("startSpotting:")==0)
        {
            int num = stoi(task.substr(14));
            //cout<<"startSpotting:"<<num<<endl;
            if (num>0)
                spottingQueue->run(num);
            else
                cout<<"ERROR: tried to start spotting with "<<num<<" threads"<<endl;
        }*/
#if !defined(TEST_MODE) && !defined(NO_NAN)
    }
    catch (exception& e)
    {
        cout <<"Exception in CATTSS::misc(), "<<e.what()<<endl;
    }
    catch (...)
    {
        cout <<"Exception in CATTSS::misc(), UNKNOWN"<<endl;
    }
#endif
}    

void* threadTask(void* cattss)
{
    //signal(SIGPIPE, SIG_IGN);
    nice(1);
    ((CATTSS*)cattss)->threadLoop();
    //pthread_exit(NULL);
}

void CATTSS::run(int numThreads)
{

    taskThreads.resize(numThreads);
    for (int i=0; i<numThreads; i++)
    {
        taskThreads[i] = new thread(threadTask,this);
        taskThreads[i]->detach();
        
    }
}
void CATTSS::stop()
{
    cont.store(0);
    for (int i=0; i<taskThreads.size(); i++)
        sem_post(&semLock);
}

//For data collection, when I deleted all my trans... :(
void CATTSS::resetAllWords_()
{
    vector<TranscribeBatch*> bs = corpus->resetAllWords_();
    vector<unsigned long> toRemoveBatches;        
    masterQueue->enqueueTranscriptionBatches(bs,&toRemoveBatches);
}

void CATTSS::threadLoop()
{
#ifdef TEST_MODE
            cout<<"threadLoop start"<<endl;
#endif
    UpdateTask* updateTask;
    while(1)
    {
        
#if !defined(TEST_MODE) && !defined(NO_NAN)
        try
        {
#endif
            updateTask = dequeue();
            if (!cont.load())
            {
                if (updateTask!=NULL)
                    delete updateTask;
                break; //END
            }
            
            if (updateTask!=NULL)
            {
                if (updateTask->type==NEW_EXEMPLAR_TASK)
                {
#ifdef TEST_MODE
                    //cout<<"START NewExemplarsTask: ["<<updateTask->id<<"]"<<endl;
                    //clock_t t;
                    //t = clock();
#endif
                    vector<pair<unsigned long,string> > toRemoveExemplars;        
                    vector<SpottingExemplar*> toSpot = masterQueue->newExemplarsFeedback(stoul(updateTask->id),
                                                                                         updateTask->labels,
                                                                                         &toRemoveExemplars);

                    spottingQueue->removeQueries(&toRemoveExemplars);
                    spottingQueue->addQueries(toSpot);
#ifdef TEST_MODE
                    //t = clock() - t;
                    //cout<<"END NewExemplarsTask: ["<<updateTask->id<<"], took: "<<((float)t)/CLOCKS_PER_SEC<<" secs"<<endl;
#endif
                }
                else if (updateTask->type==TRANSCRIPTION_TASK)
                {
#ifdef TEST_MODE
                    //cout<<"START TranscriptionTask: ["<<updateTask->id<<"]"<<endl;
                    //clock_t t;
                    //t = clock();
#endif
                    vector<pair<unsigned long, string> > toRemoveExemplars;
                    if (updateTask->resent_manual_bool)
                    {
                        vector<Spotting*> newExemplars = corpus->transcriptionFeedback(stoul(updateTask->id),updateTask->strings.front(),&toRemoveExemplars);
                        masterQueue->enqueueNewExemplars(newExemplars,&toRemoveExemplars);
                    }
                    else if (GlobalK::knowledge()->WEB_TRANS)
                    {
                        unsigned long badSpotting=0;
                        masterQueue->transcriptionFeedback(stoul(updateTask->id),updateTask->strings.front(),&toRemoveExemplars,&badSpotting);
                        if (badSpotting!=0)
                        {
                            vector<pair<unsigned long,string> > toRemoveSpottings;
                            vector<Spotting> toAdd;
                            web->badSpotting(badSpotting,&toAdd,&toRemoveSpottings);
                            vector<unsigned long> toRemoveBatches;
                            vector<TranscribeBatch*> newBatches = corpus->updateSpottings(&toAdd,&toRemoveSpottings,&toRemoveBatches,NULL,NULL);
                            masterQueue->enqueueTranscriptionBatches(newBatches,&toRemoveBatches);
                        }
                    }
                    else
                    {
                        masterQueue->transcriptionFeedback(stoul(updateTask->id),updateTask->strings.front(),&toRemoveExemplars);
                    }
                    spottingQueue->removeQueries(&toRemoveExemplars);
#ifdef TEST_MODE
                    //t = clock() - t;
                    //cout<<"END TranscriptionTask: ["<<updateTask->id<<"], took: "<<((float)t)/CLOCKS_PER_SEC<<" secs"<<endl;
#endif
                }
                else if (updateTask->type==SPOTTINGS_TASK)
                {
#ifdef TEST_MODE
                    assert(updateTask->labels.size()>0);
                    //cout<<"START SpottingsTask: ["<<updateTask->id<<"]"<<endl;
                    //clock_t t;
                    //t = clock();
#endif
                    vector<pair<unsigned long,string> > toRemoveSpottings;        
                    vector<unsigned long> toRemoveBatches;        
                    vector<Spotting>* toAdd = masterQueue->feedback(stoul(updateTask->id),updateTask->strings,updateTask->labels,updateTask->resent_manual_bool,&toRemoveSpottings);
                    if (toAdd!=NULL)
                    {
                        vector<Spotting*> newExemplars;
                        vector<pair<unsigned long,string> > toRemoveExemplars;        
                        vector<TranscribeBatch*> newBatches = corpus->updateSpottings(toAdd,&toRemoveSpottings,&toRemoveBatches,&newExemplars,&toRemoveExemplars);
                        masterQueue->enqueueTranscriptionBatches(newBatches,&toRemoveBatches);
                        if (!GlobalK::knowledge()->CLUSTER)
                        {
                            masterQueue->enqueueNewExemplars(newExemplars,&toRemoveExemplars);
                            //cout <<"Enqueued "<<newBatches.size()<<" new trans batches"<<endl;            
                            //if (toRemoveBatches.size()>0)
                            //    cout <<"Removed "<<toRemoveBatches.size()<<" trans batches"<<endl;     
                            toRemoveExemplars.insert(toRemoveExemplars.end(),toRemoveSpottings.begin(),toRemoveSpottings.end());
                            spottingQueue->removeQueries(&toRemoveExemplars);
                            spottingQueue->addQueries(*toAdd);
                        }
                        delete toAdd;
                    }
#ifdef TEST_MODE
                    //t = clock() - t;
                    //cout<<"END SpottingsTask: ["<<updateTask->id<<"], took: "<<((float)t)/CLOCKS_PER_SEC<<" secs"<<endl;
#endif
                }
                delete updateTask;
            }
            else
                break; //END
#if !defined(TEST_MODE) && !defined(NO_NAN)
        }
        catch (exception& e)
        {
            cout <<"Exception in CATTSS:threadLoop ["<<updateTask->type<<"], "<<e.what()<<endl;
        }
        catch (...)
        {
            cout <<"Exception in CATTSS::threadLoop ["<<updateTask->type<<"], UNKNOWN"<<endl;
        }
#endif
    }
}

void CATTSS::enqueue(UpdateTask* task)
{
     
#ifdef TEST_MODE
    assert(task->type != SPOTTINGS_TASK || task->labels.size()>0);
#endif
        
    taskQueueLock.lock();
    taskQueue.push_back(task);
    sem_post(&semLock);
    taskQueueLock.unlock();
}
UpdateTask* CATTSS::dequeue()
{
    sem_wait(&semLock);
    UpdateTask* ret=NULL; 
    taskQueueLock.lock();
    if (taskQueue.size()>0)
    {
        ret = taskQueue.front();
        taskQueue.pop_front();
    }
    taskQueueLock.unlock();
    return ret;
}

void CATTSS::save()
{
    if (savePrefix.length()>0)
    {
#ifdef TEST_MODE
        cout<<"START save.    "<<GlobalK::currentDateTime()<<endl;
        clock_t t;
        t = clock();
#endif

        time_t timeSec;
        time(&timeSec);

        string saveName = savePrefix+"_CATTSS.sav";
        //In the event of a crash while saveing, keep a backup of the last save
        rename( saveName.c_str() , (saveName+".bck").c_str() );

        ofstream out (saveName);

        Lexicon::instance()->save(out);
        corpus->save(out);
        masterQueue->save(out);
        spottingQueue->save(out);

        taskQueueLock.lock();
        out<<taskQueue.size()<<"\n";
        for (UpdateTask* u : taskQueue)
        {
            u->save(out);
        }
        taskQueueLock.unlock();

        out<<SpottingsBatch::getIdCounter()<<"\n";
        out<<NewExemplarsBatch::getIdCounter()<<"\n";
        out<<TranscribeBatch::getIdCounter()<<"\n";
        out<<timeSec<<"\n";
        out.close();
#ifdef TEST_MODE
        t = clock() - t;
        cout<<"END save: "<<((float)t)/CLOCKS_PER_SEC<<" secs.    "<<endl;
#endif
    }
}

void CATTSS::printFinalStats()
{
    //Summary stats for analysis
    //
    //of words in each spotting group
    //mean,std size of words in each group
    //char dists
    //of transcribed words in each spotting group
    //mean,std size of words in each group
    //char dists
    string misTrans;
    float accTrans,pWordsTrans;
    float pWords80_100, pWords60_80, pWords40_60, pWords20_40, pWords0_20, pWords0, pWordsBad;
    string misTrans_IV;
    float accTrans_IV,pWordsTrans_IV;
    float pWords80_100_IV, pWords60_80_IV, pWords40_60_IV, pWords20_40_IV, pWords0_20_IV, pWords0_IV;

    float wordsTrans80_100,  wordsTrans60_80,  wordsTrans40_60,  wordsTrans20_40,  wordsTrans0_20,  wordsTrans0, wordsTransBad;
    float meanLenPWordsTrans,  meanLenPWords80_100,  meanLenPWords60_80,  meanLenPWords40_60,  meanLenPWords20_40,  meanLenPWords0_20,  meanLenPWords0, meanLenPWordsBad;
     float stdLenPWordsTrans,  stdLenPWords80_100,  stdLenPWords60_80,  stdLenPWords40_60,  stdLenPWords20_40,  stdLenPWords0_20,  stdLenPWords0, stdLenPWordsBad;
     float meanLenWordsTrans80_100,  meanLenWordsTrans60_80,  meanLenWordsTrans40_60,  meanLenWordsTrans20_40,  meanLenWordsTrans0_20,  meanLenWordsTrans0,  meanLenWordsTransBad; 
     float stdLenWordsTrans80_100,  stdLenWordsTrans60_80,  stdLenWordsTrans40_60,  stdLenWordsTrans20_40,  stdLenWordsTrans0_20,  stdLenWordsTrans0,  stdLenWordsTransBad;
     map<char,float> charDistDone, charDistUndone;
     vector<string> badNgrams;
     map<int, float> unigramSpottingsPerLen, bigramSpottingsPerLen,trigramSpottingsPerLen;

    corpus->getStats(&accTrans,&pWordsTrans, &pWords80_100, &pWords60_80, &pWords40_60, &pWords20_40, &pWords0_20, &pWords0, &pWordsBad, &misTrans,
                &accTrans_IV,&pWordsTrans_IV, &pWords80_100_IV, &pWords60_80_IV, &pWords40_60_IV, &pWords20_40_IV, &pWords0_20_IV, &pWords0_IV, &misTrans_IV,
                &wordsTrans80_100,  &wordsTrans60_80,  &wordsTrans40_60,  &wordsTrans20_40,  &wordsTrans0_20,  &wordsTrans0 ,  &wordsTransBad,
                &meanLenPWordsTrans,  &meanLenPWords80_100,  &meanLenPWords60_80,  &meanLenPWords40_60,  &meanLenPWords20_40,  &meanLenPWords0_20,  &meanLenPWords0, &meanLenPWordsBad,
                &stdLenPWordsTrans,  &stdLenPWords80_100,  &stdLenPWords60_80,  &stdLenPWords40_60,  &stdLenPWords20_40,  &stdLenPWords0_20,  &stdLenPWords0, &stdLenPWordsBad,
                &meanLenWordsTrans80_100,  &meanLenWordsTrans60_80,  &meanLenWordsTrans40_60,  &meanLenWordsTrans20_40,  &meanLenWordsTrans0_20,  &meanLenWordsTrans0, &meanLenWordsTransBad,
                &stdLenWordsTrans80_100,  &stdLenWordsTrans60_80,  &stdLenWordsTrans40_60,  &stdLenWordsTrans20_40,  &stdLenWordsTrans0_20,  &stdLenWordsTrans0, &stdLenWordsTransBad,
                &charDistDone, &charDistUndone, 
                &badNgrams,
                &unigramSpottingsPerLen, &bigramSpottingsPerLen,&trigramSpottingsPerLen);

    for (string s : badNgrams)
        cout<<s<<endl;


    cout<<"---------Stats----------"<<endl;
    cout<<fixed<<setprecision(5)<<endl;
    cout<<"acc trans: "<<accTrans<<endl;
    cout<<"categories     trnsbd\t100-80\t80-60\t60-40\t40-20\t20-0\t0\t(bad spotting)"<<endl;
    cout<<"all, in:       "<<pWordsTrans<<'\t'<<pWords80_100<<'\t'<<pWords60_80<<'\t'<<pWords40_60<<'\t'<<pWords20_40<<'\t'<<pWords0_20<<'\t'<<pWords0<<'\t'<<pWordsBad<<endl;
    cout<<"all, mean len  "<<meanLenPWordsTrans<<'\t'<<meanLenPWords80_100<<'\t'<<meanLenPWords60_80<<'\t'<<meanLenPWords40_60<<'\t'<<meanLenPWords20_40<<'\t'<<meanLenPWords0_20<<'\t'<<meanLenPWords0<<'\t'<<meanLenPWordsBad<<endl;
    cout<<"all, std len   "<<stdLenPWordsTrans<<'\t'<<stdLenPWords80_100<<'\t'<<stdLenPWords60_80<<'\t'<<stdLenPWords40_60<<'\t'<<stdLenPWords20_40<<'\t'<<stdLenPWords0_20<<'\t'<<stdLenPWords0<<'\t'<<stdLenPWordsBad<<endl;
    cout<<"trans, in:     n/a  \t"<<wordsTrans80_100<<'\t'<<wordsTrans60_80<<'\t'<<wordsTrans40_60<<'\t'<<wordsTrans20_40<<'\t'<<wordsTrans0_20<<'\t'<<wordsTrans0<<'\t'<<wordsTransBad<<endl;
    cout<<"trans, mean len n/a \t"<<meanLenWordsTrans80_100<<'\t'<<meanLenWordsTrans60_80<<'\t'<<meanLenWordsTrans40_60<<'\t'<<meanLenWordsTrans20_40<<'\t'<<meanLenWordsTrans0_20<<'\t'<<meanLenWordsTrans0<<'\t'<<meanLenWordsTransBad<<endl;
    cout<<"trans, std len n/a  \t"<<stdLenWordsTrans80_100<<'\t'<<stdLenWordsTrans60_80<<'\t'<<stdLenWordsTrans40_60<<'\t'<<stdLenWordsTrans20_40<<'\t'<<stdLenWordsTrans0_20<<'\t'<<stdLenWordsTrans0<<'\t'<<stdLenWordsTransBad<<endl;

    cout<<".\tdone\tundone"<<endl;
    for (char a='a'; a<'z'; a++)
    {
        cout<<a<<'\t'<<charDistDone[a]<<'\t'<<charDistUndone[a]<<endl;
    }

    int maxLen = max(unigramSpottingsPerLen.rbegin()->first, max(bigramSpottingsPerLen.rbegin()->first,trigramSpottingsPerLen.rbegin()->first));
    auto iterUni = unigramSpottingsPerLen.begin();
    auto iterBi = bigramSpottingsPerLen.begin();
    auto iterTri = trigramSpottingsPerLen.begin();
    //cout<<"    ";
    //for (int i=0; i<=maxLen; i++)
    //    cout<<",\t"<<i;
    //cout<<endl;
    cout<<"Portion spotted of various word lengths:"<<endl;
    cout<<"  \tuni\tbi\ttri"<<endl;
    for (int i=0; i<=maxLen; i++)
    {
        cout<<i<<":\t";
        if (iterUni->first==i)
            cout<<(iterUni++)->second<<"\t";
        else
            cout<<"none\t";
        if (iterBi->first==i)
            cout<<(iterBi++)->second<<"\t";
        else
            cout<<"none\t";
        if (iterTri->first==i)
            cout<<(iterTri++)->second;
        else
            cout<<"none";
        cout<<endl;
    }
    /*cout<<"unigrams"<<endl;
    for (auto p : unigramSpottingsPerLen)
    {
        cout<<p.first<<"\t"<<p.second<<endl;
    }
    cout<<"bigrams"<<endl;
    for (auto p : bigramSpottingsPerLen)
    {
        cout<<p.first<<"\t"<<p.second<<endl;
    }
    cout<<"trigrams"<<endl;
    for (auto p : trigramSpottingsPerLen)
    {
        cout<<p.first<<"\t"<<p.second<<endl;
    }*/

    cout<<"Batch statistics:"<<endl;
    cout<<"ngram\tpurity\taccrcy\tsize\tnum"<<endl;
    float uniP=0;
    float uniA=0;
    float uniS=0;
    int uniC=0;
    float biP=0;
    float biA=0;
    float biS=0;
    int biC=0;
    float triP=0;
    float triA=0;
    float triS=0;
    int triC=0;
    for (auto& p : masterQueue->getBatchTracking())
    {
        string ngram = p.first;
        float meanP=0;
        float meanA=0;
        float meanS=0;
        for (auto& t : p.second)
        {
            float purity = get<0>(t);
            float accuracy = get<1>(t);
            int size = get<2>(t);
            meanP+=purity;
            meanA+=accuracy;
            meanS+=size;
        }
        meanP/=p.second.size();
        meanA/=p.second.size();
        meanS/=p.second.size();
        cout<<ngram<<"\t"<<meanP<<"\t"<<meanA<<"\t"<<meanS<<"\t"<<p.second.size()<<endl;
        if (ngram.length()==1)
        {
            uniP+=meanP;
            uniA+=meanA;
            uniS+=meanS;
            uniC++;
        }
        else if (ngram.length()==2)
        {
            biP+=meanP;
            biA+=meanA;
            biS+=meanS;
            biC++;
        }
        else if (ngram.length()==3)
        {
            triP+=meanP;
            triA+=meanA;
            triS+=meanS;
            triC++;
        }
    }
    cout<<"[1]\t"<<(uniP/uniC)<<"\t"<<(uniA/uniC)<<"\t"<<(uniS/uniC)<<"\t"<<uniC<<endl;
    cout<<"[2]\t"<<(biP/biC)<<"\t"<<(biA/biC)<<"\t"<<(biS/biC)<<"\t"<<biC<<endl;
    cout<<"[3]\t"<<(triP/triC)<<"\t"<<(triA/triC)<<"\t"<<(triS/triC)<<"\t"<<triC<<endl;
    assert(get<2>(masterQueue->getBatchTracking()["a"].front())<2*corpus->size());
}

void CATTSS::printBatchStats(string ngram, string file)
{
    int n=0;
    ofstream out;
    if (file.length()>0)
    {
        out.open(file);
        out<<"For "<<ngram<<endl;
        out<<"num\tpurity\taccrcy\tsize\trunP\trunA"<<endl;
    }
    else
    {
        cout<<"For "<<ngram<<endl;
        cout<<"num\tpurity\taccrcy\tsize\trunP\trunA"<<endl;
    }
    for (auto t : masterQueue->getBatchTracking()[ngram])
    {
        assert(get<2>(t)<2*corpus->size());
        if (file.length()>0)
            out<<n++<<"\t"<<get<0>(t)<<"\t"<<get<1>(t)<<"\t"<<get<2>(t)<<get<3>(t)<<"\t"<<get<4>(t)<<endl;
        else
            cout<<n++<<"\t"<<get<0>(t)<<"\t"<<get<1>(t)<<"\t"<<get<2>(t)<<get<3>(t)<<"\t"<<get<4>(t)<<endl;
    }
}
