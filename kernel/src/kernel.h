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
int grado_multiprogramacion;
bool iniciar_planificacion;
bool iniciar_planificacion = true;

//colas y pid
int procesos_en_new = 0;
int procesos_fin = 0;

sem_t sem; //semaforo mutex region critica cola new
sem_t sem_ready; 
sem_t sem_cant; //semaforo cant de elementos en la cola
sem_t sem_cant_ready;
sem_t sem_ready_prio;
sem_t sem_mutex_plani_corto; //semaforo oara planificacion FIFO
sem_t sem_mutex_cpu_ocupada; //semaforo para indicar si la cpu esta ocupada
sem_t sem_blocked;
sem_t sem_procesos;
sem_t sem_grado_mult;


//semaforos para garantizar exclusión mutua al acceso de las listas de ios
sem_t sem_mutex_lists_io; //un unico semaforo para el acceso a la listas de ios

//lista de recursos
t_list* listRecursos;

//TODAS LAS QUEUES
int pid = 0;
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_blocked;
t_queue* cola_ready_prioridad;

//LISTA DE PROCESOS
t_list* lista_procesos;

//LISTA DE ENTRADAS Y SALIDAS
t_list* listGenericas;
t_list* listStdin;
t_list* listStdout;
t_list* listDialfs;


//CONTADOR PARA EL VRR
t_temporal* cronometro;

//su semaforo
sem_t sem_mutex_cronometro;

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

void ejecutar_script (char* path);

PCB* encontrarProceso(t_list* lista, uint32_t pid)
{
	PCB* ret;
	int i = 0;
	while( i < list_size(lista) )
	{
		PCB* got = list_get(lista,i);
		if( got->PID == pid)
		{
			ret = got;
		}
		i++;
	}
	return ret;
}

/*
char* obtener_cadena_pids(t_list* lista)
{
    // Calculamos la longitud total de la cadena
    size_t longitud_total = 2; // Incluye '[' y ']'
    for (size_t i = 0; i < list_size(lista); ++i)
    {
        PCB* pcb = list_get(lista, i);
        // Suponemos que el PID es un número de un solo dígito
        longitud_total += 1; // Espacio para el PID
        if (i < list_size(lista) - 1)
            longitud_total += 2; // Espacio para la coma y el espacio
    }

    // Reservamos memoria para la cadena
    char* cadena_pids = (char*)malloc(longitud_total);
    if (!cadena_pids)
    {
        perror("Error al reservar memoria");
        exit(EXIT_FAILURE);
    }

    // Construimos la cadena
    sprintf(cadena_pids, "[");
    for (size_t i = 0; i < list_size(lista); ++i)
    {
        PCB* pcb = list_get(lista, i);
        sprintf(cadena_pids + strlen(cadena_pids), "%u", pcb->PID);
        if (i < list_size(lista) - 1)
            strcat(cadena_pids, ", ");
    }
    strcat(cadena_pids, "]");

    return cadena_pids;
}
*/

char* obtener_cadena_pids(t_list* lista)
{
    // Calculamos la longitud total de la cadena
    size_t longitud_total = 2; // Incluye '[' y ']'
    for (size_t i = 0; i < list_size(lista); ++i)
    {
        PCB* pcb = list_get(lista, i);
        char buffer[12]; // Suficiente para almacenar un uint32_t con signo
        int pid_len = snprintf(buffer, sizeof(buffer), "%u", pcb->PID);
        longitud_total += pid_len;
        if (i < list_size(lista) - 1)
            longitud_total += 2; // Espacio para la coma y el espacio
    }

    // Reservamos memoria para la cadena
    char* cadena_pids = (char*)malloc(longitud_total + 1); // +1 para el '\0'
    if (!cadena_pids)
    {
        perror("Error al reservar memoria");
        exit(EXIT_FAILURE);
    }

    // Construimos la cadena
    strcpy(cadena_pids, "[");
    for (size_t i = 0; i < list_size(lista); ++i)
    {
        PCB* pcb = list_get(lista, i);
        char buffer[12];
        snprintf(buffer, sizeof(buffer), "%u", pcb->PID);
        strcat(cadena_pids, buffer);
        if (i < list_size(lista) - 1)
            strcat(cadena_pids, ", ");
    }
    strcat(cadena_pids, "]");

    return cadena_pids;
}

/*
char* estado_proceso_to_string(estado_proceso estado) {
	char* to_ret = malloc(50);
    switch (estado) {
        case NEW: return "NEW";
        case READY: return "READY";
        case BLOCKED: return "BLOCKED";
        case EXEC: return "EXEC";
        case EXIT: return "EXIT";
        default: return "UNKNOWN";
    }
}
*/
char* estado_proceso_to_string(estado_proceso estado) {
    char* to_ret = malloc(50);
    if (!to_ret) {
        perror("Error al asignar memoria");
        exit(EXIT_FAILURE);
    }

    switch (estado) {
        case NEW: strcpy(to_ret, "NEW"); break;
        case READY: strcpy(to_ret, "READY"); break;
        case BLOCKED: strcpy(to_ret, "BLOCKED"); break;
        case EXEC: strcpy(to_ret, "EXEC"); break;
        case EXIT: strcpy(to_ret, "EXIT"); break;
        default: strcpy(to_ret, "UNKNOWN"); break;
    }

    return to_ret;
}


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

//Estas dos funciones a continuación son para la administración de recursos. Se busca eliminar los pid que tienen instancias del recurso en el momento.
bool es_elemento_buscado(void* elemento, void* valor_buscado) {
    return *((uint32_t*)elemento) == *((uint32_t*)valor_buscado);
}

bool eliminar_elemento_pid(t_list* lista, uint32_t valor) 
{
    uint32_t* valor_buscado = malloc(sizeof(uint32_t));
    *valor_buscado = valor;
    bool to_ret = false;

    // Iterar sobre la lista para encontrar el elemento
    for (int i = 0; i < list_size(lista); i++) {
        uint32_t* elemento = list_get(lista, i);
        if (es_elemento_buscado(elemento, valor_buscado)) {
            list_remove_and_destroy_element(lista, i, free);
            to_ret = true;
            break;  // Elemento encontrado y eliminado, salimos del bucle
        }
    }

    free(valor_buscado);  // Liberamos valor_buscado siempre, ya que ya no lo necesitamos
    return to_ret;
}


bool eliminar_elemento(t_list* lista, uint32_t pid_buscado) 
{
    bool to_ret = false;  // Inicializar el retorno en false
    for (int i = 0; i < list_size(lista); i++) {
        PCB* elemento = list_get(lista, i);
        if (elemento->PID == pid_buscado) {  // Comparar el PID
            list_remove(lista, i);  // Eliminar el elemento de la lista
            // Liberar la memoria del elemento si es necesario
            //free(elemento->path);  // Liberar el campo path, si es dinámico
            free(elemento);  // Liberar el PCB en sí
            to_ret = true;
            break;  // Salir del bucle después de eliminar el elemento
        }
    }
    return to_ret;
}

//Función para generar los recursos a partir del archivo de configuración
t_list* generarRecursos(char* recursos, char* instancias_recursos)
{
	//creamos la lista a retornar
	t_list* ret = list_create();
	//sacamos los corchetes
	removerCorchetes(recursos);
	removerCorchetes(instancias_recursos);
	//los separamos por las comas
	char** recursos_separados = string_split(recursos,",");   //
	char** instancias_recursos_separados = string_split(instancias_recursos,","); //
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
	string_array_destroy(recursos_separados);
	string_array_destroy(instancias_recursos_separados);
	return ret;
}

//Se ejecuta la interfaz genérica
void ejecutar_interfaz_generica(char* instruccion, op_code tipoDeInterfaz, int fd_io, int pid_actual)
{
	char** instruccion_split = string_split(instruccion," ");
	int* unidadesDeTrabajo = malloc(sizeof(int));
	*unidadesDeTrabajo = atoi(instruccion_split[2]);

	int* pid_io = malloc(sizeof(int));
	*pid_io = pid_actual;

	enviarEntero(pid_io,fd_io,NUEVOPID);
	enviarEntero(unidadesDeTrabajo,fd_io,GENERICA);

	string_array_destroy(instruccion_split);
	free(unidadesDeTrabajo);
	free(pid_io);
}


void new_ejecutar_interfaz_stdin_stdout(char* instruccion, op_code tipoDeInterfaz, int fd_io, int pid_actual)
{
	//debemos enviar la instrucción entero + su tamaño
	char* to_send = string_duplicate(instruccion);
	int* tamanio = malloc(sizeof(int));
	*tamanio = strlen(to_send)+1;

	//antes que todo enviamos del pid del proceso que actualmente lo usa
	int* pid = malloc(sizeof(int));
	*pid = pid_actual;
	enviarEntero(pid,fd_io,NUEVOPID);
	
	t_newBuffer* buffer = malloc(sizeof(t_newBuffer));

    //Calculamos su tamaño
	buffer->size = sizeof(int) + *tamanio;
    buffer->offset = 0;
    buffer->stream = malloc(buffer->size);
	
    //Movemos los valores al buffer
    memcpy(buffer->stream + buffer->offset,tamanio, sizeof(int));
    buffer->offset += sizeof(int);
	memcpy(buffer->stream + buffer->offset,to_send, *tamanio);

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
	//printf("Lo mande\n");
	
    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
	free(to_send);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
	free(tamanio);
	free(pid);
}

//Se buscan las IO dentro de la lista
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

