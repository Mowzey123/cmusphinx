/* ====================================================================
 * Copyright (c) 1999-2004 Carnegie Mellon University.  All rights
 * reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * This work was supported in part by funding from the Defense Advanced 
 * Research Projects Agency and the National Science Foundation of the 
 * United States of America, and the CMU Sphinx Speech Consortium.
 *
 * THIS SOFTWARE IS PROVIDED BY CARNEGIE MELLON UNIVERSITY ``AS IS'' AND 
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY
 * NOR ITS EMPLOYEES BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
/*
 * uttproc.c -- Process utterance.
 * 
 * HISTORY
 * 
 * 30-Nov-2005	M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon University
 * 		Added acoustic confidence scoring (in search_hyp_t.conf).
 * 		(Currently, needs compute-all-senones for this to work.)
 * 
 * $Log$
 * Revision 1.30  2006/02/25  01:18:56  egouvea
 * Sync'ing wiht SphinxTrain.
 * 
 * Added the flag "-seed". If dither is being used and the seed is less
 * than zero, the random number generator is initialized with time(). If
 * it is at least zero, it's initialized with the provided seed. This way
 * we have the benefit of having dither, and the benefit of being
 * repeatable.
 * 
 * This is consistent with what sphinx3 does. Well, almost. The random
 * number generator is still what the compiler provides.
 * 
 * Also, moved fe_init_params to fe_interface.c, so one can initialize a
 * variable of type param_t with meaningful values.
 * 
 * Revision 1.29  2006/02/17 00:49:58  egouvea
 * Yet another attempt at synchronizing the front end code between
 * SphinxTrain and sphinx2.
 *
 * Added support for warping functions.
 *
 * Replaced some fprintf() followed by exit() with E_WARN and return() in
 * functions that had a non void return type.
 *
 * Set return value to FE_ZERO_ENERGY_ERROR if the energy is zero in a
 * frame, allowing the application to do something (currently, uttproc
 * and raw2cep simply print a message.
 *
 * Warning: the return value in fe_process_utt() and fe_end_utt()
 * required a change in the API (the return value has a different meaning
 * now).
 *
 * Revision 1.28  2006/02/09 22:48:38  egouvea
 * Fixed computation of max file size allowed. It was using FRAME_RATE
 * instead of FRAME_SHIFT (i.e. SAMPLING_RATE / FRAME_RATE) to compute
 * max number of samples from max number of frames.
 *
 * Revision 1.27  2006/01/25 14:43:24  rkm
 * *** empty log message ***
 *
 * Revision 1.26  2005/12/13 17:04:14  rkm
 * Added confidence reporting in nbest files; fixed some backtrace bugs
 *
 * Revision 1.25  2005/12/03 17:54:34  rkm
 * Added acoustic confidence scores to hypotheses; and cleaned up backtrace functions
 *
 * Revision 1.24  2005/11/01 23:30:53  egouvea
 * Replaced explicit assignments with memcpy
 *
 * Revision 1.23  2005/10/11 13:08:40  dhdfu
 * Change the default FFT size for 8kHz to 512, as that is what Communicator models are.  Add command-line arguments to specify all FE parameters, thus removing the 8 or 16kHz only restriction.  Add default parameters for 11025Hz as well
 *
 * Revision 1.22  2005/08/18 22:56:05  egouvea
 * Fixed a bug in which the last frame, when running in live mode, was
 * ignored by uttproc_end_utt.
 *
 * Revision 1.21  2005/05/24 20:55:24  rkm
 * Added -fsgbfs flag
 *
 * Revision 1.20  2005/01/20 00:09:43  egouvea
 * Replace subtraction of elements in FILETIME structure with a subtraction of 64 bit integers, and removed some warnings about type casting
 *
 * Revision 1.19  2004/12/10 16:48:57  rkm
 * Added continuous density acoustic model handling
 *
 * 
 * 22-Nov-2004  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon Univ.
 *              Modified to use senscr module for senone score computation.
 * 
 * 18-Nov-2004  M K Ravishankar (rkm@cs.cmu.edu) at Carnegie Mellon Univ.
 *              uttproc_feat2rawfr() is buggy; set it to ignore comp2rawfr[].
 * 
 * Revision 1.18  2004/11/13 00:38:44  egouvea
 * Replaced most printf with E_INFO (or E_WARN or...). Changed the output
 * of the time_align code so it's consistent with the other decoder modes
 * (allphone, normal decoding etc). Added the file utt id to the
 * time_align output.
 *
 * Revision 1.17  2004/07/23 23:36:34  egouvea
 * Ravi's merge, with the latest fixes in the FSG code, and making the log files generated by FSG, LM, and allphone have the same 'look and feel', with the backtrace information presented consistently
 *
 * Revision 1.8  2004/07/20 20:48:40  rkm
 * Added uttproc_load_fsg().
 *
 * Revision 1.7  2004/07/20 13:40:55  rkm
 * Added FSG get/set start/final state functions.
 *
 * Revision 1.6  2004/07/16 19:55:28  rkm
 * Added state information to hypothesis.
 *
 * Revision 1.16  2004/07/16 00:57:12  egouvea
 * Added Ravi's implementation of FSG support.
 *
 * Revision 1.5  2004/07/07 13:56:33  rkm
 * Added reporting of (acoustic score - best senone score)/frame
 *
 * Revision 1.4  2004/06/22 15:35:46  rkm
 * Added partial result reporting options in batch mode
 *
 * Revision 1.3  2004/06/16 17:48:03  rkm
 * Imported pscr-based stuff from fbs_main.c
 *
 * Revision 1.2  2004/05/27 14:22:57  rkm
 * FSG cross-word triphones completed (but for single-phone words)
 *
 * Revision 1.2  2004/03/02 15:33:39  rkm
 * FSG bug fixes
 *
 * Revision 1.14  2004/03/02 04:10:14  rkm
 * FSG bugfix: need to get senscores every utt
 *
 * Revision 1.13  2004/03/01 20:30:56  rkm
 * *** empty log message ***
 *
 * Revision 1.12  2004/03/01 20:21:33  rkm
 * *** empty log message ***
 *
 * Revision 1.11  2004/02/27 21:01:25  rkm
 * Many bug fixes in multiple FSGs
 *
 * Revision 1.10  2004/02/27 19:33:01  rkm
 * *** empty log message ***
 *
 * Revision 1.9  2004/02/27 16:15:13  rkm
 * Added FSG switching
 *
 * Revision 1.8  2004/02/27 15:05:21  rkm
 * *** empty log message ***
 *
 * Revision 1.7  2004/02/26 01:14:48  rkm
 * *** empty log message ***
 *
 * Revision 1.6  2004/02/25 15:08:19  rkm
 * *** empty log message ***
 *
 * Revision 1.5  2004/02/24 18:13:05  rkm
 * Added NULL transition handling
 *
 * Revision 1.4  2004/02/23 15:53:45  rkm
 * Renamed from fst to fsg
 *
 * Revision 1.3  2004/02/23 15:09:50  rkm
 * *** empty log message ***
 *
 * Revision 1.2  2004/02/19 21:16:54  rkm
 * Added fsg_search.{c,h}
 *
 * Revision 1.1.1.1  2003/12/03 20:05:04  rkm
 * Initial CVS repository
 *
 * Revision 1.15  2001/12/11 00:24:48  lenzo
 * Acknowledgement in License.
 *
 * Revision 1.14  2001/12/07 20:32:59  lenzo
 * No unistd.h on Windows.
 *
 * Revision 1.13  2001/12/07 17:46:00  lenzo
 * Un-ifdef the include for <unistd.h>
 *
 * Revision 1.12  2001/12/07 17:30:02  lenzo
 * Clean up and remove extra lines.
 *
 * Revision 1.11  2001/12/07 05:09:30  lenzo
 * License.xsxc
 *
 * Revision 1.10  2001/12/07 04:27:35  lenzo
 * License cleanup.  Remove conditions on the names.  Rationale: These
 * conditions don't belong in the license itself, but in other fora that
 * offer protection for recognizeable names such as "Carnegie Mellon
 * University" and "Sphinx."  These changes also reduce interoperability
 * issues with other licenses such as the Mozilla Public License and the
 * GPL.  This update changes the top-level license files and removes the
 * old license conditions from each of the files that contained it.
 * All files in this collection fall under the copyright of the top-level
 * LICENSE file.
 *
 * Revision 1.9  2001/10/23 22:20:30  lenzo
 * Change error logging and reporting to the E_* macros that call common
 * functions.  This will obsolete logmsg.[ch] and they will be removed
 * or changed in future versions.
 *
 * Revision 1.8  2001/03/31 00:56:12  lenzo
 * Added <string.h>
 *
 * Revision 1.6  2001/01/25 19:36:28  lenzo
 * Fixing some memory leaks
 *
 * Revision 1.5  2000/12/21 18:04:51  lenzo
 * Fixed a nasty (but small) FRAME_RATE error.  This will need cleanup later.
 *
 * Revision 1.4  2000/12/12 23:01:42  lenzo
 * Rationalizing libs and names some more.  Split a/d and fe libs out.
 *
 * Revision 1.3  2000/12/05 01:45:12  lenzo
 * Restructuring, hear rationalization, warning removal, ANSIfy
 *
 * Revision 1.2  2000/02/08 20:44:32  lenzo
 * Changed uttproc_allphone_cepfile() to uttproc_allphone_file.
 *
 * Revision 1.1.1.1  2000/01/28 22:08:58  lenzo
 * Initial import of sphinx2
 *
 * 09-Jan-00    Kevin Lenzo <lenzo@cs.cmu.edu> at Carnegie Mellon
 *              Altered to accomodate new fe lib.
 * 
 * 30-Oct-98    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Changed rawlogfile mode to READONLY (WIN32).
 * 
 * 10-Sep-98    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Reset uttno to 0 whenever uttproc_set_auto_uttid_prefix() is called.
 * 
 * 10-Sep-98    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added uttproc_allphone_cepfile(), and minor modifications to support it.
 * 
 * 20-Aug-98    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Bugfix: 
 *              Added call to agc_emax_update() inside uttproc_end_utt().  Added call
 *              to initialize AGC with a reasonable value.
 * 
 * 20-Apr-98    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added uttproc_set_auto_uttid_prefix().
 * 
 * 11-Apr-98    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added AGC_NONE test to determining livemode in uttproc_begin_utt().
 *              Added memcpy to mfc2feat_live if AGC_NONE (bugfix).
 * 
 * 22-Jul-97    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added sampling rate spec in call to fe_init.
 * 
 * 27-May-97    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added uttprocSetcomp2rawfr() and uttprocGetcomp2rawfr() functions
 *              implemented by Bob Brennan for maintaining multiple lattices.
 * 
 * 04-Apr-97    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added dictwd_in_lm() check in uttproc_set_context.
 * 
 * 30-Oct-96    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Commented out call to search_dump_lattice_ascii.
 *              Added feature vector padding in mfc2feat_batch ().
 * 
 * 17-Jun-96    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added uttproc_set_context().
 * 
 * 04-Jun-96    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added BLOCKING option to uttproc_rawdata, uttproc_cepdata, uttproc_result.
 * 
 * 24-May-96    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Substantially modified to be driven with externally provided data, rather
 *                      than explicitly reading an A/D source.
 *              Added uttproc_abort_utt() and uttproc_partial_result().
 *              Added raw and mfc logging function.
 * 
 * 17-Nov-95    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added function uttproc_lmupdate().
 * 
 * 17-Nov-95    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Fixed bug in uttproc_feat2rawfr that could return feat2rawfr[-1].
 * 
 * 29-Sep-95    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added -matchsegfn argument and processing.
 * 
 * 17-Sep-95    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added autonumbering of utterances (typically used in live mode).
 * 
 * 02-Jul-95    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added allphone handling.
 * 
 * 13-Jun-95    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Simplified the uttproc interface by combining functions and redefining
 *              others.
 * 
 * 01-Jun-95    M K Ravishankar (rkm@cs) at Carnegie Mellon University
 *              Added uttproc_set_lm() and uttproc_set_startword().
 */

