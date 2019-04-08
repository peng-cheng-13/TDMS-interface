/**
 *
 *  @author         Peng Cheng <peng_cheng_13@163.com>
 *  @organization   NSCC-GZ
 *
 *  @author        Adam Storm <ajstorm@ca.ibm.com>
 *  @organization  IBM
 * 
 * TDMS C/C++ APIs on top of Alluxio
 *
 */

#include "Alluxio.h"
#include "Util.h"

#include <string>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <chrono>

using namespace tdms;
using namespace tdms::jni;

/**
   Constructor for TDMSClientContext

   This will attach to the JVM thread created for this process.  Prior to
   calling this AlluxioClientContext::connect() must be called to cause the JVM
   thread to connect to the master node.
*/
TDMSClientContext::TDMSClientContext()
    : m_mustDetachInDtor(false), m_mustDeleteLocalRef(false) {
  attach();
}

/**
   Destructor.
*/
TDMSClientContext::~TDMSClientContext() {
    if (m_mustDeleteLocalRef) {
        m_env.deleteLocalRef(m_baseFileSystem);
        m_mustDeleteLocalRef = false;
    }
    if (m_mustDetachInDtor) {
        m_env.DetachCurrentThread();
        m_mustDetachInDtor = false;
    }
}


/**
   Attach the current thread to a running JVM.

   This can only be used if the process had previously called connect() in
   same/other thread.
*/
void TDMSClientContext::attach() {
  // We only attach if not already done so.  This gives us nested style of
  // semantics if we run a bunch of APIs that continually need to attach.  A
  // detach is only done once we tear down the client context of when the
  // thread first attached.
  if (m_env.GetIsThreadDetached() == true) {
    m_env.AttachCurrentThread();
    m_mustDetachInDtor = true;
  }

  // Get a pointer to the alluxio java object for use in subsequent Java calls.
  //jvalue ret;
  //m_env.callStaticMethod(&ret, "alluxio/client/file/BaseFileSystem", "get",
  //                       "()Lalluxio/client/file/BaseFileSystem;");
  m_baseFileSystem = m_env.getFs();
  m_mustDeleteLocalRef = true;
}


//////////////////////////////////////////
//// TDMSURI
////////////////////////////////////////////

jTDMSURI TDMSURI::newURI(const char *pathStr)
{
  Env env;
  jobject retObj;
  jstring jPathStr;
  jPathStr = env.newStringUTF(pathStr, "path");
  retObj = env.newObject("alluxio/AlluxioURI", "(Ljava/lang/String;)V", jPathStr); 
  env->DeleteLocalRef(jPathStr);
  return new TDMSURI(env, retObj);
}




//////////////////////////////////////////
// TDMSFileSystem
//////////////////////////////////////////

/**
  Constructor

  @param[in] clientContext Context that has either connected to alluxio, or
             attached to the running JVM that has connected.
*/

TDMSFileSystem::TDMSFileSystem(TDMSClientContext &clientContext)
    : mClient(clientContext) {}

bool TDMSFileSystem::exists(const char *path) {
  jvalue ret;
  //jobject urlobj = TDMSURI::newURI(path);
  //std::unique_ptr<TDMSURI> uri(TDMSURI::newURI(path));
  jTDMSURI uri = TDMSURI::newURI(path);
  //printf("URL structure init succeed \n");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "exists",
                              "(Lalluxio/AlluxioURI;)Z", uri->getJObj());
  return ret.z;
}

void TDMSFileSystem::createDirectory(const char *path) {
  jvalue ret;
  //std::unique_ptr<AlluxioURI> uri(AlluxioURI::newURI(path));
  jTDMSURI uri = TDMSURI::newURI(path);
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "createDirectory",
                              "(Lalluxio/AlluxioURI;)V", uri->getJObj());
  return;
}

