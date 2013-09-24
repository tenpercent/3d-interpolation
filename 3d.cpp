#include <QFile>
#include <QLineF>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <qmath.h>

#include "function.hpp"
#include "3d.hpp"

#define NORMALIZE_ANGLE(phi) (phi > 180) ? (360 - phi) : phi
#define EPS .02

quint32 getPoints(QPointF **points) {
	const quint32 size = 3;
	QFile coordinatesFile("coordinates.txt");
	coordinatesFile.open(QFile::ReadOnly);
	QTextStream stream(&coordinatesFile);
	*points = new QPointF[size];
	qreal x, y;
	/*
	for (quint32 i = 0; i < size; ++i) {
		stream >> x >> y;
		(*points)[i] = QPointF(x, y);
	}*/
	quint32 i(0);
	while (!(stream.atEnd()) && (i < size)) {
		stream >> x >> y;
		(*points)[i] = QPointF(x, y);
		++i;
	}
	coordinatesFile.close();
	if (i < size) {
		QMessageBox messageBox;
		messageBox.setText("Please change coordinates.txt file");
		messageBox.setInformativeText(QString("The parallelogram can't be constructed:\ntoo few points (%1)").arg(i));
		messageBox.exec();
		return 1;
	}	
	const qreal vectorProduct = ((*points)[2].y() - (*points)[0].y()) * ((*points)[1].x() - (*points)[0].x()) - 
								((*points)[2].x() - (*points)[0].x()) * ((*points)[1].y() - (*points)[0].y());
	if (qAbs(vectorProduct) < EPS) {
		QMessageBox messageBox;
		messageBox.setText("Please change coordinates.txt file");
		messageBox.setInformativeText("The parallelogram can't be constructed:\npoints lay on one line");
		messageBox.exec();
		return 2;
	}
	if (vectorProduct < -EPS) {
		qSwap((*points)[1], (*points)[2]);
	}
	return 0;
}

quint32 getVertexIndex (const quint32 segments, const quint32 row, const quint32 column){
	const quint32 rows = segments + 1;
	quint32 result = row * rows + column;
	return result;
}

quint32 getIndexFromVertex (Vertex v){
	return v.index;
}

QPointF getPointByMeshCoordinates (QPointF * const points, const quint32 segments, const quint32 row, const quint32 column) {//row:[0..segments], column: the same
	//const qreal _segments = 1. / segments;
	const QPointF deltaRow = (points[1] - points[0]) / segments;
	const QPointF deltaColumn = (points[2] - points[0]) / segments;
	QPointF result = points[0] + row * deltaRow + column * deltaColumn;
	return result;
}

QPointF getPointByMeshIndex (QPointF * const points, const quint32 segments, const quint32 index) {
	const quint32 pointsInRow = segments + 1;
	const quint32 row = index / pointsInRow;
	//const quint32 column = index - row * pointsInRow;
	const quint32 column = index % pointsInRow;
	return getPointByMeshCoordinates (points, segments, row, column);
}

QPointF getPointFromVertex (QPointF * const points, const quint32 segments, const Vertex v) {
	return getPointByMeshCoordinates (points, segments, v.row, v.column);
}

Vertex getVertex (const quint32 row, const quint32 column, const quint32 index) {
	return Vertex (row, column, index);
}

Vertex getVertexByIndex (const quint32 segments, const quint32 index) {
	const quint32 side_points = segments + 1;
	const quint32 row = index / side_points;
	const quint32 column = index % side_points;
	return getVertex (row, column, index);
}
 
Triangle getTriangleByIndex (QPointF * const points, const quint32 segments, const quint32 triangle_index) {
	Vertex a, b, c;
	const quint32 triangles_in_row = 2 * segments;
	const quint32 row_segment = triangle_index / triangles_in_row;
	const quint32 column_segment = (triangle_index % triangles_in_row) / 2 ;
	const quint32 second_in_the_cell = triangle_index & 1;//1 if odd; 0 if even
	
	if (!second_in_the_cell) {
		a = getVertex(row_segment, column_segment, getVertexIndex(segments, row_segment, column_segment));
		b = getVertex(row_segment + 1, column_segment, getVertexIndex(segments, row_segment + 1, column_segment));
		c = getVertex(row_segment, column_segment + 1, getVertexIndex(segments, row_segment, column_segment + 1));
	} else {
		a = getVertex(row_segment + 1, column_segment, getVertexIndex(segments, row_segment + 1, column_segment));
		b = getVertex(row_segment + 1, column_segment + 1, getVertexIndex(segments, row_segment + 1, column_segment + 1));
		c = getVertex(row_segment, column_segment + 1, getVertexIndex(segments, row_segment, column_segment + 1));
	}
	return Triangle(points, segments, a, b, c);
}
 
