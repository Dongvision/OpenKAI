/*
 *  Created on: June 22, 2020
 *      Author: yankai
 */
#include "_ZLAC8015.h"

namespace kai
{

_ZLAC8015::_ZLAC8015()
{
	m_pMB = NULL;
	m_iSlave = 1;
	m_iMode = 3;
	m_pA = NULL;

	m_ieReadStatus.init(50000);
}

_ZLAC8015::~_ZLAC8015()
{
}

bool _ZLAC8015::init(void* pKiss)
{
	IF_F(!this->_ActuatorBase::init(pKiss));
	Kiss* pK = (Kiss*) pKiss;

	pK->v("iSlave", &m_iSlave);
	pK->v("iMode", &m_iMode);
	pK->v("tIntReadStatus", &m_ieReadStatus.m_tInterval);

	m_pA = &m_vAxis[0];

	string iName;
	iName = "";
	F_ERROR_F(pK->v("_Modbus", &iName));
	m_pMB = (_Modbus*) (pK->getInst(iName));
	IF_Fl(!m_pMB, iName + " not found");

	return true;
}

bool _ZLAC8015::start(void)
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

int _ZLAC8015::check(void)
{
	NULL__(m_pMB,-1);
	IF__(!m_pMB->bOpen(),-1);
	NULL__(m_pA,-1);

	return 0;
}

void _ZLAC8015::update(void)
{
	while (m_bThreadON)
	{
		this->autoFPSfrom();

		readStatus();
		updateMove();

		this->autoFPSto();
	}
}

void _ZLAC8015::updateMove(void)
{
	IF_(check()<0);

	IF_(!setMode());
	IF_(!setAccel());
	IF_(!setBrake());
	IF_(!setSpeed());
}

bool _ZLAC8015::setMode(void)
{
	IF_F(check()<0);

	IF_F(m_pMB->writeRegister(m_iSlave, 0x2032, m_iMode) != 1);

	return true;
}

bool _ZLAC8015::setAccel(void)
{
	IF_F(check()<0);

	uint16_t v = m_pA->m_a.m_vTarget;
	IF_F(m_pMB->writeRegister(m_iSlave, 0x2037, v) != 1);

	return true;
}

bool _ZLAC8015::setBrake(void)
{
	IF_F(check()<0);

	uint16_t v = m_pA->m_b.m_vTarget;
	IF_F(m_pMB->writeRegister(m_iSlave, 0x2038, v) != 1);

	return true;
}

bool _ZLAC8015::setSpeed(void)
{
	IF_F(check()<0);

	uint16_t v = m_pA->m_s.m_vTarget;
	IF_F(m_pMB->writeRegister(m_iSlave, 0x203A, v) != 1);

	return true;
}

bool _ZLAC8015::stop(void)
{
	IF_F(check()<0);

	IF_F(m_pMB->writeBit(m_iSlave, 3, true) != 1);
	return true;
}

bool _ZLAC8015::bComplete(void)
{
	IF_F(check()<0);

	uint16_t b;
	int r = m_pMB->readRegisters(m_iSlave, 12, 1, &b);
	IF_F(r != 1);

	return (b==1)?true:false;
}

bool _ZLAC8015::readStatus(void)
{
	IF_F(check()<0);
	IF_T(!m_ieReadStatus.update(m_tStamp));

	uint16_t pB[2];
	int r = m_pMB->readRegisters(m_iSlave, 22, 2, pB);
	IF_F(r != 2);

//	int p = MAKE32(pB[0], pB[1]);
	int16_t p = pB[1];
	m_pA->m_p.m_v = p;

	return true;
}

void _ZLAC8015::draw(void)
{
	this->_ActuatorBase::draw();
}

}
