#ifndef CPU_H_
#define CPU_H_

#include<stdio.h>
#include<stdlib.h>
#include<utils/utils.h>
#include <utils/utils.c>

//file descriptors de CPU y los modulos que se conectaran con el
int fd_cpu_dispatch;
int fd_cpu_interrupt; //luego se dividira en dos fd un dispatch y un interrupt, por ahora nos es suficiente con este
int fd_memoria;
int fd_kernel_dispatch;
int fd_kernel_interrupt;

sem_t sem_exe_a;
sem_t sem_exe_b;


t_log* cpu_logger; //LOG ADICIONAL A LOS MINIMOS Y OBLIGATORIOS
t_config* cpu_config;
//creemos una variable global de instruccion actual
char* instruccionActual;
char* IP_MEMORIA;
char* PUERTO_MEMORIA;
char* PUERTO_ESCUCHA_DISPATCH;
char* PUERTO_ESCUCHA_INTERRUPT;
int CANTIDAD_ENTRADAS_TLB;
char* ALGORITMO_TLB;

void enviar_pcb_memoria(int PID, int PC)
{ 
	//enviar pcb
	PCB* to_send = malloc(sizeof(PCB));
	to_send->PC = PC;
	to_send->PID = PID;
	to_send->quantum = 0;
	to_send->registro.AX  = 0;
	to_send->registro.BX = 0;
	to_send->registro.CX = 0;
	to_send->registro.DX = 0;
	to_send->registro.EAX = 0;//cambios
	to_send->registro.EBX = 0;//cambios
	to_send->registro.ECX = 0;
	to_send->registro.EDX = 0;//cambios
	to_send->registro.DI = 0;
	to_send->registro.SI = 0;//cambios

	to_send->estado = NEW;
	to_send->path_length = 1;
	to_send->path = "";
	enviarPCB (to_send, fd_memoria,PROCESO);

/*
	//esperar recibir mensaje de memoria
	t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
	paquete->buffer = malloc(sizeof(t_newBuffer));
	recibir_operacion(fd_memoria);
	recv(fd_memoria,&(paquete->buffer->size),sizeof(uint32_t),0);		
	paquete->buffer->stream = malloc(paquete->buffer->size);
	recv(fd_memoria,paquete->buffer->stream, paquete->buffer->size,0);
	printf("recibi el mensaje\n");

	void *stream = paquete->buffer->stream;
	memcpy(&(paquete->buffer->size), stream, sizeof(uint32_t));
	stream += sizeof(uint32_t);
	// deserailizamos el path como tal
	char* ret = malloc(paquete->buffer->size);
	memcpy(ret, stream, paquete->buffer->size);
	printf("deserialice el paquete correctamente\n");
*/

}

//UNA POSIBLE SOLUCION Y LOS CASOS.
//UNA POSIBLE SOLUCION Y LOS CASOS.
/*
void* obtener_registro(char* registro, PCB* proceso)
{
	if( strcmp(registro,"AX") == 0 )
	{
		//return *proceso->AX;// 
		return (void*)&(proceso->registro.AX); // Devuelve un puntero al valor de AX
	}
	else
	{
		if( strcmp(registro,"BX") == 0 )//cambios
		{
			return (void*)&(proceso->registro.BX); // Devuelve un puntero al valor de BX
		}
		else
		{
			if( strcmp(registro,"CX") == 0 )//cambios
			{
				return (void*)&(proceso->registro.CX); // Devuelve un puntero al valor de CX
			}
			else
			{
				if( strcmp(registro,"DX") == 0 )
				{
					return (void*)&(proceso->registro.DX); // Devuelve un puntero al valor de DX
				}
				else
				{
					printf("ERROR, NO SE ENCONTRO REGISTRO\n");
				}
			}
		}
	}
	return NULL;
}*/

