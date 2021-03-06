/*
 * _PCrsCV.h
 *
 *  Created on: May 24, 2020
 *      Author: yankai
 */

#ifndef OpenKAI_src_PointCloud_PCrsCV_H_
#define OpenKAI_src_PointCloud_PCrsCV_H_

#include "../Base/common.h"

#ifdef USE_OPENCV
#ifdef USE_OPEN3D
#ifdef USE_REALSENSE
#include "../Vision/_RealSense.h"
#include "_PCbase.h"

namespace kai
{

class _PCrsCV: public _PCbase
{
public:
	_PCrsCV();
	virtual ~_PCrsCV();

	bool init(void* pKiss);
	bool start(void);
	int check(void);

private:
	void updateRS(void);
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_PCrsCV *) This)->update();
		return NULL;
	}

public:
	_RealSense* m_pRS;
    rs2::pointcloud m_rsPC;
    rs2::points m_rsPoints;

	vFloat2 m_vRz;	//z region

//    Image m_imgD;
//    Image m_imgRGB;

};

}
#endif
#endif
#endif
#endif