/*
 * BUGS:
 *   - Instead of using query_fwdtree_flag() to determine which first pass to run
 *     (tree or flag), there should be an explicit uttproc_set_firstpass() call.
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef WIN32
#include <io.h>
#include <windows.h>
#include <time.h>
#include <sys/stat.h>
#else
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>
#endif

#include <assert.h>
#include <string.h>

#include "s2types.h"
#include "CM_macros.h"
#include "basic_types.h"
#include "err.h"
#include "scvq.h"
#include "senscr.h"
#include "search_const.h"
#include "msd.h"
#include "strfuncs.h"
#include "linklist.h"
#include "list.h"
#include "hash.h"
#include "dict.h"
#include "lmclass.h"
#include "lm_3g.h"
#include "kb.h"
#include "cdcn.h"
#include "fe.h"
#include "fbs.h"
#include "search.h"
#include <fsg_search.h>


#define MAX_UTT_LEN     6000    /* #frames */
#define MAX_CEP_LEN     (MAX_UTT_LEN*CEP_SIZE)
#define MAX_POW_LEN     (MAX_UTT_LEN*POW_SIZE)

typedef enum {UTTSTATE_UNDEF=-1,
              UTTSTATE_IDLE=0,
              UTTSTATE_BEGUN=1,
              UTTSTATE_ENDED=2,
              UTTSTATE_STOPPED=3
} uttstate_t;
static uttstate_t uttstate = UTTSTATE_UNDEF;

static int32 inputtype;
#define INPUT_UNKNOWN   0
#define INPUT_RAW       1
#define INPUT_MFC       2

static int32 livemode;          /* Iff TRUE, search while input being supplied.  In this
                                   case, CMN, AGC and silence compression cannot be
                                   utterance based */
static int32 utt_ofl;           /* TRUE iff buffer limits overflowed in current utt */
static int32 nosearch = 0;
static int32 fsg_search_mode = FALSE;   /* Using FSM search structure */

/* MFC vectors for entire utt */
static float **mfcbuf;
static int32 n_rawfr;           /* #raw frames before compression or feature computation */

/* Feature vectors for entire utt */
static float *cep_buf = NULL;
static float *dcep_buf;
static float *dcep_80ms_buf;
static float *pcep_buf;
static float *ddcep_buf;
static int32 n_featfr;          /* #features frames */
static int32 n_compfr;          /* #compressed frames */
static int32 n_searchfr;
static int16 *comp2rawfr;       /* Compressed frame no. to original raw frame no. */

static int32 pow_i, cep_i;      /* #feature frames total in current utt so far */
static int32 search_cep_i, search_pow_i;        /* #frames already searched */

/* CMN, AGC, silence compression; default values */
static scvq_norm_t cmn = NORM_UTT;
static scvq_agc_t agc = AGC_MAX;
static scvq_compress_t silcomp = COMPRESS_NONE;

static FILE *matchfp = NULL;
static FILE *matchsegfp = NULL;

static char *rawlogdir = NULL;
static char *mfclogdir = NULL;
static FILE *rawfp = NULL;
static FILE *mfcfp = NULL;
static char rawfilename[4096];

static int32 samp_hist[5];      /* #Samples in 0-4K, 4K-8K, 8K-16K, 16K-24K, 24K-32K */
static int32 max_samp;

static char *uttid;
static char *uttid_prefix = NULL;
#define UTTIDSIZE       4096
static int32 uttno;     /* A running sequence number assigned to every utterance.  Used as
                           an id for an utterance if uttid is undefined. */

static CDCN_type cdcn;

static float TotalCPUTime, TotalElapsedTime, TotalSpeechTime;

#ifdef WIN32
static float e_start, e_stop;
static HANDLE pid;
static FILETIME t_create, t_exit, kst, ket, ust, uet;
static double lowscale, highscale;
extern double win32_cputime();
#else
static struct rusage start, stop;
static struct timeval e_start, e_stop;
#endif


/* FIXME: These are all internal to this module, but still should go
   into internal header files... */

/* live_norm.c */
extern void mean_norm_init(int32 vlen);
extern void mean_norm_update(void);
extern void mean_norm_acc_sub(float *vec);
extern int32 cepmean_set (float *vec);
extern int32 cepmean_get (float *vec);

