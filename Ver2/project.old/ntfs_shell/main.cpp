#include <iostream>
#include <fstream>
#include <cstring>
#include <vector>
#include <windows.h>
using namespace std;
#define LL long long
#define RootFRN 1407374883553285
LL NUM,frn,prn,FL,FilePfrn;
LL FilesPfrn[10000];
string FindFile,Ftmp;
char rubbishch;

int main()
{
    ofstream ofile("c://logp.txt");
    //string ROOT(argv[1]);
    //string FindFile(argv[2]);

    string ROOT,FindFile;

    ifstream nfile("c://logn.txt");
    nfile>>NUM;
    nfile>>ROOT;
    nfile.get(rubbishch);
    getline(nfile,FindFile);
    //cout<<ROOT<<endl;
    cout<<FindFile<<endl;
    //cout<<NUM<<endl;
    nfile.close();




    ifstream fin("c://logf.txt");
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
            exist++;
            FilesPfrn[exist]=prn;
        }
    }

    //cout<<FilePfrn<<endl;
    //if (exist>0)  cout<<"Found: "<<exist<<endl;   else    cout<<"404 Not Found";
    ofile<<exist<<endl;     //cout<<exist<<endl;

    if (exist>0)
    {
        for (LL FindTimes=1; FindTimes<=exist; FindTimes++)
        {
            vector<string>  FilePath;
            FilePfrn=FilesPfrn[FindTimes];

            bool Found=false;
            LL ReadingCounter=0;
            fin.close();
            fin.open("c://logf.txt");
            while (!Found)
            {
                if (ReadingCounter==NUM)
                {
                    //fin.seekg(0,ios::beg);
                    fin.close();
                    fin.open("c://logf.txt");
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
            LL TMP=FilePath.size();

            ofile<<ROOT<<":/";          //cout<<ROOT<<":/";
            //for (vector<string>::iterator i=FilePath.begin(); i!=FilePath.end(); i++)
            for (vector<string>::iterator i=FilePath.end()-1; i!=FilePath.begin()-1; i--)
                ofile<<*i<<"/";         //cout<<*i<<"/";
            ofile<<endl;                //cout<<endl;
        }
    }
    return 0;
}

