/*Questa libreria e' stata pensata per poter offrire il miglior supporto per l'utilizzo e la manutenzione di servizi che si interfacciano con
 * il protocollo HTTP.
 * Offre in aggiunta funzioni per la gestione basso-livello della scrittura e lettura delle informazioni sui socket.
 * 
 * Federico Gerardi (aka Darkness-hack) <2011>*/
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <Netmanager.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <netdb.h>
#include <stdio.h>

#define EOH "\r\n"
#define EOH_SIZE 2
#define MAX_HTTP_RQST 4096
#define HTTP_RQST_VERSION "1.1"

#define GEN_ERR -1 //General error (Undefined fatal error)
#define PAR_ERR -2 //Partial error (Partial execution of the program flow)
#define SEG_ERR -3 //Segmentation error (Error in a segment of the memory or allocation error)
#define OOR_ERR -4 //Out of range error (Error in cicles or similar situations, in which you could incur in some bof)

#define _size_t(x) (sizeof(x) + 255)
#define _stoh_n(x) (x"\r\n")
#define _stoh_i(x) (x"\r\n\r\n")
#define _stoh_g(x,y) (x#y)

 /*Questa funzione e' studiata per mandare una singola riga HTTP. Cio' significa che il sending si blocchera' appena incontrato il token '\r\n'
 * Ritorna gli errori sopra definiti*/
int send_HTTP_line( int sockfd, unsigned char *buffer )
{
	int sent_bytes = 0;
	unsigned short int found = 0, i = 0, s = 0;
	unsigned char *ptr = buffer, *final = 0;
	
	if((final = (char *) malloc(sizeof(buffer))) == NULL)
	{
		return SEG_ERR;
	}

	for(; i < strlen(buffer); i++)
	{
		if(buffer[i] == '\r')
		{
			if(buffer[++i] == '\n')
			{
				final[s] = '\0';
				break;
			}
			else
			{
				final[s] = buffer[i];
				s++;
			}
		}
		else
		{
			final[s] = buffer[i];
			s++;
		}
	}

	#ifdef DEBUG
		printf("[!!]%s: %p", final, final);
		return 0;
	#endif
	
	#ifndef DEBUG
	if( (sent_bytes = send(sockfd, final, strlen(final), 0) ) < 0)
	{
		free(final);
		return GEN_ERR;
	}
	else if( sent_bytes < strlen(final) )
	{	
		free(final);
		return PAR_ERR;
	}
	else
	{
		free(final);
		return 0;
	}
	#endif
}

/*Questa funzione riceve byte per byte i dati dal socket passato come argomento e li inserisce in un buffer, fino al token '\r\n'*/
/*In caso di fallimento ritorna uno dei valori sopra definiti, altrimenti la lunghezza della stringa ricevuta*/
int recv_HTTP_line( int sockfd, unsigned char *buffer )
{
	unsigned char *ptr = 0;
	unsigned int eoh_seek = 0;

	ptr = buffer;

	while(recv(sockfd, ptr, 1, 0) > 0)
	{
		if( *ptr == EOH[eoh_seek] )
		{
			eoh_seek++;
			if( eoh_seek == EOH_SIZE )
			{
				*(--ptr) = '\0';
				return strlen(buffer);
			}
		}
		else
		{
			eoh_seek = 0;
		}

		ptr++;
	}

	return GEN_ERR;
}

/*Questa funzione si occupa dell'invio "necessario" di dati, in quanto effettua un controllo sui dati inviati, confrontandoli con quelli
 * che eventualmente non sono stati inviati e ne riprova l'invio. Purtroppo attualmente questa funzione non e' ottimizzata per la formattazione
 * della stringa stessa.
 * Ritorna uno dei valori di errore sopra definiti*/
