#ifndef KERNEL_H_
#define KERNEL_H_

#include <stdio.h>
#include <stdlib.h>
#include <utils/utils.h>
#include <utils/utils.c>

//file descriptors de kernel y los modulos que se conectaran con ella
int fd_kernel;
int fd_cpu_interrupt; 
int fd_cpu_dispatch; //luego se dividira en dos fd, un dispach y un interrupt, por ahora nos es suficiente con este
int fd_memoria;
int fd_entradasalida;
bool cpu_ocupada; //0 si no, 1 si sí

//colas y pid
int procesos_en_new = 0;
int procesos_fin = 0;

sem_t sem; //semaforo mutex region critica cola new
sem_t sem_ready; 
sem_t sem_cant; //semaforo cant de elementos en la cola
sem_t sem_cant_ready;
sem_t sem_mutex_plani_corto; //semaforo oara planificacion FIFO
sem_t sem_mutex_cpu_ocupada; //semaforo para indicar si la cpu esta ocupada
sem_t sem_blocked;

//semaforos para garantizar exclusión mutua al acceso de las listas de ios
sem_t sem_mutex_lists_io; //un unico semaforo para el acceso a la listas de ios

//lista de recursos
t_list* listRecursos;

//TODAS LAS QUEUES
int pid = 0;
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_blocked;

//LISTA DE ENTRADAS Y SALIDAS
t_list* listGenericas;
t_list* listStdin;
t_list* listStdout;
t_list* listDialfs;

t_log *kernel_logger; // LOG ADICIONAL A LOS MINIMOS Y OBLIGATORIOS
t_config *kernel_config;
t_log *kernel_logs_obligatorios;// LOGS MINIMOS Y OBLIGATORIOS

char *PUERTO_ESCUCHA;
char *IP_MEMORIA;
char *PUERTO_MEMORIA;
char *IP_CPU;
char *PUERTO_CPU_DISPATCH; 
char *PUERTO_CPU_INTERRUPT;
char *ALGORITMO_PLANIFICACION;
char *QUANTUM; //Da segmentation fault si lo defino como int
char* RECURSOS;
char* INSTANCIAS_RECURSOS;
char* GRADO_MULTIPROGRAMACION; //Da segmentation fault si lo defino como int

void removerCorchetes(char* str) {
    int len = strlen(str);
    int j = 0;

    for (int i = 0; i < len; i++) {
        if (str[i] != '[' && str[i] != ']') {
            str[j++] = str[i];
        }
    }
    str[j] = '\0'; // Añadir el carácter nulo al final de la cadena resultante
}

bool es_elemento_buscado(void* elemento, void* valor_buscado) {
    return *((uint32_t*)elemento) == *((uint32_t*)valor_buscado);
}

// Función para eliminar un elemento de la lista
bool eliminar_elemento(t_list* lista, uint32_t valor) 
{
	bool to_ret;
    uint32_t* valor_buscado = malloc(sizeof(uint32_t));
    *valor_buscado = valor;

    // Iterar sobre la lista para encontrar el elemento
    for (int i = 0; i < list_size(lista); i++) {
        uint32_t* elemento = list_get(lista, i);
        if (es_elemento_buscado(elemento, valor_buscado)) {
            list_remove_and_destroy_element(lista, i, free);
			to_ret = true;
            return to_ret;
        }
    }
    free(valor_buscado);
	to_ret = false;
}

t_list* generarRecursos(char* recursos, char* instancias_recursos)
{
	//creamos la lista a retornar
	t_list* ret = list_create();
	//sacamos los corchetes
	removerCorchetes(recursos);
	removerCorchetes(instancias_recursos);
	//los separamos por las comas
	char** recursos_separados = string_split(recursos,",");
	char** instancias_recursos_separados = string_split(instancias_recursos,",");
	//vamos creando los Recurso y añadimos a la lista
	int i = 0;
	while( recursos_separados[i] != NULL )
	{
		Recurso* to_add = malloc(sizeof(Recurso));
		strncpy(to_add->name, recursos_separados[i], sizeof(to_add->name) - 1);
		to_add->instancias = atoi(instancias_recursos_separados[i]);
		to_add->listBloqueados = list_create();
		to_add->pid_procesos = list_create();
		list_add(ret,to_add);
		i++;
	}
	return ret;
}

void ejecutar_interfaz_generica(char* instruccion, op_code tipoDeInterfaz, int fd_io, int pid_actual)
{
	char** instruccion_split = string_split(instruccion," ");
	int* unidadesDeTrabajo = malloc(sizeof(int));
	*unidadesDeTrabajo = atoi(instruccion_split[2]);

	int* pid_io = malloc(sizeof(int));
	*pid_io = pid_actual;

	enviarEntero(pid_io,fd_io,NUEVOPID);
	enviarEntero(unidadesDeTrabajo,fd_io,GENERICA);
}


void ejecutar_interfaz_stdinstdout(char* instruccion, op_code tipoDeInterfaz, int fd_io, int pid_actual)
{
	char** instruccion_split = string_split(instruccion," ");
	//IO_STDIN_READ (Interfaz, Registro Direccion, Registro Tamaño)
	//Debemos enviar la direccion y el tamanio a escribir
	int* direccionFisica = malloc(sizeof(int));
	int* tamanio = malloc(sizeof(int));
	*direccionFisica = atoi(instruccion_split[4]);
	*tamanio = atoi(instruccion_split[5]);

	//antes que todo enviamos del pid del proceso que actualmente lo usa
	int* pid = malloc(sizeof(int));
	*pid = pid_actual;
	enviarEntero(pid,fd_io,NUEVOPID);

	//mandaremos
	printf("mandaremos la siguiente direccion fisica: %d\n",*direccionFisica);
	printf("mandaremos el siguiente tamanio: %d\n",*tamanio);

	printf("Voy a mandar algo para el stdin\n");
	t_newBuffer* buffer = malloc(sizeof(t_newBuffer));

    //Calculamos su tamaño
	buffer->size = sizeof(int)*2;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
	
    //Movemos los valores al buffer
    memcpy(buffer->stream + buffer->offset,direccionFisica, sizeof(int));
    buffer->offset += sizeof(int);
	memcpy(buffer->stream + buffer->offset,tamanio, sizeof(int));
    buffer->offset += sizeof(int);

	//Creamos un Paquete
    t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
    //Podemos usar una constante por operación
    paquete->codigo_operacion = tipoDeInterfaz;
    paquete->buffer = buffer;

	//Empaquetamos el Buffer
    void* a_enviar = malloc(buffer->size + sizeof(op_code) + sizeof(uint32_t));
    int offset = 0;
    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));
    offset += sizeof(op_code);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
    //Por último enviamos
    send(fd_io, a_enviar, buffer->size + sizeof(op_code) + sizeof(uint32_t), 0);
	printf("Lo mande\n");
    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
	
}

