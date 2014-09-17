#include <QFlags>
#include <QPointF>
#include <QMainWindow>
#include <QToolBar>
#include <QPushButton>
#include <QAction>
#include <QLabel>
#include <QSpinBox>
#include <QMouseEvent>
#include <QGLWidget>
#include <QStatusBar>
#include <qmath.h>
#include "threads.hpp"

enum Drawable {
	DrawNone          = 0,
	DrawFunction      = 1,
	DrawInterpolation = 2,
	DrawResidual      = 4
};

Q_DECLARE_FLAGS(DrawableFlags, Drawable)

class DrawArea: public QGLWidget {

Q_OBJECT

public:
	QPointF *points;
	qreal *alphas;
	quint32 *locnzc;
	qreal *values;
	MsrMatrix *matrix;

	DrawArea(QWidget *parent=0):
		QGLWidget(parent),
		xRot(-30.),
		nSca(.61)
	{
		getPoints(&points);
	}
	~DrawArea() {
		delete[] points;
		delete[] alphas;
		delete[] locnzc;
		delete[] values;
	}
	void update(bool firstRun = false);
	void updateResidual();

private:
	qreal xRot;
	qreal yRot;
	qreal zRot;
	qreal nSca;
	QPoint mousePosition;
	quint32 drawSegments;
	quint32 calcSegments;
	quint32 threads;

	void draw(const GLfloat *surfaceColor, const GLfloat *meshColor,
              qreal f(DrawArea *, QPointF, Vertex));
	void drawSurface(const GLfloat *color, qreal f(DrawArea *, QPointF, Vertex));
	void drawMesh(const GLfloat *color, qreal f(DrawArea *, QPointF, Vertex));
	void drawOXYProjection();
	void initializeGL();
	void resizeGL(int width, int height);
	void paintGL();
	void mousePressEvent(QMouseEvent *event);
	void mouseMoveEvent(QMouseEvent *event);
	void wheelEvent(QWheelEvent *event);

	DrawableFlags drawableFlags() const;
};

class MyMainWindow: public QMainWindow {

Q_OBJECT

private:
	QToolBar toolBar;
	QLabel calcLayersLabel;
	QLabel drawLayersLabel;
	QLabel threadsLabel;
	DrawArea drawArea;
	QAction redrawAction;
	QAction functionAction;
	QAction interpolationAction;
	QAction residualAction;
	QActionGroup actionGroup;

public:
	MyMainWindow();
	DrawableFlags drawableFlags() const;
	QSpinBox calcLayersSpinBox;
	QSpinBox drawLayersSpinBox;
	QSpinBox threadsSpinBox;
	QStatusBar statusBar;
};