/* agc_emax.c */
void agc_emax_update ( void );
extern int32 agcemax_set (double m);
extern double agcemax_get ( void );
extern int agc_emax_proc (float *ocep, float const *icep, int veclen);

/* norm.c */
void norm_mean (float *vec, int32 nvec, int32 veclen);

/* r_agc_noise.c */
extern int32 delete_background (float *cep, int32 fcnt,
                                int32 cf_cnt, double thresh);
extern float histo_noise_level (float *cep, int32 fcnt, int32 cf_cnt);
extern int32 histo_add_c0 (float c0);
void compute_noise_level (void);
void real_agc_noise(float *cep,
                    register int32 fcnt,
                    register int32 cf_cnt);
void agc_max(float *cep,
             register int32 fcnt,
             register int32 cf_cnt);

/* searchlat.c */
void searchlat_set_rescore_lm (char const *lmname);

static fsg_search_t *fsg_search;


#ifdef WIN32

/* The FILETIME manual page says: "It is not recommended that you add
 * and subtract values from the FILETIME structure to obtain relative
 * times."
 */
double win32_cputime (FILETIME *st, FILETIME *et)
{
    double dt;
    ULARGE_INTEGER l_st = *(ULARGE_INTEGER *)st;
    ULARGE_INTEGER l_et = *(ULARGE_INTEGER *)et;
    LONGLONG ltime;

    ltime = l_et.QuadPart - l_st.QuadPart;

    dt = (ltime * lowscale);

    return (dt);
}

#else

double MakeSeconds (struct timeval const *s, struct timeval const *e)
/*------------------------------------------------------------------------*
 * Compute an elapsed time from two timeval structs
 */
{
    return ((e->tv_sec - s->tv_sec) + ((e->tv_usec - s->tv_usec) / 1000000.0));
}

#endif

/*
 * One time initialization
 */
static void timing_init ( void )
{
#ifdef WIN32
    lowscale = 1e-7;
    highscale = 65536.0 * 65536.0 * lowscale;

    pid = GetCurrentProcess();
#endif

    TotalCPUTime = TotalElapsedTime = TotalSpeechTime = 0.0;
}

/*
 * Start of each utterance
 */
static void timing_start ( void )
{
#ifndef WIN32
#ifndef _HPUX_SOURCE
    getrusage (RUSAGE_SELF, &start);
#endif
    gettimeofday (&e_start, 0);
#else /* WIN32 */
    e_start = (float)clock()/CLOCKS_PER_SEC;
    GetProcessTimes (pid, &t_create, &t_exit, &kst, &ust);
#endif /* WIN32 */
}

/*
 * End of each utterance
 */
static void timing_stop (int32 nfr)
{
    if (nfr <= 0)
        return;
    
    E_INFO(" %5.2f SoS", searchFrame()*0.01);
    TotalSpeechTime += searchFrame()*0.01f;
    
#ifdef WIN32
    /* ---------------- WIN32 ---------------- */
    e_stop = (float)clock()/CLOCKS_PER_SEC;
    GetProcessTimes (pid, &t_create, &t_exit, &ket, &uet);
    
    E_INFOCONT(", %6.2f sec elapsed", (e_stop - e_start));
    E_INFOCONT(", %5.2f xRT", (e_stop - e_start)/(searchFrame()*0.01));
    E_INFOCONT(", %6.2f sec CPU", win32_cputime(&ust, &uet));
    E_INFOCONT(", %5.2f xRT", win32_cputime(&ust, &uet)/(searchFrame()*0.01));
    
    TotalCPUTime += (float) win32_cputime(&ust, &uet);
    TotalElapsedTime += (e_stop - e_start);
#else
    /* ---------------- Unix ---------------- */
#ifndef _HPUX_SOURCE
    getrusage (RUSAGE_SELF, &stop);
#endif
    gettimeofday (&e_stop, 0);
    
    E_INFOCONT(", %6.2f sec elapsed", MakeSeconds (&e_start, &e_stop));
    E_INFOCONT(", %5.2f xRT", MakeSeconds (&e_start, &e_stop)/(searchFrame()*0.01));
    
#ifndef _HPUX_SOURCE
    E_INFOCONT(", %6.2f sec CPU", MakeSeconds (&start.ru_utime, &stop.ru_utime));
    E_INFOCONT(", %5.2f xRT",
            MakeSeconds (&start.ru_utime, &stop.ru_utime)/(searchFrame()*0.01));
#endif
    
    TotalCPUTime += MakeSeconds (&start.ru_utime, &stop.ru_utime);
    TotalElapsedTime += MakeSeconds (&e_start, &e_stop);
#endif
    
    E_INFOCONT("\n\n");
}

/*
 * One time cleanup before exiting program
 */
static void timing_end ( void )
{
    E_INFO("\n");

    E_INFO("TOTAL Elapsed time %.2f seconds\n",TotalElapsedTime);
#ifndef _HPUX_SOURCE
    E_INFO("TOTAL CPU time %.2f seconds\n", TotalCPUTime);
#endif
    E_INFO("TOTAL Speech %.2f seconds\n", TotalSpeechTime);

    if (TotalSpeechTime > 0.0) {
        E_INFO("AVERAGE %.2f xRT(Elapsed)", TotalElapsedTime/TotalSpeechTime);
#ifndef _HPUX_SOURCE
        E_INFOCONT(", %.2f xRT(CPU)", TotalCPUTime/TotalSpeechTime);
#endif
        E_INFOCONT("\n");
    }
}

static void feat_alloc ( void )
{
    int32 k;
    
    if (! cep_buf) {
        cep_buf       = (float *) CM_calloc (MAX_CEP_LEN, sizeof(float));
        dcep_buf      = (float *) CM_calloc (MAX_CEP_LEN, sizeof(float));
        dcep_80ms_buf = (float *) CM_calloc (MAX_CEP_LEN, sizeof(float));
        pcep_buf      = (float *) CM_calloc (MAX_POW_LEN, sizeof(float));
        ddcep_buf     = (float *) CM_calloc (MAX_CEP_LEN, sizeof(float));

        mfcbuf = (float **) CM_calloc (MAX_UTT_LEN+10, sizeof(float *));
        mfcbuf[0] = (float *) CM_calloc ((MAX_UTT_LEN+10)*CEP_SIZE, sizeof(float));
        for (k = 1; k < MAX_UTT_LEN+10; k++)
            mfcbuf[k] = mfcbuf[k-1] + CEP_SIZE;
    }
}

int32 uttproc_get_featbuf (float **cep, float **dcep, float **dcep_80ms, float **pcep,
float **ddcep)
{
    *cep        = cep_buf;
    *dcep       = dcep_buf;
    *dcep_80ms  = dcep_80ms_buf;
    *pcep       = pcep_buf;
    *ddcep      = ddcep_buf;

    return n_featfr;
}

/*
 * Compute sphinx-II feature vectors (cep, dcep, ddcep, pow) from input melcep vector.
 * The input melcep vector has had mean normalization, agc, and silence compression
 * already applied to it.  Since dcep and ddcep look at cepstra from several adjacent
 * frames, no valid feature vectors are present for several frames (4) at either end
 * of each utterance.
 * Return 1 if a valid feature is computed for this input frame, 0 otherwise.
 */
static int32 compute_features(float *cep_o,
                              float *dcep_o,
                              float *dcep_80ms_o,
                              float *pcep_o,
                              float *ddcep_o,
                              float *mfcc)
{
    float *cep_in;
    float *dcep_in;
    float *dcep_80ms_in;
    float *pcep_in;
    float *ddcep_in;

    if (SCVQComputeFeatures(&cep_in, &dcep_in, &dcep_80ms_in, &pcep_in, &ddcep_in, mfcc)) {
        memcpy(cep_o,       cep_in,       sizeof(float) * CEP_SIZE);
        memcpy(dcep_o,      dcep_in,      sizeof(float) * CEP_SIZE);
        memcpy(dcep_80ms_o, dcep_80ms_in, sizeof(float) * CEP_SIZE);
        memcpy(pcep_o,      pcep_in,      sizeof(float) * POW_SIZE);
        memcpy(ddcep_o,     ddcep_in,     sizeof(float) * CEP_SIZE);

#if 0
        {
          int32 d;
          
          printf ("C: ");
          for (d = 0; d < CEP_SIZE; d++)
            printf (" %7.3f", cep_o[d]);
          printf ("\n");

          printf ("D: ");
          for (d = 0; d < CEP_SIZE; d++)
            printf (" %7.3f", dcep_o[d]);
          printf ("\n");

          printf ("L: ");
          for (d = 0; d < CEP_SIZE; d++)
            printf (" %7.3f", dcep_80ms_o[d]);
          printf ("\n");

          printf ("P: ");
          for (d = 0; d < POW_SIZE; d++)
            printf (" %7.3f", pcep_o[d]);
          printf ("\n");
          
          printf ("2: ");
          for (d = 0; d < CEP_SIZE; d++)
            printf (" %7.3f", ddcep_o[d]);
          printf ("\n");

          printf ("\n");
        }
#endif

        return 1;
    } else
        return 0;
}

