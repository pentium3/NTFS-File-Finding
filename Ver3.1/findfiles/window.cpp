#include <QtWidgets>

#include "window.h"
#include <windows.h>

#include <string.h>
#include <iostream>
#include <Windows.h>
#include <fstream>
#include <vector>
#include <QDebug>
using namespace std;

#define LL long long
#define RootFRN 1407374883553285

static QString FName[10000000];
static long long Ffrn[10000000];
static long long Fprn[10000000];

//! [0]
Window::Window(QWidget *parent)
    : QWidget(parent)
{
    findButton = createButton(tr("&Find"), SLOT(find()));

    fileComboBox = createComboBox();
    textComboBox = createComboBox();
    directoryComboBox = createComboBox();

    fileLabel = new QLabel(tr("Named:"));
    directoryLabel = new QLabel(tr("In Drive:"));
    filesFoundLabel = new QLabel;
    filesTotalLabel = new QLabel;

    createFilesTable();

    QGridLayout *mainLayout = new QGridLayout;
    mainLayout->addWidget(fileLabel, 0, 0);
    mainLayout->addWidget(fileComboBox, 0, 1, 1, 2);
    mainLayout->addWidget(directoryLabel, 1, 0);
    mainLayout->addWidget(directoryComboBox, 1, 1,1,2);
    mainLayout->addWidget(filesTable, 3, 0, 1, 3);
    mainLayout->addWidget(filesFoundLabel, 4, 0, 1, 2);
    mainLayout->addWidget(filesTotalLabel, 4, 1, 1, 2);
    mainLayout->addWidget(findButton, 4, 2);
    setLayout(mainLayout);

    setWindowTitle(tr("Find Files"));
    resize(700, 600);
}

static void updateComboBox(QComboBox *comboBox)
{
    if (comboBox->findText(comboBox->currentText()) == -1)
        comboBox->addItem(comboBox->currentText());
}


