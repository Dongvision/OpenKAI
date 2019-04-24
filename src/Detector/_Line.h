/*
 * _Line.h
 *
 *  Created on: April 23, 2019
 *      Author: yankai
 */

#ifndef OpenKAI_src_Detector__Line_H_
#define OpenKAI_src_Detector__Line_H_

#include "../Base/common.h"
#include "_DetectorBase.h"
#include "../Vision/_VisionBase.h"

namespace kai
{

class _Line : public _DetectorBase
{
public:
	_Line();
	virtual ~_Line();

	bool init(void* pKiss);
	bool start(void);
	int check(void);
	bool draw(void);
	bool console(int& iY);

private:
	void detect(void);
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_Line*) This)->update();
		return NULL;
	}

public:
	_VisionBase* m_pV;
	int		m_rDim;
	int		m_pSlide[1280];
	int		m_wSlide;
	double	m_vThr;
	double	m_vTower;
	bool	m_bTower;
	Mat		m_mIn;

};
}

#endif