//Busca una PCB en base a su pid en una lista. La usamos para eliminar procesos de la cola de bloqueados.
PCB* encontrar_por_pid(t_list* lista, uint32_t pid_buscado) {
    for (int i = 0; i < list_size(lista); i++) 
	{
        PCB* pcb = list_get(lista, i);
		//printf("El PCB obtenido fue: %d\n",pcb->PID);
		//printf("Su estado actual es %d\n",pcb->estado);
        if (pcb->PID == pid_buscado) 
		{
			pcb = list_remove(lista, i);
            return pcb;
        }
    }
    return NULL; // Si no se encuentra el elemento
}

void encolar_procesos_vrr(PCB* proceso)
{
	if( proceso->quantum > 0 )
	{
		printf("Encontramos el siguiente quantum: %d\n",proceso->quantum);
		sem_wait(&sem_ready_prio);
		queue_push(cola_ready_prioridad,proceso);
		printf("Estoy luego del semaforo %d\n",proceso->quantum);		

		char* cadena_pids_prioridad = obtener_cadena_pids(cola_ready_prioridad->elements);
		log_info (kernel_logs_obligatorios, "Ready Prioridad: %s\n", cadena_pids_prioridad);
		free(cadena_pids_prioridad);
		char* cadena_pids = obtener_cadena_pids(cola_ready->elements);
		log_info (kernel_logs_obligatorios, "Cola Ready: %s\n", cadena_pids);
		free(cadena_pids);

		sem_post(&sem_ready_prio);
	}
	else
	{
		//reiniciamos el quantum
		proceso->quantum = atoi(QUANTUM);
		sem_wait(&sem_ready);   // mutex hace wait
		queue_push(cola_ready, proceso); // agrega el proceso a la cola de ready

		char* cadena_pids = obtener_cadena_pids(cola_ready->elements);
		log_info (kernel_logs_obligatorios, "Cola Ready: %s\n", cadena_pids);
		free(cadena_pids);

		sem_post(&sem_ready);
		sem_post(&sem_cant_ready);  // mutex hace wait
	}
}