static void warn_notidle (char const *func)
{
    if (uttstate != UTTSTATE_IDLE)
        E_WARN("%s called when not in IDLE state\n", func);
}

static void mfc2feat_live_frame (float *incep, int32 rawfr)
{
    float cep[CEP_SIZE];
    
    if (cmn == NORM_PRIOR)
        mean_norm_acc_sub (incep);

    if (agc == AGC_EMAX)
        agc_emax_proc(cep, incep, CEP_SIZE);
    else {
        memcpy (cep, incep, CEP_SIZE*sizeof(float));
    }

    if ((! silcomp) || histo_add_c0 (cep[0])) {
        comp2rawfr[n_compfr++] = rawfr;
        
        if (compute_features(cep_buf + cep_i,
                             dcep_buf + cep_i,
                             dcep_80ms_buf + cep_i,
                             pcep_buf + pow_i,
                             ddcep_buf + cep_i,
                             cep)) {
            cep_i += CEP_SIZE;
            pow_i += POW_SIZE;

            n_featfr++;
        }
    }
}

/* Convert all given mfc vectors to feature vectors */
static int32 mfc2feat_live (float **mfc, int32 nfr)
{
    int32 i;
    
    for (i = 0; i < nfr; i++, n_rawfr++)
        mfc2feat_live_frame (mfc[i], n_rawfr);

    return 0;
}

static int32 cmn_batch (float **mfc, int32 nfr)
{
    int32 i;
    
    if (cmn == NORM_UTT)
        norm_mean (mfc[0], nfr, CEP_SIZE);
    else if (cmn == NORM_PRIOR) {
        for (i = 0; i < nfr; i++)
            mean_norm_acc_sub (mfc[i]);
    }

    return 0;
}

static int32 agc_batch (float **mfc, int32 nfr)
{
    int32 i;
    float agc_out[CEP_SIZE];
    
    if (agc == AGC_NOISE) {
        real_agc_noise (mfc[0], nfr, CEP_SIZE);
    } else if (agc == AGC_MAX) {
        agc_max (mfc[0], nfr, CEP_SIZE);
    } else if (agc == AGC_EMAX) {
        for (i = 0; i < nfr; i++) {
            agc_emax_proc (agc_out, mfc[i], CEP_SIZE);
            memcpy (mfc[i], agc_out, CEP_SIZE * sizeof(float));
        }
    } else
        E_WARN("NO AGC\n");

    return 0;
}

static int32 silcomp_batch (float **mfc, int32 nfr)
{
    int32 i, j;
    float noiselevel;

    if (silcomp == COMPRESS_PRIOR) {
        j = 0;
        for (i = 0; i < nfr; i++) {
            if (histo_add_c0 (mfc[i][0])) {
                if (i != j)
                    memcpy (mfc[j], mfc[i], sizeof(float)*CEP_SIZE);

                comp2rawfr[j++] = i;
            }
            /* Else skip the frame, don't copy across */
        }
        nfr = j;
    } else {
        for (i = 0; i < nfr; i++)
            comp2rawfr[i] = i;          /* HACK!! */

        if (silcomp == COMPRESS_UTT) {
            noiselevel = histo_noise_level (mfc[0], nfr, CEP_SIZE);
            nfr = delete_background (mfc[0], nfr, CEP_SIZE, noiselevel);
        }
    }
    
    return nfr;
}

static int32 mfc2feat_batch (float **mfc, int32 nfr)
{
    int32 i, j, k;
    
    cmn_batch (mfc, nfr);
    agc_batch (mfc, nfr);
    nfr = silcomp_batch (mfc, nfr);
    
    assert (cep_i == 0);
    
    /*
     * HACK!! Hardwired knowledge that first and last 4 frames don't have features.
     * Simply copy frame[4] into frame[0..3], and frame[n-4] into the last four.
     */
    cep_i = (CEP_SIZE << 2);
    pow_i = (POW_SIZE << 2);
    for (i = 0; i < nfr; i++) {
        if (compute_features(cep_buf + cep_i,
                             dcep_buf + cep_i,
                             dcep_80ms_buf + cep_i,
                             pcep_buf + pow_i,
                             ddcep_buf + cep_i,
                             mfc[i])) {
            cep_i += CEP_SIZE;
            pow_i += POW_SIZE;

            n_featfr++;
        }
    }

    /* Copy frame[4] into frame[0]..[3] */
    for (i = 0, j = 0, k = 0; i < 4; i++, j += CEP_SIZE, k += POW_SIZE) {
        memcpy (cep_buf+j, cep_buf + (CEP_SIZE<<2), CEP_SIZE * sizeof(float));
        memcpy (dcep_buf+j, dcep_buf + (CEP_SIZE<<2), CEP_SIZE * sizeof(float));
        memcpy (dcep_80ms_buf+j, dcep_80ms_buf + (CEP_SIZE<<2), CEP_SIZE * sizeof(float));
        memcpy (ddcep_buf+j, ddcep_buf + (CEP_SIZE<<2), CEP_SIZE * sizeof(float));
        memcpy (pcep_buf+k, pcep_buf + (POW_SIZE<<2), POW_SIZE * sizeof(float));
        
        n_featfr++;
    }
    /* Similarly fill in the last 4 frames */
    for (i = 0; i < 4; i++) {
        memcpy (cep_buf + cep_i, cep_buf + (cep_i - CEP_SIZE), CEP_SIZE*sizeof(float));
        memcpy (dcep_buf + cep_i, dcep_buf + (cep_i - CEP_SIZE), CEP_SIZE*sizeof(float));
        memcpy (dcep_80ms_buf + cep_i, dcep_80ms_buf + (cep_i - CEP_SIZE),
                CEP_SIZE*sizeof(float));
        memcpy (ddcep_buf + cep_i, ddcep_buf + (cep_i - CEP_SIZE), CEP_SIZE*sizeof(float));
        memcpy (pcep_buf + pow_i, pcep_buf + (pow_i - POW_SIZE), POW_SIZE*sizeof(float));
        
        cep_i += CEP_SIZE;
        pow_i += POW_SIZE;
        
        n_featfr++;
    }
    
    return 0;
}


static void uttproc_fsg_search_fwd ( void )
{
  int32 *senscore, best;
  
#if 0
  int32 i;
  
  fprintf (stdout, "[%4d] CEP/DCEP/DCEP_80/DDCEP/POW\n", fsg_search->frame);
  for (i = 0; i < CEP_SIZE; i++) {
    fprintf (stdout, "\t%12.4e", cep_buf[search_cep_i + i]);
    fprintf (stdout, " %12.4e", dcep_buf[search_cep_i + i]);
    fprintf (stdout, " %12.4e", dcep_80ms_buf[search_cep_i + i]);
    fprintf (stdout, " %12.4e", ddcep_buf[search_cep_i + i]);
    if (i < POW_SIZE)
      fprintf (stdout, " %12.4e", pcep_buf[search_pow_i + i]);
    fprintf (stdout, "\n");
  }
#endif

  senscore = search_get_dist_scores();  /* senone scores array */
  
  if (query_compute_all_senones()) {
    best = senscr_all(senscore,
		      fsg_search_frame (fsg_search),
                      cep_buf + search_cep_i,
                      dcep_buf + search_cep_i,
                      dcep_80ms_buf + search_cep_i,
                      pcep_buf + search_pow_i,
                      ddcep_buf + search_cep_i);
  } else {
    fsg_search_sen_active(fsg_search);
    
    best = senscr_active(senscore,
			 fsg_search_frame (fsg_search),
                         cep_buf + search_cep_i,
                         dcep_buf + search_cep_i,
                         dcep_80ms_buf + search_cep_i,
                         pcep_buf + search_pow_i,
                         ddcep_buf + search_cep_i);
  }
  
  /* Note the best senone score for this frame */
  search_set_topsen_score (fsg_search_frame(fsg_search), best);
  
  fsg_search_frame_fwd (fsg_search);
}


