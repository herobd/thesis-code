#ifndef PSUEDOWORDBATCHPOITNER
#define PSUEDOWORDBATCHPOITNER

#include "WordBackPointer.h"
using namespace std;

class PsuedoWordBackPointer : public WordBackPointer
{
    public:
        PsuedoWordBackPointer(int i, string gt, int numWords) : i(i), gt(gt), numWords(numWords) {}
        vector<Spotting*> result(string selected, unsigned long batchId, bool resend, vector< pair<unsigned long, string> >* toRemoveExemplars)
        {
            return vector<Spotting*>();
        }

        TranscribeBatch* error(unsigned long batchId, bool resend, vector<Spotting*>* newExemplars, vector< pair<unsigned long, string> >* toRemoveExemplars)
        {
            return NULL;
        }

        TranscribeBatch* removeSpotting(unsigned long sid, unsigned long batchId, bool resend, vector<Spotting*>* newExemplars, vector< pair<unsigned long, string> >* toRemoveExemplars)
        {
            return NULL;
        }

        int getSpottingIndex()
        {
            return i;
        }

        int numWords;
        string gt;

    private:
        int i;
};

#endif
