#include "TrainingInstances.h"

#include "TrainingBatchWraperSpottings.h"
#include "TrainingBatchWraperTranscription.h"

TrainingInstances::TrainingInstances()
{
    line= cv::imread("data/trainingImages/line.png", CV_LOAD_IMAGE_GRAYSCALE);
    letsgo= cv::imread("data/trainingImages/letsgo.png", CV_LOAD_IMAGE_GRAYSCALE);
    spotting_0 = Spotting(398,7,435,57,  0,&line,"he",0);
    spotting_1 = Spotting(192,7,225,55,  0,&line,"he",0);
    spotting_2 = Spotting(547,9,585,53,  0,&line,"he",0);
    spotting_3 = Spotting(130,4,188,52,  0,&line,"he",0);
    spotting_4 = Spotting(587,15,632,63,  0,&line,"or",0);
    spotting_5 = Spotting(207,5,235,54,  0,&line,"or",0);
    spotting_6 = Spotting(628,21,654,62,  0,&line,"in",0);
    spotting_7 = Spotting(301,11,325,67,  0,&line,"in",0);
    word = new Knowledge::Word(0,0,0,0,&line,NULL,NULL,NULL,0);
    spotting_8a = Spotting(507,9,543,52,  0,&line,"th",0);
    spotting_9b = Spotting(429,5,459,55,  0,&line,"ir",0);
    spotting_10b = Spotting(150,4,194,56,  0,&line,"ea",0);
    spotting_11a = Spotting(275,12,315,66,  0,&line,"oi",0);
    spotting_12a = Spotting(572,12,615,66,  0,&line,"mo",0);
    spotting_12b = Spotting(655,14,689,66,  0,&line,"ng",0);
    spotting_GO = Spotting(11,6,211,63,  0,&letsgo,"[start]",0);
}


BatchWraper* TrainingInstances::getBatch(int width, int color, string prevNgram, int trainingNum)
{
#ifndef TEST_MODE
    try
    {
#endif
        BatchWraper* ret= makeInstance(trainingNum,width,color);
        if (ret!=NULL)
            return ret;
        else
            return new BatchWraperBlank();
#ifndef TEST_MODE
    }
    catch (exception& e)
    {
        cout <<"Exception in TrainingInstances::getBatch(), "<<e.what()<<endl;
    }
    catch (...)
    {
        cout <<"Exception in TrainingInstances::getBatch(), UNKNOWN"<<endl;
    }
#endif
    return new BatchWraperBlank();
}

