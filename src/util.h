/*
 * The Homework Database
 *
 * Authors:
 *    Oliver Sharma and Joe Sventek
 *     {oliver, joe}@dcs.gla.ac.uk
 *
 * (c) 2009. All rights reserved.
 */
#ifndef OCLIB_UTIL_H
#define OCLIB_UTIL_H

#include "config.h"
#include "logdefs.h"

#include <stdio.h>

/* -------- [MESSAGE] -------- */
#ifdef NMSG
#define MSG (void)
#else
#define MSG printf("DBSERVER> "); printf
#endif

#endif