/*  Delete a file or directory in Alluxio

  @param[in] path Path name to delete.  Can be a file or a directory.
  @param[in] recursive If the path to be deleted is a directory, the flag
             specifies whether the directory content should be recursively
             deleted as well

*/
void TDMSFileSystem::deletePath(const char *path, bool recursive) {
  jvalue ret;
  jTDMSURI uri = TDMSURI::newURI(path);

  if (!recursive) {
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "delete",
                                "(Lalluxio/AlluxioURI;)V", uri->getJObj());
  } else {
    jvalue deleteOptionsDefaults;
    jvalue deleteOptionsSetRecursive;

    mClient.getEnv().callStaticMethod(
        &deleteOptionsDefaults, "alluxio/client/file/options/DeleteOptions",
        "defaults", "()Lalluxio/client/file/options/DeleteOptions;");

    mClient.getEnv().callMethod(
        &deleteOptionsSetRecursive, (jobject)deleteOptionsDefaults.l,
        "setRecursive", "(Z)Lalluxio/client/file/options/DeleteOptions;",
        (jboolean)recursive);

    mClient.getEnv().callMethod(
        &ret, mClient.getJObj(), "delete",
        "(Lalluxio/AlluxioURI;Lalluxio/client/file/options/DeleteOptions;)V",
        uri->getJObj(), (jobject)deleteOptionsSetRecursive.l);
  }
}

void TDMSFileSystem::deleteAlluxioPath(const char *path, bool alluxioOnly) {
  jvalue ret;
  jTDMSURI uri = TDMSURI::newURI(path);

  if (!alluxioOnly) {
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "delete",
                                "(Lalluxio/AlluxioURI;)V", uri->getJObj());
  } else {
    jvalue deleteOptionsDefaults;
    jvalue deleteOptionsSetRecursive;

    mClient.getEnv().callStaticMethod(
        &deleteOptionsDefaults, "alluxio/client/file/options/DeleteOptions",
        "defaults", "()Lalluxio/client/file/options/DeleteOptions;");

    mClient.getEnv().callMethod(
        &deleteOptionsSetRecursive, (jobject)deleteOptionsDefaults.l,
        "setAlluxioOnly", "(Z)Lalluxio/client/file/options/DeleteOptions;",
        (jboolean)alluxioOnly);

    mClient.getEnv().callMethod(
        &ret, mClient.getJObj(), "delete",
        "(Lalluxio/AlluxioURI;Lalluxio/client/file/options/DeleteOptions;)V",
        uri->getJObj(), (jobject)deleteOptionsSetRecursive.l);
  }
}

void TDMSFileSystem::loadMetadata(const char *path) {
  jvalue ret;
  jTDMSURI uri = TDMSURI::newURI(path);
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "loadMetadata",
                                "(Lalluxio/AlluxioURI;)V", uri->getJObj());
}

jFileInStream TDMSFileSystem::openFile(const char *path,
                                          TDMSOpenFileOptions *options) {
  jvalue ret;

  jTDMSURI uri = TDMSURI::newURI(path);

  jFileInStream fileInStream = NULL;

  if (options == NULL) {
    mClient.getEnv().callMethod(
        &ret, mClient.getJObj(), "openFile",
        "(Lalluxio/AlluxioURI;)Lalluxio/client/file/FileInStream;",
        uri->getJObj());
  } else {
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "openFile",
                                "(Lalluxio/AlluxioURI;Lalluxio/client/file/"
                                "options/OpenFileOptions;)Lalluxio/client/file/"
                                "FileInStream;",
                                uri->getJObj(), options->getOptions());
  }

  // FIXME: Change to shared_ptr?
  return (new FileInStream(mClient.getEnv(), ret.l));
}

// Query file
jFileInStream TDMSFileSystem::queryFile(const char *path, const char *varname, double max, double min, bool augmented) {
  jvalue ret;
  jstring jvarname;
  jvarname = mClient.getEnv().newStringUTF(varname, varname);
  jTDMSURI uri = TDMSURI::newURI(path);
  printf("Call JNI query\n");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "queryFile",
                              "(Lalluxio/AlluxioURI;Ljava/lang/String;DDZ)Lalluxio/client/file/FileInStream;",
                              uri->getJObj(), jvarname, (jdouble)max, (jdouble)min, (jboolean)augmented);
  printf("JNI Done\n");
  return (new FileInStream(mClient.getEnv(), ret.l));
}

