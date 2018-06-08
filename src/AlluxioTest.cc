/**
 *  @author        Adam Storm <ajstorm@ca.ibm.com>
 *  @organization  IBM
 * 
 * Sample test of TDMS C/C++ APIs
 *
 */

#include "Alluxio.h"
#include "Util.h"

#include <stdlib.h>
#include <string.h>
//FIXME: This is using a mix of C and C++ IO right now.  Convert to all
// C++
#include <fstream>
#include <iostream>
#include <ctime>
#include <iomanip>
#include <chrono>
#include <functional>
#include <thread>
#include "JNIHelper.h"


using namespace tdms;

const char * program_name;

const char *masterUri;
const char *filePath;

const char *gFileToCreate = "/hello.txt";
const char *gDirToCreate = "/alluxiotest";
const char *gPathSeparatorString = "/";
const char gPathSeparatorChar = '/';

// FIXME: Change to allow keys to be entered via a config file
const char *awsAccessKey = "-----ACCESS-KEY-----";
const char *awsSecretKey = "------------SECRET-KEY------------------";

void usage()
{
   std::cerr << "Usage: " << program_name << " masterHost masterPort [file]" 
      << std::endl;
}

std::string inputFileToTDMSPath (char* file)
{
    const char *fileName;
    // Find the file name in the supplied path by looking for the last separator
    fileName = strrchr(file, gPathSeparatorChar);
    if (fileName == NULL)
    {
        // Handle the case where the supplied path is local to the directory
        fileName = file;
    }
    else
    {
        // move past the last separator since we're explicitly adding it below
        fileName++;
    }

    std::string alluxioPath(gDirToCreate);

    alluxioPath.append(gPathSeparatorString);
    alluxioPath.append(fileName);

    return alluxioPath;
}

void testCreateDirectory(jTDMSFileSystem client, const char *path)
{
  printf("TEST - CREATE DIRECTORY : \n");
  client->createDirectory(path);
  printf("SUCCESS - Created alluxio dir %s \n",path);
}

void testDirectoryExists(jTDMSFileSystem client, const char *path)
{
  //std::cout << std::endl << "TEST - DIRECTORY EXISTS: ";
   printf("TEST - DIRECTORY EXISTS: \n");
  if(client->exists(path))
    printf("SUCCESS - TDMS dir %s exists \n",path);
  else
   printf("Failed - TDMS dir %s does'nt exists \n",path);
  //std::cout << "SUCCESS - TDMS dir " << path << " exists" << std::endl;
}

void testGetFileSize(jTDMSFileSystem client, const char *path)
{
  std::cout << std::endl << "TEST - GET FILE SIZE: ";
  long int size = client->fileSize(path);
  std::cout << "SUCCESS - File " << path << " has size " << size << " bytes" << std::endl;
}

void testSeek(jTDMSFileSystem client, const char *path)
{
  std::cout << std::endl << "TEST - FILE SEEK: ";
  const long int bufSize = 10;
  const long int seekPos = 5;
  char buf[bufSize];
  jFileInStream fileInStream = client->openFile(path);

  try
  {
      fileInStream->seek(seekPos);
      int rdSz = fileInStream->read(buf, bufSize-1);
      buf[rdSz] = '\0';
      std::cout << "SUCCESS - Content of " << path << std::endl << "starting at position " << seekPos 
          << " is: \"" << buf << "\"" << std::endl;
  }
  catch (...)
  {
      fileInStream->close();
      throw;
  }

  fileInStream->close();
}

void testSkip(jTDMSFileSystem client, const char *path)
{
  std::cout << std::endl << "TEST - FILE SKIP: ";
  const long int bufSize = 5;
  const long int skipBytes = 5;
  char buf[bufSize];
  jFileInStream fileInStream = client->openFile(path);

  try
  {
      // Read from the beginning of the file, then skip and read some more
      int rdSz = fileInStream->read(buf, bufSize-1);
      buf[rdSz] = '\0';
      std::cout << std::endl << "Reading " << bufSize << " bytes from the beginning of " << path  
          << ":" << std::endl << "\"" << buf << "\"" << std::endl;
      long int bytesSkipped = fileInStream->skip(skipBytes);
      if (bytesSkipped != skipBytes)
      {
          std::cout << "FAILURE - Failed to skip " << skipBytes << " bytes. rc from skip: " << bytesSkipped<< std::endl;
      }
      else
      {
          rdSz = fileInStream->read(buf, bufSize-1);
          buf[rdSz] = '\0';
          std::cout << "Skipping " << skipBytes << " bytes " << std::endl;
          std::cout << "Post skip, buffer contains: \"" << buf << "\"" << std::endl;
      }
  }
  catch (...)
  {
      fileInStream->close();
      throw;
  }

  fileInStream->close();
}

