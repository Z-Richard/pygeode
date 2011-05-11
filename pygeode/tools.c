#include <math.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*************************************
 generic helper functions for Pygeode
*************************************/

// Sort an array, keep track of the original positions in a separate array
// Use merge sort, since it's fairly easy to code
int msort (int na, double *a, int *a_ind, int offset) {

  assert (na >= 1);

  // Base case: only one element
  if (na == 1) {
    a_ind[0] = offset;
    return 0;
  }

  // Cut the array in half, sort each half separately
  int m1 = na / 2;
  int m2 = na - m1;

  msort (m1, a, a_ind, offset);
  msort (m2, a+m1, a_ind+m1, offset+m1);

  // Make a temporary array to store the sorted indices
  double s[na];
  int s_ind[na];

  // Merge the two sorted subarrays together into a single sorted array
  int i = 0;
  int j = m1;
  for (int k = 0; k < na; k++) {
//    assert (i <= j);
    if (j == na || (i < m1 && a[i] <= a[j]) ) {
//      assert (i < m1);
      s[k] = a[i];
      s_ind[k] = a_ind[i];
      i++;
    }
    else {
//      assert (j < na);
      s[k] = a[j];
      s_ind[k] = a_ind[j];
      j++;
    }
  }

  // Copy the values back over
  for (int k = 0; k < na; k++) {
    a[k] = s[k];
    a_ind[k] = s_ind[k];
  }

  return 0;
}

// Sort an array, keep track of the original positions in a separate array
// Use merge sort, since it's fairly easy to code
// (wrapper for the above msort, to first check if the array is already sorted
//  before going into the recursion)
int sort (int na, double *a, int *a_ind) {
  // Check if the array is already sorted
  char sorted = 1;
  char rsorted = 1;
  for (int k = 1; k < na; k++) {
    if (a[k-1] > a[k]) sorted = 0;
    else if (a[k-1] < a[k]) rsorted = 0;
  }
  if (sorted == 1) {
    for (int k = 0; k < na; k++) a_ind[k] = k;
    return 0;
  }
  if (rsorted == 1) {
    for (int k = 0; k < na; k++) a_ind[k] = na-k-1;
    double a_copy[na];
    for (int k = 0; k < na; k++) a_copy[k] = a[na-k-1];
    memcpy (a, a_copy, na*sizeof(double));
    return 0;
  }

  // Otherwise, start the recursive merge sort
  msort (na, a, a_ind, 0);
  return 0;

}


// Return a mapping from array A to array B
// Note: arrays must be double-precision
int map_to (int na, double *a_orig, int nb, double *b_orig, int *indices) {
  // Use same default tolerances as numpy (don't use absolute tolerance, though)
  const double rtol=1e-05;
//  double atol=1e-08;

  double a[na], b[nb];
  int a_ind[na], b_ind[nb];

  //printf("#A: %d, #B %d\n", na, nb);
  // Work with a copy of the arrays (so we can sort them)
  memcpy (a, a_orig, na*sizeof(double));
  memcpy (b, b_orig, nb*sizeof(double));

  // Sort the arrays
  sort (na, a, a_ind);
  sort (nb, b, b_ind);

  // Fill indices with non-matching values (-1)
  for (int k = 0; k < nb; k++) indices[k] = -1;

  int ai = 0;
  for (int bi = 0; bi < nb; bi++) {
    double B = b[bi];
//    double check = atol + rtol * fabs(B);
    // Find a that's closest to b
    while (ai < na-1) {
      if (fabs(a[ai+1]-B) < fabs(a[ai]-B)) ai++;
      else break;
    }
    // Check if it's a match
    //printf("A: %d %e <-> B: %d %e\n", a_ind[ai], a[ai], b_ind[bi], b[bi]);
    if (fabs(a[ai]-B) <= rtol*fabs(B)) indices[b_ind[bi]] = a_ind[ai];
    // Otherwise, check if no more matches are expected
    else if (ai == na-1) continue;
  }


  return 0;
}