// Select Files
//char **TDMSFileSystem::selectFiles(const char* keylist[], const char* valuelist[], const char* typelist[], int argsize, int mpirsize, int mpirank) {
int TDMSFileSystem::selectFiles(char **pathlist, const char* keylist[], const char* valuelist[], const char* typelist[], int argsize, int mpirsize, int mpirank) {
  Env env;
  jstring jKeyStr;
  jstring jValueStr;
  jstring jTypeStr;
  //Init ArrayList
  //printf("TDMS: Ready to select files\n");
  jobject keyset = env.newObject("Ljava/util/ArrayList;", "()V");
  jobject valueset = env.newObject("Ljava/util/ArrayList;", "()V");
  jobject typeset = env.newObject("Ljava/util/ArrayList;", "()V");
  //printf("TDMS: Get ArrayList object\n");
  jvalue ret;
  for (int i = 0; i < argsize; i++) {
    //printf("TDMS: char to String\n");
    jKeyStr = env.newStringUTF(keylist[i], "err");
    jValueStr = env.newStringUTF(valuelist[i], "err");
    jTypeStr = env.newStringUTF(typelist[i], "err");
    //printf("TDMS: char to String Done\n");
    env.callMethod(&ret, keyset, "add", "(Ljava/lang/Object;)Z", jKeyStr);
    env.callMethod(&ret, valueset, "add", "(Ljava/lang/Object;)Z", jValueStr);
    env.callMethod(&ret, typeset, "add", "(Ljava/lang/Object;)Z", jTypeStr);
  }
  //printf("TDMS: Init query data structure \n");
  //Get path set
  env.callMethod(&ret, mClient.getJObj(), "mpiSetect",
      "(Ljava/util/List;Ljava/util/List;Ljava/util/List;II)Ljava/util/Set;", keyset, valueset, typeset, (jint) mpirsize, (jint) mpirank);
  jobject jpathset = ret.l;
  if (jpathset == NULL) {
    exit(0);
  }
  jvalue ret2;
  env.callMethod(&ret2, jpathset, "size", "()I"); 
  int numofPaths = ret2.i;
  //pathlist = (char **)malloc(numofPaths *  sizeof(char *));
  //printf("TDMS: Get path set \n");
  //Get Iterator
  mClient.getEnv().callMethod(&ret2, jpathset, "iterator", "()Ljava/util/Iterator;");
  jobject myiterator = ret2.l;
  //printf("TDMS: Get Iterator \n");
  //Retrieve paths
  jvalue ret3;
  for (int i = 0; i < numofPaths; i++) {
     env.callMethod(&ret3, myiterator, "next", "()Ljava/lang/Object;");
     jstring tmppath = (jstring) ret3.l;
     pathlist[i] = mClient.getEnv().jstringToChar(tmppath);
  }
  return numofPaths;
}
//Add DatasetInfo
void TDMSFileSystem::addDatasetInfo(char *name,  char* keylist[], char* valuelist[], int argsize) {
  Env env;
  jstring jName = env.newStringUTF(name, "err");
  jstring jKeyStr;
  jstring jValueStr;
  jobject keyset = env.newObject("Ljava/util/ArrayList;", "()V");
  jobject valueset = env.newObject("Ljava/util/ArrayList;", "()V");
  jvalue ret;
  for (int i = 0; i < argsize; i++) {
    jKeyStr = env.newStringUTF(keylist[i], "err");
    jValueStr = env.newStringUTF(valuelist[i], "err");
    env.callMethod(&ret, keyset, "add", "(Ljava/lang/Object;)Z", jKeyStr);
    env.callMethod(&ret, valueset, "add", "(Ljava/lang/Object;)Z", jValueStr);
  }
  env.callMethod(&ret, mClient.getJObj(),"addDatasetInfo", "(Ljava/lang/String;Ljava/util/List;Ljava/util/List;)V", jName, keyset, valueset);
}

void TDMSFileSystem::setDatasetInfo(const char *path) {
  jTDMSURI uri = TDMSURI::newURI(path);
  jvalue ret;
  mClient.getEnv().callMethod(&ret, mClient.getJObj(),"setDatasetInfo", "(Lalluxio/AlluxioURI;)V", uri->getJObj());
}

jFileOutStream
TDMSFileSystem::createFile(const char *path,
                              TDMSCreateFileOptions *options) {
  jvalue ret;

  jTDMSURI uri = TDMSURI::newURI(path);


  if (options == NULL) {
    mClient.getEnv().callMethod(
        &ret, mClient.getJObj(), "createFile",
        "(Lalluxio/AlluxioURI;)Lalluxio/client/file/FileOutStream;",
        uri->getJObj());
  } else {
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "createFile",
                                "(Lalluxio/AlluxioURI;Lalluxio/client/file/"
                                "options/CreateFileOptions;)Lalluxio/client/"
                                "file/FileOutStream;",
                                uri->getJObj(), options->getOptions());
  }

  return (new FileOutStream(mClient.getEnv(), ret.l));
}

