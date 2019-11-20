/*
 * _IRLock.h
 *
 *  Created on: Nov 20, 2019
 *      Author: yankai
 */

#ifndef OpenKAI_src_Detector__IRLock_H_
#define OpenKAI_src_Detector__IRLock_H_

#include "../Base/common.h"
#include "../Detector/_DetectorBase.h"
#include "../IO/_IOBase.h"

namespace kai
{

/*
 Bytes    16-bit words   Description
----------------------------------------------------------------
0, 1     0              sync (0xaa55)
2, 3     1              checksum (sum of all 16-bit words 2-6)
4, 5     2              signature number
6, 7     3              x center of object
8, 9     4              y center of object
10, 11   5              width of object
12, 13   6              height of object
*/

#define IRLOCK_N_BUF 16
#define IRLOCK_N_PACKET 14
#define IRLOCK_SYNC_L 0x55
#define IRLOCK_SYNC_H 0xaa

class _IRLock : public _DetectorBase
{
public:
	_IRLock();
	virtual ~_IRLock();

	bool init(void* pKiss);
	bool start(void);
	void draw(void);

private:
	bool readPacket(void);
	void detect(void);
	void update(void);
	static void* getUpdateThread(void* This)
	{
		((_IRLock*) This)->update();
		return NULL;
	}

public:
	_IOBase*	m_pIO;

	uint8_t		m_pBuf[IRLOCK_N_BUF];
	int			m_iBuf;

};

}
#endif