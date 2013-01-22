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
	double getTimestamp();
	void adjtimex(double value, int type);
	double origine;
	double offset;
	double drift;
	double noise;
	double sigma;
	double p;
	double delta;
	double Tcamp;
	ofstream outFile;
	cOutVector timestampVec;
	cOutVector noiseVec;
	cOutVector driftVec;
	cOutVector offsetVec;
};

Define_Module(Clock);

void Clock::initialize(){
	// ---------------------------------------------------------------------------
	// Inizializzazione variabile per il salvataggio dei dati in uscita.
	// ---------------------------------------------------------------------------
	timestampVec.setName("time value");
	noiseVec.setName("noise");
	driftVec.setName("drift");
	offsetVec.setName("offset");

	// ---------------------------------------------------------------------------
	// Lettura parametri di ingresso.
	// ---------------------------------------------------------------------------
	offset = par("offset");
	drift  = par("drift");
	sigma  = par("noise");
	p	   = par("spikeprob");
	delta  = par("spikeampl");
	Tcamp  = par("Tcamp");
	// ---------------------------------------------------------------------------
	// Lettura parametri di ingresso.
	// ---------------------------------------------------------------------------
	origine = SIMTIME_DBL(simTime());
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
		noiseVec.record(noise);
		if(!outFile){
			ev << "CLOCK: Errore apertura file" << endl;
		}else{
			outFile<<temp<<"\t"<<drift<<"\t"<<noise<<endl;	
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
		ev<<"A Packet received by clock.\n";
		switch(((Packet *)msg)->getClockType()){
			case TIME_REQ:
				ev<<"TIME_REQ Packet received by clock.\n";
				msg->setName("TIME_RES");
				msg->setTimestamp(getTimestamp());
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


double Clock::getTimestamp(){
	// ---------------------------------------------------------------------------
	// Modello di timestamping + clock.
	// ---------------------------------------------------------------------------
	// Modello del clock
	//double clock = offset + (1+drift)*(SIMTIME_DBL(simTime())-origine);
	double clock = offset + drift*(SIMTIME_DBL(simTime())-origine) + SIMTIME_DBL(simTime());
	// Modello di timestamping
	noise = normal(0,sigma);
	if(bernoulli(p)==1){
		// Simulazione degli spike.
		noise = noise + delta;
	}
	return clock + noise;
}

void Clock::adjtimex(double value, int type){
	switch(type){
	case 0: //offset
		//offset = offset - value;
		ev << "---------------------------------" << endl;
		ev << "CLOCK : AGGIORNAMENTO OFFSET" << endl;
		ev << "CLOCK : offset- = " << offset;
		//offset = offset + (1+drift)*(SIMTIME_DBL(simTime())-origine) - value;
		offset = offset + drift*(SIMTIME_DBL(simTime())-origine) - value;
		ev << " offset+ = " << offset << endl;
		ev << "CLOCK : origine- = " << origine;
		origine = SIMTIME_DBL(simTime());
		ev << " origine+ = " << origine << endl;
		break;
	case 1: //drift
		drift = drift - value;
		break;
	}
}

void Clock::finish(){
	closefile();
}


void Clock::updateDisplay(){
	char buf[100];
	sprintf(buf, "offset [msec]: %3.2f   \ndrift [ppm]: %3.2f \norigine: %3.2f",
		offset,drift*1E6,origine);
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