EntradaSalida* encontrar_io(t_list* lista, const char* nombre_buscado) {
    for (int i = 0; i < list_size(lista); i++) 
	{
        EntradaSalida* entrada = list_get(lista, i);
        if (strcmp(entrada->nombre, nombre_buscado) == 0) {
            return entrada;
        }
    }
    return NULL; // Si no se encuentra el elemento
}

PCB* encontrar_por_pid(t_list* lista, uint32_t pid_buscado) {
    for (int i = 0; i < list_size(lista); i++) 
	{
        PCB* pcb = list_get(lista, i);
        if (pcb->PID == pid_buscado) 
		{
			pcb = list_remove(lista, i);
            return pcb;
        }
    }
    return NULL; // Si no se encuentra el elemento
}

void liberar_recursos(int pid)
{
	for(int i = 0; i < list_size(listRecursos); i++)
	{
		Recurso* r = list_get(listRecursos,i);
		bool borrado = true;
		while( borrado )
		{
			borrado = eliminar_elemento(r->pid_procesos,pid);
			if( borrado )
			{
				r->instancias++;
				if (r->instancias <= 0) 
					{
						PCB* proceso_desbloqueado = list_remove(r->listBloqueados, 0);
						if (proceso_desbloqueado != NULL) {
							//añadimos que dicho proceso está usando una instancia
							uint32_t* pid = malloc(sizeof(uint32_t));
							*pid = proceso_desbloqueado->PID;
							list_add(r->pid_procesos,pid);

							// Enviamos el proceso a la cola de ready
							sem_wait(&sem_ready);   // mutex hace wait
							queue_push(cola_ready, proceso_desbloqueado); // agrega el proceso a la cola de ready
							sem_post(&sem_ready);
							sem_post(&sem_cant_ready);  // mutex hace wait
						}
					}
			}
		}
	}
}