//Se liberan los recursos una vez un proceso ha sido eliminado
void liberar_recursos(int pid)
{
	for(int i = 0; i < list_size(listRecursos); i++)
	{
		Recurso* r = list_get(listRecursos,i);
		if( eliminar_elemento(r->listBloqueados,pid) )
		{
			r->instancias++;
		}
		bool borrado = true;
		while( borrado )
		{
			borrado = eliminar_elemento_pid(r->pid_procesos,pid);
			if( borrado )
			{
				r->instancias++;
				if (r->instancias <= 0) 
				{
					//PCB* to_ready = malloc(sizeof(PCB));
					PCB* proceso_desbloqueado = list_remove(r->listBloqueados, 0);
					//*to_ready = *proceso_desbloqueado;

					sem_wait(&sem_blocked);
					eliminar_elemento(cola_blocked->elements,proceso_desbloqueado->PID);
					sem_post(&sem_blocked);
					if (proceso_desbloqueado != NULL) 
					{
						//añadimos que dicho proceso está usando una instancia
						uint32_t* pid = malloc(sizeof(uint32_t));
						*pid = proceso_desbloqueado->PID;
						list_add(r->pid_procesos,pid);

						//añadimos a una cola dependiendo si le queda quantum o no
						// Enviamos el proceso a la cola de ready

						char* estado_anterior_proceso_desbloqueado = estado_proceso_to_string(proceso_desbloqueado->estado);
						sem_wait(&sem_procesos);
						PCB* actualizado_fin = encontrarProceso(lista_procesos,proceso_desbloqueado->PID);
						actualizado_fin->estado = READY;
						char* estado_actual_proceso_desbloqueado = estado_proceso_to_string(actualizado_fin->estado);
						sem_post(&sem_procesos);

						log_info (kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", proceso_desbloqueado->PID, estado_anterior_proceso_desbloqueado, estado_actual_proceso_desbloqueado);	
						free(estado_anterior_proceso_desbloqueado);
						free(estado_actual_proceso_desbloqueado);

						if( strcmp(ALGORITMO_PLANIFICACION,"VRR") == 0 )
						{
							encolar_procesos_vrr(proceso_desbloqueado);
						}
						else
						{

							sem_wait(&sem_ready);   // mutex hace wait
							//printf("En este momento en la cola de ready se ha pusheado al pid: %d\n", proceso_desbloqueado->PID);
							queue_push(cola_ready, proceso_desbloqueado); // agrega el proceso a la cola de ready


							char* cadena_pids = obtener_cadena_pids(cola_ready->elements);
							log_info (kernel_logs_obligatorios, "Cola Ready: %s\n", cadena_pids);
							free(cadena_pids);

							sem_post(&sem_ready);
							sem_post(&sem_cant_ready);  // mutex hace wait
						}
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
				char* estado_anterior = estado_proceso_to_string(proceso->estado);
				proceso->estado = EXIT;
				char* estado_actual = estado_proceso_to_string(proceso->estado);
				
				printf("/////////////-----[EL PROCESO DE PID %d ha FINALIZADO]-----////////////\n",proceso->PID);
				log_info (kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", proceso->PID, estado_anterior, estado_actual);
				log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <SUCCESS>\n",proceso->PID);
			
				free(estado_anterior);
				free(estado_actual);

				//ACTUALIZAMOS EN LA LISTA GENERAL
				sem_wait(&sem_procesos);
				PCB* actualizado_fin = encontrarProceso(lista_procesos,proceso->PID);
				actualizado_fin->estado = EXIT;
				sem_post(&sem_procesos);

				liberar_recursos(proceso->PID);
				enviarPCB(proceso,fd_memoria,PROCESOFIN);

				//free(proceso->path);
				free(proceso);
				
				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);

                break;
			case OUT_OF_MEMORY:
				PCB* proceso_out_of_memory = deserializar_proceso_cpu(paquete->buffer);
				char* estado_anterior_out_of_memory = estado_proceso_to_string(proceso_out_of_memory->estado);
				proceso_out_of_memory->estado = EXIT;
				char* estado_actual_out_of_memory = estado_proceso_to_string(proceso_out_of_memory->estado);
			
				printf("/////////////-----[EL PROCESO DE PID %d ha FINALIZADO]-----////////////\n", proceso_out_of_memory->PID);
				log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_out_of_memory->PID, estado_anterior_out_of_memory, estado_actual_out_of_memory);
				log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <OUT_OF_MEMORT>\n",proceso_out_of_memory->PID);

				free(estado_anterior_out_of_memory);
				free(estado_actual_out_of_memory);

				// ACTUALIZAMOS EN LA LISTA GENERAL
				sem_wait(&sem_procesos);
				PCB* actualizado_fin_out_of_memory = encontrarProceso(lista_procesos, proceso_out_of_memory->PID);
				actualizado_fin_out_of_memory->estado = EXIT;
				sem_post(&sem_procesos);

				liberar_recursos(proceso_out_of_memory->PID);
				enviarPCB(proceso_out_of_memory, fd_memoria, PROCESOFIN);

				//free(proceso_out_of_memory->path);
				free(proceso_out_of_memory);

				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);

			break;
			case INTERRUPTED_BY_USER:
				PCB* proceso_interrupted_by_user = deserializar_proceso_cpu(paquete->buffer);
				char* estado_anterior_interrupted_by_user = estado_proceso_to_string(proceso_interrupted_by_user->estado);
				proceso_interrupted_by_user->estado = EXIT;
				char* estado_actual_interrupted_by_user = estado_proceso_to_string(proceso_interrupted_by_user->estado);
		
				printf("/////////////-----[EL PROCESO DE PID %d ha FINALIZADO]-----////////////\n", proceso_interrupted_by_user->PID);
				log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_interrupted_by_user->PID, estado_anterior_interrupted_by_user, estado_actual_interrupted_by_user);
				log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <INTERRUPTED_BY_USER>\n", proceso_interrupted_by_user->PID);
				
				free(estado_anterior_interrupted_by_user);
				free(estado_actual_interrupted_by_user);

				// ACTUALIZAMOS EN LA LISTA GENERAL
				sem_wait(&sem_procesos);
				PCB* actualizado_fin_interrupted_by_user = encontrarProceso(lista_procesos, proceso_interrupted_by_user->PID);
				actualizado_fin_interrupted_by_user->estado = EXIT;
				sem_post(&sem_procesos);

				liberar_recursos(proceso_interrupted_by_user->PID);
				enviarPCB(proceso_interrupted_by_user, fd_memoria, PROCESOFIN);

				free(proceso_interrupted_by_user->path);
				free(proceso_interrupted_by_user);

				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);

				
			break;
			case PROCESOIO:

				PCB* proceso_io = deserializar_proceso_cpu(paquete->buffer);
				char* estado_anterior_io = estado_proceso_to_string(proceso_io->estado);
				proceso_io->estado = BLOCKED;
				char* estado_actual_io = estado_proceso_to_string(proceso_io->estado);

				
				log_info(kernel_logs_obligatorios,"PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n",proceso_io->PID,estado_anterior_io,estado_actual_io);

				free(estado_anterior_io);
				free(estado_actual_io);


				//ACTUALIZAMOS EN LA LISTA GENERAL
				sem_wait(&sem_procesos);
				PCB* actualizado_io = encontrarProceso(lista_procesos,proceso_io->PID);
				actualizado_io->estado = BLOCKED;
				sem_post(&sem_procesos);

				printf("El quantum regresado es %d\n",proceso_io->quantum);
				//PARA VIRTUAL ROUND ROBIN
				//En caso de darse esto, debemos actualizar los valores del quantum
				/*
				if( strcmp(ALGORITMO_PLANIFICACION,"VRR") == 0)
				{
					sem_wait(&sem_mutex_cronometro);
					int64_t quantum_transcurrido= temporal_gettime(cronometro);
					//printf("La cantidad de tiempo que ha transcurrido: %d\n",quantum_transcurrido);
					sem_post(&sem_mutex_cronometro);
					//temporal_destroy(cronometro);
					proceso_io->quantum -= quantum_transcurrido;
					printf("[ACTUALIZACIÓN] Hemos actualizado el quantum y ha quedado de la siguiente forma: %d\n",proceso_io->quantum);
				}
				*/

				if (strcmp(ALGORITMO_PLANIFICACION, "VRR") == 0) 
				{
					sem_wait(&sem_mutex_cronometro);
					uint64_t quantum_transcurrido = temporal_gettime(cronometro);
					sem_post(&sem_mutex_cronometro);

					int quantum_restado;
					if (quantum_transcurrido > INT_MAX) {
						quantum_restado = -1;
						proceso_io->quantum = quantum_restado;
					} else {
						quantum_restado = (int)quantum_transcurrido;
						proceso_io->quantum -= quantum_restado;
					}
					printf("[ACTUALIZACIÓN] Hemos actualizado el quantum y ha quedado de la siguiente forma: %d\n", proceso_io->quantum);
				}
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
				char* estado_anterior_fin_de_quantum = estado_proceso_to_string(proceso_fin_de_quantum->estado);
				proceso_fin_de_quantum->estado = READY;
				char* estado_actual_fin_de_quantum = estado_proceso_to_string(proceso_fin_de_quantum->estado);
				
				sem_wait(&sem_procesos);
				PCB* actualizado_quantum = encontrarProceso(lista_procesos,proceso_fin_de_quantum->PID);
				actualizado_quantum->estado = READY;
				sem_post(&sem_procesos);

				int p = proceso_fin_de_quantum->PID;

				//printf("PID: <%d> - Desalojo por fin de Quantum",proceso_fin_de_quantum->PID);
				log_info (kernel_logs_obligatorios, "PID: <%d> - Desalojado por fin de Quantum", p);

				log_info(kernel_logs_obligatorios,"PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n",proceso_fin_de_quantum->PID,estado_anterior_fin_de_quantum,estado_actual_fin_de_quantum);

				free(estado_anterior_fin_de_quantum);
				free(estado_actual_fin_de_quantum);


				//el quantum vuelve al valor original
				proceso_fin_de_quantum->quantum = atoi(QUANTUM);
				//printf("Hemos reiniciado el quantum: %d\n",proceso_fin_de_quantum->quantum);

				//lo ideal seria tambien agregarlo a una cola de la interfaz de cada proceso
				sem_wait(&sem_ready);   // mutex hace wait
				queue_push(cola_ready,proceso_fin_de_quantum);	//agrega el proceso a la cola de ready

				char* cadena_pids = obtener_cadena_pids(cola_ready->elements);
				log_info (kernel_logs_obligatorios, "Cola Ready: %s\n", cadena_pids);
				free(cadena_pids);

  				sem_post(&sem_ready); 
				sem_post(&sem_cant_ready);  // mutex hace wait
				
				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);	
				break;
			case GENERICA:
				
				//Deserializamos
				Instruccion_io* instruccion_io_gen = deserializar_instruccion_io(paquete->buffer);
				
				//Debemos obtener el IO específico de la lista
				sem_wait(&sem_mutex_lists_io);
				char** io_gen_split = string_split(instruccion_io_gen->instruccion," ");
				EntradaSalida* io_gen = encontrar_io(listGenericas,io_gen_split[1]);
				string_array_destroy(io_gen_split);
				sem_post(&sem_mutex_lists_io);

                char** instruccion_partida_gen = string_split(instruccion_io_gen->instruccion," ");
				log_info(kernel_logs_obligatorios,"PID: <%d> - Bloqueado por: <%s>\n",instruccion_io_gen->proceso.PID,instruccion_partida_gen[1]);
				string_array_destroy(instruccion_partida_gen);


				//verificamos que exista
				if( io_gen != NULL )
				{
					
					if( io_gen->ocupado )
					{
						list_add(io_gen->procesos_bloqueados, instruccion_io_gen);
					}
					else
					{
						//marcamos la io como ocupada y ejecutamos
						io_gen->ocupado = true;
						ejecutar_interfaz_generica(instruccion_io_gen->instruccion,GENERICA,io_gen->fd_cliente,instruccion_io_gen->proceso.PID);
						free(instruccion_io_gen->instruccion);
						free(instruccion_io_gen);
					}
				}
				else
				{
					//de no existir debemos terminar el proceso	
					sem_wait(&sem_blocked);				
					PCB* proceso_to_end = encontrar_por_pid(cola_blocked->elements,instruccion_io_gen->proceso.PID);
					sem_post(&sem_blocked);	

					char* estado_anterior_proceso_to_end = estado_proceso_to_string(proceso_to_end->estado);

					sem_wait(&sem_procesos);
					PCB* actualizado_gen = encontrarProceso(lista_procesos,instruccion_io_gen->proceso.PID);
					actualizado_gen->estado = EXIT;
					sem_post(&sem_procesos);

					char* estado_actual_proceso_to_end = estado_proceso_to_string(actualizado_gen->estado);

					log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_to_end->PID, estado_anterior_proceso_to_end, estado_actual_proceso_to_end);
					log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <INVALID_INTERFACE>\n", proceso_to_end->PID);
					
					free(estado_anterior_proceso_to_end);
					free(estado_actual_proceso_to_end);


					//printf("Este proceso ha terminado\n");
					liberar_recursos(proceso_to_end->PID);
					enviarPCB(proceso_to_end,fd_memoria,PROCESOFIN);
					//free(proceso_to_end->path);
					free(proceso_to_end);
					free(instruccion_io_gen->instruccion);
					free(instruccion_io_gen);
				}
					//CPU desocupada
					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = false;
					sem_post(&sem_mutex_cpu_ocupada);
				break;
			case STDIN:
				//Deserializamos
				Instruccion_io* instruccion_io_stdin = deserializar_instruccion_io(paquete->buffer);

				//debemos obtener el io específico de la lista
				sem_wait(&sem_mutex_lists_io);
				char** io_stdin_split = string_split(instruccion_io_stdin->instruccion," ");
				EntradaSalida* io_stdin = encontrar_io(listStdin,io_stdin_split[1]);
				string_array_destroy(io_stdin_split);
				sem_post(&sem_mutex_lists_io);

				char** instruccion_partida_stdin = string_split(instruccion_io_stdin->instruccion," ");
				log_info(kernel_logs_obligatorios,"PID: <%d> - Bloqueado por: <%s>\n",instruccion_io_stdin->proceso.PID,instruccion_partida_stdin[1]);
				string_array_destroy(instruccion_partida_stdin);

				//verificamos que exista
				if( io_stdin != NULL)
				{
					//printf("Entré acá first\n");
					//Una vez encontrada la io, vemos si está ocupado
					if( io_stdin->ocupado )
					{
						//de estar ocupado añado el proceso a la lista de este
						//printf("La IO está ocupada, se bloqueará en su lista propia\n");
						Instruccion_io* to_block = malloc(sizeof(Instruccion_io));
						to_block->proceso = instruccion_io_stdin->proceso;
						to_block->tam_instruccion = instruccion_io_stdin->tam_instruccion;
						to_block->instruccion = malloc(to_block->tam_instruccion);
						strcpy(to_block->instruccion,instruccion_io_stdin->instruccion);
						list_add(io_stdin->procesos_bloqueados,to_block);
					}
					else
					{
						io_stdin->ocupado = true;
						//printf("El fd de este IO es %d\n",io_stdin->fd_cliente);
						new_ejecutar_interfaz_stdin_stdout(instruccion_io_stdin->instruccion,STDIN,io_stdin->fd_cliente,instruccion_io_stdin->proceso.PID);
					}
				}
				else
				{
					//de no existir debemos terminar el proceso
					sem_wait(&sem_blocked);				
					PCB* proceso_to_end = encontrar_por_pid(cola_blocked->elements,instruccion_io_stdin->proceso.PID);
					sem_post(&sem_blocked);	

					char* estado_anterior_proceso_to_end = estado_proceso_to_string(proceso_to_end->estado);

					//printf("1Another checkpoint\n");
					sem_wait(&sem_procesos);
					PCB* actualizado_in = encontrarProceso(lista_procesos,instruccion_io_stdin->proceso.PID);
					actualizado_in->estado = EXIT;
					char* estado_actual_proceso_to_end = estado_proceso_to_string(actualizado_in->estado);
					sem_post(&sem_procesos);
					//printf("2Another checkpoint\n");

					log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_to_end->PID, estado_anterior_proceso_to_end, estado_actual_proceso_to_end);
					log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <INVALID_INTERFACE>\n", proceso_to_end->PID);
					
					free(estado_anterior_proceso_to_end);
					free(estado_actual_proceso_to_end);


					//printf("Este proceso ha terminado\n");
					liberar_recursos(proceso_to_end->PID);
					enviarPCB(proceso_to_end,fd_memoria,PROCESOFIN);
					//free(proceso_to_end->path);
					free(proceso_to_end);

				}

				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);
				free(instruccion_io_stdin->instruccion);
				free(instruccion_io_stdin); //ESTE FREE ES PELIGROSÍSIMO, ESTAMOS BORRANDO UN DATO DE LA LISTA DE BLOQUEADOS DEL PROCESO
				break;
			case STDOUT:
				//deserializamos
				Instruccion_io* instruccion_io_stdout = deserializar_instruccion_io(paquete->buffer);
				//printf("La instruccion es: %s\n",instruccion_io_stdout->instruccion);
				//printf("El PID del proceso es: %d\n",instruccion_io_stdout->proceso.PID);

				//debemos obtener el io específico de la lista
				sem_wait(&sem_mutex_lists_io);
				char** io_stdout_split = string_split(instruccion_io_stdout->instruccion," ");
				EntradaSalida* io_stdout = encontrar_io(listStdout,io_stdout_split[1]);
				string_array_destroy(io_stdout_split);
				sem_post(&sem_mutex_lists_io);

				char** instruccion_partida_stdout = string_split(instruccion_io_stdout->instruccion," ");
				log_info(kernel_logs_obligatorios,"PID: <%d> - Bloqueado por: <%s>\n",instruccion_io_stdout->proceso.PID,instruccion_partida_stdout[1]);
				string_array_destroy(instruccion_partida_stdout);

				//verificamos que exista
				if( io_stdout != NULL)
				{
					//existe
					//una vez encontrada la io, vemos si está ocupado
					if( io_stdout->ocupado )
					{
						//de estar ocupado añado el proceso a la lista de este
						//printf("La IO está ocupada, se bloqueará en su lista propia\n");
						Instruccion_io* to_block = malloc(sizeof(Instruccion_io));
						to_block->proceso = instruccion_io_stdout->proceso;
						to_block->tam_instruccion = instruccion_io_stdout->tam_instruccion;
						to_block->instruccion = malloc(to_block->tam_instruccion);
						strcpy(to_block->instruccion,instruccion_io_stdout->instruccion);
						list_add(io_stdout->procesos_bloqueados,instruccion_io_stdout);
					}
					else
					{
						io_stdout->ocupado = true;
						//printf("El fd de este IO es %d\n",io_stdout->fd_cliente);
						new_ejecutar_interfaz_stdin_stdout(instruccion_io_stdout->instruccion,STDOUT,io_stdout->fd_cliente,instruccion_io_stdout->proceso.PID);
					}
				}
				else
				{
					//de no existir debemos terminar el proceso
					sem_wait(&sem_blocked);				
					PCB* proceso_to_end = encontrar_por_pid(cola_blocked->elements,instruccion_io_stdout->proceso.PID);
					sem_post(&sem_blocked);

					char* estado_anterior_proceso_to_end = estado_proceso_to_string(proceso_to_end->estado);

					sem_wait(&sem_procesos);
					PCB* actualizado_out = encontrarProceso(lista_procesos,instruccion_io_stdout->proceso.PID);
					actualizado_out->estado = EXIT;
					char* estado_actual_proceso_to_end = estado_proceso_to_string(actualizado_out->estado);
					sem_post(&sem_procesos);

					log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_to_end->PID, estado_anterior_proceso_to_end, estado_actual_proceso_to_end);
					log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <INVALID_INTERFACE>\n", proceso_to_end->PID);

					free(estado_anterior_proceso_to_end);
					free(estado_actual_proceso_to_end);

					//printf("Este proceso ha terminado\n");
					liberar_recursos(proceso_to_end->PC);
					enviarPCB(proceso_to_end,fd_memoria,PROCESOFIN);
					//free(proceso_to_end->path);
					free(proceso_to_end);
				}
				sem_wait(&sem_mutex_cpu_ocupada);
				cpu_ocupada = false;
				sem_post(&sem_mutex_cpu_ocupada);
				free(instruccion_io_stdout->instruccion);
				free(instruccion_io_stdout);
				break;
			case WAIT:
				//Deserializamos el recurso
				Instruccion_io* instruccion_recurso_wait = deserializar_instruccion_io(paquete->buffer);
				//printf("La instruccion es: %s\n",instruccion_recurso_wait->instruccion);
				//printf("El PID del proceso es: %d\n",instruccion_recurso_wait->proceso.PID);
				char** instruccion_partida_wait = string_split(instruccion_recurso_wait->instruccion," ");

				//verificamos que el recurso exista
				bool existe_wait = false;
				int i_wait = 0;
				Recurso* recurso_wait;
				while( i_wait < list_size(listRecursos) && existe_wait != true )
				{
					recurso_wait = list_get(listRecursos,i_wait);
					//printf("El recurso obtenido es: %s",recurso_wait->name);
					if( strcmp(recurso_wait->name,instruccion_partida_wait[1]) == 0 )
					{
						existe_wait = true;
					}
					i_wait++;
				}

				log_info(kernel_logs_obligatorios,"PID: <%d> - Realizó un WAIT del recurso <%s>\n",instruccion_recurso_wait->proceso.PID,instruccion_partida_wait[1]);

				//si existe
				if( existe_wait )
				{
					//reducimos las instancias
					recurso_wait->instancias--;

					//si es menor a cero, se bloquea
					if( recurso_wait->instancias < 0 )
					{
						log_info(kernel_logs_obligatorios,"PID: <%d> - Bloqueado por: <%s>\n",instruccion_recurso_wait->proceso.PID,instruccion_partida_wait[1]);

						//de lo contrario lo añadimos a la cola de bloqueados del recurso y también a la cola de bloqueados general
						PCB* proceso_recurso = malloc(sizeof(PCB));
						*proceso_recurso = instruccion_recurso_wait->proceso;
						
						PCB* proceso_to_block = malloc(sizeof(PCB));
						*proceso_to_block = instruccion_recurso_wait->proceso;
						printf("Vamos a guardar a la lista de bloqueados el proceso con el siguiente pid %d\n",proceso_recurso->PID);
						list_add(recurso_wait->listBloqueados,proceso_recurso);

						char* estado_anterior_proceso_wait = estado_proceso_to_string(proceso_recurso->estado);
						
						sem_wait(&sem_blocked);
						queue_push(cola_blocked,proceso_to_block);
						sem_post(&sem_blocked);
			
						

						sem_wait(&sem_procesos);
						PCB* actualizado_wait = encontrarProceso(lista_procesos,instruccion_recurso_wait->proceso.PID);
						actualizado_wait->estado = BLOCKED;
						sem_post(&sem_procesos);

						char* estado_actual_proceso_wait = estado_proceso_to_string(actualizado_wait->estado);

						log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_recurso->PID, estado_anterior_proceso_wait, estado_actual_proceso_wait);
						log_info(kernel_logs_obligatorios, "El proceso de PID: <%d> - Ha sido bloqueado porque no hay instancias disponibles del recurso\n",actualizado_wait->PID);

						free(estado_anterior_proceso_wait);
						free(estado_actual_proceso_wait);


						//CPU desocupada
						sem_wait(&sem_mutex_cpu_ocupada);
						cpu_ocupada = false;
						sem_post(&sem_mutex_cpu_ocupada);
					}
					else
					{
						log_info(kernel_logs_obligatorios,"PID: <%d> - Tomó el recurso: <%s> con éxito.\n",instruccion_recurso_wait->proceso.PID,instruccion_partida_wait[1]);

						//SI EL PROCESO TOMÓ LA INSTANCIA, DEBE VOLVER A EJECUTARSE.
						//indicamos que hay un proceso que tomó una instancia
						uint32_t* pid = malloc(sizeof(uint32_t));
						*pid = instruccion_recurso_wait->proceso.PID;
						list_add(recurso_wait->pid_procesos, pid);
						//printf("En este momento el tamaño de la lista del recurso es de %d\n",list_size(recurso_wait->pid_procesos));

						//enviamos el proceso a la cola de ready
						PCB* proceso_recurso = malloc(sizeof(PCB));
						*proceso_recurso = instruccion_recurso_wait->proceso;

						
						enviarPCB(proceso_recurso, fd_cpu_dispatch,PROCESO);
						sem_wait(&sem_mutex_cpu_ocupada);
						cpu_ocupada = true;
						sem_post(&sem_mutex_cpu_ocupada);
						free(proceso_recurso);
					}
				}
				else
				{
					log_info(kernel_logs_obligatorios,"PID: <%d> - El recurso: <%s> no existe.\n",instruccion_recurso_wait->proceso.PID,instruccion_partida_wait[1]);
					char* estado_anterior_proceso_to_end = estado_proceso_to_string(instruccion_recurso_wait->proceso.estado);

					sem_wait(&sem_procesos);
					PCB* actualizado_wait = encontrarProceso(lista_procesos,instruccion_recurso_wait->proceso.PID);
					actualizado_wait->estado = EXIT;
					char* estado_actual_proceso_to_end = estado_proceso_to_string(actualizado_wait->estado);
					sem_post(&sem_procesos);

					

					//printf("En este momento la cola de ready consta de %d procesos\n",queue_size(cola_ready));
					PCB* proceso_recurso = malloc(sizeof(PCB));
					*proceso_recurso = instruccion_recurso_wait->proceso;
					liberar_recursos(proceso_recurso->PID);
					enviarPCB(proceso_recurso,fd_memoria,PROCESOFIN);

					log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", actualizado_wait->PID, estado_anterior_proceso_to_end, estado_actual_proceso_to_end);
					log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <INVALID_RESOURCE>\n", actualizado_wait->PID);

					free(estado_anterior_proceso_to_end);
					free(estado_actual_proceso_to_end);

					//EN ESTA PARTE, LO IDEAL SERÍA TENER UNA LISTA GLOBAL DE PROCESOS CON LA CUAL PODAMOS TENER SEGUIMIENTO TOTAL DE ELLOS.
					//De darse este caso, en dicha lista diríamos que el proceso ha finalizado.
					free(proceso_recurso);
					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = false;
					sem_post(&sem_mutex_cpu_ocupada);
				}
				string_array_destroy(instruccion_partida_wait);
				free(instruccion_recurso_wait->instruccion);
				free(instruccion_recurso_wait);
				break;
			case SIGNAL:
				// Deserializamos el recurso
				Instruccion_io* instruccion_recurso_signal = deserializar_instruccion_io(paquete->buffer);
				//printf("La instruccion es: %s\n", instruccion_recurso_signal->instruccion);
				//printf("El PID del proceso es: %d\n", instruccion_recurso_signal->proceso.PID);
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

				log_info(kernel_logs_obligatorios,"PID: <%d> - Realizó un SIGNAL del recurso: <%s>\n",instruccion_recurso_signal->proceso.PID,instruccion_partida_signal[1]);

				// Si existe
				if (existe_signal) 
				{
					// Aumentamos las instancias
					recurso_signal->instancias++;

					// Si hay procesos bloqueados, los desbloqueamos
					if (recurso_signal->instancias <= 0) 
					{

						PCB* proceso_desbloqueado = list_remove(recurso_signal->listBloqueados, 0);
						sem_wait(&sem_blocked);
						eliminar_elemento(cola_blocked->elements,proceso_desbloqueado->PID);
						sem_post(&sem_blocked);
						if (proceso_desbloqueado != NULL) {
							//añadimos que dicho proceso está usando una instancia
							uint32_t* pid = malloc(sizeof(uint32_t));
							*pid = proceso_desbloqueado->PID;
							list_add(recurso_signal->pid_procesos,pid);

							char* estado_anterior_proceso_desbloqueado = estado_proceso_to_string(proceso_desbloqueado->estado);

							sem_wait(&sem_procesos);
							PCB* actualizado_signal = encontrarProceso(lista_procesos,instruccion_recurso_signal->proceso.PID);
							actualizado_signal->estado = READY;
							char* estado_actual_proceso_desbloqueado = estado_proceso_to_string(actualizado_signal->estado);
							sem_post(&sem_procesos);

							log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", actualizado_signal->PID, estado_anterior_proceso_desbloqueado, estado_actual_proceso_desbloqueado);

							free(estado_anterior_proceso_desbloqueado);
							free(estado_actual_proceso_desbloqueado);

							if( strcmp(ALGORITMO_PLANIFICACION,"VRR") == 0 )
							{
								encolar_procesos_vrr(proceso_desbloqueado);
							}
							else
							{
								// Enviamos el proceso a la cola de ready
								sem_wait(&sem_ready);   // mutex hace wait
								//printf("En este momento en la cola de ready se ha pusheado al pid: %d\n", proceso_desbloqueado->PID);
								queue_push(cola_ready, proceso_desbloqueado); // agrega el proceso a la cola de ready

								char* cadena_pids = obtener_cadena_pids(cola_ready->elements);
								log_info (kernel_logs_obligatorios, "Cola Ready: %s\n", cadena_pids);
								free(cadena_pids);

								sem_post(&sem_ready);
								sem_post(&sem_cant_ready);  // mutex hace wait
							}
						}
					}
					
					//Tenemos que sacar el pid de la lista del recurso
					eliminar_elemento_pid(recurso_signal->pid_procesos, instruccion_recurso_signal->proceso.PID);

					PCB* proceso_recurso = malloc(sizeof(PCB));
					*proceso_recurso = instruccion_recurso_signal->proceso;

					//debe volver a ejecutarse
					//printf("Enviaremos un proceso\n");
					//printf("Enviaremos el proceso cuyo pid es %d\n",proceso_recurso->PID);
					enviarPCB(proceso_recurso, fd_cpu_dispatch,PROCESO);
					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = true;
					sem_post(&sem_mutex_cpu_ocupada);
					free(proceso_recurso);

				} 
				else 
				{
					// El recurso no existe, debemos terminarlo
					//printf("El recurso no existe\n");
					//printf("Este proceso ha terminado\n");
					log_info(kernel_logs_obligatorios,"PID: <%d> - El recurso: <%s> no existe\n",instruccion_recurso_signal->proceso.PID,instruccion_partida_signal[1]);

					char* estado_anterior_proceso_to_end = estado_proceso_to_string(instruccion_recurso_signal->proceso.estado);

					sem_wait(&sem_procesos);
					PCB* actualizado_signal = encontrarProceso(lista_procesos,instruccion_recurso_signal->proceso.PID);
					actualizado_signal->estado = EXIT;
					char* estado_actual_proceso_to_end = estado_proceso_to_string(actualizado_signal->estado);
					sem_post(&sem_procesos);

					log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", actualizado_signal->PID, estado_anterior_proceso_to_end, estado_actual_proceso_to_end);
					log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <INVALID_RESOURCE>\n", actualizado_signal->PID);

					free(estado_anterior_proceso_to_end);
					free(estado_actual_proceso_to_end);

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
				string_array_destroy(instruccion_partida_signal);
				free(instruccion_recurso_signal->instruccion);
				free(instruccion_recurso_signal);
				break;				
			case DIALFS:
				
				// Deserializamos
					Instruccion_io* instruccion_io_fs = deserializar_instruccion_io(paquete->buffer);
					//printf("La instrucción es: %s\n", instruccion_io_fs->instruccion);
					//printf("El PID del proceso es: %d\n", instruccion_io_fs->proceso.PID);

					// Debemos obtener la IO específica de la lista
					sem_wait(&sem_mutex_lists_io);
					char** io_fs_split = string_split(instruccion_io_fs->instruccion, " ");
					EntradaSalida* io_fs = encontrar_io(listDialfs, io_fs_split[1]);
					string_array_destroy(io_fs_split);
					sem_post(&sem_mutex_lists_io);


					// Verificamos que exista
					if (io_fs != NULL)
					{
						// Existe
						// Una vez encontrada la IO, vemos si está ocupada
						char** instruccion_partida_dialfs = string_split(instruccion_io_fs->instruccion," ");
						log_info(kernel_logs_obligatorios,"PID: <%d> - Bloqueado por: <%s>\n",instruccion_io_fs->proceso.PID,instruccion_partida_dialfs[1]);

						if (io_fs->ocupado)
						{
							/*
							// Si está ocupada, añadimos el proceso a la lista de bloqueados
							printf("La IO está ocupada, se bloqueará en su lista propia\n");
							Instruccion_io* to_block = malloc(sizeof(Instruccion_io));
							to_block->proceso = instruccion_io_fs->proceso;
							to_block->tam_instruccion = instruccion_io_fs->tam_instruccion;
							to_block->instruccion = malloc(to_block->tam_instruccion+1);
							strcpy(to_block->instruccion,instruccion_io_fs->instruccion);
							list_add(io_fs->procesos_bloqueados, to_block);
							*/

							// Si está ocupada, añadimos el proceso a la lista de bloqueados
							printf("La IO está ocupada, se bloqueará en su lista propia\n");

							// Creamos una copia de la instrucción para bloquearla
							Instruccion_io* to_block = malloc(sizeof(Instruccion_io));
							to_block->proceso = instruccion_io_fs->proceso;
							to_block->tam_instruccion = instruccion_io_fs->tam_instruccion;

							// Reservamos memoria para la instrucción y copiamos el contenido
							to_block->instruccion = malloc(to_block->tam_instruccion + 1);
							strncpy(to_block->instruccion, instruccion_io_fs->instruccion, to_block->tam_instruccion);
							to_block->instruccion[to_block->tam_instruccion] = '\0';  // Aseguramos la terminación de la cadena

							printf("La instrucción añadida a esta lista es la siguiente: %s\n", to_block->instruccion);
							printf("Su tamaño es este: %d\n", to_block->tam_instruccion);

							// Añadimos la copia a la lista de procesos bloqueados
							list_add(io_fs->procesos_bloqueados, to_block);


						}
						else
						{
							io_fs->ocupado = true;
							printf("La IO no está ocupada, proceda\n");
							
							if( strcmp(instruccion_partida_dialfs[0], "IO_FS_CREATE") == 0 )
							{
								//crear archivo 
								new_ejecutar_interfaz_stdin_stdout(instruccion_io_fs->instruccion, IO_FS_CREATE, io_fs->fd_cliente, instruccion_io_fs->proceso.PID);
							}
							else
							{
								if( strcmp(instruccion_partida_dialfs[0], "IO_FS_DELETE") == 0 )
								{
									//archivo borrar
									new_ejecutar_interfaz_stdin_stdout(instruccion_io_fs->instruccion, IO_FS_DELETE, io_fs->fd_cliente, instruccion_io_fs->proceso.PID);
								}
								else
								{
									if( strcmp(instruccion_partida_dialfs[0], "IO_FS_TRUNCATE") == 0 )
									{
										//archivo truncar
										new_ejecutar_interfaz_stdin_stdout(instruccion_io_fs->instruccion, IO_FS_TRUNCATE, io_fs->fd_cliente, instruccion_io_fs->proceso.PID);
									}
									else
									{
										if( strcmp(instruccion_partida_dialfs[0], "IO_FS_WRITE") == 0 )
										{
											//archivo escribir
											new_ejecutar_interfaz_stdin_stdout(instruccion_io_fs->instruccion, IO_FS_WRITE, io_fs->fd_cliente, instruccion_io_fs->proceso.PID);
										}
										else
										{
											//archivo leer	
											new_ejecutar_interfaz_stdin_stdout(instruccion_io_fs->instruccion, IO_FS_READ, io_fs->fd_cliente, instruccion_io_fs->proceso.PID);
										}
									}
								}
								
							}
						}
						string_array_destroy(instruccion_partida_dialfs);
					}
					else
					{
						// Si no existe, debemos terminar el proceso
						sem_wait(&sem_blocked);
						PCB* proceso_to_end = encontrar_por_pid(cola_blocked->elements, instruccion_io_fs->proceso.PID);
						sem_post(&sem_blocked);

						char* estado_anterior_proceso_to_end = estado_proceso_to_string(proceso_to_end->estado);

						sem_wait(&sem_procesos);
						PCB* actualizado_fs = encontrarProceso(lista_procesos, instruccion_io_fs->proceso.PID);
						actualizado_fs->estado = EXIT;
						char* estado_actual_proceso_to_end = estado_proceso_to_string(actualizado_fs->estado);
						sem_post(&sem_procesos);

						log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_to_end->PID, estado_anterior_proceso_to_end, estado_actual_proceso_to_end);
						log_info(kernel_logs_obligatorios, "Finaliza el proceso <%u> - Motivo: <INVALID_INTERFACE>\n", proceso_to_end->PID);

						free(estado_anterior_proceso_to_end);
						free(estado_actual_proceso_to_end);

						//printf("Este proceso ha terminado\n");
						liberar_recursos(proceso_to_end->PC);
						enviarPCB(proceso_to_end, fd_memoria, PROCESOFIN);
						//free(proceso_to_end->path);
						free(proceso_to_end);
					}

					sem_wait(&sem_mutex_cpu_ocupada);
					cpu_ocupada = false;
					sem_post(&sem_mutex_cpu_ocupada);
					free(instruccion_io_fs->instruccion);
					free(instruccion_io_fs);
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

