#ifndef THREAD_HPP
#define THREAD_HPP

#include <QThread>
#include "window.hpp"

class CalculatingThread : public QThread
{
  Q_OBJECT

public:
  CalculatingThread(DrawArea *parentArea, bool firstRun);
  void stop();
signals:
  void resultReady(qreal residual);
protected:
  void run();
private:
  DrawArea *parentDrawArea;
  volatile bool stopped;
  bool firstRun;
  friend class DrawArea;
};

#endif