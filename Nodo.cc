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
#include "Nodo.h"

Define_Module(Nodo);

void Nodo::initialize(){

	dpropVec.setName("prop");
	dmsVec.setName("dms");
	dsmVec.setName("dsm");
	offsetVec.setName("offset");
	/* Inizializzazione variabili di stato. */
	ts_s_sync = ts_m_sync =	ts_s_dreq =	ts_m_dreq = 0;
	dprop  = dms = dsm  = 0;
	drift = 0; 
	if(ev.isGUI()){updateDisplay();}
	/* Lettura dei parametri di ingresso. */
	Tsync = par("Tsync");

	/*Parametri servo clock*/
	Ts = Ts_correct = Tm = Tm_previous = Ts_previous = 0;

	/* Nome e Id del nodo. */
	name = "slave";
	address = getId();
	
	/* Identificazione del master. */
	cModule *masterModule = getParentModule()->getParentModule()->getSubmodule("master");
	master = masterModule->getId();
	
	/* Inizializzazione del timer.	*/
	//scheduleAt(simTime()+intuniform(Tsync,14*Tsync)+CONSTDELAY, new cMessage("SLtimer"));

	/* Registrazione nodo nella rete. */
	Packet * temp = new Packet("REGISTER");
	temp->setPckType(OTHER);
	temp->setSource(address);
	temp->setDestination(-2);
	temp->setByteLength(0);
	send(temp,"out");
	ev << "SLAVE " << getId() << " : Fine inizializzazione.\n";
}


void Nodo::handleMessage(cMessage *msg){
	if(msg->isSelfMessage()){handleSelfMessage(msg);}
	if(msg->arrivedOn("inclock")){handleClockMessage(msg);}
	if(msg->arrivedOn("in"))
		//Un messaggio arrivato da questa porta ? sicuramente un pacchetto
		if(((Packet*)msg)->getSource()!=address & 
			(((Packet*)msg)->getDestination()==address |
			((Packet*)msg)->getDestination()==-1))
			if((((Packet *)msg)->getPckType())==PTP)
				handleMasterMessage(msg);
	if(msg->arrivedOn("inevent")){handleEventMessage(msg);}
	delete msg;
	if(ev.isGUI()){updateDisplay();}
}
//ToDo Modify handleSelfMessage()
void Nodo::handleSelfMessage(cMessage *msg){
	Packet *pck = new Packet("DREQ_TIME_REQ");
	pck->setPckType(PTP);
	pck->setSource(address);
	pck->setDestination(master);
	pck->setPtpType(DREQ);
	pck->setClockType(TIME_REQ);
	send(pck,"outclock");
	scheduleAt(simTime()+intuniform(Tsync,14*Tsync)+CONSTDELAY, new cMessage("SLtimer"));
}

void Nodo::handleOtherPacket(cMessage *msg){}

void Nodo::handleEventMessage(cMessage *msg){
	if(((Event *)msg)->getEventType()==CICLICO){
		Packet *pck = new Packet("CICLICO");
		pck->setPckType(OTHER);
		pck->setSource(address);
		pck->setDestination(((Event *)msg)->getDest());
		pck->setByteLength(((Event *)msg)->getPckLength());
		for(int i=0; i<((Event *)msg)->getPckNumber()-1; i++){
				send((cMessage *)pck->dup(),"out");
			}
		send(pck,"out");
	}
}


void Nodo::handleClockMessage(cMessage *msg){
	switch(((Packet *)msg)->getPtpType()){
		case SYNC:
			ts_s_sync =SIMTIME_DBL(msg->getTimestamp());
			ev<<"The time of T2:"<<endl;
            ev<<"T2="<<ts_s_sync<<endl;
			handleSync();
			/*********************************
			 **    Salvataggio valori.      **
			 *********************************/
			dsmVec.record(dsm);
			dpropVec.record(dprop);
			dmsVec.record(dms);
			offsetVec.record(offset);
			break;
		case DREQ:
			ts_s_dreq = SIMTIME_DBL(msg->getTimestamp());
			ev<<"The time of T3:"<<endl;
            ev<<"T3="<<ts_s_dreq<<endl;
			msg->setName("DREQ");
			((Packet*)msg)->setDestination(master);
			((Packet*)msg)->setByteLength(DREQ_BYTE);
			send((cMessage *)msg->dup(),"out");
			break;
		case DRES:
			error("Invalid clock message\n");
			break;
	}
}

void Nodo::handleMasterMessage(cMessage *msg){
	switch(((Packet *)msg)->getPtpType()){
		case SYNC:
			ts_m_sync = ((Packet *)msg)->getData();
			ev<<"The time of T1:"<<endl;
            ev<<"T1="<<ts_m_sync<<endl;
			msg->setName("SYN_TIME_REQ");
			((Packet *)msg)->setClockType(TIME_REQ);
			send((cMessage *)msg->dup(),"outclock");
			break;
		case DRES:
			ev<<"Nodo handleMasterMesage processes DRES Packet from Master.\n";
			ts_m_dreq = ((Packet *)msg)->getData();
			ev<<"The time of T4:"<<endl;
            ev<<"T4="<<ts_m_dreq<<endl;
			handleDelayResponse();
			break;
		case DREQ:
			error("Invalid master message\n");
	}
}

void Nodo::handleDelayResponse(){
	ev<<"\nhandleDelayResponse called by handleMasterMessage.\n";
	//error("\nhandleDelayResponse called by handleMasterMessage.\n");
	dsm = ts_m_dreq - ts_s_dreq;
	dprop = (dms + dsm)/2;
}

void Nodo::handleSync(){
	ev<<"dsm="<<dsm<<"dms"<<dms<<"dprop"<<dprop<<"\n";
	dms = ts_s_sync - ts_m_sync;
	offset = dms - dprop;
	servo_clock();
}

void Nodo::finish(){}

void Nodo::updateDisplay(){
	char buf[100];
	sprintf(buf, "dms [ms]: %3.2f \ndsm [ms]: %3.2f \ndpr [ms]: %3.2f \noffset [ms]: %3.2f\n ",
		dms*1000,dsm*1000,dprop*1000,offset*1000);
	getDisplayString().setTagArg("t",0,buf);
}
/* 
-------------------------------------------------------------------------------
SERVO CLOCK IMPLEMENTATION.
This function must be overwritten by the user.
-------------------------------------------------------------------------------*/
void Nodo::servo_clock(){	
	double alpha = 1;
	double beta = 64;
	double y =offset/alpha;
	Packet *pck = new Packet("ADJ_OFFSET");
	pck->setPckType(CLOCK);
	pck->setClockType(OFFSET_ADJ);
	pck->setData(y);
	send(pck,"outclock");

	Ts = ts_s_sync;
	Tm = ts_m_sync;
	if(Tm_previous > 0){
		drift = (Ts-Ts_previous+y)/(Tm-Tm_previous)-1;
		double DELTADRIFT=10E-6;
		if(drift>DELTADRIFT){drift=DELTADRIFT;}
		else if(drift<-DELTADRIFT){drift=-DELTADRIFT;}
		Packet *pckd = new Packet("ADJ_FREQ");
		pckd->setPckType(CLOCK);
		pckd->setClockType(FREQ_ADJ);
		pckd->setData(drift/beta);
		send(pckd,"outclock");
	}
	Tm_previous = Tm;
	Ts_previous = Ts;
}
