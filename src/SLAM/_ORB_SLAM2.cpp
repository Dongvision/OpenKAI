/*
 * _ORB_SLAM2.cpp
 *
 *  Created on: Jul 18, 2017
 *      Author: yankai
 */

#include "_ORB_SLAM2.h"

#ifdef USE_ORB_SLAM2

namespace kai
{

_ORB_SLAM2::_ORB_SLAM2()
{
	m_width = 640;
	m_height = 360;
	m_tStartup = 0;

	m_pVision = NULL;
	m_pOS = NULL;
	m_pFrame = NULL;

	m_vT.init();
	m_bTracking = false;
	m_bViewer = false;
}

_ORB_SLAM2::~_ORB_SLAM2()
{
	if (m_pOS)
	{
		m_pOS->Shutdown();
		delete m_pOS;
	}

	DEL(m_pFrame);
}

bool _ORB_SLAM2::init(void* pKiss)
{
	IF_F(!this->_ThreadBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;
	pK->m_pInst = this;

	KISSm(pK, width);
	KISSm(pK, height);
	KISSm(pK, bViewer);

	string fileVocabulary = "";
	string fileSetting = "";

	F_INFO(pK->v("fileVocabulary", &fileVocabulary));
	F_INFO(pK->v("fileSetting", &fileSetting));

	// Create SLAM system. It initializes all system threads and gets ready to process frames.
	m_pOS = new ORB_SLAM2::System(fileVocabulary, fileSetting,
			ORB_SLAM2::System::MONOCULAR, m_bViewer);

	m_pFrame = new Frame();
	m_tStartup = 0;

	reset();

	return true;
}

bool _ORB_SLAM2::link(void)
{
	IF_F(!this->_ThreadBase::link());
	Kiss* pK = (Kiss*) m_pKiss;

	string iName = "";
	F_INFO(pK->v("_VisionBase", &iName));
	m_pVision = (_VisionBase*) (pK->root()->getChildInstByName(&iName));

	return true;
}

bool _ORB_SLAM2::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		m_bThreadON = false;
		return false;
	}

	return true;
}

bool _ORB_SLAM2::bTracking(void)
{
	return m_bTracking;
}

void _ORB_SLAM2::reset(void)
{
	m_mPrevPose = Mat::eye(4, 4, CV_32F);
	m_mWorld = Mat::eye(4, 4, CV_32F);
	m_mFlipSign = (Mat_<float>(4, 4) << 1, -1, -1, 1,
									 	-1, 1, -1, 1,
										-1, -1, 1, 1,
										1, 1, 1, 1);
}

void _ORB_SLAM2::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		detect();

		this->autoFPSto();
	}
}

void _ORB_SLAM2::detect(void)
{
	static const double usecBase = 1.0 / ((double) USEC_1SEC);

	NULL_(m_pOS);
	NULL_(m_pVision);
	Frame* pGray = m_pVision->gray();
	NULL_(pGray);
	IF_(pGray->empty());
	m_pFrame->getResizedOf(pGray, m_width, m_height);

	uint64_t tNow = get_time_usec();
	if (m_tStartup <= 0)
	{
		m_tStartup = tNow;
	}

	double t = ((double) (tNow - m_tStartup)) * usecBase;

	m_pose = m_pOS->TrackMonocular(*m_pFrame->getCMat(), t);
	if (m_pose.empty())
	{
		m_bTracking = false;

		//TODO: if lost too long reset the system

		return;
	}

	m_bTracking = true;

	Mat mT = (m_pose * m_mPrevPose.inv()).mul(m_mFlipSign);
	m_mWorld = m_mWorld * mT;
	m_mPrevPose = m_pose.clone();

	m_vT.x = (double) m_mWorld.at<float>(0, 3); //Right
	m_vT.y = (double) m_mWorld.at<float>(1, 3); //Down
	m_vT.z = (double) m_mWorld.at<float>(2, 3); //Front

	Eigen::Matrix3f mWorld;
	cv2eigen(m_mWorld,mWorld);
	Eigen::Quaternionf qW;
	qW = mWorld;
	Eigen::Vector3f vEA = qW.toRotationMatrix().eulerAngles(1,0,2);

	m_vR.x = (double) vEA[0]; //yaw
	m_vR.y = (double) vEA[1]; //pitch
	m_vR.z = (double) vEA[2]; //roll

	cout.precision(5);
	cout << "vT: " << "  " << fixed << m_vT.x << "  " << fixed << m_vT.y << "  " << fixed << m_vT.z << "  " <<
		    "vR: " << "  " << fixed << m_vR.x << "  " << fixed << m_vR.y << "  " << fixed << m_vR.z << endl;
}

bool _ORB_SLAM2::draw(void)
{
	IF_F(!this->_ThreadBase::draw());
	Window* pWin = (Window*) this->m_pWindow;

	string msg;
	pWin->tabNext();

	if (m_bTracking)
	{
		msg = "Tracking";
	}
	else
	{
		msg = "Tracking lost";
	}
	pWin->addMsg(&msg);

	msg = "Global pos: x=" + f2str(m_vT.x) + ", y=" + f2str(m_vT.y) + ", z=" + f2str(m_vT.z);
	pWin->addMsg(&msg);

	pWin->tabPrev();

	return true;
}

}

#endif