void Window::USNFindingFile()
{
    sDriveLetter=DriveLetter.toStdString();
    sfileName=fileName.toStdString();

    char volName[10];
    string sv=sDriveLetter+":\\";
    strcpy( volName, sv.c_str() );

    HANDLE hVol;
    USN_JOURNAL_DATA UsnInfo;

    #define BUF_LEN 4096

    long counter = 0;		//the number of files

    bool status;
    bool isNTFS = false;
    bool getHandleSuccess = false;
    bool initUsnJournalSuccess = false;

    char sysNameBuf[MAX_PATH] = {0};
    status = GetVolumeInformationA(volName,
                                   NULL,
                                   0,
                                   NULL,
                                   NULL,
                                   NULL,
                                   sysNameBuf,
                                   MAX_PATH);

    if(0!=status){
        printf("fs: %s\n", sysNameBuf);

        if(0==strcmp(sysNameBuf, "NTFS")){
            isNTFS = true;
        }else{
            printf("not a NTFS drive\n");
        }

    }


    if(isNTFS){
        char fileName[MAX_PATH];
        fileName[0] = '\0';

        strcpy(fileName, "\\\\.\\");
        strcat(fileName, volName);
        string fileNameStr = (string)fileName;
        fileNameStr.erase(fileNameStr.find_last_of(":")+1);

        printf("Address: %s\n", fileNameStr.data());


        hVol = CreateFileA(fileNameStr.data(),
                           GENERIC_READ | GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_READONLY,
                           NULL);

        if(INVALID_HANDLE_VALUE!=hVol){
            getHandleSuccess = true;
        }else{
            printf("failed —— handle:%x error:%d\n", hVol, GetLastError());
        }

    }

    if(getHandleSuccess){
        DWORD br;
        CREATE_USN_JOURNAL_DATA cujd;
        cujd.MaximumSize = 0;
        cujd.AllocationDelta = 0;
        status = DeviceIoControl(hVol,
                                 FSCTL_CREATE_USN_JOURNAL,
                                 &cujd,
                                 sizeof(cujd),
                                 NULL,
                                 0,
                                 &br,
                                 NULL);

        if(0!=status){
            initUsnJournalSuccess = true;
        }else{
            printf("failed —— status:%x error:%d\n", status, GetLastError());
        }

    }

    if(initUsnJournalSuccess){

        bool getBasicInfoSuccess = false;

        DWORD br;
        status = DeviceIoControl(hVol,
                                 FSCTL_QUERY_USN_JOURNAL,
                                 NULL,
                                 0,
                                 &UsnInfo,
                                 sizeof(USN_JOURNAL_DATA),
                                 &br,
                                 NULL);

        if(0!=status){
            getBasicInfoSuccess = true;
        }else{
            printf("failed —— status:%x error:%d\n", status, GetLastError());
        }

        if(getBasicInfoSuccess){

            printf("UsnJournalID: %xI64\n", UsnInfo.UsnJournalID);
            printf("lowUsn: %xI64\n", UsnInfo.FirstUsn);
            printf("highUsn: %xI64\n", UsnInfo.NextUsn);


            // from MSDN
            // On the first call, set the starting point, the StartFileReferenceNumber member of the MFT_ENUM_DATA structure, to (DWORDLONG)0.
            // Each call to FSCTL_ENUM_USN_DATA retrieves the starting point for the subsequent call as the first entry in the output buffer.
            MFT_ENUM_DATA med;
            med.StartFileReferenceNumber = 0;
            med.LowUsn = 0;
            med.HighUsn = UsnInfo.NextUsn;

            CHAR buffer[BUF_LEN];
            DWORD usnDataSize;
            PUSN_RECORD UsnRecord;

            while(0!=DeviceIoControl(hVol,
                                     FSCTL_ENUM_USN_DATA,
                                     &med,
                                     sizeof(med),
                                     buffer,
                                     BUF_LEN,
                                     &usnDataSize,
                                     NULL))
            {

                DWORD dwRetBytes = usnDataSize - sizeof(USN);
                UsnRecord = (PUSN_RECORD)(((PCHAR)buffer)+sizeof(USN));

                while(dwRetBytes>0){


                    const int strLen = UsnRecord->FileNameLength;
                    char fileName[MAX_PATH] = {0};
                    WideCharToMultiByte(CP_OEMCP,NULL,UsnRecord->FileName,strLen/2,fileName,strLen,NULL,FALSE);

                    counter++;
                    FName[counter]=QString::fromLocal8Bit(fileName);
                    //qDebug()<<FName[counter]<<endl;
                    Ffrn[counter]=UsnRecord->FileReferenceNumber;
                    Fprn[counter]=UsnRecord->ParentFileReferenceNumber;


                    DWORD recordLen = UsnRecord->RecordLength;
                    dwRetBytes -= recordLen;
                    UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord)+recordLen);
                }
                med.StartFileReferenceNumber = *(USN *)&buffer;

            }

            printf("%d\n", counter);
            totalFileNum=counter;
            qDebug() <<fileName<<endl;
        }


        DELETE_USN_JOURNAL_DATA dujd;
        dujd.UsnJournalID = UsnInfo.UsnJournalID;
        dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;

        status = DeviceIoControl(hVol,
                                 FSCTL_DELETE_USN_JOURNAL,
                                 &dujd,
                                 sizeof(dujd),
                                 NULL,
                                 0,
                                 &br,
                                 NULL);

        if(0!=status){
            printf("deleted\n");
        }else{
            printf("failed —— status:%x error:%d\n", status, GetLastError());
        }

    }

    if(getHandleSuccess){
        CloseHandle(hVol);
    }
}