//encuentra la io en alguna de las listas, indica que ya no está ocupada y envía a un proceso si es que está ahí esperando
void modificar_io_en_listas(int fd_io)
{
	EntradaSalida* to_ret;
	to_ret = encontrar_por_fd_cliente(listGenericas, fd_io);
	if( to_ret != NULL )
	{
		to_ret->ocupado = false;
		if( list_size(to_ret->procesos_bloqueados) > 0 )
		{
			Instruccion_io* proceso_bloqueado_io = list_remove(to_ret->procesos_bloqueados,0);
			//printf("De la lista hemos extraído la instrucción %s\n",proceso_bloqueado_io->instruccion);
			//printf("De la lista hemos extraído el pid %d\n",proceso_bloqueado_io->proceso.PID);
			ejecutar_interfaz_generica(proceso_bloqueado_io->instruccion,GENERICA,fd_io,proceso_bloqueado_io->proceso.PID);

			//Cuidado que puede romper 1/8 12:03hs
			free(proceso_bloqueado_io->instruccion);
			free(proceso_bloqueado_io);
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
				new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion,STDIN,fd_io,proceso_bloqueado_io->proceso.PID);
				free(proceso_bloqueado_io->instruccion);
				free(proceso_bloqueado_io);
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
					new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion,STDOUT,fd_io,proceso_bloqueado_io->proceso.PID);
					free(proceso_bloqueado_io->instruccion);
					free(proceso_bloqueado_io);
				}
			}
			else
			{
				to_ret = encontrar_por_fd_cliente(listDialfs, fd_io);
				to_ret->ocupado = false;
				if( list_size(to_ret->procesos_bloqueados) > 0 )
				{
					Instruccion_io* proceso_bloqueado_io = list_remove(to_ret->procesos_bloqueados,0);
					//new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion,DIALFS,fd_io,proceso_bloqueado_io->proceso.PID);
					char** instruccion_partida_dialfs = string_split(proceso_bloqueado_io->instruccion," ");
					if( strcmp(instruccion_partida_dialfs[0], "IO_FS_CREATE") == 0 )
					{
						//crear archivo 
						new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion, IO_FS_CREATE, fd_io, proceso_bloqueado_io->proceso.PID);
					}
					else
					{
						if( strcmp(instruccion_partida_dialfs[0], "IO_FS_DELETE") == 0 )
						{
							//archivo borrar
							new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion, IO_FS_DELETE, fd_io, proceso_bloqueado_io->proceso.PID);
						}
						else
						{
							if( strcmp(instruccion_partida_dialfs[0], "IO_FS_TRUNCATE") == 0 )
							{
								//archivo truncar
								new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion, IO_FS_TRUNCATE, fd_io, proceso_bloqueado_io->proceso.PID);
							}
							else
							{
								if( strcmp(instruccion_partida_dialfs[0], "IO_FS_WRITE") == 0 )
								{
									//archivo escribir
									new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion, IO_FS_WRITE, fd_io, proceso_bloqueado_io->proceso.PID);
								}
								else
								{
									//archivo leer	
									new_ejecutar_interfaz_stdin_stdout(proceso_bloqueado_io->instruccion, IO_FS_READ, fd_io, proceso_bloqueado_io->proceso.PID);
								}
							}
						}
						
					}
					string_array_destroy(instruccion_partida_dialfs);
					free(proceso_bloqueado_io->instruccion);
					free(proceso_bloqueado_io);
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
			//printf("El fd de este IO es %d\n",*fd_io);
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
				//printf("La IO ha despertado :O\n");
				int* pid_io = paquete->buffer->stream;
				//printf("El pid que ha llegado de la IO es %d\n",*pid_io);

				//PCB* proceso_awaken = queue_pop(cola_blocked);
				sem_wait(&sem_blocked);
				PCB* proceso_awaken = encontrar_por_pid(cola_blocked->elements,(uint32_t)*pid_io);
				sem_post(&sem_blocked);
				
				char* estado_anterior_awaken = estado_proceso_to_string(proceso_awaken->estado);
				
				//ACÁ METERÉ EN READY DEPENDIENDO DEL QUANTUM DEL PROCESO
				proceso_awaken->estado = READY;

				sem_wait(&sem_procesos);
				PCB* actualizado = encontrarProceso(lista_procesos,(uint32_t)*pid_io);
				actualizado->estado = READY;
				char* estado_actual_awaken = estado_proceso_to_string(actualizado->estado);
				sem_post(&sem_procesos);

				log_info(kernel_logs_obligatorios,"PID: <%u> - Estado Anterior : <%s> - Estado Actual: <%s>\n",proceso_awaken->PID,estado_anterior_awaken,estado_actual_awaken);

				free(estado_anterior_awaken);
				free(estado_actual_awaken);

				//si el quantum del proceso es mayor a cero, debe ir a la cola de mayor prioridad
				//meter en ready
				if( strcmp(ALGORITMO_PLANIFICACION, "VRR") == 0 )
				{
					encolar_procesos_vrr(proceso_awaken);
				}
	 			else
				{
					sem_wait(&sem_ready);   // mutex hace wait
					queue_push(cola_ready,proceso_awaken);	//agrega el proceso a la cola de ready

					char* cadena_pids = obtener_cadena_pids(cola_ready->elements);
					log_info (kernel_logs_obligatorios, "Cola Ready: %s\n", cadena_pids);
					free(cadena_pids);

					sem_post(&sem_ready); 
					sem_post(&sem_cant_ready);
				}

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
	pcb->quantum = atoi(QUANTUM);
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
	//pcb->path_length = strlen(path)+1;
	//pcb->path = string_duplicate(path); //consejo

	//en este punto aprovecho para enviarlo a memoria
	ProcesoMemoria* proceso = malloc(sizeof(ProcesoMemoria));
	proceso->path = malloc(strlen(path)+1);
	//proceso->path = pcb->path;
	strcpy(proceso->path, path);
	proceso->PID = pcb->PID;
	proceso->path_length = strlen(path)+1;

	enviarProcesoMemoria(proceso,fd_memoria);

	free(proceso->path);
	free(proceso);

	//agrego el pcb a la cola new
	
	PCB* new_pcb = malloc(sizeof(PCB));
	memcpy(new_pcb, pcb, sizeof(PCB));
    new_pcb->path = strdup(path);

	sem_wait(&sem_procesos);
	list_add(lista_procesos,new_pcb);
	//printf("Agregué el proceso %d a la lista de procesos\n", pcb->PID);
	sem_post(&sem_procesos);
	
	
	sem_wait(&sem);
	queue_push(cola_new,pcb);	
    // Señalar (incrementar) el semáforo
    sem_post(&sem);
	sem_post(&sem_cant);
	
	//procesos_en_new++;
	
	log_info (kernel_logs_obligatorios, "Se crea el proceso <%d> en NEW\n", pcb->PID);
}

