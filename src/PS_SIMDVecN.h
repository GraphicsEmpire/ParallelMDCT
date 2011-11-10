#ifndef PS_SIMDVECN_H
#define PS_SIMDVECN_H

#include <malloc.h>
#include "PS_VectorMath.h"

#define __OPTIMIZE__ 1
//Enable the SIMD Length Here
#define SIMD_USE_M128
//#define SIMD_USE_M256
//#define SIMD_USE_M512

#ifndef PS_L1_CACHE_LINE_SIZE
#define PS_L1_CACHE_LINE_SIZE 64
#endif


/////////////////////////////////////////////////////////////////////////
//Global Functions

//CPUID handy function to read Processor features
#if defined(_MSC_VER)
	#define cpuid(func,a,b,c,d)\
		asm {\
		mov	eax, func\
		cpuid\
		mov	a, eax\
		mov	b, ebx\
		mov	c, ecx\
		mov	d, edx\
		}
#elif defined(__GNUC__)
	#define cpuid(func,ax,bx,cx,dx)\
		__asm__ __volatile__ ("cpuid":\
		"=a" (ax), "=b" (bx), "=c" (cx), "=d" (dx) : "a" (func));
#endif

/*
#define SupportFeature(CONST)\
   __asm__ __volatile__ ("; result returned in eax":\
   "mov eax, 1":\
   "cpuid":\
   "and ecx," (CONST) :\
   cmp ecx, CONSTANT; check desired feature flags
   jne not_supported
   ; processor supports features
   mov ecx, 0; specify 0 for XFEATURE_ENABLED_MASK register
   XGETBV; result in EDX:EAX
   and eax, 06H
   cmp eax, 06H; check OS has enabled both XMM and YMM state support
   jne not_supported
   mov eax, 1; mark as supported
   jmp done
   NOT_SUPPORTED:
   mov eax, 0 ; // mark as not supported
   done:
*/


#define OSXSAVEFlag (1UL<<27)
#define AVXFlag     ((1UL<<28)|OSXSAVEFlag)
#define FMAFlag     ((1UL<<12)|AVXFlag|OSXSAVEFlag)
#define CLMULFlag   ((1UL<< 1)|AVXFlag|OSXSAVEFlag)
#define VAESFlag    ((1UL<<25)|AVXFlag|OSXSAVEFlag)



inline bool SimdDetectFeature(U32 idFeature)
{
	int EAX, EBX, ECX, EDX;
	cpuid(0, EAX, EBX, ECX, EDX);
	if((ECX & idFeature) != idFeature)
		return false;
	return true;
}

/*
inline U64 GetTicks()
{
     unsigned a, d;
     asm("cpuid");
     asm volatile("rdtsc" : "=a" (a), "=d" (d));

     return (((U64)a) | (((U64)d) << 32));
}
*/


// Memory Allocation Functions
inline void *AllocAligned(U32 size)
{
#if defined(PS_IS_WINDOWS)
    return _aligned_malloc(size, PS_L1_CACHE_LINE_SIZE);
#elif defined (PS_IS_OPENBSD) || defined(PS_IS_APPLE)
    // Allocate excess memory to ensure an aligned pointer can be returned
    void *mem = malloc(size + (PS_L1_CACHE_LINE_SIZE-1) + sizeof(void*));
    char *amem = ((char*)mem) + sizeof(void*);
#if (PS_POINTER_SIZE == 8)
    amem += PS_L1_CACHE_LINE_SIZE - (reinterpret_cast<uint64_t>(amem) &
    								(PS_L1_CACHE_LINE_SIZE - 1));
#else
    amem += PS_L1_CACHE_LINE_SIZE - (reinterpret_cast<uint32_t>(amem) &
                                       (PS_L1_CACHE_LINE_SIZE - 1));
#endif
    ((void**)amem)[-1] = mem;
    return amem;
#else
    return memalign(PS_L1_CACHE_LINE_SIZE, size);
#endif
}

template <typename T> T *AllocAligned(U32 count) {
    return (T *)AllocAligned(count * sizeof(T));
}


inline void FreeAligned(void *ptr)
{
    if (!ptr) return;
#if defined(PBRT_IS_WINDOWS)
    _aligned_free(ptr);
#elif defined (PBRT_IS_OPENBSD) || defined(PBRT_IS_APPLE)
    free(((void**)ptr)[-1]);
#else
    free(ptr);
#endif
}
/////////////////////////////////////////////////////////////////////////