void kernel_escuchar_cpu ()
{
	bool control_key = 1;
	while (control_key) {
	//recibimos operacion y guardamos todo en un paquete
			int cod_op = recibir_operacion(fd_cpu_dispatch);

			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
			recv(fd_cpu_dispatch,&(paquete->buffer->size),sizeof(uint32_t),0);	
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(fd_cpu_dispatch,paquete->buffer->stream, paquete->buffer->size,0);

			switch (cod_op) {
			case PROCESOFIN:
			//recibo el proceso de la CPU
			    PCB* proceso = deserializar_proceso_cpu(paquete->buffer);
				proceso->estado = EXIT;
				printf("Recibimos el proceso con el pid: %d\n",proceso->PID);
				printf("Recibimos el proceso con el AX: %d\n",proceso->registro.AX);//cambios
				printf("Recibimos el proceso con el BX: %d\n",proceso->registro.BX);//cambios
				printf("Recibimos el proceso con el CX: %d\n",proceso->registro.CX);//cambios
				printf("Recibimos el proceso con el DX: %d\n",proceso->registro.DX);//cambios
				printf("Recibimos el proceso con el EAX: %d\n",proceso->registro.EAX);//cambios
				printf("Recibimos el proceso con el EBX: %d\n",proceso->registro.EBX);//cambios
				printf("Recibimos el proceso con el ECX: %d\n",proceso->registro.ECX);//cambios
				printf("Recibimos el proceso con el EDX: %d\n",proceso->registro.EDX);//cambios
				printf("Recibimos el proceso con el SI: %d\n",proceso->registro.SI);//cambios
				printf("Recibimos el proceso con el DI: %d\n",proceso->registro.DI);//cambios
				printf("Este proceso ha terminado\n");
				liberar_recursos(proceso->PID);
				enviarPCB(proceso,fd_memoria,PROCESOFIN);

				free(proceso->path);
				free(proceso);
				
				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);

				printf("Los recursos han quedado de la siguiente forma:\n");
				for(int i = 0; i < list_size(listRecursos); i++)
				{
					Recurso* got = list_get(listRecursos,i);
					printf("El nombre del recurso es %s y tiene %d instancias\n",got->name,got->instancias);
				}				
			//	
                break;
			case PROCESOIO:

				PCB* proceso_io = deserializar_proceso_cpu(paquete->buffer);
				proceso_io->estado = BLOCKED;
				printf("Recibimos el proceso con el pid: %d\n",proceso_io->PID);
				printf("Recibimos el proceso con el AX: %d\n",proceso_io->registro.AX);//cambios
				printf("Recibimos el proceso con el BX: %d\n",proceso_io->registro.BX);//cambios
				printf("Recibimos el proceso con el CX: %d\n",proceso_io->registro.CX);//cambios
				printf("Recibimos el proceso con el DX: %d\n",proceso_io->registro.DX);//cambios
				printf("Recibimos el proceso con el EAX: %d\n",proceso_io->registro.EAX);//cambios
				printf("Recibimos el proceso con el EBX: %d\n",proceso_io->registro.EBX);//cambios
				printf("Recibimos el proceso con el ECX: %d\n",proceso_io->registro.ECX);//cambios
				printf("Recibimos el proceso con el EDX: %d\n",proceso_io->registro.EDX);//cambios
				printf("Recibimos el proceso con el SI: %d\n",proceso_io->registro.SI);//cambios
				printf("Recibimos el proceso con el DI: %d\n",proceso_io->registro.DI);//cambios
				printf("Este SE HA BLOQUEADO\n");

				//lo ideal seria tambien agregarlo a una cola de la interfaz de cada proceso
				sem_wait(&sem_blocked);
				queue_push(cola_blocked, proceso_io);
				sem_post(&sem_blocked);


				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);		

				break;
			case FIN_DE_QUANTUM:

				PCB* proceso_fin_de_quantum = deserializar_proceso_cpu(paquete->buffer);
				proceso_fin_de_quantum->estado = READY;
				printf("Recibimos el proceso con el pid: %d\n",proceso_fin_de_quantum->PID);
				printf("Recibimos el proceso con el AX: %d\n",proceso_fin_de_quantum->registro.AX);//cambios
				printf("Recibimos el proceso con el BX: %d\n",proceso_fin_de_quantum->registro.BX);//cambios
				printf("Recibimos el proceso con el CX: %d\n",proceso_fin_de_quantum->registro.CX);//cambios
				printf("Recibimos el proceso con el DX: %d\n",proceso_fin_de_quantum->registro.DX);//cambios
				printf("Recibimos el proceso con el EAX: %d\n",proceso_fin_de_quantum->registro.EAX);//cambios
				printf("Recibimos el proceso con el EBX: %d\n",proceso_fin_de_quantum->registro.EBX);//cambios
				printf("Recibimos el proceso con el ECX: %d\n",proceso_fin_de_quantum->registro.ECX);//cambios
				printf("Recibimos el proceso con el EDX: %d\n",proceso_fin_de_quantum->registro.EDX);//cambios
				printf("Recibimos el proceso con el SI: %d\n",proceso_fin_de_quantum->registro.SI);//cambios
				printf("Recibimos el proceso con el DI: %d\n",proceso_fin_de_quantum->registro.DI);//cambios
				printf("SE HA ACABADO EL QUANTUM DE ESTE PROCESO\n");
				log_info (kernel_logs_obligatorios, "PID: <%d> - Desalojado por fin de Quantum", proceso_fin_de_quantum->PID);

				//lo ideal seria tambien agregarlo a una cola de la interfaz de cada proceso
				sem_wait(&sem_ready);   // mutex hace wait
				queue_push(cola_ready,proceso_fin_de_quantum);	//agrega el proceso a la cola de ready
  				sem_post(&sem_ready); 
				sem_post(&sem_cant_ready);  // mutex hace wait
				
				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);	
				break;
			case GENERICA:
				
				//Deserializamos
				Instruccion_io* instruccion_io_gen = deserializar_instruccion_io(paquete->buffer);
				printf("La instruccion es: %s\n",instruccion_io_gen->instruccion);
				printf("El PID del proceso es: %d\n",instruccion_io_gen->proceso.PID);

				//Debemos obtener el IO específico de la lista
				sem_wait(&sem_mutex_lists_io);
				EntradaSalida* io_gen = encontrar_io(listGenericas,string_split(instruccion_io_gen->instruccion," ")[1]);
				sem_post(&sem_mutex_lists_io);

				//verificamos que exista
				if( io_gen != NULL )
				{
					//una vez encontrada la io, veo si está ocupado
					printf("El io encontrado tiene como nombre %s\n",io_gen->nombre);
					if( io_gen->ocupado )
					{
						//de estar ocupado, añado el proceso a la lista de este (al proceso con la instruccion y todo)
						printf("La IO está ocupada, se bloqueará en su lista propia\n");
						printf("En la lista se guarda la instruccion %s\n",instruccion_io_gen->instruccion);
						printf("En la lista se guarda el PID %d\n",instruccion_io_gen->proceso.PID);
						list_add(io_gen->procesos_bloqueados, instruccion_io_gen);
					}
					else
					{
						//marcamos la io como ocupada y ejecutamos
						io_gen->ocupado = true;
						ejecutar_interfaz_generica(instruccion_io_gen->instruccion,GENERICA,io_gen->fd_cliente,instruccion_io_gen->proceso.PID);
					}
				}
				else
				{
					//de no existir debemos terminar el proceso	
					sem_wait(&sem_blocked);				
					PCB* proceso_to_end = encontrar_por_pid(cola_blocked->elements,instruccion_io_gen->proceso.PID);
					sem_post(&sem_blocked);	
					printf("Este proceso ha terminado\n");
					liberar_recursos(proceso_to_end->PID);
					enviarPCB(proceso_to_end,fd_memoria,PROCESOFIN);
					free(proceso_to_end->path);
					free(proceso_to_end);
				}
					//CPU desocupada
					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = false;
					sem_post(&sem_mutex_cpu_ocupada);
				break;
			case STDIN:
				//Deserializamos
				Instruccion_io* instruccion_io_stdin = deserializar_instruccion_io(paquete->buffer);
				printf("La instruccion es: %s\n",instruccion_io_stdin->instruccion);
				printf("El PID del proceso es: %d\n",instruccion_io_stdin->proceso.PID);

				//debemos obtener el io específico de la lista
				sem_wait(&sem_mutex_lists_io);
				EntradaSalida* io_stdin = encontrar_io(listStdin,string_split(instruccion_io_stdin->instruccion," ")[1]);
				sem_post(&sem_mutex_lists_io);

				//verificamos que exista
				if( io_stdin != NULL)
				{
					//Una vez encontrada la io, vemos si está ocupado
					if( io_stdin->ocupado )
					{
						//de estar ocupado añado el proceso a la lista de este
						printf("La IO está ocupada, se bloqueará en su lista propia\n");
						list_add(io_stdin->procesos_bloqueados,instruccion_io_stdin);
					}
					else
					{
						io_stdin->ocupado = true;
						printf("El fd de este IO es %d\n",io_stdin->fd_cliente);
						ejecutar_interfaz_stdinstdout(instruccion_io_stdin->instruccion,STDIN,io_stdin->fd_cliente,instruccion_io_stdin->proceso.PID);
					}
				}
				else
				{
					//de no existir debemos terminar el proceso
					sem_wait(&sem_blocked);				
					PCB* proceso_to_end = encontrar_por_pid(cola_blocked->elements,instruccion_io_stdin->proceso.PID);
					sem_post(&sem_blocked);	
					printf("Este proceso ha terminado\n");
					liberar_recursos(proceso_to_end->PID);
					enviarPCB(proceso_to_end,fd_memoria,PROCESOFIN);
					free(proceso_to_end->path);
					free(proceso_to_end);
				}

				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);
				break;
			//case FS_CREATE:
				//
			//	break;
			case STDOUT:
				//deserializamos
				Instruccion_io* instruccion_io_stdout = deserializar_instruccion_io(paquete->buffer);
				printf("La instruccion es: %s\n",instruccion_io_stdout->instruccion);
				printf("El PID del proceso es: %d\n",instruccion_io_stdout->proceso.PID);

				//debemos obtener el io específico de la lista
				sem_wait(&sem_mutex_lists_io);
				EntradaSalida* io_stdout = encontrar_io(listStdout,string_split(instruccion_io_stdout->instruccion," ")[1]);
				sem_post(&sem_mutex_lists_io);

				//verificamos que exista
				if( io_stdout != NULL)
				{
					//existe
					//una vez encontrada la io, vemos si está ocupado
					if( io_stdout->ocupado )
					{
						//de estar ocupado añado el proceso a la lista de este
						printf("La IO está ocupada, se bloqueará en su lista propia\n");
						list_add(io_stdout->procesos_bloqueados,instruccion_io_stdout);
					}
					else
					{
						io_stdout->ocupado = true;
						printf("El fd de este IO es %d\n",io_stdout->fd_cliente);
						ejecutar_interfaz_stdinstdout(instruccion_io_stdout->instruccion,STDOUT,io_stdout->fd_cliente,instruccion_io_stdout->proceso.PID);
					}
				}
				else
				{
					//de no existir debemos terminar el proceso
					sem_wait(&sem_blocked);				
					PCB* proceso_to_end = encontrar_por_pid(cola_blocked->elements,instruccion_io_stdout->proceso.PID);
					sem_post(&sem_blocked);	
					printf("Este proceso ha terminado\n");
					liberar_recursos(proceso_to_end->PC);
					enviarPCB(proceso_to_end,fd_memoria,PROCESOFIN);
					free(proceso_to_end->path);
					free(proceso_to_end);
				}
				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);
				break;
			case WAIT:
				//Deserializamos el recurso
				Instruccion_io* instruccion_recurso_wait = deserializar_instruccion_io(paquete->buffer);
				printf("La instruccion es: %s\n",instruccion_recurso_wait->instruccion);
				printf("El PID del proceso es: %d\n",instruccion_recurso_wait->proceso.PID);
				char** instruccion_partida_wait = string_split(instruccion_recurso_wait->instruccion," ");

				//verificamos que el recurso exista
				bool existe_wait = false;
				int i_wait = 0;
				Recurso* recurso_wait;
				while( i_wait < list_size(listRecursos) && existe_wait != true )
				{
					recurso_wait = list_get(listRecursos,i_wait);
					if( strcmp(recurso_wait->name,instruccion_partida_wait[1]) == 0 )
					{
						existe_wait = true;
					}
					i_wait++;
				}

				//si existe
				if( existe_wait )
				{
					//reducimos las instancias
					recurso_wait->instancias--;

					//si es menor a cero, se bloquea
					if( recurso_wait->instancias < 0 )
					{
						//de lo contrario lo añadimos a la cola de bloqueados del recurso
						PCB* proceso_recurso = malloc(sizeof(PCB));
						*proceso_recurso = instruccion_recurso_wait->proceso;
						list_add(recurso_wait->listBloqueados,proceso_recurso);
						printf("El proceso se ha bloqueado porque no hay instancias disponibles\n");
						sem_wait(&sem_mutex_cpu_ocupada);
						cpu_ocupada = false;
						sem_post(&sem_mutex_cpu_ocupada);
						printf("En este momento la cola de ready consta de %d procesos\n",queue_size(cola_ready));
					}
					else
					{
						//indicamos que hay un proceso que tomó una instancia
						uint32_t* pid = malloc(sizeof(uint32_t));
						*pid = instruccion_recurso_wait->proceso.PID;
						list_add(recurso_wait->pid_procesos, pid);
						printf("En este momento el tamaño de la lista del recurso es de %d\n",list_size(recurso_wait->pid_procesos));

						//enviamos el proceso a la cola de ready
						PCB* proceso_recurso = malloc(sizeof(PCB));
						*proceso_recurso = instruccion_recurso_wait->proceso;
						sem_wait(&sem_ready);   // mutex hace wait
						queue_push(cola_ready,proceso_recurso);	//agrega el proceso a la cola de ready
						sem_post(&sem_ready); 
						sem_post(&sem_cant_ready);  // mutex hace wait
					
						sem_wait(&sem_mutex_cpu_ocupada);
						cpu_ocupada = false;
						sem_post(&sem_mutex_cpu_ocupada);
						printf("Los recursos han quedado de la siguiente forma:\n");
						for(int i = 0; i < list_size(listRecursos); i++)
						{
							Recurso* got = list_get(listRecursos,i);
							printf("El nombre del recurso es %s y tiene %d instancias\n",got->name,got->instancias);
						}
					}
				}
				else
				{
					//el recurso no existe, debemos terminarlo
					printf("El recurso no existe\n");
					printf("Este proceso ha terminado\n");
					printf("En este momento la cola de ready consta de %d procesos\n",queue_size(cola_ready));
					PCB* proceso_recurso = malloc(sizeof(PCB));
					*proceso_recurso = instruccion_recurso_wait->proceso;
					liberar_recursos(proceso_recurso->PID);
					enviarPCB(proceso_recurso,fd_memoria,PROCESOFIN);

					//EN ESTA PARTE, LO IDEAL SERÍA TENER UNA LISTA GLOBAL DE PROCESOS CON LA CUAL PODAMOS TENER SEGUIMIENTO TOTAL DE ELLOS.
					//De darse este caso, en dicha lista diríamos que el proceso ha finalizado.
					free(proceso_recurso);
					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = false;
					sem_post(&sem_mutex_cpu_ocupada);
				}
				
				break;
			case SIGNAL:
				// Deserializamos el recurso
				Instruccion_io* instruccion_recurso_signal = deserializar_instruccion_io(paquete->buffer);
				printf("La instruccion es: %s\n", instruccion_recurso_signal->instruccion);
				printf("El PID del proceso es: %d\n", instruccion_recurso_signal->proceso.PID);
				char** instruccion_partida_signal = string_split(instruccion_recurso_signal->instruccion, " ");

				// Verificamos que el recurso exista
				bool existe_signal = false;
				int i_signal = 0;
				Recurso* recurso_signal;
				while (i_signal < list_size(listRecursos) && existe_signal != true) {
					recurso_signal = list_get(listRecursos, i_signal);
					if (strcmp(recurso_signal->name, instruccion_partida_signal[1]) == 0) {
						existe_signal = true;
					}
					i_signal++;
				}

				// Si existe
				if (existe_signal) 
				{
					// Aumentamos las instancias
					recurso_signal->instancias++;

					// Si hay procesos bloqueados, los desbloqueamos
					if (recurso_signal->instancias <= 0) 
					{
						PCB* proceso_desbloqueado = list_remove(recurso_signal->listBloqueados, 0);
						if (proceso_desbloqueado != NULL) {
							//añadimos que dicho proceso está usando una instancia
							uint32_t* pid = malloc(sizeof(uint32_t));
							*pid = proceso_desbloqueado->PID;
							list_add(recurso_signal->pid_procesos,pid);

							// Enviamos el proceso a la cola de ready
							sem_wait(&sem_ready);   // mutex hace wait
							queue_push(cola_ready, proceso_desbloqueado); // agrega el proceso a la cola de ready
							sem_post(&sem_ready);
							sem_post(&sem_cant_ready);  // mutex hace wait
						}
					}
					
					//Tenemos que sacar el pid de la lista del recurso
					eliminar_elemento(recurso_signal->pid_procesos, instruccion_recurso_signal->proceso.PID);

					PCB* proceso_recurso = malloc(sizeof(PCB));
					*proceso_recurso = instruccion_recurso_signal->proceso;
					sem_wait(&sem_ready);   // mutex hace wait
					queue_push(cola_ready,proceso_recurso);	//agrega el proceso a la cola de ready
					sem_post(&sem_ready); 
					sem_post(&sem_cant_ready);  // mutex hace wait
					printf("(EXISTE SIGNAL)Se ha añadido un proceso con PID %d\n",proceso_recurso->PID);

					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = false;
					sem_post(&sem_mutex_cpu_ocupada);
				} 
				else 
				{
					// El recurso no existe, debemos terminarlo
					printf("El recurso no existe\n");
					printf("Este proceso ha terminado\n");
					PCB* proceso_recurso = malloc(sizeof(PCB));
					*proceso_recurso = instruccion_recurso_signal->proceso;
					liberar_recursos(proceso_recurso->PID);
					enviarPCB(proceso_recurso, fd_memoria, PROCESOFIN);

					// EN ESTA PARTE, LO IDEAL SERÍA TENER UNA LISTA GLOBAL DE PROCESOS CON LA CUAL PODAMOS TENER SEGUIMIENTO TOTAL DE ELLOS.
					// De darse este caso, en dicha lista diríamos que el proceso ha finalizado.
					free(proceso_recurso);
					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = false;
					sem_post(&sem_mutex_cpu_ocupada);
				}

				break;	
			case MENSAJE:
				//
				break;
			case PAQUETE:
				//
				break;
			case -1:
				log_error(kernel_logger, "Desconexion de cpu.");
				control_key = 0;
			default:
				log_warning(kernel_logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
			free(paquete->buffer->stream);
			free(paquete->buffer);
			free(paquete);
		}	
}