/* Convert all given mfc vectors to feature vectors, and search one frame */
static int32 uttproc_frame ( void )
{
  int32 pr, frm;
  char *str;
  search_hyp_t *hyp;
  
  /* Search one frame */
  if (fsg_search_mode)
    uttproc_fsg_search_fwd();
  else if (query_fwdtree_flag())
    search_fwd (cep_buf + search_cep_i,
                dcep_buf + search_cep_i,
                dcep_80ms_buf + search_cep_i,
                pcep_buf + search_pow_i,
                ddcep_buf + search_cep_i);
  else
    search_fwdflat_frame (cep_buf + search_cep_i,
                          dcep_buf + search_cep_i,
                          dcep_80ms_buf + search_cep_i,
                          pcep_buf + search_pow_i,
                          ddcep_buf + search_cep_i);
  search_cep_i += CEP_SIZE;
  search_pow_i += POW_SIZE;
  
  n_searchfr++;
  
  pr = query_report_partial_result();
  if ((pr > 0) && ((n_searchfr % pr) == 1)) {
    /* Report partial result string */
    uttproc_partial_result (&frm, &str);
    printf ("PART[%d]: %s\n", frm, str);
    fflush (stdout);
  }
  
  pr = query_report_partial_result_seg();
  if ((pr > 0) && ((n_searchfr % pr) == 1)) {
    /* Report partial result segmentation */
    uttproc_partial_result_seg (&frm, &hyp);
    printf ("PARTSEG[%d]:", frm);
    for (; hyp; hyp = hyp->next)
      printf (" %s %d %d %.2f", hyp->word, hyp->sf, hyp->ef, hyp->conf);
    printf ("\n");
    fflush (stdout);
  }
  
  return 0;
}

static void fwdflat_search (int32 n_frames)
{
    int32 i, j, k;
    
    search_fwdflat_start ();

    for (i = 0, j = 0, k = 0; i < n_frames; i++, j += CEP_SIZE, k += POW_SIZE)
        search_fwdflat_frame (cep_buf+j, dcep_buf+j, dcep_80ms_buf+j, pcep_buf+k, ddcep_buf+j);

    search_fwdflat_finish ();
}

static void write_results (char const *hyp, int32 aborted)
{
    search_hyp_t *hyplist;       /* Hyp with word segmentation information */
    
    /* Check if need to autonumber utterances */
    if (matchfp) {
        fprintf (matchfp, "%s (%s %s %d)\n",
                 hyp, uttid, aborted ? "[ABORTED]" : "", search_get_score());
        fflush (matchfp);
    }
    
    if (matchsegfp) {
        fprintf (matchsegfp, "%s ", uttid);
        for (hyplist = search_get_hyp (); hyplist; hyplist = hyplist->next) {
            fprintf (matchsegfp, " %d %d %.2f %s",
                     hyplist->sf, hyplist->ef, hyplist->conf, hyplist->word);
        }
        fprintf (matchsegfp, "\n");
        fflush (matchsegfp);
    }
}

static void uttproc_windup (int32 *fr, char **hyp)
{
  char *dir;
  char filename[4096];
  FILE *pscrlat_fp;
  
  /* Wind up first pass and run next pass, if necessary */
  if (fsg_search_mode)
    fsg_search_utt_end(fsg_search);
  else {
    if (query_fwdtree_flag()) {
        search_finish_fwd ();
        
        if (query_fwdflat_flag() && (searchFrame() > 0))
            fwdflat_search (n_featfr);
    } else
        search_fwdflat_finish ();

    /* Run bestpath pass if specified */
    if ((searchFrame() > 0) && query_bestpath_flag())
        bestpath_search ();
  }
  
  /* Moved out of the above else clause (rkm:2005/03/08) */
  if (query_phone_conf()) {
    search_hyp_t *pseg, *search_uttpscr2allphone();
    
    /* Obtain pscr-based allphone segmentation */
    pseg = search_uttpscr2allphone();
    search_hyp_free (pseg);
  }
  
  /* Moved out of the above else clause (rkm:2005/03/02) */
  if ((dir = query_pscr2lat()) != NULL) {
    sprintf (filename, "%s/%s.pscrlat", dir, uttid);
    
    if ((pscrlat_fp = fopen(filename, "w")) == NULL) {
      E_ERROR("fopen(%s,w) failed; writing to stdout\n", filename);
      search_uttpscr2phlat_print (stdout);
    } else {
      search_uttpscr2phlat_print (pscrlat_fp);
      fclose (pscrlat_fp);
    }
  }
  
  search_result (fr, hyp);
  
  write_results (*hyp, 0);
    
  timing_stop (*fr);
  
  uttstate = UTTSTATE_IDLE;
}

/*
 * One time initialization
 */

static fe_t    *fe;
static param_t fe_param;

int32 uttproc_init ( void )
{
    char const *fn;

    if (uttstate != UTTSTATE_UNDEF) {
        E_ERROR("uttproc_init called when not in UNDEF state\n");
        return -1;
    }
    
    fe_init_params(&fe_param);
    query_fe_params(&fe_param);
    fe = fe_init(&fe_param);

    if (!fe) 
      return -1;

    mean_norm_init (CEP_SIZE);
    
    feat_alloc ();

    comp2rawfr = (int16 *) CM_calloc (MAX_UTT_LEN, sizeof(int16));
    uttid = (char *) CM_calloc (UTTIDSIZE, 1);

    if ((fn = query_match_file_name()) != NULL) {
        if ((matchfp = fopen (fn, "w")) == NULL)
            E_ERROR("fopen(%s,w) failed\n", fn);
    }
    if ((fn = query_matchseg_file_name()) != NULL) {
        if ((matchsegfp = fopen (fn, "w")) == NULL)
            E_ERROR("fopen(%s,w) failed\n", fn);
    }
    
    if ((fn = query_cdcn_file()) != NULL) {
        E_INFO("Initializing CDCN module from %s\n", fn);
        cdcn_init (fn, &cdcn);
    }
    
    timing_init ();

    uttstate = UTTSTATE_IDLE;
    utt_ofl = 0;
    uttno = 0;

    /* Initialize the FSG search module */
    {
      char *fsgfile;
      char *fsgname;
      char *fsgctlfile;
      FILE *ctlfp;
      char line[16384], word[16384];
      
      fsg_search = fsg_search_init (NULL);
      
      fsgfile = kb_get_fsg_file_name();
      
      fsg_search_mode = (fsgfile != NULL);
      
      if (fsg_search_mode) {
        fsgname = uttproc_load_fsgfile(fsgfile);
        if (! fsgname)
          E_FATAL("Error loading FSG file '%s'\n", fsgfile);
        
        /* Make this FSG the currently active one */
        if (uttproc_set_fsg (fsgname) < 0)
          E_FATAL("Error setting current FSG to '%s'\n", fsgname);
        
        E_INFO("FSG Mode; lextree, flat, bestpath searches disabled\n");
      }
      
      fsgctlfile = kb_get_fsg_ctlfile_name();
      if (fsgctlfile) {
        if ((ctlfp = fopen(fsgctlfile, "r")) == NULL) {
          /* Should this be E_ERROR?? */
          E_FATAL("fopen(%s,r) failed\n", fsgctlfile);
        }
        
        while (fgets (line, sizeof(line), ctlfp) != NULL) {
          if ((line[0] == '#')                          /* Commented out */
              || (sscanf (line, "%s", word) != 1))      /* Blank line */
            continue;
          
          fsgfile = word;
          fsgname = uttproc_load_fsgfile(fsgfile);
          if (! fsgname) {
            /* Should this be E_ERROR?? */
            E_FATAL("Error loading FSG file '%s'\n", fsgfile);
          }
        }
        
        fclose (ctlfp);
      }
    }
    
    return 0;
}

