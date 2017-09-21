
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
    while (1)
    {
        string line;
        cout <<"Enter page number or 'q': ";
        getline(cin, line);
        if (line[0] == 'q')
            break;
        cattss.misc("showInteractive:"+line);
    }


}