//128 bit SIMD
#ifdef SIMD_USE_M128
	#include <xmmintrin.h>
	#include <x86intrin.h>

	#define PS_SIMD_FLEN	4
	#define PS_SIMD_ALIGN_SIZE 16

	inline const __m128 fast_pow( const __m128& base, const __m128& exponent)
	{
		__m128 denom = _mm_mul_ps( exponent, base);
		denom = _mm_sub_ps( exponent, denom);
		denom = _mm_add_ps( base, denom);
		return _mm_mul_ps( base, _mm_rcp_ps(denom));
	}

#endif

//256 bit SIMD
#ifdef SIMD_USE_M256
	#include <immintrin.h>
	//#include <intrin.h>

	#define PS_SIMD_FLEN	8
	#define PS_SIMD_ALIGN_SIZE 32

	inline const __m256 fast_pow( const __m256& base, const __m256& exponent)
	{
		__m256 denom = _mm256_mul_ps( exponent, base);
		denom = _mm256_sub_ps( exponent, denom);
		denom = _mm256_add_ps( base, denom);
		return _mm256_mul_ps( base, _mm256_rcp_ps(denom));
	}

#endif





// gives the number of SIMD blocks necessary for _SIZE_ elements
#define	PS_SIMD_BLOCKS(_SIZE_)			(((unsigned)(_SIZE_) + (PS_SIMD_FLEN-1)) / PS_SIMD_FLEN)

// gives the block index from the global index _GLOB_IDX_
#define	PS_SIMD_BLK_IDX(_GLOB_IDX_)	(((unsigned)(_GLOB_IDX_)) / PS_SIMD_FLEN)

// gives the index of a SIMD block element from the global index _GLOB_IDX_
#define	PS_SIMD_SUB_IDX(_GLOB_IDX_)	(((unsigned)(_GLOB_IDX_)) & (PS_SIMD_FLEN-1))

// gives the size padded to the SIMD length
#define	PS_SIMD_PADSIZE(_SIZE_)		(((unsigned)(_SIZE_) + (PS_SIMD_FLEN-1)) & ~(PS_SIMD_FLEN-1))

#if defined(_MSC_VER)
	#define PS_SIMD_ALIGN( _X_ )	__declspec(align(PS_SIMD_ALIGN_SIZE))	_X_
	#define PACKED
	#define PS_BEGIN_ALIGNED(_x) __declspec(align(_x))
	#define PS_END_ALIGNED(_x)
#elif defined(__GNUC__)
	#define PS_SIMD_ALIGN( _X_ )	_X_ __attribute__ ((aligned(PS_SIMD_ALIGN_SIZE)))
	#define PACKED __attribute__ ((packed))
	#define PS_BEGIN_ALIGNED(_x)
	#define PS_END_ALIGNED(_x) __attribute__ ((aligned(_x)))

#endif


//==================================================================
/// VecN
//==================================================================

//==================================================================
#define FOR_I_N	for (int i=0; i<_N; ++i)

template<class _S, size_t _N>
class VecN
{
public:
	_S	v[_N];

	//==================================================================
	VecN()						{}
	VecN( const VecN &v_ )		{ FOR_I_N v[i] = v_.v[i]; }
	VecN( const _S& a_ )		{ FOR_I_N v[i] = a_;		}
	VecN( const _S *p_ )		{ FOR_I_N v[i] = p_[i];	}

	void set( const _S *p_ )	{ FOR_I_N v[i] = p_[i];	}
	void setZero()				{ FOR_I_N v[i] = 0;		}


	VecN operator + (const _S& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] + rval; return tmp; }
	VecN operator - (const _S& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] - rval; return tmp; }
	VecN operator * (const _S& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] * rval; return tmp; }
	VecN operator / (const _S& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] / rval; return tmp; }

	VecN operator + (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] + rval.v[i]; return tmp; }
	VecN operator - (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] - rval.v[i]; return tmp; }
	VecN operator * (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] * rval.v[i]; return tmp; }
	VecN operator / (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] / rval.v[i]; return tmp; }

	VecN operator -() const	{ VecN tmp; FOR_I_N tmp.v[i] = -v[i]; return tmp; }

	VecN operator +=(const VecN &rval)	{ *this = *this + rval; return *this; }

	const _S &operator [] (size_t i) const	{ return v[i]; }
		  _S &operator [] (size_t i)		{ return v[i]; }
};

