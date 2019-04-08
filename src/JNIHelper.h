/**
 *  @author        Ryan Huang <ryanhuang@cs.ucsd.edu>
 *  @organization  University of California, San Diego
 * 
 * JNI helper functions
 *
 */

#ifndef __JNI_HELPER_H__
#define __JNI_HELPER_H__

#include <jni.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sstream>
#include <memory>
#include <mutex>

#include <string>
#include <stdexcept>
#include <map>

#define CTORNAME "<init>"

#define JTHROWABLE_CLS "java/lang/Throwable"
#define JRUNTIMEEXCEPT_CLS "RuntimeException"

#define JRUNTIMEEXCEPT_CTOR "(java/lang/String;)V"

// FIXME: The location of these JARs should be specified using a config file.  We should also
// be more flexible about release numbers.
// Defines for classpath additions needed by liballuxio.  If the locations of these files
// don't match in your environment, you'll either have to set the CLASSPATH env var, or modify
// these defines to point to the correct location
#define CLASSPATH_ALLUXIO_JAR "/home/condor/alluxio/core/client/runtime/target/alluxio-core-client-runtime-1.6.1-jar-with-dependencies.jar"
#define CLASSPATH_SLF4J_JAR "$HOME/slf4j/slf4j-nop-1.7.21.jar"

// Jackson JARs are required if you plan to configure an S3 underfs
#define CLASSPATH_JACKSON_JARS "$HOME/jackson/jackson-annotations-2.8.1.jar:$HOME/jackson/jackson-core-2.8.1.jar:$HOME/jackson/jackson-databind-2.8.1.jar"

#define JVM_BUF_LEN   1 
#define MAX_CLS_SIG 256
#define MAX_EXCEPT_MSG_LEN 256

// jvm type signature
// reference: http://docs.oracle.com/javase/7/docs/technotes/guides/jni/spec/types.html#wp9502

#define J_BOOL   'Z'
#define J_BYTE   'B'
#define J_CHAR   'C'
#define J_SHORT  'S'
#define J_INT    'I'
#define J_LONG   'J'
#define J_FLOAT  'F'
#define J_DOUBLE 'D'
#define J_VOID   'V'
#define J_OBJ    'L'
#define J_ARRAY  '['

namespace tdms { namespace jni {

/**
 * simple pthread_mutex wrapper
 */
class Mutex { 

public:
  Mutex()
  {
    pthread_mutex_init(&m_mutex, NULL);
  }

  ~Mutex()
  {
    pthread_mutex_destroy(&m_mutex);
  }

  int lock()
  {
    return pthread_mutex_lock(&m_mutex);
  }

  int unlock()
  {
    return pthread_mutex_unlock(&m_mutex);
  }

private:
  pthread_mutex_t m_mutex;
};

/**
 * A wrapper for jthrowable into a C++ exception
 */
class JavaThrowable {

public:
  JavaThrowable(JNIEnv *env, jthrowable except)
  {
    m_env = env;
    m_except = (jthrowable) env->NewGlobalRef(except);
  }

  ~JavaThrowable()
  {
    m_env->DeleteGlobalRef(m_except);
  }

  jthrowable getException()
  {
    return (jthrowable) m_env->NewLocalRef(m_except);
  }

  void printStackTrace() const;
  bool getStackTrace(std::string &out);

private:
  JNIEnv *m_env;
  jthrowable m_except;
};

/**
 * Exception that would be thrown while setting up JNI calls,
 * e.g., cannot find a class definition.
 *
 */
class NativeException: public std::exception {
public:
  NativeException() {}
  NativeException(const char *msg, JavaThrowable *detail = NULL)
                    : m_detail(detail), m_msg(msg), m_DetailedMsg(toString()) {}

  /**
   * Get the original Java exception that was thrown.
   *
   * If no such exception was kept, return NULL.
   */
  JavaThrowable* detail() const { return m_detail; }

  /**
   * Dump the exception information to stderr.
   */
  void dump() const
  {
    fprintf(stderr, "Message: %s\nFrom Java: ", m_msg.c_str());
    printDetailStackTrace();
  }