void finalizar_proceso (char* pid) {
    uint32_t pid_to_eliminar = (uint32_t)atoi(pid);
   // printf("El pid solicitado a eliminar es %d\n", pid_to_eliminar);

    sem_wait(&sem_procesos);
    PCB* proceso_a_terminar = encontrarProceso(lista_procesos, pid_to_eliminar);
    sem_post(&sem_procesos);

    //printf("El proceso a terminar luce de la siguiente forma: \n");
    //printf("PID: %d\n", proceso_a_terminar->PID);
   // printf("Length del path: %d\n", proceso_a_terminar->path_length);
    //printf("Path: %s\n", proceso_a_terminar->path);

    char* estado_anterior = estado_proceso_to_string(proceso_a_terminar->estado);

    switch (proceso_a_terminar->estado) {
        case NEW:
            //printf("El proceso de pid %d es eliminado estando en NEW\n", proceso_a_terminar->PID);
            liberar_recursos(pid_to_eliminar);
            enviarPCB(proceso_a_terminar, fd_memoria, PROCESOFIN);
            sem_wait(&sem);
            eliminar_elemento(cola_new->elements, pid_to_eliminar);
            sem_post(&sem);
            break;
        case READY:
           // printf("El proceso de pid %d es eliminado estando en READY\n", proceso_a_terminar->PID);
            liberar_recursos(pid_to_eliminar);
            enviarPCB(proceso_a_terminar, fd_memoria, PROCESOFIN);
            sem_wait(&sem_ready);
            eliminar_elemento(cola_ready->elements, pid_to_eliminar);
            sem_post(&sem_ready);
            break;
        case EXEC:
            int* pid_del_proceso = malloc(sizeof(int));
            *pid_del_proceso = proceso_a_terminar->PID;
            enviarEntero(pid_del_proceso, fd_cpu_interrupt, FINALIZAR_PROCESO);
			free(pid_del_proceso);
            break;
        case BLOCKED:
           // printf("El proceso de pid %d es eliminado estando en BLOCKED\n", proceso_a_terminar->PID);
			liberar_recursos(pid_to_eliminar);
            enviarPCB(proceso_a_terminar, fd_memoria, PROCESOFIN);
            sem_wait(&sem_blocked);
            eliminar_elemento(cola_blocked->elements, pid_to_eliminar);
            sem_post(&sem_blocked);
            break;
    }
    proceso_a_terminar->estado = EXIT;
    char* estado_actual = estado_proceso_to_string(proceso_a_terminar->estado);

    log_info(kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>\n", proceso_a_terminar->PID, estado_anterior, estado_actual);
	free(estado_anterior);
	free(estado_actual);
}



void proceso_estado(){
	sem_wait(&sem_procesos);
	for(int i = 0; i < list_size(lista_procesos); i++)
	{

		PCB* pcb = list_get(lista_procesos,i);
		char* estado = estado_proceso_to_string (pcb->estado);
		printf("El proceso %d esta en estado <%s>\n",pcb->PID,estado);
		free(estado);
		
	}
	sem_post(&sem_procesos);
}

void cambio_multiprogramacion (char* valor){
	int valor_nuevo = atoi (valor);
	printf ("Grado de multiprogramacion ANTES: %d\n",grado_multiprogramacion);
	sem_wait(&sem_grado_mult);
	grado_multiprogramacion = valor_nuevo;
	printf ("Grado de multiprogramacion DESPUES: %d\n",grado_multiprogramacion);
	sem_post(&sem_grado_mult);
}

char* remove_last_two_chars(const char* str) {
    int len = strlen(str);
    if (len <= 2) {
        return strdup(""); // Devuelve una cadena vacía si la longitud es menor o igual a 2
    }

    char* new_str = (char*)malloc((len - 1) * sizeof(char)); // Reserva memoria para la nueva cadena
    if (new_str == NULL) {
        return NULL; // Manejo de error en caso de que malloc falle
    }

    strncpy(new_str, str, len - 2); // Copia la cadena original sin los dos últimos caracteres
    new_str[len - 2] = '\0'; // Añade el carácter nulo al final

    return new_str;
}

void atender_instruccion (char* leido)
{
    char** comando_consola = string_split(leido, " ");
	//printf("%s\n",comando_consola[0]);

    if((strcmp(comando_consola[0], "INICIAR_PROCESO") == 0))
	{ 
        iniciar_proceso(comando_consola[1]);  
    }else if(strcmp(comando_consola [0], "EJECUTAR_SCRIPT") == 0){
		ejecutar_script(comando_consola[1]);
		
    }else if(strcmp(comando_consola [0], "FINALIZAR_PROCESO") == 0){
		finalizar_proceso(comando_consola[1]);

    }else if(strcmp(comando_consola [0], "DETENER_PLANIFICACION") == 0){
		iniciar_planificacion = false;

    }else if(strcmp(comando_consola [0], "INICIAR_PLANIFICACION") == 0){
		iniciar_planificacion = true;

    }else if(strcmp(comando_consola [0], "MULTIPROGRAMACION") == 0){
		cambio_multiprogramacion(comando_consola[1]);
		 
    }else if(strcmp(comando_consola [0], "PROCESO_ESTADO") == 0){
		proceso_estado();
		
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
		atender_instruccion(leido);
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
				atender_instruccion(leido);
	        }
			else
			{
				if(strcmp(valorLeido[0], "DETENER_PLANIFICACION") == 0)
	            {  
		            printf("Comando válido\n");
					atender_instruccion(leido);
	            }
				else
				{
					if(strcmp(valorLeido[0], "INICIAR_PLANIFICACION") == 0)
	                {  
		                printf("Comando válido\n");
						atender_instruccion(leido);
	                }
					else
					{
						if(strcmp(valorLeido[0], "MULTIPROGRAMACION") == 0)
	                    {  
		                       printf("Comando válido\n");
							   atender_instruccion(leido);
	                    }
						else
						{
							if(strcmp(valorLeido[0], "PROCESO_ESTADO") == 0)
	                        {  
		                       printf("Comando válido\n");
							   atender_instruccion(leido);
	                        }
						}
					}
				}
			}
		}
	 }
	 string_array_destroy(valorLeido);
}

