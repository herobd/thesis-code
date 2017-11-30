
#include "CATTSS.h"

using namespace std;





int main(int argc, char** argv)
{
    if (argc!=2)
    {
        cout<<"usage: "<<argv[0]<<" saveFile"<<endl;
        return 1;
    }
    string save=argv[1];
    CATTSS cattss(save);
    cattss.printFinalStats();
    while (1)
    {
        string line;
        cout <<"Enter page number or ngram or 'quit'/'!': ";
        getline(cin, line);
        if (line[0] == '!' || string(line).compare("quit")==0)
            break;
        if (line[0]>='0' && line[0]<='9')
            cattss.misc("showInteractive:"+line);
        else
        {
            cout<<"File?(blank to std out): ";
            string file;
            getline(cin,file);
            cattss.printBatchStats(line,file);
        }
    }


}