template <class _S, size_t _N> VecN<_S,_N> operator + (const _S &lval, const VecN<_S,_N> &rval) { return rval + lval; }
template <class _S, size_t _N> VecN<_S,_N> operator - (const _S &lval, const VecN<_S,_N> &rval) { return rval - lval; }
template <class _S, size_t _N> VecN<_S,_N> operator * (const _S &lval, const VecN<_S,_N> &rval) { return rval * lval; }
template <class _S, size_t _N> VecN<_S,_N> operator / (const _S &lval, const VecN<_S,_N> &rval) { return rval / lval; }

/*
template <class _S, size_t _N> VecNMask CmpMaskLT( const VecN<_S,_N> &lval, const VecN<_S,_N> &rval ) { DASSERT( 0 ); return VecNMaskEmpty; }
template <class _S, size_t _N> VecNMask CmpMaskGT( const VecN<_S,_N> &lval, const VecN<_S,_N> &rval ) { DASSERT( 0 ); return VecNMaskEmpty; }
template <class _S, size_t _N> VecNMask CmpMaskEQ( const VecN<_S,_N> &lval, const VecN<_S,_N> &rval ) { DASSERT( 0 ); return VecNMaskEmpty; }
template <class _S, size_t _N> VecNMask CmpMaskNE( const VecN<_S,_N> &lval, const VecN<_S,_N> &rval ) { DASSERT( 0 ); return VecNMaskEmpty; }
template <class _S, size_t _N> VecNMask CmpMaskLE( const VecN<_S,_N> &lval, const VecN<_S,_N> &rval ) { DASSERT( 0 ); return VecNMaskEmpty; }
template <class _S, size_t _N> VecNMask CmpMaskGE( const VecN<_S,_N> &lval, const VecN<_S,_N> &rval ) { DASSERT( 0 ); return VecNMaskEmpty; }
*/

//==================================================================
#undef FOR_I_N
#define FOR_I_N	for (int i=0; i<PS_SIMD_FLEN; ++i)

#ifdef SIMD_USE_M128

struct VecNMask
{

	union
	{
		U32		    	m[PS_SIMD_FLEN];
		__m128			v;
	} u;

	VecNMask()						{}

	VecNMask( const __m128 &from )	{	u.v = from; }

	VecNMask( unsigned int a, unsigned int b, unsigned int c, unsigned int d )
	{
		u.m[0] = a;
		u.m[1] = b;
		u.m[2] = c;
		u.m[3] = d;
	}

	friend VecNMask CmpMaskEQ( const VecNMask &lval, const VecNMask &rval ) { return _mm_cmpeq_ps( lval.u.v, rval.u.v );  }
	friend VecNMask CmpMaskNE( const VecNMask &lval, const VecNMask &rval ) { return _mm_cmpneq_ps( lval.u.v, rval.u.v ); }

	friend bool operator ==( const VecNMask &lval, const VecNMask &rval )
	{
		__m128	tmp = _mm_xor_ps( lval.u.v, rval.u.v );

		unsigned int	folded =  ((const unsigned int *)&tmp)[0]
								| ((const unsigned int *)&tmp)[1]
								| ((const unsigned int *)&tmp)[2]
								| ((const unsigned int *)&tmp)[3];
		return !folded;
	}

	friend bool operator !=( const VecNMask &lval, const VecNMask &rval )
	{
		return !(lval == rval);
	}

	VecNMask operator & ( const VecNMask &rval ) {	return _mm_and_ps( u.v, rval.u.v ); }
	VecNMask operator | ( const VecNMask &rval ) {	return _mm_or_ps( u.v, rval.u.v ); }
};

static const VecNMask PS_SIMD_ALLONE( 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF );
static const VecNMask PS_SIMD_ALLZERO( 0, 0, 0, 0 );

static const VecNMask VecNMaskFull = PS_SIMD_ALLONE.u.v;
static const VecNMask VecNMaskEmpty = PS_SIMD_ALLZERO.u.v;

#endif

//===================================================================================
#ifdef SIMD_USE_M256

struct VecNMask
{

	union
	{
		U32				m[PS_SIMD_FLEN];
		__m256			v;
	} u;

	VecNMask()						{}

	VecNMask( const __m256 &from )	{	u.v = from; }

	VecNMask( U32 a, U32 b, U32 c, U32 d,
			  U32 e, U32 f, U32 g, U32 h)
	{
		u.m[0] = a;
		u.m[1] = b;
		u.m[2] = c;
		u.m[3] = d;
		u.m[4] = e;
		u.m[5] = f;
		u.m[6] = g;
		u.m[7] = h;
	}