void ejecutar_script (char* path)
{
	FILE *file = fopen(path, "r");
    if (file == NULL) {
        perror("No se pudo abrir el archivo");
        return;
    }
	
	char linea[256];
    while (fgets(linea, sizeof(linea), file)) {
		//printf("La linea leida es: %s", linea);
		char* linea_modificada = remove_last_two_chars(linea);
		//printf("La linea modificada leida es: %s\n", linea_modificada);
		validarFuncionesConsola(linea_modificada);
		free(linea_modificada);
		sleep (1);
    }
	fclose(file);
}


void consolaInteractiva()
{
	char* leido;
	leido = readline("> ");
	
	while( strcmp(leido,"--------") != 0 )
	{
		validarFuncionesConsola(leido);
		free(leido);
		leido = readline("> ");
	}
}

void mover_procesos_a_ready()
{
	bool added = false;
	sem_wait(&sem_cant);   // mutex hace wait
	sem_wait(&sem);   // mutex hace wait
	while( added != true )
	{
		if (iniciar_planificacion == true){
			sem_wait(&sem_grado_mult);
			sem_wait(&sem_blocked);
			sem_wait(&sem_ready);
			if( (queue_size(cola_ready) + queue_size(cola_blocked) + 1) < grado_multiprogramacion )
			{
				PCB* pcb = queue_pop(cola_new);
				sem_post(&sem_blocked);
				sem_post(&sem_grado_mult);

				sem_wait(&sem_procesos);
				PCB* actualizado = encontrarProceso(lista_procesos,pcb->PID);
				char* estado_anterior = estado_proceso_to_string (actualizado->estado);
				actualizado->estado = READY;
				char* estado_actual = estado_proceso_to_string (actualizado->estado);
				sem_post(&sem_procesos);

				queue_push(cola_ready,pcb);	//agrega el proceso a la cola de ready
				char* cadena_pids = obtener_cadena_pids(cola_ready->elements);
				char* cadena_copied = malloc(strlen(cadena_pids)+1);
				strncpy(cadena_copied,cadena_pids,strlen(cadena_pids)+1);
				log_info (kernel_logs_obligatorios, "Cola Ready: %s\n", cadena_copied);
				//printf("Cola Ready: %s",cadena_pids);
				free(cadena_pids);
				free(cadena_copied);
				log_info (kernel_logs_obligatorios, "PID: <%d> - Estado Anterior: <%s> - Estado Actual: <%s>", pcb->PID, estado_anterior, estado_actual);
				
				free(estado_anterior);
				free(estado_actual);

				sem_post(&sem_ready); 
				sem_post(&sem_cant_ready);  // mutex hace wait
				added = true;
			}
			else
			{
				sem_post(&sem_grado_mult);
				sem_post(&sem_blocked);
				sem_post(&sem_ready);
			}
		}
		sem_post(&sem);   // mutex hace signal
		sleep(1);
	}

}

