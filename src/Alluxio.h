/**
 *  @author         Peng Cheng <peng_cheng_13@163.com>
 *  @organization   NSCC-GZ
 * 
 * @author        Adam Storm <ajstorm@ca.ibm.com>
 * @organization  IBM
 *
 * TDMS C/C++ APIs on top of Alluxio
 *
 */

#ifndef __ALLUXIO_H_
#define __ALLUXIO_H_

#include<jni.h>
#include<stdint.h>
#include<chrono>
#include<functional>
#include <memory>
#include <vector>

#include "JNIHelper.h"

#define BBUF_CLS                    "java/nio/ByteBuffer"

#define TREADT_CLS                  "alluxio/client/ReadType"
#define TWRITET_CLS                 "alluxio/client/WriteType"

// TODO: While these macros have been changed (via search and replace) to
// // alluxio, they may not be used correctly yet.  No porting has been done
// // to the underlying calling functions.
// // InStream
// //!!!#define TISTREAM_CLS                "alluxio/client/InStream"
#define TFILE_ISTREAM_CLS           "alluxio/client/file/FileInStream"
#define TBLOCK_ISTREAM_CLS          "alluxio/client/block/stream/BlockInStream"
//
// // OutStream
//#define TOSTREAM_CLS                "alluxio/client/OutStream"
#define TFILE_OSTREAM_CLS           "alluxio/client/file/FileOutStream"
#define TBLOCK_OSTREAM_CLS          "alluxio/client/block/BlockOutStream"
#define UFS_OSTREAM_CLS             "alluxio/client/block/stream/UnderFileSystemFileOutStream"


namespace tdms {

class TDMSFileSystem;
class TDMSURI;
class FileOutStream;
class FileInStream;
class TDMSCreateFileOptions;
class TDMSOpenFileOptions;

typedef TDMSFileSystem* jTDMSFileSystem;
typedef TDMSURI* jTDMSURI;
typedef FileOutStream* jFileOutStream;
typedef FileInStream* jFileInStream;
typedef TDMSCreateFileOptions* jTDMSCreateFileOptions; 
typedef TDMSOpenFileOptions* jTDMSOpenFileOptions;

class JNIStringBase {
  public:
    JNIStringBase(jni::Env env, jstring localString): m_env(env) { 
      m_string = reinterpret_cast<jstring>(env->NewGlobalRef(localString));
      // this means after the constructor, the localObj will be destroyed
      env->DeleteLocalRef(localString);
    }

    ~JNIStringBase() { m_env->DeleteGlobalRef(m_string); }

    jstring getJString() { return m_string; }
    jni::Env& getEnv() { return m_env; }

  protected:
    jni::Env m_env;
    jstring m_string; // the underlying jstring
};

class JNIObjBase {
  public:
    JNIObjBase(jni::Env env, jobject localObj): m_env(env) {
      m_obj = env->NewGlobalRef(localObj);
      // this means after the constructor, the localObj will be destroyed
      env->DeleteLocalRef(localObj);
    }

    ~JNIObjBase() { m_env->DeleteGlobalRef(m_obj); }

    jobject getJObj() { return m_obj; }
    jni::Env& getEnv() { return m_env; }

  protected:
    jni::Env m_env;
    jobject m_obj; // the underlying jobject
};

class TDMSClientContext
{
    public:
        TDMSClientContext();
        ~TDMSClientContext();
        //static void connect();
        //static void setAlluxioStringConstant(jni::Env &env, const char *key, const char *value);

        jobject getJObj() { return m_baseFileSystem; }
        jni::Env &getEnv() { return m_env; }