void TDMSFileSystem::renameFile(const char *origPath, const char *newPath) {
  jvalue ret;

  jTDMSURI origURI = TDMSURI::newURI(origPath);
  jTDMSURI newURI  = TDMSURI::newURI(newPath);

  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "rename",
                              "(Lalluxio/AlluxioURI;Lalluxio/AlluxioURI;)V",
                              origURI->getJObj(), newURI->getJObj());
}

// FIXME: We should be able to query the open file options and not require them
// to be passed in
jFileOutStream
TDMSFileSystem::openFileForAppend(const char *path,
                                     TDMSCreateFileOptions *options) {
  const int bufferSize = 1000000;
  const char *appendSuffix = ".append";
  int bytesRead = bufferSize;
  jFileInStream origFile = NULL;
  jFileOutStream newFile = NULL;
  // FIXME: Change to std::vector
  char *inputBuffer = (char *)calloc(bufferSize, 1);

  if (inputBuffer == NULL) {
    throw std::bad_alloc();
  }

  int appendPathLength = strlen(path) + strlen(appendSuffix) + 1;

  // FIXME: Change to std::vector
  char *appendPath = (char *)malloc(appendPathLength);

  if (appendPath == NULL) {
    throw std::bad_alloc();
  }

  strncpy(appendPath, path, appendPathLength);
  strcat(appendPath, ".append");

  try {
    origFile = openFile(path, nullptr);
    newFile = createFile(appendPath, options);

    // Copy the original file over to the new file
    while (bytesRead == bufferSize) {
      bytesRead = origFile->read(inputBuffer, bufferSize);
      if (bytesRead > 0) {
          newFile->write(inputBuffer, bytesRead);
      }
    }

    origFile->close();
  }
  catch (...) {
    free(inputBuffer);
    free(appendPath);
    throw;
  }

  free(inputBuffer);
  free(appendPath);

  return newFile;
}

void TDMSFileSystem::completeAppend(const char *path,
                                       jFileOutStream fileOutStream) {
  const char *appendSuffix = ".append";
  int appendPathLength = strlen(path) + strlen(appendSuffix) + 1;
  char *appendPath = (char *)malloc(appendPathLength);

  if (appendPath == NULL) {
    throw std::bad_alloc();
  }

  strncpy(appendPath, path, appendPathLength);
  strcat(appendPath, ".append");

  try {
    fileOutStream->close();

    // Delete original file
    deletePath(path);

    // NOTE: This is the critical section of this method.  If a failure
    // occurs here, we will be left with a .append file and no original file.

    // TODO: we should add some defensive code in the open file path to
    // perform this rename automatically if we only find a .append file

    // Rename file
    renameFile(appendPath, path);
  }
  catch (...) {
    free(appendPath);
    throw;
  }

  free(appendPath);
}

long int TDMSFileSystem::fileSize(const char *path) {
  jvalue retGetStatus;
  jvalue retGetSize;

  // FIXME: where are we cleaning up ret* memory?  Is there a huge memory leak
  // going on here?  Also, this whole file doesn't appear to be exception safe.
  jTDMSURI uri = TDMSURI::newURI(path);

  mClient.getEnv().callMethod(
      &retGetStatus, mClient.getJObj(), "getStatus",
      "(Lalluxio/AlluxioURI;)Lalluxio/client/file/URIStatus;", uri->getJObj());

  mClient.getEnv().callMethod(&retGetSize, retGetStatus.l, "getLength", "()J");

  return retGetSize.j;
}

/**
   Return a list of files in the given path.

   @param[in] path Path to list files/dirs from
   @param[in] filter Type of filter to apply to list call
   @return Vector of strings.  Each entry is a file in the path.
*/

