/*
Copyright(C) 2007 Giada Giorgi

This file is part of X-Simulator.

    X-Simulator is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    X-Simulator is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <string.h>
#include <math.h>
#include <omnetpp.h>
#include "Packet_m.h"
#include "Event_m.h"
#include "Constant.h"

class Nodo:public cSimpleModule{
protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void finish();
	virtual void updateDisplay();
private:
	/*Dichiarazione metodi.*/
	void handleSelfMessage(cMessage *msg);
	void handleClockMessage(cMessage *msg);
	void handleMasterMessage(cMessage *msg);
	void handleDelayResponse();
	void handleSync();
	void handleOtherPacket(cMessage *msg);
	void handleEventMessage(cMessage *msg);
	void servo_clock();

	/*Metodi ausiliari.*/

	/*Dichiarazione variabili.*/
	const char *name;
	int address;
	int master;

	/* Variabili algoritmo di sincronizzazione.*/
	// Variabili definite dal protocollo PTP
	double Tsync;		//Periodo con cui vengono inviati i messaggi di SYNC
	double ts_s_sync;	//Timestamp assegnato dallo slave al msg SYNC
	double ts_m_sync;	//Timestamp assegnato dal master al msg SYN
	double ts_s_dreq;	//Timestamp assegnato dallo slave al msg DREQ
	double ts_m_dreq;	//Timestamp assegnato dal master al msg DREQ
	double dprop;		//Ritardo di propagazione one-way-delay
	double dms;			//Ritardo di propagazione tra master e slave
	double dsm;			//Ritardo di propagazione tra slave e master
	double offset;		//Offset misurato tra master e slave
	double drift;		//Drift misurato tra master e slave

	/* Parametri servo clock*/
	double Ts;
	double Ts_correct;
	double Ts_previous;
	double Tm;
	double Tm_previous;
	
	/* Variabili di uscita */
	cOutVector dpropVec;
	cOutVector dmsVec;
	cOutVector dsmVec;
	cOutVector offsetVec;
};