EntradaSalida* encontrar_por_fd_cliente(t_list* lista, int fd_cliente_buscado) {
    for (int i = 0; i < list_size(lista); i++) 
	{
        EntradaSalida* entrada = list_get(lista, i);
        if (entrada->fd_cliente == fd_cliente_buscado) {
            return entrada;
        }
    }
    return NULL; // Si no se encuentra el elemento
}

//encuentra la io en alguna de las listas y envía a un proceso si es que está ahí esperando
EntradaSalida* modificar_io_en_listas(int fd_io)
{
	EntradaSalida* to_ret;
	to_ret = encontrar_por_fd_cliente(listGenericas, fd_io);
	if( to_ret != NULL )
	{
		to_ret->ocupado = false;
		if( list_size(to_ret->procesos_bloqueados) > 0 )
		{
			Instruccion_io* proceso_bloqueado_io = list_remove(to_ret->procesos_bloqueados,0);
			printf("De la lista hemos extraído la instrucción %s\n",proceso_bloqueado_io->instruccion);
			printf("De la lista hemos extraído el pid %d\n",proceso_bloqueado_io->proceso.PID);
			ejecutar_interfaz_generica(proceso_bloqueado_io->instruccion,GENERICA,fd_io,proceso_bloqueado_io->proceso.PID);
		}
	}
	else
	{
		to_ret = encontrar_por_fd_cliente(listStdin, fd_io);
		if( to_ret != NULL )
		{
			to_ret->ocupado = false;
			if( list_size(to_ret->procesos_bloqueados) > 0 )
			{
				Instruccion_io* proceso_bloqueado_io = list_remove(to_ret->procesos_bloqueados,0);
				ejecutar_interfaz_stdinstdout(proceso_bloqueado_io->instruccion,STDIN,fd_io,proceso_bloqueado_io->proceso.PID);
			}
		}
		else
		{
			to_ret = encontrar_por_fd_cliente(listStdout, fd_io);
			if( to_ret != NULL )
			{
				to_ret->ocupado = false;
				if( list_size(to_ret->procesos_bloqueados) > 0 )
				{
					Instruccion_io* proceso_bloqueado_io = list_remove(to_ret->procesos_bloqueados,0);
					ejecutar_interfaz_stdinstdout(proceso_bloqueado_io->instruccion,STDOUT,fd_io,proceso_bloqueado_io->proceso.PID);
				}
			}
			else
			{
				to_ret = encontrar_por_fd_cliente(listDialfs, fd_io);
				to_ret->ocupado = false;
				if( list_size(to_ret->procesos_bloqueados) > 0 )
				{
					Instruccion_io* proceso_bloqueado_io = list_remove(to_ret->procesos_bloqueados,0);
				}
			}
		}
	}
}