  /**
   * Dump the exception information to a string
   */
  std::string toString() const;

  /**
   * Discard this exception. Mainly clean up the original Java exception.
   */
  void discard()
  {
    if (m_detail != NULL) {
      delete m_detail;
    }
  }

  /**
   * Print the stack trace.
   */
  void printDetailStackTrace() const
  {
    if (m_detail != NULL) {
      m_detail->printStackTrace();
    }
  }

  /**
   * Returns the explanatory string
   */
  const char* what() const noexcept
  {
     return m_DetailedMsg.c_str();
  }

  // We don't free the m_detail in destructor because the exception
  // might be re-thrown, and we don't want to destroy the detail.
  // It is up to the final exception handler to free up the exception
  // detail.
  ~NativeException() throw() {}

protected:
  JavaThrowable *m_detail;
  std::string m_msg;
  std::string m_DetailedMsg;
  std::string m_stack_trace;
};

class ClassNotFoundException: public NativeException {
public:
  ClassNotFoundException(const char *className, 
          JavaThrowable *detail = NULL); 
};

class MethodNotFoundException: public NativeException {
public:
  MethodNotFoundException(const char *className, const char *methodName,
          JavaThrowable *detail = NULL); 
};

class FieldNotFoundException: public NativeException {
public:
  FieldNotFoundException(const char *className, const char *fieldName,
          JavaThrowable *detail = NULL); 
};

class NewGlobalRefException: public NativeException {
public:
  NewGlobalRefException(const char *refName,
          JavaThrowable *detail = NULL); 
};

class NewObjectException: public NativeException {
public:
  NewObjectException(const char *className,
          JavaThrowable *detail = NULL); 
};

class NewEnumException: public NativeException {
public:
  NewEnumException(const char *className, const char *valueName,
          JavaThrowable *detail = NULL); 
};

class Env;
class ClassCache;
class JNIHelper;

class Env {

public:
  Env();
  Env(JNIEnv *env): m_env(env) {}
  Env(Env const & copy): m_env(copy.m_env) {}

  // make it be able to cast to JNIEnv* 
  operator JNIEnv *() const { return m_env; }
  JNIEnv *operator ->() { return m_env; }
  JNIEnv *get() {return m_env; }

  // simple FindClass wrapper with exception checking
  jclass findClass(const char *className);

  // similar to findClass except that the result would be cached
  // inside GlobalClassCache
  jclass findClassAndCache(const char *className);

  jstring newStringUTF(const char *bytes, const char *err_desc);
  jobject newGlobalRef(jobject obj);
  jbyteArray newByteArray(jsize length);

  void deleteGlobalRef(jobject obj);
  void deleteLocalRef(jobject obj);

  // get method id in a java class
  jmethodID getMethodId(const char *className, const char *methodName, 
                        const char *methodSignature);
  jmethodID getStaticMethodId(const char *className, const char *methodName, 
                        const char *methodSignature);

  jmethodID getMethodId(jclass cls, const char *methodName, 
                        const char *methodSignature);
  jmethodID getStaticMethodId(jclass cls, const char *methodName, 
                        const char *methodSignature);

  // create a new object for a class
  jobject newObject(const char *className, const char *ctorSignature, ...);
  jobject newObject(jclass cls, const char *className, const char *ctorSignature, ...);

  // get the filesystem object
  jobject getFs();

  // get the FileWriteLocationPolicy
  jobject getLocationPolicy(char* myLocationPolicy, char* targethost);

 
  // get the jobject for a enum value
  jobject getEnumObject(const char *className, const char * valueName, const char * objType);

  // create a runtime exception
  jthrowable newRuntimeException(const char *message);

  // get a method's return type based on the signature
  bool getMethodRetType(char * rettOut, const char *methodSignature);

  bool getClassName(jclass cls, jobject instance, std::string& nameStr);
  bool jstringToString(jstring str, std::string& cStr);
  char *jstringToChar(jstring str);
  bool throwableToString(jthrowable except, std::string& exceptStr);

