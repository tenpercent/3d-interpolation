#include <Qt>
#include <QTextStream>
#include <QString>
#include <QtAlgorithms>
#include <QPalette>

#include "window.hpp"
#include "function.hpp"

#define VERTEX(p, v) point = p; glVertex3f(point.x()-.5, point.y()-.5, f(this, point, v))
#define WINDOW ((MyMainWindow *)parentWidget())

static const GLfloat LightPosition[4] = {0.0f, 0.0f, 10.0f, 1.0f};
static const GLfloat LightAmbient[4] = {0.5f, 0.5f, 0.5f, 1.0f};
static const GLfloat LightDiffuse[4] = {1.0f, 1.0f, 1.0f, 1.0f};

static const GLfloat goldColor[3] = {0.85f, 0.85f, 0.25f};
static const GLfloat lightBlueColor[3] = {0.4f, 0.4f, 1.0f};
static const GLfloat blueColor[3] = {0.0f, 0.0f, 1.0f};
static const GLfloat redColor[3]  = {1.0f, 0.4f, 0.4f};
static const GLfloat orangeColor[3]  = {1.0f, 0.5f, 0.2f};

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
	calcLayersSpinBox.setRange(1, 50);
	drawLayersSpinBox.setRange(1, 50);
	threadsSpinBox.setRange(1, 10);
	calcLayersSpinBox.setValue(10);
	drawLayersSpinBox.setValue(10);
	threadsSpinBox.setValue(4);
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

double origFunction(DrawArea *, QPointF p, Vertex) {
	return function(p.x(), p.y());
}

double interpolationFunction(DrawArea *a, QPointF, Vertex v) {
	return a->values[getIndexFromVertex(v)];
}

double residualFunction(DrawArea *a, QPointF p, Vertex v) {
	return origFunction(a, p, v) - interpolationFunction(a, p, v);
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
	/*
	alphas = new double[POINTS(calcLayers, size)];
	locnzc = new quint32[POINTS(calcLayers, size)];
	values = new double[POINTS(drawLayers, size)];
	*/
	quint32 calcPoints = (calcSegments + 1) * (calcSegments + 1);
	quint32 drawPoints = (drawSegments + 1) * (drawSegments + 1);
	alphas = new double[calcPoints];
	locnzc = new quint32[calcPoints];
	values = new double[drawPoints];

	pthread_t *thr = new pthread_t[threads];
	Args *args = new Args[threads];
	quint32 donethreads = 0;
	quint32 i, nzCount;
	pthread_mutex_lock(&mutex);
	for (i = 0; i < threads; ++i) {
		args[i].mutex = &mutex;
		args[i].condvar_tomf = &condvar_tomf;
		args[i].condvar_totf = &condvar_totf;
		args[i].threads = threads;
		args[i].thread = i;
		args[i].donethreads = &donethreads;
		args[i].points = points;
		args[i].segments = calcSegments;
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
	QFile debugF ("debug");
	debugF.open(QFile::WriteOnly);
	QTextStream debugTS (&debugF);
	for (i = 0; i < calcPoints; ++i){
		nzCount += locnzc[i];
		debugTS << locnzc[i] << " ";
		if (!((i + 1)%(calcSegments + 1))){
			debugTS << endl;
		}
	}
	debugF.close();
	QTextStream(stdout) << "\b\b\b" << nzCount << endl;
	QTextStream(stdout) << "Start filling matrix" << endl;
	QTextStream(stdout).flush();
	MsrMatrix *matrix = new MsrMatrix(calcPoints, nzCount);
	QTextStream(stdout) << "Matrix initialization success!" << endl;
	QTextStream(stdout).flush();
	for (i = 0; i < threads; ++i) {
		args[i].matrix = matrix;
		args[i].task = FillAndSolveMatrix;
	}
	donethreads = 0;
	pthread_cond_broadcast(&condvar_totf);
	pthread_cond_wait(&condvar_tomf, &mutex);
	
	QTextStream(stdout) << "Filling completed" << endl;
	QTextStream(stdout) << "Left part:" << endl;
	for (i = 0; i < calcPoints + nzCount + 1; ++i) {
		QTextStream(stdout) << matrix->elements[i] << " " << endl;
	}
	QTextStream(stdout) << "Indices:" << endl;
	for (i = 0; i < calcPoints + nzCount + 1; ++i) {
		QTextStream(stdout) << matrix->indices[i] << " " << endl;
	}
	QTextStream(stdout) << "Right part:" << endl;
	for (i = 0; i < calcPoints; ++i) {
		QTextStream(stdout) << matrix->rightCol[i] << " " << endl;
	}
	QTextStream(stdout) << "Solving system: ...";
	QTextStream(stdout).flush();

	quint32 iterations = matrix->solve(alphas);
	QTextStream(stdout) << "\b\b\bsolved in " << iterations
		<< " iterations" << endl;
	delete matrix;
	donethreads = 0;
	for (i = 0; i < threads; ++i)
		args[i].task = FillValues;
	QTextStream(stdout) << "Filling values array: ...";
	QTextStream(stdout).flush();
	pthread_cond_broadcast(&condvar_totf);
	pthread_cond_wait(&condvar_tomf, &mutex);
	
	QTextStream(stdout) << "\b\b\bdone" << endl;
	for (i = 0; i < threads; ++i)
		args[i].task = FinishWork;
	pthread_cond_broadcast(&condvar_totf);
	pthread_mutex_unlock(&mutex);
	for (i = 0; i < threads; ++i)
		pthread_join(thr[i], NULL);
	updateResidual();
}
//???
void DrawArea::updateResidual() {
	double residual = 0, value, phi;
	uint calcPoints = (calcSegments + 1) * (calcSegments + 1);
	QPointF point;
	quint32 i, j, k;
	const double residualSegments (42.);
	//const double residualPoints (residualSegments * residualSegments);
	for (i = 0; i <= residualSegments; ++i) {
		for(j = 0; j <= residualSegments; ++j) {
			value = 0.;
			point = getPointByMeshCoordinates (points, residualSegments, i, j);
			for (k = 0; k < calcPoints; ++k) {
				phi = phiFunctionGetByVertexIndex (points, calcSegments, point, k);
				value += alphas[i] * phi;
			}
			value = qAbs(value - function(point.x(), point.y()));
			residual = qMax(value, residual);
		}
	}
	WINDOW->statusBar.showMessage(QString("Residual: %1").arg(residual));
}

void DrawArea::drawSurface(const GLfloat *color,
double f(DrawArea *, QPointF, Vertex)) {
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
	glBegin(GL_TRIANGLES);
	QPointF point;
	quint32 drawTriangles = 2 * drawSegments * drawSegments;
	for (quint32 i = 0; i < drawTriangles; ++i) {
		//Triangle triangle = getTriangle(points, size, drawLayers, i, &center);
		Triangle triangle = getTriangleByIndex(points, drawSegments, i);
		VERTEX(triangle[0], triangle.a);
		VERTEX(triangle[1], triangle.b);
		VERTEX(triangle[2], triangle.c);
	}
	glEnd();
}

void DrawArea::drawMesh(const GLfloat *color,
double f(DrawArea *, QPointF, Vertex)) {
	quint32 drawTriangles = 2 * drawSegments * drawSegments;
	glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, color);
	QPointF point;
	for (quint32 i = 0; i < drawTriangles; ++i) {
		//Triangle triangle = getTriangle(points, size, drawLayers, i, &center);
		Triangle triangle = getTriangleByIndex (points, drawSegments, i);
		glBegin(GL_LINE_STRIP);
		VERTEX(triangle[0], triangle.a);
		VERTEX(triangle[1], triangle.b);
		VERTEX(triangle[2], triangle.c);
		VERTEX(triangle[0], triangle.a);
		glEnd();
	}
}