void planificador_de_largo_plazo()
{
	
	while(1)
	{	
		if (iniciar_planificacion == true){
		
			//sem_wait(&sem_cant);   // mutex hace wait
			//sem_wait(&sem);   // mutex hace wait

			mover_procesos_a_ready();
			
			//sem_post(&sem);   // mutex hace signal
		}
	}	
}

void interrumpir_por_quantum_vrr(int proceso_quantum)
{
	//iniciamos el temporaizador
	sem_wait(&sem_mutex_cronometro);
	cronometro = temporal_create();
	sem_post(&sem_mutex_cronometro);

	printf("Esperaremos el siguiente intervalo de tiempo: %d\n",proceso_quantum);
	//esperamos el tiempo
	int quantum = atoi(QUANTUM);
	if( proceso_quantum != quantum )
	{
		//si es distinto, le queda quantum restande
		usleep(proceso_quantum*1000);
	}
	else
	{
		//de lo contrario esperamos el quantum normal
		usleep(quantum*1000);
	}

	//Una vez el tiempo haya pasado interrumpimos
	int* enteroRandom = malloc(sizeof(int));
	*enteroRandom = 0;
	enviarEntero(enteroRandom,fd_cpu_interrupt,FIN_DE_QUANTUM);
	free(enteroRandom);
	sem_wait(&sem_mutex_cronometro);
	temporal_destroy(cronometro);
	sem_post(&sem_mutex_cronometro);
}

