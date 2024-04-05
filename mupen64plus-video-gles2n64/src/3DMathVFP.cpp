#include "3DMath.h"

// Clobber lists for vfp registers
#define VFP_CLOBBER_HALF1 \
    "s0", "s1", "s2", "s3"

#define VFP_CLOBBER_HALF2 \
    "s8", "s9", "s10", "s11"

#define VFP_CLOBBER_HALF3 \
    "s16", "s17", "s18", "s19"

#define VFP_CLOBBER_HALF4 \
    "s24", "s25", "s26", "s27"

#define VFP_CLOBBER_BANK1 \
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7"

#define VFP_CLOBBER_BANK2 \
    "s8", "s9", "s10", "s11", "s12", "s13", "s14", "s15"

#define VFP_CLOBBER_BANK3 \
    "s16", "s17", "s18", "s19", "s20", "s21", "s22", "s23"

#define VFP_CLOBBER_BANK4 \
    "s24", "s25", "s26", "s27", "s28", "s29", "s30", "s31"

#define VFP_CLOBBER_ALL \
    VFP_CLOBBER_BANK1, \
    VFP_CLOBBER_BANK2, \
    VFP_CLOBBER_BANK3, \
    VFP_CLOBBER_BANK4

// Reset vector length & stride
#define VFP_VEC_LEN_RESET \
    "fmrx       r0, fpscr           \n\t" \
    "bic        r0, r0, #0x00370000 \n\t" \
    "fmxr       fpscr, r0           \n\t"

// Set vector length. VEC_LENGTH must be between 0 and 3. The VFP vector
// length will be set to VEC_LENGTH+1
#define VFP_VEC_LEN(VEC_LEN) \
    "fmrx       r0, fpscr                      \n\t" \
    "bic        r0, r0, #0x00370000            \n\t" \
    "orr        r0, r0, #0x000" #VEC_LEN "0000 \n\t" \
    "fmxr       fpscr, r0                      \n\t"

static void MultMatrix_vfp(float m0[4][4], float m1[4][4], float dest[4][4])
{
    // m0*m1 computed by (m1^T * m0^T)^T
    // loads, saves and muls/fmacs are interleaved to better utilise pipelines

    asm volatile (
        VFP_VEC_LEN(3)                                 // Set vector length to 4.

        "fldmias        %[m0], {s8-s23}       \n\t"    // Load m0 into memory

        "fldmias        %[m1]!, {s0-s3}       \n\t"    // Load 1st col of m1 into scalar bank.
        "fmuls          s24, s8, s0           \n\t"    // 1st col m1 * m0.
        "fldmias        %[m1]!, {s4-s7}       \n\t"    // Load 1st col of m1 into scalar bank.
        "fmacs          s24, s12, s1          \n\t"    // 1st col m1 * m0.

        "fmuls          s28, s16, s6          \n\t"    // 2nd col m1 * m0.
        "fmacs          s24, s16, s2          \n\t"    // 1st col m1 * m0.
        "fmacs          s28, s20, s7          \n\t"    // 2nd col m1 * m0.
        "fmacs          s24, s20, s3          \n\t"    // 1st col m1 * m0.

        "fstmias        %[dest]!, {s24-s27}   \n\t"    // Save 1st column.
        "fmacs          s28, s8, s4           \n\t"    // 2nd col m1 * m0.
        "fmacs          s28, s12, s5          \n\t"    // 2nd col m1 * m0.

        "fldmias        %[m1]!, {s0-s3}       \n\t"    // Load 3rd column to scalar bank.
        "fstmias        %[dest]!, {s28-s31}   \n\t"    // Save 2nd column.
        "fmuls          s24, s8, s0           \n\t"    // 3rd col m1 * m0.
        "fldmias        %[m1],  {s4-s7}       \n\t"    // Load 4th column to scalar bank.
        "fmacs          s24, s12, s1          \n\t"    // 3rd col m1 * m0.
        
        "fmuls          s28, s16, s6          \n\t"    // 4th col m1 * m0.
        "fmacs          s24, s16, s2          \n\t"    // 3rd col m1 * m0.
        "fmacs          s28, s20, s7          \n\t"    // 4th col m1 * m0.
        "fmacs          s24, s20, s3          \n\t"    // 3rd col m1 * m0.

        "fstmias        %[dest]!, {s24-s27}   \n\t"    // Save 3rd column.
        "fmacs          s28, s8, s4           \n\t"    // 4th col m1 * m0.
        "fmacs          s28, s12, s5          \n\t"    // 4th col m1 * m0.

        "fstmias        %[dest], {s28-s31}    \n\t"    // Save 4th column.

        VFP_VEC_LEN_RESET
        : [dest] "+r" (dest), [m1] "+r" (m1), [m0] "+r" (m0)
        :
        : "r0", "cc", "memory", VFP_CLOBBER_ALL
    );
}