	friend VecNMask CmpMaskEQ( const VecNMask &lval, const VecNMask &rval ) { return _mm256_cmp_ps( lval.u.v, rval.u.v, _CMP_EQ_OQ ); }
	friend VecNMask CmpMaskNE( const VecNMask &lval, const VecNMask &rval ) { return _mm256_cmp_ps( lval.u.v, rval.u.v, _CMP_NEQ_OQ ); }

	friend bool operator ==( const VecNMask &lval, const VecNMask &rval )
	{
		__m256	tmp = _mm256_xor_ps( lval.u.v, rval.u.v );

		unsigned int	folded =  ((const unsigned int *)&tmp)[0]
								| ((const unsigned int *)&tmp)[1]
								| ((const unsigned int *)&tmp)[2]
								| ((const unsigned int *)&tmp)[3]
                                | ((const unsigned int *)&tmp)[4]
                                | ((const unsigned int *)&tmp)[5]
   								| ((const unsigned int *)&tmp)[6]
   								| ((const unsigned int *)&tmp)[7];
		return !folded;
	}

	friend bool operator !=( const VecNMask &lval, const VecNMask &rval )
	{
		return !(lval == rval);
	}

	VecNMask operator & ( const VecNMask &rval ) {	return _mm256_and_ps( u.v, rval.u.v ); }
	VecNMask operator | ( const VecNMask &rval ) {	return _mm256_or_ps( u.v, rval.u.v ); }
};

static const VecNMask PS_SIMD_ALLONE( 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF,
									  0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF);
static const VecNMask PS_SIMD_ALLZERO( 0, 0, 0, 0, 0, 0, 0, 0 );

static const VecNMask VecNMaskFull = PS_SIMD_ALLONE.u.v;
static const VecNMask VecNMaskEmpty = PS_SIMD_ALLZERO.u.v;

#endif


//=====================================================================
// Float SIMD
template<>
class VecN<float,PS_SIMD_FLEN>
{
//==================================================================
/// 128 bit 4-way float SIMD
//==================================================================
#ifdef SIMD_USE_M128
public:
	__m128	v;

	//==================================================================
	VecN()						{}
	VecN( const __m128 &v_ )	{ v = v_; }
	VecN( const VecN &v_ )		{ v = v_.v; }
	VecN( float a_ )			{ v = _mm_set_ps1( a_ );	}
	VecN( const float *p_ )		{ v = _mm_load_ps( p_ );	}

	void setZero()				{ v = _mm_setzero_ps();		}
	void store(float* lpAlignedDest) 			{ _mm_store_ps(lpAlignedDest, v);}

	VecN operator + (const float& rval) const	{ return _mm_add_ps( v, _mm_set_ps1( rval )	); }
	VecN operator - (const float& rval) const	{ return _mm_sub_ps( v, _mm_set_ps1( rval )	); }
	VecN operator * (const float& rval) const	{ return _mm_mul_ps( v, _mm_set_ps1( rval )	); }
	VecN operator / (const float& rval) const	{ return _mm_div_ps( v, _mm_set_ps1( rval )	); }
	VecN operator + (const VecN &rval) const	{ return _mm_add_ps( v, rval.v	); }
	VecN operator - (const VecN &rval) const	{ return _mm_sub_ps( v, rval.v	); }
	VecN operator * (const VecN &rval) const	{ return _mm_mul_ps( v, rval.v	); }
	VecN operator / (const VecN &rval) const	{ return _mm_div_ps( v, rval.v	); }

	VecN operator -() const						{ return _mm_sub_ps( _mm_setzero_ps(), v ); }

	VecN operator +=(const VecN &rval)			{ *this = *this + rval; return *this; }

	const float &operator [] (size_t i) const	{ return ((const float *)&v)[i]; }
		  float &operator [] (size_t i)			{ return ((float *)&v)[i]; }


    //Comparison Operations
	friend inline VecN SimdMin( const VecN &a, const VecN &b )	{	return _mm_min_ps( a.v, b.v );	}
	friend inline VecN SimdMax( const VecN &a, const VecN &b )	{	return _mm_max_ps( a.v, b.v );	}

	friend inline VecN SimdEQ( const VecN &lval, const VecN &rval ) { return _mm_cmpeq_ps( lval.v, rval.v );  }
	friend inline VecN SimdGT( const VecN &lval, const VecN &rval ) { return _mm_cmpgt_ps( lval.v, rval.v );  }
	friend inline VecN SimdGTE( const VecN &lval, const VecN &rval ) { return _mm_cmpge_ps( lval.v, rval.v );  }