Polynom psiFunctionPolynom (const Triangle &triangle, const short n) {
	//double u1, u2, v1, v2;
	qreal u1, u2, v1, v2;
	QPointF a = triangle[0], b = triangle[1], c = triangle[2];
	Polynom result;
	if (n == 0) {
		u1 = b.x();
		v1 = b.y();
	} else {
		u1 = a.x();
		v1 = a.y();
	}
	if (n == 2) {
		u2 = b.x();
		v2 = b.y();
	} else {
		u2 = c.x();
		v2 = c.y();
	}
	addToPolynom(result, 1, 0, v2 - v1);
	addToPolynom(result, 0, 1, u1 - u2);
	addToPolynom(result, 0, 0, u2 * v1 - v2 * u1);
	return result;
}

qreal psiFunction (const QPointF point, const Triangle &triangle, const short n) {
	//double u1, u2, v1, v2;
	qreal u1, u2, v1, v2;
	QPointF a = triangle[0], b = triangle[1], c = triangle[2];
	if (n == 0) {
		u1 = b.x();
		v1 = b.y();
	} else {
		u1 = a.x();
		v1 = a.y();
	}
	if (n == 2) {
		u2 = b.x();
		v2 = b.y();
	} else {
		u2 = c.x();
		v2 = c.y();
	}
	return (point.x() - u1) * (v2 - v1) - (point.y() - v1) * (u2 - u1);
}

bool pointInTriangle (const QPointF point, const Triangle &triangle) {
	QPointF a = triangle[0], b = triangle[1], c = triangle[2];
	if (point == a || point == b || point == c)
		return true;
	QLineF line0(point, a), line1(point, b), line2(point, c);
	qreal angle0 = line0.angleTo(line1);
	angle0 = NORMALIZE_ANGLE(angle0);
	qreal angle1 = line1.angleTo(line2);
	angle1 = NORMALIZE_ANGLE(angle1);
	qreal angle2 = line2.angleTo(line0);
	angle2 = NORMALIZE_ANGLE(angle2);
	return qAbs(angle0 + angle1 + angle2 - 360) < EPS;
}

qreal phiFunctionGetByTriangle (const QPointF point, const Triangle &triangle, const QPointF vertex) {
	qint8 n = 0;
	while (triangle[n] != vertex) {
		++n;
		if (n > 2){
			qDebug() << "at phiFunctionGetByTriangle():" << vertex;
			break;
		}
	}
	Q_ASSERT(qAbs(psiFunction(vertex, triangle, n)) > 1e-10);
	Q_ASSERT(qAbs(psiFunction(point, triangle, n)) < 1e10);
	return psiFunction(point, triangle, n) / psiFunction(vertex, triangle, n);
}

Polynom phiFunctionPolynom (const Triangle &triangle, const QPointF vertex) {
	qint8 n = 0;
	while (triangle[n] != vertex) {
		++n;
		if (n > 2) {
			qDebug() << "at phiFunctionPolynom():" << vertex;
			break;
		}
	}
	const qreal psiValue = psiFunction(vertex, triangle, n);
	Polynom result = psiFunctionPolynom(triangle, n);
	for (qint32 i = 0; i < result.size(); ++i)
		result[i].c /= psiValue;
	return result;
}

//#define resultappend(i) qDebug() << __LINE__; result.append(i)

TriangleList getCommonTriangles (QPointF * const points, const quint32 segments, 
const Vertex v1, const Vertex v2){
	Vertex v1_ = v1;
	Vertex v2_ = v2;
	TriangleList result;
	const quint32 last_item (segments + 1);
	if (v1_.index > v2_.index){
		qSwap(v1_, v2_);
	}
	const qint32 diff_i (v2_.index - v1_.index);
	if (diff_i < 0) {
		qDebug() << "diff < 0:" << diff_i << " " << v1_.index << " " << v2_.index;
	}
	const quint32 diff (diff_i);
	if ((diff == 1) && ((v1_.column + 1) != last_item)) {
		if (v1_.row == 0) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 0));
		}
		else if ((v1_.row + 1) == last_item) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 5));
		}
		else{
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 0));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 5));
		}
	}
	else if (diff == last_item) {
		if (v1_.column == 0) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 0));
		}
		else if ((v1_.column + 1) == last_item) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 1));
		}
		else{
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 0));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 1));
		}
	}
	else if ((diff == (last_item - 1)) && (v1_.column != 0)) {
		//qDebug() << "My vertices are" << v1 << v2;
		result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 1));
		result.append(getTriangleByVertexAndLocalIndex(points, segments, v1_, 2));
	}
	return result;
}