CDCN_type *uttproc_get_cdcn_ptr ( void )
{
    return &cdcn;
}

/*
 * One time cleanup
 */
int32 uttproc_end ( void )
{
    if (uttstate != UTTSTATE_IDLE) {
        E_ERROR("uttproc_end called when not in IDLE state\n");
        return -1;
    }
    
    if (matchfp)
        fclose (matchfp);
    if (matchsegfp)
        fclose (matchsegfp);
    
    timing_end ();

    return 0;
}

int32 uttproc_begin_utt (char const *id)
{
    char filename[1024];
    int32 i;
    
    for (i = 0; i < 5; i++)
        samp_hist[i] = 0;
    max_samp = 0;
    
    if (uttstate != UTTSTATE_IDLE) {
        E_ERROR("uttproc_begin_utt called when not in IDLE state\n");
        return -1;
    }

    if (fe_start_utt(fe) < 0)
        return -1;
    
    inputtype = INPUT_UNKNOWN;

    livemode = (nosearch ||
                (cmn == NORM_UTT) ||
                ((agc != AGC_EMAX) && (agc != AGC_NONE)) ||
                (silcomp == COMPRESS_UTT)) ? 0 : 1;
    E_INFO("%s\n", livemode ? "Livemode" : "Batchmode");
    
    /*
     * One-time initialization of AGC as necessary. Done here rather than in
     * uttproc_init because type of cmn/agc not known until now.
     */
    if ((uttno == 0) && (agc == AGC_EMAX)) {
        if (cmn == NORM_PRIOR)
            uttproc_agcemax_set (5.0);  /* Hack!! Hardwired max(C0) of 5.0 with CMN */
        else
            uttproc_agcemax_set (10.0); /* Hack!! Hardwired max(C0) of 10.0 without CMN */
    }
    
    pow_i = cep_i = 0;
    search_pow_i = search_cep_i = 0;
    n_rawfr = n_featfr = n_searchfr = n_compfr = 0;
    utt_ofl = 0;
    
    uttno++;
    if (! id)
        sprintf (uttid, "%s%08d", uttid_prefix ? uttid_prefix : "", uttno);
    else
        strcpy (uttid, id);
    
    if (rawlogdir) {
        sprintf (filename, "%s/%s.raw", rawlogdir, uttid);
        if ((rawfp = fopen(filename, "wb")) == NULL)
            E_ERROR("fopen(%s,wb) failed\n", filename);
        else {
            strcpy (rawfilename, filename);
            E_INFO("Rawfile: %s\n", filename);
        }
    }
    if (mfclogdir) {
        int32 k = 0;

        sprintf (filename, "%s/%s.mfc", mfclogdir, uttid);
        if ((mfcfp = fopen(filename, "wb")) == NULL)
            E_ERROR("fopen(%s,wb) failed\n", filename);
        else
            fwrite (&k, sizeof(int32), 1, mfcfp);
    }
    
    timing_start ();
    
    SCVQNewUtt ();
    
    if (! nosearch) {
      if (fsg_search_mode)
        fsg_search_utt_start (fsg_search);
      else if (query_fwdtree_flag())
        search_start_fwd ();
      else
        search_fwdflat_start ();
    }
    
    search_uttpscr_reset ();
    
    uttstate = UTTSTATE_BEGUN;
    
    return 0;
}

int32 uttproc_rawdata (int16 *raw, int32 len, int32 block)
{
    int32 i, k, v;
    int32 fe_ret;
    
    for (i = 0; i < len; i++) {
        v = raw[i];
        if (v < 0)
            v = -v;
        if (v > max_samp)
            max_samp = v;
        
        if (v < 4096)
            samp_hist[0]++;
        else if (v < 8192)
            samp_hist[1]++;
        else if (v < 16384)
            samp_hist[2]++;
        else if (v < 30720)
            samp_hist[3]++;
        else
            samp_hist[4]++;
    }
    
    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_rawdata called when utterance not begun\n");
        return -1;
    }
    if (inputtype == INPUT_MFC) {
        E_ERROR("uttproc_rawdata mixed with uttproc_cepdata in same utterance??\n");
        return -1;
    }
    inputtype = INPUT_RAW;
    
    if (utt_ofl)
        return -1;
    
    /* FRAME_SHIFT is SAMPLING_RATE/FRAME_RATE, thus resulting in
     * number of sample per frame.
     */
    k = (MAX_UTT_LEN - n_rawfr) * fe->FRAME_SHIFT;
    if (len > k) {
        len = k;
        utt_ofl = 1;
        E_ERROR("Utterance too long; truncating to about %d frames\n", MAX_UTT_LEN);
    }

    if (rawfp && (len > 0))
        fwrite (raw, sizeof(int16), len, rawfp);
    
    /*    
          if ((k = fe_raw2cep (raw, len, mfcbuf + n_rawfr)) < 0)
          return -1;
    */
    fe_ret = fe_process_utt (fe, raw, len, mfcbuf + n_rawfr, &k);
    if (fe_ret != FE_SUCCESS) {
      if (fe_ret == FE_ZERO_ENERGY_ERROR) {
	E_WARN("uttproc_rawdata processed some frames with zero energy. Consider using dither.\n");
      } else {
        return -1;
      }
    }

    if (mfcfp && (k > 0))
        fwrite (mfcbuf[n_rawfr], sizeof(float), k * CEP_SIZE, mfcfp);

    if (livemode) {
        mfc2feat_live (mfcbuf+n_rawfr, k);

        if (search_cep_i < cep_i)
            uttproc_frame ();

        if (block) {
            while (search_cep_i < cep_i)
                uttproc_frame ();
        }
    } else
        n_rawfr += k;

    return (n_featfr - n_searchfr);
}

int32 uttproc_cepdata (float **cep, int32 nfr, int32 block)
{
    int32 i, k;
    
    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_cepdata called when utterance not begun\n");
        return -1;
    }
    if (inputtype == INPUT_RAW) {
        E_ERROR("uttproc_cepdata mixed with uttproc_rawdata in same utterance??\n");
        return -1;
    }
    inputtype = INPUT_MFC;
    
    if (utt_ofl)
        return -1;
    
    k = MAX_UTT_LEN - n_rawfr;
    if (nfr > k) {
        nfr = k;
        utt_ofl = 1;
        E_ERROR("Utterance too long; truncating to about %d frames\n", MAX_UTT_LEN);
    }
    
    for (i = 0; i < nfr; i++)
        memcpy (mfcbuf[i+n_rawfr], cep[i], CEP_SIZE*sizeof(float));

    if (mfcfp && (nfr > 0))
        fwrite (mfcbuf[n_rawfr], sizeof(float), nfr * CEP_SIZE, mfcfp);

    if (livemode) {
        mfc2feat_live (mfcbuf+n_rawfr, nfr);

        if (search_cep_i < cep_i)
            uttproc_frame ();

        if (block) {
            while (search_cep_i < cep_i)
                uttproc_frame ();
        }
    } else
        n_rawfr += nfr;

    return (n_featfr - n_searchfr);
}