jFileOutStream testCreateFileWithOptions(jTDMSFileSystem client, const char *path)
{
  std::cout << std::endl << "TEST - CREATE FILE WITH OPTIONS: ";
  jFileOutStream fileOutStream;
  bool fileExists = false;
  TDMSCreateFileOptions* options = TDMSCreateFileOptions::getCreateFileOptions();
  printf("\n");
  options->setDataAccessPattern("PIPELINE","cn17662");
  options->setDataAccessPattern("PIPELINE");
  options->setDataAccessPattern("SCATTER");
  options->setDataAccessPattern("GATHER");
  options->setDataAccessPattern("GATHER","cn17662");
  options->setDataAccessPattern("MULTICAST");
  options->setDataAccessPattern("UNOWN");
  //options->setWriteType(CACHE_THROUGH);

  fileOutStream = client->createFile(path, options);

  std::cout << "SUCCESS - Created alluxio file: " << path << std::endl;

  return fileOutStream;
}

void testLsCommand(jTDMSFileSystem client, const std::string &expectedPath,
                   ListPathFilter filter) {
  std::cout << std::endl << "TEST - LS COMMAND: ";

  std::vector<std::string> files = client->listPath("/", filter);
  bool foundPath = false;
  for(const std::string& i : files) {
      foundPath |= (i.compare(expectedPath) == 0);
  }

  if (foundPath) {
      std::cout << "SUCCESS - ls command found the path " << expectedPath << std::endl;
  }
  else
  {
      std::cout << "ERROR - did not find " << expectedPath << std::endl;
  }
}

jFileOutStream testCreateFile(jTDMSFileSystem client, const char *path)
{
  std::cout << std::endl << "TEST - CREATE FILE: ";
  jFileOutStream fileOutStream;
  bool fileExists = false;

  fileOutStream = client->createFile(path);

  if (fileOutStream == NULL) 
  {
     std::cout << "FAILED - Failed to create alluxio file: " << path << std::endl;
  }
  else
  {
      std::cout << "SUCCESS - Created alluxio file: " << path << std::endl;
  }

  return fileOutStream;
}

void testReadLargeFile(jTDMSFileSystem client, jFileInStream fileInStream, 
      const char* path, char* inputBuffer, int bufferSize)
{
  std::cout << std::endl << "TEST - READ LARGE FILE: ";
  std::chrono::duration<double> duration = std::chrono::duration<double>::zero();
  std::chrono::duration<double> elapsedTime = std::chrono::duration<double>::zero();
  std::chrono::duration<double> bufferCreationTime = std::chrono::duration<double>::zero();
  std::chrono::duration<double> alluxioReadTime = std::chrono::duration<double>::zero();
  std::chrono::duration<double> bufferCopyTime = std::chrono::duration<double>::zero();
  std::chrono::time_point<std::chrono::system_clock> startTime, stopTime;
  TDMSOpenFileOptions* openOptions = TDMSOpenFileOptions::getOpenFileOptions();
  int bytesRead = bufferSize;
  const bool measureTime = true;

  //openOptions->setReadType(CACHE_PROMOTE);
  
  // Now do the reading from TDMS
  startTime = std::chrono::system_clock::now();
  fileInStream = client->openFile(path, openOptions);
  stopTime = std::chrono::system_clock::now();
  duration = stopTime - startTime;
  std::cout << "Opened file " << path << " from TDMS in " << duration.count() 
     << " seconds" << std::endl;

  if (fileInStream == NULL) 
  {
     std::cout << "failed to open alluxio file " << path << std::endl;
     goto exit;
  }
  else
  {
      std::cout << "successfully opened file: " << path << " for reading" << std::endl;
  }

  elapsedTime = std::chrono::duration<double>::zero();

  while (bytesRead == bufferSize)
  {
     startTime = std::chrono::system_clock::now();
     bytesRead = fileInStream->read(
           inputBuffer, bufferSize, 0, bufferSize, measureTime,
           &bufferCreationTime, &alluxioReadTime, &bufferCopyTime);
     stopTime = std::chrono::system_clock::now();
     elapsedTime += stopTime - startTime;
  }

  std::cout << bufferCreationTime.count() 
     << " seconds creating buffers" << std::endl << alluxioReadTime.count() << 
     " seconds reading buffers" << std::endl << bufferCopyTime.count() << 
     " seconds gaining access to buffers"<< std::endl;

  fileInStream->close();

  std::cout << "Read complete" << std::endl;
  std::cout << "Spent " << elapsedTime.count() << " seconds reading from TDMS" << std::endl;
  delete (fileInStream); 

exit:
  return;
}

