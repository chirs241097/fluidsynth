/* FluidSynth - A Software Synthesizer
 *
 * Copyright (C) 2003  Peter Hanappe and others.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License
 * as published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA
 */


/**

   This header contains a bunch of (mostly) system and machine
   dependent functions:

   - timers
   - current time in milliseconds and microseconds
   - debug logging
   - profiling
   - memory locking
   - checking for floating point exceptions

 */

#ifndef _FLUID_SYS_H
#define _FLUID_SYS_H

#include "fluidsynth_priv.h"


/**
 * Macro used for safely accessing a message from a GError and using a default
 * message if it is NULL.
 * @param err Pointer to a GError to access the message field of.
 * @return Message string
 */
#define fluid_gerror_message(err)
//#define fluid_gerror_message(err)  ((err) ? err->message : "No error details")


void fluid_sys_config(void);
void fluid_log_config(void);
void fluid_time_config(void);


/* Misc */

#define fluid_return_val_if_fail(exp,val) if(!(exp))return val
#define fluid_return_if_fail(exp)         if(!(exp))return
#define FLUID_INLINE                      inline
#define FLUID_POINTER_TO_UINT(a)          ((uint32_t)(uint64_t)(a))
#define FLUID_UINT_TO_POINTER(a)          ((void*)(uint64_t)(a))
#define FLUID_POINTER_TO_INT(a)           ((int)(uint64_t)(a))
#define FLUID_INT_TO_POINTER(a)           ((void*)(long)(a))
#define FLUID_N_ELEMENTS(struct)          (sizeof (struct) / sizeof (struct[0]))

#ifdef __GNUC__
#define FLUID_IS_BIG_ENDIAN               (__BYTE_ORDER__==__ORDER_BIG_ENDIAN__)
#else
#define FLUID_IS_BIG_ENDIAN               0
#endif

/*
 * Utility functions
 */
char *fluid_strtok (char **str, char *delim);


/**

  Additional debugging system, separate from the log system. This
  allows to print selected debug messages of a specific subsystem.
 */

extern unsigned int fluid_debug_flags;

#if DEBUG

enum fluid_debug_level {
  FLUID_DBG_DRIVER = 1
};

int fluid_debug(int level, char * fmt, ...);

#else
#define fluid_debug
#endif


#if defined(__OS2__)
#define INCL_DOS
#include <os2.h>

typedef int socklen_t;
#endif

unsigned int fluid_curtime(void);
double fluid_utime(void);


/**
    Timers

 */

/* if the callback function returns 1 the timer will continue; if it
   returns 0 it will stop */
typedef int (*fluid_timer_callback_t)(void* data, unsigned int msec);

typedef struct _fluid_timer_t fluid_timer_t;

fluid_timer_t* new_fluid_timer(int msec, fluid_timer_callback_t callback,
                               void* data, int new_thread, int auto_destroy,
                               int high_priority);

int delete_fluid_timer(fluid_timer_t* timer);
int fluid_timer_join(fluid_timer_t* timer);
int fluid_timer_stop(fluid_timer_t* timer);

/* Muteces */

#ifdef HAVE_STDTHREADS_H //C11 Threads

/* Regular mutex */

typedef mtx_t fluid_mutex_t;
#define FLUID_MUTEX_INIT          { 0 }
#define fluid_mutex_init(_m)      mtx_init (&(_m), mtx_plain)
#define fluid_mutex_destroy(_m)   mtx_destroy (&(_m))
#define fluid_mutex_lock(_m)      mtx_lock(&(_m))
#define fluid_mutex_unlock(_m)    mtx_unlock(&(_m))

/* Recursive lock capable mutex */
typedef mtx_t fluid_rec_mutex_t;
#define fluid_rec_mutex_init(_m)      mtx_init(&(_m), mtx_plain | mtx_recursive)
#define fluid_rec_mutex_destroy(_m)   mtx_destroy(&(_m))
#define fluid_rec_mutex_lock(_m)      mtx_lock(&(_m))
#define fluid_rec_mutex_unlock(_m)    mtx_unlock(&(_m))

/* Dynamically allocated mutex suitable for fluid_cond_t use */
typedef mtx_t    fluid_cond_mutex_t;
#define fluid_cond_mutex_lock(m)        mtx_lock(m)
#define fluid_cond_mutex_unlock(m)      mtx_unlock(m)

#else //C++11 Threads bridged to C

/* Regular mutex */
typedef _mutex fluid_mutex_t;
#define FLUID_MUTEX_INIT          { 0 }
#define fluid_mutex_init(_m)      _mutex_init (&(_m))
#define fluid_mutex_destroy(_m)   _mutex_clear (&(_m))
#define fluid_mutex_lock(_m)      _mutex_lock(&(_m))
#define fluid_mutex_unlock(_m)    _mutex_unlock(&(_m))

/* Recursive lock capable mutex */
typedef _rec_mutex fluid_rec_mutex_t;
#define fluid_rec_mutex_init(_m)      _rec_mutex_init(&(_m))
#define fluid_rec_mutex_destroy(_m)   _rec_mutex_clear(&(_m))
#define fluid_rec_mutex_lock(_m)      _rec_mutex_lock(&(_m))
#define fluid_rec_mutex_unlock(_m)    _rec_mutex_unlock(&(_m))

