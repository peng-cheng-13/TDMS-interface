#include "Alluxio.h"
#include "Util.h"
#include <stdlib.h>
#include <string.h>
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <functional>
#include <thread>
#include "JNIHelper.h"
using namespace tdms;

void setarg(int num,...){
  va_list ap;
  va_start(ap,num);
  char* tmp1;
  char* tmp2;
  for(int i=0; i<num; i++){
    tmp1 = va_arg(ap, char*);
    tmp2 = va_arg(ap, char*);
    printf("varname is %s, type is %s \n",tmp1, tmp2);
  }
}

struct sdouble{
  double v1;
  double v2;
  double v3;
};

int main(int argc, char *argv[]){

        const int wsize = 786432;
	char* testfile = "/lustre/testfile-c-jni";
	TDMSClientContext acc;
	printf("Context successed \n");
	TDMSFileSystem stackFS(acc);
        jTDMSFileSystem client = &stackFS;
        printf("Init jTDMSFileSystem  successed \n");
  	// Create file output stream
	TDMSCreateFileOptions* options = TDMSCreateFileOptions::getCreateFileOptions();
	if(client->exists(testfile))
                client->deletePath(testfile, false);

	client->setDataAccessPattern("SCATTER");
        client->setDataAccessPattern("PIPELINE");
        client->setDataAccessPattern("GATHER");
        client->setDataAccessPattern("GATHER","cn17662");
        client->setDataAccessPattern("MULTICAST");
        client->setDataAccessPattern("MULTICAST", 1000);
        client->setDataAccessPattern("PIPELINE");

        //Defined a new data access pattern
        client->defineDataAccessPattern("MyPattern");
        client->setStorageTier("MyPattern", 1);
        client->setBlockSize("MyPattern", 1024*1024*128);
        client->setLayoutStrategy("MyPattern", "RoundRobinPolicy");
        client->setLoadBalanceStrategy("MyPattern", "MaxFree");

        client->setDataAccessPattern("MyPattern");
	jFileOutStream fileOutStream = client->createFile(testfile);
	sdouble *mydata = (sdouble *)malloc(wsize * sizeof(sdouble));
  	std::chrono::duration<double> duration = std::chrono::duration<double>::zero();
  	std::chrono::time_point<std::chrono::system_clock> startTime, stopTime;
  	int i,j,k=0;

	startTime = std::chrono::system_clock::now();

        for(j=0;j<100;j++){
          for(i=0;i<wsize;i++) {
               mydata[i].v1 = j*wsize + i;
               mydata[i].v2 = 1;
               mydata[i].v3 = 3;
          }
  	  fileOutStream->write(mydata, wsize * sizeof(sdouble));
	}
        printf("size of mydata is : %d \n",sizeof(mydata[1]));     
  	fileOutStream->close();
	stopTime =  std::chrono::system_clock::now();
  	duration = stopTime - startTime;
  	std::cout << "Write data to TDMS in " << duration.count() << " seconds" << std::endl;

	delete fileOutStream;
	return 0;

}

