#ifndef YANTA_STROKE_POINT_H__
#define YANTA_STROKE_POINT_H__

struct StrokePoint {

	StrokePoint(
			util::point<double,2> position_,
			double pressure_,
			unsigned long timestamp_) :
		position(position_),
		pressure(pressure_),
		timestamp(timestamp_) {}

	util::point<double,2> position;
	double                pressure;
	unsigned long         timestamp;
};

#endif // YANTA_STROKE_POINT_H__