std::vector<std::string> TDMSFileSystem::listPath(const char *path,
                                                     ListPathFilter filter) {
  std::vector<std::string> files;

  jvalue retList;
  jTDMSURI uri = TDMSURI::newURI(path);
  mClient.getEnv().callMethod(&retList, mClient.getJObj(), "listStatus",
                              "(Lalluxio/AlluxioURI;)Ljava/util/List;", uri->getJObj());

  // List<URIStatus>.size()
  jvalue retGetSize;
  mClient.getEnv().callMethod(&retGetSize, retList.l, "size", "()I");

  files.reserve(retGetSize.i);
  for(int i = 0; i < retGetSize.i; i++) {
    jvalue uriObj;
    mClient.getEnv().callMethod(&uriObj, retList.l, "get",
                                "(I)Ljava/lang/Object;", (jint)i);

    jvalue uriString;
    mClient.getEnv().callMethod(&uriString, uriObj.l, "toString",
            "()Ljava/lang/String;", uriObj.l);
    std::string rawObjStr;
    mClient.getEnv().jstringToString(static_cast<jstring>(uriString.l), rawObjStr);

    // Parse out just the file name from the raw string
    const static std::string PATH_MARKER_START = "path=";
    const static std::string PATH_MARKER_END = ", ";
    std::string pathStr = rawObjStr.substr(rawObjStr.find(PATH_MARKER_START) + PATH_MARKER_START.size());
    pathStr = pathStr.substr(0, pathStr.find(PATH_MARKER_END));

    switch (filter) {
    case ListPathFilter::NONE:
      files.push_back(pathStr);
      break;

    case ListPathFilter::DIRECTORIES_ONLY:
      if (rawObjStr.find("folder=true") != std::string::npos) {
        files.push_back(pathStr);
      }
      break;

    default:
      std::ostringstream errMsg;
      errMsg << "Unknown listPath() filter " << int(filter);
      throw std::runtime_error(errMsg.str());
    }
  }

  return files;
}

// Optimizaiton for different data access patterns
void TDMSFileSystem::setDataAccessPattern(char* DAP)
{
  Env env;
  jvalue  ret;  
  jobject filewritepolicy;
  // Set block size to 512MB by default
  long myblocksize = 512*1024*1024;
  int mytier = 0;
  std::string mydap = DAP;
  jstring pattern = env.newStringUTF(DAP, "err");
  jstring targetHost = env.newStringUTF("NULL", "err");
  if(mydap == "PIPELINE"){ 
    printf("Set  Data Access Pattern to <PIPELINE>\n");
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, (jint)mytier, targetHost, (jlong)myblocksize);
  }else if (mydap == "SCATTER"){
    printf("Set  Data Access Pattern to <SCATTER>\n");
    // Using MaxFree LoadBalanceStrategy to choose storage tier
    char * lb = "MaxFree";
    jvalue jmytier;
    jstring jmyLoadBalanceStrategy = env->NewStringUTF(lb);
    env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);
    printf("Write with storage tier %d \n",jmytier.i);
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, jmytier.i, targetHost, (jlong)myblocksize); 
  }else if (mydap == "GATHER"){

    printf("Error: Data Access Pattern <GATHER> needs to specify a hostname to collect data\nUsage: setDataAccessPattern(char* DAP, char* tagethost)\n");

  }else if (mydap == "MULTICAST"){
    printf("Set  Data Access Pattern to <MULTICAST>, write data to memory tier by default. Enable adaptive tier selection by using setDataAccessPattern(char* DAP, double size)\n");
    // Set Write tier to memory tier by default
    mytier = 0;
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, (jint)mytier, targetHost, (jlong)myblocksize);
  }else if (mydap == "REDUCE"){

    printf("Set  Data Access Pattern to <REDUCE>\n");
    // Using RoundRobin LoadBalanceStrategy to choose storage tier
    char * lb = "MaxFree";
    jvalue jmytier;
    jstring jmyLoadBalanceStrategy = env->NewStringUTF(lb);
    env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);
    printf("Write with storage tier %d \n",jmytier.i);
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, jmytier.i, targetHost, (jlong)myblocksize);
  }else{
    // Using RoundRobin LoadBalanceStrategy to choose storage tier
    char * lb = "MaxFree";
    jvalue jmytier;
    jstring jmyLoadBalanceStrategy = env->NewStringUTF(lb);
    env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);
    printf("Write with storage tier %d \n",jmytier.i);
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, jmytier.i, targetHost, (jlong)myblocksize);
    printf("Set  Data Access Pattern to %s \n", DAP);
  }

}

void TDMSFileSystem::setDataAccessPattern(char* DAP, char* tagethost){
  Env env;
  jvalue  ret;
  long myblocksize = 512*1024*1024;
  int mytier = 0;
  jstring pattern = env.newStringUTF(DAP, "err");
  jstring targetHost = env.newStringUTF(tagethost, "err");
  std::string mydap = DAP;
  if(mydap == "GATHER" || mydap == "REDUCE"){
    printf("Set  Data Access Pattern to <GATHER> with host %s\n",tagethost);
    // Using MaxFree LoadBalanceStrategy to choose storage tier
    char * lb = "MaxFree";
    jvalue jmytier;
    jstring jmyLoadBalanceStrategy = env->NewStringUTF(lb);
    env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);

    // Set block size to 512MB by default
    long myblocksize = 512*1024*1024;
    mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, jmytier.i, targetHost, (jlong)myblocksize);
   }else{
    printf("Failed to  Data Access Pattern \nOnly GATHER PATTERN needs argment <tagethost>\nTry setDataAccessPattern(char* DAP) instead for other data access pattern!\n");
  }

}

