/*
 *  Created on: June 22, 2020
 *      Author: yankai
 */
#include "_DRV8825_RS485.h"

namespace kai
{

_DRV8825_RS485::_DRV8825_RS485()
{
	m_pMB = NULL;
	m_iSlave = 1;
	m_iData = 0;
	m_dpr = 1;
	m_pA = NULL;
	m_dInit = 20;
	m_cmdInt = 50000;

	m_ieReadStatus.init(50000);
}

_DRV8825_RS485::~_DRV8825_RS485()
{
}

bool _DRV8825_RS485::init(void* pKiss)
{
	IF_F(!this->_ActuatorBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	pK->v("iSlave", &m_iSlave);
	pK->v("iData", &m_iData);
	pK->v("dpr", &m_dpr);
	pK->v("dInit", &m_dInit);
	pK->v("cmdInt", &m_cmdInt);
	pK->v("tIntReadStatus", &m_ieReadStatus.m_tInterval);

	m_pA = &m_vAxis[0];

	string iName;
	iName = "";
	F_ERROR_F(pK->v("_Modbus", &iName));
	m_pMB = (_Modbus*) (pK->getInst(iName));
	IF_Fl(!m_pMB, iName + " not found");

	return true;
}

bool _DRV8825_RS485::start(void)
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

int _DRV8825_RS485::check(void)
{
	NULL__(m_pMB,-1);
	IF__(!m_pMB->bOpen(),-1);
	NULL__(m_pA,-1);

	return 0;
}

void _DRV8825_RS485::update(void)
{
	while(!initPos());

	while (m_bThreadON)
	{
		this->autoFPSfrom();

		readStatus();
		updateMove();

		this->autoFPSto();
	}
}

void _DRV8825_RS485::updateMove(void)
{
	IF_(check()<0);

	IF_(!bComplete()); //Moving

	IF_(!setPos());
	IF_(!setSpeed());
	IF_(!setAccel());
	run();
}

bool _DRV8825_RS485::setDistPerRound(int32_t dpr)
{
	IF_F(check()<0);

	uint16_t pB[2];
	pB[0] = HIGH16(dpr);
	pB[1] = LOW16(dpr);
	IF_F(m_pMB->writeRegisters(m_iSlave, 4, 2, pB) != 2);
	this->sleepTime(m_cmdInt);

	return true;
}

bool _DRV8825_RS485::setPos(void)
{
	IF_F(check()<0);

	int32_t step = m_pA->m_p.m_vTarget - m_pA->m_p.m_v;
	int32_t ds = abs(step);
	IF_F(step==0);

	uint16_t pB[2];
	pB[0] = HIGH16(ds);
	pB[1] = LOW16(ds);
	IF_F(m_pMB->writeRegisters(m_iSlave, 9, 2, pB) != 2);
	this->sleepTime(m_cmdInt);

	pB[0] = (step > 0)?0:1;
	IF_F(m_pMB->writeRegisters(m_iSlave, 11, 1, pB) != 1);
	this->sleepTime(m_cmdInt);

	return true;
}

bool _DRV8825_RS485::setSpeed(void)
{
	IF_F(check()<0);

	uint16_t b = m_pA->m_s.m_vTarget;
	IF_F(m_pMB->writeRegisters(m_iSlave, 8, 1, &b) != 1);
	this->sleepTime(m_cmdInt);

	return true;
}

bool _DRV8825_RS485::setAccel(void)
{
	IF_F(check()<0);

	uint16_t b = m_pA->m_a.m_vTarget;
	IF_F(m_pMB->writeRegisters(m_iSlave, 2, 1, &b) != 1);
	this->sleepTime(m_cmdInt);

	return true;
}

bool _DRV8825_RS485::setBrake(void)
{
	IF_F(check()<0);

	return true;
}

bool _DRV8825_RS485::run(void)
{
	IF_F(check()<0);

	IF_F(m_pMB->writeBit(m_iSlave, 7, true) != 1);
	this->sleepTime(m_cmdInt);
	return true;
}

bool _DRV8825_RS485::stop(void)
{
	IF_F(check()<0);

	IF_F(m_pMB->writeBit(m_iSlave, 3, true) != 1);
	this->sleepTime(m_cmdInt);
	return true;
}

bool _DRV8825_RS485::resetPos(void)
{
	IF_F(check()<0);

	IF_F(m_pMB->writeBit(m_iSlave, 10, true) != 1);
	this->sleepTime(m_cmdInt);
	return true;
}

bool _DRV8825_RS485::initPos(void)
{
	IF_F(check()<0);
	IF_F(!setDistPerRound(m_dpr));

	uint16_t pB[2];
	int32_t ds = abs(m_dInit);
	pB[0] = HIGH16(ds);
	pB[1] = LOW16(ds);
	IF_F(m_pMB->writeRegisters(m_iSlave, 9, 2, pB) != 2);
	this->sleepTime(m_cmdInt);

	pB[0] = (m_dInit > 0)?0:1;
	IF_F(m_pMB->writeRegisters(m_iSlave, 11, 1, pB) != 1);
	this->sleepTime(m_cmdInt);

	IF_F(!setSpeed());
	IF_F(!setAccel());
	IF_F(!run());

	while(!bComplete());
	while(!resetPos());

	return true;
}

bool _DRV8825_RS485::bComplete(void)
{
	IF_F(check()<0);

	uint16_t b;
	int r = m_pMB->readRegisters(m_iSlave, 12, 1, &b);
	IF_F(r != 1);
	this->sleepTime(m_cmdInt);

	return (b==1)?true:false;
}

bool _DRV8825_RS485::readStatus(void)
{
	IF_F(check()<0);
	IF_T(!m_ieReadStatus.update(m_tStamp));

	uint16_t pB[2];
	int r = m_pMB->readRegisters(m_iSlave, 22, 2, pB);
	IF_F(r != 2);
	this->sleepTime(m_cmdInt);

//	int p = MAKE32(pB[0], pB[1]);
	int16_t p = pB[1];
	m_pA->m_p.m_v = p;

	return true;
}

void _DRV8825_RS485::draw(void)
{
	this->_ActuatorBase::draw();
}

}
