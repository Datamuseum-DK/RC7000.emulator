#include <sys/queue.h>

struct process;

typedef void sendmsg_f(struct process *, uint16_t msg);

struct process {
	uint8_t				name[6];
	TAILQ_ENTRY(process)		list;
	sendmsg_f			*sendm;
	void				*priv;
};

struct process *new_proc(const char *name, sendmsg_f *sendm);

extern const uint8_t domac_file[];
extern const int domac_size;
extern const uint8_t domxr_file[];
extern const int domxr_size;
