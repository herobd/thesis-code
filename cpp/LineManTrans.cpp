
LineManTrans::LineManTrans(
                string pageImageDir, 
                string segmentationFile, 
                string savePrefix,
                int numTaskThreads,
                int contextPad
                ) : savePrefix(savePrefix)
{
    ifstream in (savePrefix+"_LineManTrans.sav");
    if (in.good())
    {
        cout<<"Load file found."<<endl;
        Lexicon::instance()->load(in);
        corpus = new Knowledge::Corpus(in);
        corpus->loadSpotter(spottingModelPrefix);
        lineQueue = new LineQueue(in,corpus);

    }
    else
    {
        Lexicon::instance()->readIn(lexiconFile);
        corpus = new Knowledge::Corpus(contextPad, ngramWWFile);
        corpus->addWordSegmentaionAndGT(pageImageDir, segmentationFile);
        lineQueue = new lineQueue(contextPad,corpus);
    }
}

BatchWraper* LineManTrans::getBatch()
{
#if !defined(TEST_MODE) && !defined(NO_NAN)
    try
    {
#endif

        BatchWraper* ret= lineQueue->getBatch();
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