int32 uttproc_end_utt ( void )
{
    int32 i, k;
    float cep[13], c0;
    float *leftover_cep;
    int live_nframe;

    /* kal */
    leftover_cep       = (float *) CM_calloc (MAX_CEP_LEN, sizeof(float));

    /* Dump samples histogram */
    k = 0;
    for (i = 0; i < 5; i++)
        k += samp_hist[i];
    if (k > 0) {
        E_INFO("Samples histogram (%s) (4/8/16/30/32K):", uttproc_get_uttid());
        for (i = 0; i < 5; i++)
            E_INFOCONT(" %.1f%%(%d)", samp_hist[i]*100.0/k, samp_hist[i]);
        E_INFOCONT("; max: %d\n", max_samp);
    }
    
    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_end_utt called when utterance not begun\n");
        return -1;
    }

    if (livemode) {
        fe_end_utt(fe, leftover_cep, &live_nframe);
        mfc2feat_live_frame (leftover_cep, live_nframe);
    } else {
        mfc2feat_batch (mfcbuf, n_rawfr);
        /* 
         * fe_end_utt, in this case, should be a no-op. But, just in
         * case the code changes, we can fe_end_utt here for front end
         * cleanups.
         */
        fe_end_utt(fe, leftover_cep, &live_nframe);
    }

    uttstate = nosearch ? UTTSTATE_IDLE : UTTSTATE_ENDED;

    SCVQEndUtt();
    
    /* Update estimated CMN vector */ 
    if (cmn == NORM_PRIOR) {
        uttproc_cepmean_get (cep);
        c0 = cep[0];
        mean_norm_update ();
        uttproc_cepmean_get (cep);

        /* Update estimated AGC Max (C0) */ 
        if (agc == AGC_EMAX) {
            agc_emax_update ();
        }
    } else {
        /* Update estimated AGC Max (C0) */ 
        if (agc == AGC_EMAX) {
            agc_emax_update ();
        }
    }
    
    if (silcomp == COMPRESS_PRIOR)
        compute_noise_level ();
    
    if (rawfp) {
        fclose (rawfp);
        rawfp = NULL;
#ifdef WIN32
        if (_chmod(rawfilename, _S_IREAD ) < 0)
            E_ERROR("chmod(%s,READONLY) failed\n", rawfilename);
#endif
    }
    if (mfcfp) {
        int32 k;
        
        fflush (mfcfp);
        fseek (mfcfp, 0, SEEK_SET);
        k = n_rawfr * CEP_SIZE;
        fwrite (&k, sizeof(int32), 1, mfcfp);

        fclose (mfcfp);
        mfcfp = NULL;
    }

    free(leftover_cep);

    return 0;
}

int32 uttproc_abort_utt ( void )
{
    int32 fr;
    char *hyp;
    
    if (uttproc_end_utt () < 0)
        return -1;

    /* Truncate utterance to the portion already processed */
    cep_i = search_cep_i;
    pow_i = search_pow_i;
    
    uttstate = UTTSTATE_IDLE;

    if (! nosearch) {
      if (fsg_search_mode)
        fsg_search_utt_end(fsg_search);
      else {
        if (query_fwdtree_flag())
            search_finish_fwd ();
        else
            search_fwdflat_finish ();
        
        search_result (&fr, &hyp);
        
        write_results (hyp, 1);
      }
      timing_stop (fr);
    }
    
    return 0;
}

int32 uttproc_stop_utt ( void )
{
    if (uttstate != UTTSTATE_BEGUN) {
        E_ERROR("uttproc_stop_utt called when utterance not begun\n");
        return -1;
    }

    uttstate = UTTSTATE_STOPPED;
    
    if (! nosearch) {
      if (fsg_search_mode)
        fsg_search_utt_end(fsg_search);
      else {
        if (query_fwdtree_flag())
            search_finish_fwd ();
        else
          search_fwdflat_finish ();
      }
    }
    
    return 0;
}

int32 uttproc_restart_utt ( void )
{
    if (uttstate != UTTSTATE_STOPPED) {
        E_ERROR("uttproc_restart_utt called when decoding not stopped\n");
        return -1;
    }

    uttstate = UTTSTATE_BEGUN;
    
    if (! nosearch) {
      if (fsg_search_mode)
        fsg_search_utt_start (fsg_search);
      else if (query_fwdtree_flag())
        search_start_fwd ();
      else
        search_fwdflat_start ();
      
      search_cep_i = 0;
      search_pow_i = 0;
      n_searchfr = 0;
    }
    
    return 0;
}

int32 uttproc_partial_result (int32 *fr, char **hyp)
{
    if ((uttstate != UTTSTATE_BEGUN) && (uttstate != UTTSTATE_ENDED)) {
        E_ERROR("uttproc_partial_result called outside utterance\n");
        *fr = -1;
        *hyp = NULL;
        return -1;
    }

    if (fsg_search_mode) {
      fsg_search_history_backtrace(fsg_search, FALSE);
      search_result (fr, hyp);
    } else
      search_partial_result (fr, hyp);
    
    return 0;
}

int32 uttproc_result (int32 *fr, char **hyp, int32 block)
{
    if (uttstate != UTTSTATE_ENDED) {
        E_ERROR("uttproc_result called when utterance not ended\n");
        *hyp = NULL;
        *fr = -1;

        return -1;
    }
    
    if (search_cep_i < cep_i)
        uttproc_frame ();

    if (block) {
        while (search_cep_i < cep_i)
            uttproc_frame ();
    }
    
    if (search_cep_i < cep_i)
        return (n_featfr - n_searchfr);

    uttproc_windup (fr, hyp);
    
    return 0;
}

void uttproc_align (char *sent)
{
    time_align_utterance ("alignment", NULL, "<s>", -1, sent, -1, "</s>");
}


int32 uttproc_partial_result_seg (int32 *fr, search_hyp_t **hyplist)
{
    char *str;
    
    if ((uttstate != UTTSTATE_BEGUN) && (uttstate != UTTSTATE_ENDED)) {
        E_ERROR("uttproc_partial_result called outside utterance\n");
        *fr = -1;
        *hyplist = NULL;
        return -1;
    }

    if (fsg_search_mode) {
      fsg_search_history_backtrace(fsg_search, FALSE);
      search_result (fr, &str);
    } else
      search_partial_result (fr, &str); /* Internally makes partial result */
    
    *hyplist = search_get_hyp();
    if ((*hyplist)->wid < 0)
      *hyplist = NULL;
    
    return 0;
}

int32 uttproc_result_seg (int32 *fr, search_hyp_t **hyplist, int32 block)
{
    char *str;
    int32 res;
    
    if ((res = uttproc_result (fr, &str, block)) != 0)
        return res;     /* Not done yet; or ERROR */
    
    *hyplist = search_get_hyp();
    if ((*hyplist)->wid < 0)
      *hyplist = NULL;
    
    return 0;
}

int32 uttproc_lmupdate (char const *lmname)
{
    lm_t *lm, *cur_lm;
    
    warn_notidle ("uttproc_lmupdate");
    
    if ((lm = lm_name2lm (lmname)) == NULL)
        return -1;
    
    cur_lm = lm_get_current ();
    if (lm == cur_lm)
        search_set_current_lm ();

    return 0;
}

int32 uttproc_set_context (char const *wd1, char const *wd2)
{
    int32 w1, w2;
    
    warn_notidle ("uttproc_set_context");
    
    if (wd1) {
        w1 = kb_get_word_id (wd1);
        if ((w1 < 0) || (! dictwd_in_lm (w1))) {
            E_ERROR("Unknown word: %s\n", wd1);
            search_set_context (-1, -1);

            return -1;
        }
    } else
        w1 = -1;

    if (wd2) {
        w2 = kb_get_word_id (wd2);
        if ((w2 < 0) || (! dictwd_in_lm (w2))) {
            E_ERROR("Unknown word: %s\n", wd2);
            search_set_context (-1, -1);
            
            return -1;
        }
    } else
        w2 = -1;
    
    if (w2 < 0) {
        search_set_context (-1, -1);
        return ((w1 >= 0) ? -1 : 0);
    } else {
        /* Because of the perverse way search_set_context was defined... */
        if (w1 < 0)
            search_set_context (w2, -1);
        else
            search_set_context (w1, w2);
    }
    
    return 0;
}

int32 uttproc_set_lm (char const *lmname)
{
    warn_notidle ("uttproc_set_lm");
    
    if (lmname == NULL) {
        E_ERROR("uttproc_set_lm called with NULL argument\n");
        return -1;
    }
    
    if (lm_set_current (lmname) < 0)
        return -1;
    
    fsg_search_mode = FALSE;
    
    search_set_current_lm ();

    E_INFO("LM= \"%s\"\n", lmname);
    
    return 0;
}


int32 uttproc_load_fsg (s2_fsg_t *fsg,
                        int32 use_altpron,
                        int32 use_filler,
                        float32 silprob,
                        float32 fillprob,
                        float32 lw)
{
  word_fsg_t *word_fsg;
  
  word_fsg = word_fsg_load(fsg, use_altpron, use_filler, silprob, fillprob, lw);
  
  if (! word_fsg)
    return 0;
  
  if (! fsg_search_add_fsg (fsg_search, word_fsg)) {
    E_ERROR("Failed to add FSG '%s' to system\n", word_fsg_name(word_fsg));
    word_fsg_free (word_fsg);
    return 0;
  }
  
  return 1;
}


