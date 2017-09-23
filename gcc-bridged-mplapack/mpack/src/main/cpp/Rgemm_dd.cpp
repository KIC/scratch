/*
 * Copyright (c) 2008-2012
 *	Nakata, Maho
 * 	All rights reserved.
 *
 * $Id: Rgemm_dd.cpp,v 1.4 2010/08/07 05:50:09 nakatamaho Exp $
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#define ___MPACK_BUILD_WITH_DD___
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <mblas.h>
#include <mlapack.h>
#include <mpack_benchmark.h>

#define TOTALSTEPS 1000

int main (int argc, char *argv[])
{
  REAL alpha, beta, dummy;
  REAL *dummywork;
  double elapsedtime, t1, t2;
  char transa, transb, normtype;
  int N0, M0, K0, STEPN, STEPM, STEPK;
  int lda, ldb, ldc;
  int i, j, m, n, k, ka, kb, p, q;
  int check_flag = 1;

  const char mblas_sym[] = SYMBOL_GCC_RGEMM;
  const char raxpy_sym[] = SYMBOL_GCC_RAXPY;
  void *handle;
  void (*mblas_ref) (const char *, const char *, mpackint, mpackint, mpackint,
		     REAL, REAL *, mpackint, REAL *, mpackint, REAL, REAL *, mpackint);
  void (*raxpy_ref) (mpackint, REAL, REAL *, mpackint, REAL *, mpackint);
  char *error;
  REAL diff;
  double diffr;

  // initialization
  N0 = M0 = K0 = 1;
  STEPM = STEPN = STEPK = 1;
  transa = transb = 'n';
  normtype = 'm';
  if (argc != 1) {
    for (i = 1; i < argc; i++) {
      if (strcmp ("-N", argv[i]) == 0) {
	N0 = atoi (argv[++i]);
      }
      else if (strcmp ("-M", argv[i]) == 0) {
	M0 = atoi (argv[++i]);
      }
      else if (strcmp ("-K", argv[i]) == 0) {
	K0 = atoi (argv[++i]);
      }
      else if (strcmp ("-STEPN", argv[i]) == 0) {
	STEPN = atoi (argv[++i]);
      }
      else if (strcmp ("-STEPM", argv[i]) == 0) {
	STEPM = atoi (argv[++i]);
      }
      else if (strcmp ("-STEPK", argv[i]) == 0) {
	STEPK = atoi (argv[++i]);
      }
      else if (strcmp ("-NN", argv[i]) == 0) {
	transa = transb = 'n';
      }
      else if (strcmp ("-TT", argv[i]) == 0) {
	transa = transb = 't';
      }
      else if (strcmp ("-NT", argv[i]) == 0) {
	transa = 'n';
	transb = 't';
      }
      else if (strcmp ("-TN", argv[i]) == 0) {
	transa = 't';
	transb = 'n';
      }
      else if (strcmp ("-NOCHECK", argv[i]) == 0) {
	check_flag = 0;
      }
    }
  }

  if (check_flag) {
    handle = dlopen (MBLAS_REF_LIB DYLIB_SUFFIX, RTLD_LAZY);
    if (!handle) {
      printf ("dlopen: %s\n", dlerror ());
      return 1;
    }
    mblas_ref = (void (*)
		 (const char *, const char *, mpackint, mpackint, mpackint, REAL,
		  REAL *, mpackint, REAL *, mpackint, REAL, REAL *, mpackint)) dlsym (handle, mblas_sym);
    if ((error = dlerror ()) != NULL) {
      fprintf (stderr, "%s\n", error);
      return 1;
    }

    raxpy_ref = (void (*)(mpackint, REAL, REAL *, mpackint, REAL *, mpackint)) dlsym (handle, raxpy_sym);
    if ((error = dlerror ()) != NULL) {
      fprintf (stderr, "%s\n", error);
      return 1;
    }
  }

  m = M0;
  n = N0;
  k = K0;
  for (p = 0; p < TOTALSTEPS; p++) {
    if (Mlsame (&transa, "n")) {
      ka = k;
      lda = m;
    }
    else {
      ka = m;
      lda = k;
    }
    if (Mlsame (&transb, "n")) {
      kb = n;
      ldb = k;
    }
    else {
      kb = k;
      ldb = n;
    }
    ldc = m;

    REAL *A = new REAL[lda * ka];
    REAL *B = new REAL[ldb * kb];
    REAL *C = new REAL[ldc * n];
    REAL *Cd = new REAL[ldc * n];
    REAL mOne = -1;
    alpha = randomnumber (dummy);
    beta = randomnumber (dummy);
    for (i = 0; i < lda * ka; i++) {
      A[i] = randomnumber (dummy);
    }
    for (i = 0; i < ldb * kb; i++) {
      B[i] = randomnumber (dummy);
    }
    for (i = 0; i < ldc * n; i++) {
      C[i] = Cd[i] = randomnumber (dummy);
    }
    if (check_flag) {
      t1 = gettime ();
      Rgemm (&transa, &transb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
      t2 = gettime ();
      elapsedtime = (t2 - t1);
      (*mblas_ref) (&transa, &transb, m, n, k, alpha, A, lda, B, ldb, beta, Cd, ldc);
      (*raxpy_ref) ((mpackint) (ldc * n), mOne, C, (mpackint) 1, Cd, (mpackint) 1);
      diff = Rlange (&normtype, (mpackint) ldc, (mpackint) n, Cd, ldc, dummywork);
      diffr = cast2double (diff);
      printf ("    m     n     k     MFLOPS   error   transa   transb\n");
      //2mnk+2mn flops are needed
      printf ("%5d %5d %5d %10.3f %5.2e       %c        %c\n", (int) m, (int) n, (int) k,
	      (2.0 * (double) m * (double) n * (double) k + 2.0 * (double) m * (double) n) / elapsedtime * MFLOPS,
	      diffr, transa, transb);
    }
    else {
      t1 = gettime ();
      Rgemm (&transa, &transb, m, n, k, alpha, A, lda, B, ldb, beta, C, ldc);
      t2 = gettime ();
      elapsedtime = (t2 - t1);
      printf ("    m     n     k     MFLOPS    transa   transb\n");
      //2mnk+2mn flops are needed
      printf ("%5d %5d %5d %10.3f         %c        %c\n", (int) m, (int) n, (int) k,
	      (2.0 * (double) m * (double) n * (double) k + 2.0 * (double) m * (double) n) / elapsedtime * MFLOPS,
	      diffr, transa, transb);
    }
    delete[]Cd;
    delete[]C;
    delete[]B;
    delete[]A;
    m = m + STEPM;
    n = n + STEPN;
    k = k + STEPK;
  }
  if (check_flag)
    dlclose (handle);
}