void* obtener_registro(char* registro, PCB* proceso)
{
	if( strcmp(registro,"AX") == 0 )
	{
		return (void*)&(proceso->registro.AX); // Devuelve un puntero al valor de AX
	}
	else
	{
		if( strcmp(registro,"BX") == 0 )
		{
			return (void*)&(proceso->registro.BX); // Devuelve un puntero al valor de BX
		}
		else
		{
			if( strcmp(registro,"CX") == 0 )
			{
				return (void*)&(proceso->registro.CX); // Devuelve un puntero al valor de CX
			}
			else
			{
				if( strcmp(registro,"DX") == 0 )
				{
					return (void*)&(proceso->registro.DX); // Devuelve un puntero al valor de DX
				}
				else
				{
					printf("REGISTRO uint32\n");
	// casos registros uint32
					if( strcmp(registro,"EAX") == 0 )
					{
						return (void*)&(proceso->registro.EAX); // Devuelve un puntero al valor de EAX
					}
					else
					{
						if( strcmp(registro,"EBX") == 0 )
						{
							return (void*)&(proceso->registro.EBX); // Devuelve un puntero al valor de EBX
						}
						else{
							if( strcmp(registro,"ECX") == 0 )
							{
								return (void*)&(proceso->registro.ECX); // Devuelve un puntero al valor de ECX
							}
							else
							{
								if( strcmp(registro,"EDX") == 0 )
								{
									return (void*)&(proceso->registro.EDX); // Devuelve un puntero al valor de EDX
								}
								else
								{
									if( strcmp(registro,"SI") == 0 )
									{
										return (void*)&(proceso->registro.SI); // Devuelve un puntero al valor de SI
									}
									else
									{
										if( strcmp(registro,"DI") == 0 )
										{
											return (void*)&(proceso->registro.DI); // Devuelve un puntero al valor de DI
										}
										else
										{
											printf("ERROR, NO SE ENCONTRO REGISTRO\n");
										}
									}
								}
							}
						}

					}

				}
			}
		}
	}
	return NULL;
}



bool esRegistroUint8(char* registro)
{
	bool to_ret = false;
	if( strcmp(registro, "AX") == 0 )
	{
		to_ret = true;
	}
	else
	{
		if( strcmp(registro, "BX") == 0 )
		{
			to_ret = true;
		}
		else
		{
			if( strcmp(registro, "CX") == 0 )
			{
				to_ret = true;
			}
			else
			{
				if( strcmp(registro, "CX") == 0 )
				{
					to_ret = true;
				}
			}
		}
	}
	return to_ret;
}


bool esRegistroUint32(char* registro)
{
	bool to_ret = false;
	if( strcmp(registro, "EAX") == 0 )
	{
		to_ret = true;
	}
	else
	{
		if( strcmp(registro, "EBX") == 0 )
		{
			to_ret = true;
		}
		else
		{
			if( strcmp(registro, "ECX") == 0 )
			{
				to_ret = true;
			}
			else
			{
				if( strcmp(registro, "EDX") == 0 )
				{
					to_ret = true;
				}
				else
				{
					if( strcmp(registro, "SI") == 0 )
					{
						to_ret = true;
					}
					else
					{
						if( strcmp(registro, "DI") == 0 )
						{
							to_ret = true;
						}
					}
				}
			}
		}
	}
	return to_ret;
}



