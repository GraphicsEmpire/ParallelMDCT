
#include "PS_DrawChart.h"
#include "GL/glew.h"

CDrawChart::CDrawChart()
{
    m_idxSelectedSeries = 0;
}

void CDrawChart::cleanup()
{
    for(size_t i=0;i<m_series.size(); i++)
    {
        m_series[i].vData.resize(0);
        //m_series[i].vColor.resize(0);
    }
    m_series.resize(0);
}

bool CDrawChart::addSeries(const SIGNALDATA<float>& signalData,
                           const string& strTitle,
                           bool bRealPart,
                           float scaleY,
                           const vec2f& offset,
                           const vec3f& color)
{
    size_t ctSamples = signalData.szData;

    SERIES s;
    s.color = color;
    s.strTitle = strTitle;
    s.vData.resize(ctSamples * 2);
    //s.vColor.resize(ctSamples * 3);

    float scaleX = 1.0f / static_cast<float>(ctSamples);



    for(size_t i=0; i<ctSamples; i++)
    {
        s.vData[i*2 + 0]  = offset.x + (float)i * scaleX;

        if(bRealPart)
            s.vData[i*2 + 1]  = offset.y + signalData.real[i] * scaleY;
        else
            s.vData[i*2 + 1]  = offset.y + signalData.img[i] * scaleY;
/*
        s.vColor[i*3 + 0] = color.x;
        s.vColor[i*3 + 1] = color.y;
        s.vColor[i*3 + 2] = color.z;
        */
    }


    m_series.push_back(s);
    return true;
}

bool CDrawChart::addSeries(const vector<float>& seriesData,
                           const string& strTitle,	
						   float scaleY,
                           const vec2f& offset,
                           const vec3f& color)
{
    size_t ctSamples = seriesData.size();

    SERIES s;
    s.color = color;
    s.strTitle = strTitle;
    s.vData.resize(ctSamples * 2);
    //s.vColor.resize(ctSamples * 3);
    float scaleX = 1.0f / static_cast<float>(ctSamples);

    for(size_t i=0;i<ctSamples; i++)
    {
        s.vData[i*2 + 0]  = offset.x + (float)i * scaleX;
        s.vData[i*2 + 1]  = offset.y + seriesData[i] * scaleY;
/*
        s.vColor[i*3 + 0] = color.x;
        s.vColor[i*3 + 1] = color.y;
        s.vColor[i*3 + 2] = color.z;
        */
    }
    m_series.push_back(s);
    return true;
}

bool CDrawChart::addSeries(const vector<double>& seriesData,
                           const string& strTitle,
						   float scaleY,
                           const vec2f& offset,
                           const vec3f& color)
{
    size_t ctSamples = seriesData.size();

    SERIES s;
    s.color = color;
    s.strTitle = strTitle;
    s.vData.resize(ctSamples * 2);
    //s.vColor.resize(ctSamples * 3);
    float scaleX = 1.0f / static_cast<float>(ctSamples);

    for(size_t i=0;i<ctSamples; i++)
    {
        s.vData[i*2 + 0]  = offset.x + (float)i * scaleX;
        s.vData[i*2 + 1]  = offset.y + seriesData[i] * scaleY;
/*
        s.vColor[i*3 + 0] = color.x;
        s.vColor[i*3 + 1] = color.y;
        s.vColor[i*3 + 2] = color.z;
        */
    }
    m_series.push_back(s);
    return true;
}


void CDrawChart::draw(int plotMode)
{
    for(size_t i=0; i<m_series.size(); i++)
    	draw(i, plotMode);
}

void CDrawChart::draw(int iSeries, int plotMode)
{
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	vec3f c = m_series[iSeries].color;
	glColor3f(c.x, c.y, c.z);
    glVertexPointer(2, GL_FLOAT, 0, &m_series[iSeries].vData[0]);
    glEnableClientState(GL_VERTEX_ARRAY);

    //glColorPointer(3, GL_FLOAT, 0, &m_series[iSeries].vColor[0]);
    //glEnableClientState(GL_COLOR_ARRAY);

    if(plotMode & PLOT_LINE_STRIP)
        glDrawArrays(GL_LINE_STRIP, 0, m_series[iSeries].vData.size() / 2);
    if(plotMode & PLOT_POINTS)
        glDrawArrays(GL_POINTS, 0, m_series[iSeries].vData.size() / 2);


    //glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopAttrib();
}