void TDMSFileSystem::setDataAccessPattern(char* DAP, double size){
  Env env;
  jvalue  ret;
  long myblocksize = (long) size;
  int mytier = 0;
  std::string mydap = DAP;
  jstring pattern = env.newStringUTF(DAP, "err");
  jstring targetHost = env.newStringUTF("NULL", "err");
  if(mydap == "MULTICAST" || mydap == "SCATTER"){
    printf("Set  Data Access Pattern to <MULTICAST>, write data to memory tier by default. Enable adaptive tier selection by using setDataAccessPattern(char* DAP, double size)\n"); 

    // choose storage tier depends on file size.
    if (size < 1024*1024) {
      mytier = 0;
      mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, (jint) mytier, targetHost, (jlong)myblocksize);
    } else {
      // Using MaxFree LoadBalanceStrategy to choose storage tier
      char * lb = "MaxFree";
      jvalue jmytier;
      jstring jmyLoadBalanceStrategy = env->NewStringUTF(lb);
      env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);
      mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setDataAccessPattern", "(Ljava/lang/String;ILjava/lang/String;J)V", pattern, jmytier.i, targetHost, (jlong)myblocksize);
    }

  } else {
    printf("Failed to  Data Access Pattern \nOnly MULTICAST PATTERN needs argment <FileSize>\nTry setDataAccessPattern(char* DAP) instead for other data access pattern!\n");
  }
}

void TDMSFileSystem::defineDataAccessPattern(char* DAP) {
  Env env;
  jvalue  ret;
  jstring pattern = env.newStringUTF(DAP, "err");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "defineDataAccessPattern", "(Ljava/lang/String;)V", pattern);
}

void TDMSFileSystem::setStorageTier(char* DAP, int tier) {
  Env env;
  jvalue  ret;
  jstring pattern = env.newStringUTF(DAP, "err");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setStorageTier", "(Ljava/lang/String;I)V", pattern, (jint)tier);
}

void TDMSFileSystem::setBlockSize(char* DAP, double size) {
  Env env;
  long myblocksize = (long) size;
  jvalue  ret;
  jstring pattern = env.newStringUTF(DAP, "err");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setBlockSize", "(Ljava/lang/String;J)V", pattern, (jlong)myblocksize);
}

void TDMSFileSystem::setLayoutStrategy(char* DAP, char* layout) {
  Env env;
  jvalue  ret;
  jstring pattern = env.newStringUTF(DAP, "err");
  jstring mlayout = env.newStringUTF(layout, "err");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setLayoutStrategy", "(Ljava/lang/String;Ljava/lang/String;)V", pattern, mlayout);
}

void TDMSFileSystem::setLoadBalanceStrategy(char* DAP, char* loadbalance) {
  Env env;
  jvalue  ret;
  jstring pattern = env.newStringUTF(DAP, "err");
  jstring mloadbalance = env.newStringUTF(loadbalance, "err");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setLoadBalanceStrategy", "(Ljava/lang/String;Ljava/lang/String;)V", pattern, mloadbalance);
}

void TDMSFileSystem::setHost(char* DAP, char* host) {
  Env env;
  jvalue  ret;
  jstring pattern = env.newStringUTF(DAP, "err");
  jstring mhost = env.newStringUTF(host, "err");
  mClient.getEnv().callMethod(&ret, mClient.getJObj(), "setHost", "(Ljava/lang/String;Ljava/lang/String;)V", pattern, mhost);
}

//////////////////////////////////////////
//// FileOutStream
////////////////////////////////////////////

// Close FileOutStream
void FileOutStream::close()
{
  m_env.callMethod(NULL, m_obj, "close", "()V");
}

//Aborts the output stream. Not tested yet.  Use with care
void FileOutStream::cancel() 
{
  m_env.callMethod(NULL, m_obj, "cancel", "()V");
}

//Flush the data
void FileOutStream::flush()
{
  m_env.callMethod(NULL, m_obj, "flush", "()V");
}