int ssend_SOCK_line( int sockfd, unsigned char *string )
{
	int sent_bytes = 0, to_send_bytes = strlen(string);

	for(; to_send_bytes > 0;)
	{
		#ifdef DEBUG
			printf("[!!]%s", string);
		#endif

		if((sent_bytes = send(sockfd, string, to_send_bytes, 0)) < 0)
		{
			return GEN_ERR;
		}
		else
		{
			to_send_bytes -= sent_bytes;
			string += sent_bytes;
		}
	}

	return 0;
}

/*Questa funzione esegue l'invio di una stringa qualsiasi, permettendone la formattazione dell'output (printf-like).
 * I dati utilizzabili sono di tipo stringa, char, ed int.
 * Ritorna uno dei valori di errore sopra definiti*/
int send_SOCK_line( int sockfd, unsigned char *format, ... )
{
	va_list args;
	char *ptr = format, *buffer = 0, c_container, *f_buffer = 0;
	t_buff aux = {0};
	int container = 0, sent_bytes = 0;

	va_start(args, format);
	
	if((buffer = (char *)  malloc(_size_t(format))) == NULL)
	{
		return SEG_ERR;
	}

	if((f_buffer = (char *) malloc(_size_t(format))) == NULL)
	{
		free(buffer);
		return SEG_ERR;
	}

	for( ; *ptr; ptr++ )
	{
		if(*ptr == '%')
		{
			ptr++;
				
			switch(*ptr)
			{
				case 'i':
				case 'd':
					container = va_arg(args, int);

					#ifdef DEBUG
						printf(	"[!!]container = %i\n", container);
					#endif
					
					if( snprintf( f_buffer, _size_t(format), "%s%i", buffer, container ) < 0 )
					{
						free(buffer);
						free(f_buffer);
						return OOR_ERR;
					}
					
					break;

				case 'c':
					c_container = (char) va_arg(args, int);
					
					if( snprintf( f_buffer, _size_t(format), "%s%c", buffer, container ) < 0 )
					{
						free(buffer);
						free(f_buffer);
						return OOR_ERR;
					}
					
					break;

				case 's':
					if(strncpy(aux, va_arg(args, char *), sizeof(aux)) == NULL)
					{
						free(buffer);
						free(f_buffer);
						return OOR_ERR;
					}
					
					if( snprintf( f_buffer, _size_t(format), "%s%s", buffer, aux ) < 0)
					{
						free(buffer);
						free(f_buffer);
						return OOR_ERR;
					}
					
					break;

				default:
					ptr--;
					if( snprintf( f_buffer, _size_t(format), "%s%c", buffer, *ptr ) < 0)
					{
						free(buffer);
						free(f_buffer);
						return OOR_ERR;
					}

					break;
			}

		}
		else
		{
			if( (sent_bytes = snprintf( f_buffer, _size_t(format), "%s%c", buffer, *ptr ) ) < 0)
			{
				free(buffer);
				free(f_buffer);
				return OOR_ERR;
			}

			#ifdef DEBUG
				printf(	"[!!]bytes ==> %d\n", sent_bytes );
			#endif
		}

		memset(aux, 0x0, sizeof(aux));
	
		if(snprintf(buffer, _size_t(format), "%s", f_buffer) < 0)
		{
			free(buffer);
			free(f_buffer);
			return OOR_ERR;
		}
	}

	va_end(args);

	#ifdef DEBUG
		printf("[!!]%s: %p\n", buffer, buffer);
		return 0;
	#endif
	#ifndef DEBUG
		if( ( sent_bytes = send( sockfd, buffer, strlen(buffer), 0 ) ) < 0)
		{
			free(buffer);
			free(f_buffer);
			return GEN_ERR;
		}
		else if( sent_bytes < strlen(buffer) )
		{
			free(buffer);
			free(f_buffer);
			return PAR_ERR;
		}
		else
		{
			free(buffer);
			free(f_buffer);
			return 0;
		}
	#endif
}

/* Questa funzione effettua una richiesta POST ad un determinato host.
 * Ritorna uno dei valori di errore sopra definiti*/
