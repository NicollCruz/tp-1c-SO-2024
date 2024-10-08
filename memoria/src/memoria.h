#ifndef MEMORIA_H_
#define MEMORIA_H_

#include<stdio.h>
#include<stdlib.h>
#include<utils/utils.h>
#include <utils/utils.c>

//file descriptors de memoria y los modulos que se conectaran con ella
int fd_memoria;
int fd_kernel;
int fd_cpu;
int fd_entradasalida;

sem_t protect_list_procesos;

t_log* memoria_logger; //LOG ADICIONAL A LOS MINIMOS Y OBLIGATORIOS
t_config* memoria_config;
t_list* listProcesos; 
t_list* listMarcos;

t_list* listStdin;
t_list* listStdout;
t_list* listDialfs;

char* PUERTO_ESCUCHA;
int TAM_MEMORIA;
int TAM_PAGINA;
char* PATH_INSTRUCCIONES;
int RETARDO_RESPUESTA;

void* espacio_usuario;
void* espacio_pagina;

t_log *memoria_logs_obligatorios;// LOGS MINIMOS Y OBLIGATORIOS

t_list* generarListaDeMarcos(t_list* listMarcos)
{
	t_list* to_return = list_create();
	//agregamos -1 en todos los marcos para indicar que están libres
	int i = 0;
	while( i < (TAM_MEMORIA / TAM_PAGINA))
	{
		int* valor = malloc(sizeof(int));
		*valor = -1;
		list_add(to_return,valor);
		i++;
	}
	return to_return;
}

char* abrir_archivo(char* path, int PC);

ProcesoMemoria* encontrarProceso(t_list* lista, uint32_t pid)
{
	ProcesoMemoria* ret;
	int i = 0;
	while( i < list_size(lista) )
	{
		ProcesoMemoria* got = list_get(lista,i);
		if( got->PID == pid)
		{
			ret = got;
		}
		i++;
	}
	return ret;
}

int cantiMarcosLibresMemoria(t_list* listMarcos)
{
    int contador = 0;
    for(int i = 0; i < list_size(listMarcos); i++) {
        int* elemento = list_get(listMarcos, i);
        if(elemento != NULL && *elemento == -1) {
            contador++;
        }
    }
    return contador;
}

int dividir_y_redondear_arriba(int num1, int num2) 
{
    if (num2 == 0) {
        // Manejar división por cero
        return -1;
    }
    double division = (double)num1 / num2;
    return (int)ceil(division);
}

