#define SUCCESS 0
// MIN VALUES
#define MIN_THREADS 1
#define MIN_WAIT    1
// MAX VALUES        
#define MAX_THREADS 256
#define MAX_QUEUE   256
#define PATH_MAX    4096
#define MAX_WAIT    60000
// ERRORS
#define ERROR              -1  // general error
#define INVALID_MODEL_TYPE -2  // config.c
#define INVALID_QUEUE_TYPE -3  // config.c
#define THREAD_NUM_OOB     -4  // config.c - Thread Request Out Of Bounds
#define QUEUE_CAP_OOB      -5  // config.c - Queue Request Out Of Bounds
#define PORT_RANGE_OOB     -6  // config.c - Port Range Out Of Bounds
#define ALPHA_VAL_OOB      -7  // config.c - Alpha Value Out Of Bounds
#define WEIGHT_VAL_OOB     -8  // config.c - Weight (w) Value Out Of Bounds
#define SEED_VAL_OOB       -9  // config.c - Seed Value Out Of Bounds
#define RCV_WAIT_OOB       -10 // config.c - Receive Wait Time Out Of Bounds
#define SND_WAIT_OOB       -11 // config.c - Send Wait Time Out Of Bounds
#define LOGPATH_TOO_LONG   -12 // config.c - Log Path Too Long