// Map between two arrays
// Given two sorted arrays, create 2 integer arrays filled with indices where
// the arrays share common values, and -1 for elements with no common value
// I.e., (1,3,5,7,9),(2,3,5,7) -> (-1,1,2,3,-1),(-1,1,2,3)
int common_map (int na, double *a, int nb, double *b, int *nmap, int *a_map, int *b_map) {

  const double rtol=1e-05;
  const double atol=1e-08;

  int ai = 0, bi = 0;
  int m = 0;
  while (ai < na && bi < nb) {
    int cmp = a[ai] - b[bi];
    // A match?
    if (cmp == 0) {
      a_map[m] = ai;
      b_map[m] = bi;
      m++;
      // Special cases: one of the arrays  has repeated values
      if (ai < na-1) if (a[ai] == a[ai+1]) {
        ai++;
        continue;
      }
      if (bi < nb-1) if (b[bi] == b[bi+1]) {
        bi++;
        continue;
      }
      // If no repeated values, it is safe to increment both arrays
      ai++;
      bi++;
      continue;
    }

    // No match, increment whichever array had the smaller value
    if (cmp < 0) ai++;
    else bi++;
  }

  *nmap = m;

/*
  // Fill indices with non-matching values (-1)
  for (int k = 0; k < na; k++) a_map[k] = -1;
  for (int k = 0; k < nb; k++) b_map[k] = -1;

  int ai = 0, bi = 0;
  while (ai < na && bi < nb) {
    // A match?
//    if (fabs(a[ai] - b[bi]) <= atol + rtol * fabs(b[bi])) {
    if (fabs(a[ai] - b[bi]) <= rtol * fabs(b[bi])) {
      a_map[ai] = ai;
      b_map[bi] = bi;
      ai++; bi++;
    }
    else if (a[ai] < b[bi]) ai++;
    else bi++;
  }
*/
 return 0;
}


// Partial sum of an array
#define PARTIAL_SUM(TYPE)						\
int partial_sum_##TYPE (int nx, int nin, int nout, int ny, 		\
                 TYPE *in, TYPE *out, int *count, int *outmap) {	\
  for (int x = 0; x < nx; x++) {					\
    TYPE *in_ = in + x*nin*ny;						\
    TYPE *out_ = out + x*nout*ny;					\
    int *count_ = count + x*nout*ny;					\
    for (int i = 0; i < nin; i++) {					\
      TYPE *in__ = in_ + i*ny;						\
      TYPE *out__ = out_ + outmap[i]*ny;				\
      int *count__ = count_ + outmap[i]*ny;				\
      for (int y = 0; y < ny; y++) {					\
        out__[y] += in__[y];						\
        count__[y] += 1;						\
      }									\
    }									\
  }									\
  return 0;								\
}

typedef float float32;
typedef double float64;
typedef int int32;
typedef long long int64;
PARTIAL_SUM (float32);
PARTIAL_SUM (float64);
PARTIAL_SUM (int32);
PARTIAL_SUM (int64);

typedef struct {
  float32 real;
  float32 imag;
} complex64;

typedef struct {
  float64 real;
  float64 imag;
} complex128;

#define PARTIAL_SUM_COMPLEX(TYPE)					\
int partial_sum_##TYPE (int nx, int nin, int nout, int ny, 		\
                 TYPE *in, TYPE *out, int *count, int *outmap) {	\
  for (int x = 0; x < nx; x++) {					\
    TYPE *in_ = in + x*nin*ny;						\
    TYPE *out_ = out + x*nout*ny;					\
    int *count_ = count + x*nout*ny;					\
    for (int i = 0; i < nin; i++) {					\
      TYPE *in__ = in_ + i*ny;						\
      TYPE *out__ = out_ + outmap[i]*ny;				\
      int *count__ = count_ + outmap[i]*ny;				\
      for (int y = 0; y < ny; y++) {					\
        out__[y].real += in__[y].real;					\
        out__[y].imag += in__[y].imag;					\
        count__[y] += 1;						\
      }									\
    }									\
  }									\
  return 0;								\
}

PARTIAL_SUM_COMPLEX (complex64);
PARTIAL_SUM_COMPLEX (complex128);