//Write data 
void FileOutStream::write(int byte) 
{
  m_env.callMethod(NULL, m_obj, "write", "(I)V", (jint) byte);
}


void FileOutStream::write(const void *buff, int length)
{
  write(buff, length, 0, length);
}

void FileOutStream::write(const void *buff, int length, int off, int maxLen)
{
  jthrowable exception;
  jbyteArray jBuf;

  jBuf = m_env.newByteArray(length);
  m_env->SetByteArrayRegion(jBuf, 0, length, (jbyte*) buff);

  std::unique_ptr<char[]> jbuff(new char[length * sizeof(char)]);
  m_env->GetByteArrayRegion(jBuf, 0, length, (jbyte*) jbuff.get());
  // printf("byte array in write: %s\n", jbuff);
 
  if (off < 0 || maxLen <= 0 || length == maxLen)
    m_env.callMethod(NULL, m_obj, "write", "([B)V", jBuf);
  else
    m_env.callMethod(NULL, m_obj, "write", "([BII)V", jBuf, (jint) off, (jint) maxLen);
 
  m_env->DeleteLocalRef(jBuf);
}


//Build index
void FileOutStream::buildIndex(int size, bool augmented){
  m_env.callMethod(NULL, m_obj, "buildIndex", "(IZ)V", (jint)size, (jboolean)augmented);
}

//Get index flag
bool FileOutStream::shouldIndex(){
  jvalue ret;
  m_env.callMethod(&ret, m_obj, "shouldIndex","()Z");
  return ret.z;
}

//Get block max value
double FileOutStream::getBlockMax(){
  jvalue ret;
  m_env.callMethod(&ret, m_obj,"getBlockMax","()D");
  return ret.d;
}

//Get block min value
double FileOutStream::getBlockMin(){
  jvalue ret;
  m_env.callMethod(&ret, m_obj,"getBlockMin","()D");
  return ret.d;
}

//////////////////////////////////////////
////// FileInStream
//////////////////////////////////////////////

void FileInStream::close()
{
  m_env.callMethod(NULL, m_obj, "close", "()V");
}

int FileInStream::read()
{
  jvalue ret;
  m_env.callMethod(&ret, m_obj, "read", "()I");
  return ret.i;
}

int FileInStream::read(void *buff, int length)
{
  return read(buff, length, 0, length, false, NULL, NULL, NULL);
}


// TODO: Template this funtion for time measurement?
int FileInStream::read(void *buff, int length, int off, int maxLen, 
      bool measureTime = false,
      std::chrono::duration<double>* pBufferCreationTimeCounter = NULL,
      std::chrono::duration<double>* pReadTimeCounter = NULL,
      std::chrono::duration<double>* pBufferCopyTimeCounter = NULL)
{
   jbyteArray jBuf;
   jvalue ret;
   int rdSz;

   std::chrono::duration<double> duration = std::chrono::duration<double>::zero();
   std::chrono::time_point<std::chrono::system_clock> startTime, stopTime;

   try {

      if (measureTime)
      {
         startTime = std::chrono::system_clock::now();
      }

      jBuf = m_env.newByteArray(length);

      if (measureTime)
      {
         stopTime = std::chrono::system_clock::now();
         duration = stopTime - startTime;
         *pBufferCreationTimeCounter += duration;
      }

      if (off < 0 || maxLen <= 0 || length == maxLen)
      {
         if (measureTime)
         {
            startTime = std::chrono::system_clock::now();
         }

         m_env.callMethod(&ret, m_obj, "read", "([B)I", jBuf);

         if (measureTime)
         {
            stopTime = std::chrono::system_clock::now();
         }
      }
      else
      {
         if (measureTime)
         {
            startTime = std::chrono::system_clock::now();
         }

         m_env.callMethod(&ret, m_obj, "read", "([BII)I", jBuf, off, maxLen);

         if (measureTime)
         {
            stopTime = std::chrono::system_clock::now();
         }
      }
      if (measureTime)
      {
         duration = stopTime - startTime;
         *pReadTimeCounter += duration;
      }
   } catch (NativeException) {
      if (jBuf != NULL) {
         m_env->DeleteLocalRef(jBuf);
      }
      throw;
   }
   rdSz = ret.i;
   if (rdSz > 0) {
      if (measureTime)
      {
         startTime = std::chrono::system_clock::now();
      }
      // TODO: It's much more efficient to get direct access to the buffer,
      //       but doing so requires the caller to create the java buffer.
      //       Create a read method that allows for this option.
      //       buff = m_env->GetDirectBufferAddress(jBuf);
      m_env->GetByteArrayRegion(jBuf, 0, rdSz, (jbyte*) buff);
      if (measureTime)
      {
         stopTime = std::chrono::system_clock::now();
         duration = stopTime - startTime;
         *pBufferCopyTimeCounter += duration;
      }
   }
   m_env->DeleteLocalRef(jBuf);
   return rdSz;
}

