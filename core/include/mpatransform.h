
#ifndef MPA_TRANSFORM_H
#define MPA_TRANSFORM_H

#include <Eigen/Dense>
#include <stdexcept>
#include "track.h"

namespace core {

/** \brief Performs various transforms and index translations for MPA type sensors
 *
 */
class MpaTransform
{
public:
	MpaTransform()
	{
		// Initialize normal vector and rotation matrix
		setRotation(0.0);
	}

	/// \brief Transform pixel index to world coordinates	
	Eigen::Vector3d transform(const size_t& pixelIdx) const
	{
		return pixelCoordToGlobal(translatePixelIndex(pixelIdx));
	}

	/// \brief Translates the pixel index to 2D pixel coordinates
	Eigen::Vector2i translatePixelIndex(const size_t& pixelIdx) const
	{
		if(pixelIdx > _numPixels(0)*_numPixels(1)) {
			throw std::out_of_range("Pixel index out of range");
		}
		int py = 2 - pixelIdx/16;
		return Eigen::Vector2i(
			((py==1)?15:0) + static_cast<unsigned int>(pixelIdx)%16 * ((py==1)?-1:1),
			py
		);
	}

	/** \brief Transforms pixel coordinates to world-space coordinates
	 *
	 * Takes geometry and positioning of the MPA into account.
	 */
	Eigen::Vector3d pixelCoordToGlobal(const Eigen::Vector2i& pixelCoord) const
	{
		return pixelCoordToGlobal(Eigen::Vector2d{pixelCoord.cast<double>()});
	}

	/** \brief Transforms pixel coordinates to world-space coordinates
	 *
	 * Takes geometry and positioning of the MPA into account. This is the generalized version which takes
	 * floating-point coordinates.
	 */
	Eigen::Vector3d pixelCoordToGlobal(const Eigen::Vector2d& pixelCoord) const
	{
		auto scaled = _sensitiveSize*(pixelCoord.array()
				/_numPixels.cast<double>() - 0.5) + _pixelSize/2;
		Eigen::Vector3d coord(scaled(0), scaled(1), 0.0);
		// todo: rotate
		coord += _offset;
		return coord;
	}

	Eigen::Vector3d mpaPlaneTrackIntersect(const Track& track, const size_t& a=0, const size_t& b=1) const
	{
		Eigen::Hyperplane<double, 3> plane(_normal, _offset);
		Eigen::ParametrizedLine<double, 3> line(track.points.at(a),
		                                     track.points.at(b) - track.points.at(a));
		auto t = line.intersectionParameter(plane);
		return line.pointAt(t);
	}

	size_t getPixelIndex(const Eigen::Vector3d& global) const
	{
		return pixelCoordToIndex(globalToPixelCoord(global).cast<int>());
	}

	size_t getPixelIndex(const Track& track, const size_t& a=0, const size_t& b=5) const
	{
		return pixelCoordToIndex(globalToPixelCoord(mpaPlaneTrackIntersect(track, a, b)).cast<int>());
	}

	size_t pixelCoordToIndex(const Eigen::Vector2i& local) const
	{
		if((local.array() < 0 || local.array() >= _numPixels).any()) {
			throw std::out_of_range("Hit is not in pixel plane");
		}
		if(local(1) == 1) {
			return 31 - local(0);
		} else if(local(1) == 0) {
			return 32 + local(0);
		} else if(local(1) == 2) {
			return local(0);
		}
		return 0; // todo implement further pixel arrangements...
	}

	Eigen::Vector2d globalToPixelCoord(const Eigen::Vector3d& global) const
	{
		auto local = global - _offset;
		// todo: un-rotate
		Eigen::Array2d pixelCoord(local(0), local(1));
		pixelCoord = (pixelCoord/_sensitiveSize + 0.5) * _numPixels.cast<double>();
		return pixelCoord.matrix();
	}

	void setRotation(const double& tilt)
	{
		_rotation = Eigen::AngleAxis<double>(tilt, Eigen::Vector3d::UnitX());
		_normal = _rotation * Eigen::Vector3d::UnitZ();
	}

	void setBaseOffset(const Eigen::Vector3d& offset)
	{
		_baseOffset = offset;
		_offset = _baseOffset + _alignmentOffset;
	}
	void setAlignmentOffset(const Eigen::Vector3d& offset)
	{
		_alignmentOffset = offset;
		_offset = _baseOffset + _alignmentOffset;
	}
	void setSensitiveSize(const Eigen::Array2d& sensitiveSize) { _sensitiveSize = sensitiveSize; }
	void setPixelSize(const Eigen::Array2d& pixelSize) { _pixelSize = pixelSize; }
	void setNumPixels(const Eigen::Array2i& numPixels) { _numPixels = numPixels; }

	Eigen::Vector3d getOffset() const { return _offset; }
	Eigen::Vector3d getBaseOffset() const { return _baseOffset; }
	Eigen::Vector3d getAlignemntOffset() const { return _alignmentOffset; }
	Eigen::Array2d getSensitiveSize() const { return _sensitiveSize; }
	Eigen::Array2d getPixelSize() const { return _pixelSize; }
	Eigen::Array2i getNumPixels() const { return _numPixels; }

private:
	Eigen::Array2d _sensitiveSize;
	Eigen::Array2i _numPixels;
	Eigen::Array2d _pixelSize;
	Eigen::Matrix3d _rotation;
	Eigen::Vector3d _normal;
	Eigen::Vector3d _offset;
	Eigen::Vector3d _baseOffset;
	Eigen::Vector3d _alignmentOffset;
};

} // namespace core

#endif