void memoria_escuchar_cpu (){
		bool control_key = 1;
	while (control_key) {
			int cod_op = recibir_operacion(fd_cpu);

			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
			recv(fd_cpu,&(paquete->buffer->size),sizeof(uint32_t),0);
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(fd_cpu,paquete->buffer->stream, paquete->buffer->size,0);

			printf("Recibi el codigo operacion\n");

			switch (cod_op) {
			case MENSAJE: 
				//
				break;
			case PROCESO:
				//
				printf("------------------------------\n");
				//desearializamos lo recibido
				
				PCB* proceso = deserializar_proceso_cpu(paquete->buffer);
				//printf("Los datos recibidos de CPU son pid: %d\n",proceso->PID);
				//printf("Los datos recibidos de CPU son pc: %d\n",proceso->PC);
				
				sem_wait(&protect_list_procesos);
				ProcesoMemoria* datos = encontrarProceso(listProcesos,proceso->PID);
				sem_post(&protect_list_procesos);
				//printf("Los datos encontrados son los siguientes pid: %d\n",datos->PID);
				//printf("Los datos encontrados son los siguientes path: %s\n",datos->path);

				//leemos la linea indicada por el PC
			    char* instruccion = abrir_archivo(datos->path, proceso->PC); 
				//printf("Enviaremos a la cpu: %s\n",instruccion); 
				//enviamos, sin antes dormir el tiempo esperado
				//printf("Me voy a mimir, buenas noches\n");
				usleep(RETARDO_RESPUESTA*1000);
				enviar_mensaje_cpu_memoria(instruccion,fd_cpu,MENSAJE);
				//free(proceso->path);
				free(proceso);
				free(instruccion);
				break;

			case RESIZE:
				PCB* proceso_resize = deserializar_proceso_cpu(paquete->buffer);
				printf("Los datos recibidos de CPU para HACER RESIZE son pid: %d\n",proceso_resize->PID);
				printf("Los datos recibidos de CPU para HACER RESIZE pc: %d\n",proceso_resize->PC);
				
				sem_wait(&protect_list_procesos);
				ProcesoMemoria* datos_resize = encontrarProceso(listProcesos,proceso_resize->PID);
				sem_post(&protect_list_procesos);
				printf("Los datos encontrados son los siguientes pid: %d\n",datos_resize->PID);
				printf("Los datos encontrados son los siguientes path: %s\n",datos_resize->path);

				printf("Se busca hacer el siguiente resize %d\n",proceso_resize->path_length);

				printf("Voy a asignarle marcos al proceso\n");
				//veamos el tamaño actual de la lista del proceso
				//dejamos especificado en el path length el tamaño de resize que queremos
				
				//cuantas paginas necesita? 
				int paginas_requeridas = dividir_y_redondear_arriba(proceso_resize->path_length,TAM_PAGINA);
				int tam_actual_tabla = list_size(datos_resize->TablaDePaginas);
				if( tam_actual_tabla < paginas_requeridas )
				{
					if( cantiMarcosLibresMemoria(listMarcos) >= paginas_requeridas )
					{
					//Asignar marcos
						int i = 0;
						while( i < list_size(listMarcos) && paginas_requeridas != 0 )
						{
							//aquellos elementos que sean -1 se asignan hasta acabar con la cantidad de resizes pedidos
							int* marco = list_get(listMarcos,i);
							if( *marco == -1 )
							{
								//reemplazo dicha casilla con el pid del proceso
								uint32_t* pid = malloc(sizeof(u_int32_t));
								*pid = proceso_resize->PID;
								void* replaced = list_replace(listMarcos,i,pid);
								free(replaced);
								paginas_requeridas--;
							}
							i++; 
							//free(pid);
						}
						
						printf("LLEGUE HASTA ACA EN LA PARTE DE ASIGNAR MARCOS1");
						
						printf("Pude asignarle, la lista de marcos quedó así:\n");
						int j = 0;
						while( j < list_size(listMarcos) )
						{
							int* number = list_get(listMarcos,j);
							if (number != NULL) 
							{
								printf("El marco numero %d\n",j);
								printf("Esta asignado al proceso %d\n",*number);
							} 
							else 
							{
								printf("El marco numero %d no está asignado a ningún proceso",j);
							}
							j++;
							printf("-----------------\n");					
						}
						printf("LLEGUE HASTA ACA EN LA PARTE DE ASIGNAR MARCOS2\n");
						for(int k = 0; k < list_size(listMarcos); k++)
						{
							int * marco = list_get(listMarcos,k);
							if( *marco == proceso_resize->PID )
							{
								//añado el marco a la tabla de paginas del proceso
								int* to_add = malloc(sizeof(int));
								*to_add = k;
								list_add(datos_resize->TablaDePaginas,to_add);
							}
						}
						//free(to_add);
						int tamanio_ampliado = list_size(datos_resize->TablaDePaginas);
						log_info (memoria_logs_obligatorios,  "PID: <%d> - Tamaño Actual: <%d> - Tamaño a Ampliar: <%d>", proceso_resize->PID , tam_actual_tabla, tamanio_ampliado);

						printf("LLEGUE HASTA ACA EN LA PARTE DE ASIGNAR MARCOS3\n");
						//pude asignar los marcos, la lista del proceso quedó así:
						int l = 0;
						while( l < list_size(datos_resize->TablaDePaginas) )
						{
							int* number = list_get(datos_resize->TablaDePaginas,l);
							if (number != NULL) 
							{
								printf("la pagina numero %d\n",l);
								printf("Esta asignado al marco %d\n",*number);
							} 
							else 
							{
								printf("explosion\n");
							}
							l++;
							printf("-----------------\n");					
						}

						//EXITOSA ASIGNACION
						int hola = 20;
						char* to_send = malloc(hola);
						strncpy(to_send, "vegeta", hola - 1);
						to_send[hola - 1] = '\0';
						usleep(RETARDO_RESPUESTA*1000);
						enviar_mensaje_cpu_memoria(instruccion,fd_cpu,ASIGNACION_CORRECTA);
						free(to_send);
					}
					else
					{
						//OUT of memory
						int hola = 20;
						char* to_send = malloc(hola);
						strncpy(to_send, "vegeta", hola - 1);
						to_send[hola - 1] = '\0';
						usleep(RETARDO_RESPUESTA*1000);
						enviar_mensaje_cpu_memoria(instruccion,fd_cpu,OUT_OF_MEMORY);
						free(to_send);
					}
				}
				else
				{
					//reducir
					//Ahora el ser menor la cantidad de paginas debo ir quitando paginas de la tabla y liberando el marco
					int i = list_size(datos_resize->TablaDePaginas)-1;
					while( i+1 != paginas_requeridas )
					{
						//me muevo al ultimo elemento de la tabla de paginas y obtengo el marco asignado
						int* marco_asignado = list_get(datos_resize->TablaDePaginas, i);
						//libero el marco
						int* liberado = malloc(sizeof(int));
						*liberado = (-1);
						list_replace(listMarcos,*marco_asignado,liberado);

						//saco el elemento de la tabla
						list_remove_and_destroy_element(datos_resize->TablaDePaginas,i,free);
						i--;
					}

					printf("Pude asignarle, la lista de marcos quedó así:\n");
					int j = 0;
					while( j < list_size(listMarcos) )
					{
						int* number = list_get(listMarcos,j);
						if (number != NULL) 
						{
							printf("El marco numero %d\n",j);
							printf("Esta asignado al proceso %d\n",*number);
						} 
						else 
						{
							printf("El marco numero %d no está asignado a ningún proceso",j);
						}
						j++;
						printf("-----------------\n");					
					}

					//pude asignar los marcos, la lista del proceso quedó así:
					int l = 0;
					while( l < list_size(datos_resize->TablaDePaginas) )
					{
						int* number = list_get(datos_resize->TablaDePaginas,l);
						if (number != NULL) 
						{
							printf("la pagina numero %d\n",l);
							printf("Esta asignado al marco %d\n",*number);
						} 
						else 
						{
							printf("explosion\n");
						}
						l++;
						printf("-----------------\n");					
					}
					int tamanio_reducido = list_size(datos_resize->TablaDePaginas);
					log_info (memoria_logs_obligatorios,  "PID: <%d> - Tamaño Actual: <%d> - Tamaño a Reducir: <%d>", proceso_resize->PID , tam_actual_tabla, tamanio_reducido);


					//EXITOSA ASIGNACION, o exitosa reduccion
					int hola = 20;
					char* to_send = malloc(hola);
					strncpy(to_send, "vegeta", hola - 1);
					to_send[hola - 1] = '\0';
					usleep(RETARDO_RESPUESTA*1000);
					enviar_mensaje_cpu_memoria(instruccion,fd_cpu,ASIGNACION_CORRECTA);
					free(to_send);
				}
				free(proceso_resize);
				printf("EL TAMANIO DE LA LISTA AL MOMENTO DE HACER RESIZE QUEDA DE LA SIGUIENTE FORMA: %d\n",datos_resize->TablaDePaginas);
				break;
				
			case LECTURA:
				void* copy_stream = paquete->buffer->stream;
				printf("He recibido un pedido de LECTURA\n");

				int* direccionFisica = malloc(sizeof(int));
				int* tamanioDato = malloc(sizeof(int)); // puede que sea int*
				int* pid_lectura = malloc(sizeof(int));
				memcpy(direccionFisica, copy_stream, sizeof(int));
				copy_stream += sizeof(int);
				memcpy(tamanioDato, copy_stream, sizeof(int));
				copy_stream += sizeof(int);
				memcpy(pid_lectura, copy_stream, sizeof(int));

				printf("Recibimos la direccion fisica: %d\n", *direccionFisica);
				printf("Recibimos el tamaño del dato: %d\n", *tamanioDato);
				printf("Recibimos el PID: %d\n", *pid_lectura);

				// OBTENER LO ALMACENADO EN LA DIRECCION FISICA
				void* datoObtenido = malloc(*tamanioDato);
				memcpy(datoObtenido, espacio_usuario + *direccionFisica, *tamanioDato);
				log_info (memoria_logs_obligatorios,  "PID: <%d> - Accion: <LEER> - Dirección fisica: <%d> - Tamaño <%d>", *pid_lectura, *direccionFisica, *tamanioDato);
				printf("Enviaremos %d\n", *(uint8_t*)datoObtenido);
				uint8_t* to_enviar = malloc(sizeof(uint8_t));
				*to_enviar = *(uint8_t*)datoObtenido;
				usleep(RETARDO_RESPUESTA*1000);
				enviarUint8(to_enviar, fd_cpu, LECTURA);

				// liberar memoria
				free(to_enviar);
				free(direccionFisica);
				free(pid_lectura);
				free(tamanioDato);
				free(datoObtenido);

   				break;

			case ESCRITURA_NUMERICO:

				void* copy_stream_e_n = paquete->buffer->stream;
				printf("He recibido un pedido de ESCRITURA_NUMERICO\n");
				int* direccionFisica_e_n = malloc(sizeof(int));
				uint8_t* valor = malloc(sizeof(uint8_t));
				int* pid_e_n = malloc(sizeof(int));
				memcpy(direccionFisica_e_n,copy_stream_e_n,sizeof(int));
				copy_stream_e_n += sizeof(int);
				memcpy(valor,copy_stream_e_n,sizeof(uint8_t));
				copy_stream_e_n += sizeof(uint8_t);
				memcpy(pid_e_n,copy_stream_e_n,sizeof(int));
				copy_stream_e_n += sizeof(int);

				printf("Recibimos la siguiente direccion: %d\n",*direccionFisica_e_n);//BX
				printf("Recibimos el siguiente valor a escribir: %d\n",*valor);//AX
				printf("Recibimos el siguiente PID: %d\n",*pid_e_n);
				
				memmove(espacio_usuario + *direccionFisica_e_n, valor, sizeof(uint8_t));

				void* to_show_n = malloc(sizeof(uint8_t));
				memcpy(to_show_n,espacio_usuario + *direccionFisica_e_n, sizeof(uint8_t));

				uint8_t* show_n = (uint8_t*)to_show_n;
				log_info (memoria_logs_obligatorios,  "PID: <%d> - Accion: <ESCRIBIR> - Dirección fisica: <%d> - Tamaño <%d>", *pid_e_n, *direccionFisica_e_n, sizeof(*valor));
				printf("Mostrar lo escrito %u\n",*show_n);

				int hola = 20;
				char* to_send = malloc(hola);
				strncpy(to_send, "vegeta", hola - 1);//mensaje aleatorio
				to_send[hola - 1] = '\0';
				usleep(RETARDO_RESPUESTA*1000);
				enviar_mensaje_cpu_memoria(to_send,fd_cpu,ESCRITO);
				free(to_send);
				free(valor);
				free(pid_e_n);
				free(direccionFisica_e_n);
				free(to_show_n);
				break;
			case COPY_STRING:
				//
				int* df_origen = paquete->buffer->stream;
				int* df_destino = paquete->buffer->stream + sizeof(int);
				int* pid_c_s = paquete->buffer->stream + sizeof(int);

				printf("Ha llegado la siguiente dirección origen: %d\n",*df_origen);
				printf("Ha llegado la siguiente dirección destino: %d\n", *df_destino);	
				printf("Ha llegado la siguiente PID: %d\n", *pid_c_s);	

				memmove(espacio_usuario + *df_destino, espacio_usuario + *df_origen,1);
				log_info (memoria_logs_obligatorios,  "PID: <%d> - Accion: <LEER> - Dirección fisica: <%d> - Tamaño <%d>", *pid_c_s, *df_origen, sizeof(uint8_t));
				log_info (memoria_logs_obligatorios,  "PID: <%d> - Accion: <ESCRIBIR> - Dirección fisica: <%d> - Tamaño <%d>", *pid_c_s, *df_destino, sizeof(u_int8_t));

				int* random = malloc(sizeof(int));
				usleep(RETARDO_RESPUESTA*1000);
				enviarEntero(random,fd_cpu,COPY_STRING);
				free(random);
				break;
			case PAQUETE:
				//
				break;
			case MARCO:
			/*
				//llega de cpu numero_pagina y pid del proceso
				void* copy_stream_m = paquete->buffer->stream;
				printf("He recibido un pedido de MARCO\n");
				int* numero_pagina = malloc(sizeof(int));
				uint32_t* pid = malloc(sizeof(uint32_t));	
				memcpy(numero_pagina,copy_stream_m,sizeof(int));
				copy_stream_m += sizeof(int);
				memcpy(pid,copy_stream_m,sizeof(uint32_t));

				printf("Recibimos numero de pagina: %d\n",*numero_pagina);
				printf("Recibimos el pid: %d\n",*pid);

				ProcesoMemoria* proceso_encontrado;
				for(int i=0; i < list_size(listProcesos); i++)
				{
					ProcesoMemoria *proceso_actual = list_get(listProcesos,i);
					
					if(proceso_actual->PID == *pid)
					{
						proceso_encontrado = proceso_actual;
					}
				}
				int* marco = malloc(sizeof(int));
				printf("El proceso encontrado es: %d\n",proceso_encontrado->PID);
				printf("EL tamanio de la lista es %d\n",list_size(proceso_encontrado->TablaDePaginas));
				for(int i = 0; i < list_size(proceso_encontrado->TablaDePaginas);i++)
				{
					int* pag_proceso_encontrado = list_get(proceso_encontrado->TablaDePaginas,i);
					if(i == *numero_pagina)
					{
						//enviar el marco
						//int* marco = pag_proceso_encontrado;
						*marco = *pag_proceso_encontrado;
						printf("Numero de marco a enviar: %d\n", *marco);
						
						//enviarEntero(marco,fd_cpu,MARCO);
						i=list_size(proceso_encontrado->TablaDePaginas)+1;
					}
				}
				printf("Estoy a punto de enviar el marco\n");
				usleep(RETARDO_RESPUESTA*1000);
				enviarEntero(marco,fd_cpu,MARCO);
				free(marco);
				free(numero_pagina);		
				free(pid);	
			*/
			void* copy_stream_m = paquete->buffer->stream;
				printf("He recibido un pedido de MARCO\n");

				// No es necesario usar malloc aquí
				int numero_pagina;
				uint32_t pid;

				// Copiamos directamente los valores
				memcpy(&numero_pagina, copy_stream_m, sizeof(int));
				copy_stream_m += sizeof(int);
				memcpy(&pid, copy_stream_m, sizeof(uint32_t));

				printf("Recibimos numero de pagina: %d\n", numero_pagina);
				printf("Recibimos el pid: %d\n", pid);

				ProcesoMemoria* proceso_encontrado = NULL;
				for (int i = 0; i < list_size(listProcesos); i++) {
					ProcesoMemoria* proceso_actual = list_get(listProcesos, i);
					if (proceso_actual->PID == pid) {
						proceso_encontrado = proceso_actual;
						break;
					}
				}

				if (proceso_encontrado != NULL) {
					printf("El proceso encontrado es: %d\n", proceso_encontrado->PID);
					printf("El tamanio de la lista es %d\n", list_size(proceso_encontrado->TablaDePaginas));
					int* marco = NULL;
					for (int i = 0; i < list_size(proceso_encontrado->TablaDePaginas); i++) {
						int* pag_proceso_encontrado = list_get(proceso_encontrado->TablaDePaginas, i);
						if (i == numero_pagina) {
							marco = pag_proceso_encontrado;
							printf("Numero de marco a enviar: %d\n", *marco);
							break;
						}
					}
					log_info (memoria_logs_obligatorios,  "Acceso a Tabla de Páginas: PID: <%d> - Pagina: <%d> - Marco: <%d>", proceso_encontrado->PID , numero_pagina, *marco);

					if (marco != NULL) {
						printf("Estoy a punto de enviar el marco\n");
						usleep(RETARDO_RESPUESTA * 1000);
						enviarEntero(marco, fd_cpu, MARCO);
						// No liberar marco ya que no fue asignado dinámicamente en este contexto
					} else {
						printf("No se encontró el marco para la página %d del proceso %d\n", numero_pagina, pid);
					}
				} else {
					printf("No se encontró el proceso con PID: %d\n", pid);
				}			
				break;
			case -1:
				log_error(memoria_logger, "El cliente cpu se desconecto. Terminando servidor");
				control_key = 0;
			default:
				log_warning(memoria_logger,"Operacion desconocida. No quieras meter la pata");
				break;
			}
			//Liberando los paquetes solucionamos los errores presentados anteriormente
		
			free(paquete->buffer->stream);
			free(paquete->buffer);
			free(paquete);
		}
}

