#ifndef PS_DFT_H
#define PS_DFT_H

#include <vector>
#include "PS_MathBase.h"

using namespace std;

struct SINWAVEINFO{
    float freq;
    float amp;
    float phase;
};

template <typename T>
struct COMPLEX{
    T real;
    T image;
};

template <typename T>
class SIGNALDATA{
public:
    SIGNALDATA()
    {
        real = NULL; img = NULL; szData = 0;
    }

    SIGNALDATA(size_t szData_)
    {
        szData = szData_;
        real = new T[szData];
        img = new T[szData];
    }

    ~SIGNALDATA()
    {
        clear();
    }

    void clear()
    {
        SAFE_DELETE(real);
        SAFE_DELETE(img);
        szData = 0;
    }

    void insertFront(T x, T y)
    {
        assert(this->szData > 0);
        for(size_t i=szData - 1; i>0; i--)
        {
            real[i] = real[i-1];
            img[i] = img[i-1];
        }
        real[0] = x;
        img[0] = y;
    }

    void setZero()
    {
        memset(real, 0, szData * sizeof(T));
        memset(img, 0, szData * sizeof(T));
    }

    void copyFrom(const SIGNALDATA<T>& from);

    void resize(size_t szData_)
    {
        clear();
        szData = szData_;
        real = new T[szData];
        img = new T[szData];
    }

    COMPLEX<T> getMaxData() const;

    COMPLEX<T> getMinData() const;

    void normalize();

    void convertToMagnitudeAndPhase(bool bMagnitudeInDB = false);


public:
    T* real;
    T* img;
    size_t szData;
};

/*
bool GenerateSinosoid1D(int ctSamples, float amp, float freq, float phase, SIGNALDATA<float>& output)
{
    if(output.szData < ctSamples) return false;

    float nInv = 1 / (float)ctSamples;
    float f = TwoPi * freq * nInv;
    for(size_t i=0; i<ctSamples; i++)
    {
        output.real[i] = amp * sin(f*(float)i + phase);
        output.img[i] = (float)i * nInv;
    }
    return true;
}
*/

////////////////////////////////////////////////////////////////////////////////////////////////////////
//DFT Computer
template <typename T>
class CDftComp
{
public:    
    CDftComp()
    {
    }

    CDftComp(size_t szWindow):m_szWindow(szWindow)
    {

    }

    ~CDftComp()
    {
    }

    int compute(const SIGNALDATA<T>& vInput, SIGNALDATA<T>& vOutput, bool bIDFT = false);

    static bool ComputeSignalMagnitudePhase(const SIGNALDATA<T>& vInput,
                                            SIGNALDATA<T>& vOutput,
                                            bool bMag = true, bool bDB = false, bool bPhase = true);


    size_t getWindowSize() const {return m_szWindow;}
    void setWindowSize(size_t szWindow){m_szWindow = szWindow;}    
private:
    size_t m_szWindow;
};
////////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
void SIGNALDATA<T>::copyFrom(const SIGNALDATA<T>& from)
{
    if(from.szData == 0) return;

    this->clear();
    this->resize(from.szData);
    memcpy(this->real, from.real, sizeof(T)*from.szData);
    memcpy(this->img, from.img, sizeof(T)*from.szData);
}

template <typename T>
COMPLEX<T> SIGNALDATA<T>::getMaxData() const
{
    assert(this->szData > 0);

    COMPLEX<T> myMax;
    myMax.real  = real[0];
    myMax.image = img[0];

    for(size_t i=1; i<szData; i++)
    {
        if(real[i] > myMax.real)
            myMax.real = real[i];
        if(img[i] > myMax.image)
            myMax.image = img[i];
    }
    return myMax;
}

template <typename T>
COMPLEX<T> SIGNALDATA<T>::getMinData() const
{
    if(szData == 0) return 0;
    COMPLEX<T> myMin;
    myMin.real  = real[0];
    myMin.image = img[0];

    for(size_t i=1; i<szData; i++)
    {
        if(real[i] < myMin.real)
            myMin.real = real[i];
        if(img[i] < myMin.image)
            myMin.image = img[i];
    }
    return myMin;
}

template <typename T>
void SIGNALDATA<T>::normalize()
{
    COMPLEX<T> myMax = getMaxData();
    T ivsReal = (myMax.real > 0.0)?1.0 / myMax.real:1.0;
    T ivsImg  = (myMax.image > 0.0)?1.0 / myMax.image:1.0;
    for(size_t i=0; i<szData; i++)
    {
        real[i] *= ivsReal;
        img[i] *= ivsImg;
    }
}

template <typename T>
void SIGNALDATA<T>::convertToMagnitudeAndPhase(bool bMagnitudeInDB)
{
    assert(this->szData > 0);

    T x,y;
    for(size_t i=0; i<szData; i++)
    {
        x = this->real[i];
        y = this->img[i];

        if(bMagnitudeInDB)
            this->real[i] = 10.0f * 0.5f * log10(x*x + y*y);
        else
            this->real[i] = FastSqrt(x*x + y*y);

        this->img[i] = atan2(y, x);
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////
template <typename T>
int CDftComp<T>::compute(const SIGNALDATA<T>& vInput, SIGNALDATA<T>& vOutput, bool bIDFT)
{
    if((m_szWindow == 0 || vInput.szData == 0 || vOutput.szData == 0))
        return 0;

    T pi2 = (bIDFT)?2.0 * Pi : -2.0 * Pi;
    T a, ca, sa;
    T invs = 1 / (double)m_szWindow;

    for(size_t y=0; y<m_szWindow; y++)
    {
        vOutput.real[y] = 0;
        vOutput.img[y] = 0;

        for(size_t x=0; x<m_szWindow; x++)
        {
            a = pi2 * y * x * invs;
            ca = cos(a);
            sa = sin(a);
            vOutput.real[y] += vInput.real[x] * ca - vInput.img[x] * sa;
            vOutput.img[y] += vInput.real[x] * sa + vInput.img[x] * ca;
        }

        if(bIDFT)
        {
            vOutput.real[y] *= invs;
            vOutput.img[y] *= invs;
        }
    }

    return m_szWindow;
}













































#endif // PS_DFT_H
