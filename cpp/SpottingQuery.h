#ifndef SPOTTING_QUERY_H
#define SPOTTING_QUERY_H

#include "opencv2/core/core.hpp"
#include "spotting.h"


using namespace std;

class SpottingQuery 
{
    public:
    SpottingQuery(const Spotting* e) : id(e->id), ngram(e->ngram), img(e->ngramImg()), type(e->type), wordId(e->wordId), x0(e->wordX0) {x1=e->wordX0+(e->brx-e->tlx);}
    SpottingQuery(const Spotting& e) : SpottingQuery(&e) {}
    SpottingQuery(string ngram) : id(-1), ngram(ngram), type(SPOTTING_TYPE_NONE), x0(-1), x1(-1) {}
    string getNgram() const {return ngram;}
    unsigned long getId() const {return id;}
    cv::Mat getImg() {return img;}
    SpottingType getType() const {return type;}
    int getWordId() const {return wordId;}
    int getX0() const {return x0;}
    int getX1() const {return x1;}

    SpottingQuery(ifstream& in)
    {
        string line;
        getline(in,ngram);
        getline(in,line);
        id = stoul(line);
        GlobalK::loadImage(img,in);
        getline(in,line);
        type = static_cast<SpottingType>(stoi(line));
        getline(in,line);
        wordId = stoi(line);
        getline(in,line);
        x0 = stoi(line);
        getline(in,line);
        x1 = stoi(line);
    }
    void save(ofstream& out)
    {
        out<<ngram<<"\n";
        out<<id<<"\n";
        GlobalK::saveImage(img,out);
        out<<type<<"\n";
        out<<wordId<<"\n";
        out<<x0<<"\n"<<x1<<endl;
    }

    private:
    string ngram;
    unsigned long id;
    cv::Mat img;
    SpottingType type;
    int wordId;
    int x0, x1;
};

#endif