void interrumpir_por_quantum()
{
	int quantumAUsar = atoi(QUANTUM);
	unsigned int quantum = quantumAUsar;
	//printf("El quantum sera: %d\n",quantumAUsar);
	usleep(quantum*1000);
	//MANDAR A MEMORIA FIN DE QUANTUM POR DISPATCH
	int* enteroRandom = malloc(sizeof(int));
	*enteroRandom = 0;
 	//printf("Me desperte uwu\n");
	enviarEntero(enteroRandom,fd_cpu_interrupt,FIN_DE_QUANTUM);
	free(enteroRandom);
}

void enviar_pcb_a_cpu()
{
	sem_wait(&sem_mutex_cpu_ocupada);
	while( cpu_ocupada != false )
	{
		sem_post(&sem_mutex_cpu_ocupada);
		sleep(1);
		//usleep(500000);

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

	//to_send->path = string_duplicate( pcb_cola->path);

	//ACTUALIZAMOS EN LA LISTA GENERAL
	sem_wait(&sem_procesos);
	PCB* actualizado = encontrarProceso(lista_procesos,pcb_cola->PID);
	actualizado->estado = EXEC;
	log_info(kernel_logs_obligatorios,"PID: <%u> - Estado Anterior : <READY> - Estado Actual: <EXEC>\n",actualizado->PID);
	sem_post(&sem_procesos);


	//printf("Enviaremos un proceso\n");
	//printf("Enviaremos el proceso cuyo pid es %d\n",to_send->PID);
	enviarPCB(to_send, fd_cpu_dispatch,PROCESO);
	sem_wait(&sem_mutex_cpu_ocupada);
	cpu_ocupada = true;
	sem_post(&sem_mutex_cpu_ocupada);
	if( strcmp(ALGORITMO_PLANIFICACION,"RR") == 0 )
	{
		interrumpir_por_quantum();
	}
	//free(pcb_cola->path);
	free(pcb_cola);
	//free(to_send->path);
	free(to_send);
}

void enviar_pcb_a_cpu_vrr()
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

	PCB* pcb_cola;

	/*
	//Priorizaremos a aquellos procesos con mayor prioridad
	sem_wait(&sem_ready_prio);
	if( queue_size(cola_ready_prioridad) > 0 )
	{
		printf("Entré en la cola de prioridad\n");

		pcb_cola = queue_pop(cola_ready_prioridad); //saca el proceso de la cola de ready
	}
	else
	{
		printf("Entré en la cola normal\n");
		sem_wait(&sem_cant_ready);   // mutex hace wait
		sem_wait(&sem_ready);   // mutex hace wait
		pcb_cola = queue_pop(cola_ready); //saca el proceso de la cola de ready
		sem_post(&sem_ready); // mutex hace signal
	}
	sem_post(&sem_ready_prio);
	*/

	bool encolado = false;
	while( encolado != true )
	{
		sem_wait(&sem_ready_prio);
		if( queue_size(cola_ready_prioridad) > 0 )
		{
			printf("Entré en la cola de prioridad\n");
			pcb_cola = queue_pop(cola_ready_prioridad); //saca el proceso de la cola de ready
			encolado = true;
		}
		else
		{
			sem_wait(&sem_ready);   // mutex hace wait
			if( queue_size(cola_ready) > 0)
			{
				pcb_cola = queue_pop(cola_ready); //saca el proceso de la cola de ready
				encolado = true;
			}
			sem_post(&sem_ready); // mutex hace signal
		}
		sem_post(&sem_ready_prio);
	}

	to_send->PID = pcb_cola->PID;
	to_send->PC = pcb_cola->PC;
	to_send->quantum = pcb_cola->quantum;
	to_send->estado = EXEC;
	to_send->registro = pcb_cola->registro;//cambios
	//to_send->path = string_duplicate( pcb_cola->path);

	//ACTUALIZAMOS EN LA LISTA GENERAL
	sem_wait(&sem_procesos);
	PCB* actualizado = encontrarProceso(lista_procesos,pcb_cola->PID);
	actualizado->estado = EXEC;
	log_info(kernel_logs_obligatorios,"PID: <%u> - Estado Anterior : <READY> - Estado Actual: <EXEC>\n",actualizado->PID);
	sem_post(&sem_procesos);

	printf("Enviaremos un proceso\n");
	printf("Enviaremos el proceso cuyo pid es %d\n",to_send->PID);
	printf("El quantum de dicho proceso es %d\n",to_send->quantum);
	enviarPCB(to_send, fd_cpu_dispatch,PROCESO);
	sem_wait(&sem_mutex_cpu_ocupada);
	cpu_ocupada = true;
	sem_post(&sem_mutex_cpu_ocupada);
	if(strcmp(ALGORITMO_PLANIFICACION,"VRR")==0)
	{
		interrumpir_por_quantum_vrr(to_send->quantum);
	}
	//free(pcb_cola->path);
	free(pcb_cola);
	//free(to_send->path);
	free(to_send);
}


/*
void enviar_pcb_a_cpu_vrr()
{
    // Reservo memoria para enviarla
    PCB* to_send = malloc(sizeof(PCB));
    PCB* pcb_cola;

    // Priorizaremos a aquellos procesos con mayor prioridad
    sem_wait(&sem_ready_prio);
    if (queue_size(cola_ready_prioridad) > 0)
    {
        printf("Entré en la cola de prioridad\n");
        pcb_cola = queue_pop(cola_ready_prioridad); // saca el proceso de la cola de ready
    }
    else
    {
        sem_wait(&sem_cant_ready);
        sem_wait(&sem_ready);
        pcb_cola = queue_pop(cola_ready); // saca el proceso de la cola de ready
        sem_post(&sem_ready);
    }
    sem_post(&sem_ready_prio);

    // Copiamos los datos del proceso seleccionado en el PCB a enviar
    to_send->PID = pcb_cola->PID;
    to_send->PC = pcb_cola->PC;
    to_send->quantum = pcb_cola->quantum;
    to_send->estado = EXEC;
    to_send->registro = pcb_cola->registro;

    // ACTUALIZAMOS EN LA LISTA GENERAL
    sem_wait(&sem_procesos);
    PCB* actualizado = encontrarProceso(lista_procesos, pcb_cola->PID);
    if (actualizado != NULL) {
        actualizado->estado = EXEC;
        log_info(kernel_logs_obligatorios, "PID: <%u> - Estado Anterior : <READY> - Estado Actual: <EXEC>\n", actualizado->PID);
    }
    sem_post(&sem_procesos);

    printf("Enviaremos un proceso\n");
    printf("Enviaremos el proceso cuyo pid es %d\n", to_send->PID);
    printf("El quantum de dicho proceso es %d\n", to_send->quantum);

    // Sección crítica: verificar y actualizar cpu_ocupada
    sem_wait(&sem_mutex_cpu_ocupada);
    while (cpu_ocupada)
    {
        sem_post(&sem_mutex_cpu_ocupada);
        sleep(1);
        sem_wait(&sem_mutex_cpu_ocupada);
    }
    cpu_ocupada = true;
    sem_post(&sem_mutex_cpu_ocupada);

    // Enviar el PCB al CPU
    enviarPCB(to_send, fd_cpu_dispatch, PROCESO);

    if (strcmp(ALGORITMO_PLANIFICACION, "VRR") == 0)
    {
        interrumpir_por_quantum_vrr(to_send->quantum);
    }

    // Liberar memoria utilizada
    free(pcb_cola);
    free(to_send);
}
*/

void planificador_corto_plazo()
{
	if (iniciar_planificacion == true){
	
		if(strcmp(ALGORITMO_PLANIFICACION,"FIFO")==0)
		{
			printf("Planificare por FIFO\n");
			while(1)
			{
				if (iniciar_planificacion == true){

					//PLANIFICAR POR FIFO
					sem_wait(&sem_mutex_plani_corto);
					enviar_pcb_a_cpu();
					sem_post(&sem_mutex_plani_corto);
				}
			}
		}
		
		if(strcmp(ALGORITMO_PLANIFICACION,"RR")==0)
		{
			printf("Planificare por RR\n");
			//PLANIFICAR POR RR
			while(1)
			{
				if (iniciar_planificacion == true){

					sem_wait(&sem_mutex_plani_corto);
					enviar_pcb_a_cpu();
					sem_post(&sem_mutex_plani_corto);
					//interrumpir_por_quantum();
				}
			}
		}
		if(strcmp(ALGORITMO_PLANIFICACION,"VRR")==0)
		{
			//PLANIFICAR POR VRR
			printf("Planificare por VRR\n");
			//PLANIFICAR POR VRR
			while(1)
			{
				if (iniciar_planificacion == true){

				sem_wait(&sem_mutex_plani_corto);
				enviar_pcb_a_cpu_vrr();
				sem_post(&sem_mutex_plani_corto);
				}
			}
		}
	}
}
	

#endif /* KERNEL_H_ */
