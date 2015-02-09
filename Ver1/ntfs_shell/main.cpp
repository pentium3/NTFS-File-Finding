#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
using namespace std;
#define LL long long
#define RootFRN 1407374883553285
LL NUM,frn,prn,FL,FilePfrn;
string FindFile,Ftmp;
char rubbishch;
vector<string>  FilePath;

int main()
{
    ifstream nfile("logn.txt");
    nfile>>NUM;
    cout<<NUM<<endl;
    nfile.close();

    getline(cin,FindFile);

    ifstream fin("logf.txt");
    LL exist=0;
    string Ftmp;
    for (LL i=1; i<=NUM; i++)
    {
        fin>>FL;
        fin.get(rubbishch);
        getline(fin,Ftmp);
        fin>>frn>>prn;
        //cout<<Ftmp<<endl;
        if (Ftmp==FindFile)
        {
            //exist=true;
            exist++;
            FilePath.push_back(Ftmp);
            FilePfrn=prn;
        }
    }

    //cout<<FilePfrn<<endl;
    if (exist>0)  cout<<"Found: "<<exist<<endl;
    else    cout<<"404 Not Found";

    if (exist>0)
    {
        for (LL FindTimes=1; FindTimes<=exist; FindTimes++)
        {
            bool Found=false;
            LL ReadingCounter=0;
            fin.close();
            fin.open("logf.txt");
            while (!Found)
            {
                if (ReadingCounter==NUM)
                {
                    //fin.seekg(0,ios::beg);
                    fin.close();
                    fin.open("logf.txt");
                    ReadingCounter=0;
                }
                fin>>FL;
                fin.get(rubbishch);
                getline(fin,Ftmp);
                fin>>frn>>prn;
                ReadingCounter++;
                //cout<<ReadingCounter<<" Finding :  "<<Ftmp<<endl;
                if (frn==FilePfrn)
                {
                    FilePath.push_back(Ftmp);
                    FilePfrn=prn;
                }
                if (FilePfrn==RootFRN)
                {
                    Found=true;
                    break;
                }
            }
            for (vector<string>::iterator i=FilePath.begin(); i!=FilePath.end(); i++)
                cout<<*i<<"  ";
            cout<<"ROOT"<<endl<<endl;
        }
    }
    return 0;
}

