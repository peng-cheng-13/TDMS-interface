/**
 *  @author        Adam Storm <ajstorm@ca.ibm.com>
 *  @organization  IBM
 * 
 * Alluxio C/C++ APIs
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
  Set a string value in the Alluxio constants

  @param[in] env The jni environment
  @param[in] key Key of string constant to be set
  @param[in] value Value to be assigned

void TDMSClientContext::setAlluxioStringConstant(jni::Env &env, const char *key, const char *value) {
  jvalue  retSet;

  // Get the key object
  JNIObjBase jKeyObject(env, env.getEnumObject("alluxio/Constants", key, "Ljava/lang/String;"));

  // Construct the string
  JNIStringBase jValueString(env, env.newStringUTF(value, value));

  // Call the methods to set the string
  env.callStaticMethod(&retSet, "alluxio/Configuration", "set",
                      "(Ljava/lang/String;Ljava/lang/String;)V", 
                      static_cast<jstring>(jKeyObject.getJObj()),
                      jValueString.getJString());
}
*/
/**
  Connect to the Alluxio master node.

  This only needs to be done once per process.  However, each thread that
  interface with Alluxio needs to perform an attach first before it can use
  JNI.

  @param[in] host Host name of the Alluxio master node
  @param[in] port Port name for the Alluxio master node
*/
/*void TDMSClientContext::connect() {
  //Env env;
  //jclass cls;
  //jvalue ret; 
  //AlluxioClientContext::setAlluxioStringConstant(env, "MASTER_HOSTNAME", "cn17638-enp5s0");
  //AlluxioClientContext::setAlluxioStringConstant(env, "MASTER_RPC_PORT", "39999");
}*/

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
  printf("URL structure init succeed \n");
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

void FileInStream::seek(long pos)
{
  m_env.callMethod(NULL, m_obj, "seek", "(J)V", (jlong) pos);
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

// Optimizaiton for different data access patterns
void TDMSCreateFileOptions::setDataAccessPattern(char* DAP)
{
  Env env;
  jvalue  ret;  
  jobject filewritepolicy;
  long myblocksize;
  int mytier;
  std::string mydap = DAP;
  if(mydap == "PIPELINE"){   

    printf("Set  Data Access Pattern to <PIPELINE>\n");
    // Set filewritepolicy to localfirst policy
    filewritepolicy = env.getLocationPolicy(DAP,NULL);
    m_env.callMethod(&ret, m_obj,"setLocationPolicy","(Lalluxio/client/file/policy/FileWriteLocationPolicy;)Lalluxio/client/file/options/CreateFileOptions;",filewritepolicy);
    // Set block size to 512MB by default
    //myblocksize = 512*1024*1024;
    //m_env.callMethod(&ret, m_obj,"setBlockSizeBytes","(J)Lalluxio/client/file/options/CreateFileOptions;",(jlong)myblocksize);
    // Set Write tier to memory tier
    mytier = 0;
    m_env.callMethod(&ret, m_obj,"setWriteTier","(I)Lalluxio/client/file/options/CreateFileOptions;",(jint)mytier);

  }else if (mydap == "SCATTER"){

    printf("Set  Data Access Pattern to <SCATTER>\n");
    // Set filewritepolicy to RoundRobinPolicy
    filewritepolicy = env.getLocationPolicy(DAP,NULL);
    m_env.callMethod(&ret, m_obj,"setLocationPolicy","(Lalluxio/client/file/policy/FileWriteLocationPolicy;)Lalluxio/client/file/options/CreateFileOptions;",filewritepolicy);
    // Using MaxFree LoadBalanceStrategy to choose storage tier
    char * lb = "MaxFree";
    jvalue jmytier;
    jstring jmyLoadBalanceStrategy = m_env->NewStringUTF(lb);
    m_env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);
    printf("Write with storage tier %d \n",jmytier.i);
    m_env.callMethod(&ret, m_obj,"setWriteTier","(I)Lalluxio/client/file/options/CreateFileOptions;",jmytier.i);
  
  }else if (mydap == "GATHER"){

    printf("Error: Data Access Pattern <GATHER> needs to specify a hostname to collect data\nUsage: setDataAccessPattern(char* DAP, char* tagethost)\n");

  }else if (mydap == "MULTICAST"){

    printf("Set  Data Access Pattern to <MULTICAST>\n");
    // Set filewritepolicy to RoundRobinPolicy policy
    filewritepolicy = env.getLocationPolicy(DAP,NULL);
    m_env.callMethod(&ret, m_obj,"setLocationPolicy","(Lalluxio/client/file/policy/FileWriteLocationPolicy;)Lalluxio/client/file/options/CreateFileOptions;",filewritepolicy);
    // Set Write tier to memory tier
    mytier = 0;
    m_env.callMethod(&ret, m_obj,"setWriteTier","(I)Lalluxio/client/file/options/CreateFileOptions;",(jint)mytier); 
    
  }else if (mydap == "REDUCE"){

    printf("Set  Data Access Pattern to <REDUCE>\n");
    // Set filewritepolicy to RoundRobinPolicy
    filewritepolicy = env.getLocationPolicy(DAP,NULL);
    m_env.callMethod(&ret, m_obj,"setLocationPolicy","(Lalluxio/client/file/policy/FileWriteLocationPolicy;)Lalluxio/client/file/options/CreateFileOptions;",filewritepolicy);
    // Using RoundRobin LoadBalanceStrategy to choose storage tier
    char * lb = "RoundRobin";
    jvalue jmytier;
    jstring jmyLoadBalanceStrategy = m_env->NewStringUTF(lb);
    m_env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);
    printf("Write with storage tier %d \n",jmytier.i);
    m_env.callMethod(&ret, m_obj,"setWriteTier","(I)Lalluxio/client/file/options/CreateFileOptions;",jmytier.i);
  }else{

    printf("Unknown Data Access Pattern!\nSupported Data Access Pattern are PIPELINE / SCATTER / GATHER / MULTICAST / REDUCE ! \n");

  }

}