int FileInStream::read(void *buff, int off, int length){
    jbyteArray jBuf;
    jvalue ret;
    int rdSz;
    try{
      jBuf = m_env.newByteArray(length);
      m_env.callMethod(&ret, m_obj, "read", "([BII)I", jBuf, off, length);
    } catch (NativeException) {
      if (jBuf != NULL) {
         m_env->DeleteLocalRef(jBuf);
      }
      throw;
    }
    rdSz = ret.i;
    if (rdSz > 0) {
      m_env->GetByteArrayRegion(jBuf, 0, rdSz, (jbyte*) buff);
    }
    m_env->DeleteLocalRef(jBuf);
    return rdSz;
}

void FileInStream::seek(long pos)
{
  m_env.callMethod(NULL, m_obj, "seek", "(J)V", (jlong) pos);
}

long FileInStream::maxSeekPosition()
{
  jvalue ret;
  m_env.callMethod(&ret, m_obj, "maxSeekPosition", "()J");
  return ret.j;
}

long FileInStream::skip(long n)
{
  jvalue ret;
  m_env.callMethod(&ret, m_obj, "skip", "(J)J", (jlong) n);
  return ret.j;
}



// File create/open Options

jTDMSCreateFileOptions TDMSCreateFileOptions::getCreateFileOptions()
{
    Env env;
    jvalue jFileOptions;

    // First get the ClientContext.Configuration
    env.callStaticMethod(&jFileOptions, 
            "alluxio/client/file/options/CreateFileOptions", "defaults", 
            "()Lalluxio/client/file/options/CreateFileOptions;");

    return new TDMSCreateFileOptions(env, jFileOptions.l);
}

jTDMSOpenFileOptions TDMSOpenFileOptions::getOpenFileOptions()
{
    Env env;
    jvalue jFileOptions;

    // First get the ClientContext.Configuration
    env.callStaticMethod(&jFileOptions, 
            "alluxio/client/file/options/OpenFileOptions", "defaults", 
            "()Lalluxio/client/file/options/OpenFileOptions;");

    return new TDMSOpenFileOptions(env, jFileOptions.l);
}

//Set file struct info to guide the index generation
void TDMSCreateFileOptions::setFileInfo(int num, ...){
  Env m_env;
  jclass list_jcs = m_env.findClass("java/util/ArrayList");
  if (list_jcs == NULL) {
    printf("Java class: ArrayList not find\n");
    exit(-1);
  }
  jmethodID list_init = m_env.getMethodId(list_jcs, "<init>", "()V");

  jobject varlist = m_env->NewObject(list_jcs, list_init, "");
  jobject typelist = m_env->NewObject(list_jcs, list_init, "");

  jstring jvarname;
  jstring jvartype;

  va_list ap;
  va_start(ap,num);
  char* tmp1;
  char* tmp2;
  for(int i=0; i<num; i++){
    tmp1 = va_arg(ap, char*);
    tmp2 = va_arg(ap, char*);
    if(tmp1 == NULL || tmp2 == NULL) {
      printf("Error in var number, please check it carefully! \n");
      exit(-1);
    }
    jvarname = m_env.newStringUTF(tmp1, tmp1);
    jvartype = m_env.newStringUTF(tmp2, tmp2);
    m_env.callMethod(NULL, varlist, "add", "(Ljava/lang/Object;)Z", jvarname);
    m_env.callMethod(NULL, typelist, "add", "(Ljava/lang/Object;)Z", jvartype);
    printf("varname is %s, type is %s \n",tmp1, tmp2);
  }

  m_env.callMethod(NULL, m_obj, "setFileInfo", "(Ljava/util/ArrayList;Ljava/util/ArrayList;)V", varlist, typelist);

  va_end(ap);
  m_env->DeleteLocalRef(varlist);
  m_env->DeleteLocalRef(typelist);
  m_env->DeleteLocalRef(jvarname);
  m_env->DeleteLocalRef(jvartype);
}

/* vim: set ts=4 sw=4 : */
