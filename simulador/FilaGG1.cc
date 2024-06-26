#include <FilaGG1.hh>
#include <global.hh>

FilaGG1::FilaGG1(): Simulator(), cajaLibre(true)
{
	
}

void Llegada::processEvent()
{
	std::stringstream ssEvLog;

	ssEvLog << "==> llega al minimarket.\n";

	double tiempoSeleccionAbarrotes = Random::exponential(tasaSeleccionAbarrotes);

	if(tiempoSeleccionAbarrotes < 0){
		tiempoSeleccionAbarrotes = -tiempoSeleccionAbarrotes;
	}

	ssEvLog << "==> id: "<< this->id << " se toma en seleccionar " << tiempoSeleccionAbarrotes << " segundos." << "\n";

	
	// Medias de abarrotes, pueden fluctuar segun la desviacion estandar, este valor puede ser parametrizado.
	// Por defecto su desviacion estandar es 10.

	abarrotesA = static_cast<int>(round(Random::normal(mediaAbarrotesA, 10)));
	if (abarrotesA < 0){
		abarrotesA = -abarrotesA;
	}

	ssEvLog << "Lleva: " << abarrotesA << " abarrotes de tipo A.\n";

	abarrotesB = static_cast<int>(round(Random::normal(mediaAbarrotesB, 10)));
	if (abarrotesB < 0){
		abarrotesB = -abarrotesB;
	}
	ssEvLog << "\t" << abarrotesB << " abarrotes de tipo B.\n\n";
	this->log(ssEvLog);



	theSim->scheduleEvent(new LlegadaCaja(time + tiempoSeleccionAbarrotes, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA, abarrotesB));

}

// Eventos de llegada a la fila de la caja
// Solo planificacion preestablecida, modelo original no considera eventos de abandono.
void LlegadaCaja::processEvent()
{
	std::stringstream ssEvLog;
	
	ssEvLog << "==> llega a la fila de la caja.\n";
	this->log(ssEvLog);
	
	if( theSim->cajaLibre ){
		theSim->cajaLibre = false;
		ssEvLog << "==> pasa a la caja.\n";
		this->log(ssEvLog);
		
		Event* ev = new OcuparCaja(time, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA, abarrotesB);
		ev->itRescheduled = false;
		theSim->scheduleEvent(ev);	
	}
	else{
		// El evento de llegada debe ser re-planificado.
		// el nuevo tiempo es 'newTime'
		
		// (1) determinar el tiempo de postergación
		double newTime;
		newTime = time + theSim->rescheduleTime;
		
		ssEvLog << std::setprecision(6) << std::fixed;
		ssEvLog << "==> caja ocupada, replanificado para t=" << newTime << "\n";
		this->log(ssEvLog);
		
		// (2) Se crea un nuevo evento, manteniendo el mismo identificador del 
		//     evento original
		Event* ev = new LlegadaCaja(newTime, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA, abarrotesB);
		ev->itRescheduled = true;
		
		// (3) Se planifica el nuevo evento
		theSim->scheduleEvent(ev);	
		
		// (4) El evento actual es eliminado en el ciclo de simulación
		
	}
	
}




// Cliente es atendido por la caja
void OcuparCaja::processEvent()
{
	std::stringstream ssEvLog;
	
	theSim->cajaLibre = false;

	
	ssEvLog << "==> Llega a la caja con tiempo:" << time <<  "\n";

	this->log(ssEvLog);
	
	if (abarrotesA==0){
		theSim->scheduleEvent(new EscanearB(time, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA, abarrotesB));
	}else{
		theSim->scheduleEvent(new EscanearA(time, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA, abarrotesB));
	}

}

// Escaneado de productos de tipo A
void EscanearA::processEvent()
{
	std::stringstream ssEvLog;

	// Tiempo que se va a demorar en pasar las compras
	uint32_t  Tservicio = Random::integer(1,10);

	if (abarrotesA > 0) {

        double fallo = Random::integer(0, 100);

        if (fallo < rateFallo) {

			total_a++;
            Tservicio += Random::integer(10, 30);  // Tiempo extra si hay fallo en el escaneo. Mismos valores de los abarrotes de tipo B
            ssEvLog << "==> Fallo en el abarrote. +" << Tservicio << " segundos\n";

        } else {
			total_a++;
            ssEvLog << "==> Se escanea abarrote. +" << Tservicio << " segundos\n";

        }
		this->log(ssEvLog);
        theSim->scheduleEvent(new EscanearA(time + Tservicio, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA - 1, abarrotesB));

    } else if (abarrotesA == 0 && abarrotesB > 0) {

        // Si no quedan abarrotes A pero aún hay abarrotes B, comienza a escanear B
        theSim->scheduleEvent(new EscanearB(time, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA, abarrotesB));

    } else if (abarrotesA == 0 && abarrotesB == 0) {

        // Si no quedan ni abarrotes A ni B, planifica el evento de salida
        theSim->scheduleEvent(new Salir(time, id, tasaSeleccionAbarrotes, rateFallo));

    }

}

// Escaneado de productos de tipo B
void EscanearB::processEvent()
{
	std::stringstream ssEvLog;

	// Escanear los articulos de tipo B toma entre 10 y 30 segundos
	uint32_t  Tservicio = Random::integer(10, 30); 

	
	if (abarrotesB > 0) {

		total_b++;
        theSim->scheduleEvent(new EscanearB(time + Tservicio, id, tasaSeleccionAbarrotes, rateFallo, abarrotesA, abarrotesB - 1));
        ssEvLog << "==> Escanea abarrote de tipo B. +" << Tservicio << " segundos\n";
		this->log(ssEvLog);

    } else if (abarrotesB == 0 && abarrotesA == 0) {

        // Si ya no quedan abarrotes de tipo B y se han terminado los de tipo A, se planifica el evento de salida
        theSim->scheduleEvent(new Salir(time + Tservicio, id, tasaSeleccionAbarrotes, rateFallo));

    }
}

void Salir::processEvent()
{
	std::stringstream ssEvLog;
	
	theSim->cajaLibre = true;
	
	ssEvLog << "==> Fin servicio.\n";
	this->log(ssEvLog);
	
	e++;
	// Debe replanificar los eventos que fueron pospuestos
	theSim->rescheduleDelayedEvents();

	
}