void DrawArea::initializeGL() {
	qglClearColor(QPalette().color(QPalette::Window));
	glEnable(GL_DEPTH_CLAMP);
	//glEnable(GL_DEPTH_TEST);
}

void DrawArea::resizeGL(int width, int height) {
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	GLfloat ratio = height;
	ratio /= width;

	if (width > height)
		glOrtho(-1./ratio, 1./ratio, -1., 1., -10., 1.);
	else
		glOrtho(-1., 1., -ratio, ratio, -10., 1.);

	glViewport(0, 0, width, height);
}

void DrawArea::draw(const GLfloat *surfaceColor, const GLfloat *meshColor,
                    double f(DrawArea *, QPointF, Vertex))
{
	drawSurface(surfaceColor, f);
	drawMesh(meshColor, f);
}

void DrawArea::paintGL() {
	if (
		quint32(WINDOW->calcLayersSpinBox.value()) != calcSegments ||
		quint32(WINDOW->drawLayersSpinBox.value()) != drawSegments
	)
		update();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glScalef(nSca, nSca, nSca);
	glRotatef(xRot, 1.0f, 0.0f, 0.0f);
	glRotatef(yRot, 0.0f, 1.0f, 0.0f);
	glRotatef(zRot, 0.0f, 0.0f, 1.0f);
	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT1, GL_AMBIENT, LightAmbient);
	glLightfv(GL_LIGHT1, GL_DIFFUSE, LightDiffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, LightPosition);
	glEnable(GL_LIGHT1);
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
	xRot += 180/nSca*(GLfloat)(event->y() - mousePosition.y()) / height();
	zRot += 180/nSca*(GLfloat)(event->x() - mousePosition.x()) / width();

	mousePosition = event->pos();
	updateGL();
}

void DrawArea::wheelEvent(QWheelEvent *event) {
	int delta = event->delta();
	nSca *= qExp(double(delta) / 2048);
	repaint();
}