BatchWraper* TrainingInstances::makeInstance(int trainingNum, int width,int color)
{
    if (trainingNum==0){//spotting right he*/
            
        SpottingsBatch* batch = new SpottingsBatch("he",0);
        SpottingImage spottingImage(spotting_0,width,0,color);
        batch->push_back(spottingImage);            
        string correct="1";
        string instructions =
                "<h3>Subword spotting approval task</h3>"
                "<p>In this task you’ll look at the letter combination at the <i>bottom</i> of the screen to see if it matches the highlighted letters in the word directly above it. Swipe right if it matches, left if it doesn’t.</p>"
                //"<p>In this task you help correct the system's finding of subwords (parts of words)."
                //" If the highlighted region of the <i>bottom</i> image matches the text at the <i>bottom</i> of the screen, swipe right.</p>"
                "<p>Alternate controls are available under the <b>menu</b> if you're not using a touch device or have trouble swiping.</p>"
                "<p><i>(tap the screen to continue)</i></p>";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==1) {//spotting wrong (not he)
        SpottingsBatch* batch = new SpottingsBatch("he",0);
        SpottingImage spottingImage(spotting_1,width,0,color,"he");
        batch->push_back(spottingImage);            
        string correct="0";
        string instructions ="";
                //"<p>If the highlighted region does not match the text, swipe left.</p>";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==2) {//spotting bad BB (right) he
        SpottingsBatch* batch = new SpottingsBatch("he",0);
        SpottingImage spottingImage(spotting_3,width,0,color,"he");
        batch->push_back(spottingImage);            
        string correct="1";
        string instructions =
            "<p>If the highlighted region isn't perfectly aligned to the letter combination, but it’s close, swipe right.</p>";
                //"<p>If the highlighted region isn't perfectly aligned to the desired subword, but it's close, swipe right.</p>";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==3) {//bad BB (wrong) he
        SpottingsBatch* batch = new SpottingsBatch("he",0);
        SpottingImage spottingImage(spotting_2,width,0,color,"he");
        batch->push_back(spottingImage);            
        string correct="0";
        string instructions =
            "<p>If the highlighted region doesn’t cover much of the letter combination, swipe left.</p>";
                //"<p>If the highlighted region is significantly misaligned with the desired subword, it's wrong, so swipe left.</p>";
                //"<p>The system uses the region boundaries to count unspotted letters, but you don't need to be perfect.</p>";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==4) {//p spotting right or
        SpottingsBatch* batch = new SpottingsBatch("or",0);
        SpottingImage spottingImage(spotting_4,width,0,color);
        batch->push_back(spottingImage);            
        string correct="1";
        string instructions =
                "<p>Do these next few subwords for practice.</p>"
                "<p>The colors will change when the letter combinations you’re working on change.</p>";
                //"<p>You will see a color change when your target subword changes (the text that should be highlighted).</p>";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==5) {//p spotting right or
        SpottingsBatch* batch = new SpottingsBatch("or",0);
        SpottingImage spottingImage(spotting_5,width,0,color,"or");
        batch->push_back(spottingImage);            
        string correct="1";
        string instructions ="";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==6) {//p spotting wrong in
        SpottingsBatch* batch = new SpottingsBatch("in",0);
        SpottingImage spottingImage(spotting_6,width,0,color);
        batch->push_back(spottingImage);            
        string correct="0";
        string instructions ="";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==7) {//p spotting right in
        SpottingsBatch* batch = new SpottingsBatch("in",0);
        SpottingImage spottingImage(spotting_7,width,0,color,"in");
        batch->push_back(spottingImage);            
        string correct="1";
        string instructions ="";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,false));
    } else if (trainingNum==8) {//transcribe (tap) the
        multimap<float,string> scored;
        scored.insert( make_pair(-1,"the") );
        scored.insert( make_pair(0,"tho") );
        scored.insert( make_pair(1,"thy") );
        scored.insert( make_pair(2,"thee") );
        multimap<int,Spotting> spottings;
        spottings.insert( make_pair(507,spotting_8a) );
        TranscribeBatch* batch = new TranscribeBatch(word,scored,&line,&spottings,511,12,563,54,"the",0);
        batch->setWidth(width,0);
        string correct="the";
        string instructions =
                "<h3>Transcription task</h3>"
                //"<p>When the system has gotten enough subwords for a word, it will show you this type of task.</p>"
                //"<p>Tap on the correct transcription for the word in the yellow box.</p>";
                "<p>When the system has gotten enough letter combinations for a word, it will show you this type of task.</p>"
                "<p>Tap on the correct text of the handwritten word in the yellow box.</p>";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));
    } else if (trainingNum==9) {//transcribe (spotting error) there (err, 'ir')
        multimap<float,string> scored;
        scored.insert( make_pair(-1,"their") );
        scored.insert( make_pair(0,"theirs") );
        scored.insert( make_pair(1,"heir") );
        multimap<int,Spotting> spottings;
        spottings.insert( make_pair(398,spotting_0) );
        spottings.insert( make_pair(429,spotting_9b) );//id:10
        TranscribeBatch* batch = new TranscribeBatch(word,scored,&line,&spottings,383,5,465,59, "there",0);
        batch->setWidth(width,0);
        string correct="$REMOVE:"+to_string(spotting_9b.id)+"$";
        string instructions =
                //"<p>Sometimes a subword will have been classified incorrectly, meaning the correct transcription won't be shown. "
                //"The non-yellow highlights on the word are the subwords previously approved.</p>"
                //"<p>If the correct transcription is not shown, it may be because a subword was incorrectly labeled.</p>"
                //"<p>Tap the <b>X</b> below the '<b>ir</b>' to indicate this is an incorrect subword.</p>";
            "<p>In this example, the right word isn’t an option because one of the letter combinations was identified incorrectly. They appear just under the image of the word.</p>"
            "<p>In this case, tap the <b>X</b> below the 'ir' to tell the system it’s wrong.</p>";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));
    } else if (trainingNum==10) {//transcribe (all error) Theadore
        multimap<float,string> scored;
        scored.insert( make_pair(-1,"theater") );
        scored.insert( make_pair(0,"heater") );
        scored.insert( make_pair(1,"wheat") );
        multimap<int,Spotting> spottings;
        spottings.insert( make_pair(120,spotting_3) );
        spottings.insert( make_pair(150,spotting_10b) );
        TranscribeBatch* batch = new TranscribeBatch(word,scored,&line,&spottings,85,2,252,56, "Theadore",0);
        batch->setWidth(width,0);
        string correct="$ERROR$";
        string instructions =
                //"<p>If the subwords are all correct, but the correct transcription is still not available, just tap the <b>[None / Error]</b> button at the bottom of the transcription list.</p>";
            "<p>If the letter combinations are all correct, but the right word isn’t available, tap the box at the bottom that says <b>[None / Error]</b>.</p>";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));
    } else if (trainingNum==11) {//p transcribe (tap) join
        multimap<float,string> scored;
        scored.insert( make_pair(-1,"coined") );
        scored.insert( make_pair(-0.5,"joiner") );
        scored.insert( make_pair(0,"joined") );
        scored.insert( make_pair(1,"doings") );
        scored.insert( make_pair(2,"goings") );
        multimap<int,Spotting> spottings;
        spottings.insert( make_pair(275,spotting_11a) );//oi
        spottings.insert( make_pair(301,spotting_7) );//in
        TranscribeBatch* batch = new TranscribeBatch(word,scored,&line,&spottings,261,13,368,65, "joined",0);
        batch->setWidth(width,0);
        string correct="joined";
        string instructions =
            "<p>Here's a couple more to practice on.</p>"
            "<p>The most likely transcription (according to the system) will be on top, but you may need to scroll down to get to the right one.</p>";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));
    } else if (trainingNum==12) {//p transcribe (tap) morning (scroll down)
        multimap<float,string> scored;
        scored.insert( make_pair(-1,"moaning") );
        scored.insert( make_pair(0,"mooring") );
        scored.insert( make_pair(1,"mousing") );
        //scored.insert( make_pair(2,"mobbing") );
        //scored.insert( make_pair(3,"moating") );
        scored.insert( make_pair(4,"mocking") );
        scored.insert( make_pair(5,"mopping") );
        scored.insert( make_pair(6,"morning") );
        multimap<int,Spotting> spottings;
        spottings.insert( make_pair(572,spotting_12a) );//mo
        spottings.insert( make_pair(655,spotting_12b) );//ng
        TranscribeBatch* batch = new TranscribeBatch(word,scored,&line,&spottings,570,19,691,65, "morning",0);
        batch->setWidth(width,0);
        string correct="morning";
        string instructions ="";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));
    } else if (trainingNum==13) {//manual (no help)
        vector<string> prunedDictionary;
        multimap<int,Spotting> spottings;
        spottings.insert( make_pair(120,spotting_3) );//he
        spottings.insert( make_pair(150,spotting_10b) );//ea
        TranscribeBatch* batch = new TranscribeBatch(word,prunedDictionary,&line,&spottings,85,2,252,56, "Theadore",0);
        batch->setWidth(width,0);
        string correct="Theadore";
        string instructions =
            "<h3>Manual transcription task</h3>"
            "<p>When the system can't do any more for a word, you will have to manually type in the transcription.</p>"
            "<p><i>*sigh*</i></p>"
            "<p>You can ignore capitalization in <i>all</i> tasks.<p>";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));
    } else if (trainingNum==14) {//manual (auto compelete)
        vector<string> prunedDictionary = {
                "theft",
                "their",
                "theme",
                "thens",
                "there",
                "these",
                "theta",
                "theres",
                "thereof",
                "thereon"
        };
        multimap<int,Spotting> spottings;
        spottings.insert( make_pair(398,spotting_0) );
        TranscribeBatch* batch = new TranscribeBatch(word,prunedDictionary,&line,&spottings,383,5,465,59, "there",0);
        batch->setWidth(width,0);
        string correct="there";
        string instructions =
            "<p>It will try and help you by providing an intelligent autocomplete.<p>";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));
