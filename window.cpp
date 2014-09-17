#include <Qt>
#include <QTextStream>
#include <QString>
#include <QtAlgorithms>
#include <QPalette>

#include "window.hpp"
#include "function.hpp"

#ifdef SHIFT_VERTICAL
	#define VERTEX(p, v) point = p; glVertex3f(point.x() - 1., point.y() - 1., f(this, point, v) - average)
#endif
#define VERTEX(p, v) point = p; glVertex3f(point.x() - 1., point.y() - 1., f(this, point, v))
#define WINDOW ((MyMainWindow *)parentWidget())

static const GLfloat goldColor[3] = {0.85f, 0.85f, 0.25f};
static const GLfloat lightBlueColor[3] = {0.4f, 0.4f, 1.0f};
static const GLfloat blueColor[3] = {0.0f, 0.0f, 1.0f};
static const GLfloat redColor[3]  = {1.0f, 0.4f, 0.4f};
static const GLfloat orangeColor[3]  = {1.0f, 0.5f, 0.2f};
static const GLfloat greenColor[3] = {0.2f, 1.0f, 0.6f};

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t condvar_tomf = PTHREAD_COND_INITIALIZER;
static pthread_cond_t condvar_totf = PTHREAD_COND_INITIALIZER;

MyMainWindow::MyMainWindow():
	toolBar("Toolbar", this),
	calcLayersLabel("Calculation layers:"),
	drawLayersLabel("Draw layers:"),
	threadsLabel("Threads:"),
	drawArea(this),
	redrawAction("Redraw", this),
	functionAction("Original", this),
	interpolationAction("Interpolation", this),
	residualAction("Residual", this),
	actionGroup(this),
	calcLayersSpinBox(this),
	drawLayersSpinBox(this),
	threadsSpinBox(this)
{
	// prepare widgets
	calcLayersLabel.setMargin(5);
	drawLayersLabel.setMargin(5);
	threadsLabel.setMargin(5);
	calcLayersSpinBox.setRange(1, 100);
	drawLayersSpinBox.setRange(1, 40);
	threadsSpinBox.setRange(1, 10);
	calcLayersSpinBox.setValue(10);
	drawLayersSpinBox.setValue(10);
	threadsSpinBox.setValue(1);
	drawArea.update(true);
	// prepare actions
	functionAction.setCheckable(true);
	functionAction.setChecked(true);
	interpolationAction.setCheckable(true);
	residualAction.setCheckable(true);
	actionGroup.addAction(&functionAction);
	actionGroup.addAction(&interpolationAction);
	actionGroup.addAction(&residualAction);
	// prepare toolbar
	toolBar.setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
	toolBar.addWidget(&calcLayersLabel);
	toolBar.addWidget(&calcLayersSpinBox);
	toolBar.addWidget(&drawLayersLabel);
	toolBar.addWidget(&drawLayersSpinBox);
	toolBar.addWidget(&threadsLabel);
	toolBar.addWidget(&threadsSpinBox);
	toolBar.addSeparator();
	toolBar.addAction(&functionAction);
	toolBar.addAction(&interpolationAction);
	toolBar.addAction(&residualAction);
	toolBar.addSeparator();
	toolBar.addAction(&redrawAction);
	connect(&redrawAction, SIGNAL(triggered()), &drawArea, SLOT(repaint()));
	connect(&functionAction, SIGNAL(triggered(bool)), &drawArea, SLOT(repaint()));
	connect(&interpolationAction, SIGNAL(triggered(bool)), &drawArea, SLOT(repaint()));
	connect(&residualAction, SIGNAL(triggered(bool)), &drawArea, SLOT(repaint()));
	// prepare window
	resize(1000, 800);
	setCentralWidget(&drawArea);
	setStatusBar(&statusBar);
	addToolBar(&toolBar);
}