/* Dynamically allocated mutex suitable for fluid_cond_t use */
typedef _mutex    fluid_cond_mutex_t;
#define fluid_cond_mutex_lock(m)        _mutex_lock(m)
#define fluid_cond_mutex_unlock(m)      _mutex_unlock(m)

#endif

static FLUID_INLINE fluid_cond_mutex_t *
new_fluid_cond_mutex (void)
{
  _mutex *mutex;
  mutex = FLUID_NEW (_mutex);
  _mutex_init (mutex);
  return (mutex);
}

static FLUID_INLINE void
delete_fluid_cond_mutex (fluid_cond_mutex_t *m)
{
  _mutex_clear (m);
  free (m);
}

#ifdef HAVE_STDTHREADS_H //C11 Threads

/* Thread condition signaling */
typedef cnd_t fluid_cond_t;
#define fluid_cond_signal(cond)         cnd_signal(cond)
#define fluid_cond_broadcast(cond)      cnd_broadcast(cond)
#define fluid_cond_wait(cond, mutex)    cnd_wait(cond, mutex)

static FLUID_INLINE fluid_cond_t *
new_fluid_cond (void)
{
  _cond *cond;
  cond = FLUID_NEW (_cond);
  cnd_init (cond);
  return (cond);
}

static FLUID_INLINE void
delete_fluid_cond (fluid_cond_t *cond)
{
  cnd_destroy (cond);
  free (cond);
}

/* Thread private data */

typedef tss_t fluid_private_t;
#define fluid_private_init(_priv)                  tss_create (&_priv, free)
#define fluid_private_free(_priv)                  tss_delete (_priv)
#define fluid_private_get(_priv)                   tss_get(_priv)
#define fluid_private_set(_priv, _data)            tss_set(_priv, _data)

#else //C++11 Threads bridged to C

/* Thread condition signaling */
typedef _cond fluid_cond_t;
#define fluid_cond_signal(cond)         _cond_signal(cond)
#define fluid_cond_broadcast(cond)      _cond_broadcast(cond)
#define fluid_cond_wait(cond, mutex)    _cond_wait(cond, mutex)

static FLUID_INLINE fluid_cond_t *
new_fluid_cond (void)
{
  _cond *cond;
  cond = FLUID_NEW (_cond);
  _cond_init (cond);
  return (cond);
}

static FLUID_INLINE void
delete_fluid_cond (fluid_cond_t *cond)
{
  _cond_clear (cond);
  free (cond);
}

/* Thread private data */

typedef _private fluid_private_t;
#define fluid_private_init(_priv)                  memset (&_priv, 0, sizeof(_priv))
#define fluid_private_free(_priv)
#define fluid_private_get(_priv)                   _private_get(&_priv)
#define fluid_private_set(_priv, _data)            _private_set(&_priv, _data)

#endif

/* Atomic operations */

#define fluid_atomic_int_inc(_pi) atomic_fetch_add(_pi, 1)
#define fluid_atomic_int_add(_pi, _val) atomic_fetch_add(_pi, _val)
#define fluid_atomic_int_get(_pi) atomic_load(_pi)
#define fluid_atomic_int_set(_pi, _val) atomic_store(_pi, _val)
#define fluid_atomic_int_dec_and_test(_pi) atomic_fetch_add(_pi, -1)
#define fluid_atomic_int_compare_and_exchange(_pi, _old, _new) \
  atomic_compare_exchange_weak(_pi, _old, _new)


#define fluid_atomic_int_exchange_and_add(_pi, _add) \
  atomic_fetch_add(_pi, _add)

#define fluid_atomic_pointer_get(_pp)           atomic_load(_pp)
#define fluid_atomic_pointer_set(_pp, val)      atomic_store(_pp, val)
#define fluid_atomic_pointer_compare_and_exchange(_pp, _old, _new) \
  atomic_compare_exchange_weak(_pp, _old, _new)

static FLUID_INLINE void
fluid_atomic_float_set(volatile float *fptr, float val)
{
  sint32 ival;
  memcpy (&ival, &val, 4);
  fluid_atomic_int_set ((volatile int *)fptr, ival);
}

static FLUID_INLINE float
fluid_atomic_float_get(volatile float *fptr)
{
  sint32 ival;
  float fval;
  ival = fluid_atomic_int_get ((volatile int *)fptr);
  memcpy (&fval, &ival, 4);
  return fval;
}


/* Threads */
#ifdef HAVE_STDTHREADS_H //C11 Threads

typedef thrd_t fluid_thread_t;
typedef int (*fluid_thread_func_t)(void* data);

#define FLUID_THREAD_ID_NULL            NULL                    /* A NULL "ID" value */
#define fluid_thread_id_t               thrd_t *                /* Data type for a thread ID */
#define fluid_thread_get_id()           &thrd_current()         /* Get unique "ID" for current thread */
#define fluid_thread_usleep(us)         thrd_sleep(&(struct timespec){.tv_nsec=us*1000},NULL)