void TDMSCreateFileOptions::setDataAccessPattern(char* DAP, char* tagethost){
  Env env;
  jvalue  ret;
  jobject filewritepolicy;
  std::string mydap = DAP;
  if(mydap == "GATHER"){
    printf("Set  Data Access Pattern to <GATHER> with host %s\n",tagethost);
    //Set filewritepolicy to write data to target host
    filewritepolicy = env.getLocationPolicy(DAP,tagethost);
    m_env.callMethod(&ret, m_obj,"setLocationPolicy","(Lalluxio/client/file/policy/FileWriteLocationPolicy;)Lalluxio/client/file/options/CreateFileOptions;",filewritepolicy);

    // Using MaxFree LoadBalanceStrategy to choose storage tier
    char * lb = "MaxFree";
    jvalue jmytier;
    jstring jmyLoadBalanceStrategy = m_env->NewStringUTF(lb);
    m_env.callStaticMethod(&jmytier,"InitTDMS","getLoadBalanceStrategy", "(Ljava/lang/String;)I",jmyLoadBalanceStrategy);
    // m_env.callMethod(&ret, m_obj,"setWriteTier","(I)Lalluxio/client/file/options/CreateFileOptions;",jmytier.i);
	m_env.callMethod(&ret, m_obj,"setWriteTier","(I)Lalluxio/client/file/options/CreateFileOptions;",(jint)0);	

    // Set block size to 512MB by default
    long myblocksize = 512*1024*1024;
    m_env.callMethod(&ret, m_obj,"setBlockSizeBytes","(J)Lalluxio/client/file/options/CreateFileOptions;",(jlong)myblocksize);
   }else{
    printf("Failed to  Data Access Pattern \nOnly GATHER PATTERN needs argment <tagethost>\nTry setDataAccessPattern(char* DAP) instead for other data access pattern!\n");
  }

}


/* vim: set ts=4 sw=4 : */