static void TransformVectorNormalize_vfp(float vec[3], float mtx[4][4])
{
    // transforms vector 'vec' using tranform matrix 'mtx' and normalises the resultant vector
    // Loads and muls/fmacs are interleaved to better utilise the pipelines
    volatile float one = 1.0f;
    volatile void* p_one = &one;
    asm volatile(
        "flds           s0, [%[v], #0]   \n\t"    // load vec[0]
        "fldmias        %[m]!, {s1-s3}   \n\t"    // load mtx[0][0] to mtx[0][2]
        "add            %[m], %[m], #4   \n\t"    // skip mtx[0][3]

        "fmuls          s12, s0, s1      \n\t"    // s12 = vec[0] * m[0][0]
        "flds           s4, [%[v], #4]   \n\t"    // load vec[1]
        "fmuls          s13, s0, s2      \n\t"    // s13 = vec[0] * m[0][1]

        "fldmias        %[m]!, {s5-s7}   \n\t"    // load mtx[1][0] to mtx[1][2]
        "add            %[m], %[m], #4   \n\t"    // skip mtx[1][3]
        "fmuls          s14, s0, s3      \n\t"    // s14 = vec[0] * m[0][2]
        "fmacs          s12, s4, s5      \n\t"    // s12 += vec[1] * mtx[1][0]

        "flds           s8, [%[v], #8]   \n\t"    // load vec[2]
        "fmacs          s13, s4, s6      \n\t"    // s13 += vec[1] * mtx[1][1]
        "fldmias        %[m], {s9-s11}   \n\t"    // load mtx into vec regs
        "fmacs          s14, s4, s7      \n\t"    // s14 += vec[2] * mtx[1][2]

        "fmacs          s12, s8, s9      \n\t"    // s12 += vec[2] * mtx[2][0]
        "fmacs          s13, s8, s10     \n\t"    // s13 += vec[2] * mtx[2][1]
        "fmacs          s14, s8, s11     \n\t"    // s14 += vec[2] * mtx[2][2]
        
        "flds           s15, [%[one], #0]\n\t"    // load 1.0 into s15

        "fmuls          s1, s12, s12     \n\t"
        "fmacs          s1, s13, s13     \n\t"
        "fmacs          s1, s14, s14     \n\t"    // s1 = x*x + y*y + z*z

        "fmrs           r0, s1           \n\t"    // move s1 to arm reg
        "cmp            r0, #0           \n\t"    // if r0 == 0:
        "beq            1f               \n\t"    //   return

        "fsqrts         s1, s1           \n\t"    // s1 = sqrt(s1) == len(v_trans)
        "fdivs          s15, s15, s1     \n\t"

        "fmuls          s12, s12, s15    \n\t"    // normalise v_trans
        "fmuls          s13, s13, s15    \n\t"    // normalise v_trans
        "fmuls          s14, s14, s15    \n\t"    // normalise v_trans

        "fstmias        %[v], {s12-s14}  \n\t"
        "1:;                             \n\t"

        : [v] "+r" (vec), [m] "+r" (mtx), [one] "+r" (p_one)
        : 
        : "r0", "cc", "memory", VFP_CLOBBER_BANK1, VFP_CLOBBER_BANK2
    );
}

void MathInitVFP()
{
    MultMatrix = MultMatrix_vfp;
    TransformVectorNormalize = TransformVectorNormalize_vfp;
}