	friend inline VecNMask SimdCmpEQ( const VecN &lval, const VecN &rval ) { return _mm_cmpeq_ps( lval.v, rval.v );  }
    friend inline VecNMask SimdCmpGT( const VecN &lval, const VecN &rval ) { return _mm_cmpgt_ps( lval.v, rval.v );  }
    friend inline VecNMask SimdCmpGTE( const VecN &lval, const VecN &rval ) { return _mm_cmpge_ps( lval.v, rval.v );  }

    //Vector Arithmatic
	friend inline VecN	SimdSqrt( const VecN &a )	{ return _mm_sqrt_ps( a.v );	}
	friend inline VecN	SimdRSqrt( const VecN &a )	{ return _mm_rsqrt_ps( a.v );	}

	friend inline VecN	SimdPower( const VecN &lval, const VecN &rval )
	{
		return fast_pow(lval.v, rval.v);
	}


	//Logical
	friend inline VecN	SimdAnd( const VecN &lval, const VecN &rval )	{ return _mm_and_ps( lval.v, rval.v );	}
	friend inline VecN	SimdOr ( const VecN &lval, const VecN &rval )	{ return _mm_or_ps( lval.v, rval.v );	}
	friend inline VecN	SimdXor( const VecN &lval, const VecN &rval )	{ return _mm_xor_ps( lval.v, rval.v );	}




	friend void SimdNormalize(VecN& nX, VecN& nY, VecN& nZ)
	{
		//TODO: Handle problem when Magnitude is zero
		VecN invMag = SimdRSqrt(nX * nX + nY * nY + nZ * nZ);
		nX = nX * invMag;
		nY = nY * invMag;
		nZ = nZ * invMag;
	}


#elif defined(SIMD_USE_M256)
	//==================================================================
	/// 256 bit 8-way float SIMD
	//==================================================================
public:
	__m256	v;

	//==================================================================
	VecN()						{}
	VecN( const __m256 &v_ )	{ v = v_; }
	VecN( const VecN &v_ )		{ v = v_.v; }
	VecN( float a_ )		{ v = _mm256_set1_ps( a_ );	}

	//Loads Aligned
	VecN( const float *p_ )		{ v = _mm256_load_ps(p_);  }


	void setZero()				{ v = _mm256_setzero_ps();		}
	void store(float* lpAlignedDest) 			{ _mm256_store_ps(lpAlignedDest, v);}

	VecN operator + (const float& rval) const	{ return _mm256_add_ps( v, _mm256_set1_ps( rval )	); }
	VecN operator - (const float& rval) const	{ return _mm256_sub_ps( v, _mm256_set1_ps( rval )	); }
	VecN operator * (const float& rval) const	{ return _mm256_mul_ps( v, _mm256_set1_ps( rval )	); }
	VecN operator / (const float& rval) const	{ return _mm256_div_ps( v, _mm256_set1_ps( rval )	); }
	VecN operator + (const VecN &rval) const	{ return _mm256_add_ps( v, rval.v	); }
	VecN operator - (const VecN &rval) const	{ return _mm256_sub_ps( v, rval.v	); }
	VecN operator * (const VecN &rval) const	{ return _mm256_mul_ps( v, rval.v	); }
	VecN operator / (const VecN &rval) const	{ return _mm256_div_ps( v, rval.v	); }

	VecN operator -() const						{ return _mm256_sub_ps( _mm256_setzero_ps(), v ); }

	VecN operator +=(const VecN &rval)			{ *this = *this + rval; return *this; }

	// TODO: verify that it works !
	const float &operator [] (size_t i) const	{ return ((const float *)&v)[i]; }
	float &operator [] (size_t i)			{ return ((float *)&v)[i]; }

    //Comparison Operations
	friend VecN	SimdMin( const VecN &a, const VecN &b )	{	return _mm256_min_ps( a.v, b.v );	}
	friend VecN	SimdMax( const VecN &a, const VecN &b )	{	return _mm256_max_ps( a.v, b.v );	}
	friend VecN SimdEQ( const VecN &lval, const VecN &rval ) { return _mm256_cmp_ps( lval.v, rval.v, _CMP_EQ_OQ );  }
	friend VecN SimdGT( const VecN &lval, const VecN &rval ) { return _mm256_cmp_ps( lval.v, rval.v, _CMP_GT_OQ );  }
	friend VecN SimdGTE( const VecN &lval, const VecN &rval ) { return _mm256_cmp_ps( lval.v, rval.v, _CMP_GE_OQ );  }

