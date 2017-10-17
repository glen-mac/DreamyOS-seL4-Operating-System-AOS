/*
 * Copyright 2014, NICTA
 *
 * This software may be distributed and modified according to the terms of
 * the BSD 2-Clause license. Note that NO WARRANTY is provided.
 * See "LICENSE_BSD2.txt" for details.
 *
 * @TAG(NICTA_BSD)
 */
#ifndef _UTILS_TIME_H
#define _UTILS_TIME_H

/* seconds */
#define SEC_IN_MINUTE 60llu
#define NS_IN_MINUTE (SEC_IN_MINUTE*NS_IN_S
#define SEC_TO_MS(t) (t * MS_IN_S)

/* Calculate number of microseconds for a given time of seconds and milliseconds */
#define SECONDS(t) (t * US_IN_S)
#define MILLISECONDS(t) (t * US_IN_MS)

/* Calculate number of milliseconds given microseconds */
#define U_TO_MS(t) (t / US_IN_MS)

/* milliseconds */
#define MS_IN_S 1000llu

/* microseconds */
#define US_IN_MS 1000llu
#define US_IN_S  1000000llu
#define US_TO_MS(t) (t / US_IN_MS)

/* nanoseconds */
#define NS_IN_US 1000llu
#define NS_IN_MS 1000000llu
#define NS_IN_S  1000000000llu

/* picoseconds */
#define PS_IN_NS 1000llu
#define PS_IN_US 1000000llu
#define PS_IN_MS 1000000000llu
#define PS_IN_S  1000000000000llu

/* femptoseconds */
#define FS_IN_PS 1000llu
#define FS_IN_NS 1000000llu
#define FS_IN_US 1000000000llu
#define FS_IN_MS 1000000000000llu
#define FS_IN_S  1000000000000000llu

#endif /* _UTILS_TIME_H */