void enviar_texto_io(char* text, int fd_io, op_code codigo_operacion)
{
	int* tamanio = malloc(sizeof(int));
	*tamanio = strlen(text)+1;

	//Preparamos el buffer
	t_newBuffer* buffer = malloc(sizeof(t_newBuffer));
	buffer->offset = 0;
	buffer->size = sizeof(int) + *tamanio;
	buffer->stream = malloc(buffer->size);

	//vamos reservando la memoria
	memcpy(buffer->stream + buffer->offset,tamanio, sizeof(int));
    buffer->offset += sizeof(int);
	memcpy(buffer->stream + buffer->offset,text, *tamanio);

	t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
    //Podemos usar una constante por operación
    paquete->codigo_operacion = codigo_operacion;
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

    // No nos olvidamos de liberar la memoria que ya no usaremos
    free(a_enviar);
	free(tamanio);
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void memoria_escuchar_entradasalida_mult(int* fd_io){
	bool control_key = 1;
	while (control_key) {
			int cod_op = recibir_operacion(*fd_io);

			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
			recv(*fd_io,&(paquete->buffer->size),sizeof(uint32_t),0);	
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(*fd_io,paquete->buffer->stream, paquete->buffer->size,0);

			switch (cod_op) {
			case STDIN:

				EntradaSalida* new_io_stdin = deserializar_entrada_salida(paquete->buffer);
				printf("Llego una IO cuyo nombre es: %s\n",new_io_stdin->nombre);
		   		printf("Llego una IO cuyo path es: %s\n",new_io_stdin->path);
				
				list_add(listStdin,new_io_stdin);
				//
				break;
			case STDIN_TOWRITE:
				/*
				int* direccionFisicaIN = malloc(sizeof(int));
				direccionFisicaIN = paquete->buffer->stream;
				char* caracter_to_write = malloc(sizeof(char));
				caracter_to_write = paquete->buffer->stream + sizeof(int);

				if( *direccionFisicaIN == -1 )
				{
					int* random = malloc(sizeof(int));
					*random = 10;
					enviarEntero(random,*fd_io,DESPERTAR);
					free(random);
				}
				else
				{
					printf("--------EN LA DIRECCIÓN %d escribiremos el caracter %c\n",*direccionFisicaIN,*caracter_to_write);
					memmove(espacio_usuario + *direccionFisicaIN, caracter_to_write, 1);
					//usleep(atoi(RETARDO_RESPUESTA)*1000);
				}
				free(direccionFisicaIN);
				free(caracter_to_write);
				*/
				int* direccionFisicaIN = (int*) paquete->buffer->stream;
				char* caracter_to_write = (char*) (paquete->buffer->stream + sizeof(int));

				if (*direccionFisicaIN == -1) {
					int random = 10;
					enviarEntero(&random, *fd_io, DESPERTAR);
				} else {
					printf("*/*/*/*/ EN LA DIRECCIÓN %d escribiremos el caracter %c\n", *direccionFisicaIN, *caracter_to_write);
					memmove(espacio_usuario + *direccionFisicaIN, caracter_to_write, 1);
				}

				break;
			case STDOUT:

				EntradaSalida* new_io_stdout = deserializar_entrada_salida(paquete->buffer);
				printf("Llego una IO cuyo nombre es: %s\n",new_io_stdout->nombre);
		   		printf("Llego una IO cuyo path es: %s\n",new_io_stdout->path);
				
				list_add(listStdout,new_io_stdout);
				//
				break;
			case STDOUT_TOPRINT:

				int* tamanio_out = (int*)paquete->buffer->stream;
				printf("El tamanio es el siguiente %d\n", *tamanio_out);

				t_list* direcciones_fisicas = list_create();
				for (int i = 1; i < *(tamanio_out) + 1; i++) {
					list_add(direcciones_fisicas, paquete->buffer->stream + sizeof(int) * i);
				}

				for (int i = 0; i < list_size(direcciones_fisicas); i++) {
					int* df = list_get(direcciones_fisicas, i);
					printf("Las direcciones fisicas que he obtenido es: %d\n", *df);
				}

				// debemos leer las direcciones y concatenarlas
				char* text = (char*)malloc(*tamanio_out + 1); // +1 para el carácter nulo
				text[0] = '\0'; // inicializar la cadena

				for (int i = 0; i < *tamanio_out; i++) {
					int* df = list_get(direcciones_fisicas, i);
					char caracter = *(char*)(espacio_usuario + *df);
					text[i] = caracter; // asignar directamente el carácter
					printf("El caracter leído es: caracter\n");
				}
				text[*tamanio_out] = '\0'; // añadir el carácter nulo al final

				printf("El texto quedó de la siguiente forma: %s\n", text);
				
				enviar_texto_io(text,*fd_io,STDOUT_TOPRINT);

				// liberar memoria
				free(text);
				list_destroy(direcciones_fisicas);
				break;
			case DIALFS:

				EntradaSalida* new_io_dialfs = deserializar_entrada_salida(paquete->buffer);
				printf("Llego una IO cuyo nombre es: %s\n",new_io_dialfs->nombre);
		   		printf("Llego una IO cuyo path es: %s\n",new_io_dialfs->path);
				
				list_add(listStdin,new_io_dialfs);
				//
				break;
			case IO_FS_WRITE:
				//
				int* tamanio_out_FS = (int*)paquete->buffer->stream;
				printf("El tamanio es el siguiente %d\n", *tamanio_out_FS);

				t_list* direcciones_fisicas_FS = list_create();
				for (int i = 1; i < *(tamanio_out_FS) + 1; i++) {
					list_add(direcciones_fisicas_FS, paquete->buffer->stream + sizeof(int) * i);
				}

				for (int i = 0; i < list_size(direcciones_fisicas_FS); i++) {
					int* df_FS = list_get(direcciones_fisicas_FS, i);
					printf("Las direcciones fisicas que he obtenido es: %d\n", *df_FS);
				}

				char* text_FS = (char*)malloc(*tamanio_out_FS + 1); // +1 para el carácter nulo
				text_FS[0] = '\0'; // inicializar la cadena

				for (int i = 0; i < *tamanio_out_FS; i++) {
					int* df_FS = list_get(direcciones_fisicas_FS, i);
					char caracter = *(char*)(espacio_usuario + *df_FS);
					text_FS[i] = caracter; // asignar directamente el carácter
				}
				text_FS[*tamanio_out_FS] = '\0'; // añadir el carácter nulo al final

				printf("El texto quedó de la siguiente forma: %s\n", text_FS);

				enviar_texto_io(text_FS, *fd_io,IO_FS_WRITE);

				// liberar memoria
				free(text_FS);
				list_destroy(direcciones_fisicas_FS);
				break;
			case MENSAJE:
				//
				break;
			case PAQUETE:
				//
				break;
			case -1:
				log_error(memoria_logger, "El cliente EntradaSalida se desconecto. Terminando servidor\n");
				control_key = 0;
			default:
				log_warning(memoria_logger,"Operacion desconocida. No quieras meter la pata\n");
				break;
			}
			free(paquete->buffer->stream);
    	    free(paquete->buffer);
	        free(paquete);
		}
}


void escuchar_io()
{
	printf("Hola, estoy acá\n");
	while (1) 
	{
		pthread_t thread;
		int *fd_conexion_ptr = malloc(sizeof(int));
		*fd_conexion_ptr = accept(fd_memoria, NULL, NULL);
		printf("(1)Se ha conectado un cliente de tipo IO\n");
		handshakeServer(*fd_conexion_ptr);
		printf("Se ha conectado un cliente de tipo IO\n");
		pthread_create(&thread, NULL, (void*) memoria_escuchar_entradasalida_mult, fd_conexion_ptr);
		pthread_detach(thread);
	}
}


void iterator(char* value) 
{
	log_info(memoria_logger,"%s", value);
}



char* leerArchivo(FILE* file)
{
	fseek(file,0,SEEK_END); //a veces falla

	int tamanioArchivo = ftell(file);

	rewind(file);

	char* contenido = malloc((tamanioArchivo + 1) * sizeof(char) );  
	if( contenido == NULL )
	{
		printf("Error al intentar reservar memoria\n");
		return NULL;
	}

	size_t leidos = fread(contenido, sizeof(char), tamanioArchivo, file);
	if( leidos < tamanioArchivo )
	{
		printf("No se pudo leer el contenido del archivo\n");
		free(contenido);
		return NULL;
	}

	contenido[tamanioArchivo] = '\0';

	return contenido;
}

int calcularCantidadDeInstrucciones(char* content)
{
	int i = 0;
	int cantInstrucciones = 0;
	while( i < strlen(content) )
	{
		if( content[i] == '\n' )
		{
			cantInstrucciones++;
		}
		i++;
	}
	return cantInstrucciones+1;
}

char* abrir_archivo(char* path, int PC)
{
	FILE* file = fopen(path,"r");
	if( file == NULL )
	{
		printf("Error al abrir archivo, sorry\n");
	}
	char* content = leerArchivo(file);
	char** newContent = string_split(content,"\n");

	int cantInstrucciones = calcularCantidadDeInstrucciones(content);
	printf("La cantidad de instrucciones que tiene es: %d\n", cantInstrucciones);

	char* to_ret;

	if( PC < cantInstrucciones )
	{
		to_ret = malloc(strlen(newContent[PC])+1);
		//to_ret = newContent[PC];
		if (to_ret == NULL) {
        printf("Error al asignar memoria para to_ret\n");
		free(content);
		string_array_destroy(newContent);
		fclose(file);
        // Manejar el error apropiadamente
    	}
    	
		strcpy(to_ret, newContent[PC]); // Copiar el contenido de newContent[PC] a to_ret
	}
	else
	{
		to_ret = strdup("");
		if (to_ret == NULL){
			free(content);
			string_array_destroy(newContent);
			fclose(file);
			return NULL;
		}
		
	}

    free(content);
	string_array_destroy(newContent);
	fclose(file);
	return to_ret;
}

void memoria_escuchar_kernel (){
	bool control_key = 1;
			
	//t_list* lista;
	while (control_key) {
			int cod_op = recibir_operacion(fd_kernel);

			//debemos extraer el resto, primero el tamaño y luego el contenido
			t_newPaquete* paquete = malloc(sizeof(t_newPaquete));
			paquete->buffer = malloc(sizeof(t_newBuffer));
			recv(fd_kernel,&(paquete->buffer->size),sizeof(uint32_t),0);			
			paquete->buffer->stream = malloc(paquete->buffer->size);
			recv(fd_kernel,paquete->buffer->stream, paquete->buffer->size,0);

			switch (cod_op) {
			case MENSAJE:
				//
					printf("Hemos recibido un mensaje del kernel\n");
					break;
			case PROCESOFIN:
			//recibimos la instruccion de finalizar un proceso
		            printf("Hemos recibido la orden de eliminar un proceso que ha finalizado\n");
					PCB* proceso = deserializar_proceso_cpu(paquete->buffer);
					
					printf("Borraremos el proceso con pid: %d\n",proceso->PID);
					//hay que liberar los marcos
					int i = 0;
					while( i < list_size(listMarcos) )
					{
						int* got = list_get(listMarcos,i);
						if( *got == proceso->PID )
						{
							int* to_add = malloc(sizeof(int));
							*to_add = (-1);
							void* replaced = list_replace(listMarcos,i,to_add);
							free(replaced);
						}
						i++;
					}

					printf("Pude asignarle, la lista de marcos quedó así:\n");
					int j = 0;
					while( j < list_size(listMarcos) )
					{
						int* number = list_get(listMarcos,j);
						if (number != NULL) 
						{
							printf("El marco numero %d\n",j);
							printf("Esta asignado al proceso %d\n",*number);
						} 
						else 
						{
							printf("El marco numero %d no está asignado a ningún proceso",j);
						}
						j++;
						printf("-----------------\n");					
					}
					
					sem_wait(&protect_list_procesos);
					ProcesoMemoria* dato = encontrarProceso(listProcesos,proceso->PID);
					sem_post(&protect_list_procesos);
					printf("Borraremos [CONFIRMADO] el proceso con pid: %d\n", dato->PID);
					log_info (memoria_logs_obligatorios,  "Destruccion de Tabla de Paginas: PID: <%d> - Tamaño: <%d>", dato->PID , list_size(dato->TablaDePaginas));
					list_destroy_and_destroy_elements(dato->TablaDePaginas,free);
					free(dato->path);
					free(dato);
					//free(proceso->path);
					free(proceso);
					printf("Proceso borrado con exito\n");
					usleep(RETARDO_RESPUESTA*1000);
					break;
			case PAQUETE:
				ProcesoMemoria* nuevoProceso = deserializar_proceso_memoria(paquete->buffer);
				nuevoProceso->TablaDePaginas = list_create();//creamos la lista de páginas
				if(nuevoProceso != NULL){ 
					//añadimos el proceso a la lista de procesos que posee la memoria, además le creamos un array con su tabla de páginas
					//printf("Me voy a mimir, buenas noches\n");
					//sleep(RETARDO_RESPUESTA); INCLUIR EL SLEEP ACA HARIA QUE CAMBIASEMOS GRAN PARTE DE LA SINCRONIZACION ENTRE MEMORIA Y KERNEL EN LA CREACION DE PROCESOS
					sem_wait(&protect_list_procesos);
					list_add(listProcesos, nuevoProceso);
					sem_post(&protect_list_procesos);
					printf("El PID que recibi es: %d\n", nuevoProceso->PID);
					printf("El PATH que recibi es: %s\n", nuevoProceso->path);
					log_info (memoria_logs_obligatorios,  "Creacion: PID: <%d> - Tamaño: <%d>", nuevoProceso->PID , list_size(nuevoProceso->TablaDePaginas));

				} else{
					printf("No se pudo deserializar\n");
				}
			break;
			case -1:
				log_error(memoria_logger, "El cliente kernel se desconecto. Terminando servidor\n");
				control_key = 0;

			default:
				log_warning(memoria_logger,"Operacion desconocida. No quieras meter la pata\n");
				break;
			}
			//liberar memoria
			free(paquete->buffer->stream);
			free(paquete->buffer);
			free(paquete);
			//cod_op = 0;
		}

}




#endif /* MEMORIA_H_ */