	friend VecNMask SimdCmpEQ( const VecN &lval, const VecN &rval ) { return _mm256_cmp_ps( lval.v, rval.v, _CMP_EQ_OQ);  }
	friend VecNMask SimdCmpGT( const VecN &lval, const VecN &rval ) { return _mm256_cmp_ps( lval.v, rval.v, _CMP_GT_OQ);  }
	friend VecNMask SimdCmpGTE( const VecN &lval, const VecN &rval ) { return _mm256_cmp_ps( lval.v, rval.v, _CMP_GE_OQ); }

	//Vector Arithmatic
	friend VecN	SimdSqrt( const VecN &a )	{ return _mm256_sqrt_ps( a.v );	}
	friend VecN	SimdRSqrt( const VecN &a )	{ return _mm256_rsqrt_ps( a.v );	}
	friend inline VecN	SimdPower( const VecN &lval, const VecN &rval )
	{
		return fast_pow(lval.v, rval.v);
	}


	//Logical
	friend VecN	SimdAnd( const VecN &lval, const VecN &rval )	{ return _mm256_and_ps( lval.v, rval.v );	}
	friend VecN	SimdOr ( const VecN &lval, const VecN &rval )	{ return _mm256_or_ps( lval.v, rval.v );	}
	friend VecN	SimdXor( const VecN &lval, const VecN &rval )	{ return _mm256_xor_ps( lval.v, rval.v );	}


	friend void SimdNormalize(VecN& nX, VecN& nY, VecN& nZ)
	{
		//TODO: Handle problem when Magnitude is zero
		VecN invMag = SimdRSqrt(nX * nX + nY * nY + nZ * nZ);
		nX = nX * invMag;
		nY = nY * invMag;
		nZ = nZ * invMag;
	}


#elif defined(DMATH_USE_M512)
//==================================================================
/// 512 bit 16-way float SIMD
//==================================================================
public:
	__m512	v;

	//==================================================================
	VecN()						{}
	explicit VecN( const __m512 &v_ )	{ v = v_; }
	explicit VecN( const VecN &v_ )		{ v = v_.v; }
	explicit VecN( const float& a_ )		{ v = _mm512_set_1to16_ps( a_ );	}
	explicit VecN( const float *p_ )		{ v = _mm512_expandd( (void *)p_, _MM_FULLUPC_NONE, _MM_HINT_NONE ); }	// unaligned

	void set( const float *p_ )	{ v = _mm512_expandd( (void *)p_, _MM_FULLUPC_NONE, _MM_HINT_NONE ); }	// unaligned
	void setZero()				{ v = _mm512_setzero_ps();		}

	float addReduce() const		{ return _mm512_reduce_add_ps( v ); }

	VecN operator + (const float& rval) const	{ return _mm512_add_ps( v, _mm512_set_1to16_ps( rval )	); }
	VecN operator - (const float& rval) const	{ return _mm512_sub_ps( v, _mm512_set_1to16_ps( rval )	); }
	VecN operator * (const float& rval) const	{ return _mm512_mul_ps( v, _mm512_set_1to16_ps( rval )	); }
	VecN operator / (const float& rval) const	{ return _mm512_div_ps( v, _mm512_set_1to16_ps( rval )	); }
	VecN operator + (const VecN &rval) const	{ return _mm512_add_ps( v, rval.v	); }
	VecN operator - (const VecN &rval) const	{ return _mm512_sub_ps( v, rval.v	); }
	VecN operator * (const VecN &rval) const	{ return _mm512_mul_ps( v, rval.v	); }
	VecN operator / (const VecN &rval) const	{ return _mm512_div_ps( v, rval.v	); }

	VecN operator -() const						{ return _mm512_sub_ps( _mm512_setzero_ps(), v ); }

	VecN operator +=(const VecN &rval)			{ *this = *this + rval; return *this; }

	// TODO: verify that it works !
	friend VecNMask CmpMaskLT( const VecN &lval, const VecN &rval ) { return _mm512_cmplt_ps( lval.v, rval.v );  }
	friend VecNMask CmpMaskGT( const VecN &lval, const VecN &rval ) { return ~_mm512_cmple_ps( lval.v, rval.v );  }
	friend VecNMask CmpMaskEQ( const VecN &lval, const VecN &rval ) { return _mm512_cmpeq_ps( lval.v, rval.v );  }
	friend VecNMask CmpMaskNE( const VecN &lval, const VecN &rval ) { return _mm512_cmpneq_ps( lval.v, rval.v ); }
	friend VecNMask CmpMaskLE( const VecN &lval, const VecN &rval ) { return _mm512_cmple_ps( lval.v, rval.v );  }
	friend VecNMask CmpMaskGE( const VecN &lval, const VecN &rval ) { return ~_mm512_cmplt_ps( lval.v, rval.v );  }

