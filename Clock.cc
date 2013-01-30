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
#include <omnetpp.h>
#include <fstream>
#include "Packet_m.h"
#include "Constant.h"
using namespace std;



class Clock:public cSimpleModule{
protected:
	virtual void initialize();
	virtual void handleMessage(cMessage *msg);
	virtual void finish();
 	virtual void updateDisplay();
	virtual void openfile();
	virtual void closefile();
private:
	double Phyclockupdate();
	double getTimestamp();
	void adjtimex(double value, int type);
	double lastupdatetime;
	double phyclock;
	double softclock;
	double softclock_t2;
	double softclock_t3;
	double offset;
	double drift;
	double sigma1;
	double sigma2;
	double sigma3;
	double noise1;
	double noise2;
	double noise3;
	double Tcamp;
	ofstream outFile;
	cOutVector softclockVec;
	cOutVector softclock_t2Vec;
	cOutVector softclock_t3Vec;
	cOutVector noise1Vec;
	cOutVector noise2Vec;
	cOutVector noise3Vec;
	cOutVector driftVec;
	cOutVector offsetVec;

};

Define_Module(Clock);

void Clock::initialize(){
	// ---------------------------------------------------------------------------
	// Inizializzazione variabile per il salvataggio dei dati in uscita.
	// ---------------------------------------------------------------------------
	//timestampVec.setName("time value");
	softclockVec.setName("softclock");
	softclock_t2Vec.setName("softclock_t2");
	softclock_t3Vec.setName("softclock_t3");
	noise1Vec.setName("noise1");
	noise2Vec.setName("noise2");
	noise3Vec.setName("noise3");
	driftVec.setName("drift");
	offsetVec.setName("offset");

	// ---------------------------------------------------------------------------
	// Lettura parametri di ingresso.
	// ---------------------------------------------------------------------------
	offset = par("offset");
	drift =  par("drift");
	sigma1  = par("sigma1");
	sigma2 = par("sigma2");
	sigma3 = par("sigma3");
	//p	   = par("spikeprob");
	//delta  = par("spikeampl");
	Tcamp  = par("Tcamp");
	// ---------------------------------------------------------------------------
	// Lettura parametri di ingresso.
	// ---------------------------------------------------------------------------
	lastupdatetime = SIMTIME_DBL(simTime());
	phyclock = softclock = offset;
	openfile();
	// ---------------------------------------------------------------------------
	// Inizializzazione del timer. Il timer viene ripristinato con un tempo fisso
	// pari a 1.0 secondo. Viene utilizzato per campionare in intervalli di
	// durata costante il clock di sistema. I valori campionati vengono salvati
	// nella vraibile di uscita timestampVec.
	// ---------------------------------------------------------------------------
	if(ev.isGUI()){updateDisplay();}
	scheduleAt(simTime() + Tcamp, new cMessage("CLTimer"));
}