void enviar_instruccion_kernel (char** instruccion, int tam_instruccion, PCB* proceso, op_code codigo_operacion ){
	void* to_send = malloc( sizeof(tam_instruccion)+ sizeof(op_code)+sizeof(PCB));
	to_send = instruccion;
//INSTRUCCION,TAM INSTRUCCION ,PCB
	t_newBuffer* buffer = malloc(sizeof(t_newBuffer));

	//Calculamos su tamaño
    
	buffer->size = (tam_instruccion+1) + sizeof (PCB) + ;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
	//
    memcpy(buffer->stream + buffer->offset, instruccion, tam_instruccion);
    
	char* paquete = malloc( sizeof(tam_instruccion));
	paquete->buffer = buffer;
	void* a_enviar = malloc(buffer->size);
    int offset = 0;
	
	memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
    //Por último enviamos
    send(fd_kernel_dispatch, a_enviar, buffer->size, 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
	free(to_send);
}

void ejecutar_proceso(PCB* proceso)
{
	//enviar mensaje a memoria, debemos recibir primera interrupcion
	instruccionActual = "Goku";
	enviar_pcb_memoria(proceso->PID,proceso->PC);

	sem_wait(&sem_exe_b);
	while( strcmp(instruccionActual,"") != 0 )
	{
		//obtengo instruccion actual
		char* instruccion = string_duplicate(instruccionActual);
		printf("%s\n",instruccion); //verificamos que la instruccion actual sea correcta
		char** instruccion_split = string_split (instruccion, " ");

		//CASO DE TENER UNA INSTRUCCION SET
		if(strcmp(instruccion_split[0], "SET") == 0)
		{
			if( esRegistroUint8(instruccion_split[1]))
			{
				uint8_t* valor_registro = (uint8_t*)obtener_registro(instruccion_split[1],proceso);
				if( valor_registro != NULL )
				{
					int dato = atoi(instruccion_split[2]);
					*valor_registro = dato;
					printf("El valor de %s es: %d\n", instruccion_split[1], *valor_registro);
				}
				else
				{
					printf("El registro no se encontró en el proceso.\n");
				}
			}
			else
			{
				if( esRegistroUint32(instruccion_split[1]))
				{
					uint32_t* valor_registro = (uint32_t*)obtener_registro(instruccion_split[1],proceso);
					if( valor_registro != NULL )
					{
						int dato = atoi(instruccion_split[2]);
						*valor_registro = dato;
						printf("El valor de %s es: %d\n", instruccion_split[1], *valor_registro);
					}
					else
					{
						printf("El registro no se encontró en el proceso.\n");
					}
				}
			}
		}
		//CASO DE TENER UNA INSTRUCCION SUM
		if (strcmp(instruccion_split[0], "SUM") == 0)
		{
			if( esRegistroUint8(instruccion_split[1]) )
			{
				//si es un registro de 8 bits, tenemos que interpretarlo como tal
				uint8_t* valor_registro1 = (uint8_t*)obtener_registro(instruccion_split[1],proceso);
				uint8_t* valor_registro2 = (uint8_t*)obtener_registro(instruccion_split[2],proceso);
				if (valor_registro1 != NULL && valor_registro2 != NULL) 
				{
					*valor_registro1 = *valor_registro1 + *valor_registro2; // Asigna el valor a través del puntero
					printf("El valor de %s es: %d\n", instruccion_split[1], *valor_registro1);
				} 
				else 
				{
					printf("El registro no se encontró en el proceso.\n");
				}
			}
			else
			{
				if( esRegistroUint32(instruccion_split[1]) )
				{
					uint32_t* valor_registro1 = (uint32_t*)obtener_registro(instruccion_split[1],proceso);
					uint32_t* valor_registro2 = (uint32_t*)obtener_registro(instruccion_split[2],proceso);
					if (valor_registro1 != NULL && valor_registro2 != NULL) 
					{
						*valor_registro1 = *valor_registro1 + *valor_registro2;
						printf("El valor de %s es: %d\n", instruccion_split[1], *valor_registro1);
					} 
					else 
					{
						printf("El registro no se encontró en el proceso.\n");
					}
				}
			}
		}
		//CASO DE TENER UNA INSTRUCCION SUB
		if (strcmp(instruccion_split[0], "SUB") == 0)
		{
			//TENEMOS QUE APRECIAR LOS CASOS DONDE OBTENGAMOS AX, BX, CX o DX y los casos donde tengamos EAX, EBX, ECX, EDX, SI o DI. ASUMIENDO QUE NO SE INTENTA SUMAR UN AX con EAX
			if( esRegistroUint8(instruccion_split[1]) )
			{
				//si es un registro de 8 bits, tenemos que interpretarlo como tal
				uint8_t* valor_registro1 = (uint8_t*)obtener_registro(instruccion_split[1],proceso);
				uint8_t* valor_registro2 = (uint8_t*)obtener_registro(instruccion_split[2],proceso);
				if (valor_registro1 != NULL && valor_registro2 != NULL) 
				{
					*valor_registro1 = *valor_registro1 - *valor_registro2; // Asigna el valor a través del puntero
					printf("El valor de %s es: %d\n", instruccion_split[1], *valor_registro1);
				} 
				else 
				{
					printf("El registro no se encontró en el proceso.\n");
				}
			}
			else
			{
				if( esRegistroUint32(instruccion_split[1]) )
				{
					//si es un registro de 8 bits, tenemos que interpretarlo como tal
					uint32_t* valor_registro1 = (uint32_t*)obtener_registro(instruccion_split[1],proceso);
					uint32_t* valor_registro2 = (uint32_t*)obtener_registro(instruccion_split[2],proceso);
					if (valor_registro1 != NULL && valor_registro2 != NULL) 
					{
						*valor_registro1 = *valor_registro1 - *valor_registro2; // Asigna el valor a través del puntero
						printf("El valor de %s es: %d\n", instruccion_split[1], *valor_registro1);
					} 
					else 
					{
						printf("El registro no se encontró en el proceso.\n");
					}
				}
			}
		}
		//CASO DE TENER UNA INSTRUCCION JNZ
		if (strcmp(instruccion_split[0], "JNZ") == 0)
		{
			if( esRegistroUint8(instruccion_split[1]))
			{
				uint8_t* valor_registro = (uint8_t*)obtener_registro(instruccion_split[1],proceso);
				if( valor_registro != NULL )
				{
					if( *valor_registro != 0 )
					{
						proceso->PC = atoi(instruccion_split[2]);
						printf("El registro PC ha sido modificado a: %d", proceso->PC);
					}
				}
				else
				{
					printf("El registro no se encontró en el proceso.\n");
				}
			}
			else
			{
				if( esRegistroUint32(instruccion_split[1]))
				{
					uint32_t* valor_registro = (uint32_t*)obtener_registro(instruccion_split[1],proceso);
					if( valor_registro != NULL )
					{
						if( *valor_registro != 0 )
						{
							proceso->PC = atoi(instruccion_split[2]);
							printf("El registro PC ha sido modificado a: %d", proceso->PC);
						}
					}
				}
				else
				{
					printf("El registro no se encontró en el proceso.\n");
				}
				
			}
		}

		//CASO DE TENER UNA INSTRUCCION IO_GEN_SLEEP
		if (strcmp(instruccion_split[0], "IO_GEN_SLEEP") == 0)
		{
			int instruccion_length = strlen(instruccion_split)+1;
			enviar_instruccion_kernel(instruccion_split, instruccion_length,proceso,IO_GEN_SLEEP);
			
		}
		
		//CASO DE TENER UNA INSTRUCCION 
		
		//AUMENTAMOS EL PC Y PEDIMOS NUEVAMENTE
			proceso->PC++;
			enviar_pcb_memoria(proceso->PID,proceso->PC);
			printf("------------------------------\n");
			sem_post(&sem_exe_a);
			sem_wait(&sem_exe_b);
		
	}
	enviarPCB(proceso,fd_kernel_dispatch,PROCESOFIN);
}



void cpu_escuchar_kernel_dispatch (){
		bool control_key = 1;
	while (control_key) {
			int cod_op = recibir_operacion(fd_kernel_dispatch);

			//debemos extraer el resto, primero el tamaño y luego el contenido
			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
			recv(fd_kernel_dispatch,&(paquete->buffer->size),sizeof(uint32_t),0);
				
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(fd_kernel_dispatch,paquete->buffer->stream, paquete->buffer->size,0);
			
		    switch (cod_op) {
			case MENSAJE:
				//
				break;
			case PAQUETE:
				//
				break;
			case PROCESO:
				PCB* proceso = deserializar_proceso_cpu(paquete->buffer);

				printf("Recibi el siguiente proceso:\n");
				printf("Su PID es: %d\n",proceso->PID);
				printf("Su AX es: %d\n",proceso->registro.AX);//cambios
				printf("Su BX es: %d\n",proceso->registro.BX);
				printf("Su CX es: %d\n",proceso->registro.CX);//cambios
				printf("Su DX es: %d\n",proceso->registro.DX);
				printf("Su DI es: %d\n",proceso->registro.DI);//cambios
				printf("Su EAX es: %d\n",proceso->registro.EAX);

				ejecutar_proceso(proceso);
				
				sem_init(&sem_exe_a,0,1);
                sem_init(&sem_exe_b,0,0);
			    break;
			case -1:
				log_error(cpu_logger, "El cliente Kernel se desconecto. Terminando servidor\n");
				control_key = 0;
			default:
				log_warning(cpu_logger,"Operacion desconocida. No quieras meter la pata\n");
				break;
			}
			free(paquete->buffer->stream);
			free(paquete->buffer);
			free(paquete);

		}
}

void cpu_escuchar_kernel_interrupt (){
		bool control_key = 1;
	while (control_key) {
			int cod_op = recibir_operacion(fd_kernel_interrupt);

			//debemos extraer el resto, primero el tamaño y luego el contenido
			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
			recv(fd_kernel_interrupt,&(paquete->buffer->size),sizeof(uint32_t),0);
				
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(fd_kernel_interrupt,paquete->buffer->stream, paquete->buffer->size,0);
			
		    switch (cod_op) {
			case MENSAJE:
				//
				break;
			case PAQUETE:
				//
				break;
			case PROCESO:
				printf("Hola recibi algo por interrupt\n");
			    break;
			case -1:
				log_error(cpu_logger, "El cliente Kernel se desconecto. Terminando servidor\n");
				control_key = 0;
			default:
				log_warning(cpu_logger,"Operacion desconocida. No quieras meter la pata\n");
				break;
			}
			free(paquete->buffer->stream);
			free(paquete->buffer);
			free(paquete);

		}
}

void cpu_escuchar_memoria (){
		bool control_key = 1;
	while (control_key) {
            //recibimos operacion y mensaje
			int cod_op = recibir_operacion(fd_memoria);

			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
            //deserializamos el mensaje, primero el tamaño del buffer, luego el offset
			recv(fd_memoria,&(paquete->buffer->size),sizeof(uint32_t),0);		
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(fd_memoria,&(paquete->buffer->offset), sizeof(uint32_t),0);
			recv(fd_memoria,paquete->buffer->stream,paquete->buffer->size,0);
			

			switch (cod_op) {
			case MENSAJE:
			    sem_wait(&sem_exe_a);
				printf("Instruccion de la memoria recibida con exito\n");
				//variable global con 
				instruccionActual = malloc(paquete->buffer->size);
				char* instruccionQueLlego = paquete->buffer->stream;
				instruccionActual = string_duplicate(instruccionQueLlego);
				printf("La instruccion que llego fue: %s\n",instruccionActual);
				//instruccionActual = paquete->buffer->stream;
				sem_post(&sem_exe_b);
				break;
			case PAQUETE:
				//
				break;
			case -1:
				log_error(cpu_logger, "Desconexion de memoria.\n");
				control_key = 0;
			default:
				log_warning(cpu_logger,"Operacion desconocida. No quieras meter la pata\n");
				break;
			}
			//liberamos todo para evitar los errores pasados
			free(paquete->buffer->stream);
			free(paquete->buffer);
			free(paquete);
		}	
}


#endif /* CPU_H_ */