/*
 * File: multi-lookup.h
 * Author: Christopher Costello
 * Project: CSCI 3753 Programming Assignment 2
 * Create Date: 2014/02/18
 * Modify Date: 2014/02/18
 * Description:
 * 	This file contains the reference non-threaded
 *      solution to this assignment.
 *  
 */

#ifndef MULTI_H
#define MULTI_H
 
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <semaphore.h>

#include "util.h"
#include "queue.h"


/* Read in name files*/
void* readUrl(void* file);
/* Resolve Ip's and write results */
void* resolve(void* tid);
/* Initialize variables */
void initialize();
/* Destroy and clean up*/
void cleanUp();

#endif
