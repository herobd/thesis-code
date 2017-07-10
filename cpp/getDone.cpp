
#include "CATTSS.h"

using namespace std;





int main(int argc, char** argv)
{
    if (argc<4)
    {
        cout<<"usage: "<<argv[0]<<" saveFile outCompleted.csv outIncomplete.csv"<<endl;
        return 1;
    }
    string save=argv[1];
    string outCompleted=argv[2];
    string outImcomplete=argv[3];
    CATTSS cattss(
                        save,
                        outCompleted,
                        outImcomplete);
}
