#include <QTextStream>
#include <QString>

#include "my_thread.hpp"
#include "window.hpp"
#include "function.hpp"
#include "3d.hpp"

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condvar_tomf = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condvar_totf = PTHREAD_COND_INITIALIZER;

CalculatingThread::CalculatingThread(DrawArea *area, bool firstRun)
: parentDrawArea (area)
, firstRun (firstRun)
{}

void CalculatingThread::run()
{
  if (!firstRun) {
    delete[] parentDrawArea->alphas;
    delete[] parentDrawArea->locnzc;
    delete[] parentDrawArea->values;
  }

  MyMainWindow *parentWindow = static_cast<MyMainWindow *> (parentDrawArea->parentWidget());

  parentDrawArea->calcSegments = parentWindow->calcLayersSpinBox.value();
  parentDrawArea->drawSegments = parentWindow->drawLayersSpinBox.value();
  parentDrawArea->threads = parentWindow->threadsSpinBox.value();

  quint32 calcPoints = (parentDrawArea->calcSegments + 1) * (parentDrawArea->calcSegments + 1);
  quint32 drawPoints = (parentDrawArea->drawSegments + 1) * (parentDrawArea->drawSegments + 1);

  parentDrawArea->alphas = new qreal[calcPoints];
  parentDrawArea->locnzc = new quint32[calcPoints];
  parentDrawArea->values = new qreal[drawPoints];

  pthread_t *thr = new pthread_t[parentDrawArea->threads];
  Args *args = new Args[parentDrawArea->threads];
  quint32 donethreads(0);
  quint32 nzCount(0);
  pthread_mutex_lock(&mutex);
  for (quint32 i = 0; i < parentDrawArea->threads; ++i) {
    args[i].mutex = &mutex;
    args[i].condvar_tomf = &condvar_tomf;
    args[i].condvar_totf = &condvar_totf;
    args[i].threads = parentDrawArea->threads;
    args[i].thread = i;
    args[i].donethreads = &donethreads;
    args[i].points = parentDrawArea->points;
    args[i].calcSegments = parentDrawArea->calcSegments;
    args[i].drawSegments = parentDrawArea->drawSegments;
    args[i].locnzc = parentDrawArea->locnzc;
    args[i].alphas = parentDrawArea->alphas;
    args[i].values = parentDrawArea->values;
    args[i].task = GetNzCount;
    pthread_create(&(thr[i]), NULL, &tf, &(args[i]));
  }
  // let the threads initialize
  pthread_cond_wait(&condvar_tomf, &mutex);

  donethreads = 0;
  QTextStream(stdout) << "Getting non-zero count: ...";
  QTextStream(stdout).flush();
  pthread_cond_broadcast(&condvar_totf);
  pthread_cond_wait(&condvar_tomf, &mutex);
  nzCount = 0;
  for (quint32 i = 0; i < calcPoints; ++i)
    nzCount += parentDrawArea->locnzc[i];
  QTextStream(stdout) << "\b\b\b" << nzCount << endl;
  QTextStream(stdout) << "Start filling matrix" << endl;
  QTextStream(stdout).flush();
  MsrMatrix *matrix = new MsrMatrix(calcPoints, nzCount);
  QTextStream(stdout) << "Matrix initialization success!" << endl;
  QTextStream(stdout).flush();
  for (quint32 i = 0; i < parentDrawArea->threads; ++i) {
    args[i].matrix = matrix;
    args[i].task = FillAndSolveMatrix;
  }

  donethreads = 0;
  pthread_cond_broadcast(&condvar_totf);
  pthread_cond_wait(&condvar_tomf, &mutex);
  
  QTextStream(stdout) << "Filling completed" << endl;
  QTextStream(stdout) << "Solving system: ...";
  QTextStream(stdout).flush();

  quint32 iterations = matrix->solve(parentDrawArea->alphas);
  QTextStream(stdout) << "\b\b\bsolved in " << iterations
    << " iterations" << endl;
  delete matrix;

  donethreads = 0;
  for (quint32 i = 0; i < parentDrawArea->threads; ++i)
    args[i].task = FillValues;
  QTextStream(stdout) << "Filling values array: ...";
  QTextStream(stdout).flush();
  pthread_cond_broadcast(&condvar_totf);
  pthread_cond_wait(&condvar_tomf, &mutex);
  
  QTextStream(stdout) << "\b\b\bdone" << endl;
  for (quint32 i = 0; i < parentDrawArea->threads; ++i)
    args[i].task = FinishWork;
  pthread_cond_broadcast(&condvar_totf);
  pthread_mutex_unlock(&mutex);
  for (quint32 i = 0; i < parentDrawArea->threads; ++i)
    pthread_join(thr[i], NULL);

  // calculate residual

  qreal residual (0), 
        value (0), 
        phi (0), 
        local_residual(0);
  QPointF point;
  const quint32 residualSegments (parentDrawArea->calcSegments + 1);
  for (quint32 i = 0; i <= residualSegments; ++i) {
    for(quint32 j = 0; j <= residualSegments; ++j) {
      value = 0.;
      point = getPointByMeshCoordinates (parentDrawArea->points, residualSegments, i, j);
      for (quint32 k = 0; k < calcPoints; ++k) {
        phi = phiFunctionGetByVertexIndex (parentDrawArea->points, parentDrawArea->calcSegments, point, k);
        value += parentDrawArea->alphas[k] * phi;
      }
      local_residual = qAbs(value - function(point.x(), point.y()));
      residual = qMax(local_residual, residual);
    }
  }

  QTextStream(stdout) << "Emitting signal: ...";
  emit resultReady(residual);
  return;
}