void testCopyFile(jTDMSFileSystem client, const char *inPath, const char *alluxioPath)
{
  std::cout << std::endl << "TEST - COPY FILE: ";
  const char *fileNameInInpath;
  jFileOutStream targetOutStream;
  jFileInStream  fileInStream;
  bool fileExists = false;
  std::ifstream inputFile;
  char *inputBuffer;
  const int bufferSize = 1000000;
  int bytesRead = bufferSize;
  TDMSCreateFileOptions* createOptions = NULL;
  TDMSOpenFileOptions* openOptions = NULL;
  std::chrono::duration<double> duration = std::chrono::duration<double>::zero();
  std::chrono::duration<double> elapsedTimeReading = std::chrono::duration<double>::zero();
  std::chrono::duration<double> elapsedTimeWriting = std::chrono::duration<double>::zero();
  std::chrono::time_point<std::chrono::system_clock> startTime, stopTime;

  std::cout.precision(15);

  TDMSClientContext localContext;
  TDMSFileSystem afs(localContext);

  createOptions = TDMSCreateFileOptions::getCreateFileOptions();
  createOptions->setDataAccessPattern("PIPELINE");
  //createOptions->setWriteType(CACHE_THROUGH);

  targetOutStream = afs.createFile(alluxioPath, createOptions);

  startTime = std::chrono::system_clock::now();
  inputFile.open(inPath, std::ios::in | std::ios::binary);
  stopTime = std::chrono::system_clock::now();
  duration = stopTime - startTime;
  std::cout << "Opened file " << inPath << " on disk in " << duration.count() 
     << " seconds" << std::endl;

  // FIXME: Change to std::vector
  inputBuffer = (char *) calloc(bufferSize, 1);

  if (!inputBuffer)
  {
      throw std::bad_alloc();
  }

  while (bytesRead == bufferSize)
  {
     startTime = std::chrono::system_clock::now();

     inputFile.read(inputBuffer, bufferSize);

     stopTime = std::chrono::system_clock::now();

     duration = stopTime - startTime;

     elapsedTimeReading += duration;

     bytesRead = inputFile.gcount();

     startTime = std::chrono::system_clock::now();

     targetOutStream->write(inputBuffer, bytesRead);

     stopTime = std::chrono::system_clock::now();

     duration = stopTime - startTime;

     elapsedTimeWriting += duration;
  }

  startTime = std::chrono::system_clock::now();
  targetOutStream->close();
  stopTime = std::chrono::system_clock::now();
  elapsedTimeWriting += stopTime - startTime;

  std::cout << "File copy complete" << std::endl;
  std::cout << "Spent " << elapsedTimeReading.count() 
     << " seconds reading from disk" << std::endl;
  std::cout << "Spent " << elapsedTimeWriting.count() 
     << " seconds writing to TDMS" << std::endl;
  delete(targetOutStream);

  // Now do the reading from TDMS
  testReadLargeFile(&afs, fileInStream, alluxioPath, inputBuffer, bufferSize);

  free(inputBuffer);

  return;
}

void testAppendFile(jTDMSFileSystem client, const char* path, char* appendString)
{
    std::cout << std::endl << "TEST - APPEND FILE: ";
    // TODO: This is all common code (from testCopyFile).  It should be encapsulated
    //client->appendToFile(path, appendString, strlen(appendString));
    jFileOutStream targetOutStream;
    TDMSCreateFileOptions* options = TDMSCreateFileOptions::getCreateFileOptions();
    //options->setWriteType(CACHE_THROUGH);
    
    targetOutStream = client->openFileForAppend(path, options);

    // Note that this strlen test only works if the append string is a string.  If it's a byte buffer
    // it could get truncated in the middle.
    targetOutStream->write(appendString, strlen(appendString));

    client->completeAppend(path, targetOutStream);

    std::cout << "Successfully appended: \n\"" << appendString << "\" to file " <<
        path << std::endl;
}