/*    } else if (trainingNum==15) {//?p manual (auto complete, but no help) 
        vector<string> prunedDictionary;
        multimap<int,Spotting> spottings;
        TranscribeBatch* batch = new TranscribeBatch(word,prunedDictionary,&line,&spottings,7,8,73,59, "he",0);
        batch->setWidth(width,0);
        string correct="he";
        string instructions ="";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,false));*/
    } else if (trainingNum==15) {//p spotting right or
        /*SpottingsBatch* batch = new SpottingsBatch("[start]",0);
        SpottingImage spottingImage(spotting_GO,width,0,color);
        batch->push_back(spottingImage);            
        string correct="1";
        string instructions ="";
        return (BatchWraper*) (new TrainingBatchWraperSpottings(batch,correct,instructions,true));*/
        multimap<float,string> scored;
        scored.insert( make_pair(-1,"[start]") );
        multimap<int,Spotting> spottings;
        TranscribeBatch* batch = new TranscribeBatch(word,scored,&letsgo,&spottings,11,6,211,63, "[start]",0);
        batch->setWidth(width,0);
        string correct="[start]";
        string instructions =
            "<p>Got that?</p>"
            "<p>Now you'll be doing this on some tasks from a some actual historical documents. Work quickly and accurately. If an instance is particularly challenging, feel free to use the <b>skip</b> button in the top-right. And don't forget the <b>undo</b> button in the top-left.</p>"
            "<p>Good luck!</p>";
        return (BatchWraper*) (new TrainingBatchWraperTranscription(batch,correct,instructions,true));
    }
    return NULL;
}