DrawableFlags MyMainWindow::drawableFlags() const {
	DrawableFlags result = DrawNone;
	if (functionAction.isChecked())
		result |= DrawFunction;
	if (residualAction.isChecked())
		result |= DrawResidual;
	if (interpolationAction.isChecked())
		result |= DrawInterpolation;
	return result;
}

DrawableFlags DrawArea::drawableFlags() const {
	return WINDOW->drawableFlags();
}

qreal origFunction(DrawArea *, QPointF p, Vertex) {
	return function(p.x(), p.y());
}

qreal interpolationFunction(DrawArea *a, QPointF, Vertex v) {
	return a->values[getIndexFromVertex(v)];
}

qreal residualFunction(DrawArea *a, QPointF p, Vertex v) {
	return qAbs(origFunction(a, p, v) - interpolationFunction(a, p, v));
}

void DrawArea::update(bool firstRun) {
	if (!firstRun) {
		delete[] alphas;
		delete[] locnzc;
		delete[] values;
	}
	calcSegments = WINDOW->calcLayersSpinBox.value();
	drawSegments = WINDOW->drawLayersSpinBox.value();
	threads = WINDOW->threadsSpinBox.value();
	quint32 calcPoints = (calcSegments + 1) * (calcSegments + 1);
	quint32 drawPoints = (drawSegments + 1) * (drawSegments + 1);
	alphas = new qreal[calcPoints];
	locnzc = new quint32[calcPoints];
	values = new qreal[drawPoints];

	pthread_t *thr = new pthread_t[threads];
	Args *args = new Args[threads];
	quint32 donethreads(0);
	quint32 nzCount(0);
	pthread_mutex_lock(&mutex);
	for (quint32 i = 0; i < threads; ++i) {
		args[i].mutex = &mutex;
		args[i].condvar_tomf = &condvar_tomf;
		args[i].condvar_totf = &condvar_totf;
		args[i].threads = threads;
		args[i].thread = i;
		args[i].donethreads = &donethreads;
		args[i].points = points;
		args[i].calcSegments = calcSegments;
		args[i].drawSegments = drawSegments;
		args[i].locnzc = locnzc;
		args[i].alphas = alphas;
		args[i].values = values;
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
		nzCount += locnzc[i];
	QTextStream(stdout) << "\b\b\b" << nzCount << endl;
	QTextStream(stdout) << "Start filling matrix" << endl;
	QTextStream(stdout).flush();
	MsrMatrix *matrix = new MsrMatrix(calcPoints, nzCount);
	QTextStream(stdout) << "Matrix initialization success!" << endl;
	QTextStream(stdout).flush();
	for (quint32 i = 0; i < threads; ++i) {
		args[i].matrix = matrix;
		args[i].task = FillAndSolveMatrix;
	}

	donethreads = 0;
	pthread_cond_broadcast(&condvar_totf);
	pthread_cond_wait(&condvar_tomf, &mutex);
	
	QTextStream(stdout) << "Filling completed" << endl;
/*
	QTextStream(stdout) << "Left part & indices:" << endl;
	for (quint32 i = 0; i < calcPoints + nzCount + 1; ++i) {
		QTextStream(stdout) << matrix->elements[i] << " " << matrix->indices[i] << endl;
	}
	for (quint32 i = 0; i < calcPoints; ++i) {
		QTextStream(stdout) << locnzc[i] << " ";
	}
	
	QTextStream(stdout) << "Right part:" << endl;
	for (quint32 i = 0; i < calcPoints; ++i) {
		QTextStream(stdout) << matrix->rightCol[i] << " " << endl;
	}
*/
	QTextStream(stdout) << "Solving system: ...";
	QTextStream(stdout).flush();

	quint32 iterations = matrix->solve(alphas);
	QTextStream(stdout) << "\b\b\bsolved in " << iterations
		<< " iterations" << endl;
	delete matrix;

	donethreads = 0;
	for (quint32 i = 0; i < threads; ++i)
		args[i].task = FillValues;
	QTextStream(stdout) << "Filling values array: ...";
	QTextStream(stdout).flush();
	pthread_cond_broadcast(&condvar_totf);
	pthread_cond_wait(&condvar_tomf, &mutex);
	
	QTextStream(stdout) << "\b\b\bdone" << endl;
	for (quint32 i = 0; i < threads; ++i)
		args[i].task = FinishWork;
	pthread_cond_broadcast(&condvar_totf);
	pthread_mutex_unlock(&mutex);
	for (quint32 i = 0; i < threads; ++i)
		pthread_join(thr[i], NULL);
	updateResidual();
}
//???
void DrawArea::updateResidual() {
	qreal residual (0), value (0), phi (0), local_residual(0);
	quint32 calcPoints = (calcSegments + 1) * (calcSegments + 1);
	QPointF point;
	const quint32 residualSegments (calcSegments + 1);
	for (quint32 i = 0; i <= residualSegments; ++i) {
		for(quint32 j = 0; j <= residualSegments; ++j) {
			value = 0.;
			point = getPointByMeshCoordinates (points, residualSegments, i, j);
			for (quint32 k = 0; k < calcPoints; ++k) {
				phi = phiFunctionGetByVertexIndex (points, calcSegments, point, k);
				value += alphas[k] * phi;
			}
			local_residual = qAbs(value - function(point.x(), point.y()));
			residual = qMax(local_residual, residual);
		}
	}
	WINDOW->statusBar.showMessage(QString("Residual: %1").arg(residual));
}

void DrawArea::drawSurface(const GLfloat *color,
qreal f(DrawArea *, QPointF, Vertex)) {
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
	glBegin(GL_TRIANGLES);
#ifdef SHIFT_VERTICAL
	const qreal average = 1./4 * (
		f(this, points[0], getVertexByIndex(drawSegments, 0)) + 
		f(this, points[2], getVertexByIndex(drawSegments, drawSegments)) + 
		f(this, points[1], getVertexByIndex(drawSegments, drawSegments * (drawSegments + 1 ))) + 
		f(this, points[1] + points[2] - points[0], getVertexByIndex(drawSegments, drawSegments * (drawSegments + 2)))
		);
#endif
	QPointF point;
	quint32 drawTriangles = 2 * drawSegments * drawSegments;
	for (quint32 i = 0; i < drawTriangles; ++i) {
		Triangle triangle = getTriangleByIndex(points, drawSegments, i);
		VERTEX(triangle[0], triangle.a);
		VERTEX(triangle[1], triangle.b);
		VERTEX(triangle[2], triangle.c);
	}
	glEnd();
}

void DrawArea::drawMesh(const GLfloat *color,
qreal f(DrawArea *, QPointF, Vertex)) {
	quint32 drawTriangles = 2 * drawSegments * drawSegments;
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
	QPointF point;
#ifdef SHIFT_VERTICAL
	const qreal average = 1./4 * (
		f(this, points[0], getVertexByIndex(drawSegments, 0)) + 
		f(this, points[2], getVertexByIndex(drawSegments, drawSegments)) + 
		f(this, points[1], getVertexByIndex(drawSegments, drawSegments * (drawSegments + 1 ))) + 
		f(this, points[1] + points[2] - points[0], getVertexByIndex(drawSegments, drawSegments * (drawSegments + 2)))
		);
#endif
	for (quint32 i = 0; i < drawTriangles; ++i) {
		Triangle triangle = getTriangleByIndex (points, drawSegments, i);
		glBegin(GL_LINE_STRIP);
		VERTEX(triangle[0], triangle.a);
		VERTEX(triangle[1], triangle.b);
		VERTEX(triangle[2], triangle.c);
		VERTEX(triangle[0], triangle.a);
		glEnd();
	}
}
void DrawArea::drawOXYProjection() {
	const GLfloat color[4] = {0., 0., 0., 1.}; 
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
	for (quint32 i = 0; i < drawSegments; ++i) {
		for (quint32 j = 0; j < drawSegments; ++j){
			glBegin(GL_LINE_STRIP);

			glVertex3f(
				getPointByMeshCoordinates(points, drawSegments, i, j).x() - 1., 
				getPointByMeshCoordinates(points, drawSegments, i, j).y() - 1., 
				0.);
			glVertex3f(
				getPointByMeshCoordinates(points, drawSegments, i + 1, j).x() - 1., 
				getPointByMeshCoordinates(points, drawSegments, i + 1, j).y() - 1., 
				0.);
			glVertex3f(
				getPointByMeshCoordinates(points, drawSegments, i + 1, j + 1).x() - 1., 
				getPointByMeshCoordinates(points, drawSegments, i + 1, j + 1).y() - 1., 
				0.);
			glVertex3f(
				getPointByMeshCoordinates(points, drawSegments, i, j + 1).x() - 1., 
				getPointByMeshCoordinates(points, drawSegments, i, j + 1).y() - 1., 
				0.);
			glVertex3f(
				getPointByMeshCoordinates(points, drawSegments, i, j).x() - 1., 
				getPointByMeshCoordinates(points, drawSegments, i, j).y() - 1., 
				0.);

			glEnd();
		}
	}
}

void DrawArea::initializeGL() {
	qglClearColor(QPalette().color(QPalette::Window));
	glEnable(GL_DEPTH_CLAMP);
}

void DrawArea::resizeGL(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLfloat ratio = height;
	ratio /= width;

	if (width > height)
		glOrtho(-1./ratio, 1./ratio, -1., 1., -1., 1.);
	else
		glOrtho(-1., 1., -ratio, ratio, -1., 1.);


	glViewport(0, 0, width, height);
}

void DrawArea::draw(const GLfloat *surfaceColor, const GLfloat *meshColor,
                    qreal f(DrawArea *, QPointF, Vertex))
{
	drawSurface(surfaceColor, f);
	drawMesh(meshColor, f);
	drawOXYProjection();
}

void DrawArea::paintGL() {
	if (
		quint32(WINDOW->calcLayersSpinBox.value()) != calcSegments ||
		quint32(WINDOW->drawLayersSpinBox.value()) != drawSegments
	)
		update();
	glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode (GL_MODELVIEW);
	glLoadIdentity ();
	glScalef (nSca, nSca, nSca);
	glRotatef (xRot, 1.0f, 0.0f, 0.0f);
	glRotatef (yRot, 0.0f, 1.0f, 0.0f);
	glRotatef (zRot, 0.0f, 0.0f, 1.0f);
	
	const GLfloat light0_position[4] = {0., 0., 1., 0.};
	const GLfloat light0_diffuse[4] = {1., 1., 1., .2};
	const GLfloat light0_ambient[4] = {.2, .2, .2, 1.};

	glEnable(GL_LIGHTING);

	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);
	glLightfv(GL_LIGHT0, GL_AMBIENT, light0_ambient);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light0_diffuse);
	glEnable(GL_LIGHT0);

	DrawableFlags flags = drawableFlags();
	if (flags & DrawFunction)
		draw(goldColor, blueColor, origFunction);
	if (flags & DrawInterpolation)
		draw(lightBlueColor, blueColor, interpolationFunction);
	if (flags & DrawResidual)
		draw(redColor, blueColor, residualFunction);

}

void DrawArea::mousePressEvent(QMouseEvent *event) {
	mousePosition = event->pos();
}

void DrawArea::mouseMoveEvent(QMouseEvent *event) {
	xRot += 180 / nSca * (GLfloat)(event->y() - mousePosition.y()) / height();
	zRot += 180 / nSca * (GLfloat)(event->x() - mousePosition.x()) / width();

	mousePosition = event->pos();
	updateGL();
}

void DrawArea::wheelEvent(QWheelEvent *event) {
	int delta = event->delta();
	nSca *= qExp(qreal(delta) / 2048);
	repaint();
}