	friend bool operator ==( const VecN &lval, const VecN &rval ) { return VecNMaskEmpty == CmpMaskNE( lval, rval ); }
	friend bool operator !=( const VecN &lval, const VecN &rval ) { return VecNMaskFull  != CmpMaskEQ( lval, rval ); }

	const float &operator [] (size_t i) const	{ return v.v[i]; }
		  float &operator [] (size_t i)			{ return v.v[i]; }


	friend VecN	DSqrt( const VecN &a )	{ return _mm512_sqrt_ps( a.v );	}
	friend VecN	DRSqrt( const VecN &a )	{ return _mm512_rsqrt_ps( a.v );	}

	friend VecN	DPow( const VecN &a, const VecN &b ){ return _mm512_pow_ps( a.v, b.v );	}

	//==================================================================
	friend VecN	DSign( const VecN &a )
	{
		const __m512	zero		= _mm512_setzero_ps();
		const __mmask	selectPos	= _mm512_cmpnle_ps( a.v, zero );	// >
		const __mmask	selectNeg	= _mm512_cmplt_ps( a.v, zero );		// <

		__m512	res = _mm512_mask_movd( zero, selectPos, _mm512_set_1to16_ps(  1.0f ) );
				res = _mm512_mask_movd( res , selectNeg, _mm512_set_1to16_ps( -1.0f ) );

		return res;
	}

	friend VecN	DAbs( const VecN &a )
	{
		return _mm512_maxabs_ps( a.v, a.v );
	}

	friend VecN	DMin( const VecN &a, const VecN &b )	{	return _mm512_min_ps( a.v, b.v );	}
	friend VecN	DMax( const VecN &a, const VecN &b )	{	return _mm512_max_ps( a.v, b.v );	}

#else
//==================================================================
/// No hardware SIMD
//==================================================================
public:
	float	v[PS_SIMD_FLEN];

	//==================================================================
	VecN()						{}
	VecN( const VecN &v_ )		{ FOR_I_N v[i] = v_.v[i]; }
	VecN( const float& a_ )		{ FOR_I_N v[i] = a_;		}
	VecN( const float *p_ )		{ FOR_I_N v[i] = p_[i];	}

	//void Set( const float *p_ )	{ FOR_I_N v[i] = p_[i];	}
	void SetZero()				{ FOR_I_N v[i] = 0;		}

	float	AddReduce() const		{ float acc=float(0); FOR_I_N acc += v[i]; return acc; }