void Window::FindFilesPath()
{
    LL NUM,frn,prn,FL;
    LL FilesPfrn[100000];
    //string FindFile,Ftmp,ROOT;
    QString FindFile,Ftmp,ROOT;

    NUM=totalFileNum;
    FindFile=fileName;     //QString    fileName
    ROOT=DriveLetter;

    qDebug()<<NUM<<"  |  "<<ROOT<<"  |  "<<FindFile<<endl;

    QString RST;
    LL exist=0;

    for (LL i=1; i<=NUM; i++)
    {
        Ftmp=FName[i];
        frn=Ffrn[i];
        prn=Fprn[i];

        if (Ftmp==FindFile)
        //if (Ftmp.indexOf())
        {
            exist++;
            FilesPfrn[exist]=prn;
        }
    }

    //cout<<FilePfrn<<endl;
    //if (exist>0)  cout<<"Found: "<<exist<<endl;   else    cout<<"404 Not Found";
    cout<<"exist:  "<<exist<<endl;

    if (exist>0)
    {
        for (LL FindTimes=1; FindTimes<=exist; FindTimes++)
        {
            LL FilePfrn;
            //vector<string>  FilePath;
            vector<QString> FilePath;
            FilePfrn=FilesPfrn[FindTimes];
            //cout<<"Finding:"<<FindTimes<<endl;
            //cout<<FilePfrn<<endl;

            bool Found=false;
            LL ReadingCounter=0;
            while (!Found)
            {
                if (ReadingCounter==NUM)
                {
                    ReadingCounter=0;
                }
                ReadingCounter++;
                Ftmp=FName[ReadingCounter];
                frn=Ffrn[ReadingCounter];
                prn=Fprn[ReadingCounter];

                //cout<<ReadingCounter<<" Finding :  "<<Ftmp<<endl;
                if (frn==FilePfrn)
                {
                    //cout<<FilePfrn<<endl;
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

            QString pathtmp=ROOT+":/";
            for (vector<QString>::iterator i=FilePath.end()-1; i!=FilePath.begin()-1; i--)
                pathtmp=pathtmp+*i+"/";
            //cout<<pathtmp<<endl;
            //path[FindTimes-1]=QString::fromStdString(pathtmp);
            path[FindTimes-1]=pathtmp;
        }
    }
    FileNum=exist;
}


void Window::find()
{
    filesTable->setRowCount(0);

    fileName = fileComboBox->currentText();
    DriveLetter=directoryComboBox->currentText();

    USNFindingFile();
    FindFilesPath();
    qDebug()<<FileNum<<"   "<<fileName<<endl;

    updateComboBox(fileComboBox);
    updateComboBox(textComboBox);
    updateComboBox(directoryComboBox);

    for (int i=1;i<=FileNum;i++)
        files.append(fileName);

    showFiles(files);
}


void Window::showFiles(const QStringList &files)
{
    for (int i = 0; i < FileNum; ++i)
    {
        currentFileDir=QDir(path[i]);
        QFile file(currentFileDir.absoluteFilePath(files[i]));
        qint64 size = QFileInfo(file).size();

        QTableWidgetItem *fileNameItem = new QTableWidgetItem(files[i]);
        fileNameItem->setFlags(fileNameItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *pathItem = new QTableWidgetItem(path[i]);
        pathItem->setFlags(pathItem->flags() ^ Qt::ItemIsEditable);
        QTableWidgetItem *sizeItem = new QTableWidgetItem(tr("%1 KB")
                                             .arg(int((size + 1023) / 1024)));
        sizeItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        sizeItem->setFlags(sizeItem->flags() ^ Qt::ItemIsEditable);

        int row = filesTable->rowCount();
        filesTable->insertRow(row);
        filesTable->setItem(row, 0, fileNameItem);
        filesTable->setItem(row, 1, pathItem);
        filesTable->setItem(row, 2, sizeItem);
    }
    filesFoundLabel->setText(tr("%1 file(s) found").arg(files.size()));
    filesFoundLabel->setWordWrap(true);
    filesTotalLabel->setText(tr("Total file(s): %1").arg(totalFileNum));
    filesTotalLabel->setWordWrap(true);
}


QPushButton *Window::createButton(const QString &text, const char *member)
{
    QPushButton *button = new QPushButton(text);
    connect(button, SIGNAL(clicked()), this, member);
    return button;
}

QComboBox *Window::createComboBox(const QString &text)
{
    QComboBox *comboBox = new QComboBox;
    comboBox->setEditable(true);
    comboBox->addItem(text);
    comboBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    return comboBox;
}

void Window::createFilesTable()
{
    filesTable = new QTableWidget(0, 3);
    filesTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    QStringList labels;
    labels << tr("Filename") << tr("Path") << tr("Size");
    filesTable->setHorizontalHeaderLabels(labels);
    filesTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    filesTable->verticalHeader()->hide();
    filesTable->setShowGrid(true);
}
