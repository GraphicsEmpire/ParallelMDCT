#ifndef PS_DRAWCHART_H
#define PS_DRAWCHART_H

#include <vector>
#include <string>

#include "PS_VectorMath.h"
#include "PS_DFT.h"


using namespace std;
//using namespace PS::MATH;

#define PLOT_LINE_STRIP 1
#define PLOT_POINTS     2


class CDrawChart
{
public:
    struct SERIES
    {
        vector<float> vData;
        vector<float> vColor;
        std::string strTitle;
        vec3f color;        
    };

    CDrawChart();

    ~CDrawChart() {cleanup();}



    bool addSeries(const SIGNALDATA<float>& signalData,                   
                   const string& strTitle,
				   bool bRealPart,
				   float scaleY,
                   const vec2f& offset = vec2f(0,0),
                   const vec3f& color = vec3f(0,0,0));
    bool addSeries(const vector<float>& seriesData,
                   const string& strTitle,
				   float scaleY,
                   const vec2f& offset = vec2f(0,0),
                   const vec3f& color = vec3f(0,0,0));

    void cleanup();

    void draw(int plotMode = PLOT_LINE_STRIP);

    void setSelectedSeries(int idxSelected) {m_idxSelectedSeries = idxSelected;}
    int getSelectedSeries() const {return m_idxSelectedSeries;}
private:
    int m_idxSelectedSeries;
    vec2f m_offset;
    string m_strHTitle;


    vector<SERIES> m_series;
};

#endif // PS_DRAWCHART_H