char *uttproc_load_fsgfile (char *fsgfile)
{
  word_fsg_t *fsg;
  
  fsg = word_fsg_readfile(fsgfile, 
                          query_fsg_use_altpron(),
                          query_fsg_use_filler(),
                          kb_get_silpen(),
                          kb_get_fillpen(),
                          kb_get_lw());
  if (! fsg)
    return NULL;
  
  if (! fsg_search_add_fsg (fsg_search, fsg)) {
    E_ERROR("Failed to add FSG '%s' to system\n", word_fsg_name(fsg));
    word_fsg_free (fsg);
    return NULL;
  }
  
  return fsg->name;
}


int32 uttproc_del_fsg (char *fsgname)
{
    warn_notidle ("uttproc_del_fsg");
    
    if (fsgname == NULL) {
        E_ERROR("uttproc_del_fsg called with NULL argument\n");
        return -1;
    }
    
    if (! fsg_search_del_fsg_byname(fsg_search, fsgname))
        return -1;
    
    return 0;
}


int32 uttproc_set_fsg (char *fsgname)
{
    warn_notidle ("uttproc_set_fsg");
    
    if (fsgname == NULL) {
        E_ERROR("uttproc_set_fsg called with NULL argument\n");
        return -1;
    }
    
    if (! fsg_search_set_current_fsg (fsg_search, fsgname))
        return -1;
    
    fsg_search_mode = TRUE;
    
    E_INFO("FSG= \"%s\"\n", fsgname);
    
    return 0;
}



int32 uttproc_get_fsg_start_state ( void )
{
  return fsg_search_get_start_state (fsg_search);
}


int32 uttproc_get_fsg_final_state ( void )
{
  return fsg_search_get_final_state (fsg_search);
}


int32 uttproc_set_fsg_start_state (int32 state)
{
  return fsg_search_set_start_state (fsg_search, state);
}


int32 uttproc_set_fsg_final_state (int32 state)
{
  return fsg_search_set_final_state (fsg_search, state);
}


boolean uttproc_fsg_search_mode ( void )
{
  return fsg_search_mode;
}


int32 uttproc_set_rescore_lm (char const *lmname)
{
    searchlat_set_rescore_lm (lmname);
    return 0;
}

int32 uttproc_set_startword (char const *str)
{
    warn_notidle ("uttproc_set_startword");
    
    search_set_startword (str);
    return 0;
}

int32 uttproc_set_cmn (scvq_norm_t n)
{
    warn_notidle ("uttproc_set_cmn");
    
    switch (n) {
    case NORM_NONE: E_INFO("CMN: None\n");
      break;
    case NORM_UTT: E_INFO("CMN: Based on current utterance\n");
      break;
    case NORM_PRIOR: E_INFO("CMN: Estimated, based on past history\n");
      break;
    default: E_FATAL("CMN: Unknown type %d\n", n);
      break;
    }
    
    cmn = n;
    
    return 0;
}

int32 uttproc_set_agc (scvq_agc_t a)
{
    warn_notidle ("uttproc_set_agc");
    
    agc = a;
    
    switch (a) {
    case AGC_NONE: E_INFO("AGC: None\n");
      break;
    case AGC_EMAX: E_INFO("AGC: MAX estimated from past history\n");
      break;
    case AGC_MAX: E_INFO("AGC: MAX based on current utterance\n");
      break;
    default: E_WARN("AGC: %d; Obsolete, use none, max, or emax\n", a);
      break;
    }
    
    return 0;
}

int32 uttproc_set_silcmp (scvq_compress_t c)
{
    warn_notidle ("uttproc_set_silcmp");

    if (c != COMPRESS_NONE)
        E_WARN("Silence compression doesn't work well; use the cont_ad module instead\n");
    
    silcomp = c;
    return 0;
}

#if 0
int32 uttproc_set_uttid (char const *id)
{
    warn_notidle ("uttproc_set_uttid");
    
    assert (strlen(id) < UTTIDSIZE);
    strcpy (uttid, id);
    
    return 0;
}
#endif

char const *uttproc_get_uttid ( void )
{
    return uttid;
}

int32 uttproc_set_auto_uttid_prefix (char const *prefix)
{
    if (uttid_prefix)
        free (uttid_prefix);
    uttid_prefix = salloc(prefix);
    uttno = 0;
    
    return 0;
}

int32   uttprocGetcomp2rawfr(int16 **ptr)
{
    *ptr = comp2rawfr;
    return n_featfr;
}

void    uttprocSetcomp2rawfr(int32 num, int32 const *ptr)
{
    int32               i;
    
    n_featfr = num;
    for (i = 0; i < num; i++)
        comp2rawfr[i] = ptr[i];
}

int32 uttproc_feat2rawfr (int32 fr)
{
    return fr;  /* comp2rawfr[] is buggy :(  ignore it for now (rkm) */
    
    if (fr >= n_featfr)
        fr = n_featfr-1;
    if (fr < 0)
        fr = 0;

    return comp2rawfr[fr+8]-4;
}

int32 uttproc_raw2featfr (int32 fr)
{
    int32 i;

    fr += 4;
    for (i = 0; (i < n_featfr) && (comp2rawfr[i] != fr); i++);
    if (i >= n_featfr)
        return -1;
    return (i-8);
}

int32 uttproc_cepmean_set (float *cep)
{
    warn_notidle ("uttproc_cepmean_set");

    return (cepmean_set (cep));
}

int32 uttproc_cepmean_get (float *cep)
{
    return (cepmean_get (cep));
}

int32 uttproc_agcemax_set (float c0max)
{
    warn_notidle ("uttproc_agcemax_set");
    agcemax_set (c0max);
    return 0;
}

double uttproc_agcemax_get ( void )
{
    extern double agcemax_get();
    
    warn_notidle ("uttproc_agcemax_get");
    return agcemax_get ();
}

int32 uttproc_nosearch (int32 flag)
{
    warn_notidle ("uttproc_nosearch");

    nosearch = flag;
    return 0;
}

int32 uttproc_set_rawlogdir (char const *dir)
{
    warn_notidle ("uttproc_set_rawlogdir");

    if (! rawlogdir) {
        if ((rawlogdir = calloc (1024,1)) == NULL) {
            E_ERROR("calloc(1024,1) failed\n");
            return -1;
        }
    }
    if (rawlogdir)
        strcpy (rawlogdir, dir);

    return 0;
}

int32 uttproc_set_mfclogdir (char const *dir)
{
    warn_notidle ("uttproc_set_mfclogdir");

    if (! mfclogdir) {
        if ((mfclogdir = calloc (1024,1)) == NULL) {
            E_ERROR("calloc(1024,1) failed\n");
            return -1;
        }
    }
    if (mfclogdir)
        strcpy (mfclogdir, dir);

    return 0;
}

search_hyp_t *uttproc_allphone_file (char const *utt)
{
    int32 nfr;
    extern search_hyp_t *allphone_utt();
    extern char *build_uttid (const char *utt); /* in fbs_main.c */
    extern int32 utt_file2feat();       /* in fbs_main.c */
    search_hyp_t *hyplist, *h;
    
    build_uttid (utt);

    if ((nfr = utt_file2feat (utt, 1)) < 0)
        return NULL;
    
    hyplist = allphone_utt (nfr, cep_buf, dcep_buf, dcep_80ms_buf, pcep_buf, ddcep_buf);

    /* Write match and matchseg files if needed */
    if (matchfp) {
        for (h = hyplist; h; h = h->next)
            fprintf (matchfp, "%s ", h->word);
        fprintf (matchfp, "(%s)\n", uttid);
        fflush (matchfp);
    }
    if (matchsegfp) {
        fprintf (matchsegfp, "%s ", uttid);
        for (h = hyplist; h; h = h->next)
            fprintf (matchsegfp, " %d %d %s", h->sf, h->ef, h->word);
        fprintf (matchsegfp, "\n");
        fflush (matchsegfp);
    }

    return hyplist;
}