#else //C++11 Threads bridged to C

typedef _thread fluid_thread_t;
typedef int (*fluid_thread_func_t)(void* data);

#define FLUID_THREAD_ID_NULL            NULL                    /* A NULL "ID" value */
#define fluid_thread_id_t               _thread *               /* Data type for a thread ID */
#define fluid_thread_get_id()           _thread_get_id()        /* Get unique "ID" for current thread */
#define fluid_thread_usleep(us)         _thread_sleep(us)

#endif

fluid_thread_t* new_fluid_thread(const char *name, fluid_thread_func_t func, void *data,
                                 int prio_level, int detach);
void delete_fluid_thread(fluid_thread_t* thread);
void fluid_thread_self_set_prio (int prio_level);
int fluid_thread_join(fluid_thread_t* thread);

/* Sockets and I/O */

fluid_istream_t fluid_get_stdin (void);
fluid_ostream_t fluid_get_stdout (void);
int fluid_istream_readline(fluid_istream_t in, fluid_ostream_t out, char* prompt, char* buf, int len);
int fluid_ostream_printf (fluid_ostream_t out, char* format, ...);

/* The function should return 0 if no error occured, non-zero
   otherwise. If the function return non-zero, the socket will be
   closed by the server. */
typedef int (*fluid_server_func_t)(void* data, fluid_socket_t client_socket, char* addr);

fluid_server_socket_t* new_fluid_server_socket(int port, fluid_server_func_t func, void* data);
int delete_fluid_server_socket(fluid_server_socket_t* sock);
int fluid_server_socket_join(fluid_server_socket_t* sock);
void fluid_socket_close(fluid_socket_t sock);
fluid_istream_t fluid_socket_get_istream(fluid_socket_t sock);
fluid_ostream_t fluid_socket_get_ostream(fluid_socket_t sock);



/* Profiling */


/**
 * Profile numbers. List all the pieces of code you want to profile
 * here. Be sure to add an entry in the fluid_profile_data table in
 * fluid_sys.c
 */
enum {
  FLUID_PROF_WRITE,
  FLUID_PROF_ONE_BLOCK,
  FLUID_PROF_ONE_BLOCK_CLEAR,
  FLUID_PROF_ONE_BLOCK_VOICE,
  FLUID_PROF_ONE_BLOCK_VOICES,
  FLUID_PROF_ONE_BLOCK_REVERB,
  FLUID_PROF_ONE_BLOCK_CHORUS,
  FLUID_PROF_VOICE_NOTE,
  FLUID_PROF_VOICE_RELEASE,
  FLUID_PROF_LAST
};


#if WITH_PROFILING

void fluid_profiling_print(void);


/** Profiling data. Keep track of min/avg/max values to execute a
    piece of code. */
typedef struct _fluid_profile_data_t {
  int num;
  char* description;
  double min, max, total;
  unsigned int count;
} fluid_profile_data_t;

extern fluid_profile_data_t fluid_profile_data[];

/** Macro to obtain a time refence used for the profiling */
#define fluid_profile_ref() fluid_utime()

/** Macro to create a variable and assign the current reference time for profiling.
 * So we don't get unused variable warnings when profiling is disabled. */
#define fluid_profile_ref_var(name)     double name = fluid_utime()

/** Macro to calculate the min/avg/max. Needs a time refence and a
    profile number. */
#define fluid_profile(_num,_ref) { \
  double _now = fluid_utime(); \
  double _delta = _now - _ref; \
  fluid_profile_data[_num].min = _delta < fluid_profile_data[_num].min ? _delta : fluid_profile_data[_num].min; \
  fluid_profile_data[_num].max = _delta > fluid_profile_data[_num].max ? _delta : fluid_profile_data[_num].max; \
  fluid_profile_data[_num].total += _delta; \
  fluid_profile_data[_num].count++; \
  _ref = _now; \
}


#else

/* No profiling */
#define fluid_profiling_print()
#define fluid_profile_ref()  0
#define fluid_profile_ref_var(name)
#define fluid_profile(_num,_ref)

#endif



/**

    Memory locking

    Memory locking is used to avoid swapping of the large block of
    sample data.
 */

#if defined(HAVE_SYS_MMAN_H) && !defined(__OS2__)
#define fluid_mlock(_p,_n)      mlock(_p, _n)
#define fluid_munlock(_p,_n)    munlock(_p,_n)
#else
#define fluid_mlock(_p,_n)      0
#define fluid_munlock(_p,_n)
#endif


/**

    Floating point exceptions

    fluid_check_fpe() checks for "unnormalized numbers" and other
    exceptions of the floating point processsor.
*/
#ifdef FPE_CHECK
#define fluid_check_fpe(expl) fluid_check_fpe_i386(expl)
#define fluid_clear_fpe() fluid_clear_fpe_i386()
#else
#define fluid_check_fpe(expl)
#define fluid_clear_fpe()
#endif

unsigned int fluid_check_fpe_i386(char * explanation_in_case_of_fpe);
void fluid_clear_fpe_i386(void);

#endif /* _FLUID_SYS_H */