//Al escuchar las entradas salidas
void kernel_escuchar_entradasalida_mult(int* fd_io)
{
	bool control_key = 1;
	while (control_key) {

			int cod_op = recibir_operacion(*fd_io);
			printf("El fd de este IO es %d\n",*fd_io);
			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
			recv(*fd_io,&(paquete->buffer->size),sizeof(uint32_t),0);	
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(*fd_io,paquete->buffer->stream, paquete->buffer->size,0);

			switch (cod_op) {
			case GENERICA:
				//creamos la interfaz genérica
				EntradaSalida* new_io_generica = deserializar_entrada_salida(paquete->buffer);
				new_io_generica->fd_cliente = *fd_io;
				printf("Llego una IO cuyo nombre es: %s\n",new_io_generica->nombre);
		   		printf("Llego una IO cuyo path es: %s\n",new_io_generica->path);
				
				//lo añadimos a la lista de ios genéricas.
				sem_wait(&sem_mutex_lists_io);
				list_add(listGenericas,new_io_generica);
				sem_post(&sem_mutex_lists_io);
			    break;
			case STDIN:
				//creamos la interfaz genérica
				EntradaSalida* new_io_stdin = deserializar_entrada_salida(paquete->buffer);
				new_io_stdin->fd_cliente = *fd_io;
				printf("Llego una IO cuyo nombre es: %s\n",new_io_stdin->nombre);
		   		printf("Llego una IO cuyo path es: %s\n",new_io_stdin->path);
				
				//lo añadimos a la lista de ios genéricas.
				sem_wait(&sem_mutex_lists_io);
				list_add(listStdin,new_io_stdin);
				sem_post(&sem_mutex_lists_io);		
			    break;
			case STDOUT:
				//creamos la interfaz genérica
				EntradaSalida* new_io_stdout = deserializar_entrada_salida(paquete->buffer);
				new_io_stdout->fd_cliente = *fd_io;
				printf("Llego una IO cuyo nombre es: %s\n",new_io_stdout->nombre);
		   		printf("Llego una IO cuyo path es: %s\n",new_io_stdout->path);

				//lo añadimos a la lista de ios genéricas.
				sem_wait(&sem_mutex_lists_io);
				list_add(listStdout,new_io_stdout);
				sem_post(&sem_mutex_lists_io);			
			    break;
			case DIALFS:
				//creamos la interfaz genérica
				EntradaSalida* new_io_dialfs = deserializar_entrada_salida(paquete->buffer);
				new_io_dialfs->fd_cliente = *fd_io;
				printf("Llego una IO cuyo nombre es: %s\n",new_io_dialfs->nombre);
		   		printf("Llego una IO cuyo path es: %s\n",new_io_dialfs->path);
				
				//lo añadimos a la lista de ios genéricas.
				sem_wait(&sem_mutex_lists_io);
				list_add(listDialfs,new_io_dialfs);
				sem_post(&sem_mutex_lists_io);
			    break;
			case DESPERTAR:
				//sacamos al proceso de la cola de blocked y lo añadimos a IO
				printf("La IO ha despertado :O\n");
				int* pid_io = paquete->buffer->stream;
				printf("El pid que ha llegado de la IO es %d\n",*pid_io);

				//PCB* proceso_awaken = queue_pop(cola_blocked);
				sem_wait(&sem_blocked);
				PCB* proceso_awaken = encontrar_por_pid(cola_blocked->elements,(uint32_t)*pid_io);
				sem_post(&sem_blocked);
				
				proceso_awaken->estado = READY;
				//meter en ready
				sem_wait(&sem_ready);   // mutex hace wait
				queue_push(cola_ready,proceso_awaken);	//agrega el proceso a la cola de ready
    			sem_post(&sem_ready); 
				sem_post(&sem_cant_ready);

				//también debemos marcar la io como desocupada, buscamos por fd
				sem_wait(&sem_mutex_lists_io);
				modificar_io_en_listas(*fd_io);
				sem_post(&sem_mutex_lists_io);
			    break;
			case MENSAJE:
				//
				break;
			case PAQUETE:
				//
				break;
			case -1:
				log_error(kernel_logger, "El cliente EntradaSalida se desconecto. Terminando servidor");
				control_key = 0;
			default:
				log_warning(kernel_logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
			free(paquete->buffer->stream);
			free(paquete->buffer);
			free(paquete);
		}	
}

void escuchar_io()
{
	while (1) 
	{
		pthread_t thread;
		int *fd_conexion_ptr = malloc(sizeof(int));
		*fd_conexion_ptr = accept(fd_kernel, NULL, NULL);
		handshakeServer(*fd_conexion_ptr);
		printf("Se ha conectado un cliente de tipo IO\n");
		pthread_create(&thread, NULL, (void*) kernel_escuchar_entradasalida_mult, fd_conexion_ptr);
		pthread_detach(thread);
	}
}

void kernel_escuchar_memoria ()
{
	bool control_key = 1;
	while (control_key) {
			int cod_op = recibir_operacion(fd_memoria);
			switch (cod_op) {
			case MENSAJE:
				//
				break;
			case PAQUETE:
				//
				break;
			case -1:
				log_error(kernel_logger, "Desconexion de memoria.");
				control_key = 0;
			default:
				log_warning(kernel_logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
		}	
}

void iniciar_proceso(char* path)
{
	PCB* pcb = malloc(sizeof(PCB));
	if ( pcb == NULL )
	{
		printf("Error al crear pcb");
	}

	//inicializo el PCB del proceso
	pid++;
	pcb->PID = pid;
	pcb->PC = 0;
	pcb->quantum = 0;
	pcb->registro.AX = 0;//cambios
	pcb->registro.BX = 0;//cambios
	pcb->registro.CX = 0;//cambios
	pcb->registro.DX = 0;//cambios
	pcb->registro.EAX = 0;//cambios
	pcb->registro.EBX = 0;//cambios
	pcb->registro.ECX = 0;//cambios
	pcb->registro.EDX = 0;//cambios
	pcb->registro.DI = 0;//cambios
	pcb->registro.SI = 0;//cambios
	pcb->estado = NEW;
	pcb->path = string_duplicate(path); //consejo

	//agrego el pcb a la cola new
	
	sem_wait(&sem);
	queue_push(cola_new,pcb);	
    // Señalar (incrementar) el semáforo
    sem_post(&sem);
	sem_post(&sem_cant);

	//procesos_en_new++;
	
	log_info (kernel_logs_obligatorios, "Se crea el proceso %d en NEW, funcion iniciar proceso\n", pcb->PID);
}

void atender_instruccion (char* leido)
{
    char** comando_consola = string_split(leido, " ");
	//printf("%s\n",comando_consola[0]);

    if((strcmp(comando_consola[0], "INICIAR_PROCESO") == 0))
	{ 
        iniciar_proceso(comando_consola[1]);  
    }else if(strcmp(comando_consola [0], "FINALIZAR_PROCESO") == 0){
    }else if(strcmp(comando_consola [0], "DETENER_PLANIFICACION") == 0){
    }else if(strcmp(comando_consola [0], "INICIAR_PLANIFICACION") == 0){
    }else if(strcmp(comando_consola [0], "MULTIPROGRAMACION") == 0){ 
    }else if(strcmp(comando_consola [0], "PROCESO_ESTADO") == 0){
    }else if(strcmp(comando_consola [0], "HELP") == 0){
    }else if(strcmp(comando_consola [0], "PRINT") == 0){
    }else{   
        log_error(kernel_logger, "Comando no reconocido, pero que paso el filtro ???");  
        exit(EXIT_FAILURE);
    }
    string_array_destroy(comando_consola); 
}

void validarFuncionesConsola(char* leido)
{
	 char** valorLeido = string_split(leido, " ");
	 //printf("%s\n",valorLeido[0]);

	 if(strcmp(valorLeido[0], "EJECUTAR_SCRIPT") == 0)
	 {
		printf("Comando válido\n");
	 }
	 else
	 {
		if(strcmp(valorLeido[0], "INICIAR_PROCESO") == 0)
	    {
		    printf("Comando válido\n");
			atender_instruccion(leido);
	    }
		else
		{
			if(strcmp(valorLeido[0], "FINALIZAR_PROCESO") == 0)
	        {  
		         printf("Comando válido\n");
	        }
			else
			{
				if(strcmp(valorLeido[0], "DETENER_PLANIFICACION") == 0)
	            {  
		             printf("Comando válido\n");
	            }
				else
				{
					if(strcmp(valorLeido[0], "INICIAR_PLANIFICACION") == 0)
	                {  
		                printf("Comando válido\n");
	                }
					else
					{
						if(strcmp(valorLeido[0], "MULTIPROGRAMACION") == 0)
	                    {  
		                       printf("Comando válido\n");
	                    }
						else
						{
							if(strcmp(valorLeido[0], "PROCESO_ESTADO") == 0)
	                        {  
		                       printf("Comando válido\n");
	                        }
						}
					}
				}
			}
		}
	 }
	 string_array_destroy(valorLeido);
}

void consolaInteractiva()
{
	char* leido;
	leido = readline("> ");
	
	while( strcmp(leido,"") != 0 )
	{
		validarFuncionesConsola(leido);
		free(leido);
		leido = readline("> ");
	}
}

void enviarProcesoMemoria (ProcesoMemoria* proceso, int socket_servidor)
{
    //Creamos un Buffer
    t_newBuffer* buffer = malloc(sizeof(t_newBuffer));

    //Calculamos su tamaño
    buffer->size = sizeof(uint32_t)*2 + proceso->path_length +1;//MODIFCADO
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
	
    //Movemos los valores al buffer
    memcpy(buffer->stream + buffer->offset, &proceso->PID, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);

    // Para el nombre primero mandamos el tamaño y luego el texto en sí:
    memcpy(buffer->stream + buffer->offset, &proceso->path_length, sizeof(uint32_t));
    buffer->offset += sizeof(uint32_t);
    memcpy(buffer->stream + buffer->offset, proceso->path, proceso->path_length);
    
	//Creamos un Paquete
    t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
    //Podemos usar una constante por operación
    paquete->codigo_operacion = PAQUETE;
    paquete->buffer = buffer;

    //Empaquetamos el Buffer
    void* a_enviar = malloc(buffer->size + sizeof(op_code) + sizeof(uint32_t));
    int offset = 0;
    memcpy(a_enviar + offset, &(paquete->codigo_operacion), sizeof(op_code));
    offset += sizeof(op_code);
    memcpy(a_enviar + offset, &(paquete->buffer->size), sizeof(uint32_t));
    offset += sizeof(uint32_t);
    memcpy(a_enviar + offset, paquete->buffer->stream, paquete->buffer->size);
    //Por último enviamos
    send(socket_servidor, a_enviar, buffer->size + sizeof(op_code) + sizeof(uint32_t), 0);

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void informar_memoria_nuevo_procesoNEW()
{
	//CREAMOS BUFFER
	//POR EL MOMENTO ESTAMOS HARCODEANDO para intentar mandar algo a memoria
	ProcesoMemoria* proceso = malloc(sizeof(ProcesoMemoria));

	PCB* pcb = queue_pop(cola_new);//
	proceso->path = malloc(strlen(pcb->path)+1);
	//proceso->path = pcb->path;
	strcpy(proceso->path, pcb->path);
	proceso->PID = pcb->PID;
	proceso->path_length = strlen(pcb->path)+1;
	proceso->TablaDePaginas = list_create();
	
	enviarProcesoMemoria(proceso,fd_memoria);
	
	sem_wait(&sem_ready);   // mutex hace wait
	queue_push(cola_ready,pcb);	//agrega el proceso a la cola de ready
    sem_post(&sem_ready); 
	sem_post(&sem_cant_ready);  // mutex hace wait
	
}

void mover_procesos_ready(int grado_multiprogramacion)
{
	//CONSULTA. TENDREMOS QUE USAR SEMAFOROS?
    //Cantidad de procesos en las colas de NEW y READY
    int cantidad_procesos_new = queue_size(cola_new);
    int cantidad_procesos_ready = queue_size(cola_ready);

    //El grado de multiprogramación lo permite?
    if (cantidad_procesos_ready < grado_multiprogramacion)
    {
        // Mover procesos de la cola de NEW a la cola de READY
        while (cantidad_procesos_new > 0 && cantidad_procesos_ready < grado_multiprogramacion)
        {
            // Seleccionar el primer proceso de la cola de NEW y los borramos de la cola
            PCB* proceso_nuevo = queue_pop(cola_new);
           // queue_pop(cola_new);

            // Cambiar el estado del proceso a READY
            proceso_nuevo->estado = READY;

            // Agregar el proceso a la cola de READY
		

			queue_push(cola_ready, proceso_nuevo);
            cantidad_procesos_ready++;
			
		
           

            // Reducir la cantidad de procesos en la cola de NEW
            cantidad_procesos_new--;
        }
    }
    else
    {
        printf("El grado de multiprogramación máximo ha sido alcanzado. %d procesos permanecerán en la cola de NEW.\n",cantidad_procesos_new);
    }
}

void planificador_largo_plazo()
{
    // //Obtenemos el grado de multiprogramación especificado por el archivo de configuración
    // //int grado_multiprogramacion = config_get_int_value(kernel_config, "GRADO_MULTIPROGRAMACION");
	// int grado_multiprogramacion = 20;
	// //pregunto constantemente
	// while(1)
	// {
	// 	//hay nuevos procesos en new?
	// 	if( procesos_en_new > 0 )
	// 	{
	// 		//avisar a memoria
	// 		//USAR QUEUE Pop
	while(1)
	{
		//SEMAFOROS (PRODUCTOR-CONSUMIDOR)  consumidor de la cola de NEW

		//if( queue_size(cola_new) > 0)
		//{	
		    sem_wait(&sem_cant);   // mutex hace wait
		    sem_wait(&sem);   // mutex hace wait

			informar_memoria_nuevo_procesoNEW();
			
			sem_post(&sem);   // mutex hace signal

			//sem_post(&sem_cant_ready); //avisamos al planificador que hay un nuevo proceso listo
			
		//}

	}			
	// 	}
	// 	else
	// 	{
	// 		//algun proceso terminó?
	// 		if(procesos_fin > 0 )
	// 		{
	// 			//
	// 			printf("esta parte es la del fin del proceso");
	// 		}
	// 	}
	// 	//luego de esto muevo los procesos a ready mientras pueda
	// 	//mover_procesos_ready(grado_multiprogramacion);
	// 	sleep(1);
	//}	
}

void enviar_pcb_a_cpu()
{
	sem_wait(&sem_mutex_cpu_ocupada);
	while( cpu_ocupada != false )
	{
		sem_post(&sem_mutex_cpu_ocupada);
		sleep(1);
		sem_wait(&sem_mutex_cpu_ocupada);
	}
	sem_post(&sem_mutex_cpu_ocupada);
	//Reservo memoria para enviarla
	PCB* to_send = malloc(sizeof(PCB));
	

	sem_wait(&sem_cant_ready);   // mutex hace wait
	sem_wait(&sem_ready);   // mutex hace wait

	PCB* pcb_cola = queue_pop(cola_ready); //saca el proceso de la cola de ready
			
	sem_post(&sem_ready); // mutex hace signal
	

	to_send->PID = pcb_cola->PID;
	to_send->PC = pcb_cola->PC;
	to_send->quantum = pcb_cola->quantum;
	to_send->estado = EXEC;
	to_send->registro = pcb_cola->registro;//cambios
	/*to_send->registro.BX = pcb_cola->registro.BX;//cambios
	to_send->registro.CX = pcb_cola->registro.CX;//cambios
	to_send->registro.DX = pcb_cola->registro.DX;//cambios*/

	to_send->path = string_duplicate( pcb_cola->path);

	printf("Enviaremos un proceso\n");
	printf("Enviaremos el proceso cuyo pid es %d\n",to_send->PID);
	enviarPCB(to_send, fd_cpu_dispatch,PROCESO);
	sem_wait(&sem_mutex_cpu_ocupada);
	cpu_ocupada = true;
	sem_post(&sem_mutex_cpu_ocupada);
}

void interrumpir_por_quantum()
{
	printf("Entre a dormir un poco para el quantum\n");
	int quantumAUsar = atoi(QUANTUM);
	printf("El quantum sera: %d\n",quantumAUsar);
	sleep(quantumAUsar/1000);
	//MANDAR A MEMORIA FIN DE QUANTUM POR DISPATCH
	int* enteroRandom = malloc(sizeof(int));
	*enteroRandom = 0;
 	printf("Me desperte uwu\n");
	enviarEntero(enteroRandom,fd_cpu_interrupt,FIN_DE_QUANTUM);
}

void planificador_corto_plazo()
{

		if(strcmp(ALGORITMO_PLANIFICACION,"FIFO")==0)
		{
			printf("Planificare por FIFO\n");
			while(1)
			{
				//PLANIFICAR POR FIFO
				sem_wait(&sem_mutex_plani_corto);
				enviar_pcb_a_cpu();
				sem_post(&sem_mutex_plani_corto);
			}
		}
		
		if(strcmp(ALGORITMO_PLANIFICACION,"RR")==0)
		{
			printf("Planificare por RR\n");
			//PLANIFICAR POR RR
			while(1)
			{
				sem_wait(&sem_mutex_plani_corto);
				enviar_pcb_a_cpu();
				sem_post(&sem_mutex_plani_corto);
				interrumpir_por_quantum();
			}
		}
		if(strcmp(ALGORITMO_PLANIFICACION,"VRR")==0)
		{

			//PLANIFICAR POR VRR
		}

}
	

#endif /* KERNEL_H_ */