	VecN operator + (const float& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] + rval; return tmp; }
	VecN operator - (const float& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] - rval; return tmp; }
	VecN operator * (const float& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] * rval; return tmp; }
	VecN operator / (const float& rval) const	{ VecN tmp; FOR_I_N tmp.v[i] = v[i] / rval; return tmp; }
	VecN operator + (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] + rval.v[i]; return tmp; }
	VecN operator - (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] - rval.v[i]; return tmp; }
	VecN operator * (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] * rval.v[i]; return tmp; }
	VecN operator / (const VecN &rval) const{ VecN tmp; FOR_I_N tmp.v[i] = v[i] / rval.v[i]; return tmp; }

	VecN operator -() const	{ VecN tmp; FOR_I_N tmp.v[i] = -v[i]; return tmp; }

	VecN operator +=(const VecN &rval)	{ *this = *this + rval; return *this; }

	friend VecNMask CmpMaskLT( const VecN &lval, const VecN &rval ) { VecNMask mask=0; FOR_I_N mask |= (lval[i] < rval[i] ? (1<<i) : 0); return mask; }
	friend VecNMask CmpMaskGT( const VecN &lval, const VecN &rval ) { VecNMask mask=0; FOR_I_N mask |= (lval[i] > rval[i] ? (1<<i) : 0); return mask; }
	friend VecNMask CmpMaskEQ( const VecN &lval, const VecN &rval ) { VecNMask mask=0; FOR_I_N mask |= (lval[i] ==rval[i] ? (1<<i) : 0); return mask; }
	friend VecNMask CmpMaskNE( const VecN &lval, const VecN &rval ) { VecNMask mask=0; FOR_I_N mask |= (lval[i] !=rval[i] ? (1<<i) : 0); return mask; }
	friend VecNMask CmpMaskLE( const VecN &lval, const VecN &rval ) { VecNMask mask=0; FOR_I_N mask |= (lval[i] <=rval[i] ? (1<<i) : 0); return mask; }
	friend VecNMask CmpMaskGE( const VecN &lval, const VecN &rval ) { VecNMask mask=0; FOR_I_N mask |= (lval[i] >=rval[i] ? (1<<i) : 0); return mask; }

	friend bool operator ==( const VecN &lval, const VecN &rval ) { return VecNMaskEmpty == CmpMaskNE( lval, rval ); }
	friend bool operator !=( const VecN &lval, const VecN &rval ) { return VecNMaskFull  != CmpMaskEQ( lval, rval ); }

	const float &operator [] (size_t i) const	{ return v[i]; }
		  float &operator [] (size_t i)		{ return v[i]; }

	friend VecN	DSqrt( const VecN &a )				 { VecN tmp; FOR_I_N tmp[i] = DSqrt( a[i] ); return tmp; }
	friend VecN	DRSqrt( const VecN &a )				 { VecN tmp; FOR_I_N tmp[i] = DRSqrt( a[i] ); return tmp; }
	friend VecN	DPow( const VecN &a,  const VecN &b ){ VecN tmp; FOR_I_N tmp[i] = DPow( a[i], b[i] ); return tmp; }
	friend VecN	DSign( const VecN &a )				 { VecN tmp; FOR_I_N tmp[i] = DSign( a[i] ); return tmp; }
	friend VecN	DAbs( const VecN &a	)				 { VecN tmp; FOR_I_N tmp[i] = DAbs( a[i] ); return tmp; }
	friend VecN	DMin( const VecN &a, const VecN &b ) { VecN tmp; FOR_I_N tmp[i] = DMin( a[i], b[i] ); return tmp; }
	friend VecN	DMax( const VecN &a, const VecN &b ) { VecN tmp; FOR_I_N tmp[i] = DMax( a[i], b[i] ); return tmp; }
	//friend VecN	DSin( const VecN &a )				 { VecN tmp; FOR_I_N tmp[i] = DSin( a[i] ); return tmp; }
	//friend VecN	DCos( const VecN &a )				 { VecN tmp; FOR_I_N tmp[i] = DCos( a[i] ); return tmp; }

#endif

};

#undef FOR_I_N

//==================================================================
#if defined(_MSC_VER)
typedef __declspec(align(PS_SIMD_ALIGN_SIZE)) VecN<float,PS_SIMD_FLEN>			Float_;
typedef __declspec(align(PS_SIMD_ALIGN_SIZE)) VecN<int,PS_SIMD_FLEN>			Int_;
typedef __declspec(align(PS_SIMD_ALIGN_SIZE)) VecN<U8,PS_SIMD_FLEN>			Bool_;

typedef __declspec(align(PS_SIMD_ALIGN_SIZE)) vec2< VecN<float,PS_SIMD_FLEN> >	Float2_;
typedef __declspec(align(PS_SIMD_ALIGN_SIZE)) vec3< VecN<float,PS_SIMD_FLEN> >	Float3_;
typedef __declspec(align(PS_SIMD_ALIGN_SIZE)) vec4< VecN<float,PS_SIMD_FLEN> >	Float4_;

#elif defined(__GNUC__)

//#define PS_SIMD_ALIGN_BEGIN()
//#define PS_SIMD_ALIGN_END)

typedef	VecN<float,PS_SIMD_FLEN>			Float_ __attribute__ ((aligned(PS_SIMD_ALIGN_SIZE)));
typedef	VecN<int,PS_SIMD_FLEN>				Int_ __attribute__ ((aligned(PS_SIMD_ALIGN_SIZE)));
typedef	VecN<U8,PS_SIMD_FLEN>				Bool_ __attribute__ ((aligned(PS_SIMD_ALIGN_SIZE)));

typedef	vec2< VecN<float,PS_SIMD_FLEN> >	Float2_ __attribute__ ((aligned(PS_SIMD_ALIGN_SIZE)));
typedef	vec3< VecN<float,PS_SIMD_FLEN> >	Float3_ __attribute__ ((aligned(PS_SIMD_ALIGN_SIZE)));
typedef	vec4< VecN<float,PS_SIMD_FLEN> >	Float4_ __attribute__ ((aligned(PS_SIMD_ALIGN_SIZE)));

#endif


#endif