void testWriteFile(jFileOutStream fileOutStream)
{
  std::cout << std::endl << "TEST - WRITE FILE: ";
  char content[] = "hello, alluxio!!";
   //Write 2GB file test
  char testbyte[1024];
  std::chrono::duration<double> duration = std::chrono::duration<double>::zero();
  std::chrono::time_point<std::chrono::system_clock> startTime, stopTime;
  
  int i,j,k=0;
  for(i=0;i<1024;i++)
    testbyte[i]='0';
  fileOutStream->write(content, strlen(content));
  std::cout << "len of testbyte is " << strlen(testbyte) << std::endl;
  startTime = std::chrono::system_clock::now();
  for(i=0;i<1024;i++)
    for(j=0;j<2048;j++)
      fileOutStream->write(testbyte, strlen(testbyte));
  stopTime =  std::chrono::system_clock::now();
  duration = stopTime - startTime;
  std::cout << "Write 2GB to TDMS in " << duration.count()
     << " seconds" << std::endl;
  
  std::cout << "SUCCESS - Wrote \"" << content << "\" to file" << std::endl;
}

jFileInStream testOpenFile(jTDMSFileSystem client, const char *path)
{
  std::cout << std::endl << "TEST - OPEN FILE: ";
  char *rpath;
  jFileInStream fileInStream = client->openFile(path);
  std::cout << "SUCCESS - Opened file: " << path << std::endl;
  return fileInStream;
}

void testReadFile(jFileInStream fileInStream)
{
  std::cout << std::endl << "TEST - READ FILE: ";
  char buf[32];
  int rdSz = fileInStream->read(buf, 31);
  buf[rdSz] = '\0';
  std::cout << "SUCCESS - Content of the created file:" << buf << std::endl;
}

void testDeleteFile(jTDMSFileSystem client, const char *path, bool recursive)
{
  std::cout << std::endl << "TEST - DELETE FILE: ";
  client->deletePath(path, recursive);
  std::cout << "SUCCESS - Deleted path " << path << std::endl;
}


int main(int argc, char*argv[])
{
  printf("Starting liballuxio Test \n");
  program_name = argv[0];
  char* file = argv[1];
  try {
      //TDMSClientContext::connect();
      TDMSClientContext acc;
      printf("Context successed \n");
      TDMSFileSystem stackFS(acc);
      jTDMSFileSystem client = &stackFS;
      printf("Init jTDMSFileSystem  successed \n");
           
      //Test the creation of a file with the default options
      jFileOutStream fileOutStream;
      fileOutStream = testCreateFile(client, gFileToCreate);

      // Close and delete created file
      fileOutStream->close();
      
      delete fileOutStream;
      testDeleteFile(client, gFileToCreate, false);

      // Test the creation of a file with non-default options
      fileOutStream = testCreateFileWithOptions(client, gFileToCreate);

      // Write to newly created file
      testWriteFile(fileOutStream);

      fileOutStream->close();
      delete fileOutStream; 

      // Test Append to file
      testAppendFile(client, gFileToCreate, "Test append str");

      // Open file for reading
      jFileInStream fileInStream = testOpenFile(client, gFileToCreate);

      // Read from file
      testReadFile(fileInStream);
      fileInStream->close(); 
      delete fileInStream; 
      
      // Do an ls to see the file we created
      testLsCommand(client, gFileToCreate, ListPathFilter::NONE);
      
      // Test file deletion
      testDeleteFile(client, gFileToCreate, false);
      
      // Create a new directory
      testCreateDirectory(client, gDirToCreate);
      
      // Test directory creation
      testDirectoryExists(client, gDirToCreate);

      // Test file deletion
      testDeleteFile(client, gDirToCreate, true);


      
      // Test directory shows up in ls
      testLsCommand(client, gDirToCreate, ListPathFilter::DIRECTORIES_ONLY);

      if (file != NULL)
      {
          std::string alluxioFile = inputFileToTDMSPath(file);

          // FIXME: Make number of threads a command line option
          const int numThreads = 1;
          std::vector<std::thread> threads;
          std::vector<std::string> fileNames;

          // Copy a variable number of files to TDMS (in parallel)
          for (int i=0; i<numThreads; i++)
          {
              fileNames.push_back(std::string(alluxioFile + "." + std::to_string(i)));
              threads.push_back(std::thread(testCopyFile, client, file, fileNames.back().c_str()));
          }

          for (int j=0; j<numThreads; j++)
          {
              threads.back().join();
              threads.pop_back();
          }

          // Copy a single file to TDMS (required for append test below)
          testCopyFile(client, file, alluxioFile.c_str());

          // Append to file
          testAppendFile(client, alluxioFile.c_str(), "ASDAFASDFASDF");

          // Test seek
          testSeek(client, alluxioFile.c_str());

          // Test skip
          testSkip(client, alluxioFile.c_str());

          // Get file size
          testGetFileSize(client, alluxioFile.c_str());
	
      }
      
      // Test path deletion
      testDeleteFile(client, gDirToCreate, true);

  } catch (const jni::NativeException &e) {
    e.dump();
  }
  return 0;
}

/* vim: set ts=4 sw=4 : */