TriangleList getSurroundingTriangles (QPointF * const points, const quint32 segments, const Vertex v1) {
	TriangleList result;
	const quint32 total_rows (segments + 1);
	if (v1.row == 0) {
		if (v1.column == 0) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 0));
		}
		else if ((v1.column + 1) == total_rows) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 1));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 2));
		}
		else {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 0));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 1));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 2));
		}
	}
	else if ((v1.row + 1) == total_rows) {
		if (v1.column == 0) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 4));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 5));
		}
		else if ((v1.column + 1) == total_rows) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 3));
		}
		else {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 3));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 4));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 5));
		}
	}
	else {
		if (v1.column == 0) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 0));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 4));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 5));
		}
		else if ((v1.column + 1) == total_rows) {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 1));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 2));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 3));
		}
		else {
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 0));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 1));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 2));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 3));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 4));
			result.append(getTriangleByVertexAndLocalIndex(points, segments, v1, 5));
		}
	}
	return result;
}
Triangle getTriangleByVertexAndLocalIndex (QPointF * const points, const quint32 segments, const Vertex v, const quint32 triIndex) {
	Triangle result;
	const quint32 row(v.row);
	const quint32 column(v.column);
	const quint32 index(v.index);
	Vertex a, b, c;
	switch(triIndex) {
		case 0:
			if ((row == segments) || (column == segments)) {
				qDebug() << "alarm! (case 0)" << endl;
			}
			a = getVertex(row, column, index);
			b = getVertex(row + 1, column, getVertexIndex(segments, row + 1, column));
			c = getVertex(row, column + 1, getVertexIndex(segments, row, column + 1));
			result = Triangle(points, segments, a, b, c);
			break;
		case 1:
			if ((row == segments) || (column == 0)) {
				qDebug() << "alarm! (case 1)" << endl;
			}
			a = getVertex(row, column, index);
			b = getVertex(row + 1, column - 1, getVertexIndex(segments, row + 1, column - 1));
			c = getVertex(row + 1, column, getVertexIndex(segments, row + 1, column));
			result = Triangle(points, segments, a, b, c);
			break;
		case 2:
			if ((row == segments) || (column == 0)) {
				qDebug() << "alarm! (case 2)" << endl;
			}
			a = getVertex(row, column, index);
			b = getVertex(row, column - 1, getVertexIndex(segments, row, column - 1));
			c = getVertex(row + 1, column - 1, getVertexIndex(segments, row + 1, column - 1));
			result = Triangle(points, segments, a, b, c);
			break;
		case 3:
			if ((row == 0) || (column == 0)) {
				qDebug() << "alarm! (case 3)" << endl;
			}
			a = getVertex(row, column, index);
			b = getVertex(row - 1, column, getVertexIndex(segments, row - 1, column));
			c = getVertex(row, column - 1, getVertexIndex(segments, row, column - 1));
			result = Triangle(points, segments, a, b, c);
			break;
		case 4:
			if ((row == 0) || (column == segments)) {
				qDebug() << "alarm! (case 4)" << endl;
			}
			a = getVertex(row, column, index);
			b = getVertex(row - 1, column + 1, getVertexIndex(segments, row - 1, column + 1));
			c = getVertex(row - 1, column, getVertexIndex(segments, row - 1, column));
			result = Triangle(points, segments, a, b, c);
			break;
		case 5:
			if ((row == 0) || (column == segments)) {
				qDebug() << "alarm! (case 4)" << endl;
			}
			a = getVertex(row, column, index);
			/*
			b = getVertex(row, column + 1, getVertexIndex(segments, row, column + 1));
			c = getVertex(row - 1, column + 1, getVertexIndex(segments, row - 1, column + 1));
			*/
			c = getVertex(row, column + 1, getVertexIndex(segments, row, column + 1));
			b = getVertex(row - 1, column + 1, getVertexIndex(segments, row - 1, column + 1));
			result = Triangle(points, segments, a, b, c);
			break;
		default:
			break;
	}
	return result;
}
 
