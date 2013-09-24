#include <pthread.h>
#include <QtGlobal>
#include "3d.hpp"

enum Task {
	FinishWork         = 0,
	GetNzCount         = 1,
	FillAndSolveMatrix = 2,
	FillValues         = 4
};

struct Args {
	Task task;
	pthread_mutex_t *mutex;
	pthread_cond_t *condvar_tomf;
	pthread_cond_t *condvar_totf;
	quint32 thread;
	quint32 threads;
	quint32 *donethreads;
	MsrMatrix *matrix;
	QPointF *points;
	quint32 calcSegments;
	quint32 drawSegments;
	quint32 *locnzc;
	qreal *alphas;
	qreal *values;
};

void *tf(void *args);
