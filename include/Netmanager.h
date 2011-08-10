typedef char t_buff[255];
typedef enum { false, true } bool;

int send_HTTP_line( int , unsigned char * );
int recv_HTTP_line( int , unsigned char * );
int ssend_SOCK_line( int , unsigned char * );
int send_SOCK_line( int , unsigned char *, ... );
int send_POST_rqst(int , unsigned char *, unsigned char *, unsigned char *);
int send_GET_rqst(int , unsigned char *, unsigned char *, unsigned char *);
char *is_HTTP(const unsigned char *);
bool is_GET(const unsigned char *);
bool is_POST(const unsigned char *);
bool is_HEAD(const unsigned char *);
char *DNS_lookup(const unsigned char *);
