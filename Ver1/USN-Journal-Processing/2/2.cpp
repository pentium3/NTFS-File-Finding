#include <stdafx.h>
#include <string.h>
#include <iostream>
#include <Windows.h>
#include <fstream>
using namespace std;

char* volName = "c:\\"; // ����������
char* FindFileName="233.txt";	
	
HANDLE hVol; // ���ڴ��������̾��

USN_JOURNAL_DATA UsnInfo; // ���ڴ���USN��־�Ļ�����Ϣ

#define BUF_LEN 4096

ofstream fout("c:\\logf.txt"); // ���������ݼ�¼���ı�������鿴
//ofstream tout("c:\\logt.txt");	//debugging file
//ifstream  fin("c:\\logf.txt");
ofstream nout("c:\\logn.txt"); // the number of files

long counter = 0;		//the number of files

int main(){

	bool status;
	bool isNTFS = false;
	bool getHandleSuccess = false;
	bool initUsnJournalSuccess = false;

	/**
	 * step 01. �ж��������Ƿ�NTFS��ʽ
	 * msdn:http://msdn.microsoft.com/en-us/library/aa364993%28VS.85%29.aspx
	 */
	char sysNameBuf[MAX_PATH] = {0};
	status = GetVolumeInformationA(volName,
		                           NULL, // �����������壬�������ǲ���Ҫ
								   0,
								   NULL,
								   NULL,
								   NULL,
								   sysNameBuf, // �����̵�ϵͳ����FAT/NTFS��
								   MAX_PATH);

	if(0!=status){
		printf("�ļ�ϵͳ��: %s\n", sysNameBuf);

		// �Ƚ��ַ���
		if(0==strcmp(sysNameBuf, "NTFS")){
			isNTFS = true;
		}else{
			printf("�������̷�NTFS��ʽ\n");
		}

	}

	// ֻ��NTFS����USN�����ܽ��в���
	if(isNTFS){
		/**
	     * step 02. ��ȡ�����̾��
	     * msdn:http://msdn.microsoft.com/en-us/library/aa363858%28VS.85%29.aspx
	     */
		char fileName[MAX_PATH];
		fileName[0] = '\0';

		// ������ļ�������Ϊ\\.\C:����ʽ
		strcpy(fileName, "\\\\.\\");
		strcat(fileName, volName);
		// Ϊ�˷������������תΪstring����ȥβ
		string fileNameStr = (string)fileName;
		fileNameStr.erase(fileNameStr.find_last_of(":")+1);

		printf("�����̵�ַ: %s\n", fileNameStr.data());

		// ���øú�����Ҫ����ԱȨ��
		hVol = CreateFileA(fileNameStr.data(),
			               GENERIC_READ | GENERIC_WRITE, // ����Ϊ0
						   FILE_SHARE_READ | FILE_SHARE_WRITE, // ���������FILE_SHARE_WRITE
						   NULL, // ���ﲻ��Ҫ
						   OPEN_EXISTING, // �������OPEN_EXISTING, CREATE_ALWAYS���ܻᵼ�´���
						   FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL���ܻᵼ�´���
						   NULL); // ���ﲻ��Ҫ

		if(INVALID_HANDLE_VALUE!=hVol){
			getHandleSuccess = true;
		}else{
			printf("��ȡ�����̾��ʧ�� ���� handle:%x error:%d\n", hVol, GetLastError());
		}

	}

	if(getHandleSuccess){

		/**
	     * step 03. ��ʼ��USN��־�ļ�
	     * msdn:http://msdn.microsoft.com/en-us/library/aa364558%28v=VS.85%29.aspx
	     */
		DWORD br;
		CREATE_USN_JOURNAL_DATA cujd;
		cujd.MaximumSize = 0; // 0��ʾʹ��Ĭ��ֵ
		cujd.AllocationDelta = 0; // 0��ʾʹ��Ĭ��ֵ
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
			printf("��ʼ��USN��־�ļ�ʧ�� ���� status:%x error:%d\n", status, GetLastError());
		}

	}

	if(initUsnJournalSuccess){
		
		bool getBasicInfoSuccess = false;

		/**
	     * step 04. ��ȡUSN��־������Ϣ(���ں�������)
	     * msdn:http://msdn.microsoft.com/en-us/library/aa364583%28v=VS.85%29.aspx
	     */
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
			printf("��ȡUSN��־������Ϣʧ�� ���� status:%x error:%d\n", status, GetLastError());
		}

		if(getBasicInfoSuccess){

			printf("UsnJournalID: %xI64\n", UsnInfo.UsnJournalID);
			printf("lowUsn: %xI64\n", UsnInfo.FirstUsn);
			printf("highUsn: %xI64\n", UsnInfo.NextUsn);

			/**
	         * step 05. ö��USN��־�ļ��е����м�¼
	         * msdn:http://msdn.microsoft.com/en-us/library/aa364563%28v=VS.85%29.aspx
	         */
			
			// from MSDN
			// On the first call, set the starting point, the StartFileReferenceNumber member of the MFT_ENUM_DATA structure, to (DWORDLONG)0. 
			// Each call to FSCTL_ENUM_USN_DATA retrieves the starting point for the subsequent call as the first entry in the output buffer.
			MFT_ENUM_DATA med;
			med.StartFileReferenceNumber = 0;
			med.LowUsn = 0;//UsnInfo.FirstUsn; ���ﾭ���Է��֣������FirstUsn��ʱ����ȷ�����»�ȡ�������������ݣ�����ֱ��д0��.
			med.HighUsn = UsnInfo.NextUsn;

			CHAR buffer[BUF_LEN]; // ���ڴ����¼�Ļ���,�����㹻�ش�
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

				// �ҵ���һ��USN��¼
				// from MSDN(http://msdn.microsoft.com/en-us/library/aa365736%28v=VS.85%29.aspx):
				// return a USN followed by zero or more change journal records, each in a USN_RECORD structure. 
				UsnRecord = (PUSN_RECORD)(((PCHAR)buffer)+sizeof(USN));

		//		printf(" ********************************** \n");
				while(dwRetBytes>0){

					// ��ӡ��ȡ������Ϣ
					const int strLen = UsnRecord->FileNameLength;
					char fileName[MAX_PATH] = {0};
					WideCharToMultiByte(CP_OEMCP,NULL,UsnRecord->FileName,strLen/2,fileName,strLen,NULL,FALSE);
		//			printf("FileName: %s\n", fileName);
					// ��������file reference number����������ȡ�ļ���·����Ϣ
		//			printf("FileReferenceNumber: %xI64\n", UsnRecord->FileReferenceNumber);
		//			printf("ParentFileReferenceNumber: %xI64\n", UsnRecord->ParentFileReferenceNumber);
		//			printf("\n");
					
					fout<<strLen/2<<endl;
					fout << fileName << endl;
					fout << UsnRecord->FileReferenceNumber << endl;
					fout << UsnRecord->ParentFileReferenceNumber << endl;
					//fout << endl;
					counter++;

					// ��ȡ��һ����¼
					DWORD recordLen = UsnRecord->RecordLength;
					dwRetBytes -= recordLen;
					UsnRecord = (PUSN_RECORD)(((PCHAR)UsnRecord)+recordLen);
				}

				//��ȡ��һҳ���ݣ�MTF����Ƿֶ�ҳ������İɣ�
				// from MSDN(http://msdn.microsoft.com/en-us/library/aa365736%28v=VS.85%29.aspx):
		        // The USN returned as the first item in the output buffer is the USN of the next record number to be retrieved. 
		        // Use this value to continue reading records from the end boundary forward.
				med.StartFileReferenceNumber = *(USN *)&buffer;

			}

			printf("��%d���ļ�\n", counter);

			fout << flush;
			fout.close();
			nout << counter << endl;
			nout << flush;
			nout.close();

		}

		/**
	     * step 06. ɾ��USN��־�ļ�(��ȻҲ���Բ�ɾ��)
	     * msdn:http://msdn.microsoft.com/en-us/library/aa364561%28v=VS.85%29.aspx
	     */
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
			printf("�ɹ�ɾ��USN��־�ļ�!\n");
		}else{
			printf("ɾ��USN��־�ļ�ʧ�� ���� status:%x error:%d\n", status, GetLastError());
		}

	}

	// ����ͷ�һЩ��Դ
	if(getHandleSuccess){
		CloseHandle(hVol);
	}
	
/*
	bool exist=false;
	long L=0,Ffrn=0,Fpfn=0;
	char m[1000];
	//string s;
	for (int i=1;i<=counter;i++)
	{
		fin>>L;
		fin>>m;
		//fin.getline(m,L);
		//getline(fin,s);
		fin>>Ffrn>>Fpfn;
		cout<<L<<" "<<Ffrn<<" "<<Fpfn<<endl;
		if (strcmp(m,FindFileName)==0)		exist=true;
	}
	if (exist)	cout<<"File Found"<<endl;	else cout<<"File Not Found"<<endl;
*/

	// �����̨����һ������
	MessageBox(0, L"��ȷ���˳�", L"����", MB_OK);

	return 0;
}