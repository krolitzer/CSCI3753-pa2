/*
 * File: multi-lookup.c
 * Author: Christopher Costello
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2014/02/18
 * Modify Date: 2014/02/18
 * Description:
 *  This file contains the reference non-threaded
 *      solution to this assignment.
 *  
 */
#include "multi-lookup.h"
#define NUM_THREADS 3
#define USAGE "<inputFilePath> <outputFilePath>"
#define MINARGS 3
#define SBUFSIZE 1025
#define INPUTFS "%1024s"

queue request;
pthread_mutex_t queue_mutex;
pthread_mutex_t write_mutex;
pthread_mutex_t counter_mutex;

int resolvingThreads;
int readCount;
int writeCount;

void* readUrl(void* file) {
  pthread_mutex_lock(&counter_mutex);
  resolvingThreads++;
  pthread_mutex_unlock(&counter_mutex);

  FILE* inputfp = NULL;
  char* hostname = malloc(SBUFSIZE*sizeof(char));
  int qp;

  /* Check if file exists */
  inputfp = fopen((char *)file, "r");
  // Couldn't open file
  if(!inputfp) {
    fprintf(stderr,"Error Opening Input File: %s\n", (char *) file);
    //Clean up after the failed file
    pthread_mutex_lock(&counter_mutex);
    resolvingThreads--;
    pthread_mutex_unlock(&counter_mutex);
    free(hostname);
    return NULL; 
  }

  /* Read File */
  while(fscanf(inputfp, INPUTFS, hostname) > 0) {
    // Duplicate copy of hostname to put in queue
    char* name = strdup(hostname);

    // Push hostname to queue.
    pthread_mutex_lock(&queue_mutex);
    qp = queue_push(&request, (void*) name);
    pthread_mutex_unlock(&queue_mutex);


    // Try again if queue is full
    while(qp == QUEUE_FAILURE){
      pthread_mutex_lock(&queue_mutex);
      qp = queue_push(&request, (void*) name);
      pthread_mutex_unlock(&queue_mutex);
      if (qp == QUEUE_FAILURE) {
        usleep(rand()%100);
      }
    }

    if(qp == QUEUE_FAILURE){
      printf("WARNING: Couldn't push to queue %s\n", hostname);
    } 

    pthread_mutex_lock(&counter_mutex);
    readCount++;
    pthread_mutex_unlock(&counter_mutex);
  }

  pthread_mutex_lock(&counter_mutex);
  resolvingThreads--;
  pthread_mutex_unlock(&counter_mutex);

  free(hostname);
  fclose(inputfp);
  return NULL;
}

void* resolve(void* outputFile) {

  char* hostname;
  char firstipstr[INET6_ADDRSTRLEN];

  /* Open Output File */
  FILE* outputfp = NULL;
  outputfp = fopen((char*) outputFile, "a");
  if(!outputfp) {
    perror("Error Opening Output File\n");
    return NULL;
  }

  while(1) {
    /* Grab hostname off the queue */
    pthread_mutex_lock(&queue_mutex);
    hostname = queue_pop(&request);
    pthread_mutex_unlock(&queue_mutex);

    // Only resolve successful pops.
    if (hostname != NULL) {
      if(dnslookup(hostname, firstipstr, sizeof(firstipstr))
          == UTIL_FAILURE) {
        fprintf(stderr, "dnslookup error: %s\n", hostname);
        strncpy(firstipstr, "", sizeof(firstipstr));
      }
      /* Write ip name to file */ 
      pthread_mutex_lock(&write_mutex);
      fprintf(outputfp, "%s,%s\n", hostname, firstipstr);
      writeCount++;
      pthread_mutex_unlock(&write_mutex);
      free(hostname);
      // No more resolvingThreads requester threads.
    } else if(resolvingThreads == 0){
      break;
    }
  }
  free(hostname);
  fclose(outputfp);
  return NULL;
}

void initialize() {
  int maxSize = queue_init(&request, -1);
  resolvingThreads = 0;
  readCount = 0;
  writeCount = 0;
  int qm = pthread_mutex_init(&queue_mutex, NULL);
  int wm = pthread_mutex_init(&write_mutex, NULL);
  int qf = pthread_mutex_init(&counter_mutex, NULL);
  if(qm) {
    printf("pthread mutex init didn't work with error code %d\n", qm);
  }
  if(wm) {
    printf("pthread mutex init didn't work with error code %d\n", wm);
  }
  if(qf) {
    printf("pthread mutex init didn't work with error code %d\n", wm);
  }
  if(maxSize != 50){
    printf("Error creating the queue\n");
  }
}

int main(int argc, char* argv[]) {
  int num_inFiles = argc - 2;

  /* Check Usage */
  if (argc < MINARGS) {
    fprintf(stderr, "Not enough arguments: %d\n", (argc - 1));
    fprintf(stderr, "Usage:\n %s %s\n", argv[0], USAGE);
    return EXIT_FAILURE;
  }
  FILE* output_test = fopen(argv[argc-1], "w");
  if(!output_test) {
    fprintf(stderr, "ERROR: Invalid output file. Exiting\n");
    exit(-1);
  }
  fclose(output_test);
  initialize();

  /* One thread per file */
  pthread_t requester_threads[num_inFiles];

  pthread_t resolver_threads[NUM_THREADS];

  int rc;
  long t;
  void* status;

  /* Start requester threads */
  for(t=0; t<num_inFiles; t++) {
    rc = pthread_create(&requester_threads[t], NULL, readUrl, (void *)argv[t+1]);
    if(rc) {
      fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
    }
  }

  /* Start resolver threads*/
  for(t=0; t<NUM_THREADS; t++) {
    rc = pthread_create(&resolver_threads[t], NULL, resolve, (void *)argv[argc-1]);
    if(rc) { 
      fprintf(stderr, "ERROR; return code from pthread_create() is %d\n", rc);
    }
  }

  /* Join the requester threads */
  for(t=0; t<num_inFiles; t++) {
    rc = pthread_join(requester_threads[t], &status);
    if (rc) {
      fprintf(stderr, "ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }

  /* Join the resolver threads */
  for(t=0; t<NUM_THREADS; t++) {
    rc = pthread_join(resolver_threads[t], &status);
    if (rc) {
      fprintf(stderr, "ERROR; return code from pthread_join() is %d\n", rc);
      exit(-1);
    }
  }
  printf("Read %d names from files\n", readCount);
  printf("Wrote %d names to file\n", writeCount);

  queue_cleanup(&request);  
  pthread_mutex_destroy(&queue_mutex);
  pthread_mutex_destroy(&write_mutex);
  pthread_mutex_destroy(&counter_mutex);
  pthread_exit(NULL);

  printf("Main: program completed. Exiting.\n");
  return 0;
}