    private:
        bool m_mustDetachInDtor;
        bool m_mustDeleteLocalRef;
        jni::Env m_env;
        /// Pointer to Java object that has all APIs for Alluxio file system.
        jobject m_baseFileSystem;
        void attach();
};

/// Enum to control what is filtered in the listPath call
enum class ListPathFilter {
  // No filtering in listPath().  Return everything under given path.
  NONE,
  // Only return directories in listPath() call
  DIRECTORIES_ONLY
};

class TDMSFileSystem {
    public:
        TDMSFileSystem(TDMSClientContext& clientContext);
        bool exists(const char *path);
        long int fileSize(const char *path);
        void createDirectory(const char *path);
        void deletePath(const char *path, bool recursive = false);
        void deleteAlluxioPath(const char *path, bool alluxioOnly = false);
        void loadMetadata(const char *path);
        jFileInStream openFile(const char *path, TDMSOpenFileOptions *options = nullptr);
        jFileInStream queryFile(const char *path, const char *varname, double max, double min, bool augmented);
        //char **selectFiles(const char* keylist[], const char* valuelist[], const char* typelist[], int argsize, int mpirsize, int mpirank);
        int selectFiles(char **pathlist, const char* keylist[], const char* valuelist[], const char* typelist[], int argsize, int mpirsize, int mpirank);
        void addDatasetInfo(char *name,  char* keylist[], char* valuelist[], int argsize);
        void setDatasetInfo(const char *path);
        jFileOutStream createFile(const char * path, TDMSCreateFileOptions *options = nullptr);
        jFileOutStream openFileForAppend(const char *path, TDMSCreateFileOptions *options);
        void completeAppend(const char *path, jFileOutStream fileOutStream);
        void renameFile(const char *origPath, const char *newPath);
        std::vector<std::string> listPath(const char * path, ListPathFilter filter);
        void setDataAccessPattern(char* DAP);
        void setDataAccessPattern(char* DAP, char* tagethost);
        void setDataAccessPattern(char* DAP, double size);
        void defineDataAccessPattern(char* DAP);
        void setStorageTier(char* DAP, int tier);
        void setBlockSize(char* DAP, double size);
        void setLayoutStrategy(char* DAP, char* layout);
        void setLoadBalanceStrategy(char* DAP, char* loadbalance);
        void setHost(char* DAP, char* host);

    private:
        TDMSClientContext& mClient;
};

class TDMSURI : public JNIObjBase {
  public:
    static jTDMSURI newURI(const char *pathStr);
    //static jTDMSURI newURI(const char *scheme, const char *authority, const char *path);
    //static jTDMSURI newURI(jAlluxioURI parent, jAlluxioURI child);

    TDMSURI(jni::Env env, jobject uri): JNIObjBase(env, uri){}
};

//File output stream 
class FileOutStream : public JNIObjBase
{
  public: 
    FileOutStream(jni::Env env, jobject fileOutStream) : JNIObjBase (env, fileOutStream){}

  void cancel();
  void close();
  void flush();
  void write(int byte);
  void write(const void *buff, int length);
  void write(const void *buff, int length, int off, int maxLen);
  void buildIndex(int size, bool augmented);
  bool shouldIndex();
  double getBlockMax();
  double getBlockMin();
};

class TDMSCreateFileOptions : public JNIObjBase
{
  public:
    static jTDMSCreateFileOptions getCreateFileOptions();
    void setFileInfo(int num, ...);
    //void setWriteType(WriteType writeType);
    jobject getOptions()
    {
      return m_obj;
    }

  private:
    TDMSCreateFileOptions(jni::Env env, jobject createFileOptions) :  JNIObjBase(env, createFileOptions) {}
};



//File input stream
class FileInStream : public JNIObjBase {
  public:
    FileInStream(jni::Env env, jobject istream): JNIObjBase(env, istream){}
  
    void close();
    int read();
    int read(void *buff, int length);
    int read(void *buff, int length, int off, int maxLen, 
          bool measureTime,
          std::chrono::duration<double>* pBufferCreationTimeCounter,
          std::chrono::duration<double>* pReadTimeCounter,
          std::chrono::duration<double>* pBufferCopyTimeCounter);
    int read(void *buff, int off, int length);
    long maxSeekPosition();
    void seek(long pos);
    long skip(long n);
  
};


class TDMSOpenFileOptions : public JNIObjBase
{
    public:
        static jTDMSOpenFileOptions getOpenFileOptions();
        //void setReadType(ReadType readType);
        jobject getOptions()
        {
            return m_obj;
        }

    private:
        TDMSOpenFileOptions(jni::Env env, jobject openFileOptions) : JNIObjBase(env, openFileOptions) {}
};

}
/*
#ifdef __cplusplus
extern "C" {
#endif

jobject enumObjReadType(tdms::jni::Env& env, alluxio::ReadType readType);
jobject enumObjWriteType(tdms::jni::Env& env, alluxio::WriteType writeType);

char* fullAlluxioPath(const char *masterUri, const char *filePath);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __TDMS_H_ */

/* vim: set ts=4 sw=4 : */