qreal phiFunctionGetByVertexIndex (QPointF * const points, const quint32 segments, const QPointF point, const quint32 vertexNumber) {
	Vertex v = getVertexByIndex(segments, vertexNumber);
	QPointF v_point = getPointByMeshIndex(points, segments, vertexNumber);
	TriangleList triangles = getSurroundingTriangles(points, segments, v);
	for (qint32 i = 0; i < triangles.size(); ++i)
	{
		if(pointInTriangle(point, triangles.at(i))){
			return phiFunctionGetByTriangle(point, 
											triangles.at(i), 
											v_point);
		}
	}
	return 0;
}

qreal determinant (const qreal a11, const qreal a12, const qreal a13, const qreal a21,
const qreal a22, const qreal a23, const qreal a31, const qreal a32, const qreal a33) {
	return a11 * a22 * a33 + a21 * a32 * a13 + a12 * a23 * a31
	     - a31 * a22 * a13 - a11 * a23 * a32 - a21 * a12 * a33;
}

Polynom getLinearInterpolation (QPointF * const p) {
	Polynom result;
	qreal a, b, c, d;
	d = determinant(
		p[0].x(), p[0].y(), 1,
		p[1].x(), p[1].y(), 1,
		p[2].x(), p[2].y(), 1
	);
	a = determinant(
		function(p[0].x(), p[0].y()), p[0].y(), 1,
		function(p[1].x(), p[1].y()), p[1].y(), 1,
		function(p[2].x(), p[2].y()), p[2].y(), 1
	) / d;
	b = determinant(
		p[0].x(), function(p[0].x(), p[0].y()), 1,
		p[1].x(), function(p[1].x(), p[1].y()), 1,
		p[2].x(), function(p[2].x(), p[2].y()), 1
	) / d;
	c = determinant(
		p[0].x(), p[0].y(), function(p[0].x(), p[0].y()),
		p[1].x(), p[1].y(), function(p[1].x(), p[1].y()),
		p[2].x(), p[2].y(), function(p[2].x(), p[2].y())
	) / d;
	addToPolynom(result, 1, 0, a);
	addToPolynom(result, 0, 1, b);
	addToPolynom(result, 0, 0, c);
	return result;
}
quint32 getSurroundingNeighborCount (const quint32 segments, const quint32 index) {
	const quint32 points_in_row = segments + 1;
	const quint32 row = index / points_in_row;
	const quint32 column = index % points_in_row;
	const quint32 last_item_index = segments;
	if (row == 0){
		if (column == 0){
			return 1;
		}
		else if (column == last_item_index) {
			return 2;
		}
		else {
			return 3;
		}
	}
	else if (row == last_item_index) {
		if (column == 0){
			return 2;
		}
		else if (column == last_item_index) {
			return 1;
		}
		else {
			return 3;
		}
	}
	else {
		if (column == 0){
			return 3;
		}
		else if (column == last_item_index) {
			return 3;
		}
		else {
			return 6;
		}
	}
	qDebug() << "@getSurroundingNeighborCount we left something..." << endl;
	return 0;
}
quint32 getCommonNeighborCount (const quint32 segments, const quint32 i_index, const quint32 j_index) {
	const qint32 points_in_row = segments + 1;
	const qint32 i_row = i_index / points_in_row;
	const qint32 i_column = i_index % points_in_row;
	const qint32 j_row = j_index / points_in_row;
	const qint32 j_column = j_index % points_in_row;
	const qint32 last_item_index = segments;

	if ((qAbs(i_row - j_row) > 1) || (qAbs (i_column - j_column) > 1)) {
		return 0;
	}
	if (((i_row - j_row) == 1) && ((i_column - j_column) == 1)) {
		return 0;
	}
	if (((j_row - i_row) == 1) && ((j_column - i_column) == 1)) {
		return 0;
	}
	if (i_row == j_row) {
		if (i_row == 0) {
			return 1;
		}
		else if (i_row == last_item_index){
			return 1;
		}
		else {
			return 2;
		}
	}
	else if (i_column == j_column) {
		if (i_column == 0){
			return 1;
		}
		else if (i_column == last_item_index) {
			return 1;
		}
		else {
			return 2;
		}
	}
	else {
		return 2;
	}
	qDebug() << "@getCommonNeighborCount we left something..." << endl;
	return 0;
}
void getLinearInterpolationPlane (QPointF* p, qreal* coef) {
	const qreal det = determinant(
		0, 0, 1,
		0, 1, 1,
		1, 0, 1
	);
	coef[0] = determinant(
		function(p[0].x(), p[0].y()), 0, 1,
		function(p[1].x(), p[1].y()), 1, 1,
		function(p[2].x(), p[2].y()), 0, 1
	) / det;
	coef[1] = determinant(
		0, function(p[0].x(), p[0].y()), 1,
		0, function(p[1].x(), p[1].y()), 1,
		1, function(p[2].x(), p[2].y()), 1
	) / det;
	coef[2] = determinant(
		0, 0, function(p[0].x(), p[0].y()),
		0, 1, function(p[1].x(), p[1].y()),
		1, 0, function(p[2].x(), p[2].y())
	) / det;
	return;
}
void getLinearInterpolationPlane_1 (QPointF* p, qreal* coef) {
	const qreal det = determinant(
		0, 0, 1,
		0, .5, 1,
		.5, 0, 1
	);
	coef[0] = determinant(
		function(p[0].x(), p[0].y()), 0, 1,
		function(p[1].x(), p[1].y()), .5, 1,
		function(p[2].x(), p[2].y()), 0, 1
	) / det;
	coef[1] = determinant(
		0, function(p[0].x(), p[0].y()), 1,
		0, function(p[1].x(), p[1].y()), 1,
		.5, function(p[2].x(), p[2].y()), 1
	) / det;
	coef[2] = determinant(
		0, 0, function(p[0].x(), p[0].y()),
		0, .5, function(p[1].x(), p[1].y()),
		.5, 0, function(p[2].x(), p[2].y())
	) / det;
	return;
}
void getLinearInterpolationPlane_2 (QPointF* p, qreal* coef) {
	const qreal det = determinant(
		0, .5, 1,
		0, 1, 1,
		.5, .5, 1
	);
	coef[0] = determinant(
		function(p[0].x(), p[0].y()), .5, 1,
		function(p[1].x(), p[1].y()), 1, 1,
		function(p[2].x(), p[2].y()), .5, 1
	) / det;
	coef[1] = determinant(
		0, function(p[0].x(), p[0].y()), 1,
		0, function(p[1].x(), p[1].y()), 1,
		.5, function(p[2].x(), p[2].y()), 1
	) / det;
	coef[2] = determinant(
		0, .5, function(p[0].x(), p[0].y()),
		0, 1, function(p[1].x(), p[1].y()),
		.5, .5, function(p[2].x(), p[2].y())
	) / det;
	return;
}
void getLinearInterpolationPlane_3 (QPointF* p, qreal* coef) {
	const qreal det = determinant(
		.5, 0, 1,
		.5, .5, 1,
		1, 0, 1
	);
	coef[0] = determinant(
		function(p[0].x(), p[0].y()), 0, 1,
		function(p[1].x(), p[1].y()), .5, 1,
		function(p[2].x(), p[2].y()), 1, 1
	) / det;
	coef[1] = determinant(
		.5, function(p[0].x(), p[0].y()), 1,
		.5, function(p[1].x(), p[1].y()), 1,
		1, function(p[2].x(), p[2].y()), 1
	) / det;
	coef[2] = determinant(
		.5, 0, function(p[0].x(), p[0].y()),
		.5, .5, function(p[1].x(), p[1].y()),
		1, 0, function(p[2].x(), p[2].y())
	) / det;
	return;
}
void getLinearInterpolationPlane_4 (QPointF* p, qreal* coef) {
	const qreal det = determinant(
		0, .5, 1,
		.5, .5, 1,
		.5, 0, 1
	);
	coef[0] = determinant(
		function(p[0].x(), p[0].y()), .5, 1,
		function(p[1].x(), p[1].y()), .5, 1,
		function(p[2].x(), p[2].y()), 0, 1
	) / det;
	coef[1] = determinant(
		0, function(p[0].x(), p[0].y()), 1,
		.5, function(p[1].x(), p[1].y()), 1,
		.5, function(p[2].x(), p[2].y()), 1
	) / det;
	coef[2] = determinant(
		0, .5, function(p[0].x(), p[0].y()),
		.5, .5, function(p[1].x(), p[1].y()),
		.5, 0, function(p[2].x(), p[2].y())
	) / det;
	return;
}