  /** Exception related methods **/
  bool hasException();
  void checkException();
  void checkExceptionAndClear();
  void checkExceptionAndAbort();
  void checkExceptionAndPrint();

  #define GET_JNI_X_FIELD(R, T)                                                   \
  R get##T##Field(jobject obj, const char *fieldName,                             \
                    const char *fieldSignature)                                   \
  {                                                                               \
    jclass cls = m_env->GetObjectClass(obj);                                      \
    jfieldID fid = m_env->GetFieldID(cls, fieldName, fieldSignature);             \
    R ret = m_env->Get##T##Field(obj, fid);                                       \
    jthrowable except = m_env->ExceptionOccurred();                               \
    if (except) {                                                                 \
      m_env->ExceptionClear();                                                    \
      std::string nameStr;                                                        \
      if (getClassName(cls, obj, nameStr)) {                                      \
        throw FieldNotFoundException(nameStr.c_str(), fieldName,                  \
                  new JavaThrowable(m_env, except));                              \
      } else {                                                                    \
        throw FieldNotFoundException("unknown class", fieldName,                  \
                  new JavaThrowable(m_env, except));                              \
      }                                                                           \
    }                                                                             \
    return ret;                                                                   \
  }

  GET_JNI_X_FIELD(jboolean, Boolean)
  GET_JNI_X_FIELD(jbyte, Byte)
  GET_JNI_X_FIELD(jchar, Char)
  GET_JNI_X_FIELD(jshort, Short)
  GET_JNI_X_FIELD(jint, Int)
  GET_JNI_X_FIELD(jlong, Long)
  GET_JNI_X_FIELD(jdouble, Double)
  GET_JNI_X_FIELD(jobject, Object)

  #undef GET_JNI_X_FIELD

  // invoke a method of a java class
  void callMethod(jvalue *retOut, jobject obj, const char *methodName, 
                  const char * methodSignature, ...);

  // invoke a static method of a java class
  void callStaticMethod(jvalue *retOut, const char *className, 
                  const char *methodName, const char * methodSignature, ...);

  // use macro concatenation to call the exact type of jni method and set
  // the return value (union) appropriately

  #define CALL_JNI_X_METHOD(R, T, F)                                            \
  inline void call##T##Method(jvalue *retOut,                                   \
          jobject obj, jmethodID mid, va_list args)                             \
  {                                                                             \
    R ret = m_env->Call##T##MethodV(obj, mid, args);                            \
    checkExceptionAndClear();                                                   \
    if (retOut != NULL) {                                                       \
      retOut->F = ret;                                                          \
    }                                                                           \
  }
    
  // refer to JNI specification for the encodings
  CALL_JNI_X_METHOD(jboolean, Boolean, z)
  CALL_JNI_X_METHOD(jbyte, Byte, b)
  CALL_JNI_X_METHOD(jchar, Char, c)
  CALL_JNI_X_METHOD(jshort, Short, s)
  CALL_JNI_X_METHOD(jint, Int, i)
  CALL_JNI_X_METHOD(jlong, Long, j)
  CALL_JNI_X_METHOD(jfloat, Float, f)
  CALL_JNI_X_METHOD(jdouble, Double, d)
  CALL_JNI_X_METHOD(jobject, Object, l)

  #undef CALL_JNI_X_METHOD