int send_POST_rqst(int sockfd, unsigned char *page, unsigned char *host, unsigned char *post_rqst)
{
	unsigned char rqst[MAX_HTTP_RQST] = {0};
	size_t sent_bytes = 0;
	
	if(snprintf(rqst, sizeof(rqst), "POST %s HTTP/%s\r\n"
					"Host: %s\r\n"
					"Content-type: application/x-www-form-urlencoded\r\n"
					"Content-length: %d\r\n\r\n"
					"%s", page, HTTP_RQST_VERSION, host, strlen(post_rqst), post_rqst) < 0)
	{
		return GEN_ERR;
	}
	else
	{
		#ifdef DEBUG
			printf("[!!]%s\n", rqst);
		#endif
		#ifndef DEBUG
			if((sent_bytes = send(sockfd, rqst, sizeof(rqst), 0)) < 0)
			{
				return GEN_ERR;
			}
			else if(sent_bytes < strlen(rqst))
			{
				return PAR_ERR;
			}
			else
			{
				return 0;
			}
		#endif
	}

}

/* Questa funzione effettua l'invio di una richiesta GET ad un determinato host.
 * Ritorna uno dei valori di errore sopra citati*/
int send_GET_rqst(int sockfd, unsigned char *page, unsigned char *host, unsigned char *get_rqst)
{
	unsigned char rqst[MAX_HTTP_RQST] = {0};
	size_t sent_bytes = 0;

	if(snprintf(rqst, sizeof(rqst), "GET %s%s HTTP/%s\r\n"
					"Host: %s\r\n\r\n", page, get_rqst, HTTP_RQST_VERSION, host) < 0)
	{
		return GEN_ERR;
	}
	else
	{
		#ifdef DEBUG
			printf("[!!]%s\n", rqst);
		#endif
		#ifndef DEBUG
			if((sent_bytes = send(sockfd, rqst, sizeof(rqst), 0)) < 0)
			{
				return GEN_ERR;
			}
			else if(sent_bytes < strlen(rqst))
			{
				return PAR_ERR;
			}
			else
			{
				return 0;
			}
		#endif
	}
}

/* Questa funzione verifica che il pacchetto passato come argomento, contenga il token "HTTP /"
 * Ritorna un puntatore al token in caso di successo, NULL altrimenti*/
char *is_HTTP(const unsigned char *packet)
{
	return strstr(packet, "HTTP/");
}

/* Questa funzione verifica se il pacchetto presenta una richiesta GET.
 * Ritorna true o false*/
bool is_GET(const unsigned char *packet)
{
	if(!strncmp(packet, "GET ", 4))
		return true;
	else
		return false;
}

/* Questa funzione verifica se il pacchetto presenta una richiesta POST.
 * Ritorna true o false*/
bool is_POST(const unsigned char *packet)
{
	if(!strncmp(packet, "POST ", 5))
		return true;
	else
		return false;
}

/* Questa funzione verifica se il pacchetto presenta una richiesta HEAD.
 * Ritorna true o false*/
bool is_HEAD(const unsigned char *packet)
{
	if(!strncmp(packet, "HEAD ", 5))
		return true;
	else
		return false;
}

/* Questa funzione effettua un semplice DNS lookup.
 * Ritorna un puntatore alla stringa contenente l'IP, altrimenti un puntatore a NULL*/
char *DNS_lookup(const unsigned char *host)
{
	struct hostent *host_info;

	if((host_info = (struct hostent *)gethostbyname(host)) == NULL)
	{
		return NULL;
	}
	else
	{
		#ifdef DEBUG
			printf("[!!]%s\n", inet_ntoa(*(struct in_addr *)(host_info->h_addr)));
		#endif
		return inet_ntoa(*(struct in_addr *)(host_info->h_addr));
	}
}

#ifdef DEBUG
int main(int argc, char **argv)
{
	return 0;
}
#endif
