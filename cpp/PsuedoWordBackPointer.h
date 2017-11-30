#ifndef PSUEDOWORDBATCHPOITNER
#define PSUEDOWORDBATCHPOITNER

#include "WordBackPointer.h"
using namespace std;

class PsuedoWordBackPointer : public WordBackPointer
{
    public:
        PsuedoWordBackPointer(int i) : i(i) {}
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

    private:
        int i;
};

#endif