  #define CALL_STATIC_JNI_X_METHOD(R, T, F)                                     \
  void callStatic##T##Method(jvalue *retOut,                                    \
                  jclass cls, jmethodID mid, va_list args)                      \
  {                                                                             \
    R ret = m_env->CallStatic##T##MethodV(cls, mid, args);                      \
    checkExceptionAndClear();                                                   \
    if (retOut != NULL) {                                                       \
      retOut->F = ret;                                                          \
    }                                                                           \
  }

  // refer to JNI specification for the encodings
  CALL_STATIC_JNI_X_METHOD(jboolean, Boolean, z)
  CALL_STATIC_JNI_X_METHOD(jbyte, Byte, b)
  CALL_STATIC_JNI_X_METHOD(jchar, Char, c)
  CALL_STATIC_JNI_X_METHOD(jshort, Short, s)
  CALL_STATIC_JNI_X_METHOD(jint, Int, i)
  CALL_STATIC_JNI_X_METHOD(jlong, Long, j)
  CALL_STATIC_JNI_X_METHOD(jfloat, Float, f)
  CALL_STATIC_JNI_X_METHOD(jdouble, Double, d)
  CALL_STATIC_JNI_X_METHOD(jobject, Object, l)

  #undef CALL_STATIC_JNI_X_METHOD

  // void method is a bit different, define them separately
  inline void callVoidMethod(jobject obj, jmethodID mid, va_list args)
  {
    m_env->CallVoidMethodV(obj, mid, args);
    checkExceptionAndClear();
  }

  inline void callStaticVoidMethod(jclass cls, jmethodID mid, va_list args)
  {
    m_env->CallStaticVoidMethodV(cls, mid, args);
    checkExceptionAndClear();
  }

  inline void AttachCurrentThread() {
      JavaVM *jvm = nullptr;
      m_env->GetJavaVM(&jvm);
      jvm->AttachCurrentThread((void**)&m_env, NULL);
  }

  inline bool GetIsThreadDetached() {
      JavaVM *jvm = nullptr;
      m_env->GetJavaVM(&jvm);
      int getEnvStat = jvm->GetEnv((void **)&m_env, JNI_VERSION_1_6);
      return (getEnvStat == JNI_EDETACHED);
  }

  inline void DetachCurrentThread() {
      JavaVM *jvm = nullptr;
      m_env->GetJavaVM(&jvm);
      jvm->DetachCurrentThread();
  }

private:
  jobject newObjectV(jclass cls, const char *className, const char *ctorSignature, 
                      va_list args);
  jmethodID getMethodId(jclass cls, const char *className, const char *methodName, 
                        const char *methodSignature, bool isStatic);
  void callMethodV(jvalue *retOut, jobject obj, const char *className, 
                    const char *methodName, const char * methodSignature, 
                    bool isStatic, va_list args);

private:

  JNIEnv *m_env;
};

#define ENV_CHECK_CLEAR(expr)      \
  do {                             \
      (expr);                      \
      checkExceptionAndClear();    \
  } while (0)

/**
 * Cache for holding JNI class resolution results. 
 * Upon caching, a global reference will be created to hold the class.
 */
class ClassCache {
public: 
  ClassCache(JNIEnv *env): m_env(env) {}
  ~ClassCache();
  static ClassCache* instance(JNIEnv *env);

  jclass get(const char *); 
  bool set(const char *, jclass);

private:
  Env m_env;
  std::map<const char *, jclass>  m_cls_map;
  Mutex m_lock;
};

/**
 * A singleton class that maintains a list of ClassCache objects
 */
class ClassCaches {
public:
  static ClassCaches &getInstance() {
    static ClassCaches instance;
    return instance;
  }

  ClassCache* getCache(JNIEnv *env);

private:
  std::map<JNIEnv *, std::shared_ptr<ClassCache> > m_caches;
  std::mutex m_cache_lock;
};

/**
 * A singleton helper class to obtain the JNI env and facilitate other common
 * JNI routines 
 */
class JNIHelper {

public:
  static JNIHelper& get() {
    static JNIHelper instance;
    return instance;
  }
  ~JNIHelper() {}

  JNIEnv* getEnv();
  void printThrowableStackTrace(JNIEnv *env, jthrowable exce);
  bool getThrowableStackTrace(JNIEnv *env, jthrowable exce, std::string &out);

private:
  JNIHelper() {}
  JNIHelper(JNIHelper const &);
  void operator=(JNIHelper const &);

  Mutex m_env_lock;
}; // class JNIHelper

}} // namespace Tachyon::JNI

#endif /* __JNI_HELPER_H_ */

/* vim: set ts=4 sw=4 : */
