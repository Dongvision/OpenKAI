/*
 *  Created on: May 16, 2019
 *      Author: yankai
 */
#include "_OrientalMotor.h"

namespace kai
{

_OrientalMotor::_OrientalMotor()
{
	m_pA = NULL;
	m_pMB = NULL;
	m_iSlave = 1;
	m_iData = 0;

	m_ieCheckAlarm.init(100000);
	m_ieSendCMD.init(50000);
	m_ieReadStatus.init(50000);
}

_OrientalMotor::~_OrientalMotor()
{
}

bool _OrientalMotor::init(void* pKiss)
{
	IF_F(!this->_ActuatorBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	pK->v("iSlave",&m_iSlave);
	pK->v("iData",&m_iData);

	pK->v("tIntCheckAlarm", &m_ieCheckAlarm.m_tInterval);
	pK->v("tIntSendCMD", &m_ieSendCMD.m_tInterval);
	pK->v("tIntReadStatus", &m_ieReadStatus.m_tInterval);

	m_pA = &m_vAxis[0];

	string iName;
	iName = "";
	F_ERROR_F(pK->v("_Modbus", &iName));
	m_pMB = (_Modbus*) (pK->getInst(iName));
	IF_Fl(!m_pMB, iName + " not found");

	return true;
}

bool _OrientalMotor::start(void)
{
	m_bThreadON = true;
	int retCode = pthread_create(&m_threadID, 0, getUpdateThread, this);
	if (retCode != 0)
	{
		LOG_E(retCode);
		m_bThreadON = false;
		return false;
	}

	return true;
}

void _OrientalMotor::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		if(m_bFeedback)
		{
			checkAlarm();
			readStatus();
		}
		sendCMD();

		this->autoFPSto();
	}
}

int _OrientalMotor::check(void)
{
	NULL__(m_pA,-1);
	NULL__(m_pMB,-1);
	IF__(!m_pMB->bOpen(),-1);

	return 0;
}

void _OrientalMotor::checkAlarm(void)
{
	IF_(check()<0);
	IF_(!m_ieCheckAlarm.update(m_tStamp));

	uint16_t pB[2];
	pB[0] = 1<<7;
	pB[1] = 0;
	m_pMB->writeRegisters(m_iSlave, 125, 1, pB);
}

void _OrientalMotor::sendCMD(void)
{
	IF_(check()<0);
	IF_(!m_ieSendCMD.update(m_tStamp));
	IF_(m_pA->m_p.m_vTarget < 0.0);

	//update normalized value to actual unit
	int32_t step = m_pA->m_p.m_vTarget;
	int32_t speed = m_pA->m_s.m_vTarget;
	int32_t accel = m_pA->m_a.m_vTarget;
	int32_t brake = m_pA->m_b.m_vTarget;
	int32_t current = m_pA->m_c.m_vTarget;

	//create the command
	uint16_t pB[18];
	//88
	pB[0] = 0;
	pB[1] = m_iData;
	pB[2] = 0;
	pB[3] = 1;
	//92
	pB[4] = HIGH16(step);
	pB[5] = LOW16(step);
	pB[6] = HIGH16(speed);
	pB[7] = LOW16(speed);

	//96
	pB[8] = HIGH16(accel);
	pB[9] = LOW16(accel);
	pB[10] = HIGH16(brake);
	pB[11] = LOW16(brake);
	pB[12] = HIGH16(current);
	pB[13] = LOW16(current);
	pB[14] = 0;
	pB[15] = 1;
	pB[16] = 0;
	pB[17] = 0;

	if(m_pMB->writeRegisters(m_iSlave, 88, 18, pB) != 18)
	{
		m_ieSendCMD.reset();
	}
}

void _OrientalMotor::readStatus(void)
{
	IF_(check()<0);
	IF_(!m_ieReadStatus.update(m_tStamp));

	uint16_t pB[18];
	int nR = 6;
	int r = m_pMB->readRegisters(m_iSlave, 204, nR, pB);
	IF_(r != 6);

	//update actual unit to normalized value
	IF_(m_pA->m_p.m_vTarget < 0.0);

	m_pA->m_p.m_v = MAKE32(pB[0], pB[1]);
	m_pA->m_s.m_v = MAKE32(pB[4], pB[5]);

	LOG_I("step: "+f2str(m_pA->m_p.m_v) + ", speed: " + f2str(m_pA->m_s.m_v));
}

void _OrientalMotor::draw(void)
{
	this->_ActuatorBase::draw();
}

}