void Clock::handleMessage(cMessage *msg){
	if(msg->isSelfMessage()){
	// ---------------------------------------------------------------------------
	// Timer. Viene salvato il valore del clock in uscita, ottenuto mediante la
	// funzione interna getTimestamp(). Viene ripristinato il timer a 1.0 s.
	// ---------------------------------------------------------------------------
		//double temp = offset + (1+drift)*(SIMTIME_DBL(simTime())-origine)-SIMTIME_DBL(simTime());
		//timestampVec.record(temp);
		driftVec.record(drift);
		offsetVec.record(offset);
		noise1Vec.record(noise1);
		noise2Vec.record(noise2);
		noise3Vec.record(noise3);
		Phyclockupdate();
		if(!outFile){
			ev << "CLOCK: Errore apertura file" << endl;
		}else{
			outFile<<offset<<"\t"<<drift<<"\t"<<sigma1<<endl;
		}
		scheduleAt(simTime()+ Tcamp, new cMessage("CLTimer"));
	}else{
	// ---------------------------------------------------------------------------
	// Messaggio ricevuto dal sistema (nodo a cui il clock si riferisce).
	// Il messaggio ricevuto puo` essere:
	// - un messaggio di tipo TIME_REQ, con il quale il sistema interroga il clock
	//   per ottenere una misura di tempo, vale a dire un timestamp;
	// - un messaggio di tipo OFFSET_ADJ, con il quale viene fatta una correzione 
	//   dell'offset del clock;
	// - un messaggio di tipo FREQ_ADJ, con il quale viene fatta una correzione del
	//   tick rate del clock.
	// ---------------------------------------------------------------------------
		ev<<"A Packet received by clock to gettimestamp.\n";
		switch(((Packet *)msg)->getClockType()){
			case TIME_REQ:
				ev<<"TIME_REQ Packet received by clock.\n";
				msg->setName("TIME_RES");
				//TODO:
				switch(((Packet *)msg)->getPtpType()){
				case SYNC:
					ev<<"T2 gettimestamp"<<endl;
					softclock_t2 = getTimestamp();
					msg->setTimestamp(softclock_t2);
					softclock_t2Vec.record(softclock_t2);
					break;
				case DREQ:
					ev<<"T3 gettimestamp"<<endl;
				    softclock_t3 = getTimestamp();
					msg->setTimestamp(softclock_t3);
					softclock_t3Vec.record(softclock_t3);
					break;
				}

				((Packet *)msg)->setClockType(TIME_RES);
				send((cMessage *)msg->dup(),"outclock");
				break;
			case OFFSET_ADJ:	//correzione dell'offset
				adjtimex(((Packet *)msg)->getData(),0);
				break;
			case FREQ_ADJ:		//correzione del drift
				//ev<<"FREQ_ADJ Packet received by clock.\n";
				adjtimex(((Packet *)msg)->getData(),1);
				break;
		}
	}
	delete msg;
	if(ev.isGUI()){updateDisplay();}
}
// TODo:adding Phyclockupdate()
double Clock::Phyclockupdate(){
	ev<<"update Phyclock&offset:"<<endl;
	noise2 =  normal(0,sigma2);
	drift = drift + noise2;
	noise1 =  normal(0,sigma1);
	offset = offset + drift*(SIMTIME_DBL(simTime())-lastupdatetime)+ noise1;
	phyclock = offset + SIMTIME_DBL(simTime());
	lastupdatetime = SIMTIME_DBL(simTime());
	ev<<"offset="<<offset<<endl;
	ev<<"simTime=lastupdatetime="<<SIMTIME_DBL(simTime())<<endl;
    return phyclock;
}
double Clock::getTimestamp(){
	//TODO:Modify
	ev<<"getTimestamp:"<<endl;
	ev<<"simTime="<<SIMTIME_DBL(simTime())<<" lastupdatetime="<<lastupdatetime<<endl;
	double clock = offset + drift*(SIMTIME_DBL(simTime())-lastupdatetime) + SIMTIME_DBL(simTime());
	noise3 = normal(0,sigma3);
	softclock = clock + noise3;
	softclockVec.record(softclock);
	//ev.printf("phyclock_float=%8f",phyclock_float);
	return softclock;
}
//TODO: Problem at 182 offset=offset-value?
void Clock::adjtimex(double value, int type){
	switch(type){
	case 0: //offset
		ev << "---------------------------------" << endl;
		ev << "CLOCK : AGGIORNAMENTO OFFSET" << endl;
		ev << "CLOCK : offset- = " << offset<<endl;
		ev<<"simTime="<<SIMTIME_DBL(simTime())<<" lastupdatetime="<<lastupdatetime<<endl;
		offset = offset - value;
		ev << " offset+ = " << offset << endl;
		break;
	case 1: //drift
		drift = drift - value;
		ev<<"drift="<<drift<<endl;
		break;
	}
}

void Clock::finish(){
	recordScalar("measure Uncertainty",softclock);
	closefile();
}


void Clock::updateDisplay(){
	char buf[100];
	sprintf(buf, "offset [msec]: %3.2f   \ndrift [ppm]: %3.2f \norigine: %3.2f",
		offset,drift*1E6,lastupdatetime);
	getDisplayString().setTagArg("t",0,buf);
}

void Clock::openfile(){
	ev << "CLOCK: ---- APERTURA FILE ----\n";
	outFile.open("clockdata.txt");
	if(!outFile){
		ev << "Il file non puo` essere aperto\n";
	}
	else{
		ev << "Il file e` stato aperto con successo\n";
	}
}

void Clock::closefile(){
	ev << "CLOCK: ---- CHIUSURA FILE ----\n";
	outFile.close();
}
